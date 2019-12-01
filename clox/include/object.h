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



typedef enum eObjType
{
	OBJ_STRING,
	OBJ_UPVALUE,
	OBJ_FUNCTION,
	OBJ_CLOSURE,
	OBJ_NATIVE,
} ObjType;

typedef struct sObj
{
	ObjType type;
	bool isMarked;
	struct sObj * next;
} Obj;

typedef struct sObjString
{
	Obj obj;
	uint32_t hash;
	int length;
	const char * aChars;
} ObjString;

typedef struct sObjUpvalue
{
	Obj obj;
	Value * location;
	Value closed;
	struct sObjUpvalue * next;
} ObjUpvalue;

typedef struct sObjFunction
{
	Obj obj;
	int arity;
	int upvalueCount;
	Chunk chunk;
	ObjString * name;
} ObjFunction;

typedef struct sObjClosure
{
	Obj obj;
	ObjFunction * function;
	ObjUpvalue ** upvalues;
	int upvalueCount;
} ObjClosure;

typedef bool (*NativeFn)(int argCount, Value * args);

typedef struct sObjNative
{
	Obj obj;
	NativeFn function;
} ObjNative;



extern ObjUpvalue * newUpvalue(Value * slot);
extern ObjFunction * newFunction(void);
extern ObjClosure * newClosure(ObjFunction * function);
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

#define IS_UPVALUE(value)	isObjType(value, OBJ_UPVALUE)
#define IS_FUNCTION(value)	isObjType(value, OBJ_FUNCTION)
#define IS_CLOSURE(value)	isObjType(value, OBJ_CLOSURE)
#define IS_NATIVE(value)	isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)	isObjType(value, OBJ_STRING)

#define AS_UPVALUE(value)	((ObjUpvalue*)AS_OBJ(value))
#define AS_FUNCTION(value)	((ObjFunction*)AS_OBJ(value))
#define AS_CLOSURE(value)	((ObjClosure*)AS_OBJ(value))
#define AS_NATIVE(value)	(((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)	((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)	(((ObjString*)AS_OBJ(value))->aChars)
