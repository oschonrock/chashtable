#include "hashtable.h"
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

bool parseul(const char* str, size_t* val) {
  char* end; // NOLINT
  errno = 0;
  *val  = strtoul(str, &end, 0);
  if (end == str || *end != '\0' || errno == ERANGE) return false;
  return true;
}

static inline size_t minul(size_t a, size_t b) { return a < b ? a : b; }

int rand_range(int start, int end) {
  return start + rand() / (RAND_MAX / (end - start + 1) + 1);
}

typedef struct timespec timespec;

static void print_elapsed(timespec start, timespec end, char* label) {
  timespec e;
  if ((end.tv_nsec - start.tv_nsec) < 0) {
    e.tv_sec  = end.tv_sec - start.tv_sec - 1;
    e.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
  } else {
    e.tv_sec  = end.tv_sec - start.tv_sec;
    e.tv_nsec = end.tv_nsec - start.tv_nsec;
  }
  printf("%s: %ld.%09lds\n", label, e.tv_sec, e.tv_nsec);
}

static void parse_and_map(FILE* fp, size_t limit) {
  HashTable* ht = ht_create(32 * 1024);

  timespec start, stop;
  clock_gettime(CLOCK_MONOTONIC, &start);
  
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
        ht_inc(ht, word); // record (takes a copy)
        word_ptr = word;     // restart new word
      }
      ++bufptr; // next char from buf
    }
  }
  
  clock_gettime(CLOCK_MONOTONIC, &stop);

  // build a flat view of the hashtable items
  HashTableItem** view = ht_create_flat_view(ht);

  // now sort that view and print the top 10
  qsort(view, ht->itemcount, sizeof(HashTableItem*), cmp_ht_items);
  size_t wordcnt = 0;
  for (size_t i = 0; i < ht->itemcount; i++) wordcnt += view[i]->value;

  printf("\n%s\n----------------------------\n", "file wordcounts");
  printf("%-17s %'10zu\n", "Word count", wordcnt);
  printf("%-17s %'10zu\n", "Unique count", ht->itemcount);
  printf("%-17s %'10zu\n", "Slot count", ht->size);
  print_elapsed(start, stop, "read + parse + ht_inc()");

  printf("\nTop %zu\n----------------------------\n", limit);
  for (size_t i = 0; i < minul(limit, ht->itemcount); i++)
    printf("%-13s %'6d %6.2f%%\n", view[i]->key, view[i]->value,
           100.0 * view[i]->value / wordcnt);

  free(view);
  ht_free(ht);

}

static void rand_ht_bench(size_t limit) {
  srand(1); // fixed seed

  const size_t str_count      = 1000000;
  const size_t str_min_length = 1;
  const size_t str_max_length = 3;

  char** strs = malloc(str_count * sizeof(char*));
  if (!strs) {
    perror("malloc str pointers");
    exit(EXIT_FAILURE);
  }
  for (size_t s = 0; s < str_count; ++s) {
    size_t length = rand_range(str_min_length, str_max_length);
    strs[s]       = malloc((length + 1) * sizeof(char));
    if (!strs[s]) {
      perror("malloc str");
      exit(EXIT_FAILURE);
    }
    for (size_t chr = 0; chr < length; ++chr)
      strs[s][chr] = (char)rand_range('A', 'Z');
    strs[s][length] = '\0';
  }

  HashTable* ht = ht_create(32 * 1024);

  timespec start, stop;
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (size_t i = 0; i < str_count; ++i) ht_inc(ht, strs[i]);
  clock_gettime(CLOCK_MONOTONIC, &stop);
  
  HashTableItem** view = ht_create_flat_view(ht);
  qsort(view, ht->itemcount, sizeof(HashTableItem*), cmp_ht_items);
  size_t wordcnt = 0;
  for (size_t i = 0; i < ht->itemcount; i++) wordcnt += view[i]->value;

  printf("\n%s\n----------------------------\n", "rand bench test");
  printf("%-17s %'10zu\n", "Word count", wordcnt);
  printf("%-17s %'10zu\n", "Unique count", ht->itemcount);
  printf("%-17s %'10zu\n", "Slot count", ht->size);

  print_elapsed(start, stop, "ht_inc()");
  
  printf("\nTop %zu\n----------------------------\n", limit);
  for (size_t i = 0; i < minul(limit, ht->itemcount); i++)
    printf("%-13s %'6d %6.2f%%\n", view[i]->key, view[i]->value,
           100.0 * view[i]->value / wordcnt);

  free(view);
  ht_free(ht);
  for (size_t i = 0; i < str_count; ++i) free(strs[i]);
  free(strs);
}

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
  size_t limit = 10;
  if (argc > 2) {
    if (!parseul(argv[2], &limit)) {
      fputs(usage, stderr);
      fprintf(stderr, "Invalid `limit`: \"%s\"\n", argv[2]);
      fclose(fp);
      exit(EXIT_FAILURE);
    }
  }
  setlocale(LC_NUMERIC, ""); // for thousands separator

  rand_ht_bench(limit);
  parse_and_map(fp, limit);
  
  fclose(fp);
}
