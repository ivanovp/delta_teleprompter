/**
 * @file        linkedlist.c
 * @brief       Linked list functions
 * @author      Copyright (C) Peter Ivanov, 2013, 2014, 2021
 *
 * Created      2013-12-30 11:48:53
 * Last modify: 2021-02-15 19:01:21 ivanovp {Time-stamp}
 * Licence:     GPL
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"
#include "linkedlist.h"

/**
 * Allocate memory for a new element of linked list.
 *
 * @param[in] aItem Linked list item to store.
 * @param[in] aPrev Previous linked list item.
 */
linkedListElement_t* addElement (void* aItem, linkedListElement_t* aPrev)
{
    linkedListElement_t* linkedList = NULL;

    linkedList = malloc (sizeof (linkedListElement_t));
    if (linkedList)
    {
        linkedList->item = aItem;
        linkedList->prev = aPrev;
    }

    return linkedList;
}

/**
 * @brief freeElement
 * Releases memory of a linked list's item and linked list element too.
 *
 * @param aLinkedList
 * @return
 */
linkedListElement_t* freeElement (linkedListElement_t* aLinkedList)
{
    linkedListElement_t* next = NULL;

    if (aLinkedList)
    {
        next = aLinkedList->next;
        if (aLinkedList->item)
        {
            free (aLinkedList->item);
        }
        free (aLinkedList);
    }

    return next;
}

/**
 * @brief freeLinkedList
 * @param aFirstLinkedList
 */
void freeLinkedList (linkedListElement_t* aFirstLinkedList)
{
    linkedListElement_t* linkedList = aFirstLinkedList;
    bool_t end = FALSE;
    bool_t first = TRUE;

    while (linkedList && !end)
    {
        if (!first && linkedList == aFirstLinkedList)
        {
            end = TRUE;
        }
        linkedList = freeElement (linkedList);
        first = FALSE;
    }
}

bool_t addScriptElement(char * aText, linkedList_t * aLinkedList)
{
    bool_t ok = TRUE;
    size_t len = strlen(aText);
    char * linkedListText = malloc(len + 1); // +1 due to end of string

    if (linkedListText)
    {
        strncpy(linkedListText, aText, len + 1);
//        linkedListText[len] = CHR_EOS; // end of string
//        printf("selected text: [%s]\n", linkedListText);

        (*aLinkedList->it) = addElement (linkedListText, aLinkedList->it_prev);
        if (*aLinkedList->it)
        {
            aLinkedList->last = *(aLinkedList->it);
            aLinkedList->it_prev = *(aLinkedList->it);
            aLinkedList->it = &((*(aLinkedList->it))->next);
        }
        else
        {
            printf("ERROR: cannot allocate memory for list element!\n");
            ok = FALSE;
        }
    }
    else
    {
        printf("ERROR: cannot allocate memory for text!\n");
        ok = FALSE;
    }

    return ok;
}
