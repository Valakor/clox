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

	for (unsigned i = 0; i < ARY_LEN(chunk->aryB);)
	{
		i = disassembleInstruction(chunk, i);
	}
}

static unsigned constantInstruction(const char * name, Chunk * chunk, unsigned offset, bool isLong)
{
	unsigned constant;
	unsigned constantBytes;

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

	ASSERT(constant < ARY_LEN(chunk->aryValConstants));

	printf("%-16s %4u '", name, constant);
	printValue(chunk->aryValConstants[constant]);
	printf("'\n");

	return offset + 1 + constantBytes;
}

static unsigned simpleInstruction(const char * name, unsigned offset)
{
	printf("%s\n", name);
	return offset + 1;
}

unsigned disassembleInstruction(Chunk * chunk, unsigned offset)
{
	ASSERT(offset < ARY_LEN(chunk->aryB));

	printf("%04d ", offset);

	unsigned line = getLine(chunk, offset);

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
		case OP_NIL:
			return simpleInstruction("OP_NIL", offset);
		case OP_TRUE:
			return simpleInstruction("OP_TRUE", offset);
		case OP_FALSE:
			return simpleInstruction("OP_FALSE", offset);
		case OP_POP:
			return simpleInstruction("OP_POP", offset);
		case OP_GET_GLOBAL:
			return constantInstruction("OP_GET_GLOBAL", chunk, offset, false);
		case OP_DEFINE_GLOBAL:
			return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset, false);
		case OP_SET_GLOBAL:
			return constantInstruction("OP_SET_GLOBAL", chunk, offset, false);
		case OP_EQUAL:
			return simpleInstruction("OP_EQUAL", offset);
		case OP_GREATER:
			return simpleInstruction("OP_GREATER", offset);
		case OP_LESS:
			return simpleInstruction("OP_LESS", offset);
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
		case OP_NOT:
			return simpleInstruction("OP_NOT", offset);
		case OP_PRINT:
			return simpleInstruction("OP_PRINT", offset);
		case OP_RETURN:
			return simpleInstruction("OP_RETURN", offset);
		default:
			printf("Unknown opcode %d\n", instruction);
			return offset + 1;
	}
}
