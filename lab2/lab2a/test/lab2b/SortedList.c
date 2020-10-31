#include <stdio.h>
#include <stdlib.h>
#include "SortedList.h"
#include <string.h>
#include <pthread.h>

/**
 * SortedList (and SortedListElement)
 *
 *	A doubly linked list, kept sorted by a specified key.
 *	This structure is used for a list head, and each element
 *	of the list begins with this structure.
 *
 *	The list head is in the list, and an empty list contains
 *	only a list head.  The next pointer in the head points at
 *      the first (lowest valued) elment in the list.  The prev
 *      pointer in the list head points at the last (highest valued)
 *      element in the list.  Thus, the list is circular.
 *
 *      The list head is also recognizable by its NULL key pointer.
 *
 * NOTE: This header file is an interface specification, and you
 *       are not allowed to make any changes to it.
 */

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
	SortedList_t *traverse = list->next; // head of the list
	//list empty
//	printf("enter insert\n");
	while( traverse != list && strcmp(element->key,traverse->key) > 0 ) {
		traverse = traverse->next;
	}

	if (opt_yield & INSERT_YIELD) {
//		printf("Insert yield\n");
		sched_yield();
	}
	element->prev = traverse->prev; 
	traverse->prev->next = element;
	traverse->prev = element;
	element->next = traverse;
	
}

int SortedList_delete( SortedListElement_t *element) {
	
	if(element->next->prev == element && element->prev->next == element) {
		if (opt_yield & DELETE_YIELD) {
//			printf("delete yield\n");
			sched_yield();
		}
		element->prev->next = element->next;
		element->next->prev = element->prev;
		return 0;
	}
	else {
		return 1; //fail
	}
}

SortedListElement_t* SortedList_lookup(SortedList_t *list, const char *key) {
		SortedListElement_t* traverse = list->next;
		//since the head is empty, no need to look up the head key
/*		if (list->key == key) {
			SortedListElement_t* re = list;
			return re;
		}
*/
		while(traverse != list) {
			 if (traverse->key == key) {
				return traverse;	
			}
			if (opt_yield & LOOKUP_YIELD) {
				sched_yield();
			}
			traverse = traverse->next;
		}

	return NULL;

}


int SortedList_length(SortedList_t *list) {
	
	int count = 0; //since the head is empty, so we can ignore the first head
	if (list == NULL) return -1;
	SortedList_t* traverse = list->next;
	while(traverse != list) {
		if (traverse == NULL || traverse->next == NULL || traverse->prev== NULL) return -1;
		if (traverse->prev->next != traverse || traverse->next->prev != traverse) return -1;
		if (opt_yield & LOOKUP_YIELD) {
//			printf("Length yield\n");
			sched_yield();
		}
		count = count + 1;
		traverse = traverse->next;
	}
	return count;
}




