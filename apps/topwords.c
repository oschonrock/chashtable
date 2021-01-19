#include "hashtable.h"
#include <errno.h>
#include <limits.h>
#include <locale.h>
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

static void parse_and_map(FILE* fp, HashTable* table) {
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
        *word_ptr++ = '\0';  // terminate word
        ht_inc(table, word); // record (takes a copy)
        word_ptr = word;     // restart new word
      }
      ++bufptr; // next char from buf
    }
  }
}

bool parse_long(const char* str, long* val) {
  char* end; // NOLINT
  errno = 0;
  *val  = strtol(str, &end, 0);
  if (end == str || *end != '\0' || errno == ERANGE) return false;
  return true;
}

bool parse_int(const char* str, int* val) {
  long long_val; // NOLINT
  bool result = parse_long(str, &long_val);
  if (result && long_val >= INT_MIN && long_val <= INT_MAX) {
    *val = (int)long_val;
    return true;
  }
  return false;
}

static inline size_t uminl(size_t a, size_t b) { return a < b ? a : b; }

int main(int argc, char** argv) {
  char usage[50];
  snprintf(usage, 50, "Usage: %s filename [limit]\n", argv[0]);
  if (argc < 2) {
    fputs(usage, stderr);
    exit(EXIT_FAILURE);
  }
  FILE* fp = fopen(argv[1], "re");
  if (!fp) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }
  long limit = 10;
  if (argc > 2) {
    if (!parse_long(argv[2], &limit)) {
      fputs(usage, stderr);
      fprintf(stderr, "Invalid `limit`: \"%s\"\n", argv[2]);
      fclose(fp);
      exit(EXIT_FAILURE);
    }
  }
  HashTable* ht = ht_create(32 * 1024);
  parse_and_map(fp, ht);
  fclose(fp);

  // build a flat view of the hashtable items
  HashTableItem** view = ht_create_flat_view(ht);

  // now sort that view and print the top 10
  qsort(view, ht->itemcount, sizeof(HashTableItem*), cmp_ht_items);
  size_t wordcnt = 0;
  for (size_t i = 0; i < ht->itemcount; i++) wordcnt += view[i]->value;

  setlocale(LC_NUMERIC, ""); // for thousands separator
  printf("\n%s\n----------------------------\n", argv[1]);
  printf("%-17s %'10zu\n", "Word count", wordcnt);
  printf("%-17s %'10zu\n\n", "Unique count", ht->itemcount);

  printf("Top %zu\n----------------------------\n", limit);
  for (size_t i = 0; i < uminl(limit, ht->itemcount); i++)
    printf("%-13s %'6d %6.2f%%\n", view[i]->key, view[i]->value,
           100.0 * view[i]->value / wordcnt);

  free(view);
  ht_free(ht);
}
