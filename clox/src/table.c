//
//  table.c
//  clox
//
//  Created by Matthew Pohlmann on 12/21/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include <stdlib.h>
#include <string.h>

#include "table.h"
#include "memory.h"
#include "object.h"
#include "array.h"

#define TABLE_MAX_LOAD 0.75



void initTable(Table * table)
{
	CLEAR_STRUCT(*table);
}

void freeTable(Table * table)
{
	CARY_FREE(Entry, table->aEntries, table->capacity);
	initTable(table);
}

static Entry * findEntry(Entry * aEntries, int capacity, ObjString * key)
{
	uint32_t index = key->hash % capacity;
	Entry * tombstone = NULL;

	for (;;)
	{
		Entry * entry = &aEntries[index];

		if (entry->key == NULL)
		{
			if (IS_NIL(entry->value))
			{
				// Empty entry

				return tombstone != NULL ? tombstone : entry;
			}
			else
			{
				// We found a tombstone

				if (tombstone == NULL) tombstone = entry;
			}
		}
		else if (entry->key == key)
		{
			return entry;
		}

		index = (index + 1) % capacity;
	}

	// Unreachable
}

static void adjustCapacity(Table * table, int capacity)
{
	Entry * aEntries = CARY_ALLOCATE(Entry, capacity);

	for (int i = 0; i < capacity; ++i)
	{
		aEntries[i].key = NULL;
		aEntries[i].value = NIL_VAL;
	}

	table->count = 0;

	for (int i = 0; i < table->capacity; ++i)
	{
		Entry * entry = &table->aEntries[i];
		if (entry->key == NULL) continue;

		Entry * dest = findEntry(aEntries, capacity, entry->key);
		dest->key = entry->key;
		dest->value = entry->value;
		table->count++;
	}

	CARY_FREE(Entry, table->aEntries, table->capacity);

	table->aEntries = aEntries;
	table->capacity = capacity;
}

bool tableSet(Table * table, ObjString * key, Value value)
{
	if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
	{
		int capacity = CARY_GROW_CAPACITY(table->capacity);
		adjustCapacity(table, capacity);
	}

	Entry * entry = findEntry(table->aEntries, table->capacity, key);

	// Only increase count if this is a new key and the key is not a tombstone
	//  (Tombstones are included in the count already)

	bool isNewKey = (entry->key == NULL);
	if (isNewKey && IS_NIL(entry->value)) table->count++;

	entry->key = key;
	entry->value = value;

	return isNewKey;
}

bool tableGet(Table * table, ObjString * key, Value * value)
{
	if (table->aEntries == NULL) return false;

	Entry * entry = findEntry(table->aEntries, table->capacity, key);
	if (entry->key == NULL) return false;

	*value = entry->value;
	return true;
}

bool tableDelete(Table * table, ObjString * key)
{
	if (table->count == 0) return false;

	// Find the entry.

	Entry * entry = findEntry(table->aEntries, table->capacity, key);
	if (entry->key == NULL) return false;

	// Place a tombstone in the entry

	entry->key = NULL;
	entry->value = BOOL_VAL(true);

	return true;
}

void tableAddAll(Table * from, Table * to)
{
	for (int i = 0; i < from->capacity; ++i)
	{
		Entry * entry = &from->aEntries[i];
		if (entry->key != NULL)
		{
			tableSet(to, entry->key, entry->value);
		}
	}
}

ObjString * tableFindString(Table * table, const char * aCh, int length, uint32_t hash)
{
	// If the table is empty, we definitely won't find it

	if (table->aEntries == NULL) return NULL;

	// Figure out where to insert in the table. Use open addressing and basic linear probing

	uint32_t index = hash % table->capacity;

	for (;;)
	{
		Entry * entry = &table->aEntries[index];

		// BB (matthewp) Return Entry* instead (like findEntry does) so that we immediately know where
		//  to insert this string when doing a string interning operation in object.c::allocateString.
		//  This avoids doing the bucket probe twice.

		if (entry->key == NULL)
		{
			if (IS_NIL(entry->value))
				return NULL;
		}
		else if (entry->key->length == length &&
			entry->key->hash == hash &&
			memcmp(entry->key->aChars, aCh, length) == 0)
		{
			// Found it

			return entry->key;
		}

		// Try the next slot

		index = (index + 1) % table->capacity;
	}

	// Unreachable
}

void markTable(Table* table)
{
	for (int i = 0; i < table->capacity; i++)
	{
		Entry* entry = &table->aEntries[i];
		markObject((Obj*)entry->key);
		markValue(entry->value);
	}
}

void tableRemoveWhite(Table* table)
{
	for (int i = 0; i < table->capacity; i++)
	{
		Entry* entry = &table->aEntries[i];

		if (entry->key != NULL && !getIsMarked(&entry->key->obj))
		{
			tableDelete(table, entry->key);
		}
	}
}
