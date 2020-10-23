//
// Created by Alistair O'Brien on 10/1/2020.
//

#ifndef TINY_OS_SYNCH_H
#define TINY_OS_SYNCH_H

#include <lib/stdint.h>
#include <lib/stdbool.h>
#include <lib/list.h>

#include <threads/thread.h>

typedef struct {
    uint64_t value;
    list_t waiters; // A list of threads
} semaphore_t;

void semaphore_init(semaphore_t* semaphore, uint64_t value);

bool semaphore_try_down(semaphore_t* semaphore);
void semaphore_down(semaphore_t* semaphore);
void semaphore_up(semaphore_t* semaphore);

typedef struct {
    thread_t* holder;
    semaphore_t semaphore;
} lock_t;

void lock_init(lock_t* lock);

bool lock_try_acquire(lock_t* lock);
void lock_acquire(lock_t* lock);
void lock_release(lock_t* lock);

#endif //TINY_OS_SYNC_H
