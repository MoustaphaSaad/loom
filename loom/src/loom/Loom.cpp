#include "loom/Loom.h"

#include <mn/Memory.h>
#include <mn/Pool.h>
#include <mn/Thread.h>
#include <mn/Buf.h>

using namespace mn;

namespace loom
{
	constexpr static size_t REQUEST_POOL_SIZE = 1024;

	struct ILoom
	{
		Group group;
		Pool pool;
		Mutex pool_mtx;
		Buf<Request*> gc;
		Mutex gc_mtx;
	};

	struct Main_Loom_Wrapper
	{
		ILoom self;

		Main_Loom_Wrapper()
		{
			allocator_push(memory::clib());
				self.group = group_new("Main Loom", 0);

				self.pool = pool_new(SMALL_REQUEST_SIZE, REQUEST_POOL_SIZE * 2);
				self.pool_mtx = mutex_new("Main Loom request pool mutex");

				self.gc = buf_new<Request*>();
				self.gc_mtx = mutex_new("Main Loom GC Mutex");
			allocator_pop();
		}

		~Main_Loom_Wrapper()
		{
			allocator_push(memory::clib());
				group_free(self.group);

				for (Request* r : self.gc)
					if (r->small_req == false)
						mn::free(r);

				pool_free(self.pool);
				mutex_free(self.pool_mtx);

				buf_free(self.gc);
				mutex_free(self.gc_mtx);
			allocator_pop();
		}
	};
	static Main_Loom_Wrapper MAIN_LOOM;

	Loom
	loom_new(const char* name, size_t worker_count)
	{
		Loom self = alloc<ILoom>();
		self->group = group_new(name, worker_count);
		self->pool = pool_new(SMALL_REQUEST_SIZE, REQUEST_POOL_SIZE * 2);
		self->pool_mtx = mutex_new(name);
		self->gc = buf_new<Request*>();
		self->gc_mtx = mutex_new(name);
		return self;
	}

	void
	loom_free(Loom self)
	{
		group_free(self->group);

		for(Request* r: self->gc)
			if(r->small_req == false)
				mn::free(r);

		pool_free(self->pool);
		mutex_free(self->pool_mtx);

		buf_free(self->gc);
		mutex_free(self->gc_mtx);

		free(self);
	}

	Loom
	loom_main()
	{
		return &MAIN_LOOM.self;
	}

	Group
	loom_group(Loom self)
	{
		return self->group;
	}

	void*
	loom_alloc(Loom self)
	{
		mutex_lock(self->pool_mtx);
			void* res = pool_get(self->pool);
		mutex_unlock(self->pool_mtx);
		return res;
	}

	void
	loom_free(Loom self, Request* request)
	{
		if (request->added_to_gc.exchange(true) == false)
		{
			mutex_lock(self->gc_mtx);
				buf_push(self->gc, request);
			mutex_unlock(self->gc_mtx);
		}
	}

	void
	loom_gc(Loom self)
	{
		mutex_lock(self->gc_mtx);
		if(self->gc.count >= REQUEST_POOL_SIZE)
		{
			buf_remove_if(self->gc, [self](Request* r){
				if(job_done(r->job) == true)
				{
					job_free(r->job);
					r->~Request();

					if(r->small_req)
					{
						mutex_lock(self->pool_mtx);
							pool_put(self->pool, r);
						mutex_unlock(self->pool_mtx);
					}
					else
					{
						free(r);
					}

					return true;
				}
				return false;
			});
		}
		mutex_unlock(self->gc_mtx);
		group_gc(self->group);
	}
}
