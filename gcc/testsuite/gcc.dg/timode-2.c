/* { dg-do compile { target mips64r5900-*-* } } */

/* Test assignment from a 128bit wide constant.  */
typedef int TItype __attribute__ ((mode (TI)));
TItype t1 = 0x12345678123456781234567812345678;
