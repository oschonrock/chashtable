typedef struct pair pair;
struct pair {
  char* s;
  int   val;
};

static int cmp_pairs(const void* a, const void* b) {
  int a_val = ((pair*)a)->val;
  int b_val = ((pair*)b)->val;
  if (a_val == b_val) return 0;
  return a_val < b_val ? 1 : -1;
}


  pair* pairs = calloc(ht->itemcount, sizeof(pair));
  if (!strs) {
    perror("malloc mapped strs pointers");
    exit(EXIT_FAILURE);
  }

  clock_gettime(CLOCK_MONOTONIC, &start);
  for (size_t i = 0; i < str_count; ++i) {
    pair* p = &pairs[0];
    while (p->s) {
      if (strcmp(p->s, strs[i]) == 0) {
        ++p->val;
        break;
      }
      ++p;
    }
    if (!p->s) {
      p->s   = strdup(strs[i]);
      p->val = 1;
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &stop);
  printf("vec(): %.9fs\n", timediff(start, stop));
  qsort(pairs, ht->itemcount, sizeof(pair), cmp_pairs);
  printf("\nTop %zu\n----------------------------\n", limit);
  for (size_t i = 0; i < minul(limit, ht->itemcount); i++)
    printf("%-13s %'6d %6.2f%%\n", pairs[i].s, pairs[i].val, 100.0 * pairs[i].val / wordcnt);

  for (size_t i = 0; i < ht->itemcount; ++i) free(pairs[i].s);
  free(pairs);

