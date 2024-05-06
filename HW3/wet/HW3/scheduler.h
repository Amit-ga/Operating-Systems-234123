#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "queue.h"

/* Scheduler struct definition */
typedef struct Scheduler 
{
    // queue for requests waiting to be picked up by a worker thread
    Queue* waiting_requests;
    //queue required properties
    int pool_size;
    int  http_connections_num;
    char* schedalg;
    int active_requests_num;
    int max_size;
    pthread_mutex_t global_lock;
    pthread_cond_t insertion_allowed;
    pthread_cond_t deletion_allowed;
    pthread_cond_t is_empty;
} Scheduler;

/* Scheduler mathods */
Scheduler* makeScheduler(int pool_size, int http_connections_num, char* schedalg, int max_size,
    pthread_mutex_t global_lock, pthread_cond_t insertion_allowed,
    pthread_cond_t deletion_allowed, pthread_cond_t is_empty);
void request(Scheduler* s, int connfd);
void schedule(Scheduler* s, int index);
void freeScheduler(Scheduler* s);

#endif //SCHEDULER_H