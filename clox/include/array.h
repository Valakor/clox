//
//  array.h
//  clox
//
//  Created by Matthew Pohlmann on 5/7/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "common.h"
#include "memory.h"



// Basic C-style array helpers

#define CARY_ALLOCATE(type, count) \
	ALLOCATE(type, count)

#define CARY_GROW_CAPACITY(capacity) \
	((capacity) < 8 ? 8 : (capacity) * 2)

#define CARY_GROW(previous, type, oldCount, count) \
	(type*)xrealloc(previous, sizeof(type) * (oldCount), sizeof(type) * (count))

#define CARY_FREE(type, pointer, oldCount) \
	xrealloc(pointer, sizeof(type) * (oldCount), 0)



// Implementation of stretchy buffers, credit to Sean Barrett

typedef struct
{
	uint32_t	len;
	uint32_t	cap;
	uint8_t		aB[];
} AryHdr;

// Actual API

// To view in watch window on Windows:
//   int * aryN = NULL;
//   Watch expression: aryN, [((AryHdr *)((uint8_t *)aryN - 8))->len]

#define ARY_LEN(ary) ((ary) ? ARY__HDR(ary)->len : 0)
#define ARY_CAP(ary) ((ary) ? ARY__HDR(ary)->cap : 0)
#define ARY_END(ary) ((ary) + ARY_LEN(ary))
#define ARY_TAIL(ary) (ASSERT(!ARY_EMPTY(ary)), ((ary) + ARY_LEN(ary) - 1))
#define ARY_FREE(ary) ((ary) ? (Ary__Free(ary, sizeof(*(ary))), (ary) = NULL) : 0)
#define ARY_PUSH(ary, x) ((ARY__ENSURECAP((ary), ARY_LEN(ary) + 1)), (ary)[ARY__HDR(ary)->len++] = (x))
#define ARY_POP(ary) ((ARY_LEN(ary) > 0) ? ARY__HDR(ary)->len-- : 0)
#define ARY_EMPTY(ary) (ARY_LEN(ary) == 0)
#define ARY_CLEAR(ary) ((ary) ? ARY__HDR(ary)->len = 0 : 0)

// Helpers

#define ARY__HDR(ary) ((AryHdr *)((uint8_t *)(ary) - offsetof(AryHdr, aB)))
#define ARY__ENSURECAP(ary, n) (((n) <= ARY_CAP(ary)) ? 0 : ((ary) = Ary__AllocGrow((ary), (n), sizeof(*(ary)))))

// BB (matthewp) XCode complains about this being unused in every header that includes this, but doesn't
//  use ARY_PUSH or other allocation functions. Find a better way to resolve this than using 'inline'

static inline void * Ary__AllocGrow(void * ary, uint32_t newCapMin, uint32_t elemSize)
{
	ASSERTMSG(ARY_CAP(ary) <= UINT32_MAX / 2, "Array capacity overflow");

	uint32_t curCap = ARY_CAP(ary);
	uint32_t newCap = MAX(2 * curCap, MAX(newCapMin, 8));

	ASSERT(newCapMin <= newCap);
	ASSERTMSG(newCap <= (SIZE_MAX - offsetof(AryHdr, aB)) / elemSize, "Array allocation size overflow");

	size_t sizeAlloc = newCap * elemSize + offsetof(AryHdr, aB);
	size_t sizeAllocOld = curCap * elemSize + offsetof(AryHdr, aB);
	
	AryHdr * pHdr;

	if (ary)
	{
		pHdr = xrealloc(ARY__HDR(ary), sizeAllocOld, sizeAlloc);
	}
	else
	{
		pHdr = xrealloc(NULL, 0, sizeAlloc);
		pHdr->len = 0;
	}

	pHdr->cap = newCap;

	return pHdr->aB;
}

static inline void Ary__Free(void * ary, uint32_t elemSize)
{
	if (ary)
	{
		uint32_t curCap = ARY_CAP(ary);
		size_t sizeAllocOld = curCap * elemSize + offsetof(AryHdr, aB);
		xrealloc(ARY__HDR(ary), sizeAllocOld, 0);
	}
}
