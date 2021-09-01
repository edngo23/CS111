// NAME: Ethan Ngo
// EMAIL: ethan.ngo2019@gmail.com
// ID: 205416130

#include "SortedList.h"
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <string.h>

int opt_yield = 0;
int itrNumVal, thrdNumVal;
int maxLen;
int counter, len;
int printing = 1;

void getVals(int thrdVal, int itrVal)
{
    itrNumVal = itrVal;
    thrdNumVal = thrdVal;
}

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
    if(printing) printf("we start the insert.\n");
    //if list or element isn't specified, exit
    if(list == NULL || element == NULL)
        return;

    maxLen = thrdNumVal * itrNumVal;
    counter = 0;
    if(printing) printf("%d %d\n", thrdNumVal, itrNumVal);

    // if list is empty
    if(list->next == NULL)
    {
        if(printing) printf("list->next NULL\n");
        if(opt_yield & INSERT_YIELD)
            sched_yield();
        list->next = element;
        element->prev = list;
        element->next = NULL;
        return;
    }

    if(printing) printf("list not empty, insert time\n");

    //find element's spot in list (deals with list being empty or not)
    SortedListElement_t* previous = list;
    SortedListElement_t* curr = list->next;
    //while(curr != NULL && (element->key > curr->key))
    while(curr != NULL && (strcmp(element->key, curr->key) >= 0))
    {
        if(counter > maxLen)
        {
            fprintf(stderr, "Insertion failed due to loop.\n");
            exit(2);
        }
        previous = curr;
        curr = curr->next;
        counter++;
    }
    if(opt_yield & INSERT_YIELD)
        sched_yield();

    if(printing) printf("we hit the insert spot\n");
    //if the end of the list
    if(curr == NULL)
    {
        previous->next = element;
        element->prev = previous;
        element->next = NULL;
        if(printing) printf("list insert at end\n");
    }
    else
    {
        previous->next = element;
        curr->prev = element;
        element->next = curr;
        element->prev = previous;
        if(printing) printf("list insert\n");
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
    if(printing) printf("we start the delete\n");
    if(element == NULL)
        return 1;

    if(opt_yield & DELETE_YIELD)
        sched_yield();
    
    if(element->next != NULL)
    {
        if(printing) printf("elem next not NULL\n");
        if(element->next->prev != element)
            return 1;
        else
            element->next->prev = element->prev;
    }
    if(element->prev != NULL)
    {
        if(printing) printf("elem prev not NULL\n");
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
    if(printing) printf("we starting the lookup\n");
    //undefined list
    if(list == NULL)
        return NULL;

    counter = 0;

    SortedListElement_t* temp = list->next;
    while(temp != NULL)
    {
        if(printing) printf("looking\n");
        if(counter > maxLen)
            return NULL;
        if(opt_yield & LOOKUP_YIELD)
            sched_yield();
        if(strcmp(key, temp->key) == 0)
            return temp;
        if(printing) printf("still not found\n");
        temp = temp->next;
        counter++;
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
    if(printing) printf("calculating length\n");
    if(list == NULL)
        return -1;
    
    SortedListElement_t* temp = list->next;
    len = 0;
    while(temp != NULL)
    {
        if(printing) printf("iterating\n");
        if(len > maxLen)
        {
            if(printing) printf("len too big\n");
            return -1;
        }
        if(opt_yield & LOOKUP_YIELD)
            sched_yield();
        
        // if(temp->prev == NULL)
        // {
        //     printf("temp->prev NULL\n");
        //     return -1;
        // }
        temp = temp->next;
        len++;
    }
    
    return len;
}