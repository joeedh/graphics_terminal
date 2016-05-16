#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hash.h"
#include "alloc.h"

//list of primes, each about 1.5x the previous
//keep in sync with STATIC_TABLE_SIZE in hash.h
int hashsizes[] = {
	/*2, 5, 11,*/ 17, 29, 47, 71,
	107, 163, 251, 379, 569, 857,
	1289, 1949, 2927, 4391, 6599,
	9901, 14867, 22303, 33457, 50207,
	75323, 112997, 169501, 254257,
	381389, 572087, 858149, 1287233,
	1930879, 2896319, 4344479, 6516739,
	9775111, 14662727, 21994111, 32991187
};

void hashtable_init(HashTable *ht) {
	int i;

	memset(ht, 0, sizeof(*ht));

	ht->size = hashsizes[0];
	ht->table = ht->_static_table; //malloc(sizeof(struct _hashentry)*ht->size);
	ht->used = 0;
	ht->cursize = 0;

	for (i = 0; i < ht->size; i++) {
		ht->table[i].key = KEY_UNUSED;
		ht->table[i].val = VAL_UNUSED;
	}
}

//does not free HashTable structure itself
void hashtable_release(HashTable *ht) {
	if (ht->table != ht->_static_table) {
		MEM_free(ht->table);
	}
}

void hashtable_set(HashTable *ht, uintptr_t key, intptr_t val) {
	uintptr_t hash, probe = 0;

	if (ht->used >= ht->size / 3) {
		int newsize = hashsizes[++ht->cursize];
		struct _hashentry *newtable = MEM_malloc(sizeof(*newtable)*newsize), *oldtable = ht->table;
		int i, oldsize = ht->size;

		for (i = 0; i < newsize; i++) {
			newtable[i].key = KEY_UNUSED;
			newtable[i].val = VAL_UNUSED;
		}

		//set up new, bigger hash table
		ht->size = newsize;
		ht->table = newtable;
		ht->used = 0;

		//reinsert old items
		for (i = 0; i < oldsize; i++) {
			if (oldtable[i].key != KEY_UNUSED || oldtable[i].val != VAL_UNUSED) {
				hashtable_set(ht, oldtable[i].key, oldtable[i].val);
			}
		}

		if (oldtable != ht->_static_table)
			MEM_free(oldtable);

		hashtable_set(ht, key, val);

		return;
	}

	hash = (key + probe) % ht->size;
	while (ht->table[hash].key != KEY_UNUSED || ht->table[hash].val != VAL_UNUSED) {
		if (ht->table[hash].key == key) {
			ht->table[hash].val = val;
			return;
		}

		probe = (probe + 1) * 2;
		hash = (key + probe) % ht->size;
	}

	ht->table[hash].key = key;
	ht->table[hash].val = val;
	ht->used++;
}

intptr_t hashtable_get(HashTable *ht, uintptr_t key) {
	uintptr_t hash, probe = 0;
	hash = (key + probe) % ht->size;

	while (ht->table[hash].key != KEY_UNUSED || ht->table[hash].val != VAL_UNUSED) {
		if (ht->table[hash].key == key) {
			return ht->table[hash].val;
		}

		probe = (probe + 1) * 2;
		hash = (key + probe) % ht->size;
	}

	return 0;
}

int hashtable_has(HashTable *ht, uintptr_t key, intptr_t *val_out) {
	uintptr_t hash, probe = 0;
	hash = (key + probe) % ht->size;

	while (ht->table[hash].key != KEY_UNUSED || ht->table[hash].val != VAL_UNUSED) {
		if (ht->table[hash].key == key) {
			if (val_out)
				*val_out = ht->table[hash].val;

			return 1;
		}

		probe = (probe + 1) * 2;
		hash = (key + probe) % ht->size;
	}

	return 0;
}

int hashtable_remove(HashTable *ht, uintptr_t key) {
	uintptr_t hash, probe = 0;
	hash = (key + probe) % ht->size;

	while (ht->table[hash].key != KEY_UNUSED || ht->table[hash].val != VAL_UNUSED) {
		if (ht->table[hash].key == key) {

			ht->table[hash].key = KEY_UNUSED;
			ht->table[hash].val = VAL_UNUSED;
			ht->used--;

			return 1;
		}

		probe = (probe + 1) * 2;
		hash = (key + probe) % ht->size;
	}

	return 0;
}

#define MERSENNE_PRIME 2147483647

uintptr_t hashtable_strhash(unsigned char *str, int len) {
	int i;
	uint64_t ret, mul=234523; //force 32-bit machines to work in uint64_t here

	if (len <= 0 || !str) {
		//eek! empty string!
		return 0;
	}

	ret = str[0];

	//very crappy hashing function based on prng
	for (i=0; i<len; i++) {
		mul = (mul * str[i]) & 65535;
		ret = (ret*mul + (str[i]<<5)) & MERSENNE_PRIME;
	}

	return (uintptr_t)ret;
}

void hashtable_iter(HashTable *ht, HashIter *iter, uintptr_t *key_out, intptr_t *val_out) {
	iter->ht = ht;
	iter->i = 0;

	//fetch first item. . .
	while (iter->i < ht->size && (ht->table[iter->i].key == KEY_UNUSED && ht->table[iter->i].val == VAL_UNUSED)) {
		iter->i++;
	}

	if (iter->i < ht->size) {
		*val_out = ht->table[iter->i].val;
		*key_out = ht->table[iter->i].key;
	}
}

int hashtable_idone(HashIter *iter) {
	return iter->i >= iter->ht->size;
}

int hashtable_inext(HashIter *iter, uintptr_t *key_out, intptr_t *val_out) {
	iter->i++;
	while (iter->i < iter->ht->size && (iter->ht->table[iter->i].key == KEY_UNUSED && iter->ht->table[iter->i].val == VAL_UNUSED)) {
		iter->i++;
	}

	if (iter->i < iter->ht->size) {
		*val_out = iter->ht->table[iter->i].val;
		*key_out = iter->ht->table[iter->i].key;
		
		return 1;
	}

	return 0;
}

void hashtable_releaseitems(HashTable *ht) {
	int i;

	for (i=0; i<ht->size; i++) {
		if (ht->table[i].key != KEY_UNUSED && ht->table[i].val != VAL_UNUSED) {
			MEM_free((void*)ht->table[i].val);
		}
	}

	hashtable_release(ht);
}
