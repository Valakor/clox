//
//  vm.h
//  clox
//
//  Created by Matthew Pohlmann on 4/6/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#ifndef vm_h
#define vm_h

#include "common.h"
#include "chunk.h"
#include "value.h"



#define STACK_MAX 256

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
} InterpretResult;

typedef struct
{
	Chunk * chunk;
	uint8_t * ip;
	Value stack[STACK_MAX]; // TODO (matthewp) Allow stack growth
	Value * stackTop;
} VM;

void initVM(void);
void freeVM(void);

InterpretResult interpret(Chunk * chunk);

void push(Value value);
Value pop(void);

#endif /* vm_h */
