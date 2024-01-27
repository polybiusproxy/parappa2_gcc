/* { dg-do compile { target mips64r5900-*-* } } */
typedef unsigned int u_long128 __attribute__((mode(TI)));

typedef struct {
        u_long128       p0, p1; 
} testArea;

static testArea sampleBUF __attribute__((aligned(16)));

extern u_long128 i128;
extern u_long128 j128;
extern testArea str1_128;
extern testArea str2_128;
extern u_long128 a128[];

int sceSample(testArea *p)
{
        i128 = j128;
        str1_128 = str2_128;
        a128[0] = j128;
        str1_128.p0 = j128;

        i128 = a128[0];
        a128[0] = a128[1];
        j128 = str1_128.p0;
        sampleBUF.p0 = p->p0;
        *(u_long128 *)&(sampleBUF.p1) = *(u_long128 *)&(p->p1);
}

