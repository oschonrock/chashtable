#pragma once

#include <stddef.h>

typedef char* ht_key_t;
typedef int   ht_value_t;

// key => value plus pointer to next item for hash collisions
typedef struct hash_table_item hash_table_item;
struct hash_table_item {
  ht_key_t       key;
  ht_value_t     value;
  hash_table_item* next;
};

// Array of pointers to HashTableItems, plus counters
typedef struct hash_table hash_table;
struct hash_table {
  hash_table_item** slots;     // hash slots into which items are filled
  size_t          size;      // how many slots exist
  size_t          itemcount; // how many items exist
};

hash_table* ht_create(size_t size);
void        ht_free(hash_table* table);

hash_table_item* ht_insert(hash_table* restrict table, ht_key_t key,
                           ht_value_t value);

void ht_delete(hash_table* restrict table, ht_key_t key);

hash_table_item* ht_get(const hash_table* restrict table, ht_key_t key);

hash_table_item* ht_get_or_create(hash_table* restrict table, ht_key_t key,
                                  ht_value_t value);

hash_table_item* ht_inc(hash_table* restrict table, ht_key_t key);
hash_table_item* ht_dec(hash_table* restrict table, ht_key_t key);

hash_table_item* ht_rehash(hash_table* restrict table, size_t new_size,
                           hash_table_item* restrict old_item);

hash_table_item** ht_create_flat_view(const hash_table* restrict table);

void ht_print(const hash_table* restrict table);

typedef struct hash_table_iterator hash_table_iterator;
struct hash_table_iterator {
  const hash_table*    table;
  hash_table_item*     item;
  size_t         slotidx;
};

hash_table_iterator* ht_create_iter(const hash_table* restrict table);
hash_table_item*     ht_iter_reset(hash_table_iterator* iter);
hash_table_item*     ht_iter_current(hash_table_iterator* iter);
hash_table_item*     ht_iter_next(hash_table_iterator* iter);
