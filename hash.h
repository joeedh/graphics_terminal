#ifndef _HASH_H
#define _HASH_H

//simple hash table that avoids allocating on heap for smaller tables

#include <stddef.h>
#include <stdint.h>

#define KEY_UNUSED ((1<<30)-2)
#define VAL_UNUSED ((1<<30)-3)

struct _hashentry {
  uintptr_t key;
  intptr_t val;
};

#define STATIC_TABLE_SIZE 17 //379 //must be first value of hash.c:hashsizes array

typedef struct HashTable {
  int size, used, cursize;
  struct _hashentry _static_table[STATIC_TABLE_SIZE], *table;
} HashTable;

typedef struct HashIter {
	HashTable *ht;
	int i;
} HashIter;

void hashtable_init(HashTable *ht);

//does not call free on ht itself, in case you allocated it some other way
void hashtable_release(HashTable *ht);
void hashtable_releaseitems(HashTable *ht); //calls MEM_free on values too

void hashtable_set(HashTable *ht, uintptr_t key, intptr_t val);
intptr_t hashtable_get(HashTable *ht, uintptr_t key);
int hashtable_has(HashTable *ht, uintptr_t key, intptr_t *val_out);
int hashtable_remove(HashTable *ht, uintptr_t key);

void hashtable_iter(HashTable *table, HashIter *iter, uintptr_t *key_out, intptr_t *val_out);
int hashtable_inext(HashIter *iter, uintptr_t *key_out, intptr_t *val_out);
int hashtable_idone(HashIter *iter);

uintptr_t hashtable_strhash(unsigned char *str, int len);

#endif /* _HASH_H */
