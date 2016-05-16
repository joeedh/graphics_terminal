#ifndef _ALLOC_H
#define _ALLOC_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MEM_malloc(size) _MEM_malloc(size, __FILE__, __LINE__)
#define MEM_calloc(size) _MEM_calloc(size, __FILE__, __LINE__)
#define MEM_free(mem) _MEM_free(mem, __FILE__, __LINE__)
#define MEM_len(mem) _MEM_len(mem, __FILE__, __LINE__)
#define MEM_check(mem) _MEM_check(mem, __FILE__, __LINE__)
void MEM_printblocks(FILE *file);
#define MEM_seterror(file) _MEM_seterror(file, __FILE__, __LINE__)

void *_MEM_malloc(size_t size, char *file, int line);
void _MEM_free(void *mem, char *file, int line);
void *_MEM_calloc(size_t size, char *file, int line);
size_t _MEM_len(void *mem, char *file, int line);
int _MEM_check(void *mem, char *file, int line);
void _MEM_seterror(FILE *file, char *sfile, int line);

#endif /* _ALLOC_H */
