/*
 * Kyle Verrier
 * kverrier@andrew
 * Proxy Lab
 *
 * Proxy Header File:
 * Defines Request and Response structures as well as function protoypes
 */
#include "csapp.h"

/* Structs */
typedef struct {
  char full_req[MAXLINE];
  char method[MAXLINE];
  char host[MAXLINE];
  char uri[MAXLINE];
  char version[MAXLINE];
  char req_str[MAXLINE];
  char host_str[MAXLINE];
  int port;
}Request;

typedef struct {
  char *content;
  int content_size;
}Response;

/* Function prototypes */
void *handle_requests(int);

int forward_request(int clientfd, Request *req, Response *);
int forward_response( int, int, Response *);

void send_cache_content(int, Response *);

void parse_request(int clientfd, Request *req);
void parse_url (char *src, char *domain, int *port, char *uri);
void *thread(void *);

void proxy_error(char *);


#define NTHREADS 100
#define SBUFSIZE 20
