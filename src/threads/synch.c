#include <debug.h>

#include <trap/interrupt.h>


#include <threads/thread.h>
#include <threads/synch.h>


void semaphore_init(semaphore_t* s, uint64_t value) {
    assert(s != null);

    s->value = value;
    list_init(&s->waiters);
}

bool semaphore_try_down(semaphore_t* s) {
    assert(s != null);

    intr_state_t old_state = intr_disable();
    bool success;
    if (s->value > 0) {
        s->value--;
        success = true;
    } else {
        success = false;
    }
    intr_set_state(old_state);

    return success;
}

/*
 * Procedure:   semaphore_down
 * ---------------------------
 * This procedure implements the down or "P" operation on a semaphore.
 * The procedure waits for s->value to become postive and 
 * atomically decrements it. 
 * 
 * Note that this procedure may sleep, so should not be called within the 
 * context of an interrupt handler. Nor should this procedure be called with interrupts
 * disabled. (however, if it is, it will sleep until the next scheduled process, which will probably turn interrupts back on).
 * 
 * @semaphore_t* s:     
 * 
 */ 
void semaphore_down(semaphore_t* s) {
    assert(s != null);
    
    intr_state_t old_state = intr_disable();
    while (s->value == 0) {
        list_push_tail(&s->waiters, &thread_current()->list_node);
        thread_block();
    }

    s->value--;
    intr_set_state(old_state);

}

/*
 * Procedure:   semaphore_up
 * -------------------------
 * This procedure implements the up or "V" operation on a semaphore.
 * The procedure increments s->value and wakes up one of the scheduled processes 
 * that was blocked on s (if any).
 * 
 * Note that this procedure may be called within the context of an interrupt handler. 
 * 
 * @semaphore_t* s:
 * 
 */ 
void semaphore_up(semaphore_t* s) {
    assert(s != null);

    intr_state_t old_state = intr_disable();
    if (list_size(&s->waiters) != 0) {
        thread_unblock(LIST_VALUE(thread_t, list_node, list_pop_head(&s->waiters)));
    }

    s->value++;
    intr_set_state(old_state);
}

static inline bool lock_held_by_current_thread(const lock_t* l) {
    assert(l != null);

    return l->holder == thread_current();
}

void lock_init(lock_t* l) {
    assert(l != null);

    l->holder = null;
    semaphore_init(&l->semaphore, 1);
}

void lock_acquire(lock_t* l) {
    assert(l != null && !lock_held_by_current_thread(l));

    semaphore_down(&l->semaphore);
    l->holder = thread_current();
}

bool lock_try_acquire(lock_t* l) {
    assert(l != null && !lock_held_by_current_thread(l));

    bool success = semaphore_try_down(&l->semaphore);
    if (success) l->holder = thread_current();

    return success;
}

void lock_release(lock_t* l) {
    assert(l != null && lock_held_by_current_thread(l));

    l->holder = null;
    semaphore_up(&l->semaphore);
}


