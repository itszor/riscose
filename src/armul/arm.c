/* arm.c
**
** (c) Matthew Bloch 2000
**
** See http://riscose.sourceforge.net/ for terms of distribution, and to
** pick up a later version of the software.
**
**   The ARM emulation as riscose would like to see it; this one assumes
**   that you're linking to the ARMulator.
*/

#include <config.h>

#include "riscostypes.h"
#include "armdefs.h"
#include "arminit.h"
#include "armsupp.h"
#include "armemu.h"
#include "disarm.h"
#include "mem.h"
#include "map.h"
#include "swi.h"
#include "arm.h"
#include <rom/rom.h>
#include <assert.h>

#include "monty/monty.h"
#include "rsp-server.h"

static ARMul_State *arm;

/*static
void
dumpregs(void)
{
  WORD c;
  
  for (c=0; c!=16; c++)
    printf("r%d = %08x\n", c, arm_get_reg(c));
}*/

#include <stdio.h>
static void arm_unsupported_instruction(ARMul_State *state,unsigned type,ARMword instr,ARMword value)
{
    char *regs[16] = {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                      "r8", "r9", "sl", "fp", "ip", "sp", "lr", "pc"};
    sDisOptions opts = {disopt_CommaSpace, regs};
    pInstruction p = instr_disassemble(instr, arm_get_reg(15), &opts);

    fprintf(stderr,
        "Unsupported ARM instruction %08x (%s) found at %08x (%08x), aborting\n",
        instr, p->text, arm_get_reg(15), CPSR);
    abort();
}

static void cp15_mrc(ARMul_State *state, unsigned type, ARMword instr, ARMword value)
{
  ARMword op = (instr & 0x00e00000) >> 21;
  ARMword crn = (instr & 0x000f0000) >> 16;
  ARMword rd = (instr & 0x0000f000) >> 12;
  ARMword info = (instr & 0x000000e0) >> 5;
  ARMword crm = instr & 0xf;

  if (op != 0 || crn != 0 || crm != 0 || info != 0)
    return arm_unsupported_instruction(state, type, instr, value);

  /* Claim to be an ARM720 */
  arm_set_reg(rd, 0x41807200);
}

static WORD arm_run_depth;
void
arm_init(void)
{
  int dummyInstr;
  
  ARMul_EmulateInit();

  arm = ARMul_NewState();
  for (dummyInstr = 0; dummyInstr < 16; dummyInstr++) {
      arm->LDC[dummyInstr] = (ARMul_LDCs *)arm_unsupported_instruction;
      arm->STC[dummyInstr] = (ARMul_STCs *)arm_unsupported_instruction;
      arm->MRC[dummyInstr] = (ARMul_MRCs *)arm_unsupported_instruction;
      arm->MCR[dummyInstr] = (ARMul_MCRs *)arm_unsupported_instruction;
      arm->CDP[dummyInstr] = (ARMul_CDPs *)arm_unsupported_instruction;
  }
  arm->MRC[15] = (ARMul_MRCs *)cp15_mrc;
  ARMul_SelectProcessor(arm, ARM3);
  ARMul_Reset(arm);

  /* Expect to be in SVC mode */
  assert(arm->Mode == 3 && (arm_get_r15_all() & 3) == 3);
  /* Set up SVC mode stack */
  arm_set_reg(13, MMAP_SVCSTACK_BASE+MMAP_SVCSTACK_SIZE);

  /* Drop into USR mode */
  ARMul_SetR15(arm, arm_get_r15_all() & ~3);
  assert(arm->Mode == 0 && (arm_get_r15_all() & 3) == 0);

  arm_run_depth=0;

  arm->CallDebug = 1;

  rsp_init();
}

void
arm_set_debug(int state) {
  arm->CallDebug = state;
}

void
arm_set_pc(WORD addr)
{
  ARMul_SetPC(arm, addr);
  /* ARMul_R15Altered(arm); --- only when setting the flags directly! */
}

void
arm_run_routine(WORD addr)
{
  WORD old_pc = ARM_R15, old_r14 = ARM_R14;
  
  if (addr == 0) {
    fprintf(stderr, "Branch through zero, aborting!\n");
    exit(1);
  }
  
  ARM_SET_R14(WORD_MAGIC_RETURN_ADDRESS);
  ARMul_SetPC(arm, addr);
  arm_run_depth++;
  ARMul_DoProg(arm);
  arm_run_depth--;
  ARM_SET_R14(old_r14);
  arm_set_reg(15, (old_pc+4) | (arm_get_r15_all() & 0xf0000000));
  if (arm_run_depth)
    arm->Emulate = RUN;
}

void
arm_return(void)
{
  arm->Emulate = STOP;
}

void
arm_enter_svc(void)
{
  ARMul_State *state = arm;

  arm->Spsr[SVCBANK] = CPSR;
  arm_set_reg(15, arm_get_r15_all() | 0x3);
  assert(arm_get_reg(13) > MMAP_SVCSTACK_BASE);
}

void
arm_leave_svc(void)
{
  arm_set_reg(15, arm_get_r15_all() & ~3);
  assert(arm_get_reg(13) > MMAP_USRSTACK_BASE);
}

WORD
arm_get_reg(WORD num)
{
  return arm->Reg[num];
}

WORD
arm_get_r15_all(void)
{
  ARMul_State *state = arm;

  return ECC | ER15INT | R15PC | EMODE;
}

void
arm_set_reg(WORD num, WORD val)
{
  assert(num >= 0 && num <= 15);
  if (num == 15) {
    ARMul_SetR15(arm, val);
  }
  else {
    arm->Reg[num] = val;
  }
}

void
arm_clear_v(void)
{
  arm->VFlag = 0;
}

void
arm_set_v(void)
{
  arm->VFlag = 1;
}

void
arm_clear_c(void)
{
  arm->CFlag = 0;
}

void
arm_set_c(void)
{
  arm->CFlag = 1;
}

/******************* Glue code for ARMulator follows *********************/

unsigned inline
ARMul_OSHandleSWI(ARMul_State *state, ARMword number)
{
  // Magic breakpoint SWI which gdb can insert by explicit memory writes.
  if (number == 0x9f0001) {
      error("We have hit SWI 0x9f0001, which is injected by gdb as a breakpoint. We cannot currently recover from this, the instruction pipeline makes it complicated.");
      abort();
  }
  else {
      rsp_check_swi(number);
      swi_trap(number);
  }
  return 1; /* tells ARMulator not to try OS SWI trap */
}

unsigned
ARMul_OSException(ARMul_State *state, ARMword vector, ARMword pc)
{
  return 0;
}

ARMword
ARMul_Debug(ARMul_State *state, ARMword pc, ARMword instr)
{
  rsp_check_instruction(pc);
  return instr;
}

unsigned
ARMul_MemoryInit(ARMul_State *state, unsigned long initmemsize)
{
  return 0;
}

void
ARMul_MemoryExit(ARMul_State *state)
{
  mem_final();
}

inline ARMword
ARMul_ReLoadInstr(ARMul_State *state,ARMword address,ARMword isize)
{
  if (isize == 4 || (address & 3) == 0)
    return MEM_READ_WORD(address);

  return ((WORD)MEM_READ_BYTE(address+3) << 24) |
         ((WORD)MEM_READ_BYTE(address+2) << 16) |
         ((WORD)MEM_READ_BYTE(address+1) << 8) |
         MEM_READ_BYTE(address);
}

inline ARMword
ARMul_LoadInstrS(ARMul_State *state,ARMword address,ARMword isize)
{
  return ARMul_ReLoadInstr(state, address, isize);
}

inline ARMword
ARMul_LoadInstrN(ARMul_State *state,ARMword address,ARMword isize)
{
  return ARMul_ReLoadInstr(state, address, isize);
}

/* Word accesses ignore the bottom two bits of the address, as the ARM's
 * memory interface does: an unaligned LDR reads the aligned word (and
 * ARMul_Align rotates it), an unaligned STR writes the aligned word. */
inline ARMword ARMul_LoadWordS(ARMul_State *state,ARMword address)
{
  rsp_check_memory_read(address);
  return MEM_READ_WORD(address & ~3);
}
inline ARMword ARMul_LoadWordN(ARMul_State *state,ARMword address)
{
  rsp_check_memory_read(address);
  return MEM_READ_WORD(address & ~3);
}
inline ARMword ARMul_LoadHalfWord(ARMul_State *state,ARMword address)
{
  ARMword offset = (address & 2) << 3;
  rsp_check_memory_read(address);
  return (MEM_READ_WORD(address & ~3) >> offset) & 0xffff;
}
inline ARMword ARMul_LoadByte(ARMul_State *state,ARMword address)
{
  rsp_check_memory_read(address);
  return MEM_READ_BYTE(address);
}
inline void ARMul_StoreWordS(ARMul_State *state,ARMword address, ARMword data)
{
  rsp_check_memory_write(address);
  MEM_WRITE_WORD(address & ~3, data);
}
inline void ARMul_StoreWordN(ARMul_State *state, ARMword address, ARMword data)
{
  rsp_check_memory_write(address);
  MEM_WRITE_WORD(address & ~3, data);
}
inline void ARMul_StoreHalfWord(ARMul_State *state,ARMword address, ARMword data)
{
  ARMword temp, offset;
  rsp_check_memory_write(address);
  temp = ARMul_ReadWord(state,address);
  offset = (address & 2) << 3;
  MEM_WRITE_WORD(address & ~3, (temp & ~(0xffffL << offset)) | ((data & 0xffff) << offset));

}
inline void ARMul_StoreByte(ARMul_State *state, ARMword address, ARMword data)
{
  rsp_check_memory_write(address);
  MEM_WRITE_BYTE(address, data);
}
ARMword ARMul_SwapWord(ARMul_State *state, ARMword address, ARMword data)
{
  rsp_check_memory_read(address);
  rsp_check_memory_write(address);
  WORD t = MEM_READ_WORD(address & ~3);
  MEM_WRITE_WORD(address & ~3, data);
  
  return t;
}
inline ARMword ARMul_SwapByte(ARMul_State *state, ARMword address, ARMword data)
{
  rsp_check_memory_read(address);
  rsp_check_memory_write(address);
  BYTE t = MEM_READ_BYTE(address);
  MEM_WRITE_BYTE(address, data);
  
  return t;
}
inline void ARMul_Icycles(ARMul_State *state, unsigned number, ARMword address)
{
  state->NumIcycles += number ;
  ARMul_CLEARABORT ;
}
inline void ARMul_Ccycles(ARMul_State *state, unsigned number, ARMword address)
{
  state->NumCcycles += number ;
  ARMul_CLEARABORT ;
}

// Used for reading instructions
inline ARMword ARMul_ReadWord(ARMul_State *state, ARMword address)
{
  return MEM_READ_WORD(address & ~3);
}
inline ARMword ARMul_ReadByte(ARMul_State *state, ARMword address)
{
  return MEM_READ_BYTE(address);
}
inline void ARMul_WriteWord(ARMul_State *state, ARMword address, ARMword data)
{
  rsp_check_memory_write(address);
  MEM_WRITE_WORD(address, data);
}
inline void ARMul_WriteByte(ARMul_State *state, ARMword address, ARMword data)
{
  rsp_check_memory_write(address);
  MEM_WRITE_BYTE(address, data);
}
