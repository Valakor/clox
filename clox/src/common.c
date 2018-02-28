//
//  common.c
//  clox
//
//  Created by Matthew Pohlmann on 2/25/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <execinfo.h>



void DoAssert(const char * file, int line, const char * function, const char * format, ...)
{
	char buf[2048];

	va_list args;
	va_start(args, format);

	vsnprintf(buf, sizeof(buf), format, args);

	va_end(args);

	fprintf(
		stderr,
		"\nASSERTION FAILED: \"%s\"\n"
		"\tFile: %s\n"
		"\tLine: %d\n"
		"\tFunction: %s\n",
		buf, file, line, function);

	// Print stack frame

	const int maxFrames = 5;
	void * callstack[maxFrames];

	int frames = backtrace(callstack, maxFrames);
	char ** aStr = backtrace_symbols(callstack, frames);

	// Skip frame 0 (the DoAssert call)

	for (int i = 1; i < frames; i++)
	{
		fprintf(stderr, "%s\n", aStr[i]);
	}

	free(aStr);
}
