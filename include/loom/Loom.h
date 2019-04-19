#pragma once

#include "loom/Group.h"

#include <mn/Memory.h>

#include <utility>
#include <atomic>
#include <new>

namespace loom
{
	constexpr static size_t SMALL_REQUEST_SIZE = 128;
	//Loom
	MS_HANDLE(Loom);

	API_LOOM Loom
	loom_new(const char* name, size_t worker_count);

	API_LOOM void
	loom_free(Loom loom);

	inline static void
	destruct(Loom loom)
	{
		loom_free(loom);
	}

	API_LOOM Group
	loom_group(Loom loom);

	API_LOOM void*
	loom_alloc(Loom loom);

	struct Request;

	API_LOOM void
	loom_free(Loom loom, Request* request);

	API_LOOM void
	loom_gc(Loom loom);

	//Request
	struct Request
	{
		Job job;
		Loom loom;
		bool small_req;
		std::atomic<bool> added_to_gc;

		virtual ~Request(){}
		virtual void run() = 0;
	};

	inline static void
	_request_run(void* request, void*)
	{
		Request* r = (Request*)request;
		r->run();
	}

	template<typename TProc>
	struct Lambda_Request final: Request
	{
		TProc proc;

		Lambda_Request(TProc&& p)
			:proc(std::forward<TProc>(p))
		{}

		void run() override { proc(); }
	};

	MS_HANDLE(Request_Group);

	inline static Request_Group
	request_group_new(Loom loom)
	{
		Group g = loom_group(loom);
		Job j = job_new(group_push_next(g), nullptr, nullptr, nullptr, "request group", nullptr);
		return (Request_Group)j;
	}

	inline static void
	request_group_free(Request_Group request_group)
	{
		Job self = (Job)request_group;
		job_free(self);
	}

	inline static void
	request_group_wait(Request_Group request_group)
	{
		Job self = (Job)request_group;
		job_schedule(self);
		job_wait(self);
		job_free(self);
	}

	template<typename TProc>
	inline static Request*
	request_async(Loom loom, TProc&& proc)
	{
		loom_gc(loom);

		Lambda_Request<TProc>* request = nullptr;
		if(sizeof(Lambda_Request<TProc>) <= SMALL_REQUEST_SIZE)
		{
			request = (Lambda_Request<TProc>*)loom_alloc(loom);
			::new (request) Lambda_Request<TProc>(std::forward<TProc>(proc));
			request->loom = loom;
			request->small_req = true;
		}
		else
		{
			request = mn::alloc<Lambda_Request<TProc>>();
			::new (request) Lambda_Request<TProc>(std::forward<TProc>(proc));
			request->loom = loom;
			request->small_req = false;
		}

		request->added_to_gc = false;
		Group g = loom_group(loom);
		request->job = job_new(group_push_next(g), _request_run, request, nullptr, "async request", nullptr);
		job_schedule(request->job);
		return request;
	}

	template<typename TProc>
	inline static void
	request_sync(Loom loom, TProc&& proc)
	{
		loom_gc(loom);

		Lambda_Request<TProc> request(std::forward<TProc>(proc));
		Group g = loom_group(loom);
		request.job = job_new(group_push_next(g), _request_run, &request, nullptr, "sync request", nullptr);
		job_schedule(request.job);
		job_wait(request.job);
		job_free(request.job);
	}

	inline static void
	request_free(Request* self)
	{
		loom_free(self->loom, self);
	}

	inline static void
	request_wait(Request* self)
	{
		job_wait(self->job);
	}

	inline static void
	request_done(Request* self)
	{
		job_done(self->job);
	}

	template<typename TProc>
	inline static void
	request_async(Loom loom, Request_Group group, TProc&& proc)
	{
		loom_gc(loom);

		Lambda_Request<TProc>* request = nullptr;
		if (sizeof(Lambda_Request<TProc>) <= SMALL_REQUEST_SIZE)
		{
			request = (Lambda_Request<TProc>*)loom_alloc(loom);
			::new (request) Lambda_Request<TProc>(std::forward<TProc>(proc));
			request->loom = loom;
			request->small_req = true;
		}
		else
		{
			request = mn::alloc<Lambda_Request<TProc>>();
			::new (request) Lambda_Request<TProc>(std::forward<TProc>(proc));
			request->loom = loom;
			request->small_req = false;
		}
		request->added_to_gc = false;
		Group g = loom_group(loom);
		request->job = job_new(group_push_next(g), _request_run, request, nullptr, "async request", (Job)group);
		job_schedule(request->job);
		request_free(request);
	}
}