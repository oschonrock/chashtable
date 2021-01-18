#include "hashtable.h"
#include "unity.h"

static HashTable *ht;

void setUp(void) { ht = ht_create(4); }

void tearDown(void) { ht_free(ht); }

void test_insert_count_check_delete_count(void) {
  TEST_ASSERT_EQUAL(0, ht->itemcount);

  ht_insert(ht, "key", 10);
  // ht_print(ht);

  TEST_ASSERT_EQUAL(1, ht->itemcount);
  TEST_ASSERT_EQUAL(1, ht->scount);

  HashTableItem *item = ht_search(ht, "key");
  TEST_ASSERT_NOT_NULL(item);
  TEST_ASSERT_EQUAL(10, item->value);

  ht_delete(ht, "key");
  TEST_ASSERT_EQUAL(0, ht->itemcount);
  TEST_ASSERT_EQUAL(0, ht->scount);

  HashTableItem *item2 = ht_search(ht, "key");
  TEST_ASSERT_NULL(item2);
}
void test_inc_count_check(void) {
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

  ht_inc(ht, "aaa");

  HashTableItem *a = ht_search(ht, "aaa");
  HashTableItem *b = ht_search(ht, "bbb");
  HashTableItem *c = ht_search(ht, "ccc");
  TEST_ASSERT_NOT_NULL(a);
  TEST_ASSERT_EQUAL(2, a->value);

  TEST_ASSERT_NOT_NULL(b);
  TEST_ASSERT_EQUAL(1, b->value);

  TEST_ASSERT_NOT_NULL(c);
  TEST_ASSERT_EQUAL(1, c->value);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_insert_count_check_delete_count);
  RUN_TEST(test_inc);
  return UNITY_END();
}
