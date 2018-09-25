//
//  memory.h
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "common.h"



#define CLEAR_STRUCT(s) memset(&(s), 0, sizeof(s))
#define ALLOCATE(type, count) (type*)xrealloc(NULL, 0, sizeof(type) * (count))
#define FREE(type, pointer) xrealloc(pointer, sizeof(type), 0)

// BB (matthewp) Consider inlining all or some of these

// void * xmalloc(size_t size);
// void * xcalloc(size_t elemCount, size_t elemSize);
void * xrealloc(void * previous, size_t oldSize, size_t newSize);
// void   xfree(void * p);

void freeObjects(void);

#if DEBUG_ALLOC
extern int64_t s_cBAlloc;
extern int64_t s_cBAllocMax;
#endif // #if DEBUG_ALLOC
