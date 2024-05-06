#ifndef __THREAD_H__
#define __THREAD_H__

typedef struct Thread 
{
    int stat_thread_id;
    int stat_thread_static;
    int stat_thread_dynamic;
    int stat_thread_count;
} Thread;

Thread* makeThread(int stat_thread_id);

//global vars
Thread** threads_handler;

#endif