/*
 * Kyle Verrier
 * kverrier@andrew
 * Proxy Lab
 *
 * Cache:
 * Uses linked list with last access priority when evicting items.
 *
 */
#include "proxy.h"
#include "cache.h"


Cache_Block* in_cache(char *key){
  Cache_Block *current_block = cache.head;
  while (current_block != NULL){
    if(strcmp(current_block->full_req, key) == 0){
      return current_block;
    }
    current_block = current_block->next;
  }
  return NULL;
}

Cache_Block* create_block(Request *req, Response *resp){
  Cache_Block *new_block;
  new_block = (Cache_Block*)Malloc(sizeof(Cache_Block));
  strcpy(new_block->full_req, req->full_req);
  new_block->content = resp->content;
  new_block->size = resp->content_size;
  new_block->prev = NULL;
  new_block->next = NULL;
  return new_block;
}

void add_block(Cache_Block *block){
  if (block == NULL) {
    fprintf(stderr, "Unable to add NULL block to cache\n");
    abort();
  }

  if (cache.head == NULL) {
    cache.head = block;
    cache.tail = block;
  } else {
    if (cache.head->prev == NULL) {
      cache.head->prev = block;
      block->next = cache.head;
      block->prev = NULL;
      cache.head = block;
    } else {
      fprintf(stderr, "add_block error\n");
      abort();
    }
  }
  cache.size += block->size;
  if (cache.size > MAX_CACHE_SIZE) {
    fprintf(stderr, "MAX_CACHE_SIZE exceded\n");
    abort();
  }
}

void delete_block(Cache_Block *block){
  unlink_block(block);
  free_block(block);
  cache.size -= block->size;
}

void unlink_block(Cache_Block *block){
  Cache_Block *prev_block = block->prev;
  Cache_Block *next_block = block->next;

  if (block == NULL)
    fprintf(stderr, "error deleting empty block\n");

  if(prev_block != NULL) {
    prev_block->next = next_block;
  } else {
    if (cache.head == block) {
      cache.head = next_block;
      if (cache.head != NULL)
        cache.head->prev = NULL;
    }else {
      fprintf(stderr, "error deleting head block\n");
    }
  }

  if(next_block != NULL) {
    next_block->prev = prev_block;
  } else {
    if (cache.tail == block) {
      cache.tail = prev_block;
      if (cache.tail != NULL)
        cache.tail->next = NULL;
    }else {
      fprintf(stderr, "error deleting tail block\n");
    }
  }
  block->prev = NULL;
  block->next = NULL;
}

void free_block(Cache_Block *block) {
  free(block->content);
  free(block);
}

int check_cache(Request *req, Response *resp){
  pthread_mutex_lock(&mutex_lock);
  char *key = req->full_req;
  Cache_Block *block = NULL;
  if ((block = in_cache(key)) != NULL) {
    if (block == NULL) {
      printf("Cache returned NULL block\n");
      abort();
    }
    resp->content = block->content;
    resp->content_size = block->size;

    unlink_block(block);
    cache.size -= block->size; /*To prevent over adding */
    add_block(block);
    pthread_mutex_unlock(&mutex_lock);
    return 1;
  } else {
    pthread_mutex_unlock(&mutex_lock);
    return 0;
  }
}

void save_to_cache(Request *req, Response *resp) {
  pthread_mutex_lock(&mutex_lock);
  Cache_Block *block;
  block = create_block(req, resp);

  if (cache.size + block->size > MAX_CACHE_SIZE) {
    while( cache.size + block->size > MAX_CACHE_SIZE) {
      delete_block(cache.tail);
    }
  }
  add_block(block);
  pthread_mutex_unlock(&mutex_lock);

}

void init_cache() {
  cache.head = NULL;
  cache.tail = NULL;
  cache.size = 0;
}

