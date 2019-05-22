#pragma once

#include "loom/Exports.h"

#include <stddef.h>

namespace loom
{
	//Worker
	typedef struct IWorker* Worker;
	typedef struct IGroup* Group;

	LOOM_EXPORT Worker
	worker_new(const char* name, Group g);

	LOOM_EXPORT void
	worker_free(Worker worker);

	LOOM_EXPORT Worker
	worker_local();

	LOOM_EXPORT Worker
	worker_main();

	LOOM_EXPORT void
	worker_idle();

	inline static void
	destruct(Worker worker)
	{
		worker_free(worker);
	}

	LOOM_EXPORT void
	worker_gc(Worker worker);


	//Job
	typedef struct IJob* Job;
	using Job_Func = void(*)(void* arg1, void* arg2);

	LOOM_EXPORT Job
	job_new(Worker worker, Job_Func func, void* arg1, void* arg2, const char* name, Job parent);

	LOOM_EXPORT void
	job_free(Job job);

	inline static void
	destruct(Job job)
	{
		job_free(job);
	}

	LOOM_EXPORT Job
	job_schedule(Job job);

	LOOM_EXPORT bool
	job_done(Job job);

	LOOM_EXPORT void
	job_wait(Job job);


	//Group
	//if you pass 0 then it will use the cpu/core count
	LOOM_EXPORT Group
	group_new(const char* name, size_t worker_count);

	LOOM_EXPORT void
	group_free(Group group);

	inline static void
	destruct(Group group)
	{
		group_free(group);
	}

	LOOM_EXPORT void
	group_gc(Group group);

	LOOM_EXPORT Worker
	group_steal_next(Group group);

	LOOM_EXPORT Worker
	group_push_next(Group group);
}