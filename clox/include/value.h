//
//  value.h
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "common.h"

typedef double Value;

typedef struct
{
	int capacity;
	int count;
	Value * values;
} ValueArray;

void initValueArray(ValueArray * array);
void writeValueArray(ValueArray * array, Value value);
void freeValueArray(ValueArray * array);

void printValue(Value value);
