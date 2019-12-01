//
//  vm.h
//  clox
//
//  Created by Matthew Pohlmann on 4/6/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "table.h"
#include "object.h"



#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef enum InterpretResult
{
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
} InterpretResult;

typedef struct CallFrame
{
	ObjClosure * closure;
	uint8_t * ip;
	Value * slots;
} CallFrame;

typedef struct VM
{
	CallFrame frames[FRAMES_MAX];
	int frameCount;
	Value stack[STACK_MAX]; // TODO (matthewp) Allow stack growth
	Value * stackTop;
	Table globals;
	Table strings;
	ObjUpvalue * openUpvalues;

	Obj * objects;

	// Stack growth ideas:
	//  https://blog.cloudflare.com/how-stacks-are-handled-in-go/
	//  https://wingolog.org/archives/2014/03/17/stack-overflow
	//  1. Allocate fixed-sized stack segments as-needed; chain segments
	//  2. Allocate and copy stack to new location (like std::vector)
	//  3. Reserve virtual memory and commit as-needed
	//  Q: For (1) and (2), when do I check if stack growth is required? On
	//     every PUSH(...) seems very slow, perhaps auto-generated instruction
	//     in preamble to function calls?

	// BB (matthewp) Find a way to use array.h helpers here

	Obj ** grayStack;
	int grayCount;
	int grayCapacity;

	size_t bytesAllocated;
	size_t bytesAllocatedMax;
	size_t nextGC;
} VM;

extern VM vm;

void initVM(void);
void freeVM(void);

InterpretResult interpret(const char * source);
InterpretResult interpretFunction(ObjFunction * function);

void push(Value value);
Value pop(void);
