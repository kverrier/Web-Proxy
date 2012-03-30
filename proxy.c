/*
 * Kyle Verrier
 * kverrier@andrew
 * Proxy Lab
 *
 *
 * Approach:
 * 1. Handle HTTP requests
 * 2. Deal with concurrency using Producer-Consumer Model and sbuf from
 *    textbook
 * 3. Cache using linked list
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "proxy.h"
#include "sbuf.h"
#include "cache.h"


/* Global Variables */
sbuf_t sbuf;

int main (int argc, char *argv [])
{
    int listenfd, port;
    int clientfd;
    int i;
    pthread_t tid;

    socklen_t socket_length;
    struct sockaddr_in client_socket;

    pthread_mutex_init(&mutex_lock, NULL);
    init_cache();

    if (argc != 2) {
      fprintf(stderr, "usage: %s <port>\n", argv[0]);
      exit(0);
    }
    port = atoi(argv[1]);
    socket_length = sizeof(client_socket);

    /* Handles the SIGPIPE signal by ignoring it */
    Signal(SIGPIPE, SIG_IGN);

    sbuf_init(&sbuf, SBUFSIZE);
    for (i = 0; i < NTHREADS; i++)
      Pthread_create(&tid, NULL, thread, NULL);

    listenfd = Open_listenfd(port);

    while (1) {
      clientfd = Accept(listenfd, (SA *)&client_socket, &socket_length);
      sbuf_insert(&sbuf, clientfd);
    }
    pthread_mutex_destroy(&mutex_lock);
}

void *handle_requests(int clientfd) {

    Request req;
    Response resp;

    parse_request(clientfd, &req);

    if (check_cache(&req, &resp)) {
      send_cache_content(clientfd, &resp);
    } else {
      if (forward_request(clientfd, &req, &resp) < 0) {
        close(clientfd);
        return NULL;
      } else {
        if (resp.content_size <= MAX_OBJECT_SIZE){
          save_to_cache(&req, &resp);
        }
      }
    }

    close(clientfd);
    return NULL;
}

int forward_request(int clientfd, Request *req, Response *resp){
  rio_t server_rio;
  int serverfd;

  if ((serverfd = open_clientfd(req->host, req->port)) < 0) {
    sleep(2);
    if ((serverfd = open_clientfd(req->host, req->port)) < 0) {
      sleep(2);
      if ((serverfd = open_clientfd(req->host, req->port)) < 0) {
        proxy_error("unable to connect");
        close(serverfd);
        return -1;
      }
    }
  }

  /* Write request to server  */
  rio_readinitb(&server_rio, serverfd);
  rio_writen(serverfd, req->req_str, strlen(req->req_str));
  rio_writen(serverfd, req->host_str, strlen(req->host_str));
  rio_writen(serverfd, "\r\n", strlen("\r\n"));

  if (forward_response(clientfd, serverfd, resp) < 0) {
    close(serverfd);
    return -1;
  }
  close(serverfd);
  return 1;
}

int forward_response(int clientfd, int serverfd, Response *resp){
  size_t n;
  char header_buffer[MAXLINE];
  char header[MAX_OBJECT_SIZE];
  char content_buffer[MAX_OBJECT_SIZE];
  char content[MAX_OBJECT_SIZE * 10]; /* Capable of holding large repsonses*/
  rio_t server_rio;

  rio_readinitb(&server_rio, serverfd);

  /* Copy Response Header */
  int header_size = 0;
  while((n = rio_readlineb(&server_rio, header_buffer, MAXLINE)) != 0) {
    memcpy(header + header_size, header_buffer, n * sizeof(char));
    header_size += n;

    if (!strcmp(header_buffer, "\r\n")) {
      break;
    }
  }

  /* Copy Response Header */
  int content_size = 0;
  while ((n = rio_readnb(&server_rio, content_buffer, MAX_OBJECT_SIZE)) != 0) {
    memcpy(content + content_size, content_buffer, n * sizeof(char));
    content_size += n;
  }

  /* Write Response to Client */
  if (rio_writen(clientfd, header, header_size) < 0) {
    proxy_error("unable to write response header to client");
    return -1;
  }
  if (rio_writen(clientfd, content, content_size) < 0) {
    proxy_error("unable to write response content to client");
    return -1;
  }

  /* Malloc content if right size to be future cached */
  if (content_size <= MAX_OBJECT_SIZE) {
    resp->content = Malloc(content_size * sizeof(char));
    memcpy(resp->content, content, content_size * sizeof(char));
    resp->content_size = content_size;
  } else {
    resp->content_size = content_size;
  }

  return 1;
}

void send_cache_content(int clientfd, Response *resp) {
  if (resp != NULL ){
    if (resp->content != NULL) {
      if (rio_writen(clientfd, resp->content, resp->content_size) < 0){
        printf("send_cache_content error: rio_writen\n");
      }
    } else {
      printf("send_cache_content error: content is NULL\n");
    }
  } else {
    printf("send_cache_content error: response is NULL\n");
  }
}

void *thread(void *vargp) {
  Pthread_detach(pthread_self());
  while (1) {
    int clientfd = sbuf_remove(&sbuf);
    handle_requests(clientfd);
  }
}

void proxy_error(char *msg) {
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}

void parse_request(int clientfd, Request *req){
    char buffer[MAXLINE];
    char full_req[MAXLINE];
    char method[MAXLINE], url[MAXLINE], host[MAXLINE], uri[MAXLINE], version[MAXLINE];
    int port;
    rio_t rio;

    rio_readinitb(&rio, clientfd);
    rio_readlineb(&rio, buffer, MAXLINE);

    sscanf(buffer, "%s %s %s", method, url, version);

    if (strcmp(version, "HTTP/1.0")) {
      strcpy(version, "HTTP/1.0");
    }

    sprintf(full_req, "%s %s %s", method, url, version);

    parse_url(url, host, &port, uri);

    strcpy(req->full_req, full_req);
    strcpy(req->method, method);
    strcpy(req->host, host);
    strcpy(req->uri, uri);
    strcpy(req->version, version);
    req->port = port;

    sprintf(req->req_str, "%s %s %s\r\n", method, uri, version);
    sprintf(req->host_str, "Host: %s\r\n", host);

}

void parse_url (char *url, char *host, int *port, char *uri){
  char buf[MAXLINE];
  char *port_loc;
  char *uri_loc;
  sscanf(url, "http://%s", buf);

  /* Pase URI if "/" is found */
  if ((uri_loc = strstr(buf, "/")) != NULL){
    sscanf(uri_loc, "%s", uri);
    *uri_loc = '\0';
  } else {
    uri[0] = '/';
    uri[1] = '\0';
  }

  /* Parse PORT if ":" is found */
  if ((port_loc = strstr(buf, ":")) != NULL){
    sscanf(port_loc + 1, "%d", port);
    *port_loc = '\0';
  } else {
    *port = 80;
  }

  strcpy(host, buf);

}
