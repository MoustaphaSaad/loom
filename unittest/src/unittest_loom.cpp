#include <doctest/doctest.h>

#include <loom/Group.h>
#include <loom/Loom.h>
#include <loom/MapReduce.h>

#include <mn/Buf.h>
#include <mn/IO.h>
#include <mn/Thread.h>

#include <atomic>
#include <chrono>
#include <stdint.h>

using namespace loom;
using namespace mn;

TEST_CASE("worker creation and destruction")
{
	Worker w1 = worker_new("1", nullptr);
	worker_free(w1);
}

TEST_CASE("group simple atomic increment")
{
	Group g = group_new("G", 3);

	std::atomic<size_t> counter = 0;
	auto increment = [](void* user_data, void* b) -> void {
		std::atomic<size_t>* self = (std::atomic<size_t>*)user_data;
		++(*self);
	};

	Job p = job_new(group_push_next(g), nullptr, nullptr, nullptr, "parent", nullptr);
	for(size_t i = 0; i < 100; ++i)
	{
		Job c = job_new(group_push_next(g), increment, &counter, nullptr, "child", p);
		job_schedule(c);
		job_free(c);
	}

	job_schedule(p);
	job_wait(p);
	job_free(p);

	CHECK(counter == 100);

	group_free(g);
}

TEST_CASE("loom simple atomic increment")
{
	Loom l = loom_new("L", 3);

	Buf<Request*> reqs = buf_new<Request*>();

	std::atomic<size_t> counter = 0;
	for(size_t i = 0; i < 100; ++i)
	{
		if(i % 3 == 0)
		{
			request_sync(l, [&counter](Request*){
				counter++;
			});
		}
		else
		{
			buf_push(reqs, request_async(l, [&counter](Request*){
				counter++;
			}));
		}
	}

	for(Request* r: reqs)
	{
		request_wait(r);
		request_free(r);
	}

	CHECK(counter == 100);

	buf_free(reqs);
	loom_free(l);
}

TEST_CASE("loom map increment array elements")
{
	Loom l = loom_new("L", 3);

	Buf<int> nums = buf_new<int>();
	for(int i = 0; i < 100000; ++i)
		buf_push(nums, i);

	map(l, nums, [](int& num){
		++num;
	});

	for(int i = 0; i < 1000; ++i)
		CHECK(nums[i] == i + 1);

	buf_free(nums);

	loom_free(l);
}

TEST_CASE("loom reduce array sum")
{
	Loom l = loom_new("L", 3);

	Buf<int64_t> nums = buf_new<int64_t>();
	for(int64_t i = 1; i <= 100000; ++i)
		buf_push(nums, i);

	int64_t res = reduce(l, nums, int64_t(0), [](int64_t a, int64_t b){return a + b;});

	CHECK(res == 5000050000);

	buf_free(nums);

	loom_free(l);
}


void sleep_fn(void* user_data, void* b)
{
	thread_sleep(10);
};

TEST_CASE("multiple group interaction case")
{
	Group single = group_new("G", 1);
	Group multi = group_new("G", 3);

	auto gc_fn = [](void* user_data, void* b) -> void
	{
		Group single = (Group)user_data;
		Job j = job_new(group_push_next(single), sleep_fn, nullptr, nullptr, "sleep", nullptr);

		job_schedule(j);
		job_free(j);

		group_gc(single);
	};

	Job p = job_new(group_push_next(multi), nullptr, nullptr, nullptr, "parent", nullptr);
	for(size_t i = 0; i < 1000; ++i)
	{
		Job c = job_new(group_push_next(multi), gc_fn, single, nullptr, "child", p);
		job_schedule(c);
		job_free(c);
	}

	job_schedule(p);
	job_wait(p);
	job_free(p);

	group_free(multi);
	group_free(single);
}

TEST_CASE("main loom simple atomic increment")
{
	Buf<Request*> reqs = buf_new<Request*>();
	std::atomic<size_t> counter = 0;
	for (size_t i = 0; i < 100; ++i)
	{
		if (i % 3 == 0)
		{
			request_sync(loom_main(), [&counter](Request*) {
				counter++;
			});
		}
		else
		{
			buf_push(reqs, request_async(loom_main(), [&counter](Request*) {
				counter++;
			}));
		}
	}

	for (Request* r : reqs)
	{
		request_wait(r);
		request_free(r);
	}

	CHECK(counter == 100);

	buf_free(reqs);
}

TEST_CASE("main loom reduce array sum")
{
	Buf<int64_t> nums = buf_new<int64_t>();
	for (int64_t i = 1; i <= 100000; ++i)
		buf_push(nums, i);

	int64_t res = reduce(loom_main(), nums, int64_t(0),
		[](int64_t a, int64_t b) {return a + b; });

	CHECK(res == 5000050000);

	buf_free(nums);
}
