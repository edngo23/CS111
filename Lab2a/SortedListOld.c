// NAME: Ethan Ngo
// EMAIL: ethan.ngo2019@gmail.com
// ID: 205416130

#include "SortedList.h"
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <string.h>

int opt_yield = 0;

/**
 * SortedList_insert ... insert an element into a sorted list
 *
 *	The specified element will be inserted in to
 *	the specified list, which will be kept sorted
 *	in ascending order based on associated keys
 *
 * @param SortedList_t *list ... header for the list
 * @param SortedListElement_t *element ... element to be added to the list
 */
void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
    //if list or element isn't specified, exit
    if(list == NULL || element == NULL)
        return;

    // if list is empty
    if(list->next == NULL)
    {
        list->next = element;
        element->prev = list;
        element->next = NULL;
        return;
    }

    //find element's spot in list (deals with list being empty or not)
    SortedListElement_t* previous = list;
    SortedListElement_t* curr = list->next;
    //while(curr != NULL && (element->key > curr->key))
    while(curr != NULL && (strcmp(element->key, curr->key) >= 0))
    {
        previous = curr;
        curr = curr->next;
    }
    if(opt_yield & INSERT_YIELD)
        sched_yield();

    //if the end of the list
    if(curr == NULL)
    {
        previous->next = element;
        element->prev = previous;
        element->next = NULL;
    }
    else
    {
        previous->next = element;
        element->prev = previous;
        element->next = curr;
        curr->prev = element;
    }
}

/**
 * SortedList_delete ... remove an element from a sorted list
 *
 *	The specified element will be removed from whatever
 *	list it is currently in.
 *
 *	Before doing the deletion, we check to make sure that
 *	next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 */
int SortedList_delete( SortedListElement_t *element)
{
    if(element == NULL)
        return 1;

    if(opt_yield & DELETE_YIELD)
        sched_yield();
    
    if(element->next != NULL)
    {
        if(element->next->prev != element)
            return 1;
        else
            element->next->prev = element->prev;
    }
    if(element->prev != NULL)
    {
        if(element->prev->next != element)
            return 1;
        else
            element->prev->next = element->next;
    }
    

    return 0;
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *	The specified list will be searched for an
 *	element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
    //undefined list
    if(list == NULL)
        return NULL;

    SortedListElement_t* temp = list->next;
    while(temp != NULL)
    {
        if(opt_yield & LOOKUP_YIELD)
            sched_yield();
        if(strcmp(key, temp->key) == 0)
            return temp;
        temp = temp->next;
    }

    //key not found
    return NULL;
}

/**
 * SortedList_length ... count elements in a sorted list
 *	While enumeratign list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *	   -1 if the list is corrupted
 */
int SortedList_length(SortedList_t *list)
{
    if(list == NULL)
        return -1;
    
    SortedListElement_t* temp = list->next;
    int len = 0;
    while(temp != NULL)
    {
        if(opt_yield & LOOKUP_YIELD)
            sched_yield();
        len++;
        if(temp->prev == NULL)
            return -1;
        temp = temp->next;
    }
    
    return len;
}