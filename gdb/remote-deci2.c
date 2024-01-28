/*
 * Copyright (C) 1999, 2000 Sony Computer Entertainment Inc., All Rights Reserved.
 */

#include "defs.h"
#include "gdb_string.h"
#include <fcntl.h>
#include "frame.h"
#include "inferior.h"
#include "bfd.h"
#include "symfile.h"
#include "target.h"
#include "wait.h"
/*#include "terminal.h"*/
#include "gdbcmd.h"
#include "objfiles.h"
#include "gdb-stabs.h"
#include "gdbthread.h"

#include "dcache.h"

#ifdef USG
#include <sys/types.h>
#endif

#include <signal.h>
#include "serial.h"

#include "gdbcore.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/param.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>

typedef unsigned char uchar;

#if HOST_BYTE_ORDER == LITTLE_ENDIAN
#define STORE(var, val) ((var) = (val))
#define LOAD(var)       (var)
#else
#define STORE(var, val) deci2_order_store((unsigned char *)&(var), \
					  sizeof(var),             \
					  (unsigned LONGEST)(val))
#define LOAD(var)       deci2_order_load((unsigned char *)&(var), sizeof(var))

static void deci2_order_store PARAMS((unsigned char *addr, int len, int val));
static unsigned LONGEST deci2_order_load PARAMS((unsigned char *addr, int len));
#endif

#define DECI2_DEFAULT_DSNETM_PORT	"8510"
#define DECI2_DEFAULT_PRIORITY		0xd0
#define DECI2_DEFAULT_TIMEOUT		10

#define DECI2_PROTOCOL_DCMP	0x0001
#define DECI2_PROTOCOL_NETMP	0x0400
#define DECI2_PROTOCOL_EFILEP	0x0120
#define DECI2_PROTOCOL_ISDBGP	0x0130
#define DECI2_PROTOCOL_ITDBGP	0x0140
#define DECI2_PROTOCOL_ESDBGP	0x0230
#define DECI2_PROTOCOL_ETDBGP	0x0240
#define DECI2_PROTOCOL_ITTY	0x0110
#define DECI2_PROTOCOL_ETTY	0x0210

typedef struct {
    unsigned short length;
    unsigned short reserved;
    unsigned short protocol;
    unsigned char  source;
    unsigned char  destination;
} DECI2HEAD;

#define DECI2HEAD_SZ	sizeof(DECI2HEAD)

#define DECI2_NODE_ID_IOP  'I'
#define DECI2_NODE_ID_EE   'E'
#define DECI2_NODE_ID_HOST 'H'

#define DECI2_MAX_PACKET_SIZE 65536  /* 64 KB */

/*
 * dcmp
 */

typedef struct {
    unsigned char type;
    unsigned char code;
    unsigned short unused;
} DCMPHEAD;

#define DCMPHEAD_SZ	sizeof(DCMPHEAD)

#define DCMP_TYPE_CONNECT	0
#define DCMP_TYPE_ECHO		1
#define DCMP_TYPE_STATUS	2
#define DCMP_TYPE_ERROR		3

typedef struct {
    unsigned char result;
} DCMP_CONNECT;

#define DCMP_CONNECT_SZ	sizeof(DCMP_CONNECT);

/* code for DCMP_TYPE_CONNECT */
#define DCMP_CODE_CONNECT	0
#define DCMP_CODE_CONNECTR	1
#define DCMP_CODE_DISCONNECT	2
#define DCMP_CODE_DISCONNECTR	3

/* result for DCMP_TYPE_CONNECT */
#define DCMP_ERR_INVALDEST	1  /* destination invalid */
#define DCMP_ERR_ALREADYCONN	2  /* already connected   */
#define DCMP_ERR_NOTCONNECT	3  /* not connected       */

typedef struct {
    unsigned short identifier;
    unsigned short sequence_number;
} DCMP_ECHO;

#define DCMP_ECHO_SZ	sizeof(DCMP_ECHO)

/* code for DCMP_TYPE_ECHO */
#define DCMPC_ECHO	0
#define DCMPC_ECHOR	1

typedef struct {
    unsigned short protocol;
} DCMP_STATUS;

#define DCMP_STATUS_SZ	sizeof(DCMP_STATUS)

/* code for DCMP_TYPE_STATUS */
#define DCMP_CODE_CONNECTED	0  /* !NOCONNECT */
#define DCMP_CODE_PROTO		1  /* !NOPROTO   */
#define DCMP_CODE_UNLOCKED	2  /* !LOCKED    */
#define DCMP_CODE_SPACE		3  /* !NOSPACE   */
#define DCMP_CODE_ROUTE		4  /* !NOROUTE   */

typedef struct {
    DECI2HEAD org_hdr;
    unsigned int word0;
    unsigned int word1;
    unsigned int word2;
    unsigned int word3;
} DCMP_ERROR;

#define DCMP_ERROR_SZ	sizeof(DCMP_ERROR)

/* code for DCMP_TYPE_ERROR */
#define DCMP_CODE_NOROUTE	0  /* no route to node                 */
#define DCMP_CODE_NOPROTO	1  /* protocol unreachable             */
#define DCMP_CODE_LOCKED	2  /* locked                           */
#define DCMP_CODE_NOSPACE	3  /* deci2 manager/dsnetm buffer full */
#define DCMP_CODE_INVALHEAD	4  /* invalid header                   */
#define DCMP_CODE_NOCONNECT	5  /* not connected                    */

/*
 * netmp
 */

typedef struct {
    unsigned char  code;
    unsigned char  result;
} NETMPHEAD;

#define NETMPHEAD_SZ	sizeof(NETMPHEAD)

typedef struct {
    unsigned char  priority;
    unsigned char  unused;
    unsigned short protocol;
} NETMP_CONNECT;

#define NETMP_CONNECT_SZ	sizeof(NETMP_CONNECT)

/* netmp message code */
#define NETMP_CODE_CONNECT	0	/* connect request */
#define NETMP_CODE_CONNECTR	1	/* connect reply   */
#define NETMP_CODE_RESET	2	/* reset request   */
#define NETMP_CODE_RESETR	3	/* reset reply     */
#define NETMP_CODE_MESSAGE	4	/* message request */
#define NETMP_CODE_MESSAGER	5	/* message reply   */
#define NETMP_CODE_STATUS	6	/* status request  */
#define NETMP_CODE_STATUSR	7	/* status reply    */
#define NETMP_CODE_KILL		8	/* kill request    */
#define NETMP_CODE_KILLR	9	/* kill reply      */
#define NETMP_CODE_VERSION	10	/* version request */
#define NETMP_CODE_VERSIONR	11	/* version reply   */

/* netmp result code */
#define NETMP_ERR_OK		0	/* good            */
#define NETMP_ERR_INVAL		1	/* invalid request */
#define NETMP_ERR_BUSY		2	/* protocol busy   */
#define NETMP_ERR_NOTCONN	3	/* can not connect */
#define NETMP_ERR_ALREADYCONN	4	/* already connect */
#define NETMP_ERR_NOMEM		5	/* no memory       */
#define NETMP_ERR_NOPROTO	6	/* no protocol     */

/*
 * dbgp
 */

typedef struct {
    unsigned short id;
    unsigned char  type;
    unsigned char  code;
    unsigned char  result;
    unsigned char  count;
    unsigned short unused;
} DBGPHEAD;

#define DBGPHEAD_SZ	sizeof(DBGPHEAD)

typedef struct {
    unsigned int   major_ver;
    unsigned int   minor_ver;
    unsigned int   target_id;
    unsigned int   reserved1;
    unsigned int   mem_align;
    unsigned int   reserved2;
    unsigned int   reg_size;
    unsigned int   nreg;
    unsigned int   nbrkpt;
    unsigned int   ncont;
    unsigned int   nstep;
    unsigned int   nnext;
    unsigned int   mem_limit_align;
    unsigned int   mem_limit_size;
    unsigned int   run_stop_state;
    unsigned int   hdbg_area_addr;
    unsigned int   hdbg_area_size;
} DBGP_CONF;

#define DBGP_CONF_SZ	sizeof(DBGP_CONF)

typedef struct {
    unsigned char  kind;
    unsigned char  number;
    unsigned short reserved;
} DBGP_REG;

#define DBGP_REG_SZ	sizeof(DBGP_REG)

typedef struct {
    unsigned char  space;
    unsigned char  align;
    unsigned short reserved;
    unsigned int   address;
    unsigned int   length;
} DBGP_MEM;

#define DBGP_MEM_SZ	sizeof(DBGP_MEM)

typedef struct {
    unsigned int entry;
    unsigned int gp;
    unsigned int reserved1;
    unsigned int reserved2;
    unsigned int argc;
} DBGP_RUN;

#define DBGP_RUN_SZ	sizeof(DBGP_RUN)

/* DBGP - id */
#define DBGP_CPUID_CPU	0	/* CPU (ESDBGP) */
#define DBGP_CPUID_VU0	1	/* VU0 (ESDBGP) */
#define DBGP_CPUID_VU1	2	/* VU1 (ESDBGP) */

/* DBGP - type, code */
#define DBGP_TYPE_GETCONF	0x00	/* Get Configuration Request    */
#define DBGP_TYPE_GETCONFR	0x01	/* Get Configuration Reply      */
#define DBGP_TYPE_GETREG	0x04	/* Get Register Request         */
#define DBGP_TYPE_GETREGR	0x05	/* Get Register Reply           */
#define DBGP_TYPE_PUTREG	0x06	/* Put Register Request         */
#define DBGP_TYPE_PUTREGR	0x07	/* Put Register Reply           */
#define DBGP_TYPE_RDMEM		0x08	/* Read Memory Request          */
#define DBGP_TYPE_RDMEMR	0x09	/* Read Memory Reply            */
#define DBGP_TYPE_WRMEM		0x0a	/* Write Memory Request         */
#define DBGP_TYPE_WRMEMR	0x0b	/* Write Memory Reply           */
#define DBGP_TYPE_GETBRKPT	0x10	/* Get Breakpoint Request       */
#define DBGP_TYPE_GETBRKPTR	0x11	/* Get Breakpoint Reply         */
#define DBGP_TYPE_PUTBRKPT	0x12	/* Put Breakpoint Request       */
#define DBGP_TYPE_PUTBRKPTR	0x13	/* Put Breakpoint Reply         */
#define DBGP_TYPE_BREAK		0x14	/* Break Request                */
#define DBGP_TYPE_BREAKR	0x15	/* Break Reply                  */
#define     DBGP_CODE_OTHER	  0xff	/* not-DBGP_TYPE_CONTINUE	*/
#define DBGP_TYPE_CONTINUE	0x16	/* Continue Request             */
#define   DBGP_CODE_CONT	  0x0	/* Continue                     */
#define   DBGP_CODE_STEP	  0x1	/* Step                         */
#define   DBGP_CODE_NEXT	  0x2	/* Next                         */
#define DBGP_TYPE_CONTINUER	0x17	/* Continue Reply		*/
#define DBGP_TYPE_RUN		0x18	/* Run Request (ESDBGP only)	*/
#define DBGP_TYPE_RUNR		0x19	/* Run Reply (ESDBGP only)	*/

/* DBGP - result */
#define DBGP_RESULT_GOOD	    0x00 /* Good                                 */
#define DBGP_RESULT_INVALIREQ	    0x01 /* Invalid Request                      */
#define DBGP_RESULT_UNIMPREQ	    0x02 /* Unimplemented Request                */
#define DBGP_RESULT_ERROR	    0x03 /* Error                                */
#define DBGP_RESULT_INVALCONT	    0x04 /* Invalid Continue                     */
#define DBGP_RESULT_TLBERR	    0x10 /* TLB mod/load/store while cmd exec    */
#define DBGP_RESULT_ADRERR	    0x11 /* Address Error (DBGT_XXMEM)           */
#define DBGP_RESULT_BUSERR	    0x12 /* Bus Error (DBGT_XXMEM)               */
#define DBGP_RESULT_INVALSTATE	    0x20 /* Invalid State (DBGT_{BREAK,CONT,..}) */
#define DBGP_RESULT_BREAKED	    0x21 /* Breaked (DBGP_TYPE_BREAK)            */
#define DBGP_RESULT_BRKPT	    0x22 /* Breakpoint (DBGT_{CONT,RUN})         */
#define DBGP_RESULT_STEPNEXT	    0x23 /* Step or Next (DBGC_{STEP,NEXT})      */
#define DBGP_RESULT_EXCEPTION	    0x24 /* Exception                            */
#define DBGP_RESULT_PROGEND	    0x25 /* normal end of program (EE)		 */
#define DBGP_RESULT_BUSYSTATE	    0x26 /* busy/critical state			 */
#define DBGP_RESULT_DEBUG_EXCEPTION 0x27 /* Debug Exception			 */

/* DBGP_CONF - major_ver */
#define DBGCF_MAJOR_CURRENT	0	/* Current Specification */

/* DBGP_CONF - minor_ver */
#define DBGCF_MINOR_FIRST	0	/* First Implementation */

/* DBGP_CONF - reg_size */
#define DBGP_CF_REG_SIZE_WORD	5	/* (1 << 5) =  32bit (IOP) */
#define DBGP_CF_REG_SIZE_QUAD	7	/* (1 << 7) = 128bit (EE)  */

/* DBGP_CONF - run_stop_state */
#define DBGP_CF_RSS_RUNNING	1	/* running */
#define DBGP_CF_RSS_STOPPED	2	/* stopped */

#define ALIGN_BYTE		1
#define ALIGN_HALFWORD		2
#define ALIGN_WORD		4
#define ALIGN_DOUBLEWORD	8
#define ALIGN_QUADWORD		16

#define CF_MAJOR_VER		(LOAD(deci2_target_conf->major_ver))
#define CF_MINOR_VER		(LOAD(deci2_target_conf->minor_ver))
#define CF_TARGET_ID		(LOAD(deci2_target_conf->target_id))
#define CF_MEM_ALIGN		(LOAD(deci2_target_conf->mem_align))
#define CF_REG_SIZE		(LOAD(deci2_target_conf->reg_size))
#define CF_NREG			(LOAD(deci2_target_conf->nreg))
#define CF_NBRKPT		(LOAD(deci2_target_conf->nbrkpt))
#define CF_NCONT		(LOAD(deci2_target_conf->ncont))
#define CF_NSTEP		(LOAD(deci2_target_conf->nstep))
#define CF_NNEXT		(LOAD(deci2_target_conf->nnext))
#define CF_MEM_LIMIT_ALIGN	(LOAD(deci2_target_conf->mem_limit_align))
#define CF_MEM_LIMIT_SIZE	(LOAD(deci2_target_conf->mem_limit_size))

/* IOP register kind, number */
#define ISDBGP_KIND_HL		1	/* hi,lo */
#define   ISDBGP_NUM_LO		  0
#define   ISDBGP_NUM_HI		  1
#define ISDBGP_KIND_GPR		2	/* general purpose register */
#define ISDBGP_KIND_SCC		3	/* system control register */
#define ISDBGP_KIND_GTER	6	/* GTE register */
#define ISDBGP_KIND_GTEC	7	/* GTE control register */

/* EE register kind, number */
#define DBEK_GPR		0	/* general purpose register */
#define     DBEN_GPR_MAX	  32	/*   (128bit x 32) */
#define DBEK_HLS		1	/* hi,lo,sa */
#define     DBEN_HI		  0	/*   hi  (64bit) */
#define     DBEN_LO		  1	/*   lo  (64bit) */
#define     DBEN_HI1		  2	/*   hi1 (64bit) */
#define     DBEN_LO1		  3	/*   lo1 (64bit) */
#define     DBEN_SA		  4	/* shift amount (32bit) */
#define     DBEN_HLS_MAX	  5
#define DBEK_SCR		2	/* system control register */
#define     DBEN_SCR_MAX	  32	/*   (32bit x 32) */
#define DBEK_PCR		3	/* performance counter */
#define     DBEN_PCR_MAX	  3	/*   (32bit x 3) */
#define DBEK_HDR		4	/* hardware debug register */
#define     DBEN_HDR_MAX	  8	/*   (32bit x 8) */
#define DBEK_FPR		5	/* floating point register */
#define     DBEN_FPR_MAX	  32	/*   (32bit x 32) */
#define DBEK_FPC		6	/* floating point control */
#define     DBEN_FPC_MAX	  2	/*   (32bit x 2) */
#define DBEK_V0F		7	/* VU0 floating point register */
#define     DBEN_V0F_MAX	  32	/*   (128bit x 32) */
#define DBEK_V0I		8	/* VU0 integer/control register */
#define     DBEN_V0I_MAX	  32	/*   (32bit x 32) */
#define DBEK_V1F		9	/* VU1 floating point register */
#define     DBEN_V1F_MAX	  32	/*   (128bit x 32) */
#define DBEK_V1I		10	/* VU0 integer/control register */
#define     DBEN_V1I_MAX	  32	/*   (32bit x 32) */

/*
 * EFILEP
 */
#define EFILEPT_OPEN	0
#define EFILEPT_OPENR	1
#define EFILEPT_CLOSE	2
#define EFILEPT_CLOSER	3
#define EFILEPT_READ	4
#define EFILEPT_READR	5
#define EFILEPT_WRITE	6
#define EFILEPT_WRITER	7
#define EFILEPT_SEEK	8
#define EFILEPT_SEEKR	9

#define EFILEP_EACCES	13
#define EFILEP_EDQUOT	122
#define EFILEP_EEXIST	17
#define EFILEP_ENOENT	2
#define EFILEP_ENOSPC	28
#define EFILEP_ENFILE	23
#define EFILEP_EROFS	30
#define EFILEP_EBADF	9

#define EFILEP_O_RDONLY	0x0001
#define EFILEP_O_WRONLY	0x0002
#define EFILEP_O_RDWR	0x0003
#define EFILEP_O_CREAT	0x0200
#define EFILEP_O_TRUNC	0x0400
#define EFILEP_O_EXCL	0x0800

typedef struct {
    unsigned int type;
    unsigned int seq;
} EFILEPHEAD;

#define EFILEPHEAD_SZ	sizeof(EFILEPHEAD)

typedef struct {
    unsigned int result;
    unsigned int fd;
} EFILEP_OPENR;

#define EFILEP_OPENR_SZ	sizeof(EFILEP_OPENR)

typedef struct {
    unsigned int fd;
    unsigned int bytes;
} EFILEP_READ;

#define EFILEP_READ_SZ	sizeof(EFILEP_READ)

typedef struct {
    unsigned int result;
    unsigned int bytes;
} EFILEP_READR;

#define EFILEP_READR_SZ	sizeof(EFILEP_READR)

typedef EFILEP_READ EFILEP_WRITE;

#define EFILEP_WRITE_SZ sizeof(EFILEP_WRITE)

typedef EFILEP_READR EFILEP_WRITER;

#define EFILEP_WRITER_SZ sizeof(EFILEP_WRITER)

typedef struct {
    unsigned int result;
    unsigned int pos;
} EFILEP_SEEKR;

#define EFILEP_SEEKR_SZ sizeof(EFILEP_SEEKR)


extern char *tmp_mips_processor_type;

static struct target_ops deci2_ops;

static int deci2_fd;

/* A flag. target have been opend if non zero. */
static int deci2_opened = 0;

/* target configuration. */
static DBGP_CONF *deci2_target_conf;

/* A flag. target program is loaded if non zero. */
static int program_loaded = 0;

/* Arguments of target program. */
static char *deci2_arg_buf = NULL;

/* size of deci2_arg_buf. */
static int deci2_arg_buf_sz = 0;

/* target status flags. */
static struct {
    int noproto:1;
    int locked:1;
    int nospace:1;
    int noroute:1;
    int noconnect:1;
} deci2_target_status;

/* A flag. non zero when inferior is created but not executed. */
static int use_run_instead_of_cont;

/* Prototypes */
static void print_target_conf();

extern void mips_set_processor_type_command PARAMS ((char *, int));
static void build_argv PARAMS((char *exec_file, char *args));
static void setup_deci2_header_for_netmp PARAMS((unsigned char *buffer,
						 unsigned int length));
static void setup_deci2_header_for_dbgp PARAMS((unsigned char *buffer,
						unsigned int length));
static void setup_deci2_header_for_efilep PARAMS((DECI2HEAD *buffer,
						  unsigned int length));
static void setup_netmp_connect PARAMS((unsigned char *buffer,
					unsigned short protocol));
static int deci2_file_open_flag PARAMS((unsigned int ef_flag));
static unsigned int deci2_file_errno PARAMS((void));
static void deci2_file_open PARAMS((unsigned int seq,
				    unsigned short int ef_flag,
				    unsigned short int mode,
				    char *file_name));
static void deci2_file_close PARAMS((unsigned int seq, unsigned int fd));
static void deci2_file_read PARAMS((unsigned int seq,
				    unsigned int fd,
				    unsigned int bytes));
static void deci2_file_write PARAMS((unsigned int seq,
				     unsigned int fd,
				     unsigned int bytes,
				     unsigned char *data));
static void deci2_file_seek PARAMS((unsigned int seq,
				    unsigned int fd,
				    unsigned int offset,
				    unsigned int base));
static void deci2_send PARAMS((char *buf, int len));
static void deci2_recv_with_timeout PARAMS((unsigned char *buf,
					    int *len,
					    struct timeval *timeout));
static void deci2_recv PARAMS((unsigned char *buf, int *len));
static void deci2_send_recv PARAMS((unsigned char *snd_buf,
				    int snd_len,
				    unsigned char *rcv_buf,
				    int *rcv_len));

static void deci2_get_target_conf PARAMS((void));
static void deci2_init PARAMS((char *arg));
static void deci2_free_all_resources PARAMS((void));
static void deci2_open PARAMS((char *name, int from_tty));
static void deci2_close PARAMS((int quiting));
static void deci2_detach PARAMS((char *args, int from_tty));
static void deci2_run PARAMS((void));
static void deci2_cont PARAMS((int step));
static void deci2_resume PARAMS((int pid, int step, enum target_signal siggnal));
static int deci2_wait PARAMS((int pid, struct target_waitstatus *status));
static void deci2_fetch_registers PARAMS((int regno));
static void deci2_prepare_to_store PARAMS((void));
static int load_mem PARAMS((u_long addr, char *ptr, int len));
static int store_mem PARAMS((u_long addr, char *ptr, int len));
static int deci2_xfer_memory PARAMS((CORE_ADDR memaddr,
				     char *myaddr,
				     int len,
				     int write,
				     struct target_ops *target));
static void deci2_get_vf_reg PARAMS((int regno,
				     unsigned short proc_id,
				     char *reg));
static void deci2_store_registers PARAMS((int regno));
static void deci2_files_info PARAMS((struct target_ops *ignore));
static int deci2_insert_breakpoint PARAMS((CORE_ADDR addr, char *contents_cache));
static int deci2_remove_breakpoint PARAMS((CORE_ADDR addr, char *contents_cache));
static void deci2_kill PARAMS((void));
static void deci2_load(char *prog, int fromtty);
static void deci2_create_inferior PARAMS((char *exec_file, char *args, char **env));
static void deci2_mourn_inferior PARAMS((void));
static void deci2_stop PARAMS((void));

static void
print_target_conf()
{
    printf("major_ver = %x\n", CF_MAJOR_VER);
    printf("minor_ver = %x\n", CF_MINOR_VER);
    printf("target_id = %x\n", CF_TARGET_ID);
    printf("mem_align = %x\n", CF_MEM_ALIGN);
    printf("reg_size  = %x\n", CF_REG_SIZE);
    printf("nreg      = %x\n", CF_NREG);
    printf("nbrkpt    = %x\n", CF_NBRKPT);
    printf("ncont     = %x\n", CF_NCONT);
    printf("nstep     = %x\n", CF_NSTEP);
    printf("nnext     = %x\n", CF_NNEXT);
    printf("mem_limit_align = %x\n", CF_MEM_LIMIT_ALIGN);
    printf("mem_limit_size  = %x\n", CF_MEM_LIMIT_SIZE);
}

#if HOST_BYTE_ORDER != LITTLE_ENDIAN
static void
deci2_order_store(addr, len, val)
    unsigned char *addr;
    int len;
    int val;
{
    unsigned char *p = addr + len;

    while (addr < p) {
        *addr++ = (unsigned char)val;
        val >>= 8;
    }
}

static unsigned LONGEST
deci2_order_load(addr, len)
    unsigned char *addr;
    int len;
{
    unsigned LONGEST val = 0;
    unsigned char *p = addr + len;

    if (sizeof(val) < len)
        error("deci2_order_load: invalid len(%d)", len);

    while (addr < p)
        val = (val << 8) | *--p;

    return val;
}
#endif

static void
build_argv(exec_file, args)
    char *exec_file;
    char *args;
{
    char *p, *q, *r;
    int spaces = 0;
    int *argv;
    int argc;

    if (deci2_arg_buf != NULL)
	free(deci2_arg_buf);
    argc = 1;
    p = args;
    while (*p) {
	while (*p && !isspace(*p))
	    ++ p;
	++ argc;
	while (*p && isspace(*p)) {
	    ++ spaces;
	    ++ p;
	}
    }
    deci2_arg_buf_sz = sizeof(int) * (argc + 1) + strlen(exec_file)
	               + strlen(args) - spaces + argc;
    deci2_arg_buf = xmalloc(deci2_arg_buf_sz);
    STORE(*(int *)deci2_arg_buf, argc);
    argv = (int *)(deci2_arg_buf + sizeof(int));
    STORE(*argv, strlen(exec_file));
    r = deci2_arg_buf + (argc + 1) * sizeof(int);
    strcpy(r, exec_file);
    r += strlen(exec_file) + 1;
    argc = 1;
    p = args;
    while (*p) {
	q = p;
	while (*p && !isspace(*p))
	    *(r++) = *(p++);
	*(r++) = 0;
	++ argv;
	STORE(*argv, p - q + 1);
	while (*p && isspace(*p))
	    ++ p;
    }
}

static void
setup_deci2_header_for_netmp(buffer, length)
    unsigned char *buffer;
    unsigned int length;
{
    DECI2HEAD *header = (DECI2HEAD *)buffer;

    STORE(header->length, length);
    STORE(header->reserved, 0);
    STORE(header->protocol, DECI2_PROTOCOL_NETMP);
    STORE(header->source, DECI2_NODE_ID_HOST);
    STORE(header->destination, DECI2_NODE_ID_HOST);
}

static void
setup_deci2_header_for_dbgp(buffer, length)
    unsigned char *buffer;
    unsigned int length;
{
    DECI2HEAD *header = (DECI2HEAD *)buffer;

    STORE(header->length, length);
    STORE(header->reserved, 0);
    STORE(header->protocol, DECI2_PROTOCOL_ESDBGP);
    STORE(header->source, DECI2_NODE_ID_HOST);
    STORE(header->destination, DECI2_NODE_ID_EE);
}

static void
setup_deci2_header_for_efilep(buffer, length)
    DECI2HEAD *buffer;
    unsigned int length;
    
{
    STORE(buffer->length, length);
    STORE(buffer->reserved, 0);
    STORE(buffer->protocol, DECI2_PROTOCOL_EFILEP);
    STORE(buffer->source, DECI2_NODE_ID_HOST);
    STORE(buffer->destination, DECI2_NODE_ID_IOP);
}

static void
setup_netmp_connect(buffer, protocol)
    unsigned char *buffer;
    unsigned short protocol;
{
    NETMP_CONNECT *netmp_connect = (NETMP_CONNECT *)buffer;

    STORE(netmp_connect->priority, DECI2_DEFAULT_PRIORITY);
    STORE(netmp_connect->unused, 0);
    STORE(netmp_connect->protocol, protocol);
}

static int
deci2_file_open_flag(ef_flag)
    unsigned int ef_flag;
{
    return ((ef_flag & EFILEP_O_RDONLY) ? O_RDONLY : 0)
	| ((ef_flag & EFILEP_O_WRONLY) ? O_WRONLY : 0)
	    | (((ef_flag & EFILEP_O_RDWR) == EFILEP_O_RDWR) ? O_RDWR : 0)
		| ((ef_flag & EFILEP_O_CREAT) ? O_CREAT : 0)
		    | ((ef_flag & EFILEP_O_TRUNC) ? O_TRUNC : 0)
			| ((ef_flag & EFILEP_O_EXCL) ? O_EXCL : 0);
}

static unsigned int
deci2_file_errno()
{
    switch (errno) {
    case EACCES:
	return EFILEP_EACCES;
    case EDQUOT:
	return EFILEP_EDQUOT;
    case EEXIST:
	return EFILEP_EEXIST;
    case ENOENT:
	return EFILEP_ENOENT;
    case ENOSPC:
	return EFILEP_ENOSPC;
    case ENFILE:
	return EFILEP_ENFILE;
    case EROFS:
	return EFILEP_EROFS;
    case EBADF:
	return EFILEP_EBADF;
    default:
	;
    }
    return 0xffff;
}

static void
deci2_file_open(seq, ef_flag, mode, file_name)
    unsigned int seq;
    unsigned short int ef_flag;
    unsigned short int mode;
    char *file_name;
{
    int fd;
    int flag = deci2_file_open_flag(ef_flag);
    unsigned int result;
    struct {
	DECI2HEAD    deci2head;
	EFILEPHEAD   efilep_head;
	EFILEP_OPENR efilep_openr;
    } buffer;
    if (mode == 0) {
	mode_t mask = umask(0);  /* get currnet file creation mask. */
	mode = ~mask & 0666;
	umask(mask);             /* restore file creation mask. */
    }
    if ((fd = open(file_name, flag, mode)) < 0)
	result = deci2_file_errno();
    else
	result = 0;

    setup_deci2_header_for_efilep(&buffer.deci2head, sizeof(buffer));
    STORE(buffer.efilep_head.type, EFILEPT_OPENR);
    STORE(buffer.efilep_head.seq, seq);
    STORE(buffer.efilep_openr.result, result);
    STORE(buffer.efilep_openr.fd, fd);
    deci2_send((uchar *)&buffer, sizeof(buffer));
}

static void
deci2_file_close(seq, fd)
    unsigned int seq;
    unsigned int fd;
{
    unsigned int result;
    struct {
	DECI2HEAD    deci2head;
	EFILEPHEAD   efilep_head;
	unsigned int result;
    } buffer;

    if (close(fd) < 0)
	result = deci2_file_errno();
    else
	result = 0;

    setup_deci2_header_for_efilep(&buffer.deci2head, sizeof(buffer));
    STORE(buffer.efilep_head.type, EFILEPT_CLOSER);
    STORE(buffer.efilep_head.seq, seq);
    STORE(buffer.result, result);
    deci2_send((uchar *)&buffer, sizeof(buffer));
}

static void
deci2_file_read(seq, fd, bytes)
    unsigned int seq;
    unsigned int fd;
    unsigned int bytes;
{
    int n;
    unsigned int result;
    int buf_size = DECI2HEAD_SZ + EFILEPHEAD_SZ + EFILEP_READR_SZ + bytes;
    uchar *buffer = xmalloc(buf_size);
    EFILEPHEAD *efilep_head = (EFILEPHEAD *)&buffer[DECI2HEAD_SZ];
    EFILEP_READR *efilep_readr = (EFILEP_READR *)&buffer[DECI2HEAD_SZ + EFILEPHEAD_SZ];

    if ((n = read(fd, &efilep_readr[1], bytes)) < 0)
	result = deci2_file_errno();
    else
	result = 0;

    setup_deci2_header_for_efilep((DECI2HEAD *)buffer, buf_size - (bytes - n));
    STORE(efilep_head->type, EFILEPT_READR);
    STORE(efilep_head->seq, seq);
    STORE(efilep_readr->result, result);
    STORE(efilep_readr->bytes, n);
    deci2_send(buffer, buf_size - (bytes - n));

    free(buffer);
}

static void
deci2_file_write(seq, fd, bytes, data)
    unsigned int seq;
    unsigned int fd;
    unsigned int bytes;
    unsigned char *data;
{
    int n;
    unsigned int result;
    int buf_size = DECI2HEAD_SZ + EFILEPHEAD_SZ + EFILEP_WRITER_SZ;
    uchar *buffer = xmalloc(buf_size);
    EFILEPHEAD *efilep_head = (EFILEPHEAD *)&buffer[DECI2HEAD_SZ];
    EFILEP_READR *efilep_writer = (EFILEP_WRITER *)&buffer[DECI2HEAD_SZ + EFILEPHEAD_SZ];

    if ((n = write(fd, data, bytes)) < 0)
	result = deci2_file_errno();
    else
	result = 0;

    setup_deci2_header_for_efilep((DECI2HEAD *)buffer, buf_size);
    STORE(efilep_head->type, EFILEPT_WRITER);
    STORE(efilep_head->seq, seq);
    STORE(efilep_writer->result, result);
    STORE(efilep_writer->bytes, n);
    deci2_send(buffer, buf_size);

    free(buffer);
}

static void
deci2_file_seek(seq, fd, offset, base)
    unsigned int seq;
    unsigned int fd;
    unsigned int offset;
    unsigned int base;
{
    int n;
    unsigned int result;
    int buf_size = DECI2HEAD_SZ + EFILEPHEAD_SZ + EFILEP_SEEKR_SZ;
    uchar *buffer = xmalloc(buf_size);
    EFILEPHEAD *efilep_head = (EFILEPHEAD *)&buffer[DECI2HEAD_SZ];
    EFILEP_SEEKR *efilep_seekr = (EFILEP_SEEKR *)&buffer[DECI2HEAD_SZ + EFILEPHEAD_SZ];
    int whence;

    switch (base) {
    case 0:
	whence = SEEK_SET;
	break;
    case 1:
	whence = SEEK_CUR;
	break;
    case 2:
	whence = SEEK_END;
	break;
    default:
	/* What should we do? */
    }

    if ((n = lseek(fd, offset, whence)) < 0)
	result = deci2_file_errno();
    else
	result = 0;

    setup_deci2_header_for_efilep((DECI2HEAD *)buffer, buf_size);
    STORE(efilep_head->type, EFILEPT_SEEKR);
    STORE(efilep_head->seq, seq);
    STORE(efilep_seekr->result, result);
    STORE(efilep_seekr->pos, n);
    deci2_send(buffer, buf_size);

    free(buffer);
}

static void
deci2_reset_cmd(arg, from_tty)
    char *arg;
    int from_tty;
{
    unsigned int len;
    struct {
	DECI2HEAD deci2head;
	NETMPHEAD netmp_head;
	short reserved;
	int ee_boot_param[2];
	int iop_boot_param[2];
    } buffer;

    if (!deci2_opened)
	error("Use the 'target deci2' command first.\n");
    buffer.ee_boot_param[0] = buffer.ee_boot_param[1] = -1;
    buffer.iop_boot_param[0] = buffer.iop_boot_param[1] = -1;
    setup_deci2_header_for_netmp((uchar *)&buffer.deci2head, sizeof(buffer));
    STORE(buffer.netmp_head.code, NETMP_CODE_RESET);
    STORE(buffer.netmp_head.result, 0);
    STORE(buffer.reserved, 0);
    deci2_send((uchar *)&buffer, sizeof(buffer));
    deci2_recv((uchar *)&buffer, &len);  /* get NETMP_CODE_RESETR. */
    deci2_recv((uchar *)&buffer, &len);  /* get DCMP_CODE_CONNECTED. */
    deci2_recv((uchar *)&buffer, &len);  /* get DCMP_CODE_CONNECTED. */
    flush_cached_frames();
    registers_changed();
    set_current_frame(create_new_frame(0, 0));
    program_loaded = 0;
    inferior_pid = 0;
}

static void
deci2_send(buf, len)
    char *buf;
    int len;
{
    if (write(deci2_fd, buf, len) < 0)
	perror_with_name("deci2_send - write");
}

#define READ(fd, buf, len)                                             \
{                                                                      \
    int l;                                                             \
    int read_len = 0;                                                  \
                                                                       \
    while (read_len != (len)) {                                        \
	if ((l = read((fd), &(buf)[read_len], (len) - read_len)) < 0)  \
	    perror_with_name("deci2_recv_with_timeout - read");        \
	read_len += l;                                                 \
    }                                                                  \
}

static void
deci2_recv_with_timeout(buf, len, timeout)
    unsigned char *buf;
    int *len;
    struct timeval *timeout;
{
    DECI2HEAD *deci2 = (DECI2HEAD *)buf;
    DCMPHEAD *dcmp_head;
    fd_set readmask;
    int n;
    unsigned char *recv_buf;
    int recv_len;
    int read_len, l;

 retry:
    FD_ZERO(&readmask);
    FD_SET(deci2_fd, &readmask);
    if ((n = select(FD_SETSIZE, &readmask, NULL, NULL, timeout)) < 0)
	if (errno == EINTR)
	    goto retry;
	else
	    perror_with_name("deci2_recv_with_timeout - select");
    if (!n)
	error("deci2_recv_with_timeout: timeout");
    if (!FD_ISSET(deci2_fd, &readmask))
	error("deci2_recv_with_timeout: unknown fd");

    READ(deci2_fd, buf, DECI2HEAD_SZ);
    recv_len = LOAD(deci2->length);
    recv_buf = xmalloc(recv_len - DECI2HEAD_SZ + 1);

    READ(deci2_fd, recv_buf, recv_len - DECI2HEAD_SZ);

    if (LOAD(deci2->protocol) == DECI2_PROTOCOL_ETTY) {
	recv_buf[recv_len - DECI2HEAD_SZ] = 0;
	printf_unfiltered(recv_buf + 4);
	free(recv_buf);
	goto retry;
    } else if (LOAD(deci2->protocol) == DECI2_PROTOCOL_EFILEP) {
	EFILEPHEAD *efilep_head = (EFILEPHEAD *)recv_buf;

	switch (efilep_head->type) {
	case EFILEPT_OPEN:
	    deci2_file_open(LOAD(efilep_head->seq),
			    LOAD(*(unsigned short int *)(recv_buf + 8)),
			    LOAD(*(unsigned short int *)(recv_buf + 10)),
			    (char *)(recv_buf + 12));
	    goto retry;
	case EFILEPT_CLOSE:
	    deci2_file_close(LOAD(*(unsigned int *)(recv_buf + 4)),
			     LOAD(*(unsigned int *)(recv_buf + 8)));
	    goto retry;
	case EFILEPT_READ:
	    deci2_file_read(LOAD(*(unsigned int *)(recv_buf + 4)),
			    LOAD(*(unsigned int *)(recv_buf + 8)),
			    LOAD(*(unsigned int *)(recv_buf + 12)));
	    goto retry;
	case EFILEPT_WRITE:
	    deci2_file_write(LOAD(*(unsigned int *)(recv_buf + 4)),
			     LOAD(*(unsigned int *)(recv_buf + 8)),
			     LOAD(*(unsigned int *)(recv_buf + 12)),
			     recv_buf + 16);
	    goto retry;
	case EFILEPT_SEEK:
	    deci2_file_seek(LOAD(*(unsigned int *)(recv_buf + 4)),
			     LOAD(*(unsigned int *)(recv_buf + 8)),
			     LOAD(*(unsigned int *)(recv_buf + 12)),
			     LOAD(*(unsigned int *)(recv_buf + 16)));
	    goto retry;
	default:
	    ;
	}
    } else if (LOAD(deci2->protocol) == DECI2_PROTOCOL_DCMP) {
	unsigned char code;

	dcmp_head = (DCMPHEAD *)recv_buf;
	code = LOAD(dcmp_head->code);
	if (LOAD(dcmp_head->type) == DCMP_TYPE_STATUS) {
	    if (code == DCMP_CODE_CONNECTED) {
		deci2_target_status.noconnect = 0;
	    } else {
		free(recv_buf);
		if (code == DCMP_CODE_PROTO) {
		    deci2_target_status.noproto = 0;
		    goto retry;
		} else if (code == DCMP_CODE_UNLOCKED) {
		    deci2_target_status.locked = 0;
		    goto retry;
		} else if (code == DCMP_CODE_SPACE) {
		    deci2_target_status.nospace = 0;
		    goto retry;
		} else if (code == DCMP_CODE_ROUTE) {
		    deci2_target_status.noroute = 0;
		    goto retry;
		} else
		    error("deci2_recv_with_timeout: invalid DCMP packet");
	    }
	} else if (LOAD(dcmp_head->type) == DCMP_TYPE_ERROR) {
	    free(recv_buf);
	    if (code == DCMP_CODE_NOPROTO) {
		deci2_target_status.noproto = 1;
		goto retry;
	    } else if (code == DCMP_CODE_LOCKED) {
		deci2_target_status.locked = 1;
		goto retry;
	    } else if (code == DCMP_CODE_NOSPACE) {
		deci2_target_status.nospace = 1;
		goto retry;
	    } else if (code == DCMP_CODE_NOROUTE) {
		deci2_target_status.noroute = 1;
		goto retry;
	    } else if (code == DCMP_CODE_NOCONNECT) {
		deci2_target_status.noconnect = 1;
		goto retry;
	    } else
		fatal("deci2_recv_with_timeout: received "
		      "DCMP_TYPE_ERROR message "
		      "(invalhead).");
	}
    } else if (LOAD(deci2->protocol) == DECI2_PROTOCOL_NETMP) {
	NETMPHEAD *netmp_head = (NETMPHEAD *)recv_buf;

	if (LOAD(netmp_head->code) == NETMP_CODE_RESETR) {
	    /* Check that RESETR message has extra code. */
	    if (recv_len == DECI2HEAD_SZ + NETMPHEAD_SZ + 1) {
		/* We ignore this message if a cause of reset is
		   IOP software restart. */
		if (recv_buf[DECI2HEAD_SZ + NETMPHEAD_SZ] == 0x00)
		    goto retry;
	    }
	} else
	    printf_filtered("Target hardware was reset.\n");
    }
    
    bcopy(recv_buf, (char *)&buf[DECI2HEAD_SZ], recv_len - DECI2HEAD_SZ);
    *len = recv_len;
    
    free(recv_buf);
}

static void
deci2_recv(buf, len)
    unsigned char *buf;
    int *len;
{
    struct timeval timeout;

    timeout.tv_sec = DECI2_DEFAULT_TIMEOUT;
    timeout.tv_usec = 0;
    deci2_recv_with_timeout(buf, len, &timeout);
}

static void
deci2_send_recv(snd_buf, snd_len, rcv_buf, rcv_len)
    unsigned char *snd_buf;
    int snd_len;
    unsigned char *rcv_buf;
    int *rcv_len;
{
    deci2_send(snd_buf, snd_len);
    deci2_recv(rcv_buf, rcv_len);
}

static void
deci2_get_target_conf()
{
    static struct {
	DECI2HEAD deci2head;
	DBGPHEAD  dbgp_head;
	DBGP_CONF dbgp_conf;
    } buffer;
    DBGPHEAD *dbgp_head;
    int recv_len;

    setup_deci2_header_for_dbgp((uchar *)&buffer, sizeof(buffer));

    dbgp_head = &buffer.dbgp_head;
    STORE(dbgp_head->id, DBGP_CPUID_CPU);
    STORE(dbgp_head->type, DBGP_TYPE_GETCONF);
    STORE(dbgp_head->code, 0);
    STORE(dbgp_head->result, 0);
    STORE(dbgp_head->count, 0);
    STORE(dbgp_head->unused, 0);

    deci2_target_conf = &buffer.dbgp_conf;

    deci2_send((uchar *)&buffer, sizeof(buffer));

    recv_len = sizeof(buffer);
    deci2_recv((uchar *)&buffer, &recv_len);
}

static void
deci2_init(arg)
    char *arg;
{
    char hostname[MAXHOSTNAMELEN + 1];
    char *port_num_str;
    char *colon_pos;
    unsigned long addr;
    u_short port;
    struct hostent *hp;
    struct sockaddr_in server;
    struct {
	DECI2HEAD deci2head;
	NETMPHEAD netmp_head;
	NETMP_CONNECT netmp_connect[3];
    } buffer;
    DECI2HEAD *deci2;
    NETMPHEAD *netmp_head;
    NETMP_CONNECT *netmp_connect;
    int len;
    int flag;

    deci2_opened = 1;
    deci2_target_status.noproto = 0;
    deci2_target_status.locked = 0;
    deci2_target_status.nospace = 0;
    deci2_target_status.noroute = 0;

    /* ARG points string which have a form like "host:port". */
    if (arg == NULL || !*arg) {
	/* Neither host or port are not specified.
	   Use default value. */
	if (gethostname(hostname, sizeof(hostname)) < 0)
	    fatal("internal error - invalid address is given to gethostname.");
	port_num_str = DECI2_DEFAULT_DSNETM_PORT;
    } else if ((colon_pos = strchr(arg, ':')) != NULL) {
	/* At least, hostname is specified. */
	strncpy(hostname, arg, colon_pos - arg);
	hostname[colon_pos - arg] = 0;
	if (*(colon_pos + 1) == 0)
	    /* There is no character after the colon. */
	    port_num_str = DECI2_DEFAULT_DSNETM_PORT;
	else
	    port_num_str = colon_pos + 1;
    } else {
	/* It is assumed that ARG represent only hostname or
	   the Internet standard '.' notation. */
	if (sizeof(hostname) < strlen(arg))
	    error("Invalid hostname.");
	strcpy(hostname, arg);
	port_num_str = DECI2_DEFAULT_DSNETM_PORT;
    }

    if ((addr = inet_addr(hostname)) == -1) {
	if ((hp = gethostbyname(hostname)) == NULL)
	    error("Invalid hostname.");
	addr =  *(unsigned long *)hp->h_addr;
    } else {
	if ((hp = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET)) == NULL)
	    error("Invalid address.");
    }

    if (sscanf(port_num_str, "%hu", &port) != 1 || !port)
	error("Invalid port number.");

    /* create a socket. */
    if ((deci2_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	perror_with_name("deci2_init - socket");
#ifdef NON_BLOCK
    if ((flag = fcntl(deci2_fd, F_GETFL, 0)) < 0)
	perror_with_name("deci2_init - fcntl(F_GETFL)");
    flag |= O_NONBLOCK;
    if (fcntl(deci2_fd, F_SETFL, flag) < 0)
	perror_with_name("deci2_init - fcntl(F_SETFL)");
#endif

    /* connect to the host. */
    bzero((char *)&server, sizeof(server));
    server.sin_addr.s_addr = addr;
    server.sin_family = hp->h_addrtype;
    server.sin_port = htons(port);
    if (connect(deci2_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
	error("deci2_init: can not connect to %s.", hostname);

    /* connect to the dsnetm. */
    deci2 = &buffer.deci2head;
    setup_deci2_header_for_netmp((uchar *)&buffer, sizeof(buffer));
    netmp_head = &buffer.netmp_head;
    STORE(netmp_head->code, NETMP_CODE_CONNECT);
    STORE(netmp_head->result, 0);
    netmp_connect = buffer.netmp_connect;
    setup_netmp_connect((uchar *)netmp_connect, DECI2_PROTOCOL_ESDBGP);
    setup_netmp_connect((uchar *)(netmp_connect + 1), DECI2_PROTOCOL_EFILEP);
    setup_netmp_connect((uchar *)(netmp_connect + 2), DECI2_PROTOCOL_ETTY);
    deci2_send((uchar *)&buffer, sizeof(buffer));

    /* receive reply */
    len = sizeof(buffer);
    deci2_recv((uchar *)&buffer, &len);
    if (LOAD(deci2->protocol) == DECI2_PROTOCOL_NETMP
	&& LOAD(netmp_head->code) == NETMP_CODE_CONNECTR) {
	switch (LOAD(netmp_head->result)) {
	case NETMP_ERR_ALREADYCONN:
	    error("deci2_init: already connected.");
	    /* FALLTHROUGH */
	case NETMP_ERR_OK:
	    break;
	case NETMP_ERR_BUSY:
	    error("deci2_init: manager program is busy.");
	    break;
	case NETMP_ERR_NOTCONN:
	    error("deci2_init: connection failed.");
	    break;
	case NETMP_ERR_NOMEM:
	    error("deci2_init: insufficient memory.");
	    break;
	case NETMP_ERR_NOPROTO:
	    error("deci2_init: no protocol.");
	    break;
	default:
	    error("deci2_init: invalid NETMP message code.");
	}
    } else {
	error("deci2_init: invalid packet.");
    }

    /* get target configuration */
    deci2_get_target_conf();

}

static void
deci2_free_all_resources()
{
    free(deci2_arg_buf);
    deci2_arg_buf = NULL;
}

static void
deci2_open(name, from_tty)
    char *name;
    int from_tty;
{
    target_preopen(from_tty);
    if (deci2_opened)
	unpush_target(&deci2_ops);
    deci2_init(name);
    push_target(&deci2_ops);
    tmp_mips_processor_type = "generic";
    mips_set_processor_type_command("generic", 0);
    flush_cached_frames();
    registers_changed();
    set_current_frame(create_new_frame(0, 0));
    printf_filtered("Connected to the target system.\n");
    printf_filtered("DBGP Version %d.%02d\n", CF_MAJOR_VER, CF_MINOR_VER);
    txvu_cpu = TXVU_CPU_MASTER;
    overlay_debugging = 1;  /* set overlay debugging state manual. */
}

static void
deci2_close(quiting)
    int quiting;
{
    deci2_opened = 0;
    program_loaded = 0;
    deci2_free_all_resources();
    close(deci2_fd);
}

static void
deci2_detach(args, from_tty)
    char *args;
    int from_tty;
{
    if (args)
	error("Argument given to \"detach\" when remotely debugging.");
    pop_target();
    if (from_tty)
	printf_unfiltered("Ending remote DECI2 debugging.\n");
}

static void
deci2_run()
{
    unsigned char *buffer;
    DBGPHEAD *dbgp_head;
    DBGP_RUN *dbgp_eerun;
    int buf_size;

    buf_size = DECI2HEAD_SZ + DBGPHEAD_SZ + DBGP_RUN_SZ + deci2_arg_buf_sz - sizeof(int);
    buffer = xmalloc(buf_size);
    setup_deci2_header_for_dbgp(buffer, buf_size);
    dbgp_head = (DBGPHEAD *)&buffer[DECI2HEAD_SZ];
    STORE(dbgp_head->id, DBGP_CPUID_CPU);
    STORE(dbgp_head->type, DBGP_TYPE_RUN);
    STORE(dbgp_head->code, 0);
    STORE(dbgp_head->result, 0);
    STORE(dbgp_head->count, 0);
    dbgp_eerun = (DBGP_RUN *)&buffer[DECI2HEAD_SZ + DBGPHEAD_SZ];
    STORE(dbgp_eerun->entry, bfd_get_start_address(exec_bfd));
    STORE(dbgp_eerun->gp, 0);
    STORE(dbgp_eerun->reserved1, 0);
    STORE(dbgp_eerun->reserved2, 0);
    bcopy(deci2_arg_buf, (char *)&(dbgp_eerun->argc), deci2_arg_buf_sz);
    deci2_send(buffer, buf_size);
    deci2_recv(buffer, &buf_size);

    free(buffer);
    use_run_instead_of_cont = 0;
}

static void
deci2_cont(step)
    int step;
{
    struct {
	DECI2HEAD deci2head;
	DBGPHEAD  dbgp_head;
    } buffer;
    DBGPHEAD *dbgp_head;
    int len;

    if (use_run_instead_of_cont) {
	deci2_run();
	return;
    }

    setup_deci2_header_for_dbgp((uchar *)&buffer, sizeof(buffer));
    dbgp_head = &buffer.dbgp_head;
    STORE(dbgp_head->id, DBGP_CPUID_CPU);
    STORE(dbgp_head->type, DBGP_TYPE_CONTINUE);
    STORE(dbgp_head->code, step ? DBGP_CODE_STEP : DBGP_CODE_CONT);
    STORE(dbgp_head->result, 0);
    STORE(dbgp_head->count, 1);
    deci2_send((uchar *)&buffer, sizeof(buffer));

    len = sizeof(buffer);
    deci2_recv((char *)&buffer, &len);  /* get CONTINUER */ 
}

static void
deci2_resume(pid, step, siggnal)
    int pid;
    int step;
    enum target_signal siggnal;
{
    if (inferior_pid != 42)
	error("The program is not being run.");

    if (siggnal != TARGET_SIGNAL_0 && siggnal != TARGET_SIGNAL_INT)
	warning("Can't send signal to a remote system. Try `handle %s ignore'.",
		target_signal_to_name(siggnal));

    deci2_cont(step);
}

static void
deci2_cntrl_c(signo)
    int signo;
{
    struct {
	DECI2HEAD deci2head;
	DBGPHEAD  dbgp_head;
    } buffer;
    DBGPHEAD *dbgp_head = &buffer.dbgp_head;
    unsigned int len;

    len = sizeof(buffer);
    setup_deci2_header_for_dbgp((uchar *)&buffer, len);
    STORE(dbgp_head->id, DBGP_CPUID_CPU);
    STORE(dbgp_head->type, DBGP_TYPE_BREAK);
    STORE(dbgp_head->code, 0);
    STORE(dbgp_head->result, 0);
    STORE(dbgp_head->count, 0);
    deci2_send((uchar *)&buffer, len);
}

/* Wait until the remote machine stops, then return,
   storing status in STATUS just as 'wait' would.
   Returns "pid" (though it's not clear what, if anything, that
   means in the case of this target). */

static int
deci2_wait(pid, status)
    int pid;
    struct target_waitstatus *status;
{
    struct {
	DECI2HEAD deci2head;
	DBGPHEAD  dbgp_head;
    } buffer;
    DBGPHEAD *dbgp_head = &buffer.dbgp_head;
    int recv_len;
    static RETSIGTYPE (*prev_sigint)();
    struct sigaction sa;
    struct sigaction osa;

    /* Set CTRL-C handler. */
    sa.sa_handler = deci2_cntrl_c;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, &osa);
    prev_sigint = osa.sa_handler;

    /* Wait until target send reply. */
    recv_len = sizeof(buffer);
    deci2_recv_with_timeout((char *)&buffer, &recv_len, NULL);

    /* Reset CTRL-C handler. */
    signal(SIGINT, prev_sigint);

    switch (LOAD(dbgp_head->result)) {
    case DBGP_RESULT_GOOD:
	/* This can not happen. */
	break;
    case DBGP_RESULT_INVALIREQ:
	error("deci2_wait: received DBGP_RESULT_INVALIREQ");
	break;
    case DBGP_RESULT_UNIMPREQ:
	error("deci2_wait: received DBGP_RESULT_UNIMPREQ");
	break;
    case DBGP_RESULT_ERROR:
	error("deci2_wait: received DBGP_RESULT_ERROR");
	break;
    case DBGP_RESULT_INVALCONT:
	error("deci2_wait: received DBGP_RESULT_INVALCONT");
	break;
    case DBGP_RESULT_TLBERR:
	error("deci2_wait: received DBGP_RESULT_TLBERR");
	break;
    case DBGP_RESULT_ADRERR:
    case DBGP_RESULT_BUSERR:
	status->kind = TARGET_WAITKIND_STOPPED;
	status->value.sig = TARGET_SIGNAL_BUS;
	break;
    case DBGP_RESULT_INVALSTATE:
	/* This can not happen because:
	   1. The gdb never send DBGP_TYPE_BREAK while target program
              is stopped.
	   2. The gdb never send DBGP_TYPE_CONTINUE while target program
              is running. */
	break;
    case DBGP_RESULT_BREAKED:
	status->kind = TARGET_WAITKIND_STOPPED;
	status->value.sig = TARGET_SIGNAL_INT;
	break;
    case DBGP_RESULT_BRKPT:
    case DBGP_RESULT_STEPNEXT:
	status->kind = TARGET_WAITKIND_STOPPED;
	status->value.sig = TARGET_SIGNAL_TRAP;
	break;
    case DBGP_RESULT_EXCEPTION:
	switch ((read_register(36) >> 2) & 0x1f) {
	case 1: /* TLB modification exception */
	case 2: /* TLB exception (load or instruction fetch) */
	case 3: /* TLB exception (store) */
	    status->kind = TARGET_WAITKIND_STOPPED;
	    status->value.sig = TARGET_SIGNAL_SEGV;
	    break;

	case 4: /* address error exception (load or instruction fetch) */
	case 5: /* address error exception (store) */
	    status->kind = TARGET_WAITKIND_STOPPED;
	    status->value.sig = TARGET_SIGNAL_BUS;
	    break;

	case 6: /* bus error exception (instruction fetch) */
	case 7: /* bus error exception (data reference : load or store) */
	    status->kind = TARGET_WAITKIND_STOPPED;
	    status->value.sig = TARGET_SIGNAL_BUS;
	    break;

	case 10: /* reserved instruction exception */
	    status->kind = TARGET_WAITKIND_STOPPED;
	    status->value.sig = TARGET_SIGNAL_ILL;
	    break;

	case 0: /* interrupt */
	    printf_filtered("\nException: Interrupt\n");
	    status->kind = TARGET_WAITKIND_STOPPED;
	    status->value.sig = TARGET_SIGNAL_USR1;
	    break;

	case 8: /* syscall exception */
	    printf_filtered("\nException: Syscall\n");
	    status->kind = TARGET_WAITKIND_STOPPED;
	    status->value.sig = TARGET_SIGNAL_USR1;
	    break;

	case 11: /* coprocessor unusable exception */
	    printf_filtered("\nException: Coprocessor unusable\n");
	    status->kind = TARGET_WAITKIND_STOPPED;
	    status->value.sig = TARGET_SIGNAL_USR1;
	    break;

	case 12: /* arithmetic overflow exception */
	    printf_filtered("\nException: Arithmetic overflow\n");
	    status->kind = TARGET_WAITKIND_STOPPED;
	    status->value.sig = TARGET_SIGNAL_USR1;
	    break;

	case 9: /* breakpoint exception */
	case 13: /* trap exception */
	    status->kind = TARGET_WAITKIND_STOPPED;
	    status->value.sig = TARGET_SIGNAL_TRAP;
	    break;
	default:
#if 0
	    printf_filtered("\n*** CAUSE register= 0x%x ***\n",
			    read_register(36));
#endif
	    status->kind = TARGET_WAITKIND_SIGNALLED;
	    status->value.sig = TARGET_SIGNAL_UNKNOWN;
	    break;
	}
	break;
    case DBGP_RESULT_PROGEND:
	status->kind = TARGET_WAITKIND_EXITED;
	status->value.integer = 0;  /* Fix me! */
	break;
    case DBGP_RESULT_BUSYSTATE:
	/* What does this mean? */
	break;
    case DBGP_RESULT_DEBUG_EXCEPTION:
	/* What does this mean? */
	break;
    default:
	/* This can not happen. */
    }

    return inferior_pid;
}

static int regkind_map[] = {
    /* zero   at        v0        v1        a0        a1        a2        a3 */
    DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR,
    /* t0     t1        t2        t3        t4        t5        t6        t7 */
    DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR,
    /* s0     s1        s2        s3        s4        s5        s6        s7 */
    DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR,
    /* t8     t9        k0        k1        gp        sp        s8        ra */
    DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR,
    /* sr     lo        hi        bad       cause     pc */
    DBEK_SCR, DBEK_HLS, DBEK_HLS, DBEK_SCR, DBEK_SCR, DBEK_SCR,
    /* f0     f1        f2        f3        f4        f5        f6        f7 */
    DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR,
    /* f8     f9        f10       f11       f12       f13       f14       f15 */
    DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR,
    /* f16    f17       f18       f19       f20       f21       f22       f23 */
    DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR,
    /* f24    f25       f26       f27       f28       f29       f30       f31 */
    DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR, DBEK_FPR,
    /* fsr    fir       fp        "" */
    DBEK_FPC, DBEK_FPC, DBEK_FPC, -1,
    /* ""     ""        ""        ""        ""        ""        ""        "" */
    -1,       -1,       -1,       -1,       -1,       -1,       -1,       -1,
    /* ""     ""        ""        ""        ""        ""        ""        "" */
    -1,       -1,       -1,       -1,       -1,       -1,       -1,       -1,
    /* zeroh  ath       v0h       v1h       a0h       a1h       a2h       a3h */
    DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR,
    /* t0h    t1h       t2h       t3h       t4h       t5h       t6h       t7h */
    DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR,
    /* s0h    s1h       s2h       s3h       s4h       s5h       s6h       s7h */
    DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR,
    /* t8h    t9h       k0h       k1h       gph       sph       s8h       rah */
    DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR, DBEK_GPR,
    /* loh    hih       sa        ""        ""        ""        index     random */
    DBEK_HLS, DBEK_HLS, DBEK_HLS, -1,       -1,       -1,       DBEK_SCR, DBEK_SCR,
    /* entlo0 entlo1    context   pgmsk     wired     ""        badva     count */
    DBEK_SCR, DBEK_SCR, DBEK_SCR, DBEK_SCR, DBEK_SCR, -1,       DBEK_SCR, DBEK_SCR,
    /* enthi  compare   ""        epc       ""	    prid      config      debug */
    DBEK_SCR, DBEK_SCR, -1,       DBEK_SCR, -1,     DBEK_SCR, DBEK_SCR,   DBEK_SCR,
    /* perf   taglo     taghi     err_epc */
    DBEK_SCR, DBEK_SCR, DBEK_SCR, DBEK_SCR,
    /* vu0_vi00		vu0_vi01	vu0_vi02	vu0_vi03 */
    DBEK_V0I,		DBEK_V0I,	DBEK_V0I,	DBEK_V0I,
    /* vu0_vi04		vu0_vi05	vu0_vi06	vu0_vi07 */
    DBEK_V0I,		DBEK_V0I,	DBEK_V0I,	DBEK_V0I,
    /* vu0_vi08		vu0_vi09	vu0_vi10	vu0_vi11 */
    DBEK_V0I,		DBEK_V0I,	DBEK_V0I,	DBEK_V0I,
    /* vu0_vi12		vu0_vi13	vu0_vi14	vu0_vi15 */
    DBEK_V0I,		DBEK_V0I,	DBEK_V0I,	DBEK_V0I,
    /* vu0_pc		vu0_tpc		vu0_stat	vu0_sflag */
    DBEK_V0I,		DBEK_V0I,	DBEK_V0I,	DBEK_V0I,
    /* vu0_mac		vu0_clip	vu0_cmsar	vu0_fbrst */
    DBEK_V0I,		DBEK_V0I,	DBEK_V0I,	DBEK_V0I,
    /* vu0_r		""		vu0_i		vu0_q */
    DBEK_V0I,		-1,		DBEK_V0I,	DBEK_V0I,
    /* vu0_accx		vu0_accy	vu0_accz	vu0_accw */
    -1,			-1,		-1,		-1,
    /* vu0_vf00x	vu0_vf00y	vu0_vf00z	vu0_vf00w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf01x	vu0_vf01y	vu0_vf01z	vu0_vf01w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf02x	vu0_vf02y	vu0_vf02z	vu0_vf02w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf03x	vu0_vf03y	vu0_vf03z	vu0_vf03w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf04x	vu0_vf04y	vu0_vf04z	vu0_vf04w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf05x	vu0_vf05y	vu0_vf05z	vu0_vf05w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf06x	vu0_vf06y	vu0_vf06z	vu0_vf06w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf07x	vu0_vf07y	vu0_vf07z	vu0_vf07w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf08x	vu0_vf08y	vu0_vf08z	vu0_vf08w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf09x	vu0_vf09y	vu0_vf09z	vu0_vf09w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf10x	vu0_vf10y	vu0_vf10z	vu0_vf10w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf11x	vu0_vf11y	vu0_vf11z	vu0_vf11w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf12x	vu0_vf12y	vu0_vf12z	vu0_vf12w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf13x	vu0_vf13y	vu0_vf13z	vu0_vf13w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf14x	vu0_vf14y	vu0_vf14z	vu0_vf14w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf15x	vu0_vf15y	vu0_vf15z	vu0_vf15w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf16x	vu0_vf16y	vu0_vf16z	vu0_vf16w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf17x	vu0_vf17y	vu0_vf17z	vu0_vf17w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf18x	vu0_vf18y	vu0_vf18z	vu0_vf18w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf19x	vu0_vf19y	vu0_vf19z	vu0_vf19w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf20x	vu0_vf20y	vu0_vf20z	vu0_vf20w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf21x	vu0_vf21y	vu0_vf21z	vu0_vf21w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf22x	vu0_vf22y	vu0_vf22z	vu0_vf22w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf23x	vu0_vf23y	vu0_vf23z	vu0_vf23w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf24x	vu0_vf24y	vu0_vf24z	vu0_vf24w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf25x	vu0_vf25y	vu0_vf25z	vu0_vf25w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf26x	vu0_vf26y	vu0_vf26z	vu0_vf26w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf27x	vu0_vf27y	vu0_vf27z	vu0_vf27w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf28x	vu0_vf28y	vu0_vf28z	vu0_vf28w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf29x	vu0_vf29y	vu0_vf29z	vu0_vf29w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf30x	vu0_vf30y	vu0_vf30z	vu0_vf30w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu0_vf31x	vu0_vf31y	vu0_vf31z	vu0_vf31w */
    DBEK_V0F,		DBEK_V0F,	DBEK_V0F,	DBEK_V0F,
    /* vu1_vi00		vu1_vi01	vu1_vi02	vu1_vi03 */
    DBEK_V1I,		DBEK_V1I,	DBEK_V1I,	DBEK_V1I,
    /* vu1_vi04		vu1_vi05	vu1_vi06	vu1_vi07 */
    DBEK_V1I,		DBEK_V1I,	DBEK_V1I,	DBEK_V1I,
    /* vu1_vi08		vu1_vi09	vu1_vi10	vu1_vi11 */
    DBEK_V1I,		DBEK_V1I,	DBEK_V1I,	DBEK_V1I,
    /* vu1_vi12		vu1_vi13	vu1_vi14	vu1_vi15 */
    DBEK_V1I,		DBEK_V1I,	DBEK_V1I,	DBEK_V1I,
    /* vu1_pc		vu1_tpc		vu1_stat	vu1_sflag */
    DBEK_V1I,		DBEK_V1I,	DBEK_V0I,	DBEK_V1I,
    /* vu1_mac		vu1_clip	vu1_cmsar	"" */
    DBEK_V1I,		DBEK_V1I,	DBEK_V0I,	DBEK_V1I,
    /* vu1_r		vu1_p		vu1_i		vu1_q */
    DBEK_V1I,		DBEK_V1I,	DBEK_V1I,	DBEK_V1I,
    /* vu1_accx		vu1_accy	vu1_accz	vu1_accw */
    -1,			-1,		-1,		-1,
    /* vu1_vf00x	vu1_vf00y	vu1_vf00z	vu1_vf00w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf01x	vu1_vf01y	vu1_vf01z	vu1_vf01w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf02x	vu1_vf02y	vu1_vf02z	vu1_vf02w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf03x	vu1_vf03y	vu1_vf03z	vu1_vf03w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf04x	vu1_vf04y	vu1_vf04z	vu1_vf04w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf05x	vu1_vf05y	vu1_vf05z	vu1_vf05w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf06x	vu1_vf06y	vu1_vf06z	vu1_vf06w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf07x	vu1_vf07y	vu1_vf07z	vu1_vf07w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf08x	vu1_vf08y	vu1_vf08z	vu1_vf08w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf09x	vu1_vf09y	vu1_vf09z	vu1_vf09w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf10x	vu1_vf10y	vu1_vf10z	vu1_vf10w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf11x	vu1_vf11y	vu1_vf11z	vu1_vf11w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf12x	vu1_vf12y	vu1_vf12z	vu1_vf12w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf13x	vu1_vf13y	vu1_vf13z	vu1_vf13w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf14x	vu1_vf14y	vu1_vf14z	vu1_vf14w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf15x	vu1_vf15y	vu1_vf15z	vu1_vf15w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf16x	vu1_vf16y	vu1_vf16z	vu1_vf16w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf17x	vu1_vf17y	vu1_vf17z	vu1_vf17w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf18x	vu1_vf18y	vu1_vf18z	vu1_vf18w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf19x	vu1_vf19y	vu1_vf19z	vu1_vf19w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf20x	vu1_vf20y	vu1_vf20z	vu1_vf20w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf21x	vu1_vf21y	vu1_vf21z	vu1_vf21w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf22x	vu1_vf22y	vu1_vf22z	vu1_vf22w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf23x	vu1_vf23y	vu1_vf23z	vu1_vf23w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf24x	vu1_vf24y	vu1_vf24z	vu1_vf24w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf25x	vu1_vf25y	vu1_vf25z	vu1_vf25w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf26x	vu1_vf26y	vu1_vf26z	vu1_vf26w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf27x	vu1_vf27y	vu1_vf27z	vu1_vf27w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf28x	vu1_vf28y	vu1_vf28z	vu1_vf28w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf29x	vu1_vf29y	vu1_vf29z	vu1_vf29w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf30x	vu1_vf30y	vu1_vf30z	vu1_vf30w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,
    /* vu1_vf31x	vu1_vf31y	vu1_vf31z	vu1_vf31w */
    DBEK_V1F,		DBEK_V1F,	DBEK_V1F,	DBEK_V1F,

    /* vif0_stat	vif0_fbrst	vif0_err	vif0_mark */
    -1,			-1,		-1,		-1,
    /* vif0_cycle	vif0_mode	vif0_num	vif0_mask */
    -1,			-1,		-1,		-1,
    /* vif0_code	vif0_itops	""		"" */
    -1,			-1,		-1,		-1,
    /* ""		vif0_itop	""		"" */
    -1,			-1,		-1,		-1,
    /* vif0_r0		vif0_r1		vif0_r2		vif0_r3 */
    -1,			-1,		-1,		-1,
    /* vif0_c0		vif0_c1		vif0_c2		vif0_c3 */
    -1,			-1,		-1,		-1,
    /* vif0_pc		vif0_pcx */
    -1,			-1,
    /* vif1_stat	vif1_fbrst	vif1_err	vif1_mark */
    -1,			-1,		-1,		-1,
    /* vif1_cycle	vif1_mode	vif1_num	vif1_mask */
    -1,			-1,		-1,		-1,
    /* vif1_code	vif1_itops	vif1_base	vif1_ofst */
    -1,			-1,		-1,		-1,
    /* vif1_tops	vif1_itop	vif1_top	vif1_dbf */
    -1,			-1,		-1,		-1,
    /* vif1_r0		vif1_r1		vif1_r2		vif1_r3 */
    -1,			-1,		-1,		-1,
    /* vif1_c0		vif1_c1		vif1_c2		vif1_c3 */
    -1,			-1,		-1,		-1,
    /* vif1_pc		vif1_pcx */
    -1,			-1
};

static int regno_map[] = {
    /* zero	at	v0	v1	a0	a1	a2	a3 */
    0,		1,	2,	3,	4,	5,	6,	7,
    /* t0	t1	t2	t3	t4	t5	t6	t7 */
    8,		9,	10,	11,	12,	13,	14,	15,
    /* s0	s1	s2	s3	s4	s5	s6	s7 */
    16,		17,	18,	19,	20,	21,	22,	23,
    /* t8	t9	k0	k1	gp	sp	s8	ra */
    24,		25,	26,	27,	28,	29,	30,	31,
    /* sr	lo	hi	bad	cause	pc */
    12,		DBEN_LO,DBEN_HI,8,	13,	14,
    /* f0	f1	f2	f3	f4	f5	f6	f7 */
    0,		1,	2,	3,	4,	5,	6,	7,
    /* f8	f9	f10	f11	f12	f13	f14	f15 */
    8,		9,	10,	11,	12,	13,	14,	15,
    /* f16	f17	f18	f19	f20	f21	f22	f23 */
    16,		17,	18,	19,	20,	21,	22,	23,
    /* f24	f25	f26	f27	f28	f29	f30	f31 */
    24,		25,	26,	27,	28,	29,	30,	31,
    /* fsr	fir	fp	"" */
    32,		33,	34,	-1,
    /* ""	""	""	""	""	""	""	"" */
    -1,		-1,	-1,	-1,	-1,	-1,	-1,	-1,
    /* ""	""	""	""	""	""	""	"" */
    -1,		-1,	-1,	-1,	-1,	-1,	-1,	-1,
    /* zeroh	ath	v0h	v1h	a0h	a1h	a2h	a3h */
    0,		1,	2,	3,	4,	5,	6,	7,
    /* t0h	t1h	t2h	t3h	t4h	t5h	t6h	t7h */
    8,		9,	10,	11,	12,	13,	14,	15,
    /* s0h	s1h	s2h	s3h	s4h	s5h	s6h	s7h */
    16,		17,	18,	19,	20,	21,	22,	23,
    /* t8h	t9h	k0h	k1h	gph	sph	s8h	rah */
    24,		25,	26,	27,	28,	29,	30,	31,
    /* loh	hih	sa	""	""	""	index	random */
    DBEN_LO1,	DBEN_HI1,DBEN_SA,-1,	-1,	-1,	0,	1,
    /* entlo0	entlo1	context	pgmsk	wired	""	badva	count */
    2,		3,	4,	5,	6,	-1,	8,	9,
    /* enthi	compare	""	epc	""	prid	config	debug */
    10,		11,	-1,	14,	-1,	15,	16,	24,
    /* perf	taglo	taghi	err_epc */
    25,		28,	29,	30,
    /* vu0_vi00		vu0_vi01	vu0_vi02	vu0_vi03 */
    0,			1,		2,		3,
    /* vu0_vi04		vu0_vi05	vu0_vi06	vu0_vi07 */
    4,			5,		6,		7,
    /* vu0_vi08		vu0_vi09	vu0_vi10	vu0_vi11 */
    8,			9,		10,		11,
    /* vu0_vi12		vu0_vi13	vu0_vi14	vu0_vi15 */
    12,			13,		14,		15,
    /* vu0_pc		vu0_tpc		vu0_stat	vu0_sflag */
    26,			26,		29,		16,
    /* vu0_mac		vu0_clip	vu0_cmsar	vu0_fbrst */
    17,			18,		27,		28,
    /* vu0_r		""		vu0_i		vu0_q */
    20,			-1,		21,		22,
    /* vu0_accx		vu0_accy	vu0_accz	vu0_accw */
    -1,			-1,		-1,		-1,
    /* vu0_vf00x	vu0_vf00y	vu0_vf00z	vu0_vf00w */
    0,			0,		0,		0,
    /* vu0_vf01x	vu0_vf01y	vu0_vf01z	vu0_vf01w */
    1,			1,		1,		1,
    /* vu0_vf02x	vu0_vf02y	vu0_vf02z	vu0_vf02w */
    2,			2,		2,		2,
    /* vu0_vf03x	vu0_vf03y	vu0_vf03z	vu0_vf03w */
    3,			3,		3,		3,
    /* vu0_vf04x	vu0_vf04y	vu0_vf04z	vu0_vf04w */
    4,			4,		4,		4,
    /* vu0_vf05x	vu0_vf05y	vu0_vf05z	vu0_vf05w */
    5,			5,		5,		5,
    /* vu0_vf06x	vu0_vf06y	vu0_vf06z	vu0_vf06w */
    6,			6,		6,		6,
    /* vu0_vf07x	vu0_vf07y	vu0_vf07z	vu0_vf07w */
    7,			7,		7,		7,
    /* vu0_vf08x	vu0_vf08y	vu0_vf08z	vu0_vf08w */
    8,			8,		8,		8,
    /* vu0_vf09x	vu0_vf09y	vu0_vf09z	vu0_vf09w */
    9,			9,		9,		9,
    /* vu0_vf10x	vu0_vf10y	vu0_vf10z	vu0_vf10w */
    10,			10,		10,		10,
    /* vu0_vf11x	vu0_vf11y	vu0_vf11z	vu0_vf11w */
    11,			11,		11,		11,
    /* vu0_vf12x	vu0_vf12y	vu0_vf12z	vu0_vf12w */
    12,			12,		12,		12,
    /* vu0_vf13x	vu0_vf13y	vu0_vf13z	vu0_vf13w */
    13,			13,		13,		13,
    /* vu0_vf14x	vu0_vf14y	vu0_vf14z	vu0_vf14w */
    14,			14,		14,		14,
    /* vu0_vf15x	vu0_vf15y	vu0_vf15z	vu0_vf15w */
    15,			15,		15,		15,
    /* vu0_vf16x	vu0_vf16y	vu0_vf16z	vu0_vf16w */
    16,			16,		16,		16,
    /* vu0_vf17x	vu0_vf17y	vu0_vf17z	vu0_vf17w */
    17,			17,		17,		17,
    /* vu0_vf18x	vu0_vf18y	vu0_vf18z	vu0_vf18w */
    18,			18,		18,		18,
    /* vu0_vf19x	vu0_vf19y	vu0_vf19z	vu0_vf19w */
    19,			19,		19,		19,
    /* vu0_vf20x	vu0_vf20y	vu0_vf20z	vu0_vf20w */
    20,			20,		20,		20,
    /* vu0_vf21x	vu0_vf21y	vu0_vf21z	vu0_vf21w */
    21,			21,		21,		21,
    /* vu0_vf22x	vu0_vf22y	vu0_vf22z	vu0_vf22w */
    22,			22,		22,		22,
    /* vu0_vf23x	vu0_vf23y	vu0_vf23z	vu0_vf23w */
    23,			23,		23,		23,
    /* vu0_vf24x	vu0_vf24y	vu0_vf24z	vu0_vf24w */
    24,			24,		24,		24,
    /* vu0_vf25x	vu0_vf25y	vu0_vf25z	vu0_vf25w */
    25,			25,		25,		25,
    /* vu0_vf26x	vu0_vf26y	vu0_vf26z	vu0_vf26w */
    26,			26,		26,		26,
    /* vu0_vf27x	vu0_vf27y	vu0_vf27z	vu0_vf27w */
    27,			27,		27,		27,
    /* vu0_vf28x	vu0_vf28y	vu0_vf28z	vu0_vf28w */
    28,			28,		28,		28,
    /* vu0_vf29x	vu0_vf29y	vu0_vf29z	vu0_vf29w */
    29,			29,		29,		29,
    /* vu0_vf30x	vu0_vf30y	vu0_vf30z	vu0_vf30w */
    30,			30,		30,		30,
    /* vu0_vf31x	vu0_vf31y	vu0_vf31z	vu0_vf31w */
    31,			31,		31,		31,
    /* vu1_vi00		vu1_vi01	vu1_vi02	vu1_vi03 */
    0,			1,		2,		3,
    /* vu1_vi04		vu1_vi05	vu1_vi06	vu1_vi07 */
    4,			5,		6,		7,
    /* vu1_vi08		vu1_vi09	vu1_vi10	vu1_vi11 */
    8,			9,		10,		11,
    /* vu1_vi12		vu1_vi13	vu1_vi14	vu1_vi15 */
    12,			13,		14,		15,
    /* vu1_pc		vu1_tpc		vu1_stat	vu1_sflag */
    26,			26,		29,		16,
    /* vu1_mac		vu1_clip	vu1_cmsar	"" */
    17,			18,		31,		-1,
    /* vu1_r		vu1_p		vu1_i		vu1_q */
    20,			23,		21,		22,
    /* vu1_accx		vu1_accy	vu1_accz	vu1_accw */
    -1,			-1,		-1,		-1,
    /* vu1_vf00x	vu1_vf00y	vu1_vf00z	vu1_vf00w */
    0,			0,		0,		0,
    /* vu1_vf01x	vu1_vf01y	vu1_vf01z	vu1_vf01w */
    1,			1,		1,		1,
    /* vu1_vf02x	vu1_vf02y	vu1_vf02z	vu1_vf02w */
    2,			2,		2,		2,
    /* vu1_vf03x	vu1_vf03y	vu1_vf03z	vu1_vf03w */
    3,			3,		3,		3,
    /* vu1_vf04x	vu1_vf04y	vu1_vf04z	vu1_vf04w */
    4,			4,		4,		4,
    /* vu1_vf05x	vu1_vf05y	vu1_vf05z	vu1_vf05w */
    5,			5,		5,		5,
    /* vu1_vf06x	vu1_vf06y	vu1_vf06z	vu1_vf06w */
    6,			6,		6,		6,
    /* vu1_vf07x	vu1_vf07y	vu1_vf07z	vu1_vf07w */
    7,			7,		7,		7,
    /* vu1_vf08x	vu1_vf08y	vu1_vf08z	vu1_vf08w */
    8,			8,		8,		8,
    /* vu1_vf09x	vu1_vf09y	vu1_vf09z	vu1_vf09w */
    9,			9,		9,		9,
    /* vu1_vf10x	vu1_vf10y	vu1_vf10z	vu1_vf10w */
    10,			10,		10,		10,
    /* vu1_vf11x	vu1_vf11y	vu1_vf11z	vu1_vf11w */
    11,			11,		11,		11,
    /* vu1_vf12x	vu1_vf12y	vu1_vf12z	vu1_vf12w */
    12,			12,		12,		12,
    /* vu1_vf13x	vu1_vf13y	vu1_vf13z	vu1_vf13w */
    13,			13,		13,		13,
    /* vu1_vf14x	vu1_vf14y	vu1_vf14z	vu1_vf14w */
    14,			14,		14,		14,
    /* vu1_vf15x	vu1_vf15y	vu1_vf15z	vu1_vf15w */
    15,			15,		15,		15,
    /* vu1_vf16x	vu1_vf16y	vu1_vf16z	vu1_vf16w */
    16,			16,		16,		16,
    /* vu1_vf17x	vu1_vf17y	vu1_vf17z	vu1_vf17w */
    17,			17,		17,		17,
    /* vu1_vf18x	vu1_vf18y	vu1_vf18z	vu1_vf18w */
    18,			18,		18,		18,
    /* vu1_vf19x	vu1_vf19y	vu1_vf19z	vu1_vf19w */
    19,			19,		19,		19,
    /* vu1_vf20x	vu1_vf20y	vu1_vf20z	vu1_vf20w */
    20,			20,		20,		20,
    /* vu1_vf21x	vu1_vf21y	vu1_vf21z	vu1_vf21w */
    21,			21,		21,		21,
    /* vu1_vf22x	vu1_vf22y	vu1_vf22z	vu1_vf22w */
    22,			22,		22,		22,
    /* vu1_vf23x	vu1_vf23y	vu1_vf23z	vu1_vf23w */
    23,			23,		23,		23,
    /* vu1_vf24x	vu1_vf24y	vu1_vf24z	vu1_vf24w */
    24,			24,		24,		24,
    /* vu1_vf25x	vu1_vf25y	vu1_vf25z	vu1_vf25w */
    25,			25,		25,		25,
    /* vu1_vf26x	vu1_vf26y	vu1_vf26z	vu1_vf26w */
    26,			26,		26,		26,
    /* vu1_vf27x	vu1_vf27y	vu1_vf27z	vu1_vf27w */
    27,			27,		27,		27,
    /* vu1_vf28x	vu1_vf28y	vu1_vf28z	vu1_vf28w */
    28,			28,		28,		28,
    /* vu1_vf29x	vu1_vf29y	vu1_vf29z	vu1_vf29w */
    29,			29,		29,		29,
    /* vu1_vf30x	vu1_vf30y	vu1_vf30z	vu1_vf30w */
    30,			30,		30,		30,
    /* vu1_vf31x	vu1_vf31y	vu1_vf31z	vu1_vf31w */
    31,			31,		31,		31,

    /* vif0_stat	vif0_fbrst	vif0_err	vif0_mark */
    -1,			-1,		-1,		-1,
    /* vif0_cycle	vif0_mode	vif0_num	vif0_mask */
    -1,			-1,		-1,		-1,
    /* vif0_code	vif0_itops	""		"" */
    -1,			-1,		-1,		-1,
    /* ""		vif0_itop	""		"" */
    -1,			-1,		-1,		-1,
    /* vif0_r0		vif0_r1		vif0_r2		vif0_r3 */
    -1,			-1,		-1,		-1,
    /* vif0_c0		vif0_c1		vif0_c2		vif0_c3 */
    -1,			-1,		-1,		-1,
    /* vif0_pc		vif0_pcx */
    -1,			-1,
    /* vif1_stat	vif1_fbrst	vif1_err	vif1_mark */
    -1,			-1,		-1,		-1,
    /* vif1_cycle	vif1_mode	vif1_num	vif1_mask */
    -1,			-1,		-1,		-1,
    /* vif1_code	vif1_itops	vif1_base	vif1_ofst */
    -1,			-1,		-1,		-1,
    /* vif1_tops	vif1_itop	vif1_top	vif1_dbf */
    -1,			-1,		-1,		-1,
    /* vif1_r0		vif1_r1		vif1_r2		vif1_r3 */
    -1,			-1,		-1,		-1,
    /* vif1_c0		vif1_c1		vif1_c2		vif1_c3 */
    -1,			-1,		-1,		-1,
    /* vif1_pc		vif1_pcx */
    -1,			-1
};

unsigned short processor_map[] =
{
    DBGP_CPUID_CPU,  /* DBEK_GPR */
    DBGP_CPUID_CPU,  /* DBEK_HLS */
    DBGP_CPUID_CPU,  /* DBEK_SCR */
    DBGP_CPUID_CPU,  /* DBEK_PCR */
    DBGP_CPUID_CPU,  /* DBEK_HDR */
    DBGP_CPUID_CPU,  /* DBEK_FPR */
    DBGP_CPUID_CPU,  /* DBEK_FPC */
    DBGP_CPUID_VU0,  /* DBEK_V0F */
    DBGP_CPUID_VU0,  /* DBEK_V0I */
    DBGP_CPUID_VU1,  /* DBEK_V1F */
    DBGP_CPUID_VU1   /* DBEK_V1I */
};

/* Read the remote registers into the block REGS. */

static void
deci2_fetch_registers(regno)
    int regno;
{
    struct {
	DECI2HEAD deci2head;
	DBGPHEAD  dbgp_head;
	DBGP_REG  dbgp_reg;
	uchar     reg[MIPS_REGSIZE * 2];
    } buffer;
    DBGPHEAD *dbgp_head = &buffer.dbgp_head;
    DBGP_REG *dbgp_reg = &buffer.dbgp_reg;
    unsigned char regkind;
    int i;
    int recv_len;

    if (regno == -1) {
	for (i = 0; i < NUM_REGS; ++i)
	    deci2_fetch_registers(i);
	return;
    }

    if (regno_map[regno] == -1) {
	register_valid[regno] = 1;
	return;
    }

    regkind = regkind_map[regno];
    
    setup_deci2_header_for_dbgp((uchar *)&buffer, sizeof(buffer));
    STORE(dbgp_head->id, processor_map[regkind]);
    STORE(dbgp_head->type, DBGP_TYPE_GETREG);
    STORE(dbgp_head->code, 0);
    STORE(dbgp_head->result, 0);
    STORE(dbgp_head->count, 1);
    STORE(dbgp_reg->kind, regkind);
    STORE(dbgp_reg->number, regno_map[regno]);
    memset(buffer.reg, 0, sizeof(buffer.reg));
    deci2_send((uchar *)&buffer, sizeof(buffer));
    recv_len = sizeof(buffer);
    deci2_recv((uchar *)&buffer, &recv_len);

    /* Set RA zero to stop stack trace if target program is not start. */
    if (inferior_pid != 42 && (regno == 31 || regno == 121)) {
	memset(buffer.reg, 0, sizeof(buffer.reg));
	supply_register(regno, buffer.reg);
	return;
    }

    if (regno == 37 || regno == 141) {
	/* if REG is EPC then set PC also and vice versa. */
	supply_register(37, buffer.reg);
	supply_register(141, buffer.reg);
    } else if (regno >= NUM_CORE_REGS + FIRST_VEC_REG
	       && regno < NUM_CORE_REGS + FIRST_VEC_REG + 128)
	/* vector register of VU0. */
	supply_register(regno, buffer.reg + ((regno - 2) % 4) * sizeof(int));
    else if (regno >= NUM_CORE_REGS + NUM_VU_REGS + FIRST_VEC_REG
	     && regno < NUM_CORE_REGS + NUM_VU_REGS + FIRST_VEC_REG + 128)
	/* vector register of VU1. */
	supply_register(regno, buffer.reg + ((regno - 2) % 4) * sizeof(int));
    else
	supply_register(regno, buffer.reg);
}

/* Prepare to store registers. */
static void
deci2_prepare_to_store()
{
    /* Nothing to do. */
}

static int
load_mem(addr, ptr, len)
    u_long addr;
    char *ptr;
    int len;
{
    unsigned char *buffer;
    unsigned char *data_area;
    DBGPHEAD *dbgp_head;
    DBGP_MEM *dbgp_mem;
    int buf_size;
    int data_area_size;
    int recv_len;

    if (CF_MEM_LIMIT_ALIGN & ALIGN_BYTE)
	buf_size = CF_MEM_LIMIT_SIZE;
    else
	buf_size = DECI2_MAX_PACKET_SIZE;
    buffer = xmalloc(buf_size);
    data_area_size = buf_size - DECI2HEAD_SZ - DBGPHEAD_SZ - DBGP_MEM_SZ;
    dbgp_head = (DBGPHEAD *)(buffer + DECI2HEAD_SZ);
    dbgp_mem = (DBGP_MEM *)(buffer + DECI2HEAD_SZ + DBGPHEAD_SZ);
    data_area = ((char *)dbgp_mem) + DBGP_MEM_SZ;

    while (len >= data_area_size) {
	setup_deci2_header_for_dbgp(buffer, buf_size);

	STORE(dbgp_head->id, DBGP_CPUID_CPU);
	STORE(dbgp_head->type, DBGP_TYPE_RDMEM);
	STORE(dbgp_head->code, 0);
	STORE(dbgp_head->result, 0);
	STORE(dbgp_head->count, 1);
	STORE(dbgp_mem->space, 0);
	STORE(dbgp_mem->align, 0); /*(CF_MEM_ALIGN & ALIGN_WORD) ? ALIGN_WORD : ALIGN_BYTE);*/
	STORE(dbgp_mem->reserved, 0);
	STORE(dbgp_mem->length, data_area_size);
	STORE(dbgp_mem->address, addr);

	deci2_send(buffer, buf_size);
	recv_len = buf_size;
	deci2_recv(buffer, &recv_len);
	bcopy(data_area, ptr, data_area_size);
	len -= data_area_size;
	ptr += data_area_size;
	addr += data_area_size;
    }
    if (len) {
	buf_size = DECI2HEAD_SZ + DBGPHEAD_SZ + DBGP_MEM_SZ + len;
	setup_deci2_header_for_dbgp(buffer, buf_size);

	STORE(dbgp_head->id, DBGP_CPUID_CPU);
	STORE(dbgp_head->type, DBGP_TYPE_RDMEM);
	STORE(dbgp_head->code, 0);
	STORE(dbgp_head->result, 0);
	STORE(dbgp_head->count, 1);
	STORE(dbgp_mem->space, 0);
	STORE(dbgp_mem->align, 0); /*(CF_MEM_ALIGN & ALIGN_WORD) ? ALIGN_WORD : ALIGN_BYTE);*/
	STORE(dbgp_mem->reserved, 0);
	STORE(dbgp_mem->length, len);
	STORE(dbgp_mem->address, addr);

	deci2_send(buffer, buf_size);
	recv_len = buf_size;
	deci2_recv(buffer, &recv_len);
	bcopy(data_area, ptr, len);
    }
    free(buffer);

    return 0;
}

static int
store_mem(addr, ptr, len)
    u_long addr;
    char *ptr;
    int len;
{
    unsigned char *buffer;
    unsigned char *data_area;
    DBGPHEAD *dbgp_head;
    DBGP_MEM *dbgp_mem;
    int buf_size;
    int data_area_size;
/*    unsigned char recv_buf[DECI2HEAD_SZ + DBGPHEAD_SZ + DBGP_MEM_SZ]; */
    struct {
	DECI2HEAD deci2head;
	DBGPHEAD  dbgp_head;
	DBGP_MEM  dbgp_mem;
    } recv_buf;
    int recv_len;

    if (CF_MEM_LIMIT_ALIGN & ALIGN_BYTE)
	buf_size = CF_MEM_LIMIT_SIZE;
    else
	buf_size = DECI2_MAX_PACKET_SIZE;
    buffer = xmalloc(buf_size);
    data_area_size = buf_size - DECI2HEAD_SZ - DBGPHEAD_SZ - DBGP_MEM_SZ;

    dbgp_head = (DBGPHEAD *)(buffer + DECI2HEAD_SZ);
    dbgp_mem = (DBGP_MEM *)(buffer + DECI2HEAD_SZ + DBGPHEAD_SZ);
    data_area = ((char *)dbgp_mem) + DBGP_MEM_SZ;

    while (len >= data_area_size) {
	setup_deci2_header_for_dbgp(buffer, buf_size);
	STORE(dbgp_head->id, DBGP_CPUID_CPU);
	STORE(dbgp_head->type, DBGP_TYPE_WRMEM);
	STORE(dbgp_head->code, 0);
	STORE(dbgp_head->result, 0);
	STORE(dbgp_head->count, 1);
	STORE(dbgp_mem->space, 0);
	STORE(dbgp_mem->align, 0); /*(CF_MEM_ALIGN & ALIGN_WORD) ? ALIGN_WORD : ALIGN_BYTE);*/
	STORE(dbgp_mem->reserved, 0);
	STORE(dbgp_mem->length, data_area_size);
	STORE(dbgp_mem->address, addr);

	bcopy(ptr, data_area, data_area_size);
	deci2_send(buffer, buf_size);
	recv_len = sizeof(recv_buf);
	deci2_recv((uchar *)&recv_buf, &recv_len);

	len -= data_area_size;
	ptr += data_area_size;
	addr += data_area_size;
    }
    if (len) {
	buf_size = DECI2HEAD_SZ + DBGPHEAD_SZ + DBGP_MEM_SZ + len;
	setup_deci2_header_for_dbgp(buffer, buf_size);
	STORE(dbgp_head->id, DBGP_CPUID_CPU);
	STORE(dbgp_head->type, DBGP_TYPE_WRMEM);
	STORE(dbgp_head->code, 0);
	STORE(dbgp_head->result, 0);
	STORE(dbgp_head->count, 1);
	STORE(dbgp_mem->space, 0);
	STORE(dbgp_mem->align, 0); /*(CF_MEM_ALIGN & ALIGN_WORD) ? ALIGN_WORD : ALIGN_BYTE);*/
	STORE(dbgp_mem->reserved, 0);
	STORE(dbgp_mem->length, len);
	STORE(dbgp_mem->address, addr);

	bcopy(ptr, data_area, len);
	deci2_send(buffer, buf_size);
	recv_len = sizeof(recv_buf);
	deci2_recv((uchar *)&recv_buf, &recv_len);
    }
    free(buffer);

    return 0;
}

/* Read or write LEN bytes from inferior memory at MEMADDR, transferring
   to or from debugger address MYADDR. Write to inferior if SHOULD_WRITE is
   nonzero. Returns length of data written or read; 0 for error. */
static int
deci2_xfer_memory(memaddr, myaddr, len, write, target)
	CORE_ADDR memaddr;
	char *myaddr;
	int len;
	int write;
	struct target_ops *target;
{
    if (write) {
	if (store_mem(memaddr, myaddr, len))
	    return(0);
    } else {
	if (load_mem(memaddr, myaddr, len))
	    return(0);
    }
    return(len);
}

static void
deci2_get_vf_reg(regno, proc_id, reg)
    int regno;
    unsigned short proc_id;
    char *reg;
{
    struct {
	DECI2HEAD deci2head;
	DBGPHEAD  dbgp_head;
	DBGP_REG  dbgp_reg;
	uchar     reg[MIPS_REGSIZE * 2];
    } buffer;
    DBGPHEAD *dbgp_head = &buffer.dbgp_head;
    DBGP_REG *dbgp_reg = &buffer.dbgp_reg;
    int recv_len;

    setup_deci2_header_for_dbgp((uchar *)&buffer, sizeof(buffer));
    STORE(dbgp_head->id, proc_id);
    STORE(dbgp_head->type, DBGP_TYPE_GETREG);
    STORE(dbgp_head->code, 0);
    STORE(dbgp_head->result, 0);
    STORE(dbgp_head->count, 1);
    STORE(dbgp_reg->kind, proc_id == DBGP_CPUID_VU0 ? DBEK_V0F : DBEK_V1F);
    STORE(dbgp_reg->number, regno);
    deci2_send((uchar *)&buffer, sizeof(buffer));
    recv_len = sizeof(buffer);
    deci2_recv((uchar *)&buffer, &recv_len);
    memcpy(reg, buffer.reg, sizeof(buffer.reg));
}

/* Store register REGNO, or all registers if REGNO == -1, from the contents
   of REGISTERS. */
static void
deci2_store_registers(regno)
    int regno;
{
    struct {
	DECI2HEAD deci2head;
	DBGPHEAD  dbgp_head;
	DBGP_REG  dbgp_reg;
	uchar     reg[MIPS_REGSIZE * 2];
    } buffer;
    DBGPHEAD *dbgp_head = &buffer.dbgp_head;
    DBGP_REG *dbgp_reg = &buffer.dbgp_reg;
    unsigned char regkind;
    int i;
    int recv_len;

    if (regno == -1) {
	for (i = 0; i < NUM_REGS; ++i)
	    deci2_store_registers(i);
	return;
    }

    if (regno_map[regno] == -1)
	return;

    regkind = regkind_map[regno];
    
    setup_deci2_header_for_dbgp((uchar *)&buffer, sizeof(buffer));
    STORE(dbgp_head->id, processor_map[regkind]);
    STORE(dbgp_head->type, DBGP_TYPE_PUTREG);
    STORE(dbgp_head->code, 0);
    STORE(dbgp_head->result, 0);
    STORE(dbgp_head->count, 1);
    STORE(dbgp_reg->kind, regkind);
    STORE(dbgp_reg->number, regno_map[regno]);
    if (regno >= NUM_CORE_REGS + FIRST_VEC_REG
	      && regno < NUM_CORE_REGS + FIRST_VEC_REG + 128) {
	/* vector register of VU0. */
	deci2_get_vf_reg(regno_map[regno], DBGP_CPUID_VU0, buffer.reg);
	read_register_gen(regno, buffer.reg + ((regno - 2) % 4) * sizeof(int));
    } else if (regno >= NUM_CORE_REGS + NUM_VU_REGS + FIRST_VEC_REG
	     && regno < NUM_CORE_REGS + NUM_VU_REGS + FIRST_VEC_REG + 128) {
	/* vector register of VU1. */
	deci2_get_vf_reg(regno_map[regno], DBGP_CPUID_VU1, buffer.reg);
	read_register_gen(regno, buffer.reg + ((regno - 2) % 4) * sizeof(int));
    } else
	read_register_gen(regno, buffer.reg);

    deci2_send((uchar *)&buffer, sizeof(buffer));
    recv_len = sizeof(buffer);
    deci2_recv((uchar *)&buffer, &recv_len);
}

static void
deci2_files_info(ignore)
    struct target_ops *ignore;
{
    puts_filtered("Debugging a target using DECI2 protocol.\n");
}

/* Set breakpoint at address ADDR. We does not use CONTENTS_CACHE
   because DECI2 protocol have mechanism to put/get breakpoint. */
static int
deci2_insert_breakpoint(addr, contents_cache)
    CORE_ADDR addr;
    char *contents_cache;
{
    unsigned char *buffer;
    DBGPHEAD *dbgp_head;
    int buf_len;
    int addr_part_len;

    /* Get all breakpoints. */
    buf_len = DECI2HEAD_SZ + DBGPHEAD_SZ + sizeof(CORE_ADDR) * CF_NBRKPT * 2;
    buffer = xmalloc(buf_len);
    setup_deci2_header_for_dbgp(buffer, DECI2HEAD_SZ + DBGPHEAD_SZ);
    dbgp_head = (DBGPHEAD *)&buffer[DECI2HEAD_SZ];
    STORE(dbgp_head->id, DBGP_CPUID_CPU);
    STORE(dbgp_head->type, DBGP_TYPE_GETBRKPT);
    STORE(dbgp_head->code, 0);
    STORE(dbgp_head->result, 1);
    STORE(dbgp_head->count, CF_NBRKPT);
    deci2_send(buffer, DECI2HEAD_SZ + DBGPHEAD_SZ);
    deci2_recv(buffer, &buf_len);

    if (LOAD(dbgp_head->count) == CF_NBRKPT) {
	error("too many break point");
	return -1;
    }

    /* Set all breakpoints. */
    addr_part_len = sizeof(int) * LOAD(dbgp_head->count) * 2;
    buf_len = DECI2HEAD_SZ + DBGPHEAD_SZ + addr_part_len + sizeof(int) * 2;
    setup_deci2_header_for_dbgp(buffer, buf_len);
    STORE(*(int *)&buffer[DECI2HEAD_SZ + DBGPHEAD_SZ + addr_part_len], addr);
    STORE(*(int *)&buffer[DECI2HEAD_SZ + DBGPHEAD_SZ + addr_part_len + 4], 1);
    STORE(dbgp_head->id, DBGP_CPUID_CPU);
    STORE(dbgp_head->type, DBGP_TYPE_PUTBRKPT);
    STORE(dbgp_head->code, 0);
    STORE(dbgp_head->result, 1);
    STORE(dbgp_head->count, LOAD(dbgp_head->count) + 1);
    deci2_send(buffer, buf_len);
    deci2_recv(buffer, &buf_len);
    free(buffer);

    return 0;
}

static int
deci2_remove_breakpoint(addr, contents_cache)
    CORE_ADDR addr;
    char *contents_cache;
{
    unsigned char *buffer;
    DBGPHEAD *dbgp_head;
    int buf_len;
    int *p, *q;
    int i;
    int no_of_brkpt;

    /* Get all breakpoints. */
    buf_len = DECI2HEAD_SZ + DBGPHEAD_SZ + sizeof(CORE_ADDR) * CF_NBRKPT * 2;
    buffer = xmalloc(buf_len);
    setup_deci2_header_for_dbgp(buffer, buf_len);
    dbgp_head = (DBGPHEAD *)&buffer[DECI2HEAD_SZ];
    STORE(dbgp_head->id, DBGP_CPUID_CPU);
    STORE(dbgp_head->type, DBGP_TYPE_GETBRKPT);
    STORE(dbgp_head->code, 0);
    STORE(dbgp_head->result, 1);
    STORE(dbgp_head->count, CF_NBRKPT);
    deci2_send(buffer, buf_len);
    deci2_recv(buffer, &buf_len);
    no_of_brkpt = LOAD(dbgp_head->count);

    /* Delete all breakpoints. */
    buf_len = DECI2HEAD_SZ + DBGPHEAD_SZ;
    setup_deci2_header_for_dbgp(buffer, DECI2HEAD_SZ + DBGPHEAD_SZ);
    STORE(dbgp_head->id, DBGP_CPUID_CPU);
    STORE(dbgp_head->type, DBGP_TYPE_PUTBRKPT);
    STORE(dbgp_head->code, 0);
    STORE(dbgp_head->result, 1);
    STORE(dbgp_head->count, 0);
    deci2_send(buffer, buf_len);
    deci2_recv(buffer, &buf_len);

    /* delete breakpoint which address is 'addr'. */
    p = (int *)&buffer[DECI2HEAD_SZ + DBGPHEAD_SZ];
    for (i = 0; i < no_of_brkpt; ++i) {
	if (addr == LOAD(*p)) {
	    for (q = p + 2; i < no_of_brkpt * 2; ++ i, ++ p, ++ q)
		STORE(*p, *q);
	    break;
	}
	p += 2;
    }
    -- no_of_brkpt;

    /* Nothing to do if there is no breakpoint. */
    if (! no_of_brkpt) {
	free(buffer);
	return 0;
    }

    /* Set rest of breakpoints. */
    buf_len = DECI2HEAD_SZ + DBGPHEAD_SZ + sizeof(CORE_ADDR) * no_of_brkpt * 2;
    setup_deci2_header_for_dbgp(buffer, buf_len);
    STORE(dbgp_head->id, DBGP_CPUID_CPU);
    STORE(dbgp_head->type, DBGP_TYPE_PUTBRKPT);
    STORE(dbgp_head->code, 0);
    STORE(dbgp_head->result, 1);
    STORE(dbgp_head->count, no_of_brkpt);
    deci2_send(buffer, buf_len);
    deci2_recv(buffer, &buf_len);
    free(buffer);

    return 0;
}

static void
deci2_kill()
{
    inferior_pid = 0;
}

static void
deci2_load(prog, fromtty)
    char *prog;
    int fromtty;
{
    generic_load(prog, fromtty);
    program_loaded = 1;
    inferior_pid = 0;
}

static void
deci2_create_inferior(exec_file, args, env)
    char *exec_file;
    char *args;
    char **env;
{
    CORE_ADDR entry_pt;

    if (exec_file == 0 || exec_bfd == 0)
	error("No executable file specified.");
    if (! program_loaded)
	error("No program loaded.");
    entry_pt = (CORE_ADDR)bfd_get_start_address(exec_bfd);
    use_run_instead_of_cont = 1;
    build_argv(exec_file, args);
    deci2_kill();
    remove_breakpoints();
    init_wait_for_inferior();
    inferior_pid = 42;
    proceed(entry_pt, TARGET_SIGNAL_DEFAULT, 0);
}

static void
deci2_mourn_inferior()
{
    unpush_target(&deci2_ops);
    generic_mourn_inferior();
}

static void
deci2_stop()
{
    if (kill(getpid(), SIGINT) < 0)
	perror_with_name("deci2_stop - kill");
}

static struct target_ops deci2_ops = {
    "deci2",	  		/* to_shortname */
    "Remote PSX2 debugging using the DECI2 protocol",	/* to_longname */
    "Debug using the DECI2 protocol for the PSX2 target.\n\
The argument is hostname of dsnetm it is connected to.",  /* to_doc */
    deci2_open,			/* to_open                               */
    deci2_close,		/* to_close                              */
    NULL,			/* to_attach                             */
    NULL,			/* to_post_attach                        */
    NULL,			/* to_require_attach                     */
    deci2_detach,		/* to_detach                             */
    NULL,			/* to_require_detach                     */
    deci2_resume,		/* to_resume                             */
    deci2_wait,			/* to_wait                               */
    NULL,			/* to_post_wait                          */
    deci2_fetch_registers,	/* to_fetch_registers                    */
    deci2_store_registers,	/* to_store_registers                    */
    deci2_prepare_to_store,	/* to_prepare_to_store                   */
    deci2_xfer_memory,		/* to_xfer_memory                        */
    deci2_files_info,		/* to_files_info                         */
    deci2_insert_breakpoint,	/* to_insert_breakpoint                  */
    deci2_remove_breakpoint,	/* to_remove_breakpoint                  */
    NULL,			/* to_terminal_init                      */
    NULL,			/* to_terminal_inferior                  */
    NULL,			/* to_terminal_ours_for_output           */
    NULL,			/* to_terminal_ours                      */
    NULL,			/* to_terminal_info                      */
    deci2_kill,			/* to_kill                               */
    deci2_load,			/* to_load                               */
    NULL,			/* to_lookup_symbol                      */
    deci2_create_inferior,	/* to_create_inferior                    */
    NULL,			/* to_post_startup_inferior              */
    NULL,			/* to_acknowledge_created_inferior       */
    NULL,			/* to_clone_and_follow_inferior          */
    NULL,			/* to_post_follow_inferior_by_clone      */
    NULL,			/* to_insert_fork_catchpoint             */
    NULL,			/* to_remove_fork_catchpoint             */
    NULL,			/* to_insert_vfork_catchpoint            */
    NULL,			/* to_remove_vfork_catchpoint            */
    NULL,			/* to_has_forked                         */
    NULL,			/* to_has_vforked                        */
    NULL,			/* to_can_follow_vfork_prior_to_exec     */
    NULL,			/* to_post_follow_vfork                  */
    NULL,			/* to_insert_exec_catchpoint             */
    NULL,			/* to_remove_exec_catchpoint             */
    NULL,			/* to_has_execd                          */
    NULL,			/* to_reported_exec_events_per_exec_call */
    NULL,			/* to_has_syscall_event                  */
    NULL,			/* to_has_exited                         */
    deci2_mourn_inferior,	/* to_mourn_inferior                     */
    NULL,			/* to_can_run                            */
    NULL,			/* to_notice_signal                      */
    NULL,			/* to_thread_alive                       */
    deci2_stop,			/* to_stop                               */
    NULL,			/* to_query                              */
    NULL,			/* to_enable_exception_callback          */
    NULL,			/* to_get_current_exception_callback     */
    NULL,			/* to_pid_to_exec_file                   */
    NULL,			/* to_core_file_to_sym_file              */
    process_stratum,		/* to_stratum                            */
    NULL,			/* DONT_USE                              */
    1,				/* to_has_all_memory                     */
    1,				/* to_has_memory                         */
    1,				/* to_has_stack                          */
    1,				/* to_has_registers                      */
    1,				/* to_has_execution                      */
    0,				/* to_has_thread_control                 */
    NULL,			/* sections                              */
    NULL,			/* sections_end                          */
    OPS_MAGIC			/* to_magic                              */
};

void
_initialize_remote_deci2()
{
    add_target(&deci2_ops);
    add_com("reset", class_obscure, deci2_reset_cmd, "DECI2 reset.");
}
