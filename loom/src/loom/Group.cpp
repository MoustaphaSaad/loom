#include "loom/Group.h"

#include <mn/Ring.h>
#include <mn/Memory.h>
#include <mn/Thread.h>
#include <mn/Pool.h>
#include <mn/Buf.h>

#include <atomic>
#include <thread>

using namespace mn;

namespace loom
{
	constexpr static size_t JOB_POOL_SIZE = 32;

	struct IJob
	{
		Job_Func func;
		void* arg1;
		void* arg2;
		const char* name;
		Worker worker;
		Job parent;
		std::atomic<int> unfinished;
		std::atomic<bool> done;
		std::atomic<bool> added_to_gc;
	};

	struct IWorker
	{
		const char* name;

		Pool job_pool;
		Mutex job_pool_mtx;

		Ring<Job> q;
		Mutex q_mtx;

		Buf<Job> gc;
		Mutex gc_mtx;

		Thread thread;

		Group group;

		std::atomic<bool> group_ready;
		std::atomic<bool> is_running;
		std::atomic<bool> do_gc;
	};
	thread_local Worker LOCAL_WORKER = nullptr;

	struct IGroup
	{
		Buf<Worker> workers;
		std::atomic<uint32_t> last_steal;
		std::atomic<uint32_t> last_push;
	};


	struct Main_Worker_Wrapper
	{
		IWorker self;

		Main_Worker_Wrapper()
		{
			allocator_push(memory::clib());
				self.name = "Main Worker";
				self.job_pool = pool_new(sizeof(IJob), JOB_POOL_SIZE * 2);
				self.job_pool_mtx = mutex_new("Main Worker Job Pool Mutex");

				self.q = ring_new<IJob*>();
				self.q_mtx = mutex_new("Main Worker Queue Mutex");

				self.gc = buf_new<IJob*>();
				self.gc_mtx = mutex_new("Main Worker GC Mutex");

				self.group_ready = true;
				self.is_running = true;
				self.do_gc = false;

				self.group = nullptr;

				LOCAL_WORKER = &self;
			allocator_pop();
		}

		~Main_Worker_Wrapper()
		{
			allocator_push(memory::clib());
				self.is_running = false;
				pool_free(self.job_pool);
				mutex_free(self.job_pool_mtx);

				ring_free(self.q);
				mutex_free(self.q_mtx);

				buf_free(self.gc);
				mutex_free(self.gc_mtx);
			allocator_pop();
		}
	};



	inline static void
	_yield()
	{
		thread_sleep(1);
	}

	inline static void
	_job_finish(Job self)
	{
		assert(self->unfinished > 0);
		Job parent = self->parent;
		if(self->unfinished.fetch_sub(1) == 1)
		{
			assert(self->unfinished == 0);
			if(parent) _job_finish(parent);
			self->done = true;
		}
	}

	inline static void
	_job_do(Job self)
	{
		assert(self->done == false);
		if(self->func)
			self->func(self->arg1, self->arg2);
		_job_finish(self);
		mn::memory::tmp()->free_all();
	}

	inline static Job
	_worker_pop(Worker self)
	{
		Job res = nullptr;

		//try popping a job off the q
		mutex_lock(self->q_mtx);
			if(self->q.count)
			{
				res = ring_front(self->q);
				ring_pop_front(self->q);
			}
		mutex_unlock(self->q_mtx);

		if(res == nullptr && self->group)
		{
			Worker other = group_steal_next(self->group);
			if(other != self)
			{
				mutex_lock(other->q_mtx);
				if(other->q.count)
				{
					res = ring_back(other->q);
					ring_pop_back(other->q);
				}
				mutex_unlock(other->q_mtx);
			}
		}

		return res;
	}

	inline static void
	_worker_gc(Worker self)
	{
		mutex_lock(self->gc_mtx);
		if(self->gc.count >= JOB_POOL_SIZE)
		{
			mutex_lock(self->job_pool_mtx);
				buf_remove_if(self->gc, [self](Job job){
					assert(self == job->worker);
					if(job->done == true)
					{
						pool_put(self->job_pool, job);
						return true;
					}
					return false;
				});
			mutex_unlock(self->job_pool_mtx);
		}
		mutex_unlock(self->gc_mtx);

		self->do_gc = false;
	}

	inline static void
	_worker_idle(Worker self)
	{
		if (self->do_gc)
			_worker_gc(self);

		if(Job job = _worker_pop(self))
			_job_do(job);
		else
			_yield();
	}

	static void
	_worker_main(void* arg)
	{
		Worker self = (Worker)arg;
		LOCAL_WORKER = self;

		while(self->group_ready == false && self->group != nullptr)
			_yield();

		while(self->is_running)
			_worker_idle(self);

		self->is_running = false;
	}

	//API

	//Worker
	Worker
	worker_new(const char* name, Group g)
	{
		Worker self = alloc<IWorker>();
		self->name = name;

		self->job_pool = pool_new(sizeof(IJob), JOB_POOL_SIZE * 2);
		self->job_pool_mtx = mutex_new("Worker Job Pool Mutex");

		self->q = ring_new<Job>();
		self->q_mtx = mutex_new("Worker Queue Mutex");

		self->gc = buf_new<Job>();
		self->gc_mtx = mutex_new("Worker GC Mutex");

		self->group = g;

		self->group_ready = false;
		self->is_running = true;
		self->do_gc = false;

		self->thread = thread_new(_worker_main, self, name);

		return self;
	}

	inline static void
	_worker_stop(Worker self)
	{
		self->is_running = false;

		thread_join(self->thread);
		thread_free(self->thread);
	}

	inline static void
	_worker_free(Worker self)
	{
		pool_free(self->job_pool);
		mutex_free(self->job_pool_mtx);

		ring_free(self->q);
		mutex_free(self->q_mtx);

		buf_free(self->gc);
		mutex_free(self->gc_mtx);

		free(self);
	}

	void
	worker_free(Worker self)
	{
		_worker_stop(self);
		_worker_free(self);
	}

	Worker
	worker_local()
	{
		return LOCAL_WORKER;
	}

	Worker
	worker_main()
	{
		static Main_Worker_Wrapper MAIN_WORKER;
		return &MAIN_WORKER.self;
	}

	void
	worker_idle()
	{
		if(LOCAL_WORKER)
			_worker_idle(LOCAL_WORKER);
		else
			_yield();
	}

	void
	worker_gc(Worker self)
	{
		self->do_gc = true;
	}


	//Job
	Job
	job_new(Worker self, Job_Func func, void* arg1, void* arg2, const char* name, Job parent)
	{
		mutex_lock(self->job_pool_mtx);
			Job res = (Job)pool_get(self->job_pool);
		mutex_unlock(self->job_pool_mtx);

		res->func = func;
		res->arg1 = arg1;
		res->arg2 = arg2;
		res->name = name;
		res->worker = self;
		res->parent = parent;
		if(res->parent) ++res->parent->unfinished;
		res->unfinished = 1;
		res->done = false;
		res->added_to_gc = false;

		return res;
	}

	void
	job_free(Job self)
	{
		if (self->added_to_gc.exchange(true) == false)
		{
			mutex_lock(self->worker->gc_mtx);
				buf_push(self->worker->gc, self);
			mutex_unlock(self->worker->gc_mtx);
		}
	}

	Job
	job_schedule(Job self)
	{
		mutex_lock(self->worker->q_mtx);
			ring_push_back(self->worker->q, self);
		mutex_unlock(self->worker->q_mtx);
		return self;
	}

	bool
	job_done(Job self)
	{
		return self->done;
	}

	void
	job_wait(Job self)
	{
		if(LOCAL_WORKER)
		{
			while(self->done == false)
				_worker_idle(LOCAL_WORKER);
		}
		else
		{
			while(self->done == false)
				_yield();
		}
	}


	//Group
	Group
	group_new(const char* name, size_t worker_count)
	{
		if(worker_count == 0)
			worker_count = std::thread::hardware_concurrency();

		Group self = alloc<IGroup>();
		self->workers = buf_new<Worker>();
		self->last_push = 0;
		self->last_steal = 0;

		for(size_t i = 0; i < worker_count; ++i)
			buf_push(self->workers, worker_new(name, self));

		for (size_t i = 0; i < worker_count; ++i)
			self->workers[i]->group_ready = true;

		return self;
	}

	void
	group_free(Group self)
	{
		for(Worker worker: self->workers)
			_worker_stop(worker);

		for(Worker worker: self->workers)
			_worker_free(worker);

		buf_free(self->workers);

		free(self);
	}

	void
	group_gc(Group self)
	{
		for(Worker worker: self->workers)
			worker_gc(worker);
	}

	Worker
	group_steal_next(Group self)
	{
		uint32_t ix = self->last_steal.fetch_add(1) % self->workers.count;
		return self->workers[ix];
	}

	Worker
	group_push_next(Group self)
	{
		uint32_t ix = self->last_push.fetch_add(1) % self->workers.count;
		return self->workers[ix];
	}
}