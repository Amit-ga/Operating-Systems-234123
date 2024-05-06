#include <stdio.h>
#include <stdlib.h>
#include "thread.h"

Thread* makeThread(int stat_thread_id)
{
    Thread* t = (Thread*)malloc(sizeof(Thread));
    if (t == NULL)
    {
        printf("Memory allocation error!\n");
        return NULL;
    }

    t->stat_thread_id = stat_thread_id;
    t->stat_thread_count = 0;
    t->stat_thread_static = 0;
    t->stat_thread_dynamic = 0;

    return t;
}