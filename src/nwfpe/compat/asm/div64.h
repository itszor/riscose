/* nwfpe/compat/asm/div64.h - the kernel's do_div(), for riscose: divides
 * the 64-bit n by the 32-bit base in place and returns the remainder. */

#ifndef RISCOSE_NWFPE_DIV64_H
#define RISCOSE_NWFPE_DIV64_H

#include <stdint.h>

#define do_div(n, base) ({ \
  uint32_t __base = (base); \
  uint32_t __rem = (uint32_t) ((n) % __base); \
  (n) /= __base; \
  __rem; \
})

#endif
