//
//  object.c
//  clox
//
//  Created by Matthew Pohlmann on 9/24/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "object.h"

#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "vm.h"
#include "array.h"



#define ALLOCATE_OBJ(type, objectType) (type*)allocateObject(sizeof(type), objectType)

static Obj * allocateObject(size_t size, ObjType type)
{
	Obj * object = (Obj*)xrealloc(NULL, 0, size);
	initObj(object, type, vm.objects);
	vm.objects = object;

#if DEBUG_LOG_GC
	printf("%p allocate %zd for %d\n", (void*)object, size, type);
#endif // DEBUG_LOG_GC

	return object;
}

static ObjString * allocateString(const char * aCh, int cCh, uint32_t hash)
{
	ObjString * pStr = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	pStr->hash = hash;
	pStr->length = cCh;
	pStr->aChars = aCh;

	push(OBJ_VAL(pStr));
	tableSet(&vm.strings, pStr, NIL_VAL);
	pop();

	return pStr;
}

static uint32_t hashString(const char * key, int length)
{
	// FNV-1a

	uint32_t hash = 2166136261u;

	for (int i = 0; i < length; ++i)
	{
		hash ^= key[i];
		hash *= 16777619u;
	}

	return hash;
}

ObjUpvalue* newUpvalue(Value* slot)
{
	ObjUpvalue * upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
	upvalue->location = slot;
	upvalue->closed = NIL_VAL;
	upvalue->next = NULL;
	return upvalue;
}

ObjFunction * newFunction(void)
{
	ObjFunction * function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
	function->arity = 0;
	function->upvalueCount = 0;
	function->name = NULL;
	initChunk(&function->chunk);

	return function;
}

ObjClass * newClass(ObjString * name)
{
	ObjClass * klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
	klass->name = name;
	initTable(&klass->methods);
	klass->init = NULL;
	return klass;
}

ObjInstance * newInstance(ObjClass * klass)
{
	ObjInstance * instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
	instance->klass = klass;
	initTable(&instance->fields);
	return instance;
}

ObjClosure * newClosure(ObjFunction * function)
{
	ObjUpvalue ** upvalues = CARY_ALLOCATE(ObjUpvalue *, function->upvalueCount);

	for (int i = 0; i < function->upvalueCount; ++i)
	{
		upvalues[i] = NULL;
	}

	ObjClosure * closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
	closure->function = function;
	closure->upvalues = upvalues;
	closure->upvalueCount = function->upvalueCount;
	return closure;
}

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method)
{
	ObjBoundMethod * boundMethod = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
	boundMethod->receiver = receiver;
	boundMethod->method = method;
	return boundMethod;
}

ObjNative * newNative(NativeFn function)
{
	ObjNative * native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
	native->function = function;
	return native;
}

ObjString * concatStrings(const ObjString * pStrA, const ObjString * pStrB)
{
	int length = pStrA->length + pStrB->length;
	char * aCh = CARY_ALLOCATE(char, length + 1);

	memcpy(aCh, pStrA->aChars, pStrA->length);
	memcpy(aCh + pStrA->length, pStrB->aChars, pStrB->length);
	aCh[length] = '\0';
	
	uint32_t hash = hashString(aCh, length);

	ObjString * pStr = tableFindString(&vm.strings, aCh, length, hash);
	if (pStr != NULL)
	{
		// BB (matthewp) Could probably find a way to do this without needing to allocate memory at all

		CARY_FREE(char, aCh, length + 1);
		return pStr;
	}

	return allocateString(aCh, length, hash);
}

ObjString * copyString(const char * chars, int length)
{
	uint32_t hash = hashString(chars, length);

	ObjString * pStr = tableFindString(&vm.strings, chars, length, hash);
	if (pStr != NULL)
		return pStr;

	char * aCh = CARY_ALLOCATE(char, length + 1);
	memcpy(aCh, chars, length);
	aCh[length] = '\0';

	return allocateString(aCh, length, hash);
}

ObjString * takeString(const char * chars, int length)
{
	uint32_t hash = hashString(chars, length);

	ObjString * pStr = tableFindString(&vm.strings, chars, length, hash);
	if (pStr != NULL)
	{
		CARY_FREE(char, (void *)chars, length + 1);
		return pStr;
	}

	return allocateString(chars, length, hash);
}

static void printFunction(ObjFunction* function)
{
	if (function->name == NULL)
	{
		printf("<script>");
	}
	else
	{
		printf("<fn %s>", function->name->aChars);
	}
}

static void printInstance(ObjInstance* inst)
{
	// TODO: Call (optional) user-supplied toString() method
	printf("<%s instance>", inst->klass->name->aChars);
}

void printObject(Value value)
{
	switch (OBJ_TYPE(value))
	{
		case OBJ_UPVALUE:
			printf("<upvalue>");
			break;

		case OBJ_FUNCTION:
			printFunction(AS_FUNCTION(value));
			break;

		case OBJ_CLASS:
			printf("<%s>", AS_CLASS(value)->name->aChars);
			break;

		case OBJ_INSTANCE:
			printInstance(AS_INSTANCE(value));
			break;

		case OBJ_CLOSURE:
			printFunction(AS_CLOSURE(value)->function);
			break;

		case OBJ_BOUND_METHOD:
			printFunction(AS_BOUND_METHOD(value)->method->function);
			break;

		case OBJ_NATIVE:
			printf("<native fn 0x%p>", AS_NATIVE(value));
			break;

		case OBJ_STRING:
			printf("%s", AS_CSTRING(value));
			break;
	}
}
