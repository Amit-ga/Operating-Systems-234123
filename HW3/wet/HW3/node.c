#include "node.h"

/* Nodes mathods implementation */
Node* makeNode(int connfd)
{
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node)
    {
        printf("Memmory allocation error! \n");
        return NULL;
    }
    new_node->prev = NULL;
    new_node->next = NULL;
    new_node->data = makeRequest(connfd);
    return new_node;
}