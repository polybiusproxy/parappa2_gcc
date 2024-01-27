/* { dg-do run { target mips64r5900-*-* } } */

typedef int long128 __attribute__ ((mode (TI)));

static char    a0;
static long128 a1;
       char    a2;
       long128 a3;
       char    a4 = 'a';
       long128 a5 = 0;

int main()
{
  if ((int)&a1 & 0xf)
    abort ();
  if ((int)&a3 & 0xf)
    abort ();
  if ((int)&a5 & 0xf)
    abort ();
  exit (0);
}
