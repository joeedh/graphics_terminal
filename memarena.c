#include "memarena.h"
#include "alloc.h"
#include <string.h>

MemArena *memarena_new(int min_chunksize) {
	MemArena *ma = MEM_calloc(sizeof(*ma));

	ma->cur = MEM_malloc(min_chunksize + sizeof(MemChunk));
	ma->cur->used = 0;
	ma->csize = ma->cur->size = min_chunksize;

	list_append(&ma->chunks, ma->cur);

	return ma;
}

void *memarena_malloc(MemArena *ma, size_t size) {
	if (ma->cur->used + size >= ma->cur->size) {
		size_t newsize = ma->csize < size ? size : ma->csize;
		MemChunk *chunk = MEM_malloc(sizeof(MemChunk) + newsize);

		chunk->used = 0;
		chunk->size = newsize;

		ma->cur = chunk;
		list_append(&ma->chunks, chunk);
	}

	char *ret = (unsigned char*)(ma->cur+1) + ma->cur->used;
	ma->cur->used += size;

	return (void*) ret;
}

void *memarena_calloc(MemArena *ma, size_t size) {
	void *ret = memarena_malloc(ma, size);
	memset(ret, 0, size);

	return ret;
}

void memarena_free(MemArena *ma) {
	MemChunk *ch, *next;

	for (ch=ma->chunks.first; ch; ch=next) {
		next = ch->next;

		MEM_free(ch);
	}

	MEM_free(ma);
}
