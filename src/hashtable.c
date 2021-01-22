#include "hashtable.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  if (size < 4) size = 4;
  size = next_pow2(size); // always ensure power of 2

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

// Resets an iterator, finding the first item if it exists
HashTableItem* ht_iter_reset(HashTableIterator* iter) {
  iter->item = NULL;
  iter->slotidx = 0;
  for (size_t i = 0; i < iter->table->size; ++i) {
    HashTableItem* item = iter->table->slots[i];
    if (item) {
      iter->item = item;
      iter->slotidx = i;
      break;
    }
  }
  return iter->item;
}

// Creates an iterator for a hashtable
HashTableIterator* ht_create_iter(HashTable* table) {
  HashTableIterator* iter = malloc(sizeof *iter);
  if (!iter) {
    perror("malloc iter");
    exit(EXIT_FAILURE);
  }
  iter->table = table;
  ht_iter_reset(iter);// find first item
  return iter;
}

// Resets an iterator, finding the first item if it exists
HashTableItem* ht_iter_current(HashTableIterator* iter) {
  return iter->item;
}

// Resets an iterator, finding the first item if it exists
HashTableItem* ht_iter_next(HashTableIterator* iter) {
  if (!iter->item) return NULL;
  if (iter->item->next) {
    iter->item = iter->item->next;
    return iter->item;
  }
  iter->slotidx++; // next slot
  while (iter->slotidx < iter->table->size) {
    HashTableItem* item = iter->table->slots[iter->slotidx];
    if (item) {
      iter->item = item;
      return item;
    }
    iter->slotidx++; // next slot
  }
  iter->item = NULL;
  iter->slotidx = 0;
  return NULL;
}

// Frees an item depending on their types, if the key or value members
// need freeing that needs to happen here too
static inline void ht_free_item(HashTableItem* restrict item) {
  free(item->key);
  free(item);
}

// Frees the whole hashtable
void ht_free(HashTable* restrict table) {
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
static size_t ht_hash(size_t size, const char* restrict str) {
  uint64_t hash = 0xcbf29ce484222325; // FNV_offset_basis
  while (*str) hash = (hash ^ (uint8_t)*str++) * 0x100000001b3; // FNV_prime

  return hash & (size - 1); // fit to table. we know size is power of 2
}

HashTableItem* ht_rehash(HashTable* restrict table, size_t new_size,
                         HashTableItem* restrict old_item) {
  if (new_size < 4) new_size = 4;
  new_size = next_pow2(new_size); // always ensure power of 2

  HashTableItem*  new_item = old_item;
  HashTableItem** nslots   = calloc(new_size, sizeof(HashTableItem*));
  if (!nslots) {
    perror("calloc nslots");
    exit(EXIT_FAILURE);
  }

  for (size_t i = 0; i < table->size; i++) {
    HashTableItem* item = table->slots[i];
    while (item) {
      HashTableItem*  next  = item->next; // save next
      HashTableItem** nslot = &nslots[ht_hash(new_size, item->key)];
      item->next            = *nslot; // push into new list
      *nslot                = item;
      if (item == old_item) new_item = item;
      item = next;
    }
  }
  free(table->slots);
  table->size  = new_size;
  table->slots = nslots;
  return new_item;
}

static HashTableItem* ht_grow(HashTable* restrict     table,
                              HashTableItem* restrict old_item) {
  table->itemcount++;
  HashTableItem* new_item = old_item;
  if (table->itemcount * 100 / table->size > 80)
    new_item = ht_rehash(table, table->size * 2, old_item);
  return new_item;
}

static void ht_shrink(HashTable* restrict table) {
  table->itemcount--;
  if (table->size > 4 && table->itemcount * 100 / table->size < 20)
    ht_rehash(table, table->size / 2, NULL); // no item to track
}

// Inserts an item (or updates if exists)
static inline HashTableItem** ht_find_slot(const HashTable* restrict table,
                                           ht_key_t                  key) {
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
HashTableItem* ht_insert(HashTable* restrict table, ht_key_t key,
                         ht_value_t value) {
  HashTableItem** slot = ht_find_slot(table, key);
  HashTableItem*  item = *slot;
  if (item) {
    item->value = value; // update value, free old value if needed
    return item;
  }
  *slot = ht_create_item(key, value); // new entry
  return ht_grow(table, *slot);       // dynamic resizing
}

// Deletes an item from the table
void ht_delete(HashTable* restrict table, ht_key_t key) {
  HashTableItem** slot = ht_find_slot(table, key);
  HashTableItem*  item = *slot;
  if (item) {
    *slot = item->next; // remove item from linked list
    ht_free_item(item);
    ht_shrink(table);
    return;
  }
}

// Searches the key in the hashtable
// and returns NULL ptr if it doesn't exist
HashTableItem* ht_get(const HashTable* restrict table, ht_key_t key) {
  return *ht_find_slot(table, key);
}

// increments the value for a key or inserts with value = 1
// specialised for ht_value_t=int and faster than search then update.
HashTableItem* ht_get_or_create(HashTable* restrict table, ht_key_t key,
                                ht_value_t value) {
  HashTableItem** slot = ht_find_slot(table, key);
  if (*slot) return *slot;
  *slot = ht_create_item(key, value); // not found, init with supplied value
  return ht_grow(table, *slot);       // dynamic resizing
}

HashTableItem* ht_inc(HashTable* restrict table, ht_key_t key) {
  HashTableItem* item = ht_get_or_create(table, key, 0);
  item->value++;
  return item;
}

// debug printing. customise printf format strings by key & value types
void ht_print(const HashTable* restrict table) {
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
HashTableItem** ht_create_flat_view(const HashTable* restrict table) {
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
