//
//  chunk.h
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#ifndef chunk_h
#define chunk_h

#include "common.h"
#include "value.h"

typedef enum
{
	OP_CONSTANT,
	OP_CONSTANT_LONG,
	OP_NEGATE,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_RETURN,
} OpCode;

typedef struct
{
	int instructionMic;
	int instructionMac;
	int line;
} InstructionRange;

typedef struct
{
	int count;
	int capacity;
	InstructionRange * ranges;
} Lines;

typedef struct
{
	int count;
	int capacity;
	uint8_t * code;

	ValueArray constants;

	Lines lines;
} Chunk;

void initChunk(Chunk * chunk);
void freeChunk(Chunk * chunk);
void writeChunk(Chunk * chunk, uint8_t byte, int line);
void writeConstant(Chunk * chunk, Value value, int line);
int getLine(Chunk * chunk, int instruction);

#endif /* chunk_h */
