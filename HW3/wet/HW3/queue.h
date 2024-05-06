#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include "node.h"
#include "request.h"
#include "thread.h"

/* Queue struct definition */
typedef struct Queue 
{
    //front of queue
    Node* front;
    //beck of queue
    Node* rear;
    //queue current size
    int size;
    //pool of worker threads size
    int pool_size;
    //connections descriptors queue
    int http_connections_num;
    // full queue handling method
    char* schedalg;
    //number of requests currently handled by some worker thread
    int active_requests_num;
    //max queue size when scheduling algorithm is dynamic, -1 otherwise
    int max_size;
    //queue lock
    pthread_mutex_t global_lock;
    //queue not full
    pthread_cond_t insertion_allowed;
    //queue not empty
    pthread_cond_t deletion_allowed;
    //queue is empty
    pthread_cond_t is_empty;
} Queue;

/* Queue mathods */
Queue* makeQueue(int pool_size, int http_connections_num, char* schedalg, int max_size,
    pthread_mutex_t global_lock, pthread_cond_t insertion_allowed,
    pthread_cond_t deletion_allowed, pthread_cond_t is_empty);
bool enqueue(Queue* q, Node* to_insert);
Node* dequeue(Queue* q, bool is_critical);
bool cond_dequeue(Queue* q, int index);
bool isEmpty(Queue* q);
bool isFull(Queue* q, int size);
void freeQueue(Queue* q);

#endif //QUEUE_H

