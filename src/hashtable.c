#include "hashtable.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Creates a new HashTableItem
static HashTableItem* ht_create_item(ht_key_t key, ht_value_t value) {
  HashTableItem* item = malloc(sizeof *item);
  if (!item) {
    perror("malloc item");
    exit(EXIT_FAILURE);
  }
  item->key   = strdup(key); // take a copy
  item->value = value;
  item->next  = NULL;
  return item;
}

static uint64_t next_pow2(uint64_t n) {
  // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
  --n;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n |= n >> 32;
  ++n;
  n += (n == 0);
  return n;
}

// Creates a new HashTable
HashTable* ht_create(size_t size) {
  if (size < 4) {
    fputs("ht_create(): size must be at least 4", stderr);
    exit(EXIT_FAILURE);
  }
  size = next_pow2(size);

  HashTable* table = malloc(sizeof *table);
  if (!table) {
    perror("malloc table");
    exit(EXIT_FAILURE);
  }
  table->slots = calloc(size, sizeof(HashTableItem*));
  if (!table->slots) {
    perror("calloc slots");
    exit(EXIT_FAILURE);
  }
  table->size      = size;
  table->itemcount = 0;
  return table;
}

// Frees an item depending on their types, if the key or value members
// need freeing that needs to happen here too
static void ht_free_item(HashTableItem* item) {
  free(item->key);
  free(item);
}

// Frees the whole hashtable
void ht_free(HashTable* table) {
  // free the HashTableItems in the linked lists
  for (size_t i = 0; i < table->size; i++) {
    HashTableItem* item = table->slots[i];
    while (item) {
      HashTableItem* next = item->next;
      ht_free_item(item);
      item = next;
    }
  }
  // free the array of pointers to HashTableItems
  free(table->slots);
  free(table);
}

// hash function. crucial to efficient operation
// returns value in range [0, size)
// an appropriate hash function for short strings is FNV-1a
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV-1a_hash
static size_t ht_hash(size_t size, const char* str) {
  uint64_t hash = 0xcbf29ce484222325; // FNV_offset_basis
  while (*str) hash = (hash ^ (uint8_t)*str++) * 0x100000001b3; // FNV_prime

  return hash & (size - 1); // fit to table. we know size is power of 2
}

static void ht_grow(HashTable* table) {
  table->itemcount++;
  if (table->itemcount * 100 / table->size >= 80) {
    size_t          nsize  = table->size * 2;
    HashTableItem** nslots = calloc(nsize, sizeof(HashTableItem*));
    if (!nslots) {
      perror("calloc nslots");
      exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < table->size; i++) {
      HashTableItem* item = table->slots[i];
      while (item) {
        HashTableItem*  next  = item->next; // save next
        HashTableItem** nslot = &nslots[ht_hash(nsize, item->key)];
        item->next            = *nslot; // push into new list
        *nslot                = item;
        item                  = next;
      }
    }
    free(table->slots);
    table->size  = nsize;
    table->slots = nslots;
  }
}

// Inserts an item (or updates if exists)
static inline HashTableItem** ht_find_slot(HashTable* table, ht_key_t key) {
  HashTableItem** slot = &table->slots[ht_hash(table->size, key)];
  HashTableItem*  item = *slot;
  while (item) {
    if (strcmp(item->key, key) == 0) {
      return slot;
    }
    slot = &item->next;
    item = *slot;
  }
  return slot;
}

// Inserts an item (or updates if exists)
void ht_insert(HashTable* table, ht_key_t key, ht_value_t value) {
  HashTableItem** slot = ht_find_slot(table, key);
  HashTableItem*  item = *slot;
  if (item) {
    item->value = value; // update value, free old value if needed
    return;
  }
  *slot = ht_create_item(key, value); // new entry
  ht_grow(table);                     // dynamic resizing
}

// increments the value for a key or inserts with value = 1
// specialised for ht_value_t=int and faster than search then update.
void ht_inc(HashTable* table, ht_key_t key) {
  HashTableItem** slot = ht_find_slot(table, key);
  HashTableItem*  item = *slot;
  if (item) {
    ++item->value;
    return;
  }
  *slot = ht_create_item(key, 1); // not found, init with one
  ht_grow(table);                 // dynamic resizing
}

// Deletes an item from the table
void ht_delete(HashTable* table, ht_key_t key) {
  HashTableItem** slot = ht_find_slot(table, key);
  HashTableItem*  item = *slot;
  if (item) {
    *slot = item->next; // remove item from linked list
    ht_free_item(item);
    --table->itemcount;
    return;
  }
}

// Searches the key in the hashtable
// and returns NULL ptr if it doesn't exist
HashTableItem* ht_get(HashTable* table, ht_key_t key) {
  HashTableItem** slot = ht_find_slot(table, key);
  HashTableItem*  item = *slot;
  if (item) {
    if (strcmp(item->key, key) == 0) return item;
  }
  return NULL;
}

// debug printing. customise printf format strings by key & value types
void ht_print(HashTable* table) {
  printf("\n---- Hash Table ---\n");
  for (size_t i = 0; i < table->size; i++) {
    printf("@%zu: ", i);
    HashTableItem* item = table->slots[i];
    while (item) {
      printf("%s => %d | ", item->key, item->value);
      item = item->next;
    }
    printf("\n");
  }
  printf("-------------------\n");
}

// create a flat view (array) of HashTableItem pointers for iterating and/or
// sorting. The start to array of pointers, length table->itemcount, is
// returned.
HashTableItem** ht_create_flat_view(HashTable* table) {
  HashTableItem** itemview = calloc(table->itemcount, sizeof(HashTableItem*));
  if (!itemview) {
    perror("calloc itemview");
    exit(EXIT_FAILURE);
  }
  HashTableItem** curritem = itemview;
  for (size_t i = 0; i < table->size; i++) {
    HashTableItem* item = table->slots[i];
    while (item) {
      *curritem++ = item;
      item        = item->next;
    }
  }
  return itemview;
}
