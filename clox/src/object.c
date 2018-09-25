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

ObjString * allocateString(int numChars)
{
	// NOTE (matthewp) Doesn't put anything in the chars array!

	size_t cB = offsetof(ObjString, chars) + numChars;
	ObjString * string = (ObjString*)allocateObject(cB, OBJ_STRING);

	return string;
}

ObjString * copyString(const char * chars, int length)
{
	ObjString * string = allocateString(length + 1);
	memcpy(string->chars, chars, length);
	string->chars[length] = '\0';
	string->length = length;

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
