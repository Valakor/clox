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
} ValueType;

typedef struct
{
	ValueType type;
	union
	{
		bool boolean;
		double number;
	} as;
} Value;

#define IS_BOOL(value)		((value).type == VAL_BOOL)
#define IS_NIL(value)		((value).type == VAL_NIL)
#define IS_NUMBER(value)	((value).type == VAL_NUMBER)

#define AS_BOOL(value)		((value).as.boolean)
#define AS_NUMBER(value)	((value).as.number)

#define BOOL_VAL(value)		((Value){ VAL_BOOL, { .boolean = value } })
#define NIL_VAL				((Value){ VAL_NIL, { .number = 0 } })
#define NUMBER_VAL(value)	((Value){ VAL_NUMBER, { .number = value } })

bool valuesEqual(Value a, Value b);
void printValue(Value value);
