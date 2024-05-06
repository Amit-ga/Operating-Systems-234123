#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "segel.h"

/* request struct definition */
typedef struct Request
{
    //The arrival time, as first seen by the master thread
    struct timeval stat_req_arrival;
    //The dispatch time diff (=the duration between the arrival time and)
    struct timeval stat_req_dispatch;
    //request connection decsiptor
    int connfd;
} Request;

/* Request mathods */
Request* makeRequest(int connfd);
void requestError(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg, int id);
void requestReadhdrs(rio_t *rp);
int requestParseURI(char *uri, char *filename, char *cgiargs);
void requestGetFiletype(char *filename, char *filetype);
void requestServeDynamic(int fd, char *filename, char *cgiargs, int id);
void requestServeStatic(int fd, char *filename, int filesize, int thread_id);
void requestHandle(int fd, int id);

/* global vars */
Request** requests; //saves the requests themselves (the data of the nodes)

#endif