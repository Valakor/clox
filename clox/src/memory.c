//
//  memory.c
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "memory.h"

#include <stdlib.h>

#include "array.h"
#include "vm.h"
#include "compiler.h"

#if DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif // DEBUG_LOG_GC

// #define GC_HEAP_GROW_FACTOR 1.5
#define GC_GROW_HEAP(h) (size_t)((h) + ((h) >> 1))



// TODO: perform memory management manually to avoid system malloc / free
//  See: http://www.craftinginterpreters.com/chunks-of-bytecode.html#challenges #3

#if DEBUG_ALLOC
int64_t s_cAlloc = 0;
#endif // #if DEBUG_ALLOC

void * xrealloc(void * previous, size_t oldSize, size_t newSize)
{
#if DEBUG_ALLOC
	if (newSize > 0 && oldSize == 0)
	{
		s_cAlloc++;
	}
	else if (newSize == 0 && oldSize > 0)
	{
		s_cAlloc--;
	}

	ASSERTMSG(s_cAlloc >= 0, "Allocation count went negative!");

	size_t bytesAllocatedPrev = vm.bytesAllocated;
#endif // DEBUG_ALLOC

	int64_t dCb = newSize - oldSize;
	vm.bytesAllocated += dCb;
	vm.bytesAllocatedMax = MAX(vm.bytesAllocated, vm.bytesAllocatedMax);

#if DEBUG_ALLOC
	ASSERTMSG(dCb >= 0 || vm.bytesAllocated < bytesAllocatedPrev, "Allocated bytes underflow!");
	ASSERTMSG(dCb <= 0 || vm.bytesAllocated > bytesAllocatedPrev, "Allocated bytes overflow!");
#endif // DEBUG_ALLOC

	if (newSize > oldSize)
	{
#if DEBUG_STRESS_GC
		collectGarbage();
#endif // DEBUG_STRESS_GC

		if (vm.bytesAllocated > vm.nextGC)
		{
			collectGarbage();
		}
	}

	if (newSize == 0 && oldSize == 0)
	{
		ASSERT(previous == NULL);
		return NULL;
	}
	else if (newSize == 0)
	{
		free(previous);
		return NULL;
	}
	else
	{
		void * p = realloc(previous, newSize);
		ASSERTMSG(p != NULL, "Out of memory!");
		return p;
	}
}

static void freeObject(Obj * object)
{
#if DEBUG_LOG_GC
	printf("%p free type %d\n", (void*)object, getObjType(object));
#endif // DEBUG_LOG_GC

	switch (getObjType(object))
	{
		case OBJ_UPVALUE:
		{
			FREE(ObjUpvalue, object);
			break;
		}

		case OBJ_FUNCTION:
		{
			ObjFunction * function = (ObjFunction *)object;
			freeChunk(&function->chunk);
			FREE(ObjFunction, function);
			break;
		}

		case OBJ_CLASS:
		{
			ObjClass* klass = (ObjClass*)object;
			freeTable(&klass->methods);
			FREE(ObjClass, object);
			break;
		}

		case OBJ_INSTANCE:
		{
			ObjInstance * instance = (ObjInstance*)object;
			freeTable(&instance->fields);
			FREE(ObjInstance, object);
			break;
		}

		case OBJ_CLOSURE:
		{
			ObjClosure * closure = (ObjClosure *)object;
			CARY_FREE(ObjUpvalue *, closure->upvalues, closure->upvalueCount);
			FREE(ObjClosure, closure);
			break;
		}

		case OBJ_BOUND_METHOD:
		{
			FREE(ObjBoundMethod, object);
			break;
		}

		case OBJ_NATIVE:
		{
			FREE(ObjNative, object);
			break;
		}

		case OBJ_STRING:
		{
			ObjString * string = (ObjString*)object;
			CARY_FREE(char, (void *)string->aChars, string->length + 1);
			FREE(ObjString, string);
			break;
		}
	}
}

static void markRoots(void)
{
	for (Value* slot = vm.stack; slot < vm.stackTop; slot++)
	{
		markValue(*slot);
	}

	for (int i = 0; i < vm.frameCount; i++)
	{
		markObject((Obj*)vm.frames[i].closure);
	}

	for (ObjUpvalue* upvalue = vm.openUpvalues; upvalue != NULL; upvalue = upvalue->next)
	{
		markObject((Obj*)upvalue);
	}

	markTable(&vm.globals);

	markCompilerRoots();
	markObject((Obj*)vm.initString);
}

static void blackenObject(Obj* obj)
{
#if DEBUG_LOG_GC
	printf("%p blacken ", (void*)obj);
	printValue(OBJ_VAL(obj));
	printf("\n");
#endif // DEBUG_LOG_GC

	switch (getObjType(obj))
	{
	case OBJ_CLOSURE:
	{
		ObjClosure* closure = (ObjClosure*)obj;
		markObject((Obj*)closure->function);

		for (int i = 0; i < closure->upvalueCount; i++)
		{
			markObject((Obj*)closure->upvalues[i]);
		}

		break;
	}

	case OBJ_BOUND_METHOD:
	{
		ObjBoundMethod* bound = (ObjBoundMethod*)obj;
		markValue(bound->receiver);
		markObject((Obj*)bound->method);
		break;
	}

	case OBJ_FUNCTION:
	{
		ObjFunction* function = (ObjFunction*)obj;
		markObject((Obj*)function->name);
		markArray(function->chunk.aryValConstants);
		break;
	}

	case OBJ_CLASS:
	{
		ObjClass* klass = (ObjClass*)obj;
		markObject((Obj*)klass->name);
		markTable(&klass->methods);
		break;
	}

	case OBJ_INSTANCE:
	{
		ObjInstance* instance = (ObjInstance*)obj;
		markObject((Obj*)instance->klass);
		markTable(&instance->fields);
		break;
	}

	case OBJ_UPVALUE:
		markValue(((ObjUpvalue*)obj)->closed);
		break;

	case OBJ_NATIVE:
	case OBJ_STRING:
		break;
	}
}

static void traceReferences(void)
{
	while (vm.grayCount > 0)
	{
		Obj* obj = vm.grayStack[--vm.grayCount];
		blackenObject(obj);
	}
}

static void sweep(void)
{
	Obj* previous = NULL;
	Obj* obj = vm.objects;

	while (obj != NULL)
	{
		if (getIsMarked(obj))
		{
			setIsMarked(obj, false);
			previous = obj;
			obj = getObjNext(obj);
		}
		else
		{
			Obj* unreached = obj;
			obj = getObjNext(obj);

			if (previous != NULL)
			{
				setObjNext(previous, obj);
			}
			else
			{
				vm.objects = obj;
			}

			freeObject(unreached);
		}
	}
}

void markValue(Value value)
{
	if (!IS_OBJ(value))
		return;

	markObject(AS_OBJ(value));
}

void markObject(Obj* obj)
{
	if (obj == NULL)
		return;

	if (getIsMarked(obj))
		return;

#if DEBUG_LOG_GC
	printf("%p mark ", (void*)obj);
	printValue(OBJ_VAL(obj));
	printf("\n");
#endif // DEBUG_LOG_GC

	setIsMarked(obj, true);

	if (vm.grayCapacity < vm.grayCount + 1)
	{
		// BB (matthewp) Use array/memory helpers here

		vm.grayCapacity = CARY_GROW_CAPACITY(vm.grayCapacity);
		vm.grayStack = realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
	}

	vm.grayStack[vm.grayCount++] = obj;
}

void markArray(Value* aryValue)
{
	int len = ARY_LEN(aryValue);

	for (int i = 0; i < len; i++)
	{
		markValue(aryValue[i]);
	}
}

void collectGarbage(void)
{
#if DEBUG_LOG_GC
	printf("-- gc begin\n");
	size_t before = vm.bytesAllocated;
#endif // DEBUG_LOG_GC

	markRoots();
	traceReferences();
	tableRemoveWhite(&vm.strings);
	sweep();

	vm.nextGC = GC_GROW_HEAP(vm.bytesAllocated);

#if DEBUG_LOG_GC
	printf("-- gc end\n");
	size_t after = vm.bytesAllocated;
	printf("   collected %lld bytes (from %zu to %zu) next at %zu\n", before - after, before, after, vm.nextGC);
#endif // DEBUG_LOG_GC
}

void freeObjects(void)
{
	Obj * object = vm.objects;
	while (object != NULL)
	{
		Obj * next = getObjNext(object);
		freeObject(object);
		object = next;
	}
}
