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

//GLOBAL VARIABLES
long long counter = 0;
int itrNum = 1;

int opt_yield;
char opt_sync = '\0';
int lock = 0;

pthread_mutex_t mutexThrd;

//adding
void add(long long *pointer, long long value) {
    if(opt_sync == 'm')
        pthread_mutex_lock(&mutexThrd);
    else if(opt_sync == 's')
        while(__sync_lock_test_and_set(&lock, 1));
    long long sum = *pointer + value;
    if (opt_yield)
        sched_yield();
    *pointer = sum;
    if(opt_sync == 'm')
        pthread_mutex_unlock(&mutexThrd);
    else if(opt_sync == 's')
        __sync_lock_release(&lock);
}

//compare and swap adding
void addCompSwap(long long *pointer, long long value) {
    long long temp;
    do
    {    
        temp = *pointer;
        if (opt_yield)
            sched_yield();
    } while(__sync_val_compare_and_swap(pointer, temp, temp + value) != temp);
}

void* addThreads()
{
    for(int i = 0; i < itrNum; i++)
    {
        switch(opt_sync)
        {
            case 'c':
                addCompSwap(&counter, 1);
                addCompSwap(&counter, -1);
                break;
            case 'm':
                //fall through
            case 's':
                //fall through
            default:
                add(&counter, 1);
                add(&counter, -1);
                break;
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
    char opt;
    int thrdNum = 1, operations, ret;
    long totalTime, avgTime;
    struct timespec start, finish;


    //adapted from lab0.c and getopt(3) man page
    static struct option commandOptions[] =
    {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", no_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    while(1)
    {
        if((opt = getopt_long(argc, argv, "t:i:ys", commandOptions, NULL)) != -1)
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
                    opt_yield = 1;
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

    clock_gettime(CLOCK_MONOTONIC, &start);

    if(opt_sync == 'm')
    {
        ret = pthread_mutex_init(&mutexThrd, NULL);
        if(ret)
        {
            fprintf(stderr, "Failed to initialize mutex with error %d\n", ret);
            exit(1);
        }
    }

    pthread_t threads[thrdNum];
    for(int i = 0; i < thrdNum; i++)
    {
        ret = pthread_create(&threads[i], NULL, &addThreads, NULL);
        if(ret)
        {
            fprintf(stderr, "Failed to create thread with error %d\n", ret);
            exit(1);
        }
    }

    for(int i = 0; i < thrdNum; i++)
    {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &finish);

    operations = thrdNum * itrNum * 2;
    totalTime = 1000000000L * (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec);
    avgTime = totalTime / operations;

    printf("add-");
    //yield or not
    if(opt_yield == 1)
        printf("yield-");
    
    //sync options
    if(opt_sync == 'm')
        printf("m");
    else if(opt_sync == 's')
        printf("s");
    else if(opt_sync == 'c')
        printf("c");
    else
        printf("none");

    //value prints
    printf(",%d,%d,%d,%ld,%ld,%lld\n", thrdNum, itrNum, operations, totalTime, avgTime, counter);

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