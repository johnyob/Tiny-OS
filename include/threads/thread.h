//
// Created by Alistair O'Brien on 10/1/2020.
//

#ifndef TINY_OS_THREAD_H
#define TINY_OS_THREAD_H

#include <lib/stdint.h>
#include <lib/list.h>

#include <mm/vmm.h>
#include <trap/trap.h>

#include <threads/switch.h>


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// States in a thread life cycle :)
typedef struct {
    uint64_t time;
} scheduler_info_t;

void scheduler_start(void);
void scheduler_tick(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct proc {
    char name[32];

    pagetable_t pagetable;
    size_t thread_count;
} proc_t;


//// Processes
proc_t* proc_current(void);
const char* proc_name(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum {
    THREAD_NEW,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_DEAD
} thread_state_t;

typedef uint64_t tid_t;

typedef struct {
    // Thread metadata
    tid_t tid;
    char name[32];

    thread_state_t status;    
    proc_t* proc;
    int64_t exit_code;

    // Used for list processing
    list_node_t list_node;

    // Thread context
    context_t* ctx;

    // Scheduler info
    scheduler_info_t scheduler_info;

    // Prevents overflows :)
    uint64_t magic;
} thread_t;

void thread_init(void);
void thread_vm_init(void);
void thread_hart_init(void);


thread_t* thread_current(void);
tid_t thread_tid(void);

void thread_block(void);
void thread_unblock(thread_t* t);

void thread_exit(uint64_t code);
void thread_yield(void);

void __schedule_tail(thread_t* prev);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif //TINY_OS_PROCESS_H
