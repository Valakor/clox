//
//  common.h
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#ifndef common_h
#define common_h

#include <stdbool.h>
#include <stdint.h>



#define DEBUG_TRACE_EXECUTION 1

#ifdef _MSC_VER
#define PRINTF_LIKE(iFormat, iArgs)
#define LIKELY(f) (f)
#define UNLIKELY(f) (f)
#define FUNCTION_PRETTY __FUNCSIG__
#else
#define PRINTF_LIKE(iFormat, iArgs) __attribute__((format(printf, iFormat, iArgs)))
#define LIKELY(f) __builtin_expect(!!(f), 1)
#define UNLIKELY(f) __builtin_expect(!!(f), 0)
#define FUNCTION_PRETTY __PRETTY_FUNCTION__
#endif

#if DEBUG || _DEBUG
#ifdef _MSC_VER
	#define DEBUG_BREAK() __debugbreak()
#else
	#define DEBUG_BREAK() __builtin_debugtrap()
#endif

	void DoAssert(const char * file, int line, const char * function, const char * format, ...) PRINTF_LIKE(4, 5);

	#define ASSERT_MACRO(f, format, ...) \
	do { \
		if (UNLIKELY(!(f))) { \
			DoAssert(__FILE__, __LINE__, FUNCTION_PRETTY, format, ##__VA_ARGS__); \
			DEBUG_BREAK(); \
		} \
	} while(0)

	#define ASSERT(f) ASSERT_MACRO(f, #f)
	#define ASSERTMSG(f, format, ...) ASSERT_MACRO(f, format, ##__VA_ARGS__)
#else
	#define DEBUG_BREAK() (void)0

	#define ASSERT(...) (void)0
	#define ASSERTMSG(...) (void)0
#endif

#endif /* common_h */
