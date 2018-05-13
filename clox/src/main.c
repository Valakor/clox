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

#include "vm.h"
#include "chunk.h"
#include "debug.h"
#include "array.h"



void printInstructionRanges(Chunk * chunk)
{
	InstructionRange * aryInstrange = chunk->aryInstrange;

	for (int i = 0; i < ARY_LEN(aryInstrange); i++)
	{
		InstructionRange range = aryInstrange[i];

		printf("%4d: [%d-%d)\n", range.line, range.instructionMic, range.instructionMac);
	}
}

int main(int argc, const char * argv[])
{
	(void)argc;
	(void)argv;

	initVM();

	Chunk chunk;
	initChunk(&chunk);

	writeConstant(&chunk, 1.2, 123);
	writeConstant(&chunk, 3.4, 123);

	writeChunk(&chunk, OP_ADD, 123);

	writeConstant(&chunk, 5.6, 123);

	writeChunk(&chunk, OP_DIVIDE, 123);
	writeChunk(&chunk, OP_NEGATE, 123);
	
	writeChunk(&chunk, OP_RETURN, 125);

	disassembleChunk(&chunk, "test chunk");

	interpret(&chunk);

	freeVM();
	freeChunk(&chunk);

	return 0;
}
