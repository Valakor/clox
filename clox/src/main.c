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



int main(int argc, const char * argv[])
{
	srand(0);

	Chunk chunk;
	initChunk(&chunk);

	writeConstant(&chunk, 1.2, 123);

	for (int i = 0; i < UINT8_MAX + 1; i++)
	{
		Value value = (Value)rand() / RAND_MAX;
		writeConstant(&chunk, value, 123);
	}

	writeChunk(&chunk, OP_RETURN, 123);

	disassembleChunk(&chunk, "test chunk");

	freeChunk(&chunk);

	return 0;
}
