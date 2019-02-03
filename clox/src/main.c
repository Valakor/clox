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

static char * readFile(const char * path)
{
	FILE * file = fopen(path, "rb");
	if (!file)
	{
		fprintf(stderr, "Could not open file '%s'\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	size_t length = ftell(file);
	rewind(file);

	// NOTE (matthewp) Not using xmalloc here because we don't want to use our internal allocator

	char * buffer = (char *)malloc(length + 1);
	if (!buffer)
	{
		fprintf(stderr, "Not enough memory to read file '%s'\n", path);
		exit(74);
	}

	size_t read = fread(buffer, sizeof(char), length, file);

	if (read < length)
	{
		free(buffer);
		fprintf(stderr, "Could not read file '%s'\n", path);
		exit(74);
	}

	buffer[read] = '\0';

	fclose(file);
	return buffer;
}

static void runFile(const char * path)
{
	char * source = readFile(path);
	InterpretResult result = interpret(source);
	free(source);

	if (result == INTERPRET_COMPILE_ERROR) exit(65);
	if (result == INTERPRET_RUNTIME_ERROR) exit(70);
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
