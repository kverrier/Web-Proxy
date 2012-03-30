/*
 * Kyle Verrier
 * kverrier@andrew
 * Proxy Lab
 *
 * Cache Header File:
 * Defines block stucture that will make up the nodes of the list as well as
 * the list.
 */

#define MAX_OBJECT_SIZE 102400
#define MAX_CACHE_SIZE 1048576

#include "csapp.h"

typedef struct Cache_Block Cache_Block;
struct Cache_Block{
  char full_req[MAXLINE];
  char *content;
  int size;
  Cache_Block *next;
  Cache_Block *prev;
};

typedef struct {
  int size;
  Cache_Block *head;
  Cache_Block *tail;
} Cache;

Cache cache;
pthread_mutex_t mutex_lock;

Cache_Block *in_cache(char *);
Cache_Block *create_block(Request *req, Response *resp);
void add_to_cache(Cache_Block *b);
void delete_block(Cache_Block *b);
void unlink_block(Cache_Block *b);
void free_block(Cache_Block *b);
int check_cache(Request *req, Response *resp);
void save_to_cache(Request *req, Response *resp);
void init_cache();

