//
// Created by Alistair O'Brien on 6/21/2020.
//

#include <lib/stdint.h>
#include <lib/string.h>

#include <debug.h>

void* memcpy(void* dst, const void *src, size_t n) {
    uint8_t* d = dst;
    const uint8_t* s = src;

    assert(d != null || n == 0);
    assert(s != null || n == 0);

    while (n-- > 0) *d++ = *s++;

    return dst;
}

void* memset(void* dst, int value, size_t n) {
    uint8_t* d = dst;

    assert(d != null || n == 0);

    while (n-- > 0) *d++ = value;

    return dst;
}

int memcmp(const void* ptr1, const void* ptr2, size_t n) {
    const uint8_t* p1 = ptr1;
    const uint8_t* p2 = ptr2;

    assert(p1 != null || n == 0);
    assert(p2 != null || n == 0);

    for (; n-- > 0; p1++, p2++) {
        if (*p1 != *p2) return *p1 > *p2 ? 1 : -1;
    }
    return 0;
}

size_t strlen(const char* str) {
    const char* p;

    assert(str != null);
    for (p = str; *p != '\0'; p++) continue;
    return (p - str);
}

size_t strnlen(const char* str, size_t n) {
    size_t m;
    for (m = 0; str[m] != '\0' && m < n; m++) continue;
    return m;
}

