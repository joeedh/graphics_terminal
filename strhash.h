#ifndef _STRHASH_H
#define _STRHASH_H

//todo: use c++ to implement hash table variants
//simple hash table that avoids allocating on heap for smaller tables
#include "hash.h"

void strhash_init(HashTable *ht);
HashTable *strhash_new();

//does not call free on ht itself, in case you allocated it some other way
void strhash_release(HashTable *ht);
void strhash_releaseitems(HashTable *ht); //calls MEM_free on values too

//like strhash_release, but also calls MEM_free(ht)
void strhash_free(HashTable *ht);

void strhash_set(HashTable *ht, char *key, intptr_t val);
intptr_t strhash_get(HashTable *ht, char *key);
int strhash_has(HashTable *ht, char *key, intptr_t *val_out);
int strhash_remove(HashTable *ht, char *key);

void strhash_iter(HashTable *table, HashIter *iter, char **key_out, intptr_t *val_out);
int strhash_inext(HashIter *iter, char **key_out, intptr_t *val_out);
int strhash_idone(HashIter *iter);

uintptr_t strhash_strhash(unsigned char *str, int len);

#endif /* _STRHASH_H */
