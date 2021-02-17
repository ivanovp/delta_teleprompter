/**
 * @file        kinkedlist.h
 * @brief       Linked list
 * @author      Copyright (C) Peter Ivanov, 2011, 2012, 2013, 2014, 2021
 *
 * Created      2011-01-19 11:48:53
 * Last modify: 2021-02-16 17:51:49 ivanovp {Time-stamp}
 * Licence:     GPL
 */

#ifndef INCLUDE_LINKEDLIST_H
#define INCLUDE_LINKEDLIST_H

#include <stdint.h>

#include "common.h"

typedef struct linkedListElement_tag
{
    void* item;                            /* Data to store in the element */
    struct linkedListElement_tag* next;    /* Next element, NULL if it is the last one. */
    struct linkedListElement_tag* prev;    /* Previous element, NULL if it is the first one. */
} linkedListElement_t;

typedef struct
{
    linkedListElement_t* first;     /**< First element */
    linkedListElement_t* last;      /**< Last element */
    linkedListElement_t* actual;    /**< Actual element */
    linkedListElement_t** it;       /**< List iterator. Used to build list. */
    linkedListElement_t* it_prev;   /**< List previous element. Used to build list. */
} linkedList_t;

linkedListElement_t* addElement (void* aItem, linkedListElement_t* aPrev);
linkedListElement_t* freeElement (linkedListElement_t* aLinkedList);
void freeLinkedList (linkedListElement_t* aFirstLinkedList);
bool_t addScriptElement(char * aStartPtr, char * aEndPtr, linkedList_t * aLinkedList);

#endif /* INCLUDE_COMMON_H */

