////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/21/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        string.c
//      Environment: Tiny OS
//      Description: string.c contains several functions to manipulate C strings (char*) and arrays.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <lib/stdint.h>
#include <lib/string.h>

#include <debug.h>

/*
 * Function:    memcpy
 * -------------------
 * This function copies the values of [n] bytes from the location pointed to by [src]
 * to the memory block pointed to by [dst].
 *
 * @void* dst:      The pointer to the destination where the contents is to be copied.
 * @void* src:      The pointer to the source of data to be copied.
 * @size_t n:       The number of bytes to copy.
 *
 * @returns:        The pointer to the destination block is returned
 *
 */
void* memcpy(void* dst, const void *src, size_t n) {
    assert(dst != null);
    assert(src != null);

    uint8_t* d = dst;
    const uint8_t* s = src;

    while (n-- > 0) *d++ = *s++;

    return dst;
}

/*
 * Function:    memset
 * -------------------
 * This function sets the first [n] bytes of the block of memory pointed by [dst]
 * to the specified [value] (interpreted as a uint8_t)
 *
 * @void* dst:      The pointer to the destination where the contents is to be set.
 * @int value:      The value to be set. The value is passed as int (according to the libc definition)
 *                  but is interpreted as a uint8_t.
 * @size_t n:       The number of bytes to set.
 *
 * @returns:        The pointer to the destination block is returned
 *
 */
void* memset(void* dst, int value, size_t n) {
    uint8_t* d = dst;

    assert(dst != null);

    while (n-- > 0) *d++ = value;

    return dst;
}

/*
 * Function:    memcmp
 * -------------------
 * This function compares the first [n] bytes of the block of memory pointed by [ptr1]
 * and the block of memory pointed by [ptr2]. Returning 0 if they match or a value different
 * from zero.
 *
 * @void* ptr1:     The pointer to the first block of memory.
 * @void* ptr2:     The pointer to the second block of memory.
 * @size_t n:       The number of bytes to compare.
 *
 * @returns:        An integral value indicating relationship between contents of memory blocks.
 *                  return = 0  => the contents of both memory blocks are equal
 *                  return < 0  => the first byte that doesn't match both blocks has a lower value in ptr1
 *                                 than ptr2 (if uint8_t is used to interpret the bytes)
 *                  return > 0  => the first byte that doesn't match both blocks has a greater value in ptr1
 *                                 than ptr2 (if uint8_t is used to interpret the bytes)
 *
 */
int memcmp(const void* ptr1, const void* ptr2, size_t n) {
    assert(ptr1 != null);
    assert(ptr2 != null);

    const uint8_t* p1 = ptr1;
    const uint8_t* p2 = ptr2;

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

char* strcpy(char* dst, const char* src) {
    assert(dst != null);
    assert(src != null);

    char* tmp = dst;
    while ((*dst++ = *src++) != '\0');

    return tmp;
}

char* strncpy(char* dst, const char* src, size_t n) {
    assert(dst != null);
    assert(src != null);

    char* tmp = dst;
    while (n) {
        if ((*tmp = *src) != '\0') src++;
        tmp++; n--;
    }

    return dst;
}

char* strcat(char* dst, const char* src) {
    assert(dst != null);
    assert(src != null);

    char* tmp = dst;

    size_t size = strlen(dst);
    while (*src != '\0') dst[size++] = *src++;
    dst[size] = '\0';

    return tmp;
}

