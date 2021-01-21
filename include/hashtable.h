#pragma once

#include <stddef.h>

typedef char* ht_key_t;
typedef int   ht_value_t;

// key => value plus pointer to next item for hash collisions
typedef struct HashTableItem HashTableItem;
struct HashTableItem {
  ht_key_t       key;
  ht_value_t     value;
  HashTableItem* next;
};

// Array of pointers to HashTableItems, plus counters
typedef struct HashTable HashTable;
struct HashTable {
  HashTableItem** slots;     // hash slots into which items are filled
  size_t          size;      // how many slots exist
  size_t          itemcount; // how many items exist
};

HashTable* ht_create(size_t size);
void       ht_free(HashTable* table);

HashTableItem* ht_insert(HashTable* restrict table, ht_key_t key, ht_value_t value);
void ht_delete(HashTable* restrict table, ht_key_t key);

HashTableItem* ht_get(HashTable* restrict table, ht_key_t key);

HashTableItem* ht_get_or_create(HashTable* restrict table, ht_key_t key,
                                ht_value_t value);

HashTableItem* ht_inc(HashTable* restrict table, ht_key_t key);

void ht_print(HashTable* restrict table);

HashTableItem** ht_create_flat_view(HashTable* restrict table);
