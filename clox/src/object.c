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



#define ALLOCATE_OBJ(type, objectType) (type*)allocateObject(sizeof(type), objectType)

static Obj * allocateObject(size_t size, ObjType type)
{
	Obj * object = (Obj*)xrealloc(NULL, 0, size);
	object->type = type;

	object->next = vm.objects;
	vm.objects = object;

	return object;
}

static ObjString * allocateString(int cCh)
{
	// NOTE (matthewp) Automatically adds the extra null terminator

	size_t cB = offsetof(ObjString, chars) + cCh + 1;
	ObjString * string = (ObjString*)allocateObject(cB, OBJ_STRING);
	string->length = cCh;
	string->chars[cCh] = '\0';

	if (cCh > 0)
	{
		string->chars[0] = '\0';
	}

	return string;
}

ObjString * concatStrings(const ObjString * pStrA, const ObjString * pStrB)
{
	int length = pStrA->length + pStrB->length;

	ObjString * result = allocateString(length);

	memcpy(result->chars, pStrA->chars, pStrA->length);
	memcpy(result->chars + pStrA->length, pStrB->chars, pStrB->length);

	return result;
}

ObjString * copyString(const char * chars, int length)
{
	ObjString * string = allocateString(length);
	memcpy(string->chars, chars, length);

	return string;
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
