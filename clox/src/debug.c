//
//  debug.c
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "debug.h"

#include <stdio.h>

#include "value.h"
#include "array.h"

void disassembleChunk(Chunk * chunk, const char * name)
{
	printf("== %s ==\n", name);

	for (int i = 0; i < ARY_LEN(chunk->aryB);)
	{
		i = disassembleInstruction(chunk, i);
	}
}

static int constantInstruction(const char * name, Chunk * chunk, int offset, bool isLong)
{
	int constant;
	int constantBytes;

	if (isLong)
	{
		constantBytes = 3;

		ASSERT(offset + 1 + constantBytes <= ARY_LEN(chunk->aryB));

		constant  = chunk->aryB[offset + 1];
		constant  = constant << 8;
		constant |= chunk->aryB[offset + 2];
		constant  = constant << 8;
		constant |= chunk->aryB[offset + 3];
	}
	else
	{
		constantBytes = 1;

		ASSERT(offset + 1 + constantBytes <= ARY_LEN(chunk->aryB));

		constant = chunk->aryB[offset + 1];
	}

	ASSERT(constant < chunk->constants.count);

	printf("%-16s %4d '", name, constant);
	printValue(chunk->constants.values[constant]);
	printf("'\n");

	return offset + 1 + constantBytes;
}

static int simpleInstruction(const char * name, int offset)
{
	printf("%s\n", name);
	return offset + 1;
}

int disassembleInstruction(Chunk * chunk, int offset)
{
	ASSERT(offset < ARY_LEN(chunk->aryB));

	printf("%04d ", offset);

	int line = getLine(chunk, offset);

	if (offset > 0 && line == getLine(chunk, offset - 1))
	{
		printf("   | ");
	}
	else
	{
		printf("%4d ", line);
	}

	uint8_t instruction = chunk->aryB[offset];
	switch (instruction)
	{
		case OP_CONSTANT:
			return constantInstruction("OP_CONSTANT", chunk, offset, false);
		case OP_CONSTANT_LONG:
			return constantInstruction("OP_CONSTANT_LONG", chunk, offset, true);
		case OP_NEGATE:
			return simpleInstruction("OP_NEGATE", offset);
		case OP_ADD:
			return simpleInstruction("OP_ADD", offset);
		case OP_SUBTRACT:
			return simpleInstruction("OP_SUBTRACT", offset);
		case OP_MULTIPLY:
			return simpleInstruction("OP_MULTIPLY", offset);
		case OP_DIVIDE:
			return simpleInstruction("OP_DIVIDE", offset);
		case OP_RETURN:
			return simpleInstruction("OP_RETURN", offset);
		default:
			printf("Unknown opcode %d\n", instruction);
			return offset + 1;
	}
}
