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
#include <time.h>

#include "vm.h"

#include "debug.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"



VM vm; // TODO (matthewp) Make this not a global!

static void resetStack(void);
static bool callValue(Value callee, int argCount);
static void defineNative(const char * name, NativeFn function);

static Value clockNative(int argCount, Value * args);

void initVM()
{
	resetStack();
	vm.objects = NULL;
	initTable(&vm.globals);
	initTable(&vm.strings);

	defineNative("clock", clockNative);
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
	ObjFunction * function = compile(source);

	if (function == NULL)
		return INTERPRET_COMPILE_ERROR;

	push(OBJ_VAL(function));
	callValue(OBJ_VAL(function), 0);

	return run();
}

static void resetStack(void)
{
	vm.stackTop = vm.stack;
	vm.frameCount = 0;
}

static Value clockNative(int argCount, Value * args)
{
	UNUSED(argCount);
	UNUSED(args);

	return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void runtimeError(const char * format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	for (int i = vm.frameCount - 1; i >= 0; i--)
	{
		CallFrame * frame = &vm.frames[i];
		ObjFunction * function = frame->function;

		unsigned instruction = (unsigned)(frame->ip - frame->function->chunk.aryB - 1);
		unsigned line = getLine(&frame->function->chunk, instruction);

		fprintf(stderr, "[line %u] in ", line);

		if (function->name == NULL)
		{
			fprintf(stderr, "script\n");
		}
		else
		{
			fprintf(stderr, "%s()\n", function->name->aChars);
		}
	}

	resetStack();
}

static void defineNative(const char * name, NativeFn function)
{
	push(OBJ_VAL(copyString(name, (int)strlen(name))));
	push(OBJ_VAL(newNative(function)));
	tableSet(&vm.globals, AS_STRING(peek(1)), peek(0));
	pop();
	pop();
}

static bool call(ObjFunction * function, int argCount)
{
	if (argCount != function->arity)
	{
		runtimeError("Expected %d arguments but got %d.", function->arity, argCount);
		return false;
	}

	if (vm.frameCount == FRAMES_MAX)
	{
		runtimeError("Stack overflow.");
		return false;
	}

	CallFrame * frame = &vm.frames[vm.frameCount++];
	frame->function = function;
	frame->ip = function->chunk.aryB;
	frame->slots = vm.stackTop - argCount - 1;

	return true;
}

static bool callValue(Value callee, int argCount)
{
	if (IS_OBJ(callee))
	{
		switch (OBJ_TYPE(callee))
		{
			case OBJ_FUNCTION:
				return call(AS_FUNCTION(callee), argCount);

			case OBJ_NATIVE:
			{
				// BB (matthewp) Allow reporting errors from native functions

				NativeFn native = AS_NATIVE(callee);
				Value result = native(argCount, vm.stackTop - argCount);
				vm.stackTop -= argCount + 1;
				push(result);

				return true;
			}

			default:
				// Non-callable object type;
				break;
		}
	}

	runtimeError("Can only call functions and classes.");
	return false;
}

static InterpretResult run(void)
{
	// NOTE (matthewp) frame->ip MUST be restored whenever leaving this function in case outside code wants to
	//  access the frame's current instruction pointer. The benefits here (>10% performance in simple tests) seem
	//  worth the extra complexity

	CallFrame * frame = &vm.frames[vm.frameCount - 1];
	register uint8_t * ip = frame->ip;

	// BB (matthewp) Avoid extra push-pop operations by modifying the top of the stack in-place
	//  Example: In unary negation, instead of: push(negate(pop())), do negate(peek())

#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_U24() (ip += 3, (uint32_t)((ip[-3] << 16) | (ip[-2] << 8) | ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.aryValConstants[READ_BYTE()])
#define READ_CONSTANT_LONG() (frame->function->chunk.aryValConstants[READ_U24()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op) \
	do { \
		if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
			frame->ip = ip; \
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
		disassembleInstruction(&frame->function->chunk, (unsigned)(ip - frame->function->chunk.aryB));
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
				push(frame->slots[slot]);
				break;
			}

			case OP_SET_LOCAL:
			{
				uint8_t slot = READ_BYTE();
				frame->slots[slot] = peek(0);
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
					frame->ip = ip;
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
					tableDelete(&vm.globals, name);
					frame->ip = ip;
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
					frame->ip = ip;
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
					frame->ip = ip;
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

			case OP_JUMP:
			{
				uint16_t offset = READ_SHORT();
				ip += offset;
				break;
			}

			case OP_JUMP_IF_FALSE:
			{
				uint16_t offset = READ_SHORT();
				if (isFalsey(peek(0))) ip += offset;
				break;
			}

			case OP_LOOP:
			{
				uint16_t offset = READ_SHORT();
				ip -= offset;
				break;
			}

			case OP_CALL:
			{
				int argCount = READ_BYTE();
				frame->ip = ip;

				if (!callValue(peek(argCount), argCount))
					return INTERPRET_RUNTIME_ERROR;

				frame = &vm.frames[vm.frameCount - 1];
				ip = frame->ip;
				break;
			}

			case OP_RETURN:
			{
				Value result = pop();

				vm.frameCount--;
				if (vm.frameCount == 0) return INTERPRET_OK;

				vm.stackTop = frame->slots;
				push(result);

				frame = &vm.frames[vm.frameCount - 1];
				ip = frame->ip;
				break;
			}
		}
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
#undef READ_SHORT
#undef READ_STRING
#undef BINARY_OP
}
