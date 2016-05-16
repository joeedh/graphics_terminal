#ifndef _LIST_H
#define _LIST_H

typedef struct Link {
	struct Link *next, *prev;
} Link;

typedef struct LinkNode {
	struct LinkNode *next, *prev;
	void *data;
} LinkNode;

typedef struct List {
	void *first, *last;
} List;

void list_append(List *list, void *vitem);
void list_remove(List *list, void *vitem);

#endif /* _LIST_H */
