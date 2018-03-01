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



#if DEBUG || _DEBUG

static void PrintErr(const char * format, ...) PRINTF_LIKE(1, 2);



#define STACK_CAPTURE_DEPTH 4

#ifdef _MSC_VER
#include <Windows.h>
#include <DbgHelp.h>

static void PrintStack()
{
	// See: http://blog.aaronballman.com/2011/04/generating-a-stack-crawl/

	HANDLE process = GetCurrentProcess();

	// Set up the symbol options so that we can gather information from the current
	// executable's PDB files, as well as the Microsoft symbol servers.  We also want
	// to undecorate the symbol names we're returned.  If you want, you can add other
	// symbol servers or paths via a semi-colon separated list in SymInitialized.
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_INCLUDE_32BIT_MODULES | SYMOPT_UNDNAME);
	if (!SymInitialize(process, NULL, TRUE)) return;

	// Capture up to 25 stack frames from the current call stack.  We're going to
	// skip the first stack frame returned because that's the GetStackWalk function
	// itself, which we don't care about.
	PVOID addrs[STACK_CAPTURE_DEPTH] = { 0 };
	USHORT frames = CaptureStackBackTrace(2, STACK_CAPTURE_DEPTH, addrs, NULL);

	for (USHORT i = 0; i < frames; i++) {
		// Allocate a buffer large enough to hold the symbol information on the stack and get 
		// a pointer to the buffer.  We also have to set the size of the symbol structure itself
		// and the number of bytes reserved for the name.
		char buffer[sizeof(SYMBOL_INFO) + 1024 * sizeof(TCHAR)];
		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = 1024;

		// Attempt to get information about the symbol and add it to our output parameter.
		DWORD64 displacement = 0;
		if (!SymFromAddr(process, (DWORD64)addrs[i], &displacement, pSymbol))
			continue;

		IMAGEHLP_MODULE64 module;
		module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

		SymGetModuleInfo64(process, (DWORD64)addrs[i], &module);

		PrintErr("%-3d %-35s 0x%p %s + %lld\n", i, module.ModuleName, addrs[i], pSymbol->Name, displacement);
	}

	SymCleanup(GetCurrentProcess());
}

#else
#include <execinfo.h>

#define PrintStack() \
	do { \
		void * callstack[STACK_CAPTURE_DEPTH + 1]; \
		int frames = backtrace(callstack, STACK_CAPTURE_DEPTH + 1); \
		char ** aStr = backtrace_symbols(callstack, frames); \
		for (int i = 1; i < frames; i++) \
			PrintErr("%s\n", aStr[i]); \
		free(aStr); \
	} while(0)
#endif



void DoAssert(const char * file, int line, const char * function, const char * format, ...)
{
	char buf[2048];

	va_list args;
	va_start(args, format);

	vsnprintf(buf, sizeof(buf), format, args);

	va_end(args);

	PrintErr(
		"\nASSERTION FAILED: \"%s\"\n"
		"    File: %s\n"
		"    Line: %d\n"
		"    Function: %s\n",
		buf, file, line, function);

	// Print stack frame

	PrintStack();
}

static void PrintErr(const char * format, ...)
{
	char buf[2048];

	va_list args;
	va_start(args, format);

	vsnprintf(buf, sizeof(buf), format, args);

	va_end(args);

	fprintf(stderr, "%s", buf);

#ifdef _MSC_VER
	if (IsDebuggerPresent())
	{
		OutputDebugStringA(buf);
	}
#endif
}

#endif
