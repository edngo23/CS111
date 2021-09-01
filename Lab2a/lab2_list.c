// NAME: Ethan Ngo
// EMAIL: ethan.ngo2019@gmail.com
// ID: 205416130

#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include "SortedList.h"
#include <string.h>
#include <signal.h>

extern int printing;

extern void getVals(int thrdVal, int itrVal);

//GLOBAL VARIABLES
int itrNum, thrdNum;

SortedList_t header;
SortedListElement_t* elements;
//int counter;

char opt_sync = '\0';
int lock = 0;

pthread_mutex_t mutexThrd;

void segFaultHandler()
{
    fprintf(stderr, "Segmentation Fault.\n");
    exit(2);
}

void* listThreads(void* input)
{
    SortedListElement_t* inputElem = input;
    if(opt_sync == 'm')
        pthread_mutex_lock(&mutexThrd);
    else if(opt_sync == 's')
        while(__sync_lock_test_and_set(&lock, 1));

    int i;
    
    if(printing) printf("LIST: starting insert\n");
    for(i = 0; i < itrNum; i++)
    {
        SortedList_insert(&header, (inputElem + i));
    }

    if(printing) printf("LIST: starting length\n");
    int len = SortedList_length(&header);

    if(len < itrNum)
    {
        fprintf(stderr, "List improperly created. %d %d\n", len, itrNum);
        exit(2);
    }

    if(printing) printf("LIST: starting lookup and delete\n");
    SortedListElement_t* currElem;
    char* currKey = malloc(sizeof(char) * 256);
    for(i = 0; i < itrNum; i++)
    {
        strcpy(currKey, (inputElem + i)->key);
        //counter++;
        if(i == 0 && printing) printf("LIST: lookup\n");
        currElem = SortedList_lookup(&header, currKey);
        //currElem = SortedList_lookup(&header, (inputElem + i)->key);
        // if(counter > thrdNum * itrNum)
        // {
        //     fprintf(stderr, "Looped, could not find element.\n");
        //     exit(2);
        // }
        if(currElem == NULL)
        {
            fprintf(stderr, "Element not found.\n");
            exit(2);
        }
        if(i == 0 && printing) printf("LIST: delete\n");
        int val = SortedList_delete(currElem);
        if(val)
        {
            fprintf(stderr, "Failed to delete element.\n");
            exit(2);
        }
    }

    if(opt_sync == 'm')
        pthread_mutex_unlock(&mutexThrd);
    else if(opt_sync == 's')
        __sync_lock_release(&lock);

    return NULL;
}

int main(int argc, char* argv[])
{
    signal(SIGSEGV, segFaultHandler);
    char opt;
    int operations, ret, elemNum;
    long totalTime, avgTime;
    struct timespec start, finish;

    opt_yield = 0;
    itrNum = 1;
    thrdNum = 1;


    //adapted from lab0.c and getopt(3) man page
    static struct option commandOptions[] =
    {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", required_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    while(1)
    {
        if((opt = getopt_long(argc, argv, "t:i:y:s:", commandOptions, NULL)) != -1)
        {
            switch(opt)
            {
                case 't':
                    thrdNum = atoi(optarg);
                    break;
                case 'i':
                    itrNum = atoi(optarg);
                    break;
                case 'y':
                    for(int i = 0; i < (int) strlen(optarg); i++)
                    {
                        if(*(optarg + i) == 'i')
                            opt_yield |= INSERT_YIELD;
                        else if(*(optarg + i) == 'd')
                            opt_yield |= DELETE_YIELD;
                        else if(*(optarg + i) == 'l')
                            opt_yield |= LOOKUP_YIELD;
                    }
                    break;
                case 's':
                    opt_sync = *optarg;
                    break;
                default:
                    fprintf(stderr, "Unrecognized Argument. Valid usage is --threads=thrdNum --iterations=itrNum [--sync=opt_sync][--yield]. \n");
                    exit(1);
            }
        }
        else
        {
            break;
        }
    }

    elemNum = thrdNum * itrNum;

    getVals(thrdNum, itrNum);

    elements = malloc(sizeof(SortedListElement_t) * elemNum);

    if(elements == NULL)
    {
        fprintf(stderr, "Failed to allocate the list elements.\n");
        exit(1);
    }

    // char keyTemp[elemNum];
    // for(int i = 0; i < elemNum; i++)
    // {
    //     keyTemp[i] = (char) (rand() % 126 + 33);
    //     (elements + i)->key = &keyTemp[i]; //char from range of '!' to '~'
    // }

    char** keyTemp = malloc(itrNum * thrdNum *sizeof(char*));
    if(keyTemp == NULL)
    {
        fprintf(stderr, "Failed to allocate keys.\n");
        exit(1);
    }
    for(int i = 0; i < elemNum; i++)
    {
        keyTemp[i] = malloc(sizeof(char) * 256);
        if(keyTemp[i] == NULL)
        {
            fprintf(stderr, "Failed to allocate keys.\n");
            exit(1);
        }
        for(int j = 0; j < 255; j++)
        {
            keyTemp[i][j] = (char) (rand() % 126 + 33);
        }
        keyTemp[i][255] = '\0';
        (elements + i)->key = keyTemp[i];
    }

    if(opt_sync == 'm')
    {
        ret = pthread_mutex_init(&mutexThrd, NULL);
        if(ret)
        {
            fprintf(stderr, "Failed to initialize mutex with error %d\n", ret);
            exit(1);
        }
    }

    //pthread_t threads[thrdNum];

    pthread_t* threads = malloc(sizeof(pthread_t) * thrdNum);
    if(threads == NULL)
    {
        fprintf(stderr, "Failed to create threads in memory allocation.\n");
        exit(1);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);

    for(int i = 0; i < thrdNum; i++)
    {
        ret = pthread_create(&threads[i], NULL, &listThreads, (void *) elements);
        if(ret)
        {
            fprintf(stderr, "Failed to create thread with error %d\n", ret);
            exit(1);
        }
        if(printing) printf("LIST: created thread %d\n", i);
    }

    for(int i = 0; i < thrdNum; i++)
    {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &finish);

    if(SortedList_length(&header) != 0)
    {
        fprintf(stderr, "List is not empty.\n");
        exit(2);
    }

    char yieldString[5] = "";
    char* syncString = "";

    if(opt_yield & INSERT_YIELD)
        strcat(yieldString, "i");
    if(opt_yield & DELETE_YIELD)
        strcat(yieldString, "d");
    if(opt_yield & LOOKUP_YIELD)
        strcat(yieldString, "l");
    if(opt_yield == 0)
        strcpy(yieldString, "none");

    switch(opt_sync)
    {
        case '\0':
            syncString = "none";
            break;
        default:
            syncString = &opt_sync;
            break;
    }

    operations = thrdNum * itrNum * 3;
    totalTime = 1000000000L * (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec);
    avgTime = totalTime / operations;

    printf("list-%s-%s,%d,%d,1,%d,%ld,%ld\n", yieldString, syncString, thrdNum, itrNum, operations, totalTime, avgTime);

    free(elements);
    free(threads);

    if(opt_sync == 'm')
    {
        ret = pthread_mutex_destroy(&mutexThrd);
        if(ret)
        {
            fprintf(stderr, "Failed to destroy mutex with error %d\n", ret);
            exit(1);
        }
    }

    pthread_exit(NULL);
    exit(0);
}