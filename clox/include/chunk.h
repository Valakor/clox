//
//  chunk.h
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "common.h"
#include "value.h"

typedef enum
{
	OP_CONSTANT,
	OP_CONSTANT_LONG,
	OP_NIL,
	OP_TRUE,
	OP_FALSE,
	OP_EQUAL,
	OP_GREATER,
	OP_LESS,
	OP_NEGATE,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_NOT,
	OP_RETURN,

	OP_MAX,
	OP_MIN = 0,
} OpCode;
CASSERT(OP_MAX <= UINT8_MAX + 1);

typedef struct
{
	unsigned instructionMic;
	unsigned instructionMac;
	unsigned line;
} InstructionRange; // tag = instrange

typedef struct
{
	uint8_t * aryB;
	Value * aryValConstants;

	InstructionRange * aryInstrange;
} Chunk; // tag = chunk

void initChunk(Chunk * chunk);
void freeChunk(Chunk * chunk);
void writeChunk(Chunk * chunk, uint8_t byte, unsigned line);
bool writeConstant(Chunk * chunk, Value value, unsigned line);
unsigned getLine(Chunk * chunk, unsigned instruction);

void printInstructionRanges(Chunk * chunk);
