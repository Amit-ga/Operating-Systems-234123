#include "segel.h"
#include "request.h"
#include "queue.h"
#include "scheduler.h"
// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

/* server global vars */
int pool_size;
int http_connections_nums;
char* schedalg;
int max_size;
pthread_mutex_t global_lock;
pthread_cond_t insertion_allowed;
pthread_cond_t deletion_allowed;
pthread_cond_t is_empty;
Scheduler* scheduler;

// HW3: Parse the new arguments too

void getargs(int* port, int argc, char* argv[])
{
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    pool_size = atoi(argv[2]);
    http_connections_nums = atoi(argv[3]);
    schedalg = malloc(strlen(argv[4]) * sizeof(char) + 1);
    if (!schedalg)
    {
        printf("Memmory allocation error! \n");
        return;
    }
    strcpy(schedalg, argv[4]);
    
    if (argc == 6)
    {
        max_size = atoi(argv[5]);
    }
}

void* requests_handler(void* id) 
{
    while (true) 
    {
        //keep in scheduling upcoming requests
        schedule(scheduler, *(int*)id);
    }
}

int main(int argc, char* argv[]) {
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    //init user arguments
    getargs(&port, argc, argv);

    //
    // HW3: Create some threads...
    //

    //init lock
    pthread_mutex_init(&global_lock, NULL);
    pthread_cond_init(&deletion_allowed, NULL);
    pthread_cond_init(&insertion_allowed, NULL);
    pthread_cond_init(&is_empty, NULL);

    //init global vars
    requests = malloc(pool_size * sizeof(Request*));
    if (!requests)
    {
        printf("Memmory allocation error! \n");
        return -1;
    }
    threads = malloc(pool_size * sizeof(pthread_t));
    if (!threads)
    {
        printf("Memmory allocation error! \n");
        free(requests);
        return -1;
    }
    threads_handler = malloc(pool_size * sizeof(threads_handler));
    if (!threads_handler)
    {
        printf("Memmory allocation error! \n");
        free(requests);
        free(threads);
        return -1;
    }
    scheduler = makeScheduler(pool_size, http_connections_nums, schedalg, max_size, global_lock, insertion_allowed,
        deletion_allowed, is_empty);
    if (!scheduler)
    {
        printf("Memmory allocation error! \n");
        free(requests);
        free(threads);
        free(threads_handler);
        return -1;
    }

    //init indexes
    int* workers_index = malloc(pool_size * sizeof(int));
    if (!workers_index)
    {
        printf("Memmory allocation error! \n");
        free(requests);
        free(threads);
        free(threads_handler);
        freeScheduler(scheduler);
        return -1;
    }
    for (int i = 0; i < pool_size; i++)
    {
        workers_index[i] = i;
    }

    //init worker threads
    for (int i = 0; i < pool_size; i++) 
    {
        //allocate new threads, if one fails - exit
        if (pthread_create(&threads[i], NULL, (void*)requests_handler, (void*)&workers_index[i])) 
        {
            exit(1);
        }
        threads_handler[i] = makeThread(i);
    }
    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA*)&clientaddr, (socklen_t*)&clientlen);

        //
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads
        // do the work.
        //
        request(scheduler, connfd);
    }
}