//
//  vm.c
//  clox
//
//  Created by Matthew Pohlmann on 4/6/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include <stdio.h>
#include <stdarg.h>

#include "vm.h"

#include "debug.h"
#include "compiler.h"



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

static Value peek(int distance)
{
	return vm.stackTop[-1 - distance];
}

static bool isFalsey(Value value)
{
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static InterpretResult run(void);

InterpretResult interpret(const char * source)
{
	Chunk chunk;
	initChunk(&chunk);

	if (!compile(source, &chunk))
	{
		freeChunk(&chunk);
		return INTERPRET_COMPILE_ERROR;
	}

	vm.chunk = &chunk;
	vm.ip = vm.chunk->aryB;

	InterpretResult result = run();

	freeChunk(&chunk);

	return result;
}

InterpretResult interpretChunk(Chunk * chunk)
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

static void runtimeError(const char * format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	int instruction = (int)(vm.ip - 1 - vm.chunk->aryB);
	int line = getLine(vm.chunk, instruction);
	fprintf(stderr, "[line %d] in script\n", line);

	resetStack();
}

static InterpretResult run(void)
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->aryValConstants[READ_BYTE()])
#define READ_CONSTANT_LONG() (vm.chunk->aryValConstants[((READ_BYTE() << 16) | (READ_BYTE() << 8) | (READ_BYTE()))])
#define BINARY_OP(valueType, op) \
	do { \
		if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
			runtimeError("Operands must be numbers."); \
			return INTERPRET_RUNTIME_ERROR; \
		} \
		double b = AS_NUMBER(pop()); \
		double a = AS_NUMBER(pop()); \
		push(valueType(a op b)); \
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

			case OP_NIL: push(NIL_VAL); break;
			case OP_TRUE: push(BOOL_VAL(true)); break;
			case OP_FALSE: push(BOOL_VAL(false)); break;

			case OP_EQUAL:
			{
				Value b = pop();
				Value a = pop();
				push(BOOL_VAL(valuesEqual(a, b)));
				break;
			}

			case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
			case OP_LESS: BINARY_OP(BOOL_VAL, <); break;

			case OP_NEGATE:
				if (!IS_NUMBER(peek(0)))
				{
					runtimeError("Operand must be a number.");
					return INTERPRET_RUNTIME_ERROR;
				}

				push(NUMBER_VAL(-AS_NUMBER(pop())));
				break;

			case OP_ADD: BINARY_OP(NUMBER_VAL, +); break;
			case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
			case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
			case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;

			case OP_NOT: push(BOOL_VAL(isFalsey(pop()))); break;

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
