#include "hashtable.h"
#include "unity.h"
#include <string.h>
#include <stdlib.h>

static HashTable *ht;

void setUp(void) { ht = ht_create(4); }

void tearDown(void) { ht_free(ht); }

void test_insert_delete(void) {
  TEST_ASSERT_EQUAL(0, ht->itemcount);

  ht_insert(ht, "aaa", 10); // @2
  TEST_ASSERT_EQUAL(1, ht->itemcount);
  TEST_ASSERT_EQUAL(1, ht->scount);

  HashTableItem *item = ht_search(ht, "aaa");
  TEST_ASSERT_NOT_NULL(item);
  TEST_ASSERT_EQUAL(10, item->value);

  ht_insert(ht, "mmm", 10); // @2
  
  ht_insert(ht, "bbb", 10); // @1 
  ht_insert(ht, "jjj", 10); // @1
  ht_insert(ht, "rrr", 10); // @1

  ht_insert(ht, "lll", 10); // @3
  TEST_ASSERT_EQUAL(6, ht->itemcount);
  TEST_ASSERT_EQUAL(3, ht->scount);

  ht_delete(ht, "bbb");
  TEST_ASSERT_EQUAL(3, ht->scount);
  ht_delete(ht, "jjj");
  TEST_ASSERT_EQUAL(3, ht->scount);
  ht_delete(ht, "rrr");
  TEST_ASSERT_EQUAL(2, ht->scount);
  ht_delete(ht, "mmm");
  TEST_ASSERT_EQUAL(2, ht->scount);
  ht_delete(ht, "aaa");
  TEST_ASSERT_EQUAL(1, ht->scount);
  ht_delete(ht, "lll");
  TEST_ASSERT_EQUAL(0, ht->itemcount);
  TEST_ASSERT_EQUAL(0, ht->scount);

  HashTableItem *item2 = ht_search(ht, "aaa");
  TEST_ASSERT_NULL(item2);
}
void test_inc(void) {
  ht_inc(ht, "aaa");
  TEST_ASSERT_EQUAL(1, ht->itemcount);
  TEST_ASSERT_EQUAL(1, ht->scount);

  ht_inc(ht, "bbb");
  TEST_ASSERT_EQUAL(2, ht->itemcount);
  TEST_ASSERT_EQUAL(2, ht->scount);
  TEST_ASSERT_EQUAL(4, ht->size);

  ht_inc(ht, "ccc");
  TEST_ASSERT_EQUAL(3, ht->itemcount);
  TEST_ASSERT_EQUAL(3, ht->scount);
  TEST_ASSERT_EQUAL(4, ht->size);

  HashTableItem *a = ht_search(ht, "aaa");
  HashTableItem *b = ht_search(ht, "bbb");
  HashTableItem *c = ht_search(ht, "ccc");

  ht_inc(ht, "aaa");
  ht_inc(ht, "aaa");
  TEST_ASSERT_NOT_NULL(a);
  TEST_ASSERT_EQUAL(3, a->value);

  ht_inc(ht, "bbb");
  TEST_ASSERT_NOT_NULL(b);
  TEST_ASSERT_EQUAL(2, b->value);

  TEST_ASSERT_NOT_NULL(c);
  TEST_ASSERT_EQUAL(1, c->value);
}

void test_grow() {
  ht_inc(ht, "aaa");
  ht_inc(ht, "bbb");
  ht_inc(ht, "ccc");
  ht_inc(ht, "ddd"); // grow on this one
  TEST_ASSERT_EQUAL(4, ht->itemcount);
  TEST_ASSERT_EQUAL(4, ht->scount);
  TEST_ASSERT_EQUAL(8, ht->size);
}

void test_flat_view() {
  ht_inc(ht, "aaa");
  ht_inc(ht, "bbb");
  ht_inc(ht, "ccc2");
  HashTableItem** view = ht_create_flat_view(ht);
  
  TEST_ASSERT_EQUAL(0, strcmp("bbb", view[0]->key));
  TEST_ASSERT_EQUAL(0, strcmp("aaa", view[1]->key));
  TEST_ASSERT_EQUAL(0, strcmp("ccc2", view[2]->key));
  free(view);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_insert_delete);
  RUN_TEST(test_inc);
  RUN_TEST(test_grow);
  RUN_TEST(test_flat_view);
  return UNITY_END();
}
