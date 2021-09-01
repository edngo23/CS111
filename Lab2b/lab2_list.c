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
int itrNum, thrdNum, listNum;
long NANOSEC = 1000000000;

SortedList_t* header;
SortedListElement_t* elements;
//int counter;

char opt_sync = '\0';
int* lock;

pthread_mutex_t* mutexThrd;

void segFaultHandler()
{
    fprintf(stderr, "Segmentation Fault.\n");
    exit(2);
}

unsigned int hash(const char* key)
{
    // int randNum = 5381;
    // int val;
    
    // while((val = *key++))
    //     randNum = ((randNum << 5) + randNum) + val;
    int randNum = (int) *key;

    return randNum % listNum;
}

void* listThreads(void* input)
{
    long waitTime = 0;
    struct timespec waitStart, waitFinish;
    SortedListElement_t* inputElem = input;
    int hashVal, len = 0;

    int i;
    
    if(printing) printf("LIST: starting insert\n");
    for(i = 0; i < itrNum; i++)
    {
        hashVal = hash((inputElem + i)->key);
        clock_gettime(CLOCK_MONOTONIC, &waitStart);
        if(opt_sync == 'm')
        {
            pthread_mutex_lock(mutexThrd + hashVal);
        }
        else if(opt_sync == 's')
            while(__sync_lock_test_and_set(lock + hashVal, 1));
        clock_gettime(CLOCK_MONOTONIC, &waitFinish);
        waitTime += NANOSEC * (waitFinish.tv_sec - waitStart.tv_sec) + (waitFinish.tv_nsec - waitStart.tv_nsec);

        //insert
        SortedList_insert(header + hashVal, (inputElem + i));

        if(opt_sync == 'm')
            pthread_mutex_unlock(mutexThrd + hashVal);
        else if(opt_sync == 's')
            __sync_lock_release(lock + hashVal);
    }

    if(printing) printf("LIST: starting length\n");
    for(i = 0; i < listNum; i++)
    {
        clock_gettime(CLOCK_MONOTONIC, &waitStart);
        if(opt_sync == 'm')
        {
            pthread_mutex_lock(mutexThrd + i);
        }
        else if(opt_sync == 's')
            while(__sync_lock_test_and_set(lock + i, 1));
        clock_gettime(CLOCK_MONOTONIC, &waitFinish);
        waitTime += NANOSEC * (waitFinish.tv_sec - waitStart.tv_sec) + (waitFinish.tv_nsec - waitStart.tv_nsec);

        //length
        len += SortedList_length(header + i);

        if(opt_sync == 'm')
            pthread_mutex_unlock(mutexThrd + i);
        else if(opt_sync == 's')
            __sync_lock_release(lock + i);
    }

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
        //hashVal = hash((inputElem + i)->key);
        strcpy(currKey, (inputElem + i)->key);
        //counter++;
        if(i == 0 && printing) printf("LIST: lookup\n");

        hashVal = hash((inputElem + i)->key);
        clock_gettime(CLOCK_MONOTONIC, &waitStart);
        if(opt_sync == 'm')
        {
            pthread_mutex_lock(mutexThrd + hashVal);
        }
        else if(opt_sync == 's')
            while(__sync_lock_test_and_set(lock + hashVal, 1));
        clock_gettime(CLOCK_MONOTONIC, &waitFinish);
        waitTime += NANOSEC * (waitFinish.tv_sec - waitStart.tv_sec) + (waitFinish.tv_nsec - waitStart.tv_nsec);

        //lookup
        currElem = SortedList_lookup(header + hashVal, currKey);
        //currElem = SortedList_lookup(header, (inputElem + i)->key);
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
        //delete
        int val = SortedList_delete(currElem);
        if(val)
        {
            fprintf(stderr, "Failed to delete element.\n");
            exit(2);
        }

        if(opt_sync == 'm')
            pthread_mutex_unlock(mutexThrd + hashVal);
        else if(opt_sync == 's')
            __sync_lock_release(lock + hashVal);
    }

    return (void *) waitTime;
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
    listNum = 1;


    //adapted from lab0.c and getopt(3) man page
    static struct option commandOptions[] =
    {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", required_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'},
        {"lists", required_argument, 0, 'l'},
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
                case 'l':
                    listNum = atoi(optarg);
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

    header = malloc(sizeof(SortedList_t) * listNum);
    if(header == NULL)
    {
        fprintf(stderr, "Failed to create list head.\n");
        exit(1);
    }

    for(int i = 0; i < listNum; i++)
    {
        header[i].prev = NULL;
        header[i].next = NULL;
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
        mutexThrd = malloc(sizeof(pthread_mutex_t) * listNum);
        if(mutexThrd == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for mutex\n");
            exit(1);
        }
        for(int i = 0; i < listNum; i++)
        {
            ret = pthread_mutex_init(mutexThrd + i, NULL);
            if(ret)
            {
                fprintf(stderr, "Failed to create mutexes with error %d\n", ret);
                exit(1);
            }
        }
    }
    else if(opt_sync == 's')
    {
        lock = malloc(sizeof(int) * listNum);
        if(lock == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for lock\n");
            exit(1);
        }
        for(int i = 0; i < listNum; i++)
        {
            lock[i] = 0;
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

    long waitTotal = 0;
    void** waitTemp = (void *) malloc(sizeof(void**));

    for(int i = 0; i < thrdNum; i++)
    {
        pthread_join(threads[i], waitTemp);
        waitTotal += (long) *waitTemp;
    }

    clock_gettime(CLOCK_MONOTONIC, &finish);

    int listLen = 0;
    for(int i = 0; i < listNum; i++)
    {
        listLen += SortedList_length(header + i);
    }

    if(listLen != 0)
    {
        fprintf(stderr, "List failed to be completely deleted.\n");
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
    totalTime = NANOSEC * (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec);
    avgTime = totalTime / operations;
    long waitPerOp = waitTotal/operations;

    printf("list-%s-%s,%d,%d,%d,%d,%ld,%ld,%ld\n", yieldString, syncString, thrdNum, itrNum, listNum, operations, totalTime, avgTime, waitPerOp);

    free(elements);
    free(threads);

    if(opt_sync == 'm')
    {
        for(int i = 0; i < listNum; i++)
        {
            ret = pthread_mutex_destroy(mutexThrd + i);
            if(ret)
            {
                fprintf(stderr, "Failed to destroy mutex with error %d\n", ret);
                exit(1);
            }
            free(mutexThrd);
        }
    }
    else if(opt_sync == 's')
    {
        free(lock);
    }

    free(waitTemp);

    pthread_exit(NULL);
    exit(0);
}