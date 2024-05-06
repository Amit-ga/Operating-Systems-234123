#include <stdio.h>
#include <stdlib.h>
#include<math.h>
#include "queue.h"
#include "request.h"
#include "thread.h"

/* Queue mathods implementation */
Queue* makeQueue(int pool_size, int http_connections_num, char* schedalg, int max_size,
    pthread_mutex_t global_lock, pthread_cond_t insertion_allowed,
    pthread_cond_t deletion_allowed, pthread_cond_t is_empty) 
{
    Queue* new_queue = (Queue*)malloc(sizeof(Queue));
    if (!new_queue)
    {
        printf("Memmory allocation error! \n");
        return NULL;
    }
    new_queue->front = NULL;
    new_queue->rear = NULL;
    new_queue->size = 0;
    new_queue->pool_size = pool_size;
    new_queue->http_connections_num = http_connections_num;
    new_queue->schedalg = malloc(strlen(schedalg) * sizeof(char) + 1);
    if (new_queue->schedalg == NULL)
    {
        printf("Memmory allocation error! \n");
        free(new_queue);
        return NULL;
    }
    strcpy(new_queue->schedalg, schedalg);
    new_queue->active_requests_num = 0;
    new_queue->max_size = max_size;
    new_queue->global_lock = global_lock;
    new_queue->insertion_allowed = insertion_allowed;
    new_queue->deletion_allowed = deletion_allowed;
    new_queue->is_empty = is_empty;

    return new_queue;
}

bool enqueue(Queue* q, Node* to_insert) 
{
    if (q && to_insert)
    {
        //critical section
        pthread_mutex_lock(&q->global_lock);
        while (isFull(q, q->http_connections_num )|| isFull(q, q->http_connections_num - q->active_requests_num))
        {
            //block
            if (!strcmp(q->schedalg, "block")) 
            {
                pthread_cond_wait(&q->insertion_allowed, &q->global_lock);
            }
            //drop_tail 
            // //or drop_head with empty queue 
            // //or drop_random with empty queue
            //or dynamic with max-sized queue
            else if (!strcmp(q->schedalg, "dt") ||
                (isEmpty(q) && !strcmp(q->schedalg, "dh")) || 
                (isEmpty(q) && !strcmp(q->schedalg, "random")) ||
                (!strcmp(q->schedalg, "dynamic") && q->size == q->max_size))
            {
                Close(to_insert->data->connfd);
                free(to_insert->data);
                free(to_insert);
                pthread_mutex_unlock(&q->global_lock);
                return false;
            }
            //drop_head
            else if (!strcmp(q->schedalg, "dh")) 
            {
                Node* to_dequeue = dequeue(q, false);
                Close(to_dequeue->data->connfd);
                free(to_dequeue->data);
                free(to_dequeue);
            }
            //block_flush
            else if (!strcmp(q->schedalg, "bf"))
            {
                pthread_cond_wait(&(q->is_empty), &(q->global_lock));
            }
            //Dynamic
            else if (!strcmp(q->schedalg, "dynamic"))
            {
                //assert queue has not reached max size
                q->http_connections_num++;
                Close(to_insert->data->connfd);
                free(to_insert->data);
                free(to_insert);
                pthread_mutex_unlock(&(q->global_lock));
                return false;
            }
            //random
            else if (!strcmp(q->schedalg, "random")) 
            {
                //math library is buggy with linux
                //int quantity = ceil(q->size / 2);

                int quantity = q->size / 2;
                if (q->size % 2)
                {
                    quantity++;
                }
                for (int i = 0; i < quantity; i++)
                {
                    cond_dequeue(q, rand() % q->size);
                }
            }
            else
            {
                //arguments where not passed correctly by user - abort
                exit(1);
            }
        }

        //queue is not full, can insert
        if (q->size > 0) 
        {
            Node* temp = q->rear;
            q->rear = to_insert;
            q->rear->prev = temp;
            temp->next = to_insert;
        }
        else 
        {
            q->front = to_insert;
            q->rear = to_insert;
        }
        q->size++;
        pthread_cond_signal(&q->deletion_allowed);
        pthread_mutex_unlock(&q->global_lock);
        return true;
    }
    return false;
}

Node* dequeue(Queue* q, bool is_critical) 
{
    if (q)
    {
        if (is_critical)
        {
            pthread_mutex_lock(&q->global_lock);
        }
        Node* to_dequeue;
        while (isEmpty(q)) 
        {
            pthread_cond_wait(&q->deletion_allowed, &q->global_lock);
        }
        //queue not empty, can dequeue
        to_dequeue = q->front;
        if (q->size > 1) 
        {
            q->front = (q->front)->next;
            q->front->prev = NULL;

        }
        else 
        {
            q->front = NULL;
            q->rear = NULL;
        }

        q->size--;
        pthread_cond_signal(&q->insertion_allowed);
        if (q->size == 0)
        {
            pthread_cond_signal(&q->is_empty);
        }

        if (is_critical) 
        {
            pthread_mutex_unlock(&q->global_lock);
        }
        return to_dequeue;
    }
    return NULL;
}

bool cond_dequeue(Queue* q, int index)
{
    if (!q || q->size == 0 || index < 0)
    {
        return false;
    }

    Node* to_dequeue = q->front;
    //find q[index]
    for (int i = 0; i < index; i++)
    {
        to_dequeue = to_dequeue->next;
    }
    
    if (q->size > 1) 
    {
        if (to_dequeue->prev)
        {
            Node* temp = to_dequeue->prev;
            temp->next = to_dequeue->next;
        }
        else
        {
            q->front = to_dequeue->next;
        }
        if (to_dequeue->next)
        {
                Node* temp = to_dequeue->next;
                temp->prev = to_dequeue->prev; 
        }
        else
        {
            q->rear = to_dequeue->prev;
        }
    }
    else 
    {
        q->front = NULL;
        q->rear = NULL;
    }
    q->size--;
    Close(to_dequeue->data->connfd);
    free(to_dequeue->data);
    free(to_dequeue);
    return true;
}

bool isEmpty(Queue* q) 
{
    return q && q->size == 0;
}

bool isFull(Queue* q, int size)
{
    return q && q->size == size;
}

void freeQueue(Queue* q) 
{
    if (q)
    {
        Node* to_free = q->front;;
        while (to_free)
        {
            Node* temp= to_free->next;
            free(to_free);
            to_free = temp;
        }
        free(q->schedalg);
        free(q);
    }
}
