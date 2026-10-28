/* nwfpe/compat/linux/uaccess.h - NWFPE's user memory access, for riscose.
 *
 * NWFPE holds ARM addresses in host pointers (unsigned int *); they are
 * turned back into ARM addresses and read or written in the emulated
 * memory. */

#ifndef RISCOSE_NWFPE_UACCESS_H
#define RISCOSE_NWFPE_UACCESS_H

#define __user

extern unsigned int riscose_fpe_read_word(unsigned long addr);
extern void riscose_fpe_write_word(unsigned long addr, unsigned int val);

#define get_user(x, p) \
  ((x) = riscose_fpe_read_word((unsigned long) (p)))
#define put_user(x, p) \
  riscose_fpe_write_word((unsigned long) (p), (x))

#endif
