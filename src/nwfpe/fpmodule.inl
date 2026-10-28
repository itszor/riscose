/*
    NetWinder Floating Point Emulator
    (c) Rebel.COM, 1998,1999

    Direct questions, comments to Scott Bambrough <scottb@netwinder.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
 * riscose's version of NWFPE's register access, in place of the Linux
 * kernel's (which reads the trapped process's pt_regs). The emulated
 * ARM's registers are read and written through src/armul/arm.c; reading
 * the PC gives the address of the instruction being emulated, as the
 * kernel's does, so that LDF/STF with Rn = PC can add 8 to it.
 */

extern unsigned long riscose_fpe_read_register(unsigned int nReg);
extern void riscose_fpe_write_register(unsigned int nReg, unsigned long val);
extern void riscose_fpe_write_flags(unsigned long val);

static inline unsigned long readRegister(const unsigned int nReg)
{
	return riscose_fpe_read_register(nReg);
}

static inline void
writeRegister(const unsigned int nReg, const unsigned long val)
{
	riscose_fpe_write_register(nReg, val);
}

/* The N, Z, C and V flags a compare sets (CC_MASK's bits). */
static inline void writeConditionCodes(const unsigned long val)
{
	riscose_fpe_write_flags(val & CC_MASK);
}
