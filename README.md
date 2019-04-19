# Loom Tour

19, Apr, 2019

Loom is a minimal job queue library that i use and helps me in my "Pure Coding" style.

You can communicate with me through e-mail moustapha.saad.abdelhamed@gmail.com

The library is licensed under BSD-3 and you can find it here [MN](https://github.com/MoustaphaSaad/loom)

Loom is divided into two main layers the group layer and loom layer, Let's get a quick tour of it's content.

## Group Layer
This layer is a more low level one where jobs are function pointers and usage is a bit verbose in nature. job creation usually will be stable after a while and will not incur any dynamic memory allocation since it reuses the old jobs in a form of a pool like allocator.

Let's have a look at the basic usage
```C++
//here i create a group with 3 worker threads
//if you pass 0 worker the group will make a worker for each cpu core you have
Group g = group_new("my group", 3);
//let's create an empty job function
auto empty_job = [](void* arg1, void* arg2) -> void {
	//do nothing
};

Job j = job_new(group_push_next(g), //this gets the right worker to push the job onto
	empty_job, //job function
	nullptr, //arg1 that will be passed to the job function
	nullptr, //arg2 that will be passed to the job function
	"job name", //name of the job
	nulltpr); //job parent

//now that we have created the job it's not scheduled yet
job_schedule(j);
//let's wait for the job to finish
job_wait(j);
//let's free the job, freeing the job actually adds it to a Garbage Collection for it to be recycled later
//and it's not recycled until it's done so you can actually skip the wait step and
//right free after the schedule for a fire and forget type of work
job_free(j);

//make sure to call gc function regularly to be able to recycle old finished jobs
group_gc(g);

group_free(g);
```

Jobs can be arranged in a tree-like structure this give it the ability to perform parallel for (fork/join) type of work
```C++
Group g = group_new("my group", 3);

//our dummy work function that just increments a variable
std::atomic<size_t> counter = 0;
auto increment = [](void* user_data, void* b) -> void {
	std::atomic<size_t>* self = (std::atomic<size_t>*)user_data;
	++(*self);
};

//create an empty job to act as the parent
Job parent = job_new(group_push_next(g), nullptr, nullptr, nullptr, "parent", nullptr);
for(size_t i = 0; i < 100; ++i)
{
	//[fork each small child job]
	//create a child job
	Job child = job_new(group_push_next(g), increment, &counter, nullptr, "child", parent);
	//schedule the child right away
	job_schedule(child);
	//and free it right after that since we don't care about any individual child job lifetime
	//we care about the parent that encapsulates all of them
	job_free(child);
}

//now schedule the parent job
job_schedule(parent);
//[join all the forked children jobs]
//and wait till it's done (this means that all the children jobs is done as well)
job_wait(parent);
//free it
job_free(parent);

//don't forget to free the group
group_free(g);
```

## Loom layer
This layer is a more high level and it accepts any callable C++ object, it's a bit expressive and it will incur a dynamic memory allocation if the callable object is big enough

```C++
//create a loom with 3 worker threads
Loom l = loom_new("L", 3);

//let's create an empty job function
auto empty_job = [](void* arg1, void* arg2) -> void {
	//do nothing
};

//let's create an async request which will be performed on one of the worker threads
Request* r = request_async(l, [empty_job](){
	empty_job(nullptr, nullptr);
});
//wait for it
request_wait(r);
//free it
request_free(r);

//you can perform the job in a sync mode yet the job is done on one of the worker threads
//think of this as just a combo of the async request followed by a wait and free
request_sync(l, [empty_job](){
	empty_job(nullptr, nullptr);
});

//don't forget to free the loom
loom_free(l);
```

Same parallel for kind of work
```C++
//create a loom with 3 worker threads
Loom l = loom_new("L", 3);

//create a request group for the same parallel for work
Request_Group parent = request_group_new(l);

std::atomic<size_t> counter = 0;
for(size_t i = 0; i < 100; ++i)
{
	if(i % 3 == 0)
	{
		request_sync(l, [&counter](){
			counter++;
		});
	}
	else
	{
		//note that we pass the request group to the async function
		//so it will be added as child and the child request will be freed(added to be garbage collected later) automatically
		request_async(l, parent, [&counter](){
			counter++;
		});
	}
}

//let's wait for the request group, the wait function also frees it automatically
request_group_wait(parent);

//note that unlike group_gc you don't need to explicitly call loom_gc here
//everytime you create an sync request it will decide whether it needs to do gc cycle and does it

loom_free(l);
```