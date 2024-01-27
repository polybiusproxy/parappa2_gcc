/* { dg-do compile { target mips64r5900-*-* } } */

/* Test basic TImode parsing, assignment from "small" literals, assignment
   of TImode variables, parameters, return values.  */

typedef int TItype __attribute__ ((mode (TI)));

TItype bar (TItype, TItype);

TItype foo(TItype a, TItype b)
{
    TItype t1 = 0x1234567812345678;
    TItype t2 = 0xabcedf;

    t2 = a;                                       
    t1 = bar (t1, t2);
    return t1;
}


