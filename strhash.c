//TODO: rewrite hash table to use c++ templates

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "strhash.h"
#include "alloc.h"

extern int hashsizes[];
HashTable *strhash_new() {
	HashTable *ret = MEM_malloc(sizeof(*ret));
	strhash_init(ret);

	return ret;
}

void strhash_free(HashTable *ht) {
	strhash_release(ht);
}

void strhash_init(HashTable *ht) {
	int i;

	memset(ht, 0, sizeof(*ht));

	ht->size = hashsizes[0];
	ht->table = malloc(sizeof(struct _hashentry)*ht->size);
	ht->used = 0;
	ht->cursize = 0;

	for (i = 0; i < ht->size; i++) {
		ht->table[i].key = KEY_UNUSED;
		ht->table[i].val = VAL_UNUSED;
	}
}

//does not free HashTable structure itself
void strhash_release(HashTable *ht) {
	if (ht->table != ht->_static_table) {
		MEM_free(ht->table);
	}
}

static __forceinline uintptr_t hashstr(char *str) {
	int ret = *str;
	int mul = 2335;
	//int prime = (1 << 19) - 1;
	int add = 0;
	
	//very crappy string hash

	while (*str) {
		mul = ((int)*str)*100;
		add = (1 + add*(*str)) & ((1 << 19) - 1);

		ret = (ret*mul + add) & ((1 << 19) - 1);
		str++;
	}

	return ret;
}

void strhash_set(HashTable *ht, char *key, intptr_t val) {
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
				strhash_set(ht, oldtable[i].key, oldtable[i].val);
			}
		}

		if (oldtable != ht->_static_table)
			MEM_free(oldtable);

		strhash_set(ht, key, val);

		return;
	}

	uintptr_t hashbase = hashstr(key);

	hash = (hashbase + probe) % ht->size;

	while (ht->table[hash].key != KEY_UNUSED || ht->table[hash].val != VAL_UNUSED) {
		if (!strcmp((char*)ht->table[hash].key, (char*)key)) {
			ht->table[hash].val = val;
			return;
		}

		probe = (probe + 1) * 2;
		hash = (hashbase + probe) % ht->size;
	}

	ht->table[hash].key = (uintptr_t) key;
	ht->table[hash].val = val;
	ht->used++;
}

intptr_t strhash_get(HashTable *ht, char *key) {
	uintptr_t hash, probe = 0;
	uintptr_t hashbase = hashstr(key);

	hash = (hashbase + probe) % ht->size;

	while (ht->table[hash].key != KEY_UNUSED || ht->table[hash].val != VAL_UNUSED) {
		if (!strcmp((char*)ht->table[hash].key, key)) {
			return ht->table[hash].val;
		}

		probe = (probe + 1) * 2;
		hash = (hashbase + probe) % ht->size;
	}

	return 0;
}

int strhash_has(HashTable *ht, char *key, intptr_t *val_out) {
	uintptr_t hash, probe = 0;
	uintptr_t hashbase = hashstr(key);

	hash = (hashbase + probe) % ht->size;

	while (ht->table[hash].key != KEY_UNUSED || ht->table[hash].val != VAL_UNUSED) {
		if (!strcmp((char*)ht->table[hash].key, (char*)key)) {
			if (val_out)
				*val_out = ht->table[hash].val;

			return 1;
		}

		probe = (probe + 1) * 2;
		hash = (hashbase + probe) % ht->size;
	}

	return 0;
}

int strhash_remove(HashTable *ht, char *key) {
	uintptr_t hash, probe = 0;
	uintptr_t hashbase = hashstr(key);

	hash = (hashbase + probe) % ht->size;

	while (ht->table[hash].key != KEY_UNUSED || ht->table[hash].val != VAL_UNUSED) {
		if (!strcmp((char*)ht->table[hash].key, (char*)key)) {

			ht->table[hash].key = KEY_UNUSED;
			ht->table[hash].val = VAL_UNUSED;
			ht->used--;

			return 1;
		}

		probe = (probe + 1) * 2;
		hash = (hashbase + probe) % ht->size;
	}

	return 0;
}

void strhash_iter(HashTable *ht, HashIter *iter, char **key_out, intptr_t *val_out) {
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

int strhash_idone(HashIter *iter) {
	return iter->i >= iter->ht->size;
}

int strhash_inext(HashIter *iter, char **key_out, intptr_t *val_out) {
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

void strhash_releaseitems(HashTable *ht) {
	int i;

	for (i=0; i<ht->size; i++) {
		if (ht->table[i].key != KEY_UNUSED && ht->table[i].val != VAL_UNUSED) {
			MEM_free((void*)ht->table[i].val);
		}
	}

	strhash_release(ht);
}
