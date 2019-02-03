//
//  chunk.c
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "chunk.h"

#include <stdlib.h>
#include <stdio.h>

#include "array.h"



static void addInstructionToRange(InstructionRange ** paryInstrange, unsigned instruction, unsigned line)
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

static unsigned getLineForInstruction(InstructionRange * aryInstrange, unsigned instruction)
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
	ARY_FREE(chunk->aryB);
	ARY_FREE(chunk->aryValConstants);
	ARY_FREE(chunk->aryInstrange);
	initChunk(chunk);
}

void writeChunk(Chunk * chunk, uint8_t byte, unsigned line)
{
	ARY_PUSH(chunk->aryB, byte);

	addInstructionToRange(&chunk->aryInstrange, ARY_LEN(chunk->aryB) - 1, line);
}

uint32_t addConstant(Chunk * chunk, Value value)
{
	ARY_PUSH(chunk->aryValConstants, value);
	return ARY_LEN(chunk->aryValConstants) - 1;
}

unsigned getLine(Chunk * chunk, unsigned instruction)
{
	ASSERT(chunk);
	ASSERT(instruction < ARY_LEN(chunk->aryB));
	ASSERT(chunk->aryInstrange);

	return getLineForInstruction(chunk->aryInstrange, instruction);
}

void printInstructionRanges(Chunk * chunk)
{
	InstructionRange * aryInstrange = chunk->aryInstrange;

	for (unsigned i = 0; i < ARY_LEN(aryInstrange); i++)
	{
		InstructionRange range = aryInstrange[i];

		printf("%4d: [%u-%u)\n", range.line, range.instructionMic, range.instructionMac);
	}
}
