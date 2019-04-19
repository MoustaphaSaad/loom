#pragma once

#if defined(OS_WINDOWS)
	#if defined(LOOM_DLL)
		#define API_LOOM __declspec(dllexport)
	#else
		#define API_LOOM __declspec(dllimport)
	#endif
#elif defined(OS_LINUX)
	#define API_LOOM 
#endif