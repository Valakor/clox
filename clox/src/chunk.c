//
//  chunk.c
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "chunk.h"

#include <stdlib.h>

#include "memory.h"

static void initLines(Lines * lines)
{
	lines->count = 0;
	lines->capacity = 0;
	lines->ranges = NULL;
}

static InstructionRange * addInstructionRange(Lines * lines)
{
	if (lines->capacity < lines->count + 1)
	{
		int oldCapacity = lines->capacity;
		lines->capacity = GROW_CAPACITY(oldCapacity);
		lines->ranges = GROW_ARRAY(lines->ranges, InstructionRange, oldCapacity, lines->capacity);
	}

	InstructionRange * range = &lines->ranges[lines->count];
	range->instructionMic = 0;
	range->instructionMac = 0;
	range->line = 0;

	lines->count++;
	return range;
}

static void addInstructionToLines(Lines * lines, int instruction, int line)
{
	// We assume line numbers are always non-decreasing as we add instructions

	ASSERT(lines->count == 0 || line >= lines->ranges[lines->count - 1].line);

	if (lines->count == 0 || line > lines->ranges[lines->count - 1].line)
	{
		// New instruction must come after previous instruction range max

		ASSERT(lines->count == 0 || instruction >= lines->ranges[lines->count - 1].instructionMac);

		InstructionRange * range = addInstructionRange(lines);
		range->instructionMic = instruction;
		range->instructionMac = instruction + 1;
		range->line = line;
	}
	else
	{
		lines->ranges[lines->count - 1].instructionMac++;
	}
}

static int cmpInstructionRange(const void * vKey, const void * vElem)
{
	const InstructionRange * key = vKey;
	const InstructionRange * elem = vElem;

	if (key->line < elem->instructionMic)
		return -1;
	else if (key->line >= elem->instructionMac)
		return 1;
	return 0;
}

static int getLineForInstruction(Lines * lines, int instruction)
{
	InstructionRange rangeKey = { instruction, instruction, instruction };
	InstructionRange * range = bsearch(&rangeKey, lines->ranges, lines->count, sizeof(InstructionRange), cmpInstructionRange);

	if (!range)
	{
		ASSERT(false);
		return 0;
	}

	return range->line;
}

static void freeLines(Lines * lines)
{
	FREE_ARRAY(InstructionRange, lines->ranges, lines->capacity);
	initLines(lines);
}

void initChunk(Chunk * chunk)
{
	chunk->count = 0;
	chunk->capacity = 0;
	chunk->code = NULL;

	initValueArray(&chunk->constants);

	initLines(&chunk->lines);
}

void freeChunk(Chunk * chunk)
{
	FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
	freeValueArray(&chunk->constants);
	freeLines(&chunk->lines);
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

	addInstructionToLines(&chunk->lines, chunk->count - 1, line);
}

void writeConstant(Chunk * chunk, Value value, int line)
{
	int constant = addConstant(chunk, value);

	if (constant <= UINT8_MAX)
	{
		// Use more optimal 1-byte constant op

		writeChunk(chunk, OP_CONSTANT, line);
		writeChunk(chunk, constant, line);
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

int addConstant(Chunk * chunk, Value value)
{
	writeValueArray(&chunk->constants, value);
	return chunk->constants.count - 1;
}

int getLine(Chunk * chunk, int instruction)
{
	ASSERT(instruction < chunk->count);
	return getLineForInstruction(&chunk->lines, instruction);
}
