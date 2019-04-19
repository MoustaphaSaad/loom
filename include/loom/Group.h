#pragma once

#include "loom/Exports.h"

#include <mn/Base.h>

namespace loom
{
	//Worker
	MS_HANDLE(Worker);
	MS_HANDLE(Group);

	API_LOOM Worker
	worker_new(const char* name, Group g);

	API_LOOM void
	worker_free(Worker worker);

	API_LOOM Worker
	worker_local();

	API_LOOM Worker
	worker_main();

	API_LOOM void
	worker_idle();

	inline static void
	destruct(Worker worker)
	{
		worker_free(worker);
	}

	API_LOOM void
	worker_gc(Worker worker);


	//Job
	MS_HANDLE(Job);
	using Job_Func = void(*)(void* arg1, void* arg2);

	API_LOOM Job
	job_new(Worker worker, Job_Func func, void* arg1, void* arg2, const char* name, Job parent);

	API_LOOM void
	job_free(Job job);

	inline static void
	destruct(Job job)
	{
		job_free(job);
	}

	API_LOOM Job
	job_schedule(Job job);

	API_LOOM bool
	job_done(Job job);

	API_LOOM void
	job_wait(Job job);


	//Group
	//if you pass 0 then it will use the cpu/core count
	API_LOOM Group
	group_new(const char* name, size_t worker_count);

	API_LOOM void
	group_free(Group group);

	inline static void
	destruct(Group group)
	{
		group_free(group);
	}

	API_LOOM void
	group_gc(Group group);

	API_LOOM Worker
	group_steal_next(Group group);

	API_LOOM Worker
	group_push_next(Group group);
}