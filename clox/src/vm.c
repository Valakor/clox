//
//  vm.c
//  clox
//
//  Created by Matthew Pohlmann on 4/6/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "vm.h"

#include "debug.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"



VM vm; // TODO (matthewp) Make this not a global!

static void resetStack(void);

void initVM()
{
	resetStack();
	vm.objects = NULL;
	initTable(&vm.globals);
	initTable(&vm.strings);
}

void freeVM()
{
	freeTable(&vm.globals);
	freeTable(&vm.strings);
	freeObjects();

#if DEBUG_ALLOC
	ASSERTMSG(s_cBAlloc == 0, "Memory leak detected! (s_cBAlloc=%llu)", s_cBAlloc);
	printf("[Memory] Max allocated bytes: %llu\n", s_cBAllocMax);
#endif // #if DEBUG_ALLOC
}

void push(Value value)
{
	ASSERT((int64_t)(vm.stackTop - vm.stack) < STACK_MAX);

	*vm.stackTop = value;
	vm.stackTop++;
}

Value pop(void)
{
	ASSERT((int64_t)(vm.stackTop - vm.stack) > 0);

	vm.stackTop--;
	return *vm.stackTop;
}

static Value peek(int distance)
{
	ASSERT(vm.stackTop - 1 - distance >= vm.stack);

	return vm.stackTop[-1 - distance];
}

static bool isFalsey(Value value)
{
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate(void)
{
	ObjString * b = AS_STRING(pop());
	ObjString * a = AS_STRING(pop());

	ObjString * result = concatStrings(a, b);

	push(OBJ_VAL(result));
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

	InterpretResult result = interpretChunk(&chunk);

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

	unsigned instruction = (int)(vm.ip - 1 - vm.chunk->aryB);
	unsigned line = getLine(vm.chunk, instruction);
	fprintf(stderr, "[line %u] in script\n", line);

	resetStack();
}

static InterpretResult run(void)
{
	// BB (matthewp) Avoid extra push-pop operations by modifying the top of the stack in-place
	//  Example: In unary negation, instead of: push(negate(pop())), do negate(peek())

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->aryValConstants[READ_BYTE()])
#define READ_CONSTANT_LONG() (vm.chunk->aryValConstants[((READ_BYTE() << 16) | (READ_BYTE() << 8) | (READ_BYTE()))])
#define READ_STRING() AS_STRING(READ_CONSTANT())
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
		disassembleInstruction(vm.chunk, (unsigned)(vm.ip - vm.chunk->aryB));
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
			case OP_POP: pop(); break;

			case OP_POPN:
			{
				uint32_t num = READ_BYTE() + 2;
				while (num--)
				{
					pop();
				}
				break;
			}

			case OP_GET_LOCAL:
			{
				uint8_t slot = READ_BYTE();
				push(vm.stack[slot]);
				break;
			}

			case OP_SET_LOCAL:
			{
				uint8_t slot = READ_BYTE();
				vm.stack[slot] = peek(0);
				break;
			}

			// TODO: Improve global lookup by avoiding hash-table. Consider assigning every
			//  global a unique (linear) ID during compilation and writing that to the bytecode
			//  stream. Lookup becomes just an index into an array (need an extra bool to make
			//  sure it's actually been defined already?).

			case OP_GET_GLOBAL:
			{
				// TODO: Support more globals (OP_GET_GLOBAL_LONG)
				ObjString * name = READ_STRING();
				Value value;
				if (!tableGet(&vm.globals, name, &value))
				{
					runtimeError("Undefined variable '%s'.", name->aChars);
					return INTERPRET_RUNTIME_ERROR;
				}
				push(value);
				break;
			}

			case OP_DEFINE_GLOBAL:
			{
				// TODO: Support more globals (OP_DEFINE_GLOBAL_LONG)
				ObjString * name = READ_STRING();
				tableSet(&vm.globals, name, peek(0));
				pop();
				break;
			}

			case OP_SET_GLOBAL:
			{
				// TODO: Support more globals (OP_SET_GLOBAL_LONG)
				ObjString * name = READ_STRING();
				if (tableSet(&vm.globals, name, peek(0)))
				{
					runtimeError("Undefined variable '%s'.", name->aChars);
					return INTERPRET_RUNTIME_ERROR;
				}
				break;
			}

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

			case OP_ADD:
			{
				if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
				{
					concatenate();
				}
				else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
				{
					double b = AS_NUMBER(pop());
					double a = AS_NUMBER(pop());
					push(NUMBER_VAL(a + b));
				}
				else
				{
					runtimeError("Operands must be two numbers or two strings");
					return INTERPRET_RUNTIME_ERROR;
				}
				break;
			}
			case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
			case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
			case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;

			case OP_NOT: push(BOOL_VAL(isFalsey(pop()))); break;

			case OP_PRINT:
			{
				printValue(pop());
				printf("\n");
				break;
			}

			case OP_RETURN: return INTERPRET_OK;
		}
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
#undef READ_STRING
#undef BINARY_OP
}
