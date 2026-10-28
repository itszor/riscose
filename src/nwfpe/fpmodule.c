/*
    NetWinder Floating Point Emulator
    (c) Rebel.com, 1998-1999
    (c) Philip Blundell, 1998-1999

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
 * riscose's version of NWFPE's module glue, in place of the Linux
 * kernel's: one FPA for the one task riscose runs, set up by
 * riscose_fpe_init(), and float_raise() handing a trapped exception to
 * riscose (src/armul/arm.c) rather than sending SIGFPE.
 */

#include "fpa11.h"

#include "softfloat.h"
#include "fpopcode.h"
#include "fpmodule.h"
#include "fpa11.inl"

struct thread_info riscose_thread_info;

extern void riscose_fpe_trap(unsigned int flags);

void riscose_fpe_init(void)
{
	/* The FPA11 must fit in the space the thread state gives it. */
	_Static_assert(sizeof(FPA11) <= sizeof(union fp_state),
		       "FPA11 doesn't fit in fp_state");
	_Static_assert(sizeof(FPREG) == 12, "FPREG must be 12 bytes");

	nwfpe_init_fpa(&riscose_thread_info.fpstate);
}

/*
This function is called by the SoftFloat routines to raise a floating
point exception.  We check the trap enable byte in the FPSR, and raise
a SIGFPE exception if necessary.  If not the relevant bits in the
cumulative exceptions flag byte are set and we return.
*/

void float_raise(signed char flags)
{
	register unsigned int fpsr, cumulativeTraps;

	/* Read fpsr and initialize the cumulativeTraps.  */
	fpsr = readFPSR();
	cumulativeTraps = 0;

	/* For each type of exception, the cumulative trap exception bit is only
	   set if the corresponding trap enable bit is not set.  */
	if ((!(fpsr & BIT_IXE)) && (flags & BIT_IXC))
		cumulativeTraps |= BIT_IXC;
	if ((!(fpsr & BIT_UFE)) && (flags & BIT_UFC))
		cumulativeTraps |= BIT_UFC;
	if ((!(fpsr & BIT_OFE)) && (flags & BIT_OFC))
		cumulativeTraps |= BIT_OFC;
	if ((!(fpsr & BIT_DZE)) && (flags & BIT_DZC))
		cumulativeTraps |= BIT_DZC;
	if ((!(fpsr & BIT_IOE)) && (flags & BIT_IOC))
		cumulativeTraps |= BIT_IOC;

	/* Set the cumulative exceptions flags.  */
	if (cumulativeTraps)
		writeFPSR(fpsr | cumulativeTraps);

	/* Raise an exception if necessary.  */
	if (fpsr & (flags << 16))
		riscose_fpe_trap(flags & (fpsr >> 16));
}
