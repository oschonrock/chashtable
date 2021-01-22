#include "hashtable.c"
#include "hashtable.h"
#include "unity.h"
#include <string.h>
#include <stdlib.h>

static HashTable *ht;

void setUp(void) { ht = ht_create(4); }

void tearDown(void) { ht_free(ht); }

void test_pow2() {
  TEST_ASSERT_EQUAL(1, next_pow2(0));
  TEST_ASSERT_EQUAL(1, next_pow2(1));
  TEST_ASSERT_EQUAL(4, next_pow2(3));
  TEST_ASSERT_EQUAL(16, next_pow2(16));
  TEST_ASSERT_EQUAL(1024, next_pow2(768));
  TEST_ASSERT_EQUAL(1024, next_pow2(1023));
  TEST_ASSERT_EQUAL(1024, next_pow2(1024));
}

void test_insert_delete(void) {
  TEST_ASSERT_EQUAL(0, ht->itemcount);

  HashTableItem* item = ht_insert(ht, "aaa", 10); // @2
  TEST_ASSERT_EQUAL(1, ht->itemcount);
  TEST_ASSERT_NOT_NULL(item);
  TEST_ASSERT_EQUAL(10, item->value);
  ht_delete(ht, "aaa");
  TEST_ASSERT_NULL(ht_get(ht, "aaa"));
  TEST_ASSERT_EQUAL(0, ht->itemcount);

  
  ht_insert(ht, "bbb", 10); // @1 
  ht_insert(ht, "jjj", 10); // @1
  ht_insert(ht, "rrr", 10); // @1
  TEST_ASSERT_EQUAL(3, ht->itemcount);

  ht_delete(ht, "jjj");
  TEST_ASSERT_EQUAL(2, ht->itemcount);
  TEST_ASSERT_NULL(ht_get(ht, "jjj"));
  ht_delete(ht, "bbb");
  TEST_ASSERT_EQUAL(1, ht->itemcount);
  TEST_ASSERT_NULL(ht_get(ht, "bbb"));
  ht_delete(ht, "rrr");
  TEST_ASSERT_EQUAL(0, ht->itemcount);
  TEST_ASSERT_NULL(ht_get(ht, "rrr"));

  HashTableItem* item2 = ht_get(ht, "aaa");
  TEST_ASSERT_NULL(item2);
}
void test_inc(void) {
  ht_inc(ht, "aaa");
  TEST_ASSERT_EQUAL(1, ht->itemcount);

  ht_inc(ht, "bbb");
  TEST_ASSERT_EQUAL(2, ht->itemcount);
  TEST_ASSERT_EQUAL(4, ht->size);

  ht_inc(ht, "ccc");
  TEST_ASSERT_EQUAL(3, ht->itemcount);
  TEST_ASSERT_EQUAL(4, ht->size);

  ht_inc(ht, "aaa");
  HashTableItem* a = ht_inc(ht, "aaa");
  TEST_ASSERT_NOT_NULL(a);
  TEST_ASSERT_EQUAL(3, a->value);

  HashTableItem* b = ht_inc(ht, "bbb");
  TEST_ASSERT_NOT_NULL(b);
  TEST_ASSERT_EQUAL(2, b->value);

  HashTableItem* c = ht_get(ht, "ccc");
  TEST_ASSERT_NOT_NULL(c);
  TEST_ASSERT_EQUAL(1, c->value);
}

void test_grow_shrink() {
  ht_inc(ht, "aaa");
  ht_inc(ht, "bbb");
  ht_inc(ht, "ccc");
  ht_inc(ht, "ddd"); // > 80% => grow 
  TEST_ASSERT_EQUAL(4, ht->itemcount);
  TEST_ASSERT_EQUAL(8, ht->size);
  ht_delete(ht, "aaa");
  ht_delete(ht, "bbb");
  ht_delete(ht, "ccc"); // down to 12.5 => shrink
  TEST_ASSERT_EQUAL(1, ht->itemcount);
  TEST_ASSERT_EQUAL(4, ht->size);
  ht_delete(ht, "ddd");
  TEST_ASSERT_EQUAL(0, ht->itemcount);
  TEST_ASSERT_EQUAL(4, ht->size); // min 4!
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
  RUN_TEST(test_pow2);
  RUN_TEST(test_insert_delete);
  RUN_TEST(test_inc);
  RUN_TEST(test_grow_shrink);
  RUN_TEST(test_flat_view);
  return UNITY_END();
}
