#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "hash.h"
#include "alloc.h"

int test_hash() {
	return 0;
}

#if 0
int main(int argc, char **argv) {
	int ret = 0;
	int i=0;
	uintptr_t key;
	intptr_t val;
	HashTable _table, *table=&_table;
	HashIter iter;

	MEM_seterror(stderr);
	hashtable_init(table);

	for (i=0; i<25; i++) {
		char *val = MEM_malloc(32);
		sprintf_s(val, 16, ":%d", i);

		hashtable_set(table, i, (intptr_t)val);
	}

	for (int i=0; i<25; i++) {
		char *str = (char*) hashtable_get(table, i);

		printf("%d:%d %s %p\n", i, (int)hashtable_has(table, i, NULL), hashtable_get(table, i), hashtable_strhash(str, strlen(str)) % table->size);
		fflush(stdout);
	}

	hashtable_iter(table, &iter, &key, &val);

	for (; !hashtable_idone(&iter); hashtable_inext(&iter, &key, &val)) {
		//printf("%p %p\n", key, val);
		MEM_free((void*)val);
	}

	hashtable_release(table);

	MEM_printblocks(stderr);

	system("pause");
	return ret;
}
#endif