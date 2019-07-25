//
//  object.h
//  clox
//
//  Created by Matthew Pohlmann on 9/24/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "common.h"
#include "chunk.h"
#include "value.h"



typedef enum
{
	OBJ_FUNCTION,
	OBJ_NATIVE,
	OBJ_STRING,
} ObjType;

struct sObj
{
	ObjType type;
	struct sObj * next;
};

typedef struct
{
	Obj obj;
	int arity;
	Chunk chunk;
	ObjString * name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value * args);

typedef struct
{
	Obj obj;
	NativeFn function;
} ObjNative;

struct sObjString
{
	Obj obj;
	uint32_t hash;
	int length;
	const char * aChars;
};

extern ObjFunction * newFunction(void);
extern ObjNative * newNative(NativeFn function);

extern ObjString * concatStrings(const ObjString * pStrA, const ObjString * pStrB);
extern ObjString * copyString(const char * chars, int length); // Copy into new memory
extern ObjString * takeString(const char * chars, int length); // Take ownership of chars memory

extern void printObject(Value value);

static inline bool isObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define OBJ_TYPE(value)		(AS_OBJ(value)->type)

#define IS_FUNCTION(value)	isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)	isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)	isObjType(value, OBJ_STRING)

#define AS_FUNCTION(value)	((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)	(((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)	((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)	(((ObjString*)AS_OBJ(value))->aChars)
