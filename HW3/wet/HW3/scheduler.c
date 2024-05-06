#include "scheduler.h"
#include "request.h"
#include "thread.h"

/* Scheduler mathods implementation */
Scheduler* makeScheduler(int pool_size, int http_connections_num, char* schedalg, int max_size,
    pthread_mutex_t global_lock, pthread_cond_t insertion_allowed,
    pthread_cond_t deletion_allowed, pthread_cond_t is_empty) 
{
    Scheduler* s = (Scheduler*)malloc(sizeof(Scheduler));
    if (!s) 
    {
        printf("Memmory allocation error! \n");
        return NULL;
    }
    s->waiting_requests = makeQueue(pool_size, http_connections_num, schedalg, max_size, global_lock, insertion_allowed,
        deletion_allowed, is_empty);
    if (!s)
    {
        printf("Memmory allocation error! \n");
        free(s);
        return NULL;
    }
    s->pool_size = pool_size;
    s->http_connections_num = http_connections_num;
    s->schedalg = malloc(strlen(schedalg) * sizeof(char) + 1);
    if (!s->schedalg)
    {
        printf("Memmory allocation error! \n");
        freeQueue(s->waiting_requests);
        free(s);
        return NULL;
    }
    strcpy(s->schedalg, schedalg);
    s->max_size = max_size;
    s->global_lock = global_lock;
    s->insertion_allowed = insertion_allowed;
    s->deletion_allowed = deletion_allowed;
    s->is_empty = is_empty;
    return s;
}

void request(Scheduler* s, int connfd) 
{
    Node* r = makeNode(connfd);
    if (r)
    {
        enqueue(s->waiting_requests, r);
    }
}

void schedule(Scheduler* s, int index) 
{
    if (s)
    {
        Node* temp = dequeue(s->waiting_requests, true);
        if (temp)
        {
            //set request properties
            gettimeofday(&temp->data->stat_req_dispatch, NULL);
            timersub(&temp->data->stat_req_dispatch, &temp->data->stat_req_arrival, &temp->data->stat_req_dispatch);

            requests[index] = temp->data;

            //critical section - changing queue size
            pthread_mutex_lock(&s->global_lock);
            s->waiting_requests->active_requests_num++;
            pthread_mutex_unlock(&s->global_lock);

            //handle request
            requestHandle(temp->data->connfd, index);

            //restore requests arr
            requests[index] = NULL;
            //request had been handled, close connection
            Close(temp->data->connfd);

            //critical section - changing queue size
            pthread_mutex_lock(&s->global_lock);
            s->waiting_requests->active_requests_num--;
            pthread_mutex_unlock(&s->global_lock);
            pthread_cond_signal(&s->waiting_requests->insertion_allowed);
            if (s->waiting_requests->size == 0)
            {
                pthread_cond_signal(&s->waiting_requests->is_empty);
            }
            
            //done - free allocated resources
            free(temp->data);
            free(temp);
        }
    }
}

void freeScheduler(Scheduler* s)
{
    if (s)
    {
        freeQueue(s->waiting_requests);
        free(s->schedalg);
        free(s);
    }
}