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
	object->type = type;

	object->next = vm.objects;
	vm.objects = object;

	return object;
}

static ObjString * allocateString(const char * aCh, int cCh, uint32_t hash)
{
	ObjString * pStr = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	pStr->hash = hash;
	pStr->length = cCh;
	pStr->aChars = aCh;

	tableSet(&vm.strings, pStr, NIL_VAL);

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

void freeString(ObjString ** ppStr)
{
	ObjString * pStr = *ppStr;

	CARY_FREE(char, (void *)pStr->aChars, pStr->length + 1);
	FREE(ObjString, pStr);

	*ppStr = NULL;
}

void printObject(Value value)
{
	switch (OBJ_TYPE(value))
	{
		case OBJ_STRING:
			printf("%s", AS_CSTRING(value));
			break;
	}
}
