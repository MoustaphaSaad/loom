#pragma once

#include "loom/Loom.h"

#include <mn/Buf.h>

namespace loom
{
	constexpr static size_t BATCH_SIZE = 1<<16;

	template<typename T, typename TFunc>
	inline static void
	map(Loom l, T* begin, T* end, TFunc&& func)
	{
		assert(end >= begin);
		size_t count = end - begin;
		if(count < BATCH_SIZE)
		{
			for(T* it = begin; it != end; ++it)
				func(*it);
		}
		else
		{
			T* mid = begin + count / 2;
			Request* left = request_async(l, [l, begin, mid, func]{
				map(l, begin, mid, func);
			});
			Request* right = request_async(l, [l, mid, end, func]{
				map(l, mid, end, func);
			});

			request_wait(left);
			request_free(left);

			request_wait(right);
			request_free(right);
		}
	}

	template<typename T, typename TFunc>
	inline static void
	map(Loom l, mn::Buf<T>& array, TFunc&& func)
	{
		map(l, mn::buf_begin(array), mn::buf_end(array), func);
	}

	template<typename T, typename TFunc>
	inline static T
	reduce(Loom l, T* begin, T* end, T init, TFunc&& func)
	{
		assert(end >= begin);
		size_t count = end - begin;
		if(count < BATCH_SIZE)
		{
			for(T* it = begin; it != end; ++it)
				init = func(init, *it);
		}
		else
		{
			T* mid = begin + count / 2;
			T left_init = init;
			T right_init = init;
			Request* left = request_async(l, [l, begin, mid, func, &left_init]{
				left_init = reduce(l, begin, mid, left_init, func);
			});
			Request* right = request_async(l, [l, mid, end, func, &right_init]{
				right_init = reduce(l, mid, end, right_init, func);
			});

			request_wait(left);
			request_free(left);

			request_wait(right);
			request_free(right);

			init = func(left_init, right_init);
		}
		return init;
	}

	template<typename T, typename TFunc>
	inline static T
	reduce(Loom l, mn::Buf<T>& array, T init, TFunc&& func)
	{
		return reduce(l, mn::buf_begin(array), mn::buf_end(array), init, func);
	}
}