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



#define STACK_CAPTURE_DEPTH 6
#define STACK_CAPTURE_SKIP_DEPTH 2

#if TARGET_WINDOWS
#include <Windows.h>
#include <DbgHelp.h>

static void PrintStack(void)
{
	// BB (matthewp) This function is not thread-safe, except for CaptureStackBackTrace()

	// See: http://blog.aaronballman.com/2011/04/generating-a-stack-crawl/

	HANDLE process = GetCurrentProcess();

	// Set up the symbol options so that we can gather information from the current
	// executable's PDB files. We also want to undecorate the symbol names we're returned.
	// If you want, you can add other symbol servers or paths via a semi-colon separated list
	// in SymInitialized.

	static bool fInit = false;
	static bool fInitSuccess = false;

	if (!fInit)
	{
		SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_INCLUDE_32BIT_MODULES);
		fInitSuccess = SymInitialize(process, NULL, TRUE);
		fInit = true;
	}

	if (!fInitSuccess)
		return;

	// Capture up to STACK_CAPTURE_DEPTH stack frames from the current call stack. We're
	// going to skip the first two stack frames returned because they're the PrintStack and
	// DoAssert functions themselves, which we don't care about.
	PVOID addrs[STACK_CAPTURE_DEPTH] = { 0 };
	USHORT frames = CaptureStackBackTrace(STACK_CAPTURE_SKIP_DEPTH, STACK_CAPTURE_DEPTH, addrs, NULL);

	for (USHORT i = 0; i < frames; i++) {
		// Allocate a buffer large enough to hold the symbol information on the stack and get 
		// a pointer to the buffer. We also have to set the size of the symbol structure itself
		// and the number of bytes reserved for the name.
		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(CHAR)] = { 0 };

		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = MAX_SYM_NAME;

		// Attempt to get information about the symbol and add it to our output parameter.
		DWORD64 displacement = 0;
		if (!SymFromAddr(process, (DWORD64)addrs[i], &displacement, pSymbol))
			continue;

		char symbolNameUnDecorated[MAX_SYM_NAME] = { 0 };
		const char* symbolName = pSymbol->Name;
		if (symbolName[0] == '?' && UnDecorateSymbolName(symbolName, symbolNameUnDecorated, MAX_SYM_NAME, UNDNAME_COMPLETE | UNDNAME_32_BIT_DECODE) != 0)
		{
			// Undecorate C++ symbols (which always start with a ?)

			symbolName = symbolNameUnDecorated;
		}

		IMAGEHLP_MODULE64 module = { 0 };
		module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

		SymGetModuleInfo64(process, (DWORD64)addrs[i], &module);

		const char* moduleName = module.ModuleName;
		const char* imageName = strrchr(module.ImageName, '\\');
		if (imageName)
		{
			moduleName = imageName + 1;
		}

		PrintErr("%-3d %-35s 0x%p %s + %lld\n", i, moduleName, addrs[i], symbolName, displacement);
	}
}

#else
#include <execinfo.h>

static void PrintStack(void)
{
	// BB (matthewp) Not safe to call from a signal handler because backtrace_symbols calls malloc()

	void * callstack[STACK_CAPTURE_DEPTH + STACK_CAPTURE_SKIP_DEPTH];

	int frames = backtrace(callstack, STACK_CAPTURE_DEPTH + STACK_CAPTURE_SKIP_DEPTH);
	char ** aStr = backtrace_symbols(callstack + STACK_CAPTURE_SKIP_DEPTH, frames - STACK_CAPTURE_SKIP_DEPTH);

	for (int i = 0; i < frames - STACK_CAPTURE_SKIP_DEPTH; i++)
	{
		PrintErr("%s\n", aStr[i]);
	}

	free(aStr);
}

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

#if TARGET_WINDOWS
	if (IsDebuggerPresent())
	{
		OutputDebugStringA(buf);
	}
#endif
}

#endif
