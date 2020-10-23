//
// Created by Alistair O'Brien on 6/21/2020.
//

#ifndef TINY_OS_STRING_H
#define TINY_OS_STRING_H

#include <lib/stddef.h>

// Standard Memory Methods
void* memcpy(void* dst, const void* src, size_t n);
void* memset(void* ptr, int value, size_t n);
int	  memcmp(const void* ptr1, const void* ptr2, size_t n);

#define bzero(ptr, n) (memset(ptr, '\0', n), (void) 0)

char* strcat(char* dst, const char* src);
//char* strncat(char* dst, const char* src, size_t n);
//
//int strcmp(const char* str1, const char* str2);
//int strncmp(const char* str1, const char* str2, size_t n);
//
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, size_t n);
//
//char* strstr(const char* str1, const char* str2);
//
//char* strchr(const char* str, int c);
//char* strrchr(const char* str, int c);

size_t strlen(const char* str);
size_t strnlen(const char* str, size_t n);




#endif //TINY_OS_STRING_H
