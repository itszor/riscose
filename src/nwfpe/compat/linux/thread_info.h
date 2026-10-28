/* nwfpe/compat/linux/thread_info.h - the parts of the kernel's thread state
 * that NWFPE uses, for riscose.
 *
 * riscose runs one task, so there is one FPA, in riscose_thread_info. */

#ifndef RISCOSE_NWFPE_THREAD_INFO_H
#define RISCOSE_NWFPE_THREAD_INFO_H

#include <stdint.h>
#include "linux/compiler.h"

typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* Big enough for NWFPE's FPA11 (116 bytes), as the kernel's is. */
union fp_state {
  unsigned int soft[35];
};

struct thread_info {
  union fp_state fpstate;
};

extern struct thread_info riscose_thread_info;

#define current_thread_info() (&riscose_thread_info)

#endif
