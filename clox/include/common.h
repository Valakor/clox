//
//  common.h
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>



#ifdef _MSC_VER
#define PRINTF_LIKE(...)
#define LIKELY(_f) (_f)
#define UNLIKELY(_f) (_f)
#define FUNCTION_PRETTY __FUNCSIG__
#define TARGET_WINDOWS 1
#define TARGET_MAC 0
#else
#define PRINTF_LIKE(_iFormat, _iArgs) __attribute__((format(printf, _iFormat, _iArgs)))
#define LIKELY(_f) __builtin_expect(!!(_f), 1)
#define UNLIKELY(_f) __builtin_expect(!!(_f), 0)
#define FUNCTION_PRETTY __PRETTY_FUNCTION__
#define TARGET_WINDOWS 0
#define TARGET_MAC 1
#endif

#if DEBUG || _DEBUG
#if TARGET_WINDOWS
	#define DEBUG_BREAK() __debugbreak()
#else
	#define DEBUG_BREAK() __builtin_debugtrap()
#endif

	void DoAssert(const char * file, int line, const char * function, const char * format, ...) PRINTF_LIKE(4, 5);

	#define ASSERT_MACRO(_f, _format, ...) \
		(void)(LIKELY(!!(_f)) || (DoAssert(__FILE__, __LINE__, FUNCTION_PRETTY, _format, ##__VA_ARGS__), DEBUG_BREAK(), 0))

	#define ASSERT(_f) ASSERT_MACRO(_f, #_f)
	#define ASSERTMSG(_f, _format, ...) ASSERT_MACRO(_f, _format, ##__VA_ARGS__)
#else
	#define DEBUG_BREAK() (void)0

	#define ASSERT(...) (void)0
	#define ASSERTMSG(...) (void)0
#endif

#ifndef DEBUG_TRACE_EXECUTION
#define DEBUG_TRACE_EXECUTION 0
#endif

#ifndef DEBUG_PRINT_CODE
#define DEBUG_PRINT_CODE 0
#endif

#ifndef DEBUG_STRESS_GC
#define DEBUG_STRESS_GC 0
#endif

#ifndef DEBUG_LOG_GC
#define DEBUG_LOG_GC 0
#endif

#ifndef DEBUG_ALLOC
#define DEBUG_ALLOC (DEBUG || _DEBUG)
#endif

#define CASSERT(_f) static_assert(_f, #_f)
#define CASSERTMSG(_f, _msg) static_assert(_f, _msg)
#define UNUSED(_x) (void)(_x)

#define MIN(_a, _b) ((_a) <= (_b) ? (_a) : (_b))
#define MAX(_a, _b) ((_a) >= (_b) ? (_a) : (_b))

#define UINT8_COUNT (UINT8_MAX + 1U)

#define UINT24_MAX 16777215U
#define UINT24_COUNT (UINT24_MAX + 1U)

#define IS_POW2(_n) ((_n) && (((_n) & ((_n) - 1)) == 0))
