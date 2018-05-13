//
//  vm.c
//  clox
//
//  Created by Matthew Pohlmann on 4/6/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include <stdio.h>

#include "vm.h"

#include "debug.h"



VM vm; // TODO (matthewp) Make this not a global!

static void resetStack(void);

void initVM()
{
	resetStack();
}

void freeVM()
{

}

void push(Value value)
{
	*vm.stackTop = value;
	vm.stackTop++;
}

Value pop(void)
{
	ASSERT((int)(vm.stackTop - vm.stack) > 0);

	vm.stackTop--;
	return *vm.stackTop;
}

static InterpretResult run(void);

InterpretResult interpret(Chunk * chunk)
{
	vm.chunk = chunk;
	vm.ip = chunk->aryB;

	InterpretResult result = run();

	return result;
}

static void resetStack(void)
{
	vm.stackTop = vm.stack;
}

static InterpretResult run(void)
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() (vm.chunk->constants.values[((READ_BYTE() << 16) | (READ_BYTE() << 8) | (READ_BYTE()))])
#define BINARY_OP(op) \
	do { \
		Value b = pop(); \
		Value a = pop(); \
		push (a op b); \
	} while(false)

	for (;;)
	{
#if DEBUG_TRACE_EXECUTION
		printf("          ");
		for (Value* slot = vm.stack; slot < vm.stackTop; slot++)
		{
			printf("[ ");
			printValue(*slot);
			printf(" ]");
		}
		printf("\n");
		disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->aryB));
#endif
		uint8_t instruction;
		switch (instruction = READ_BYTE())
		{
			case OP_CONSTANT:
			{
				Value constant = READ_CONSTANT();
				push(constant);
				break;
			}

			case OP_CONSTANT_LONG:
			{
				Value constant = READ_CONSTANT_LONG();
				push(constant);
				break;
			}

			case OP_NEGATE: push(-pop()); break;

			case OP_ADD: BINARY_OP(+); break;
			case OP_SUBTRACT: BINARY_OP(-); break;
			case OP_MULTIPLY: BINARY_OP(*); break;
			case OP_DIVIDE: BINARY_OP(/); break;

			case OP_RETURN:
			{
				printValue(pop());
				printf("\n");
				return INTERPRET_OK;
			}
		}
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
#undef BINARY_OP
}
