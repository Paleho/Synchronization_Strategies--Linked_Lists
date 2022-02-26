#include <stdio.h>
#include <stdlib.h> /* rand() */
#include <limits.h>
#include <pthread.h> /* for pthread_spinlock_t */
#include <errno.h> /* for EBUSY */

#include "../common/alloc.h"
#include "ll.h"

typedef struct ll_node {
	int key;
	struct ll_node *next;
	/* other fields here? */
	pthread_spinlock_t *lock;
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

	return ret;
}

/**
 * Free a linked list node.
 **/
static void ll_node_free(ll_node_t *ll_node)
{
	// pthread_spin_destroy(&(ll_node->lock));
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

int validate(ll_node_t *pred, ll_node_t *curr, ll_t *ll){
	ll_node_t *tmp = ll->head;
	while(tmp && tmp->key <= pred->key){
		if(tmp == pred)
			return pred->next == curr;
		tmp = tmp->next;
	}	
	return 0;
}

int ll_contains(ll_t *ll, int key)
{
	int ret;
	ll_node_t *pred, *curr;
	while(1){
		pred = ll->head;
		curr = pred->next;
		if(!curr) continue;
		while(curr && curr->key <= key){
			if(curr && curr->key == key) 
				break;
			pred = curr;
			if(curr) curr = curr->next;
		}
		if(!curr) continue;
		while(pred && pthread_spin_trylock(&(pred->lock)) == EBUSY);
		if(!pred) continue;
		while(curr && pthread_spin_trylock(&(curr->lock)) == EBUSY);
		if(!curr) continue;
		if (validate(pred, curr, ll)){
			if (curr->key == key){
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

int ll_add(ll_t *ll, int key)
{
	int ret, loops;
	ll_node_t *pred, *curr;
	while(1){
		pred = ll->head;
		curr = pred->next;
		if(!curr) continue;
		while(curr && curr->key <= key){
			if(curr && curr->key == key) 
				break;
			pred = curr;
			if(curr) curr = curr->next;
		}
		if(!curr) continue;
		loops = 1 ;
		while(pred && pthread_spin_trylock(&(pred->lock)) == EBUSY);
		if(!pred) continue;
		loops = 1;
		while(curr && pthread_spin_trylock(&(curr->lock)) == EBUSY);
		if(!curr) continue;
		if (validate(pred, curr, ll)){
			if (curr->key == key)
				ret = 0;
			else{
				ll_node_t *new = ll_node_new(key); 
				pred->next = new;
				new->next = curr;
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
		if(!curr) continue;
		while(curr && curr->key <= key){
			if(curr && curr->key == key) 
				break;
			pred = curr;
			if(curr) curr = curr->next;
		}
		if(!curr) continue;
		while(pred && pthread_spin_trylock(&(pred->lock)) == EBUSY);
		if(!pred) continue;
		while(curr && pthread_spin_trylock(&(curr->lock)) == EBUSY);
		if(!curr) continue;
		if (validate(pred, curr, ll)){
			if (curr->key == key){
				pred->next = curr->next;
				pthread_spin_unlock(&(curr->lock));
				// ll_node_free(curr);
				ret = 1;
				pthread_spin_unlock(&(pred->lock));
				// 
			}
			else{
				ret = 0;
				pthread_spin_unlock(&(pred->lock));
				pthread_spin_unlock(&(curr->lock));
			}
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
