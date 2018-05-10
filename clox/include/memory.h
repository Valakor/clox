//
//  memory.h
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "common.h"



#define GROW_CAPACITY(capacity) \
	((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(previous, type, oldCount, count) \
	(type*)xrealloc(previous, sizeof(type) * (count))

#define FREE_ARRAY(type, pointer, oldCount) \
	xrealloc(pointer, 0)

// BB (matthewp) Consider inlining all or some of these

void * xmalloc(size_t size);
void * xcalloc(size_t elemCount, size_t elemSize);
void * xrealloc(void * previous, size_t newSize);
void   xfree(void * p);
