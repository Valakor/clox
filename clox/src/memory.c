//
//  memory.c
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "memory.h"

#include <stdlib.h>
#include "vm.h"
#include "object.h"



// TODO: perform memory management manually to avoid system malloc / free
//  See: http://www.craftinginterpreters.com/chunks-of-bytecode.html#challenges #3

void * xmalloc(size_t size)
{
	void * p = malloc(size);
	ASSERTMSG(p != NULL, "Out of memory!");
	return p;
}

void * xcalloc(size_t elemCount, size_t elemSize)
{
	void * p = calloc(elemCount, elemSize);
	ASSERTMSG(p != NULL, "Out of memory!");
	return p;
}

void xfree(void * p)
{
	free(p);
}

#if DEBUG_ALLOC
int64_t s_cBAlloc = 0;
int64_t s_cBAllocMax = 0;
#endif // #if DEBUG_ALLOC

void * xrealloc(void * previous, size_t oldSize, size_t newSize)
{
#if DEBUG_ALLOC
	int64_t dCb = newSize - oldSize;

	s_cBAllocMax = MAX(s_cBAlloc, s_cBAlloc + dCb);
	s_cBAlloc += dCb;

	ASSERTMSG(s_cBAlloc >= 0, "Allocated bytes went negative!");
#endif // DEBUG_ALLOC

	if (newSize == 0)
	{
		xfree(previous);
		return NULL;
	}

	void * p = realloc(previous, newSize);
	ASSERTMSG(p != NULL, "Out of memory!");
	return p;
}

static void freeObject(Obj * object)
{
	switch (object->type)
	{
		case OBJ_STRING:
		{
			ObjString * string = (ObjString*)object;
			size_t cB = offsetof(ObjString, chars) + string->length + 1;
			xrealloc(object, cB, 0);
			break;
		}
	}
}

void freeObjects(void)
{
	Obj * object = vm.objects;
	while (object != NULL)
	{
		Obj * next = object->next;
		freeObject(object);
		object = next;
	}
}
