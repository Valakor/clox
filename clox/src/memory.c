//
//  memory.c
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "memory.h"

#include <stdlib.h>

void * reallocate(void * previous, size_t oldSize, size_t newSize)
{
	// TODO: perform memory management manually to avoid system malloc / free
	//  See: http://www.craftinginterpreters.com/chunks-of-bytecode.html#challenges #3

	if (newSize == 0)
	{
		free(previous);
		return NULL;
	}

	return realloc(previous, newSize);
}
