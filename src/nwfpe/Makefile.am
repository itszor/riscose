# NWFPE, the NetWinder Floating Point Emulator, from the Linux kernel's
# arch/arm/nwfpe: the FPA (coprocessors 1 and 2) for the ARMulator.
# See README.

noinst_LIBRARIES = libnwfpe.a

libnwfpe_a_SOURCES = double_cpdo.c extended_cpdo.c fpa11.c fpa11_cpdo.c \
    fpa11_cpdt.c fpa11_cprt.c fpmodule.c fpopcode.c single_cpdo.c \
    softfloat.c \
    ARM-gcc.h fpa11.h fpa11.inl fpmodule.h fpmodule.inl fpopcode.h fpsr.h \
    milieu.h softfloat.h softfloat-macros softfloat-specialize \
    compat/asm/div64.h compat/linux/compiler.h compat/linux/string.h \
    compat/linux/thread_info.h compat/linux/uaccess.h

# Extended precision, as the FPA has.
AM_CPPFLAGS = -I$(srcdir)/compat -DCONFIG_FPE_NWFPE_XP

EXTRA_DIST = ChangeLog README
