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
hash_table* ht_create(size_t size) {
  if (size < 4) size = 4;
  size = next_pow2(size); // always ensure power of 2

  hash_table* table = malloc(sizeof *table);
  if (!table) {
    perror("malloc table");
    exit(EXIT_FAILURE);
  }
  table->slots = calloc(size, sizeof(hash_table_item*));
  if (!table->slots) {
    perror("calloc slots");
    exit(EXIT_FAILURE);
  }
  table->size      = size;
  table->itemcount = 0;
  return table;
}

// Creates a new HashTableItem
static hash_table_item* ht_create_item(ht_key_t key, ht_value_t value) {
  hash_table_item* item = malloc(sizeof *item);
  if (!item) {
    perror("malloc item");
    exit(EXIT_FAILURE);
  }
  item->key   = strdup(key); // take a copy
  item->value = value;
  item->next  = NULL;
  return item;
}

// Frees an item depending on their types, if the key or value members
// need freeing that needs to happen here too
static inline void ht_free_item(hash_table_item* restrict item) {
  free(item->key);
  free(item);
}

// Frees the whole hashtable
void ht_free(hash_table* restrict table) {
  // free the HashTableItems in the linked lists
  for (size_t i = 0; i < table->size; i++) {
    hash_table_item* item = table->slots[i];
    while (item) {
      hash_table_item* next = item->next;
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

hash_table_item* ht_rehash(hash_table* restrict table, size_t new_size,
                           hash_table_item* restrict old_item) {
  if (new_size < 4) new_size = 4;
  new_size = next_pow2(new_size); // always ensure power of 2

  hash_table_item*  new_item = old_item;
  hash_table_item** nslots   = calloc(new_size, sizeof(hash_table_item*));
  if (!nslots) {
    perror("calloc nslots");
    exit(EXIT_FAILURE);
  }

  for (size_t i = 0; i < table->size; i++) {
    hash_table_item* item = table->slots[i];
    while (item) {
      hash_table_item*  next  = item->next; // save next
      hash_table_item** nslot = &nslots[ht_hash(new_size, item->key)];
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

static hash_table_item* ht_grow(hash_table* restrict      table,
                                hash_table_item* restrict old_item) {
  table->itemcount++;
  hash_table_item* new_item = old_item;
  if (table->itemcount * 100 / table->size > 80)
    new_item = ht_rehash(table, table->size * 2, old_item);
  return new_item;
}

static void ht_shrink(hash_table* restrict table) {
  table->itemcount--;
  if (table->size > 4 && table->itemcount * 100 / table->size < 20)
    ht_rehash(table, table->size / 2, NULL); // no item to track
}

// finds a slot for a key, either existing or new
// "slot" here is defined as a "primary slot" in the hashtable
// OR a ->next pointer in one of the items in the linked list
// hanging off such a primary slot
// this keeps the logic the same and allows reuse across insert,
// delete, inc, dec and get
static inline hash_table_item** ht_find_slot(const hash_table* restrict table,
                                             ht_key_t                   key) {
  hash_table_item** slot = &table->slots[ht_hash(table->size, key)];
  hash_table_item*  item = *slot;
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
hash_table_item* ht_insert(hash_table* restrict table, ht_key_t key,
                           ht_value_t value) {
  hash_table_item** slot = ht_find_slot(table, key);
  hash_table_item*  item = *slot;
  if (item) {
    item->value = value; // update value, free old value if needed
    return item;
  }
  *slot = ht_create_item(key, value); // new entry
  return ht_grow(table, *slot);       // dynamic resizing
}

// Deletes an item from the table
void ht_delete(hash_table* restrict table, ht_key_t key) {
  hash_table_item** slot = ht_find_slot(table, key);
  hash_table_item*  item = *slot;
  if (item) {
    *slot = item->next; // remove item from linked list
    ht_free_item(item);
    ht_shrink(table);
    return;
  }
}

// Searches the key in the hashtable
// and returns NULL ptr if it doesn't exist
hash_table_item* ht_get(const hash_table* restrict table, ht_key_t key) {
  return *ht_find_slot(table, key);
}

// increments the value for a key or inserts with value = 1
// specialised for ht_value_t=int and faster than search then update.
hash_table_item* ht_get_or_create(hash_table* restrict table, ht_key_t key,
                                  ht_value_t value) {
  hash_table_item** slot = ht_find_slot(table, key);
  if (*slot) return *slot;
  *slot = ht_create_item(key, value); // not found, init with supplied value
  return ht_grow(table, *slot);       // dynamic resizing
}

hash_table_item* ht_inc(hash_table* restrict table, ht_key_t key) {
  hash_table_item* item = ht_get_or_create(table, key, 0);
  item->value++;
  return item;
}

hash_table_item* ht_dec(hash_table* restrict table, ht_key_t key) {
  hash_table_item* item = ht_get_or_create(table, key, 0);
  item->value--;
  return item;
}

// debug printing. customise printf format strings by key & value types
void ht_print(const hash_table* restrict table) {
  printf("\n---- Hash Table ---\n");
  for (size_t i = 0; i < table->size; i++) {
    printf("@%zu: ", i);
    hash_table_item* item = table->slots[i];
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
hash_table_item** ht_create_flat_view(const hash_table* restrict table) {
  hash_table_item** itemview =
      calloc(table->itemcount, sizeof(hash_table_item*));
  if (!itemview) {
    perror("calloc itemview");
    exit(EXIT_FAILURE);
  }
  hash_table_item** curritem = itemview;
  for (size_t i = 0; i < table->size; i++) {
    hash_table_item* item = table->slots[i];
    while (item) {
      *curritem++ = item;
      item        = item->next;
    }
  }
  return itemview;
}

// Creates an iterator for a hashtable
hash_table_iterator* ht_create_iter(const hash_table* restrict table) {
  hash_table_iterator* iter = malloc(sizeof *iter);
  if (!iter) {
    perror("malloc iter");
    exit(EXIT_FAILURE);
  }
  iter->table = table;
  ht_iter_reset(iter);// find first item
  return iter;
}

// Resets an iterator, finding the first item if it exists
hash_table_item* ht_iter_reset(hash_table_iterator* restrict iter) {
  iter->item = NULL;
  iter->slotidx = 0;
  for (size_t i = 0; i < iter->table->size; ++i) {
    hash_table_item* item = iter->table->slots[i];
    if (item) {
      iter->item = item;
      iter->slotidx = i;
      break;
    }
  }
  return iter->item;
}

// Returns current item pointed to by iterator
hash_table_item* ht_iter_current(hash_table_iterator* restrict iter) {
  return iter->item;
}

// Moves iterator to point at the next item
hash_table_item* ht_iter_next(hash_table_iterator* restrict iter) {
  if (!iter->item) return NULL; // at end already
  
  if (iter->item->next) { // along linked list
    iter->item = iter->item->next;
    return iter->item;
  }
  iter->slotidx++; // next slot
  while (iter->slotidx < iter->table->size) {
    hash_table_item* item = iter->table->slots[iter->slotidx];
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
