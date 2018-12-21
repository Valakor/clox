//
//  object.h
//  clox
//
//  Created by Matthew Pohlmann on 9/24/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "common.h"
#include "value.h"



typedef enum
{
	OBJ_STRING,
} ObjType;

struct sObj
{
	ObjType type;
	struct sObj * next;
};

struct sObjString
{
	Obj obj;
	uint32_t hash;
	int length;
	const char * aChars;
};

extern ObjString * concatStrings(const ObjString * pStrA, const ObjString * pStrB);
extern ObjString * copyString(const char * chars, int length); // Copy into new memory
extern ObjString * takeString(const char * chars, int length); // Take ownership of chars memory
extern void freeString(ObjString ** ppStr);

extern void printObject(Value value);

static inline bool isObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define OBJ_TYPE(value)		(AS_OBJ(value)->type)

#define IS_STRING(value)	isObjType(value, OBJ_STRING)

#define AS_STRING(value)	((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)	(((ObjString*)AS_OBJ(value))->aChars)
