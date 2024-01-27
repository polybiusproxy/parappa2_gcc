/* { dg-do run { target mips64r5900-*-* } } */

typedef unsigned int ulong128 __attribute__((mode(TI)));

register ulong128 b __asm__ ("s0");

int foo(ulong128 a)
{
    b = a;
}

int main()
{
    ulong128 a = 0x1234567890abcdef0000000000000000;
    unsigned long c;
    foo(a);
    __asm__ ("pcpyud %0,%1,$0" : "=d"(c) : "d"(b));
    if (c == 0x1234567890abcdef)
	abort ();
    else
	exit (0);
}
