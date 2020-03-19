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
#include "table.h"



typedef enum ObjType
{
	OBJ_STRING,
	OBJ_UPVALUE,
	OBJ_FUNCTION,
	OBJ_CLASS,
	OBJ_INSTANCE,
	OBJ_CLOSURE,
	OBJ_BOUND_METHOD,
	OBJ_NATIVE,
} ObjType;

typedef struct Obj
{
	// ObjType type		: 8;
	// bool isMarked	: 1;
	// Obj* next		: 55;

	uint64_t n;
} Obj;

static_assert(sizeof(Obj) == 8, "");

typedef struct ObjString
{
	Obj obj;
	uint32_t hash;
	int length;
	const char * aChars;
} ObjString;

typedef struct ObjUpvalue
{
	Obj obj;
	Value * location;
	Value closed;
	struct ObjUpvalue * next;
} ObjUpvalue;

typedef struct ObjFunction
{
	Obj obj;
	int arity;
	int upvalueCount;
	Chunk chunk;
	ObjString * name;
} ObjFunction;

typedef struct ObjClass
{
	Obj obj;
	ObjString * name;
	Table methods;
	struct ObjClosure * init; // Not a GC root because also in methods table
} ObjClass;

typedef struct ObjInstance
{
	Obj obj;
	ObjClass * klass;
	Table fields;
} ObjInstance;

typedef struct ObjClosure
{
	Obj obj;
	ObjFunction * function;
	ObjUpvalue ** upvalues;
	int upvalueCount;
} ObjClosure;

typedef struct ObjBoundMethod
{
	Obj obj;
	Value receiver;
	ObjClosure* method;
} ObjBoundMethod;

typedef bool (*NativeFn)(int argCount, Value * args);

typedef struct ObjNative
{
	Obj obj;
	NativeFn function;
} ObjNative;



static inline ObjType getObjType(Obj* obj)
{
	return (ObjType)(obj->n & 0xffull);
}

static inline void setObjType(Obj* obj, ObjType type)
{
	obj->n = (obj->n & ~0xffull) | (uint64_t)type;
}

static inline bool getIsMarked(Obj* obj)
{
	return (obj->n & 0x100ull) != 0;
}

static inline void setIsMarked(Obj* obj, bool isMarked)
{
	obj->n = (obj->n & ~0x100ull) | ((isMarked) ? 0x100ull : 0);
}

static inline Obj* getObjNext(Obj* obj)
{
	return (Obj*)(obj->n >> 9);
}

static inline void setObjNext(Obj* obj, Obj* next)
{
	obj->n = (obj->n & 0x1ffull) | ((uint64_t)next << 9);
}

static inline void initObj(Obj* obj, ObjType type, Obj* next)
{
	obj->n = ((uint64_t)next << 9) | (uint64_t)type;
}



extern ObjUpvalue * newUpvalue(Value * slot);
extern ObjFunction * newFunction(void);
extern ObjClass * newClass(ObjString * name);
extern ObjInstance * newInstance(ObjClass * klass);
extern ObjClosure * newClosure(ObjFunction * function);
extern ObjBoundMethod * newBoundMethod(Value receiver, ObjClosure* method);
extern ObjNative * newNative(NativeFn function);

extern ObjString * concatStrings(const ObjString * pStrA, const ObjString * pStrB);
extern ObjString * copyString(const char * chars, int length); // Copy into new memory
extern ObjString * takeString(const char * chars, int length); // Take ownership of chars memory

extern void printObject(Value value);

static inline bool isObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && getObjType(AS_OBJ(value)) == type;
}

#define OBJ_TYPE(value)			(getObjType(AS_OBJ(value)))

#define IS_UPVALUE(value)		isObjType(value, OBJ_UPVALUE)
#define IS_FUNCTION(value)		isObjType(value, OBJ_FUNCTION)
#define IS_CLASS(value)			isObjType(value, OBJ_CLASS)
#define IS_INSTANCE(value)		isObjType(value, OBJ_INSTANCE)
#define IS_CLOSURE(value)		isObjType(value, OBJ_CLOSURE)
#define IS_BOUND_METHOD(value)	isObjType(value, OBJ_BOUND_METHOD)
#define IS_NATIVE(value)		isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)		isObjType(value, OBJ_STRING)

#define AS_UPVALUE(value)		((ObjUpvalue*)AS_OBJ(value))
#define AS_FUNCTION(value)		((ObjFunction*)AS_OBJ(value))
#define AS_CLASS(value)			((ObjClass*)AS_OBJ(value))
#define AS_INSTANCE(value)		((ObjInstance*)AS_OBJ(value))
#define AS_CLOSURE(value)		((ObjClosure*)AS_OBJ(value))
#define AS_BOUND_METHOD(value)	((ObjBoundMethod*)AS_OBJ(value))
#define AS_NATIVE(value)		(((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)		((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)		(((ObjString*)AS_OBJ(value))->aChars)
