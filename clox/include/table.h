//
//  table.h
//  clox
//
//  Created by Matthew Pohlmann on 12/21/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "common.h"
#include "value.h"



// BB (matthewp) Add support for tables of other key types

typedef struct
{
	ObjString * key;
	Value value;
} Entry;

typedef struct
{
	int count;
	int capacity;
	Entry * aEntries;
} Table;

void initTable(Table * table);
void freeTable(Table * table);

bool tableSet(Table * table, ObjString * key, Value value);
bool tableGet(Table * table, ObjString * key, Value * value);

bool tableDelete(Table * table, ObjString * key);

void tableAddAll(Table * from, Table * to);
ObjString * tableFindString(Table * table, const char * aCh, int length, uint32_t hash);
