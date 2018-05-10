//
//  memory.c
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "memory.h"

#include <stdlib.h>



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

void * xrealloc(void * previous, size_t newSize)
{
	if (newSize == 0)
	{
		xfree(previous);
		return NULL;
	}

	void * p = realloc(previous, newSize);
	ASSERTMSG(p != NULL, "Out of memory!");
	return p;
}

void xfree(void * p)
{
	free(p);
}
