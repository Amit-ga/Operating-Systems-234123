#ifndef NODE_H
#define NODE_H

#include "request.h"
#include "thread.h"

/* global vars */
pthread_t* threads; //working threads array

/* Node struct definition */
typedef struct Node
{
    Request* data;
    struct Node* prev;
    struct Node* next;
} Node;

/* Nodes mathods */
Node* makeNode(int connfd);

#endif //QUEUE_H