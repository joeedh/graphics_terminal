#include "alloc.h"
#include "list.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEM_MAGIC1 ('M' | ('~'<<8) | ('C'<<16) | ('1' << 24))
#define MEM_MAGIC2 ('M' | ('G'<<8) | ('~'<<16) | ('2' << 24))

List memblocks = {NULL, NULL};
FILE *memerr_file = NULL;
size_t totalloc = 0;

typedef struct MemNode {
	struct MemNode *next, *prev;

	uint32_t magic1, pad;
	size_t size;
	char *file;
	uint32_t line, magic2;
} MemNode;

void _MEM_seterror(FILE *file, char *sfile, int line) {
	memerr_file = file;
}

void *_MEM_malloc(size_t size, char *file, int line) {
	size_t size2 = size + sizeof(MemNode);
	size2 += 15 - (size2 & 15);

	MemNode *node = malloc(size2);

	node->size = size;

	node->magic1 = MEM_MAGIC1;
	node->magic2 = MEM_MAGIC2;

	node->file = file;
	node->line = line;

	totalloc += size2;

	list_append(&memblocks, node);

	return (void*)(node + 1);
}

void _MEM_printerr(void *vmem, char *msg, char *file, int line) {
	if (memerr_file)
		fprintf(memerr_file, "%s error: %p\n\t\"%s\":%d\n", msg, vmem, file, line);
}

void _MEM_free(void *vmem, char *file, int line) {
	MemNode *mem = vmem;
	size_t size2;

	if (!_MEM_check(vmem, file, line)) {
		_MEM_printerr(vmem, "MEM_free", file, line);
		return;
	}

	mem--;
	
	size2 = mem->size + sizeof(MemNode);
	size2 += 15 - (size2 & 15);
	totalloc -= size2;

	mem->magic1 = mem->magic2 = 0; //invalidate block
	
	list_remove(&memblocks, mem);
	free(mem);
}

void *_MEM_calloc(size_t size, char *file, int line) {
	void *ret = _MEM_malloc(size, file, line);

	memset(ret, 0, size);

	return ret;
}

size_t _MEM_len(void *vmem, char *file, int line) {
	MemNode *mem = vmem;

	if (!_MEM_check(vmem, file, line)) {
		_MEM_printerr(vmem, "MEM_free", file, line);
		return 0;
	}

	mem--;
	return mem->size;
}

int _MEM_check(void *vmem, char *file, int line) {
	MemNode *mem = vmem;

	if (!mem) {
		return 0;
	}

	mem--;

	if (!mem) {
		return 0;
	}

	if (mem->magic1 != MEM_MAGIC1 || mem->magic2 != MEM_MAGIC2) {
		return 0;
	}

	return 1;
}

void MEM_printblocks(FILE *file) {
	MemNode *mem;

	if (!memblocks.first) {
		return; //no memory blocks
	}

	fprintf(file, "\n===========Memory Blocks (leaks)=============\n");

	for (mem=memblocks.first; mem; mem=mem->next) {
		if (!MEM_check(mem+1)) {
			fprintf(file, "ERROR: corrupted memory block list!\n");
			break;
		}

		fprintf(file, "block 0x%p of size 0x%p, allocated from:\n\t%s:%d\n", (void*)(mem+1), (void*)mem->size, mem->file, mem->line);
	}

	fprintf(file, "\nLeaked memory: 0x%p\n", (void*)totalloc);
}
