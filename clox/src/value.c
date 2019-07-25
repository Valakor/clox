//
//  value.c
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "value.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "object.h"



bool valuesEqual(Value a, Value b)
{
	ValueType typeA = VAL_TYPE(a);
	ValueType typeB = VAL_TYPE(b);

	if (typeA != typeB)
		return false;

	switch (typeA)
	{
		case VAL_BOOL:		return AS_BOOL(a) == AS_BOOL(b);
		case VAL_NIL:		return true;
		case VAL_NUMBER:	return AS_NUMBER(a) == AS_NUMBER(b);
		case VAL_OBJ:
		{
			return AS_OBJ(a) == AS_OBJ(b);
		}
	}

	// Unreachable

	ASSERT(false);
	return false;
}

static inline void printNumber(double v)
{
	double i;
	double f = modf(v, &i);

	if (f == 0.0 && i >= LLONG_MIN && i <= LLONG_MAX)
	{
		printf("%lld", (long long)i);
	}
	else
	{
		printf("%f", v);
	}
}

void printValue(Value value)
{
	switch (VAL_TYPE(value))
	{
		case VAL_BOOL:		printf(AS_BOOL(value) ? "true" : "false"); break;
		case VAL_NIL:		printf("nil"); break;
		case VAL_NUMBER:	printNumber(AS_NUMBER(value)); break;
		case VAL_OBJ:		printObject(value); break;
	}
}
