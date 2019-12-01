//
//  memory.h
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "common.h"
#include "value.h"
#include "object.h"



#define CLEAR_STRUCT(s) memset(&(s), 0, sizeof(s))
#define ALLOCATE(type, count) (type*)xrealloc(NULL, 0, sizeof(type) * (count))
#define FREE(type, pointer) xrealloc(pointer, sizeof(type), 0)



extern void* xrealloc(void * previous, size_t oldSize, size_t newSize);

static inline void* xmalloc(size_t size)
{
	return xrealloc(NULL, 0, size);
}

static inline void* xcalloc(size_t elemCount, size_t elemSize)
{
	void* p = xrealloc(NULL, 0, elemCount * elemSize);

	if (p)
	{
		memset(p, 0, elemCount * elemSize);
	}

	return p;
}

static inline void xfree(void* p, size_t size)
{
	xrealloc(p, size, 0);
}



void markValue(Value value);
void markObject(Obj* obj);
void markArray(Value* aryValue);

void collectGarbage(void);
void freeObjects(void);

#if DEBUG_ALLOC
extern int64_t s_cAlloc;
#endif // #if DEBUG_ALLOC
