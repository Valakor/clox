//
//  main.c
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "common.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "chunk.h"
#include "debug.h"



void printInstructionRanges(Chunk * chunk)
{
	Lines * lines = &chunk->lines;

	for (int i = 0; i < lines->count; i++)
	{
		InstructionRange range = lines->ranges[i];

		printf("%4d: [%d-%d)\n", range.line, range.instructionMic, range.instructionMac);
	}
}

int main(int argc, const char * argv[])
{
	srand(0);

	Chunk chunk;
	initChunk(&chunk);

	writeConstant(&chunk, 1.2, 122);

	for (int i = 0; i < UINT8_MAX + 1; i++)
	{
		Value value = (Value)rand() / RAND_MAX;
		writeConstant(&chunk, value, 123);
	}

	writeChunk(&chunk, OP_RETURN, 125);

	disassembleChunk(&chunk, "test chunk");

	printf("\n");
	printInstructionRanges(&chunk);

	freeChunk(&chunk);

	return 0;
}
