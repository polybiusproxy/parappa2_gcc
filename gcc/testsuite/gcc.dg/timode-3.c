/* { dg-do run { target mips64r5900-*-* } } */
/* { dg-options -O2 } */

typedef int long128 __attribute__ ((mode (TI)));

typedef union {
     long128 veryLong;
     short int si;
} t_uniInt;

int main()
{
     t_uniInt uniInt;

     memset (&uniInt, -1, sizeof (t_uniInt));
     uniInt.veryLong = 0;
     if (uniInt.si != 0)
       abort ();
     exit (0);
}

