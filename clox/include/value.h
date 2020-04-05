//
//  value.h
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "common.h"



typedef enum ValueType
{
	VAL_BOOL,
	VAL_NIL,
	VAL_NUMBER,
	VAL_OBJ,
} ValueType;

#ifndef VALUES_USE_NAN_BOXING
#	define VALUES_USE_NAN_BOXING 1
#endif // VALUES_USE_NAN_BOXING

#if VALUES_USE_NAN_BOXING

typedef uint64_t Value;

typedef union DoubleUnion
{
	uint64_t n;
	double d;
} DoubleUnion;

CASSERT(sizeof(uint64_t) == sizeof(double));
CASSERT(sizeof(Value) == sizeof(uint64_t));

#define _VAL_QNAN			0x7ffc000000000000ull
#define _VAL_SIGN_BIT		0x8000000000000000ull

#define _VAL_NIL			1u
#define _VAL_FALSE			2u
#define _VAL_TRUE			3u

#define _VAL_PTR_MASK		(_VAL_QNAN | _VAL_SIGN_BIT)

inline static double _ValueCastToNum(Value value)
{
	DoubleUnion u;
	u.n = value;
	return u.d;
}

inline static Value _NumCastToValue(double num)
{
	DoubleUnion u;
	u.d = num;
	return u.n;
}

#define VAL_TYPE(value)		(_ValueType(value))

#define NIL_VAL				(Value)(_VAL_QNAN | _VAL_NIL)
#define FALSE_VAL			(Value)(_VAL_QNAN | _VAL_FALSE)
#define TRUE_VAL			(Value)(_VAL_QNAN | _VAL_TRUE)
#define BOOL_VAL(value)		((value) ? TRUE_VAL : FALSE_VAL)
#define NUMBER_VAL(value)	_NumCastToValue(value)
#define OBJ_VAL(object)		(Value)(_VAL_SIGN_BIT | _VAL_QNAN | (uint64_t)(uintptr_t)(object))

#define IS_BOOL(value)		(((value) & FALSE_VAL) == FALSE_VAL)
#define IS_NIL(value)		((value) == NIL_VAL)
#define IS_NUMBER(value)	(((value) & _VAL_QNAN) != _VAL_QNAN)
#define IS_OBJ(value)		(((value) & _VAL_PTR_MASK) == _VAL_PTR_MASK)

#define AS_BOOL(value)		((value) == TRUE_VAL)
#define AS_NUMBER(value)	_ValueCastToNum(value)
#define AS_OBJ(value)		((Obj*)(uintptr_t)((value) & ~_VAL_PTR_MASK))

inline static ValueType _ValueType(Value value)
{
	if (IS_NUMBER(value))
	{
		return VAL_NUMBER;
	}
	else if (IS_OBJ(value))
	{
		return VAL_OBJ;
	}
	else if (IS_BOOL(value))
	{
		return VAL_BOOL;
	}
	else
	{
		return VAL_NIL;
	}
}

#else // !VALUES_USE_NAN_BOXING

typedef struct
{
	ValueType type;
	union
	{
		bool boolean;
		double number;
		Obj * obj;
	} as;
} Value;

CASSERT(sizeof(Value) == 2 * sizeof(void *));

#define VAL_TYPE(value)		((value).type)

#define IS_BOOL(value)		((value).type == VAL_BOOL)
#define IS_NIL(value)		((value).type == VAL_NIL)
#define IS_NUMBER(value)	((value).type == VAL_NUMBER)
#define IS_OBJ(value)		((value).type == VAL_OBJ)

#define AS_BOOL(value)		((value).as.boolean)
#define AS_NUMBER(value)	((value).as.number)
#define AS_OBJ(value)		((value).as.obj)

#define BOOL_VAL(value)		((Value){ VAL_BOOL, { .boolean = value } })
#define FALSE_VAL			((Value){ VAL_BOOL, { .boolean = false } })
#define TRUE_VAL			((Value){ VAL_BOOL, { .boolean = true } })
#define NIL_VAL				((Value){ VAL_NIL, { .number = 0 } })
#define NUMBER_VAL(value)	((Value){ VAL_NUMBER, { .number = value } })
#define OBJ_VAL(object)		((Value){ VAL_OBJ, { .obj = &object->obj } })

#endif // !VALUES_USE_NAN_BOXING

bool valuesEqual(Value a, Value b);
void printValue(Value value);
