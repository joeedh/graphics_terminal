#include "stdlib.h"
#include "list.h"

void list_append(List *list, void *vitem) {
	Link *item = vitem;

	if (!list->first) {
		list->first = list->last = item;
		item->next = item->prev = NULL;
	} else {
		item->next = NULL;
		item->prev = list->last;

		((Link*)list->last)->next = item;
		list->last = item;
	}
}

void list_remove(List *list, void *vitem) {
	Link *item = vitem;
	if (item->next) {
		item->next->prev = item->prev;
	}

	if (item->prev) {
		item->prev->next = item->next;
	}

	if (item == list->first) {
		list->first = item->next;
	}

	if (item == list->last) {
		list->last = item->prev;
	}
}
