/*
 * mm_alloc.c
 */

#include "mm_alloc.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef struct metadata {
    size_t size;
	struct metadata* prev;
   	struct metadata* next;
    int free; // 1 if the block is free, 0 if the block is allocated
}metadata;

static metadata* head = NULL;
static metadata* tail = NULL;

void* mm_malloc(size_t size) {
  //TODO: Implement malloc
  if (size == 0) return NULL;
  // Check if the size is too large
  if (size > SIZE_MAX - sizeof(metadata)) {
		return NULL;
  }
  // If the list is empty, create a new block
  if (head == NULL) {
      void* new_block = sbrk(size + sizeof(metadata));
	  if (new_block == (void*) -1) {
          return NULL;
      }

	  head = (metadata*) new_block;
      head->size = size;
      head->prev = NULL;
      head->next = NULL;
      head->free = 0;
      tail = head;
      return (void*)((char*)head + sizeof(metadata));
  }

  for (metadata* curr = head; curr != NULL; curr = curr->next) {
      // If the current block is free and the size is large enough, allocate it
      if (curr->free && curr->size >= size + sizeof(metadata) && curr->size - size - sizeof(metadata) < sizeof(metadata)) {
          curr->free = 0;
		  curr->size = size;
          return (void*)((char*)curr + sizeof(metadata));
      } else if (curr->free && curr->size > size + sizeof(metadata) * 2) {
          // If the current block is free and the size is too large, split the block
          metadata* new_block = (metadata*)((char*)curr + sizeof(metadata) + size);
          new_block->size = curr->size - size - sizeof(metadata);
          new_block->prev = curr;
          new_block->next = curr->next;
          new_block->free = 1;
          if (curr->next != NULL) {
              curr->next->prev = new_block;
          }
          curr->next = new_block;
          curr->size = size;
          curr->free = 0;
          return (void*)((char*)curr + sizeof(metadata));
      }
  }

  // If no block is found, allocate a new block
  void* block = sbrk(size + sizeof(metadata));
  if (block == (void*) -1) {
      return NULL;
  }

  metadata* new_block = (metadata*)block;

  new_block->size = size;
  new_block->prev = NULL;
  new_block->next = NULL;
  new_block->free = 0;
  tail->next = new_block;
  new_block->prev = tail;
  tail = new_block;
  return (void*)((char*)new_block + sizeof(metadata));
}

// Reallocates the memory block pointed to by ptr to have new size bytes
void* mm_realloc(void* ptr, size_t size) {
	//TODO: Implement realloc
	// If the size is 0, free the block and return NULL
	if (ptr && size == 0) {
        mm_free(ptr);
        return NULL;
    }
    // If the pointer is NULL, allocate a new block
	if (ptr == NULL) {
        return mm_malloc(size);
    }
	metadata* block = (metadata*)((char*)ptr - sizeof(metadata));
	size_t old_size = block->size;
	void* new_ptr = mm_malloc(size);
	if (new_ptr == NULL) return NULL;
	memset(new_ptr, 0, size);
	memcpy(new_ptr, ptr, old_size < size ? old_size : size);
	mm_free(ptr);
    return new_ptr;
}

void mm_free(void* ptr) {
	//TODO: Implement free
	if (ptr == NULL) return;
	metadata* block = (metadata*)((char*)ptr - sizeof(metadata));
    block->free = 1;

    // If the previous block is free and the current block is free, coalesce the blocks
  	if (block->prev != NULL && block->prev->free) {
		metadata* prev_block = block->prev;
		if ((char*)prev_block + prev_block->size + sizeof(metadata) == (char*)block) {
			prev_block->size += block->size + sizeof(metadata);
			prev_block->next = block->next;
			if (block->next != NULL) {
				block->next->prev = prev_block;
          	}
		  	block = prev_block;
      	}
  	}

    // If the next block is free and the current block is free, coalesce the blocks
	if (block->next != NULL && block->next->free) {
    	metadata* next_block = block->next;
    		if ((char*)block + block->size + sizeof(metadata) == (char*)next_block) {
        		block->size += next_block->size + sizeof(metadata);
        		block->next = next_block->next;
        		if (next_block->next != NULL) {
            		next_block->next->prev = block;
        		}
    		}
	}
}
