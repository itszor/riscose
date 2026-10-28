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

#include <fenv.h>
#include <math.h>
#include <string.h>

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
 * The FPA's transcendental operations - LOG, LGN, EXP, SIN, COS, TAN, ASN,
 * ACS, ATN, POW, RPW and POL - which NWFPE leaves out, as the FPA10 left
 * them to Acorn's support code. They're computed in double precision with
 * the host's libm and the result stored at the instruction's precision;
 * the host's exceptions become the FPA's (inexact aside, which these
 * always are). Returns 1 if the instruction was one of them.
 */

static double supportOperand(unsigned int Fm, int constant)
{
	FPA11 *fpa11 = GET_FPA11();
	struct roundingData roundData = { float_round_nearest_even, 80, 0 };
	float64 f;
	double d;

	if (constant)
		f = getDoubleConstant(Fm);
	else
		switch (fpa11->fType[Fm]) {
		case typeSingle:
			f = float32_to_float64(fpa11->fpreg[Fm].fSingle);
			break;
		case typeDouble:
			f = fpa11->fpreg[Fm].fDouble;
			break;
		case typeExtended:
			f = floatx80_to_float64(&roundData, fpa11->fpreg[Fm].fExtended);
			break;
		default:
			f = 0;
		}

	memcpy(&d, &f, sizeof d);
	return d;
}

unsigned int riscose_fpe_support(const unsigned int opcode)
{
	FPA11 *fpa11 = GET_FPA11();
	struct roundingData roundData;
	unsigned int Fd = getFd(opcode), nDest;
	double m, n = 0, d;
	float64 f;
	int raised;
	signed char flags = 0;

	/* A data operation (CPDO) for coprocessor 1. */
	if ((opcode & 0x0f000f10) != 0x0e000100)
		return 0;

	nDest = getDestinationSize(opcode);
	if (typeNone == nDest)
		return 0;

	m = supportOperand(getFm(opcode), CONSTANT_FM(opcode));
	if (!MONADIC_INSTRUCTION(opcode))
		n = supportOperand(getFn(opcode), 0);

	feclearexcept(FE_ALL_EXCEPT);
	switch (opcode & MASK_ARITHMETIC_OPCODE) {
	case LOG_CODE:	d = log10(m);		break;
	case LGN_CODE:	d = log(m);		break;
	case EXP_CODE:	d = exp(m);		break;
	case SIN_CODE:	d = sin(m);		break;
	case COS_CODE:	d = cos(m);		break;
	case TAN_CODE:	d = tan(m);		break;
	case ASN_CODE:	d = asin(m);		break;
	case ACS_CODE:	d = acos(m);		break;
	case ATN_CODE:	d = atan(m);		break;
	case POW_CODE:	d = pow(n, m);		break;
	case RPW_CODE:	d = pow(m, n);		break;
	case POL_CODE:	d = atan2(n, m);	break;
	default:
		return 0;
	}
	raised = fetestexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW |
			      FE_UNDERFLOW);

	roundData.mode = SetRoundingMode(opcode);
	roundData.precision = SetRoundingPrecision(opcode);
	roundData.exception = 0;

	memcpy(&f, &d, sizeof f);
	switch (nDest) {
	case typeSingle:
		fpa11->fpreg[Fd].fSingle = float64_to_float32(&roundData, f);
		break;
	case typeDouble:
		fpa11->fpreg[Fd].fDouble = f;
		break;
	case typeExtended:
		fpa11->fpreg[Fd].fExtended = float64_to_floatx80(f);
		break;
	}
	fpa11->fType[Fd] = nDest;

	if (raised & FE_INVALID)
		flags |= float_flag_invalid;
	if (raised & FE_DIVBYZERO)
		flags |= float_flag_divbyzero;
	if (raised & FE_OVERFLOW)
		flags |= float_flag_overflow;
	if (raised & FE_UNDERFLOW)
		flags |= float_flag_underflow;
	flags |= roundData.exception & ~float_flag_inexact;
	if (flags)
		float_raise(flags);

	return 1;
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
