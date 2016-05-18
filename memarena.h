#ifndef _MEMARENA_H
#define _MEMARENA_H

#include <stdint.h>
#include <stddef.h>

#include "list.h"

//simple alloc-many, free-all allocator.

typedef struct MemChunk {
	struct MemChunk *next, *prev;
	size_t size, used;
} MemChunk;

typedef struct MemArena {
	List chunks;
	MemChunk *cur;
	size_t csize;
} MemArena;

MemArena *memarena_new(int min_chunksize);
void *memarena_malloc(MemArena *arena, size_t size);
void *memarena_calloc(MemArena *arena, size_t size);
void memarena_free(MemArena *arena);

#endif /* _MEMARENA_H */
