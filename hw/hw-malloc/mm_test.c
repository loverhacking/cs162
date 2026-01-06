#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

/* Function pointers to hw3 functions */
void* (*mm_malloc)(size_t);
void* (*mm_realloc)(void*, size_t);
void (*mm_free)(void*);

static void* try_dlsym(void* handle, const char* symbol) {
  char* error;
  void* function = dlsym(handle, symbol);
  if ((error = dlerror())) {
    fprintf(stderr, "%s\n", error);
    exit(EXIT_FAILURE);
  }
  return function;
}

static void load_alloc_functions() {
  void* handle = dlopen("hw3lib.so", RTLD_NOW);
  if (!handle) {
    fprintf(stderr, "%s\n", dlerror());
    exit(EXIT_FAILURE);
  }

  mm_malloc = try_dlsym(handle, "mm_malloc");
  mm_realloc = try_dlsym(handle, "mm_realloc");
  mm_free = try_dlsym(handle, "mm_free");
}

/* Helper function to fill memory with pattern */
static void fill_pattern(void* ptr, size_t size, char pattern) {
  memset(ptr, pattern, size);
}

/* Helper function to verify pattern */
static int verify_pattern(void* ptr, size_t size, char pattern) {
  for (size_t i = 0; i < size; i++) {
    if (((char*)ptr)[i] != pattern) {
      return 0;
    }
  }
  return 1;
}

/* Test 1: Basic malloc and free */
static void test_basic_malloc_free() {
  printf("Test 1: Basic malloc and free... ");

  int* data = mm_malloc(sizeof(int));
  assert(data != NULL);
  *data = 0x162;
  assert(*data == 0x162);
  mm_free(data);

  printf("OK\n");
}

/* Test 2: Multiple allocations */
static void test_multiple_allocations() {
  printf("Test 2: Multiple allocations... ");

  void* ptrs[100];

  // Allocate 100 small blocks
  for (int i = 0; i < 100; i++) {
    ptrs[i] = mm_malloc(16);
    assert(ptrs[i] != NULL);
    fill_pattern(ptrs[i], 16, (char)i);
  }

  // Verify and free every other block
  for (int i = 0; i < 100; i += 2) {
    assert(verify_pattern(ptrs[i], 16, (char)i));
    mm_free(ptrs[i]);
  }

  // Allocate more blocks (should reuse freed memory)
  for (int i = 0; i < 50; i++) {
    void* p = mm_malloc(8);
    assert(p != NULL);
    mm_free(p);
  }

  // Free remaining blocks
  for (int i = 1; i < 100; i += 2) {
    mm_free(ptrs[i]);
  }

  printf("OK\n");
}

/* Test 3: Realloc tests */
static void test_realloc() {
  printf("Test 3: Realloc tests... ");

  // Test realloc from NULL (should act like malloc)
  int* p1 = mm_realloc(NULL, 10 * sizeof(int));
  assert(p1 != NULL);
  for (int i = 0; i < 10; i++) {
    p1[i] = i;
  }

  // Test realloc to larger size
  p1 = mm_realloc(p1, 20 * sizeof(int));
  assert(p1 != NULL);
  // Verify old data is preserved
  for (int i = 0; i < 10; i++) {
    assert(p1[i] == i);
  }
  // Fill new space
  for (int i = 10; i < 20; i++) {
    p1[i] = i;
  }

  // Test realloc to smaller size
  p1 = mm_realloc(p1, 5 * sizeof(int));
  assert(p1 != NULL);
  // Verify first 5 elements are preserved
  for (int i = 0; i < 5; i++) {
    assert(p1[i] == i);
  }

  // Test realloc to 0 (should free and return NULL)
  p1 = mm_realloc(p1, 0);
  assert(p1 == NULL);

  printf("OK\n");
}

/* Test 4: Edge cases and error handling */
static void test_edge_cases() {
  printf("Test 4: Edge cases... ");

  // Test malloc(0) should return NULL
  void* p = mm_malloc(0);
  assert(p == NULL);

  // Test very large allocation (should fail gracefully)
  void* huge = mm_malloc((size_t)-1);
  assert(huge == NULL);

  // Test free(NULL) should not crash
  mm_free(NULL);

  // Test small allocations
  for (int i = 1; i <= 8; i++) {
    void* small = mm_malloc(i);
    assert(small != NULL);
    mm_free(small);
  }

  printf("OK\n");
}

/* Test 5: Memory coalescing */
static void test_coalescing() {
  printf("Test 5: Memory coalescing... ");

  // Allocate three consecutive blocks
  void* p1 = mm_malloc(100);
  void* p2 = mm_malloc(100);
  void* p3 = mm_malloc(100);
  assert(p1 != NULL && p2 != NULL && p3 != NULL);

  // Save the break pointer
  void* old_brk = sbrk(0);

  // Free middle block first, then the others
  mm_free(p2);
  mm_free(p1);
  mm_free(p3);

  // Allocate a block that should fit in the coalesced space
  void* large = mm_malloc(300);
  assert(large != NULL);

  // Verify we didn't move the break (coalescing worked)
  void* new_brk = sbrk(0);
  assert(new_brk == old_brk);

  mm_free(large);

  printf("OK\n");
}



/* Test 6: Free order tests */
static void test_free_order() {
  printf("Test 6: Free order tests... ");

  void* p1 = mm_malloc(100);
  void* p2 = mm_malloc(100);
  void* p3 = mm_malloc(100);

  // Free in different orders to test coalescing logic
  // Order 1: p2, p1, p3 (free middle, then left, then right)
  mm_free(p2);
  mm_free(p1);
  mm_free(p3);

  // Allocate should get coalesced block
  void* large1 = mm_malloc(300);
  assert(large1 != NULL);
  mm_free(large1);

  // Re-allocate three blocks
  p1 = mm_malloc(100);
  p2 = mm_malloc(100);
  p3 = mm_malloc(100);

  // Order 2: p1, p3, p2 (free left, then right, then middle)
  mm_free(p1);
  mm_free(p3);
  mm_free(p2);

  void* large2 = mm_malloc(300);
  assert(large2 != NULL);
  mm_free(large2);

  printf("OK\n");
}

int main() {
  load_alloc_functions();

  printf("Starting memory allocator tests...\n\n");

  test_basic_malloc_free();
  test_multiple_allocations();
  test_realloc();
  test_edge_cases();
  test_coalescing();
  test_free_order();



  printf("\nAll tests passed successfully!\n");

  return 0;
}