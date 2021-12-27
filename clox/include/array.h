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

#define CARY_ALLOCATE(_type, _count) \
	ALLOCATE(_type, _count)

#define CARY_GROW_CAPACITY(_cap) \
	((_cap) < 8 ? 8 : (_cap) + ((_cap) >> 1))

#define CARY_GROW(_previous, _type, _oldCount, _newCount) \
	(_type*)xrealloc(_previous, sizeof(_type) * (_oldCount), sizeof(_type) * (_newCount))

#define CARY_FREE(_type, _pointer, _count) \
	xrealloc(_pointer, sizeof(_type) * (_count), 0)



// Implementation of stretchy buffers, credit to Sean Barrett

typedef struct AryHdr
{
	uint32_t	len;
	uint32_t	cap;
	uint8_t		aB[];
} AryHdr;

// Actual API

// To view in watch window on Windows:
//   int * aryN = NULL;
//   Watch expression: aryN, [((AryHdr *)((uint8_t *)aryN - 8))->len]

#define ARY_LEN(_a) ((_a) ? ARY__HDR(_a)->len : 0)
#define ARY_CAP(_a) ((_a) ? ARY__HDR(_a)->cap : 0)
#define ARY_END(_a) ((_a) + ARY_LEN(_a))
#define ARY_TAIL(_a) (ASSERT(!ARY_EMPTY(_a)), ((_a) + ARY_LEN(_a) - 1))
#define ARY_FREE(_a) ((_a) ? (Ary__Free(_a, sizeof(*(_a))), (_a) = NULL) : 0)
#define ARY_PUSH(_a, x) ((ARY__ENSURECAP((_a), ARY_LEN(_a) + 1)), (_a)[ARY__HDR(_a)->len++] = (x))
#define ARY_POP(_a) ((ARY_LEN(_a) > 0) ? ARY__HDR(_a)->len-- : 0)
#define ARY_EMPTY(_a) (ARY_LEN(_a) == 0)
#define ARY_CLEAR(_a) ((_a) ? ARY__HDR(_a)->len = 0 : 0)

// Helpers

#define ARY__HDR(_a) ((AryHdr *)((uint8_t *)(_a) - offsetof(AryHdr, aB)))
#define ARY__ENSURECAP(_a, n) (((n) <= ARY_CAP(_a)) ? 0 : ((_a) = Ary__AllocGrow((_a), (n), sizeof(*(_a)))))

// BB (matthewp) XCode complains about this being unused in every header that includes this, but doesn't
//  use ARY_PUSH or other allocation functions. Find a better way to resolve this than using 'inline'

static inline void * Ary__AllocGrow(void * ary, uint32_t newCapMin, uint32_t elemSize)
{
	uint32_t curCap = ARY_CAP(ary);
	uint32_t newCap = curCap;

	do
	{
		newCap = CARY_GROW_CAPACITY(newCap);
	}
	while (newCap < newCapMin);

	ASSERTMSG(newCap <= (SIZE_MAX - offsetof(AryHdr, aB)) / elemSize, "Array allocation size overflow");

	size_t sizeAlloc = newCap * elemSize + offsetof(AryHdr, aB);
	AryHdr * pHdr;

	if (ary)
	{
		size_t sizeAllocOld = curCap * elemSize + offsetof(AryHdr, aB);
		pHdr = xrealloc(ARY__HDR(ary), sizeAllocOld, sizeAlloc);
	}
	else
	{
		pHdr = xmalloc(sizeAlloc);
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
		xfree(ARY__HDR(ary), sizeAllocOld);
	}
}
