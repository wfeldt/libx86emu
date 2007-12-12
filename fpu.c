/****************************************************************************
*
*						Realmode X86 Emulator Library
*
*            	Copyright (C) 1996-1999 SciTech Software, Inc.
* 				     Copyright (C) David Mosberger-Tang
* 					   Copyright (C) 1999 Egbert Eich
*
*  ========================================================================
*
*  Permission to use, copy, modify, distribute, and sell this software and
*  its documentation for any purpose is hereby granted without fee,
*  provided that the above copyright notice appear in all copies and that
*  both that copyright notice and this permission notice appear in
*  supporting documentation, and that the name of the authors not be used
*  in advertising or publicity pertaining to distribution of the software
*  without specific, written prior permission.  The authors makes no
*  representations about the suitability of this software for any purpose.
*  It is provided "as is" without express or implied warranty.
*
*  THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
*  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
*  EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
*  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
*  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
*  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
*  PERFORMANCE OF THIS SOFTWARE.
*
*  ========================================================================
*
* Language:		ANSI C
* Environment:	Any
* Developer:    Kendall Bennett
*
* Description:  This file contains the code to implement the decoding and
*               emulation of the FPU instructions.
*
****************************************************************************/

#include "include/x86emui.h"

/*----------------------------- Implementation ----------------------------*/

/* opcode=0xd8 */
void x86emuOp_esc_coprocess_d8(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("esc d8\n");
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

#ifdef DEBUG

static char *x86emu_fpu_op_d9_tab[] = {
    "fld dword ptr ", "esc_d9 ", "fst dword ptr ", "fstp dword ptr ",
    "fldenv ", "fldcw ", "fstenv ", "fstcw ",

    "fld dword ptr ", "esc_d9 ", "fst dword ptr ", "fstp dword ptr ",
    "fldenv ", "fldcw ", "fstenv ", "fstcw ",

    "fld dword ptr ", "esc_d9 ", "fst dword ptr ", "fstp dword ptr ",
    "fldenv ", "fldcw ", "fstenv ", "fstcw ",
};

static char *x86emu_fpu_op_d9_tab1[] = {
    "fld ", "fld ", "fld ", "fld ",
    "fld ", "fld ", "fld ", "fld ",

    "fxch ", "fxch ", "fxch ", "fxch ",
    "fxch ", "fxch ", "fxch ", "fxch ",

    "fnop", "esc_d9", "esc_d9", "esc_d9",
    "esc_d9", "esc_d9", "esc_d9", "esc_d9",

    "fstp ", "fstp ", "fstp ", "fstp ",
    "fstp ", "fstp ", "fstp ", "fstp ",

    "fchs", "fabs", "esc_d9", "esc_d9",
    "ftst", "fxam", "esc_d9", "esc_d9",

    "fld1", "fldl2t", "fldl2e", "fldpi",
    "fldlg2", "fldln2", "fldz", "esc_d9",

    "f2xm1", "fyl2x", "fptan", "fpatan",
    "fxtract", "esc_d9", "fdecstp", "fincstp",

    "fprem", "fyl2xp1", "fsqrt", "esc_d9",
    "frndint", "fscale", "esc_d9", "esc_d9",
};

#endif /* DEBUG */

/* opcode=0xd9 */
void x86emuOp_esc_coprocess_d9(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset = 0;
    u8 stkelem = 0;

    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (mod != 3) {
        DECODE_PRINTINSTR32(x86emu_fpu_op_d9_tab, mod, rh, rl);
    } else {
        DECODE_PRINTF(x86emu_fpu_op_d9_tab1[(rh << 3) + rl]);
    }
#endif
    switch (mod) {
      case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 3:                   /* register to register */
		stkelem = (u8)rl;
		if (rh < 4) {
				DECODE_PRINTF2("st(%d)\n", stkelem);
		} else {
				DECODE_PRINTF("\n");
		}
        break;
    }
#ifdef X86EMU_FPU_PRESENT
    /* execute */
    switch (mod) {
      case 3:
        switch (rh) {
          case 0:
            x86emu_fpu_R_fld(X86EMU_FPU_STKTOP, stkelem);
            break;
          case 1:
            x86emu_fpu_R_fxch(X86EMU_FPU_STKTOP, stkelem);
            break;
          case 2:
            switch (rl) {
              case 0:
                x86emu_fpu_R_nop();
                break;
              default:
                x86emu_fpu_illegal();
                break;
            }
          case 3:
            x86emu_fpu_R_fstp(X86EMU_FPU_STKTOP, stkelem);
            break;
          case 4:
            switch (rl) {
            case 0:
                x86emu_fpu_R_fchs(X86EMU_FPU_STKTOP);
                break;
            case 1:
                x86emu_fpu_R_fabs(X86EMU_FPU_STKTOP);
                break;
            case 4:
                x86emu_fpu_R_ftst(X86EMU_FPU_STKTOP);
                break;
            case 5:
                x86emu_fpu_R_fxam(X86EMU_FPU_STKTOP);
                break;
            default:
                /* 2,3,6,7 */
                x86emu_fpu_illegal();
                break;
            }
            break;

          case 5:
            switch (rl) {
              case 0:
                x86emu_fpu_R_fld1(X86EMU_FPU_STKTOP);
                break;
              case 1:
                x86emu_fpu_R_fldl2t(X86EMU_FPU_STKTOP);
                break;
              case 2:
                x86emu_fpu_R_fldl2e(X86EMU_FPU_STKTOP);
                break;
              case 3:
                x86emu_fpu_R_fldpi(X86EMU_FPU_STKTOP);
                break;
              case 4:
                x86emu_fpu_R_fldlg2(X86EMU_FPU_STKTOP);
                break;
              case 5:
                x86emu_fpu_R_fldln2(X86EMU_FPU_STKTOP);
                break;
              case 6:
                x86emu_fpu_R_fldz(X86EMU_FPU_STKTOP);
                break;
              default:
                /* 7 */
                x86emu_fpu_illegal();
                break;
            }
            break;

          case 6:
            switch (rl) {
              case 0:
                x86emu_fpu_R_f2xm1(X86EMU_FPU_STKTOP);
                break;
              case 1:
                x86emu_fpu_R_fyl2x(X86EMU_FPU_STKTOP);
                break;
              case 2:
                x86emu_fpu_R_fptan(X86EMU_FPU_STKTOP);
                break;
              case 3:
                x86emu_fpu_R_fpatan(X86EMU_FPU_STKTOP);
                break;
              case 4:
                x86emu_fpu_R_fxtract(X86EMU_FPU_STKTOP);
                break;
              case 5:
                x86emu_fpu_illegal();
                break;
              case 6:
                x86emu_fpu_R_decstp();
                break;
              case 7:
                x86emu_fpu_R_incstp();
                break;
            }
            break;

          case 7:
            switch (rl) {
              case 0:
                x86emu_fpu_R_fprem(X86EMU_FPU_STKTOP);
                break;
              case 1:
                x86emu_fpu_R_fyl2xp1(X86EMU_FPU_STKTOP);
                break;
              case 2:
                x86emu_fpu_R_fsqrt(X86EMU_FPU_STKTOP);
                break;
              case 3:
                x86emu_fpu_illegal();
                break;
              case 4:
                x86emu_fpu_R_frndint(X86EMU_FPU_STKTOP);
                break;
              case 5:
                x86emu_fpu_R_fscale(X86EMU_FPU_STKTOP);
                break;
              case 6:
              case 7:
              default:
                x86emu_fpu_illegal();
                break;
            }
            break;

          default:
            switch (rh) {
              case 0:
                x86emu_fpu_M_fld(X86EMU_FPU_FLOAT, destoffset);
                break;
              case 1:
                x86emu_fpu_illegal();
                break;
              case 2:
                x86emu_fpu_M_fst(X86EMU_FPU_FLOAT, destoffset);
                break;
              case 3:
                x86emu_fpu_M_fstp(X86EMU_FPU_FLOAT, destoffset);
                break;
              case 4:
                x86emu_fpu_M_fldenv(X86EMU_FPU_WORD, destoffset);
                break;
              case 5:
                x86emu_fpu_M_fldcw(X86EMU_FPU_WORD, destoffset);
                break;
              case 6:
                x86emu_fpu_M_fstenv(X86EMU_FPU_WORD, destoffset);
                break;
              case 7:
                x86emu_fpu_M_fstcw(X86EMU_FPU_WORD, destoffset);
                break;
            }
        }
    }
#else
    (void)destoffset;
    (void)stkelem;
#endif /* X86EMU_FPU_PRESENT */
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

#ifdef DEBUG

char *x86emu_fpu_op_da_tab[] = {
    "fiadd dword ptr ", "fimul dword ptr ", "ficom dword ptr ",
    "ficomp dword ptr ",
    "fisub dword ptr ", "fisubr dword ptr ", "fidiv dword ptr ",
    "fidivr dword ptr ",

    "fiadd dword ptr ", "fimul dword ptr ", "ficom dword ptr ",
    "ficomp dword ptr ",
    "fisub dword ptr ", "fisubr dword ptr ", "fidiv dword ptr ",
    "fidivr dword ptr ",
    
    "fiadd dword ptr ", "fimul dword ptr ", "ficom dword ptr ",
    "ficomp dword ptr ",
    "fisub dword ptr ", "fisubr dword ptr ", "fidiv dword ptr ",
    "fidivr dword ptr ",

    "esc_da ", "esc_da ", "esc_da ", "esc_da ",
    "esc_da     ", "esc_da ", "esc_da   ", "esc_da ",
};

#endif /* DEBUG */

/* opcode=0xda */
void x86emuOp_esc_coprocess_da(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset = 0;
    u8 stkelem = 0;

    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTINSTR32(x86emu_fpu_op_da_tab, mod, rh, rl);
    switch (mod) {
      case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 3:           /* register to register */
		stkelem = (u8)rl;
        DECODE_PRINTF2(" st(%d),st\n", stkelem);
        break;
    }
#ifdef X86EMU_FPU_PRESENT
    switch (mod) {
      case 3:
        x86emu_fpu_illegal();
        break;
      default:
        switch (rh) {
          case 0:
            x86emu_fpu_M_iadd(X86EMU_FPU_SHORT, destoffset);
            break;
          case 1:
            x86emu_fpu_M_imul(X86EMU_FPU_SHORT, destoffset);
            break;
          case 2:
            x86emu_fpu_M_icom(X86EMU_FPU_SHORT, destoffset);
            break;
          case 3:
            x86emu_fpu_M_icomp(X86EMU_FPU_SHORT, destoffset);
            break;
          case 4:
            x86emu_fpu_M_isub(X86EMU_FPU_SHORT, destoffset);
            break;
          case 5:
            x86emu_fpu_M_isubr(X86EMU_FPU_SHORT, destoffset);
            break;
          case 6:
            x86emu_fpu_M_idiv(X86EMU_FPU_SHORT, destoffset);
            break;
          case 7:
            x86emu_fpu_M_idivr(X86EMU_FPU_SHORT, destoffset);
            break;
        }
    }
#else
    (void)destoffset;
    (void)stkelem;
#endif
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

#ifdef DEBUG

char *x86emu_fpu_op_db_tab[] = {
    "fild dword ptr ", "esc_db 19", "fist dword ptr ", "fistp dword ptr ",
    "esc_db 1c", "fld tbyte ptr ", "esc_db 1e", "fstp tbyte ptr ",

    "fild dword ptr ", "esc_db 19", "fist dword ptr ", "fistp dword ptr ",
    "esc_db 1c", "fld tbyte ptr ", "esc_db 1e", "fstp tbyte ptr ",

    "fild dword ptr ", "esc_db 19", "fist dword ptr ", "fistp dword ptr ",
    "esc_db 1c", "fld tbyte ptr ", "esc_db 1e", "fstp tbyte ptr ",
};

#endif /* DEBUG */

/* opcode=0xdb */
void x86emuOp_esc_coprocess_db(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset = 0;

    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (mod != 3) {
        DECODE_PRINTINSTR32(x86emu_fpu_op_db_tab, mod, rh, rl);
    } else if (rh == 4) {       /* === 11 10 0 nnn */
        switch (rl) {
          case 0:
            DECODE_PRINTF("feni\n");
            break;
          case 1:
            DECODE_PRINTF("fdisi\n");
            break;
          case 2:
            DECODE_PRINTF("fclex\n");
            break;
          case 3:
            DECODE_PRINTF("finit\n");
            break;
        }
    } else {
        DECODE_PRINTF2("esc_db %0x\n", (mod << 6) + (rh << 3) + (rl));
    }
#endif /* DEBUG */
    switch (mod) {
      case 0:
        destoffset = decode_rm00_address(rl);
        break;
      case 1:
        destoffset = decode_rm01_address(rl);
        break;
      case 2:
        destoffset = decode_rm10_address(rl);
        break;
      case 3:                   /* register to register */
        break;
    }
#ifdef X86EMU_FPU_PRESENT
    /* execute */
    switch (mod) {
      case 3:
        switch (rh) {
          case 4:
            switch (rl) {
              case 0:
                x86emu_fpu_R_feni();
                break;
              case 1:
                x86emu_fpu_R_fdisi();
                break;
              case 2:
                x86emu_fpu_R_fclex();
                break;
              case 3:
                x86emu_fpu_R_finit();
                break;
              default:
                x86emu_fpu_illegal();
                break;
            }
            break;
          default:
            x86emu_fpu_illegal();
            break;
        }
        break;
      default:
        switch (rh) {
          case 0:
            x86emu_fpu_M_fild(X86EMU_FPU_SHORT, destoffset);
            break;
          case 1:
            x86emu_fpu_illegal();
            break;
          case 2:
            x86emu_fpu_M_fist(X86EMU_FPU_SHORT, destoffset);
            break;
          case 3:
            x86emu_fpu_M_fistp(X86EMU_FPU_SHORT, destoffset);
            break;
          case 4:
            x86emu_fpu_illegal();
            break;
          case 5:
            x86emu_fpu_M_fld(X86EMU_FPU_LDBL, destoffset);
            break;
                      case 6:
            x86emu_fpu_illegal();
            break;
          case 7:
            x86emu_fpu_M_fstp(X86EMU_FPU_LDBL, destoffset);
            break;
        }
    }
#else
    (void)destoffset;
#endif
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

#ifdef DEBUG
char *x86emu_fpu_op_dc_tab[] = {
    "fadd qword ptr ", "fmul qword ptr ", "fcom qword ptr ",
    "fcomp qword ptr ",
    "fsub qword ptr ", "fsubr qword ptr ", "fdiv qword ptr ",
    "fdivr qword ptr ",

    "fadd qword ptr ", "fmul qword ptr ", "fcom qword ptr ",
    "fcomp qword ptr ",
    "fsub qword ptr ", "fsubr qword ptr ", "fdiv qword ptr ",
    "fdivr qword ptr ",

    "fadd qword ptr ", "fmul qword ptr ", "fcom qword ptr ",
    "fcomp qword ptr ",
    "fsub qword ptr ", "fsubr qword ptr ", "fdiv qword ptr ",
    "fdivr qword ptr ",

    "fadd ", "fmul ", "fcom ", "fcomp ",
    "fsubr ", "fsub ", "fdivr ", "fdiv ",
};
#endif /* DEBUG */

/* opcode=0xdc */
void x86emuOp_esc_coprocess_dc(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset = 0;
    u8 stkelem = 0;

    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTINSTR32(x86emu_fpu_op_dc_tab, mod, rh, rl);
    switch (mod) {
      case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 3:                   /* register to register */
		stkelem = (u8)rl;
        DECODE_PRINTF2(" st(%d),st\n", stkelem);
        break;
    }
#ifdef X86EMU_FPU_PRESENT
    /* execute */
    switch (mod) {
      case 3:
        switch (rh) {
          case 0:
            x86emu_fpu_R_fadd(stkelem, X86EMU_FPU_STKTOP);
            break;
          case 1:
            x86emu_fpu_R_fmul(stkelem, X86EMU_FPU_STKTOP);
            break;
          case 2:
            x86emu_fpu_R_fcom(stkelem, X86EMU_FPU_STKTOP);
            break;
          case 3:
            x86emu_fpu_R_fcomp(stkelem, X86EMU_FPU_STKTOP);
            break;
          case 4:
            x86emu_fpu_R_fsubr(stkelem, X86EMU_FPU_STKTOP);
            break;
          case 5:
            x86emu_fpu_R_fsub(stkelem, X86EMU_FPU_STKTOP);
            break;
          case 6:
            x86emu_fpu_R_fdivr(stkelem, X86EMU_FPU_STKTOP);
            break;
          case 7:
            x86emu_fpu_R_fdiv(stkelem, X86EMU_FPU_STKTOP);
            break;
        }
        break;
      default:
        switch (rh) {
          case 0:
            x86emu_fpu_M_fadd(X86EMU_FPU_DOUBLE, destoffset);
            break;
          case 1:
            x86emu_fpu_M_fmul(X86EMU_FPU_DOUBLE, destoffset);
            break;
          case 2:
            x86emu_fpu_M_fcom(X86EMU_FPU_DOUBLE, destoffset);
            break;
          case 3:
            x86emu_fpu_M_fcomp(X86EMU_FPU_DOUBLE, destoffset);
            break;
          case 4:
            x86emu_fpu_M_fsub(X86EMU_FPU_DOUBLE, destoffset);
            break;
          case 5:
            x86emu_fpu_M_fsubr(X86EMU_FPU_DOUBLE, destoffset);
            break;
          case 6:
            x86emu_fpu_M_fdiv(X86EMU_FPU_DOUBLE, destoffset);
            break;
          case 7:
            x86emu_fpu_M_fdivr(X86EMU_FPU_DOUBLE, destoffset);
            break;
        }
    }
#else
    (void)destoffset;
    (void)stkelem;
#endif
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

#ifdef DEBUG

static char *x86emu_fpu_op_dd_tab[] = {
    "fld qword ptr ", "esc_dd 29,", "fst qword ptr ", "fstp qword ptr ",
    "frstor ", "esc_dd 2d,", "fsave ", "fstsw ",

    "fld qword ptr ", "esc_dd 29,", "fst qword ptr ", "fstp qword ptr ",
    "frstor ", "esc_dd 2d,", "fsave ", "fstsw ",

    "fld qword ptr ", "esc_dd 29,", "fst qword ptr ", "fstp qword ptr ",
    "frstor ", "esc_dd 2d,", "fsave ", "fstsw ",

    "ffree ", "fxch ", "fst ", "fstp ",
    "esc_dd 2c,", "esc_dd 2d,", "esc_dd 2e,", "esc_dd 2f,",
};

#endif /* DEBUG */

/* opcode=0xdd */
void x86emuOp_esc_coprocess_dd(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset = 0;
    u8 stkelem = 0;

    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTINSTR32(x86emu_fpu_op_dd_tab, mod, rh, rl);
    switch (mod) {
      case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 3:                   /* register to register */
		stkelem = (u8)rl;
        DECODE_PRINTF2(" st(%d),st\n", stkelem);
        break;
    }
#ifdef X86EMU_FPU_PRESENT
    switch (mod) {
      case 3:
        switch (rh) {
          case 0:
            x86emu_fpu_R_ffree(stkelem);
            break;
          case 1:
            x86emu_fpu_R_fxch(stkelem);
            break;
          case 2:
            x86emu_fpu_R_fst(stkelem);  /* register version */
            break;
          case 3:
            x86emu_fpu_R_fstp(stkelem); /* register version */
            break;
          default:
            x86emu_fpu_illegal();
            break;
        }
        break;
      default:
        switch (rh) {
          case 0:
            x86emu_fpu_M_fld(X86EMU_FPU_DOUBLE, destoffset);
            break;
          case 1:
            x86emu_fpu_illegal();
            break;
          case 2:
            x86emu_fpu_M_fst(X86EMU_FPU_DOUBLE, destoffset);
            break;
          case 3:
            x86emu_fpu_M_fstp(X86EMU_FPU_DOUBLE, destoffset);
            break;
          case 4:
            x86emu_fpu_M_frstor(X86EMU_FPU_WORD, destoffset);
            break;
          case 5:
            x86emu_fpu_illegal();
            break;
          case 6:
            x86emu_fpu_M_fsave(X86EMU_FPU_WORD, destoffset);
            break;
          case 7:
            x86emu_fpu_M_fstsw(X86EMU_FPU_WORD, destoffset);
            break;
        }
    }
#else
    (void)destoffset;
    (void)stkelem;
#endif
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

#ifdef DEBUG

static char *x86emu_fpu_op_de_tab[] =
{
    "fiadd word ptr ", "fimul word ptr ", "ficom word ptr ",
    "ficomp word ptr ",
    "fisub word ptr ", "fisubr word ptr ", "fidiv word ptr ",
    "fidivr word ptr ",

    "fiadd word ptr ", "fimul word ptr ", "ficom word ptr ",
    "ficomp word ptr ",
    "fisub word ptr ", "fisubr word ptr ", "fidiv word ptr ",
    "fidivr word ptr ",

    "fiadd word ptr ", "fimul word ptr ", "ficom word ptr ",
    "ficomp word ptr ",
    "fisub word ptr ", "fisubr word ptr ", "fidiv word ptr ",
    "fidivr word ptr ",

    "faddp ", "fmulp ", "fcomp ", "fcompp ",
    "fsubrp ", "fsubp ", "fdivrp ", "fdivp ",
};

#endif /* DEBUG */

/* opcode=0xde */
void x86emuOp_esc_coprocess_de(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset = 0;
    u8 stkelem = 0;

    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTINSTR32(x86emu_fpu_op_de_tab, mod, rh, rl);
    switch (mod) {
      case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 3:                   /* register to register */
		stkelem = (u8)rl;
        DECODE_PRINTF2(" st(%d),st\n", stkelem);
        break;
    }
#ifdef X86EMU_FPU_PRESENT
    switch (mod) {
      case 3:
        switch (rh) {
          case 0:
            x86emu_fpu_R_faddp(stkelem, X86EMU_FPU_STKTOP);
            break;
          case 1:
            x86emu_fpu_R_fmulp(stkelem, X86EMU_FPU_STKTOP);
            break;
          case 2:
            x86emu_fpu_R_fcomp(stkelem, X86EMU_FPU_STKTOP);
            break;
          case 3:
            if (stkelem == 1)
              x86emu_fpu_R_fcompp(stkelem, X86EMU_FPU_STKTOP);
            else
              x86emu_fpu_illegal();
            break;
          case 4:
            x86emu_fpu_R_fsubrp(stkelem, X86EMU_FPU_STKTOP);
            break;
          case 5:
            x86emu_fpu_R_fsubp(stkelem, X86EMU_FPU_STKTOP);
            break;
          case 6:
            x86emu_fpu_R_fdivrp(stkelem, X86EMU_FPU_STKTOP);
            break;
          case 7:
            x86emu_fpu_R_fdivp(stkelem, X86EMU_FPU_STKTOP);
            break;
        }
        break;
      default:
        switch (rh) {
          case 0:
            x86emu_fpu_M_fiadd(X86EMU_FPU_WORD, destoffset);
            break;
          case 1:
            x86emu_fpu_M_fimul(X86EMU_FPU_WORD, destoffset);
            break;
          case 2:
            x86emu_fpu_M_ficom(X86EMU_FPU_WORD, destoffset);
            break;
          case 3:
            x86emu_fpu_M_ficomp(X86EMU_FPU_WORD, destoffset);
            break;
          case 4:
            x86emu_fpu_M_fisub(X86EMU_FPU_WORD, destoffset);
            break;
          case 5:
            x86emu_fpu_M_fisubr(X86EMU_FPU_WORD, destoffset);
            break;
          case 6:
            x86emu_fpu_M_fidiv(X86EMU_FPU_WORD, destoffset);
            break;
          case 7:
            x86emu_fpu_M_fidivr(X86EMU_FPU_WORD, destoffset);
            break;
        }
    }
#else
    (void)destoffset;
    (void)stkelem;
#endif
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

#ifdef DEBUG

static char *x86emu_fpu_op_df_tab[] = {
    /* mod == 00 */
    "fild word ptr ", "esc_df 39\n", "fist word ptr ", "fistp word ptr ",
    "fbld tbyte ptr ", "fild qword ptr ", "fbstp tbyte ptr ",
    "fistp qword ptr ",

    /* mod == 01 */
    "fild word ptr ", "esc_df 39 ", "fist word ptr ", "fistp word ptr ",
    "fbld tbyte ptr ", "fild qword ptr ", "fbstp tbyte ptr ",
    "fistp qword ptr ",

    /* mod == 10 */
    "fild word ptr ", "esc_df 39 ", "fist word ptr ", "fistp word ptr ",
    "fbld tbyte ptr ", "fild qword ptr ", "fbstp tbyte ptr ",
    "fistp qword ptr ",

    /* mod == 11 */
    "ffree ", "fxch ", "fst ", "fstp ",
    "esc_df 3c,", "esc_df 3d,", "esc_df 3e,", "esc_df 3f,"
};

#endif /* DEBUG */

/* opcode=0xdf */
void x86emuOp_esc_coprocess_df(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset = 0;
    u8 stkelem = 0;

    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTINSTR32(x86emu_fpu_op_df_tab, mod, rh, rl);
    switch (mod) {
      case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        break;
      case 3:                   /* register to register */
		stkelem = (u8)rl;
        DECODE_PRINTF2(" st(%d)\n", stkelem);
        break;
    }
#ifdef X86EMU_FPU_PRESENT
    switch (mod) {
      case 3:
        switch (rh) {
          case 0:
            x86emu_fpu_R_ffree(stkelem);
            break;
          case 1:
            x86emu_fpu_R_fxch(stkelem);
            break;
          case 2:
            x86emu_fpu_R_fst(stkelem);  /* register version */
            break;
          case 3:
            x86emu_fpu_R_fstp(stkelem); /* register version */
            break;
          default:
            x86emu_fpu_illegal();
            break;
        }
        break;
      default:
        switch (rh) {
          case 0:
            x86emu_fpu_M_fild(X86EMU_FPU_WORD, destoffset);
            break;
          case 1:
            x86emu_fpu_illegal();
            break;
          case 2:
            x86emu_fpu_M_fist(X86EMU_FPU_WORD, destoffset);
            break;
          case 3:
            x86emu_fpu_M_fistp(X86EMU_FPU_WORD, destoffset);
            break;
          case 4:
            x86emu_fpu_M_fbld(X86EMU_FPU_BSD, destoffset);
            break;
          case 5:
            x86emu_fpu_M_fild(X86EMU_FPU_LONG, destoffset);
            break;
          case 6:
            x86emu_fpu_M_fbstp(X86EMU_FPU_BSD, destoffset);
            break;
          case 7:
            x86emu_fpu_M_fistp(X86EMU_FPU_LONG, destoffset);
            break;
        }
    }
#else
    (void)destoffset;
    (void)stkelem;
#endif
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}
