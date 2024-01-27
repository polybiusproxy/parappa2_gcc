/*
* This is a (Toronto created) wrapper program to drive the sce_tests.
*
* Copyright (C) 1998, Cygnus Solutions
*
*/

#include <stdio.h>

#define DMA_D1_START		(volatile unsigned*)0x90009000
#define DMA_D1_CHCR		(volatile unsigned*)0x90009000
#define DMA_Dn_CHCR__STR		0x00000100
#define DMA_Dn_CHCR__TIE		0x00000080
#define DMA_Dn_CHCR__TTE		0x00000040
#define DMA_Dn_CHCR__MOD		0x0000000c
#define DMA_Dn_CHCR__MOD_NORM		0x00000000
#define DMA_Dn_CHCR__MOD_CHAIN		0x00000004
#define DMA_Dn_CHCR__DIR		0x00000001
#define DMA_D1_MADR		(volatile unsigned*)0x90009010
#define DMA_D1_QWC		(volatile unsigned*)0x90009020
#define DMA_D1_TADR		(volatile unsigned*)0x90009030
#define DMA_D1_ASR0		(volatile unsigned*)0x90009040
#define DMA_D1_ASR1		(volatile unsigned*)0x90009050

#define DMA_D_CTRL		(volatile unsigned*)0x9000e000
#define DMA_D_CTRL__DMAE		0x00000001
#define DMA_D_STAT		(volatile unsigned*)0x9000e010
#define DMA_D_STAT__TOGGLE		0x63ff0000
#define DMA_D_STAT__CLEAR		0x0000e3ff
#define DMA_D_PCR		(volatile unsigned*)0x9000e020
#define DMA_D_PCR__PCE			0x80000000
#define DMA_D_PCR__CDE			0x03ff0000
#define DMA_D_PCR__CDE0			0x00010000
#define DMA_D_PCR__CDE1			0x00020000
#define DMA_D_PCR__CDE2			0x00040000
#define DMA_D_SQWC		(volatile unsigned*)0x9000e030
#define DMA_D_RBSR		(volatile unsigned*)0x9000e040
#define DMA_D_RBOR		(volatile unsigned*)0x9000e050
#define DMA_D_STADR		(volatile unsigned*)0x9000e060

#define  VIF1_STAT		(volatile unsigned*)0x90003C00
#define  VIF1_STAT_FQC_MASK		0x1F000000
#define  VIF1_STAT_ER1_MASK		0x00002000
#define  VIF1_STAT_PPS_MASK		0x00000003
        
#define  VPU_STAT		(volatile unsigned*)0x910073d0
#define  VPU_STAT_VBS1_MASK		0x00000100

extern char *__bovutext, *__eovutext;

int main()
{
    unsigned length = (unsigned) & __eovutext - (unsigned) & __bovutext;
    int vif1_stat, vpu_stat, i;

    *DMA_D_CTRL  = DMA_D_CTRL__DMAE;			/* enable subsystem */
    *DMA_D_PCR   = DMA_D_PCR__PCE | DMA_D_PCR__CDE1;	/* enable only DMA1 */
    *DMA_D1_MADR = (unsigned) & __bovutext;		/* data address */
    *DMA_D1_QWC  = length / 16;				/* quad word count */
    *DMA_D1_CHCR = DMA_Dn_CHCR__STR | DMA_Dn_CHCR__MOD_NORM | DMA_Dn_CHCR__DIR;

    do {
	vif1_stat = *VIF1_STAT;
	vpu_stat = *VPU_STAT;
        if( vif1_stat & VIF1_STAT_ER1_MASK)
        {
          write( 1, "stopped - Reserved Instruction Error\n", 37);
          break;
        }
    } while (   (vif1_stat & VIF1_STAT_PPS_MASK) != 0
	     || (vif1_stat & VIF1_STAT_FQC_MASK) != 0
	     || (vpu_stat  & VPU_STAT_VBS1_MASK) != 0);

    for (i=0; i<200000; ++i) {}

    return 0;
}
