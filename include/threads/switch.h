#ifndef TINY_OS_SWITCH_H
#define TINY_OS_SWITCH_H

#define REG_SIZE 8
#define NUM_CALLEE_SAVED_REGS 12

#ifndef __ASSEMBLER__

#include <lib/stdint.h>

typedef struct {
    // Return address
    uint64_t ra;

    // callee saved registers
    uint64_t s[NUM_CALLEE_SAVED_REGS];
} context_t;


context_t* switch_contexts(context_t** cur, context_t** next);

void __schedule_tail_entry(context_t* prev);

#endif


#endif //TINY_OS_SWITCH_H