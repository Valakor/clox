//
//  value.h
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "common.h"



typedef enum
{
	VAL_BOOL,
	VAL_NIL,
	VAL_NUMBER,
	VAL_OBJ,
} ValueType;

#ifndef USE_SMALL_VALUE
#	define USE_SMALL_VALUE 1
#endif // USE_SMALL_VALUE

#if USE_SMALL_VALUE

typedef double Value;

CASSERT(sizeof(Value) == sizeof(void *));

#define _VALUE_TYPE_MASK	0x7ffc000000000000ull
#define _VALUE_PTR_MASK		0x0000ffffffffffffull

#define _VAL_BOOL			0x7ffc000000000000ull
#define _VAL_FALSE			0x7ffc000000000000ull
#define _VAL_TRUE			0x7ffc000000000001ull
#define _VAL_NIL			0x7ffd000000000000ull
#define _VAL_OBJ			0x7fff000000000000ull

#define _VAL_MASK			0x7fff000000000000ull

inline static uint64_t valueCastToUn(Value value)
{
	union
	{
		Value		value;
		uint64_t	d;
	} u;

	u.value = value;

	return u.d;
}

inline static Value valueCastToValue(uint64_t d)
{
	union
	{
		Value		value;
		uint64_t	d;
	} u;

	u.d = d;

	return u.value;
}

inline static ValueType valueType(Value value)
{
	uint64_t d = valueCastToUn(value);

	if ((d & _VALUE_TYPE_MASK) == _VALUE_TYPE_MASK)
	{
		// Something that's not a number

		const unsigned long long mask = 0x3ull;
		const unsigned long long shift = 0x30ull;

		unsigned long long n = ((d >> shift) & mask);

		return (ValueType)n;
	}
	else
	{
		// A number

		return VAL_NUMBER;
	}
}

#define VAL_TYPE(value)		(valueType(value))

#define IS_BOOL(value)		((valueCastToUn(value) & ~0x1ull) == _VAL_BOOL)
#define IS_NIL(value)		(valueCastToUn(value) == _VAL_NIL)
#define IS_NUMBER(value)	((valueCastToUn(value) & _VALUE_TYPE_MASK) != _VALUE_TYPE_MASK)
#define IS_OBJ(value)		((valueCastToUn(value) & ~_VALUE_PTR_MASK) == _VAL_OBJ)

#define AS_BOOL(value)		((bool)(valueCastToUn(value) & 0x1))
#define AS_NUMBER(value)	((value))
#define AS_OBJ(value)		((Obj*)(valueCastToUn(value) & _VALUE_PTR_MASK))

#define BOOL_VAL(value)		valueCastToValue((value) ? _VAL_TRUE : _VAL_FALSE)
#define NIL_VAL				valueCastToValue(_VAL_NIL)
#define NUMBER_VAL(value)	((Value) (value))
#define OBJ_VAL(object)		valueCastToValue(_VAL_OBJ | (uint64_t)(void*)(object))

#else

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
#define NIL_VAL				((Value){ VAL_NIL, { .number = 0 } })
#define NUMBER_VAL(value)	((Value){ VAL_NUMBER, { .number = value } })
#define OBJ_VAL(object)		((Value){ VAL_OBJ, { .obj = &object->obj } })

#endif

bool valuesEqual(Value a, Value b);
void printValue(Value value);
