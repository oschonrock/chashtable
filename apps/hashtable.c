#include "hashtable.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static inline bool ht_is_alpha(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
static inline char ht_tolower(char c) { return (c < 'a') ? c + 'a' - 'A' : c; }

static int cmp_ht_items(const void* a, const void* b) {
  int a_val = (*(HashTableItem**)a)->value;
  int b_val = (*(HashTableItem**)b)->value;
  if (a_val == b_val) return 0;
  return a_val < b_val ? 1 : -1;
}

int main() {
  // some tests
  HashTable* ht = ht_create(4);
  // ht_insert(ht, "1", 10);
  // ht_insert(ht, "2", 20);
  // ht_insert(ht, "Hel3", 30);
  // ht_insert(ht, "Cau4", 40);
  // ht_print(ht);
  // ht_insert(ht, "Cau6", 60); // grow!
  // ht_print(ht);
  // ht_inc(ht, "11"); // increment
  // ht_inc(ht, "21"); // increment
  // ht_inc(ht, "31"); // increment
  // ht_inc(ht, "41"); // increment
  // ht_inc(ht, "51"); // increment
  // ht_inc(ht, "61"); // increment
  // ht_inc(ht, "71"); // increment
  // ht_inc(ht, "81"); // increment and grow again
  // ht_print(ht);

  // ht_print_search(ht, "1");
  // ht_print_search(ht, "2");
  // ht_print_search(ht, "3");
  // ht_print_search(ht, "Hel3");
  // ht_print_search(ht, "Cau4");

  // ht_insert(ht, "Cau6", 61); // update
  // ht_inc(ht, "1");           // increment
  // ht_inc(ht, "Hel3");        // increment
  // ht_inc(ht, "new");         // increment
  // ht_print(ht);

  // ht_delete(ht, "Hel3");
  // ht_delete(ht, "Cau6");
  // ht_delete(ht, "1");
  // ht_print(ht);

  ht_free(ht);
  // end of tests

  // shakespeare demo
  FILE* fp = fopen("data/shakespeare.txt", "re");
  if (!fp) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }
  ht = ht_create(32 * 1024);

#define BUFSIZE 1024
#define WORDSIZE 50
  char  buf[BUFSIZE];
  char  word[WORDSIZE];
  char* word_ptr = word;
  while (!feof(fp) && !ferror(fp)) {
    size_t bread  = fread(buf, 1, BUFSIZE, fp);
    char*  bufptr = buf;
    while ((bufptr < buf + bread)) {
      char c = *bufptr;
      if (ht_is_alpha(c)) {
        *word_ptr++ = ht_tolower(c);
        if (word_ptr - word == WORDSIZE - 1) { // -1 for NULL terminator
          fputs("word too long. terminating\n", stderr);
          exit(EXIT_FAILURE);
        }
      } else if (word_ptr > word) {
        *word_ptr++ = '\0'; // terminate word
        ht_inc(ht, word);   // record (takes a copy)
        word_ptr = word;    // restart new word
      }
      ++bufptr; // next char from buf
    }
  }
  fclose(fp);

  // build a flat view of the hashtable items
  HashTableItem** flatview = ht_create_flat_view(ht);

  // now sort that view and print the top 10
  qsort(flatview, ht->itemcount, sizeof(HashTableItem*), cmp_ht_items);
  for (size_t i = 0; i < 10; i++)
    printf("%s => %d\n", flatview[i]->key, flatview[i]->value);

  free(flatview);
  ht_free(ht);
}
