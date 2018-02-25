//
//  common.c
//  clox
//
//  Created by Matthew Pohlmann on 2/25/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "common.h"

#include <stdio.h>
#include <stdarg.h>



void DoAssert(const char * file, int line, const char * function, const char * format, ...)
{
	char buf[1024];

	va_list args;
	va_start(args, format);

	vsnprintf(buf, sizeof(buf), format, args);

	va_end(args);

	fprintf(
		stderr,
		"ASSERTION FAILED: \"%s\"\n"
		"\tFile: %s\n"
		"\tLine: %d\n"
		"\tFunction: %s\n",
		buf, file, line, function);
}
