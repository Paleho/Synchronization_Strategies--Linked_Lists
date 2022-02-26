#include <stdio.h>
#include <stdlib.h> /* rand() */
#include <limits.h>
#include <pthread.h> /* for pthread_spinlock_t */

#include "../common/alloc.h"
#include "ll.h"

typedef struct ll_node {
	int key;
	struct ll_node *next;
	/* other fields here? */
	pthread_spinlock_t *lock;
	char marked;
} ll_node_t;

struct linked_list {
	ll_node_t *head;
	/* other fields here? */
};

/**
 * Create a new linked list node.
 **/
static ll_node_t *ll_node_new(int key)
{
	ll_node_t *ret;

	XMALLOC(ret, 1);
	ret->key = key;
	ret->next = NULL;
	/* Other initializations here? */
	pthread_spin_init(&(ret->lock), PTHREAD_PROCESS_SHARED);
	ret->marked = 0;
	return ret;
}

/**
 * Free a linked list node.
 **/
static void ll_node_free(ll_node_t *ll_node)
{
	pthread_spin_destroy(&(ll_node->lock));
	XFREE(ll_node);
}

/**
 * Create a new empty linked list.
 **/
ll_t *ll_new()
{
	ll_t *ret;

	XMALLOC(ret, 1);
	ret->head = ll_node_new(-1);
	ret->head->next = ll_node_new(INT_MAX);
	ret->head->next->next = NULL;

	return ret;
}

/**
 * Free a linked list and all its contained nodes.
 **/
void ll_free(ll_t *ll)
{
	ll_node_t *next, *curr = ll->head;
	while (curr) {
		next = curr->next;
		ll_node_free(curr);
		curr = next;
	}
	XFREE(ll);
}

int validate(ll_node_t *pred, ll_node_t *curr){
	return (pred && curr && !pred->marked && !curr->marked && pred->next == curr);
}

int ll_contains(ll_t *ll, int key)
{
	ll_node_t *curr = ll->head;
	while (curr->key < key)
		curr = curr->next;
	return (curr->key == key && !curr->marked);

}

int ll_add(ll_t *ll, int key)
{
	int ret;
	ll_node_t *pred, *curr;
	while(1){
		pred = ll->head;
		curr = pred->next;
		while(curr && curr->key < key){
			pred = curr;
			curr = curr->next;
		}
		if(!curr) continue;
		pthread_spin_lock(&(pred->lock));
		pthread_spin_lock(&(curr->lock));
		if (validate(pred, curr)){
			if (curr->key == key)
				ret = 0;
			else{
				ll_node_t *node = ll_node_new(key); 
				node->next = curr;
				pred->next = node;
				ret = 1;
			}
			pthread_spin_unlock(&(pred->lock));
			pthread_spin_unlock(&(curr->lock));
			return ret;
		}
		pthread_spin_unlock(&(pred->lock));
		pthread_spin_unlock(&(curr->lock));
	}
}

int ll_remove(ll_t *ll, int key)
{
	int ret;
	ll_node_t *pred, *curr;
	while(1){
		pred = ll->head;
		curr = pred->next;
		while(curr && curr->key < key){
			pred = curr;
			curr = curr->next;
		}
		if(!curr) continue;
		pthread_spin_lock(&(pred->lock));
		pthread_spin_lock(&(curr->lock));
		if (validate(pred, curr)){
			if (curr->key == key){
				curr->marked = 1;
				pred->next = curr->next;
				ret = 1;
			}
			else
				ret = 0;
			
			pthread_spin_unlock(&(pred->lock));
			pthread_spin_unlock(&(curr->lock));
			return ret;
		}
		pthread_spin_unlock(&(pred->lock));
		pthread_spin_unlock(&(curr->lock));
	}
}

/**
 * Print a linked list.
 **/
void ll_print(ll_t *ll)
{
	ll_node_t *curr = ll->head;
	printf("LIST [");
	while (curr) {
		if (curr->key == INT_MAX)
			printf(" -> MAX");
		else
			printf(" -> %d", curr->key);
		curr = curr->next;
	}
	printf(" ]\n");
}
