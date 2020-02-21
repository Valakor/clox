//
//  chunk.h
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "value.h"



typedef enum OpCode
{
	OP_CONSTANT,
	OP_CONSTANT_LONG,
	OP_NIL,
	OP_TRUE,
	OP_FALSE,
	OP_POP,				// Pop once
	OP_POPN,			// Pop N times (stores N - 2 in bytecode)
	OP_GET_LOCAL,
	OP_GET_LOCAL_LONG,
	OP_SET_LOCAL,
	OP_SET_LOCAL_LONG,
	OP_GET_GLOBAL,
	OP_GET_GLOBAL_LONG,
	OP_DEFINE_GLOBAL,
	OP_DEFINE_GLOBAL_LONG,
	OP_SET_GLOBAL,
	OP_SET_GLOBAL_LONG,
	OP_GET_UPVALUE,
	OP_GET_UPVALUE_LONG,
	OP_SET_UPVALUE,
	OP_SET_UPVALUE_LONG,
	OP_GET_PROPERTY,
	OP_GET_PROPERTY_LONG,
	OP_SET_PROPERTY,
	OP_SET_PROPERTY_LONG,
	OP_EQUAL,
	OP_GREATER,
	OP_LESS,
	OP_NEGATE,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_NOT,
	OP_PRINT,
	OP_JUMP,
	OP_JUMP_IF_FALSE,
	OP_LOOP,
	OP_CALL,
	OP_INVOKE,
	OP_INVOKE_LONG,
	OP_CLOSURE,
	OP_CLOSURE_LONG,
	OP_CLOSE_UPVALUE,
	OP_RETURN,
	OP_CLASS,
	OP_CLASS_LONG,
	OP_METHOD,
	OP_METHOD_LONG,

	OP_MAX,
	OP_MIN = 0,
} OpCode;

static_assert(OP_MAX <= UINT8_MAX + 1, "OpCode Max value is too large");



typedef struct InstructionRange
{
	unsigned instructionMic;
	unsigned instructionMac;
	unsigned line;
} InstructionRange; // tag = instrange

typedef struct Chunk
{
	uint8_t * aryB;
	Value * aryValConstants;

	InstructionRange * aryInstrange;
} Chunk; // tag = chunk



void initChunk(Chunk * chunk);
void freeChunk(Chunk * chunk);
void writeChunk(Chunk * chunk, uint8_t byte, unsigned line);
uint32_t addConstant(Chunk * chunk, Value value);
unsigned getLine(Chunk * chunk, unsigned instruction);

void printInstructionRanges(Chunk * chunk);
