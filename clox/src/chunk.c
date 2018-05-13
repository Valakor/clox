//
//  chunk.c
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "chunk.h"

#include <stdlib.h>

#include "array.h"



static void addInstructionToRange(InstructionRange ** paryInstrange, int instruction, int line)
{
	// We assume line numbers are always non-decreasing as we add instructions

	ASSERT(ARY_EMPTY(*paryInstrange) || line >= ARY_TAIL(*paryInstrange)->line);

	if (ARY_EMPTY(*paryInstrange) || line > ARY_TAIL(*paryInstrange)->line)
	{
		// New instruction must come after previous instruction range max

		ASSERT(ARY_EMPTY(*paryInstrange) || instruction >= ARY_TAIL(*paryInstrange)->instructionMac);

		InstructionRange instrange = { instruction, instruction + 1, line };

		ARY_PUSH(*paryInstrange, instrange);
	}
	else
	{
		ARY_TAIL(*paryInstrange)->instructionMac++;
	}
}

static int cmpInstructionRange(const void * vKey, const void * vElem)
{
	const InstructionRange * key = (const InstructionRange *)vKey;
	const InstructionRange * elem = (const InstructionRange *)vElem;

	if (key->line < elem->instructionMic)
		return -1;
	else if (key->line >= elem->instructionMac)
		return 1;
	else
		return 0;
}

static int getLineForInstruction(InstructionRange * aryInstrange, int instruction)
{
	InstructionRange rangeKey;
	rangeKey.instructionMac = rangeKey.instructionMic = rangeKey.line = instruction;

	InstructionRange * range = bsearch(&rangeKey, aryInstrange, ARY_LEN(aryInstrange), sizeof(InstructionRange), cmpInstructionRange);

	if (!range)
	{
		ASSERT(false);
		return 0;
	}

	return range->line;
}

void initChunk(Chunk * chunk)
{
	CLEAR_STRUCT(*chunk);
}

void freeChunk(Chunk * chunk)
{
	FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
	freeValueArray(&chunk->constants);
	ARY_FREE(chunk->aryInstrange);
	initChunk(chunk);
}

void writeChunk(Chunk * chunk, uint8_t byte, int line)
{
	if (chunk->capacity < chunk->count + 1)
	{
		int oldCapacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(oldCapacity);
		chunk->code = GROW_ARRAY(chunk->code, uint8_t, oldCapacity, chunk->capacity);
	}

	chunk->code[chunk->count] = byte;
	chunk->count++;

	addInstructionToRange(&chunk->aryInstrange, chunk->count - 1, line);
}

void writeConstant(Chunk * chunk, Value value, int line)
{
	writeValueArray(&chunk->constants, value);
	int constant = chunk->constants.count - 1;

	if (constant <= UINT8_MAX)
	{
		// Use more optimal 1-byte constant op

		writeChunk(chunk, OP_CONSTANT, line);
		writeChunk(chunk, (uint8_t)constant, line);
	}
	else if (constant <= 16777215) // UINT24_MAX
	{
		// Use 3-byte constant op

		writeChunk(chunk, OP_CONSTANT_LONG, line);

		for (int i = 0; i < 3; i++)
		{
			uint8_t b = (constant >> (8 * (3 - i - 1))) & UINT8_MAX;
			writeChunk(chunk, b, line);
		}
	}
	else
	{
		ASSERTMSG(false, "Too many constants in chunk (> 16777215)");
	}
}

int getLine(Chunk * chunk, int instruction)
{
	ASSERT(chunk);
	ASSERT(instruction < chunk->count);
	ASSERT(chunk->aryInstrange);

	return getLineForInstruction(chunk->aryInstrange, instruction);
}
