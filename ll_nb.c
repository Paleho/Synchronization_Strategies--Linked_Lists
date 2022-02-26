#include <stdio.h>
#include <stdlib.h> /* rand() */
#include <limits.h>
#include <stdint.h>

#include "../common/alloc.h"
#include "ll.h"

typedef struct ll_node {
        int key;
        struct ll_node *next;
        /* other fields here? */
} ll_node_t;

struct linked_list {
        ll_node_t *head;
        /* other fields here? */
};

typedef struct window {
    ll_node_t *pred, *curr;
} window_t;

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

        return ret;
}

/**
 * Free a linked list node.
 **/
static void ll_node_free(ll_node_t *ll_node)
{
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

const uintptr_t mask = 1;

ll_node_t * getReference(ll_node_t *next){
	// reference = next(without marked bit)
	return (ll_node_t *)((uintptr_t)next & ~mask);
}

int getMarked(ll_node_t *next){
	// marked = lsb of next
	return (uintptr_t)next & mask;
}

ll_node_t * get(ll_node_t *next, int * marked){
	*marked = getMarked(next);
	return getReference(next);
}

/**
 * Free a linked list and all its contained nodes.
 **/
void ll_free(ll_t *ll)
{
        ll_node_t *next, *curr = ll->head;
        while (curr) {
                next = getReference(curr->next);
                ll_node_free(curr);
                curr = next;
        }
        XFREE(ll);
}

window_t* find(ll_t *ll, int key){
	ll_node_t *pred, *curr, *succ;
	int marked = 0;
	int snip;
retry:
	while(1){
		pred = ll->head;
		curr = getReference(pred->next);
		while(1){
			succ = get(curr->next, &marked);
			while(marked){
				// old_pointer = curr + marked bit = 0
				ll_node_t * old_pointer = (ll_node_t *)((uintptr_t)curr & ~mask);
				// new_pointer = succ + marked bit = 0
				ll_node_t * new_pointer = (ll_node_t *)((uintptr_t)succ & ~mask);
				snip = __sync_bool_compare_and_swap (&pred->next, old_pointer, new_pointer);
				if(!snip) goto retry;
				curr = succ;
				succ = get(curr->next, &marked);
			}
			if (curr->key >= key){
				window_t *win;
				XMALLOC(win,1);
				win->pred = pred;
				win->curr = curr;
				return win;
			}
			pred = curr;
			curr = succ;
		}
	}
}

int ll_contains(ll_t *ll, int key)
{
	ll_node_t *curr = ll->head;
	while(curr->key < key){
		curr = getReference(curr->next);
	}
	return (curr->key == key && !getMarked(curr->next));
}

int ll_add(ll_t *ll, int key)
{
	while(1){
		window_t *win = find(ll, key);
		ll_node_t *pred, *curr;
		pred = win->pred;
		curr = win->curr;
		if(curr->key == key){
			return 0;
		}
		else{
			ll_node_t *node = ll_node_new(key);
			node->next = curr;
			// old_pointer = curr + marked bit = 0
			ll_node_t * old_pointer = (ll_node_t *)((uintptr_t)curr & ~mask);
			// new_pointer = node + marked bit = 0
			ll_node_t * new_pointer = (ll_node_t *)((uintptr_t)node & ~mask);
			if(__sync_bool_compare_and_swap (&pred->next, old_pointer, new_pointer)){
				return 1;
			}
		}
	}
}

int ll_remove(ll_t *ll, int key)
{
    int snip;
	while(1){
		window_t *win = find(ll, key);
		ll_node_t *pred, *curr;
		pred = win->pred;
		curr = win->curr;
		if(curr->key != key){
			return 0;
		}
		else{
			ll_node_t *succ = getReference(curr->next);
			// old_pointer = succ + marked bit = 0
			ll_node_t * old_pointer = (ll_node_t *)((uintptr_t)succ & ~mask);
			// new_pointer = succ + marked bit = 1
			ll_node_t * new_pointer = (ll_node_t *)((uintptr_t)succ | mask);
			snip = __sync_bool_compare_and_swap (&curr->next, old_pointer, new_pointer);
			if(!snip) continue;
			// old_pointer = curr + marked bit = 0
			old_pointer = (ll_node_t *)((uintptr_t)succ & ~mask);
			// new_pointer = succ + marked bit = 0
			new_pointer = (ll_node_t *)((uintptr_t)succ & ~mask);
			__sync_bool_compare_and_swap (&pred->next, old_pointer, new_pointer);
			return 1;
		}
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
		if(!getMarked(curr->next)){
			if (curr->key == INT_MAX)
				printf(" -> MAX");
			else
				printf(" -> %d", curr->key);
		}
		curr = getReference(curr->next);
	}
	printf(" ]\n");
}