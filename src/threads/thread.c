#include <lib/stdbool.h>
#include <lib/string.h>

#include <debug.h>
#include <param.h>
#include <riscv.h>

#include <mm/pmm.h>
#include <mm/malloc.h>
#include <mm/vmm.h>

#include <trap/interrupt.h>

#include <dev/timer.h>

#include <threads/synch.h>
#include <threads/thread.h>


// Since we're implementing a preemptive round-robin scheduling algorithm, we need to define the time slice or quantum
// for each thread. This is the number of timer ticks that the thread is allocated before being preempted.
#define TIME_SLICE  (10000)

// A list of threads with the thread->status == THREAD_READY. This list acts as a fifo queue which
// we push and pop threads from.
static list_t ready_threads;

static proc_t kernel_proc;


#define THREAD_MAGIC (0xe87ab59efc899600)

static lock_t tid_lock;
static tid_t next_tid = 1;

static thread_t* idle_thread;
static thread_t kernel_threads[NUM_HART];

static void scheduler_push(thread_t* t);
static void schedule(void);

static void proc_register_thread(thread_t* t);
static void proc_deregister_thread(thread_t* t);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HART                                                                                                          //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Function:    hart_current
 * -------------------------
 * Returns the current hardware thread (hart).
 *
 * Note that this function must be called with interrupts disabled.
 * This is to prevent a race condition where the tp (used to store the
 * hart id) is changed after being read but before we return.
 * (Critical section being lines 2-3 of function).
 *
 * @returns:    the current hart.
 *
 */
//hart_t* hart_current(void) {
//    assert(intr_get_state() == INTR_OFF);
//
//    uint64_t hartid = r_hartid();
//    return &harts[hartid];
//}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// THREADS                                                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define switch_threads(cur, next) ((thread_t*)(PAGE_ROUND_DOWN((uint64_t)switch_contexts(&cur->ctx, &next->ctx))))

/*
 * Function:    allocate_tid
 * -------------------------
 * This function returns a new tid, a thread identifier, to use for a new thread.
 *
 * Note that due to concurrency, we use a lock to increment the next_tid value. (When we have multi-harts)
 *
 * @returns:    The new tid that is to be used for a new thread.
 *
 */
static tid_t allocate_tid(void) {
    tid_t tid;

    lock_acquire(&tid_lock);
    tid = next_tid++;
    lock_release(&tid_lock);

    return tid;
}

/*
 * Function:    is_thread
 * ----------------------
 * This function determines whether a given pointer to a potential thread structure.
 * Recall that the thread structure has a magic attribute (to prevent stack overflows).
 * Hence a thread is valid iff the magic numbers are equal.
 *
 * @returns:    Whether the thread structure with address [t] is valid.
 *
 */
static inline bool is_thread(thread_t* t) {
    return t != null && t->magic == THREAD_MAGIC;
}

/*
 * Function:    thread_current
 * ---------------------------
 * This function is a wrapper for the internal __thread_current function.
 * We add serveral sanity checks before returning.
 *
 * @returns:    The pointer to the current running thread.
 *
 */
static thread_t* __thread_current(void) {
    uint64_t sp;
    asm ("mv %0, sp" : "=r" (sp));
    return (thread_t*)(PAGE_ROUND_DOWN(sp));
}

/*
 * Function:    thread_current
 * ---------------------------
 * This function is a wrapper for the internal __thread_current function.
 * We add serveral sanity checks before returning.
 *
 * @returns:    The pointer to the current running thread.
 *
 */
thread_t* thread_current(void) {

    thread_t* t = __thread_current();

    // Here, we make sure that [t] is really a thread.
    // We assert that [t] is a thread by inspecting the [magic] field in
    // the thread struct, this will either indiciate whether a stack overflow has occured (v bad)
    // or whether the thread is invalid (e.g. because we've just read some garbage... :( )

    assert(is_thread(t));
    assert(t->status == THREAD_RUNNING);

    return t;
}

/*
 * Function:    thread_tid
 * -----------------------
 * This function returns the tid (thread identifier) of the currently executing thread.
 *
 * @returns:    The tid of the currently running thread.
 *
 */
inline tid_t thread_tid(void) {
    return thread_current()->tid;
}

/*
 * Procedure:   thread_block
 * -------------------------
 * This procedure puts the current thread to sleep. It will not be scheduled again
 * until awoken by thread_unblock().
 *
 * This procedure must be called with interrupts turned off.
 *
 */
void thread_block(void) {
    intr_state_t state = intr_disable();

    thread_current()->status = THREAD_BLOCKED;
    schedule();

    intr_set_state(state);
}

/*
 * Procedure:   thread_unblock
 * ---------------------------
 * This procedure transitions a blocked thread [t] into the ready state.
 *
 * Note that [t] must be a THREAD_BLOCKED state. Use thread_yield() to transition
 * the currently running thread to the ready state.
 *
 * @thread_t* t:    The pointer to the blocked thread.
 *
 */
void thread_unblock(thread_t* t) {
    assert(is_thread(t));

    intr_state_t state = intr_disable();

    assert(t->status == THREAD_BLOCKED);
    scheduler_push(t);

    intr_set_state(state);
}

/*
 * Procedure:   thread_exit
 * ------------------------
 * This procedure deschedules the current thread and exits it with exit code [code].
 * Once descheduled, the thread is destroyed (eventually by the scheduler).
 *
 * Note that this procedure never returns to the caller.
 *
 * @uint64_t code:  The exit code of the thread.
 *
 */
void thread_exit(uint64_t code) {
    thread_t* t = thread_current();

    intr_disable();
    t->status = THREAD_DEAD;
    t->exit_code = code;

    // Schedule will free the resources for us :)
    schedule();
    NOT_REACHABLE;
}

/*
 * Procedure:   thread_yield
 * -------------------------
 * This procedure yields the current hart. The current thread is
 * preempted and placed at the back of the scheduler's ready queue.
 *
 * Note that the current thread is not to sleep and may be scheduled again immediately
 * at the scheduler's whim.
 *
 */
void thread_yield(void) {
    thread_t* t = thread_current();

    intr_state_t state = intr_disable();

    if (t != idle_thread) scheduler_push(t);
    schedule();

    intr_set_state(state);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Procedure:   __thread_init
 * --------------------------
 *
 */
static void __thread_init(thread_t* t, char* name, proc_t* p) {
    assert(t != null && p != null);

    t->magic = THREAD_MAGIC;
    t->status = THREAD_NEW;

    strncpy(t->name, name, 32);

    t->proc = p;
    proc_register_thread(t);

    t->exit_code = -1;
}

static thread_t* alloc_thread(char* name, proc_t* p) {
    assert(p != null);

    thread_t* t = alloc_page();
    if (t == null) return null;

    __thread_init(t, name, p);

    return t;
}

static inline void thread_run(thread_t* t) {
    t->status = THREAD_RUNNING;
    t->scheduler_info.time = TIME_SLICE;
}

static void free_thread(thread_t* t) {
    assert(is_thread(t));
    assert(t->status == THREAD_DEAD);

    free_page(t);
    proc_deregister_thread(t);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SCHEDULER                                                                                                          //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tiny OS uses a preemptive round-robin scheduling algorithm. We have two fifo queues, a ready threads queue,
// designed to store threads whose status is THREAD_READY and an exited threads queue, which stores threads whose
// status is THREAD_DEAD.
//
// As noted below, Tiny OS's threads and process structures are allocated on the heap, so when a thread dies we need
// to free it's resources and potentially free the process. We do so via the exited threads queue. Prior to scheduling
// a new thread, the scheduler frees all resources related to dead threads. If a process has no threads, then it
// is also freed.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static void scheduler_init(void) {
    list_init(&ready_threads);
}

/**
 * Procedure:   scheduler_push
 * ---------------------------
 * This procedure schedules a new thread. We maintain the assertion that
 * all threads in the ready_threads have the status THREAD_READY.
 *
 * @thread_t* t:
 *
 */
static void scheduler_push(thread_t* t) {
    assert(t != null);

    // Set the status of the thread
    t->status = THREAD_READY;

    // Add the thread to the ready queue.
    list_push_tail(&ready_threads, &t->list_node);
}


/**
 * Function:    scheduler_pop
 * --------------------------
 * Determines the next thread to be scheduled. Since we always must have a thread running we need to check whether
 * we have any usable/useful threads scheduled. If not, then return the idle_thread (a thread that simply waits).
 * Otherwise pop the next thread from the fifo ready threads queue.
 *
 * @returns:
 *
 */
static thread_t* scheduler_pop(void) {
    return list_size(&ready_threads) == 0
        ? idle_thread
        : LIST_VALUE(thread_t, list_node, list_pop_head(&ready_threads));
}

/**
 * Procedure:   scheduler_tick
 * ---------------------------
 * This procedure is called by the timer interrupt handler at each timer tick.
 * The
 * If the thread reaches the end of it's time slice, it's preempted.
 *
 */
void scheduler_tick(void) {
    thread_t* t = thread_current();

    uint64_t ts = timer_ticks();
    if (ts % 100000 == 0) {
        info("%d ticks\n", ts);
    }

    // Enforce preemption
    if (--t->scheduler_info.time == 0) {
        // info("preempting thread %d...\n", t->tid);
        thread_yield();
    }
}

void __schedule_tail(thread_t* prev) {
    assert(intr_get_state() == INTR_OFF);

    thread_t* cur = __thread_current();
    assert(is_thread(cur));
    assert(cur != prev);

    thread_run(cur);

    if (prev != null && prev->status == THREAD_DEAD) {
        free_thread(prev);
    }
}

/*
 * Procedure:   schedule
 * ---------------------
 *
 */
static void schedule(void) {
    thread_t* cur = __thread_current();

    // The scheduler cannot be run while interrupts are enabled.
    assert(intr_get_state() == INTR_OFF);

    assert(is_thread(cur));
    assert(cur->status != THREAD_RUNNING);

    // Determine the next thread to be scheduled.
    thread_t* next = scheduler_pop();
    assert(is_thread(next));

    // If a context switch is required, execute it.
    context_t* prev = null;
    if (cur != next) {
        prev = switch_contexts(&cur->ctx, &next->ctx);
    }
    __schedule_tail((thread_t*)PAGE_ROUND_DOWN((uint64_t)prev));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PROCESSES                                                                                                          //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static void proc_init(void) {
    assert(intr_get_state() == INTR_OFF);

//    lock_init(&kernel_proc.lock);

    strncpy(kernel_proc.name, "kernel", 32);
}

static void proc_vm_init(void) {
    info("kproc pagetable set.\n");
    kernel_proc.pagetable = kpagetable();
}


static void proc_register_thread(thread_t* t) {
    assert(is_thread(t));
    assert(t->status == THREAD_NEW);

    proc_t* p = t->proc;

//    lock_acquire(&p->lock);
    p->thread_count++;
//    lock_release(&p->lock);
}

static void proc_deregister_thread(thread_t* t) {
    assert(is_thread(t));
    assert(t->status == THREAD_DEAD);

    proc_t* p = t->proc;

//    lock_acquire(&p->lock);
    if (--p->thread_count == 0 && p != &kernel_proc) {
        free_page(p->pagetable);
        free(p);
    }
//    lock_release(&p->lock);
}


inline proc_t* proc_current(void) {
    return thread_current()->proc;
}

inline const char* proc_name(void) {
    return proc_current()->name;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// KERNEL THREADS                                                                                                     //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void kthread(void (*function)(void*), void* arg) {
    assert(function != null);

    intr_enable();
    function(arg);
    thread_exit(0);
}


static thread_t* __kthread_create(char* name, void (*function)(void*), void* arg) {
    assert(function != null);

    thread_t* t = alloc_thread(name, &kernel_proc);
    t->tid = allocate_tid();

    // Store the trap frame on the kernel stack of the thread :)
    uint64_t sp = (uint64_t)t + PAGE_SIZE;

    trap_frame_t* tf = (trap_frame_t*)(sp -= sizeof(trap_frame_t));
    info("tf %#p\n", tf);

    bzero(tf, sizeof(trap_frame_t));

    // Setup execution of function with arg passed to register a0 (first arg register).
    tf->epc = (uint64_t)kthread;
    tf->regs[10] = (uint64_t)function;
    tf->regs[11] = (uint64_t)arg;

    // Set the status s.t previous privelege is supervisor mode and interrupts are enabled.
    tf->status = (r_sstatus() | SSTATUS_SPP_S | SSTATUS_SPIE) & ~SSTATUS_SIE_MASK;
    info("status %#p\n", tf->status);

    // Set the sp (stored in x2)
    tf->regs[2] = sp;

    // Having defined the trap frame, now define the context.
    context_t* ctx = (context_t*)(sp -= sizeof(context_t));
    ctx->ra = (uint64_t)__schedule_tail_entry;

    t->ctx = ctx;

    // Setup the kernel context, we need to return to __schedule_tail_entry. This calls schedule tail
    // and then returns to s_ret_trap :)

    return t;
}

tid_t kthread_create(char* name, void (*function)(void*), void* arg) {
    thread_t* t = __kthread_create(name, function, arg);
    if (t == null) return -1;

    t->status = THREAD_BLOCKED;
    tid_t tid = t->tid;

    thread_unblock(t);

    return tid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// THREAD INITIALIZATION                                                                                              //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void idle(void* __idle_started) {
    semaphore_t* idle_started = __idle_started;

    idle_thread = thread_current();
    semaphore_up(idle_started);

    for (;;) {
        info("Idle thread running... Now blocking.\n");
        info("%d ticks\n", timer_ticks());
        thread_block();
    }
}

void thread_hart_init(void) {
    assert(intr_get_state() == INTR_OFF);

    thread_t* init = __thread_current();

    // Initialize the thread.
    __thread_init(init, "kernel", &kernel_proc);

    // Set thread to be running and current (required for allocate_tid to work)
    thread_run(init);

    // Now allocate tid :)
    init->tid = allocate_tid();
}

/*
 * Procedure:   thread_init
 * ------------------------
 * This procedure initializes the threading system by transforming the code
 * that the kernel's current executing into a thread. This can't work in general and
 * it only works because the stack is page aligned.
 *
 * Also initializes the ready queue and the tid lock.
 *
 * After calling this procedure, ensure that the page allocator is initialized
 * before using thread_create.
 *
 * Note that is unsafe to call thread_current() until this procedure is executed.
 *
 */
void thread_init(void) {
    assert(intr_get_state() == INTR_OFF);

    scheduler_init();
    proc_init();

    lock_init(&tid_lock);
}

void thread_vm_init(void) {
    proc_vm_init();
}

void th_f1(void* data) {
    info("Thread %d started...\n", thread_tid());
    info("%d ticks\n", timer_ticks());
}


void scheduler_start(void) {
    semaphore_t idle_started;
    semaphore_init(&idle_started, 0);



    kthread_create("idle", idle, &idle_started);

    // Start preemptive thread scheduling
    intr_enable();

    semaphore_down(&idle_started);
    info("Idle thread executed :)\n");

    tid_t t1 = kthread_create("t1", th_f1, null);
    info("Thread t1: %d\n", t1);

    tid_t t2 = kthread_create("t2", th_f1, null);
    info("Thread t2: %d\n", t2);

}