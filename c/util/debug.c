#include "debug.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if __SIZEOF_POINTER__ == 4

uint32_t PtrToNum(const void* p) {
  uint32_t r = 0;
  memcpy(&r, &p, 4);
  return r;
}

#elif __SIZEOF_POINTER__ == 8

uint64_t PtrToNum(const void* p) {
  uint64_t r = 0;
  memcpy(&r, &p, 8);
  return r;
}

#endif  // __SIZEOF_POINTER__


void PrintMem(const uint8_t* ptr, int size, const char* buf_name) {
  if (buf_name) {
    printf("%s, addr:0x%lX, size:%d\n", buf_name, PtrToNum(ptr), size);
  }
  if (ptr == NULL) {
    printf("this val is NULL\n");
    return;
  }
  for (int i = 0; i < size; i++) {
    printf("0x%02X, ", ptr[i]);
    if (i % 8 == 7)
      printf("\n");
  }
  printf("\n");
}
