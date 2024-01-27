/* { dg-do compile { target mips64r5900-*-* } } */
/* { dg-options { -O2 } } */

typedef int int128 __attribute__ ((mode(TI)));

__inline__ static int128 __ppacb__(int128 rs, int128 rt)
{
    register int128 rd;

    __asm__ __volatile__ ("ppacb  %0,%1,%2" : "=d"(rd) : "d"(rs), "d"(rt));
    return rd;
}

__inline__ static int128 __paddb__(int128 rs, int128 rt)
{
    register int128 rd;

    __asm__ __volatile__ ("paddb  %0,%1,%2" : "=d"(rd) : "d"(rs), "d"(rt));
    return rd;
}

int128 i128;

void foo(int128 a, int128 b)
{
    i128 = __paddb__(a, __ppacb__(a, b));
}

