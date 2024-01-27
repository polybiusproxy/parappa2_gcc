/*
 * i82596.c -- Driver for Intel 82596 Ethernet Controller
 *
 * Copyright (c) 1998, 1999 Cygnus Support
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 */
#include <string.h>
#include <bsp/bsp.h>
#include <bsp/cpu.h>
#include <bsp/dve39.h>
#include "bsp_if.h"
#include "i82596.h"
#include "net.h"


#if DEBUG_ENET
static void DumpARP(unsigned char *eth_pkt);
static void DumpStatus(void);
#endif

/*
 *  Receive Frame Descriptors
 */
static struct rfd   __rbufs[NUM_RX_BUFFS];
static struct rfd   *rx_tail;

/*
 *  Single transmit buffer.
 */
static struct tx __tx;
static struct tx *txp;

/*
 * The scp is declared as an array of bytes so that we can adjust it to a
 * 16 byte boundary as required by the i82596.
 */
static char        _scp[sizeof(struct scp) + 15];
static struct iscp _iscp;
static struct scb  _scb;
 
static struct scp   *scp;
static struct iscp  *iscp;
static struct scb   *scb;

enet_addr_t __local_enet_addr;           /* local ethernet address */

static inline void
i596_wait(struct scb *p, int n)
{
    int i = n;

    while (scb->command) {
	if (--i <= 0) {
	    bsp_printf("timed out waiting on prior scb->command <0x%x>\n",
			scb->command);
	    i = n;
	}
    }
}


/*
 * Have the i82596 execute the given command. The cmd data must be uncached
 * as is the scb.
 */
void
i596_do_cmd(struct cmd *cmd)
{
    cmd->command |= CMD_EL;
    cmd->status = 0;
    cmd->link = NULL;

    /*
     * wait for prior commands to finish.
     */
    i596_wait(scb, 10000);
    
    scb->cba = I596_ADDR(cmd);
    scb->command = SCB_CUC_START;

    I596_CHANNEL_ATTN();

    /*
     * wait for command to finish.
     */
    i596_wait(scb, 10000);

    if (scb->status & SCB_CX)
	scb->command |= SCB_ACK_CX;
    if (scb->status & SCB_CNA)
	scb->command |= SCB_ACK_CNA;

    if (scb->command) {
	I596_CHANNEL_ATTN();

	/*
	 * wait for ack to finish.
	 */
	i596_wait(scb, 10000);
    }
}


/* 
 * Initialize the Ethernet Interface.
 */
void
i596_init(enet_addr_t enet_addr)
{
    struct rfd         *rfd;
    union {
	struct config_cmd config;
	struct ia_cmd     ia;
    } u;
    struct config_cmd  *cfgp;
    struct ia_cmd      *iap;
    int                i;

#if DEBUG_ENET >= 3
    bsp_printf("Initializing i82896...\n");
#endif

    memcpy(__local_enet_addr, enet_addr, sizeof(enet_addr_t));

    I596_RESET();

    /*
     * initialize pointers to i596 control structures using uncached
     * addresses. Note the scp must be aligned to a 16 byte boundary.
     */
    scp  = (struct scp *)CPU_UNCACHED(&_scp[0]);
    scp  = (struct scp *)(((unsigned)scp + 15) & 0xfffffff0);
    iscp = (struct iscp *)CPU_UNCACHED(&_iscp);
    scb  = (struct scb *)CPU_UNCACHED(&_scb);
    cfgp = (struct config_cmd *)CPU_UNCACHED(&u.config);
    iap  = (struct ia_cmd *)CPU_UNCACHED(&u.ia);
    txp  = (struct tx *)CPU_UNCACHED(&__tx);

    memset(scb, 0, sizeof(*scb));
    
    /*
     *  initialize receive buffers
     */
    rx_tail = CPU_UNCACHED(&__rbufs[0]);
    memset(rx_tail, 0, sizeof(*rx_tail));

    for (rfd = rx_tail; rfd <= (rx_tail + NUM_RX_BUFFS - 1); rfd++) {
	rfd->size = sizeof(rfd->data);
	rfd->rbd = (void *)0;

	/* put on the head of rfd list */
	rfd->link = scb->rfa;
	scb->rfa = I596_ADDR(rfd);
    }

    /* make circular list */
    rx_tail->link = scb->rfa;
    rx_tail->command |= CMD_EL;

    /* fill out scp */
    memset(scp, 0, sizeof(*scp));
    scp->sysbus = SYSBUS_LINEAR | 0x40;
    scp->iscp   = I596_ADDR(iscp);

    /* fill out iscp */
    memset(iscp, 0, sizeof(*iscp));
    iscp->busy = 1;
    iscp->scb  = I596_ADDR(scb);

#if DEBUG_ENET >= 3
    bsp_printf("Setting SCP...\n");
#endif
    I596_SET_SCP(scp);

#if DEBUG_ENET >= 3
    bsp_printf("Sending Channel Attn...\n");
#endif
    I596_CHANNEL_ATTN();    

#if DEBUG_ENET >= 3
    bsp_printf("Waiting for busy flag...\n");
#endif
    /* wait for busy flag to clear */
    i = 1000000;
    while (iscp->busy) {
	if (--i <= 0) {
	    bsp_printf("i82596 failed to initialize\n");
	    while (1) ;
	}
    }

#if DEBUG_ENET >= 3
    bsp_printf("Done.\n");
#endif

    scb->command = 0;

    cfgp->cmd.status = 0;
    cfgp->cmd.command = CMD_CONFIGURE;
    cfgp->cmd.link = NULL;
    cfgp->data[0]  = 0x8e;	/* length, prefetch on */
    cfgp->data[1]  = 0xc8;	/* fifo to 8, monitor off */
    cfgp->data[2]  = 0x40;	/* don't save bad frames */
    cfgp->data[3]  = 0x2e;	/* No src address insertion, 8 byte preamble */
    cfgp->data[4]  = 0x00;	/* priority and backoff defaults */
    cfgp->data[5]  = 0x60;	/* interframe spacing */
    cfgp->data[6]  = 0x00;	/* slot time LSB */
    cfgp->data[7]  = 0xf2;	/* slot time and retries */
    cfgp->data[8]  = 0x00;	/* promiscuous mode */
    cfgp->data[9]  = 0x00;	/* collision detect */
    cfgp->data[10] = 0x40;	/* minimum frame length */
    cfgp->data[11] = 0xff;
    cfgp->data[12] = 0x00;
    cfgp->data[13] = 0x3f;	/*  single IA */
    cfgp->data[14] = 0x00;
    cfgp->data[15] = 0x00;
#if DEBUG_ENET >= 3
    bsp_printf("Sending CONFIG command to i82596...\n");
#endif
    i596_do_cmd(&cfgp->cmd);
#if DEBUG_ENET >= 3
    bsp_printf("Done.\n");
#endif

    iap->cmd.status = 0;
    iap->cmd.command = CMD_IASETUP;
    iap->cmd.link = NULL;
    memcpy(iap->eth_addr, __local_enet_addr, 6);
#if DEBUG_ENET >= 3
    bsp_printf("Sending IASET command to i82596...\n");
#endif
    i596_do_cmd(&iap->cmd);
#if DEBUG_ENET >= 3
    bsp_printf("Done...\n");
#endif

#if DEBUG_ENET >= 3
    bsp_printf("Starting i82596 Receive...\n");
#endif

    scb->command = SCB_RUC_START;
    I596_CHANNEL_ATTN();
    i596_wait(scb, 10000);

#if DEBUG_ENET >= 3
    bsp_printf("Done.\n");

    DumpStatus();
#endif
}


/*
 *  Send a packet out over the ethernet.  The packet is sitting at the
 *  beginning of the transmit buffer.  The routine returns when the
 *  packet has been successfully sent.
 */
static void
i596_send(void *unused, const char *buf, int length)
{
    if (length < ETH_MIN_PKTLEN) 
        length = ETH_MIN_PKTLEN; /* and min. ethernet len */

    memcpy(((eth_header_t *)buf)->source, __local_enet_addr,
	   sizeof(__local_enet_addr));

#if DEBUG_ENET > 1
    if (((eth_header_t *)buf)->type == ntohs(ETH_TYPE_ARP))
	DumpARP(buf);
#endif

#if (DEBUG_ENET >= 16)
    bsp_printf("mb86964_send: len[%d]\n", length);
#endif

    bsp_flush_dcache(buf, length);

    txp->cmd.command = CMD_XMIT | CMD_FLEXMODE;
    txp->cmd.status = 0;
    txp->cmd.link = 0;
    txp->tbp = I596_ADDR(&txp->tbd);
    txp->size = 0;
    txp->pad = 0;
    txp->tbd.size = length | TBD_EOF;
    txp->tbd.pad = 0;
    txp->tbd.next = 0;
    txp->tbd.txbuf = I596_ADDR(buf);

    i596_do_cmd(&txp->cmd);

#if DEBUG_ENET > 1
    bsp_printf("done\n");
#endif
}


/* 
 * Test for the arrival of a packet on the Ethernet interface.
 * If a packet is found, copy it into the buffer and return its length.
 * If no packet, then return 0.
 */
int
i596_receive(void *unused, char *buf, int maxlen)
{
    int   pktlen;
    short status;
    struct rfd *rfd;

    /* convert i596 phys address into a cpu uncached address */
    rfd = I596_CPU_UNCACHED(scb->rfa);

    /* wait for outstanding commands to finish */
    if (scb->command != 0)
	return 0;

    status = scb->status;

    /* return NULL if no rcv unit events */
    if ((status & (SCB_FR | SCB_RNR)) == 0)
	return 0;

    /* return NULL if its not done */
    if ((rfd->status & CMD_C) == 0)
	return 0;

    /* a packet was received, check if it was good */
    if (rfd->status & CMD_OK) {
	pktlen = rfd->count & 0x3fff;

	memmove(buf, rfd->data, pktlen);

	rfd->status = 0;
	rfd->count = 0;
	rfd->rbd = (void *)0;

#if DEBUG_ENET > 1
	bsp_printf("got pkt - type 0x%x\n", ((eth_header_t *)buf)->type);
#endif

#if DEBUG_ENET
	if (((eth_header_t *)buf)->type == ntohs(0x806))
	{
	    arp_header_t *ahdr = (arp_header_t *)(buf + sizeof(eth_header_t));

	    if (ahdr->opcode == 1 && !memcmp(ahdr->target_ip, __local_ip_addr, 4))
		bsp_printf("got arp req\n");
	}
#endif
    } else
	pktlen = 0;

    rx_tail->command = 0;
    rx_tail = rfd;
    scb->rfa = rx_tail->link;
    rx_tail->command = CMD_EL;

    scb->command = SCB_CUC_NOP | SCB_RUC_START;
    if (scb->status & SCB_FR)
	scb->command |= SCB_ACK_FR;
    if (scb->status & SCB_RNR)
	scb->command |= SCB_ACK_RNR;

    I596_CHANNEL_ATTN();
    i596_wait(scb, 1000);

    return pktlen;
}


static int
i596_irq_handler(int irq_nr, void *regs)
{
    struct rfd *rfd;

    I596_IRQ_CLEAR();

    if (scb->status & (SCB_FR | SCB_RNR)) {
	/* convert i596 phys address into a cpu uncached address */
	rfd = I596_CPU_UNCACHED(scb->rfa);

#if 0
	if (rfd->status & CMD_C)
	    return _bsp_tcp_rx_interrupt(regs);
#endif
    }

    return 1;
}

static bsp_vec_t eth_vec;

static int
i596_control(void *unused, int func, ...)
{
    int          retval = 0;
    va_list      ap;

    va_start (ap, func);

    switch (func) {
      case COMMCTL_SETBAUD:
	retval = -1;
	break;

      case COMMCTL_GETBAUD:
	retval = -1;
	break;

      case COMMCTL_INSTALL_DBG_ISR:
	eth_vec.handler = i596_irq_handler;
	eth_vec.next = NULL;

	bsp_install_vec(BSP_VEC_INTERRUPT, BSP_IRQ_ETHER,
			BSP_VEC_REPLACE, &eth_vec);
	bsp_enable_irq(BSP_IRQ_ETHER);
	break;
	
      case COMMCTL_REMOVE_DBG_ISR:
	bsp_disable_irq(BSP_IRQ_ETHER);
	bsp_remove_vec(BSP_VEC_INTERRUPT, BSP_IRQ_ETHER, &eth_vec);
	break;

      default:
	retval = -1;
	break;
    }

    va_end(ap);
    return retval;
}


static void
dummy_putc(void *unused, char ch)
{
    bsp_printf("putc not supported for an enet link driver\n");
}

static int
dummy_getc(void *unused)
{
    bsp_printf("getc not supported for an enet link driver\n");
}


static struct bsp_comm_procs i596_procs = {
    NULL,
    i596_send, i596_receive,
    dummy_putc, dummy_getc,
    i596_control
};


void
__enet_link_init(enet_addr_t enet_addr)
{
    i596_init(enet_addr);
    _bsp_install_enet_driver(&i596_procs);
}


#if DEBUG_ENET
static void
DumpStatus(void)
{
    bsp_printf("scb: command[0x%x] status[0x%x]\n",
		scb->command, scb->status);
}

static const char *arp_op[] = {
    "invalid",
    "ARP request",
    "ARP reply",
    "RARP request",
    "RARP reply"
};

static void
DumpARP(unsigned char *eth_pkt)
{
    arp_header_t *ahdr = (arp_header_t *)(eth_pkt + sizeof(enet_header_t));

    bsp_printf("%s info:\n",arp_op[ahdr->opcode]);
    bsp_printf("  hw_type[%d]  protocol[%d]  hwlen[%d]  protolen[%d]\n",
	       ntohs(ahdr->hw_type), ntohs(ahdr->protocol),
	       ahdr->hw_len, ahdr->proto_len);
    bsp_printf("  sender_enet: %02x.%02x.%02x.%02x.%02x.%02x\n",
	       ahdr->sender_enet[0], ahdr->sender_enet[1],
	       ahdr->sender_enet[2], ahdr->sender_enet[3],
	       ahdr->sender_enet[4], ahdr->sender_enet[5]);
    bsp_printf("  sender_ip:  %d.%d.%d.%d\n",
	       ahdr->sender_ip[0], ahdr->sender_ip[1],
	       ahdr->sender_ip[2], ahdr->sender_ip[3]);
    bsp_printf("  target_eth: %02x.%02x.%02x.%02x.%02x.%02x\n",
	       ahdr->target_enet[0], ahdr->target_enet[1],
	       ahdr->target_enet[2], ahdr->target_enet[3],
	       ahdr->target_enet[4], ahdr->target_enet[5]);
    bsp_printf("  target_ip:  %d.%d.%d.%d\n",
	       ahdr->target_ip[0], ahdr->target_ip[1],
	       ahdr->target_ip[2], ahdr->target_ip[3]);
}
#endif
