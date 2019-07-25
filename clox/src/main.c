//
//  main.c
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "vm.h"



static void repl()
{
	char line[1024];

	for (;;)
	{
		printf("> ");

		if (!fgets(line, sizeof(line), stdin))
		{
			printf("\n");
			break;
		}

		if (strcmp(line, "quit()\n") == 0)
			break;

		interpret(line);
	}
}

static char * readFile(const char * path, size_t * sz)
{
	FILE * file = fopen(path, "rb");
	if (!file)
	{
		perror("Could not open file");
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	long length = ftell(file);

	if (length == EOF)
	{
		perror("Could not read file");
		fclose(file);
		exit(74);
	}

	rewind(file);

	// NOTE (matthewp) Not using xmalloc here because we don't want to use our internal allocator

	char * buffer = (char *)malloc(length + 1);
	if (!buffer)
	{
		errno = ENOMEM;
		perror("Could not read file");
		fclose(file);
		exit(74);
	}

	size_t read = fread(buffer, sizeof(char), length, file);

	if (read < (size_t)length)
	{
		fclose(file);
		free(buffer);
		perror("Could not read file");
		exit(74);
	}

	buffer[read] = '\0';

	fclose(file);
	*sz = length;

	return buffer;
}

static void runFile(const char * path)
{
	size_t sz;
	char * source = readFile(path, &sz);
	const char * interp = source;

	// Skip UTF8-BOM (if present)

	if (sz >= 3)
	{
		if (memcmp(source, "\xEF\xBB\xBF", 3) == 0)
		{
			interp += 3;
		}
	}

	InterpretResult result = interpret(interp);
	free(source);

	switch (result)
	{
	case INTERPRET_COMPILE_ERROR: exit(65);
	case INTERPRET_RUNTIME_ERROR: exit(70);
	default: break;
	}
}

int main(int argc, const char * argv[])
{
	initVM();

	if (argc == 1)
	{
		repl();
	}
	else if (argc == 2)
	{
		runFile(argv[1]);
	}
	else
	{
		fprintf(stderr, "Usage: clox [path]\n");
		exit(64);
	}

	freeVM();

	return 0;
}
