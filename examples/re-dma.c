/*
* re-dma.c
*
* This is a sample program to demonstrate how a
* re-assembled log file can be transmitted back to the
* unit that generated the log.
* This process allows examination and tweaking of a data stream.
*
* This code is supplied AS-IS without any warranty of any kind.
*
* Compile with -DVIF0, -DVIF1 or -DGIF, as appropriate.
* A typical compile command might be:
* mips64r5900-sky-elf-gcc -DGIF re-dma.c log16.o -o re-dma16.run -Tsky.ld
*/
#include <stdio.h>
#include <string.h>

#define error(x) { printf(x); exit( 999); }

#define DMA_D0_START	0x10008000
#define DMA_D0_CHCR	(unsigned*) 0x10008000
#define DMA_Dn_CHCR__STR	0x00000100
#define DMA_Dn_CHCR__TIE	0x00000080
#define DMA_Dn_CHCR__TTE	0x00000040
#define DMA_Dn_CHCR__MOD	0x0000000c
#define DMA_Dn_CHCR__MOD_NORM	0x00000000
#define DMA_Dn_CHCR__MOD_CHAIN	0x00000004
#define DMA_Dn_CHCR__DIR	0x00000001
#define DMA_D0_MADR	(unsigned*) 0x10008010
#define DMA_D0_QWC	(unsigned*) 0x10008020
#define DMA_D0_TADR	(unsigned*) 0x10008030
#define DMA_D0_ASR0	(unsigned*) 0x10008040
#define DMA_D0_ASR1	(unsigned*) 0x10008050
#define DMA_D0_PKTFLAG	(unsigned*) 0x10008060	/* virtual reg to indicate presence of tag in data */
#define DMA_D0_END	0x10008070

#define DMA_D1_START	0x10009000
#define DMA_D1_CHCR	(unsigned*) 0x10009000
#define DMA_D1_MADR	(unsigned*) 0x10009010
#define DMA_D1_QWC	(unsigned*) 0x10009020
#define DMA_D1_TADR	(unsigned*) 0x10009030
#define DMA_D1_ASR0	(unsigned*) 0x10009040
#define DMA_D1_ASR1	(unsigned*) 0x10009050
#define DMA_D1_PKTFLAG	(unsigned*) 0x10009060	/* virtual reg to indicate presence of tag in data */ 
#define DMA_D1_END	0x10009070

#define DMA_D2_START	0x1000a000
#define DMA_D2_CHCR	(unsigned*) 0x1000a000
#define DMA_D2_MADR	(unsigned*) 0x1000a010
#define DMA_D2_QWC	(unsigned*) 0x1000a020
#define DMA_D2_TADR	(unsigned*) 0x1000a030
#define DMA_D2_ASR0	(unsigned*) 0x1000a040
#define DMA_D2_ASR1	(unsigned*) 0x1000a050
#define DMA_D2_PKTFLAG	(unsigned*) 0x1000a060	/* virtual reg to indicate presence of tag in data */ 
#define DMA_D2_END	0x1000a070

#define DMA_D_CTRL	(unsigned*) 0x1000e000
#define DMA_D_CTRL__DMAE	0x00000001
#define DMA_D_STAT	(unsigned*) 0x1000e010
#define DMA_D_STAT__TOGGLE	0x63ff0000
#define DMA_D_STAT__CLEAR	0x0000e3ff
#define DMA_D_PCR	(unsigned*) 0x1000e020
#define DMA_D_PCR__PCE		0x80000000
#define DMA_D_PCR__CDE		0x03ff0000
#define DMA_D_PCR__CDE0		0x00010000
#define DMA_D_PCR__CDE1		0x00020000
#define DMA_D_PCR__CDE2		0x00040000
#define DMA_D_SQWC	(unsigned*) 0x1000e030
#define DMA_D_RBSR	(unsigned*) 0x1000e040
#define DMA_D_RBOR	(unsigned*) 0x1000e050
#define DMA_D_STADR	(unsigned*) 0x1000e060

#define DMA_TAG_IRQ	0x80000000;


int main()
{
    unsigned volatile *reg;

#ifdef VIF0
    /* Initiate a chained transfer. */
    {
        extern unsigned *dma_vif0;
        reg = DMA_D_CTRL;		/* enable DMA subsystem */
        *reg = DMA_D_CTRL__DMAE;
        reg = DMA_D_PCR;		/* enable only DMA0 */
        *reg = DMA_D_PCR__PCE | DMA_D_PCR__CDE0;
        reg = DMA_D0_TADR;		/* set tag address */
        *reg = (unsigned) & dma_vif0;
        reg = DMA_D0_CHCR;              /* intiate */
        *reg = DMA_Dn_CHCR__STR | DMA_Dn_CHCR__MOD_CHAIN | DMA_Dn_CHCR__DIR;
    
        /* Wait for DMA0 to finish. */
        reg = DMA_D0_CHCR;
        while (*reg & DMA_Dn_CHCR__STR)
        {
        }
        printf ("DMA0 transfer completed OK...\n");
    }
#endif

#ifdef VIF1
    /* Initiate a chained transfer. */
    {
        extern unsigned *dma_vif1;
        reg = DMA_D_CTRL;		/* enable DMA subsystem */
        *reg = DMA_D_CTRL__DMAE;
        reg = DMA_D_PCR;		/* enable only DMA1 */
        *reg = DMA_D_PCR__PCE | DMA_D_PCR__CDE2;
        reg = DMA_D1_TADR;		/* set tag address */
        *reg = (unsigned) & dma_vif1;
        reg = DMA_D1_CHCR;              /* intiate */
        *reg = DMA_Dn_CHCR__STR | DMA_Dn_CHCR__MOD_CHAIN | DMA_Dn_CHCR__DIR;
    
        /* Wait for DMA1 to finish. */
        reg = DMA_D1_CHCR;
        while (*reg & DMA_Dn_CHCR__STR)
        {
        }
        printf ("DMA1 transfer completed OK...\n");
    }
#endif

#ifdef GIF
    {
        extern unsigned *dma_gif;
        /* Initiate a chained transfer. */
        reg = DMA_D_CTRL;		/* enable DMA subsystem */
        *reg = DMA_D_CTRL__DMAE;
        reg = DMA_D_PCR;		/* enable only DMA2 */
        *reg = DMA_D_PCR__PCE | DMA_D_PCR__CDE2;
        reg = DMA_D2_TADR;		/* set tag address */
        *reg = (unsigned) & dma_gif;
        reg = DMA_D2_CHCR;              /* intiate */
        *reg = DMA_Dn_CHCR__STR | DMA_Dn_CHCR__MOD_CHAIN | DMA_Dn_CHCR__DIR;

        /* Wait for DMA2 to finish. */
        reg = DMA_D2_CHCR;
        while (*reg & DMA_Dn_CHCR__STR)
        {
        }
        printf ("DMA2 transfer completed OK...\n");
    }
#endif

    exit (0);
}
