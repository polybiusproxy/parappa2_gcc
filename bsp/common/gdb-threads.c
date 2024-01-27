/* 
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

/* FIXME: Scan this module for correct sizes of fields in packets */

#include <string.h>
#include <bsp/bsp.h>
#include <bsp/cpu.h>
#include <bsp/dbg-threads-api.h>
#include "gdb.h"


#ifdef GDB_THREAD_SUPPORT

#define UNIT_TEST 0
#define GDB_MOCKUP 0

#define STUB_BUF_MAX 300 /* for range checking of packet lengths */
     
#include "gdb-threads.h"

#if !defined(PKT_DEBUG)
#define PKT_DEBUG 0
#endif

#if PKT_DEBUG
#warning "PKT_DEBUG macros engaged"
#define PKT_TRACE(x) bsp_printf x
#else
#define PKT_TRACE(x)
#endif 

/* This is going to be irregular because the various implementations
 * have adopted different names for registers.
 * It would be nice to fix them to have a common convention
 *   _stub_registers
 *   stub_registers
 *   alt_stub_registers
 */

/* Registers from current general thread */
extern ex_regs_t _gdb_general_registers;

/* Pack an error response into the response packet */
#define PKT_NAK()     _gdb_pkt_append("E02")

/* Pack an OK achnowledgement */
#define PKT_ACK()     _gdb_pkt_append("OK")

/* ------ UNPACK_THREADID ------------------------------- */
/* A threadid is a 64 bit quantity                        */

#define BUFTHREADIDSIZ 16 /* encode 64 bits in 16 chars of hex */

static char *
unpack_threadid(char * inbuf, threadref * id)
{
    int i;
    char * altref = (char *) id;

    for (i = 0; i < (BUFTHREADIDSIZ/2); i++)
	*altref++ = __unpack_nibbles(&inbuf, 2);
    return inbuf ;
}


/* -------- PACK_STRING ---------------------------------------------- */
static void
pack_string(char *string)
{
    char ch ;
    int len ;

    len = strlen(string);
    if (len > 200)
	len = 200 ; /* Bigger than most GDB packets, junk??? */

    _gdb_pkt_append("%02x", len);
    while (len-- > 0) { 
	ch = *string++ ;
	if (ch == '#')
	    ch = '*'; /* Protect encapsulation */
	_gdb_pkt_append("%c", ch);
    }
}


/* ----- PACK_THREADID  --------------------------------------------- */
/* Convert a binary 64 bit threadid  and pack it into a xmit buffer */
/* Return the advanced buffer pointer */
static void
pack_threadid(threadref *id)
{
    int i;
    unsigned char *p;

    p = (unsigned char *)id;

    for (i = 0; i < 8; i++)
	_gdb_pkt_append("%02x", *p++);
}



/* UNFORTUNATLY, not all ow the extended debugging system has yet been
   converted to 64 but thread referenecces and process identifiers.
   These routines do the conversion.
   An array of bytes is the correct treatment of an opaque identifier.
   ints have endian issues.
 */
static void
int_to_threadref(threadref *id, int value)
{
    unsigned char *scan = (unsigned char *)id;
    int i;

    for (i = 0; i < 4; i++)
	*scan++ = 0;

    *scan++ = (value >> 24) & 0xff;
    *scan++ = (value >> 16) & 0xff;
    *scan++ = (value >> 8) & 0xff;
    *scan++ = (value & 0xff);
}

static int
threadref_to_int(threadref *ref)
{
    int value = 0 ;
    unsigned char * scan ;
    int i ;
  
    scan = (char *) ref ;
    scan += 4 ;
    i = 4 ;
    while (i-- > 0)
	value = (value << 8) | ((*scan++) & 0xff);
    return value ;
}


int
_gdb_get_currthread (void)
{
    threadref thread;
  
    if (dbg_currthread(&thread))
	return threadref_to_int(&thread);

    return 0;
}


void
_gdb_currthread(void)
{
    threadref thread;
  
    if (dbg_currthread(&thread)) {
	_gdb_pkt_append("QC%08x", threadref_to_int(&thread));
    } else {
	PKT_NAK();
    }
}


/*
 * Answer the thread alive query
 */
static int
thread_alive(int id)
{
    threadref thread ;
    struct cygmon_thread_debug_info info ;

    int_to_threadref(&thread, id) ;
    if (dbg_threadinfo(&thread, &info) && info.context_exists)
	return 1;

    return 0;
}


void
_gdb_thread_alive(char *inbuf)
{
    unsigned long id;
  
    if (__unpack_ulong(&inbuf, &id)) {
	if (thread_alive((int)id)) {
	    PKT_ACK();
	    return;
	}
    }
    PKT_NAK();
}
 

/* ----- _GDB_CHANGETHREAD ------------------------------- */
/* Switch the display of registers to that of a saved context */

/* Changing the context makes NO sense, although the packets define the
   capability. Therefore, the option to change the context back does
   call the function to change registers. Also, there is no
   forced context switch.
     'p' - New format, long long threadid, no special cases
     'c' - Old format, id for continue, 32 bit threadid max, possably less
                       -1 means continue all threads
     'g' - Old Format, id for general use (other than continue)

     replies:
          OK for success
	  ENN for error
   */
void
_gdb_changethread(char *inbuf, ex_regs_t *exc_regs)
{
    threadref id, cur_id;
    int       idefined = -1, is_neg = 0, new_thread = 0;
    unsigned long ul;
    gdb_regs_t gdb_regs;

    PKT_TRACE(("_gdb_changethread: <%s> gen[%d] cont[%d]\n",
	       inbuf, _gdb_general_thread, _gdb_cont_thread));

    /* Parse the incoming packet for a thread identifier */
    switch (*inbuf++) {
      case 'p':
	/* New format: mode:8,threadid:64 */
	idefined = __unpack_nibbles(&inbuf, 1);
	inbuf = unpack_threadid(inbuf, &id); /* even if startflag */
	break ;  

      case 'c':
	/* old format , specify thread for continue */
	if (*inbuf == '-') {
	    is_neg = 1;
	    ++inbuf;
	}
	__unpack_ulong(&inbuf, &ul);
	new_thread = (int)ul;
	if (is_neg) {
	    new_thread = -new_thread;
	    if (new_thread == -1)
		new_thread = 0;
	}

	if (new_thread == 0 || 		/* revert to any old thread */
	    thread_alive(new_thread)) {	/* specified thread is alive */
	    _gdb_cont_thread = new_thread;
	    PKT_ACK();
	} else
	    PKT_NAK();
	break ;

      case 'g':
	/* old format, specify thread for general operations */
	/* OLD format: parse a variable length hex string */
	/* OLD format consider special thread ids */
	if (*inbuf == '-') {
	    is_neg = 1;
	    ++inbuf;
	}
	__unpack_ulong(&inbuf, &ul);
	new_thread = (int)ul;
	if (is_neg)
	    new_thread = -new_thread;

	int_to_threadref(&id, new_thread);
	switch (new_thread) {
	  case  0 : /* pick a thread, any thread */
	  case -1 : /* all threads */
	    idefined = 2;
	    _gdb_general_thread = new_thread;
	    break ;
	  default :
	    idefined = 1 ; /* select the specified thread */
	    break ;
	}
	break ;

      default:
	PKT_NAK();
	break ;
    }

    PKT_TRACE(("_gdb_changethread: idefined[%d] gen[%d] cont[%d]\n",
	       idefined, _gdb_general_thread, _gdb_cont_thread));

    switch (idefined) {
      case -1 :
	/* Packet not supported, already NAKed, no further action */
	break ;
      case 0 :
	/* Switch back to interrupted context */
	/*_registers = &registers;*/
	break ;
      case 1 :
	/*
	 * dbg_getthreadreg will fail for the original thread.
	 * This is normal.
	 */
	if (dbg_currthread(&cur_id)) {
	    if (threadref_to_int(&cur_id) == new_thread) {
		_gdb_general_thread = 0;
		PKT_ACK();
		return;
	    }
	}
	/*
         * The OS will now update the values it has in a saved
	 * process context. This will probably not be all gdb regs,
         * so we preload the gdb_regs structure with current
         * values.
	 */
	bsp_copy_exc_to_gdb_regs(&gdb_regs, exc_regs);
	if (dbg_getthreadreg(&id, NUMREGS, &gdb_regs)) {
	    /*
	     * Now setup _gdb_general_registers. Initialize _gdb_registers
	     * with the exception frame registers so that the non-gdb regs
	     * have the best chance of being reasonably correct.
	     */
	    memcpy(&_gdb_general_registers, exc_regs, sizeof(ex_regs_t));
	    bsp_copy_gdb_to_exc_regs(&_gdb_general_registers, &gdb_regs);
	    _gdb_general_thread = new_thread;
	    PKT_ACK(); 
	} else {
	    PKT_NAK();
	}
	break ;
      case 2 :
	/* switch to interrupted context */ 
	PKT_ACK();
	break ;
      default:
	PKT_NAK();
	break ;
    }
}


/* ---- _GDB_GETTHREADLIST ------------------------------- */
/* 
 * Get a portion of the threadlist  or process list.
 * This may be part of a multipacket transaction.
 * It would be hard to tell in the response packet the difference
 * between the end of list and an error in threadid encoding.
 */
#define MAX_THREAD_CNT 24
void
_gdb_getthreadlist(char *inbuf)
{
    threadref  th_buf[MAX_THREAD_CNT], req_lastthread;
    int  i, start_flag, batchsize, count, done;
    static threadref lastthread, nextthread;

    PKT_TRACE(("_gdb_getthreadlist: <%s>\n", inbuf));

    start_flag = __unpack_nibbles(&inbuf, 1);
    batchsize  = __unpack_nibbles(&inbuf, 2);
    if (batchsize > MAX_THREAD_CNT)
	batchsize = MAX_THREAD_CNT;

    /* even if startflag */
    inbuf = unpack_threadid(inbuf, &lastthread);

    /* save for later */
    memcpy(&req_lastthread, &lastthread, sizeof(threadref));

    PKT_TRACE(("pkt_getthreadlist: start_flag[%d] batchsz[%d] last[0x%x]\n",
	       start_flag, batchsize, threadref_to_int(lastthread)));

    /* Acquire the thread ids from the kernel */
    for (done = count = 0; count < batchsize; ++count) {
	if (!dbg_threadlist(start_flag, &lastthread, &nextthread)) {
	    done = 1;
	    break;
	}
	start_flag = 0; /* redundent but effective */
#if 0 /* DEBUG */
	if (!memcmp(&lastthread, &nextthread, sizeof(threadref))) {
	    bsp_printf("FAIL: Threadlist, not incrementing\n");
	    done = 1;
	    break;
	}
#endif      
	PKT_TRACE(("pkt_getthreadlist: Adding thread[0x%x]\n",
	       threadref_to_int(nextthread)));

	memcpy(&th_buf[count], &nextthread, sizeof(threadref));
	memcpy(&lastthread, &nextthread, sizeof(threadref));
    }

    /* Build response packet    */
    _gdb_pkt_append("QM%02x%d", count, done);
    pack_threadid(&req_lastthread);
    for (i = 0; i < count; ++i)
	pack_threadid(&th_buf[i]);
}




/* ----- _GDB_GETTHREADINFO ---------------------------------------- */
/*
 * Get the detailed information about a specific thread or process.
 *
 * Encoding:
 *   'Q':8,'P':8,mask:16
 *
 *    Mask Fields
 *	threadid:1,        # always request threadid 
 *	context_exists:2,
 *	display:4,          
 *	unique_name:8,
 *	more_display:16
 */  
void
_gdb_getthreadinfo(char *inbuf)
{
    int mask ;
    int result ;
    threadref thread ;
    struct cygmon_thread_debug_info info ;

    info.context_exists = 0 ;
    info.thread_display = 0 ;
    info.unique_thread_name = 0 ;
    info.more_display = 0 ;

    /* Assume the packet identification chars have already been
       discarded by the packet demultiples routines */
    PKT_TRACE(("_gdb_getthreadinfo: <%s>\n",inbuf));
  
    mask = __unpack_nibbles(&inbuf, 8) ;
    inbuf = unpack_threadid(inbuf,&thread) ;
  
    result = dbg_threadinfo(&thread, &info); /* Make kernel call */

    if (result)	{
	_gdb_pkt_append("qp%08x", mask);
	pack_threadid(&info.thread_id); /* echo threadid */

	if (mask & 2) {
	    /* context-exists */
	    _gdb_pkt_append("%08x%02x%02x", 2, 2, info.context_exists);
	}
	if ((mask & 4) && info.thread_display) {
	    /* display */
	    _gdb_pkt_append("%08x", 4);
	    pack_string(info.thread_display);
	}
	if ((mask & 8) && info.unique_thread_name) {
	    /* unique_name */
	    _gdb_pkt_append("%08x", 8);
	    pack_string(info.unique_thread_name);
	}
	if ((mask & 16) && info.more_display) {
	    /* more display */
	    _gdb_pkt_append("%08x", 16);
	    pack_string(info.more_display);
	}
    } else {
	PKT_TRACE(("FAIL: dbg_threadinfo\n"));
	PKT_NAK();
    }
}


int
_gdb_lock_scheduler(int lock, 	/* 0 to unlock, 1 to lock */
		    int mode, 	/* 0 for step,  1 for continue */
		    long id)	/* current thread */
{
    threadref thread;

    int_to_threadref(&thread, id);
    return dbg_scheduler(&thread, lock, mode);
}



#if GDB_MOCKUP
/* ------ MATCHED GDB SIDE PACKET ENCODING AND PARSING ----------------- */

static void
buffer_threadid(char *buf, threadref *id)
{
    int i;
    unsigned char *p = (unsigned char *)id;

    for (i = 0; i < 8; i++) {
	bsp_sprintf(buf, "%02x", *p++);
	++buf;
    }
}


/* ----- PACK_SETTHREAD_REQUEST ------------------------------------- */
/* 	Encoding: ??? decode gdb/remote.c
	'Q':8,'p':8,idefined:8,threadid:32 ;
	*/

void
pack_setthread_request(char      *buf,
		       char      fmt,   /* c,g or, p */
		       int       idformat ,
		       threadref *threadid )
{
    *buf++ = fmt;
  
    if (fmt == 'p') {
	/* pack the long form */
	bsp_sprintf(buf, "%x", idformat);
	++buf;
	buffer_threadid(buf, threadid)
    } else {
	/* pack the shorter form - Serious truncation */
	/* There are reserved identifieds 0 , -1 */
	bsp_sprintf(buf, "%x", threadref_to_int(threadid));
    }
}



/* -------- PACK_THREADLIST-REQUEST --------------------------------- */
/*    Format: i'Q':8,i"L":8,initflag:8,batchsize:16,lastthreadid:32   */


static void
pack_threadlist_request(char *buf,
			int  startflag,
			int  threadcount,
			threadref *nextthread)
{
    bsp_sprintf(buf, "qL%x%02x", startflag, threadcount);
    buf += 5;
    buffer_threadid(buf, nextthread);
}


/* ---------- PARSE_THREADLIST_RESPONSE ------------------------------------ */
/* Encoding:   'q':8,'M':8,count:16,done:8,argthreadid:64,(threadid:64)* */

static int
parse_threadlist_response(char * pkt, threadref * original_echo,
			  threadref * resultlist, int * doneflag)
{
    char * limit ;
    int count, resultcount , done ;
    resultcount = 0 ;

    /* assume the 'q' and 'M chars have been stripped */
    PKT_TRACE(("parse-threadlist-response %s\n", pkt));
    limit = pkt + (STUB_BUF_MAX - BUFTHREADIDSIZ) ; /* done parse past here */
    count = __unpack_nibbles(&pkt, 2);	  /* count field */
    done  = __unpack_nibbles(&pkt, 1);
    /* The first threadid is the argument threadid */
    pkt = unpack_threadid(pkt,original_echo) ; /* should match query packet */
    while ((count-- > 0) && (pkt < limit)) {
	pkt = unpack_threadid(pkt,resultlist++) ;
	resultcount++ ;
    }
    if (doneflag)
	*doneflag = done ;
    return resultcount ; /* successvalue */
}
 
struct gdb_ext_thread_info
{
    threadref threadid ;
    int active ;
    char display[256] ;
    char shortname[32] ;
    char more_display[256] ;
};

/* ----- PACK_THREAD_INFO_REQUEST -------------------------------- */

/* 
   threadid:1,        # always request threadid
   context_exists:2,
   display:4,
   unique_name:8,
   more_display:16
 */

/* Encoding:  'Q':8,'P':8,mask:32,threadid:64 */

static void
pack_threadinfo_request(char *buf, int mode, threadref * id)
{
    bsp_sprintf(buf, "QP%08x", mode);
    buf += 10;
    buffer_threadid(buf, id); /* threadid */
}


static char *
unpack_string(char *src, char *dest, int length)
{
    while (length--)
	*dest++ = *src++ ;
    *dest = '\0' ;
    return src ;
}


static char *
threadid_to_str(threadref *ref)
{
    static char hexid[20];

    buffer_threadid(hexid, ref);
  
    return hexid;
}

/* ------ REMOTE_UPK_THREAD_INFO_RESPONSE ------------------------------- */
/* Unpack the response of a detailed thread info packet */
/* Encoding:  i'Q':8,i'R':8,argmask:16,threadid:64,(tag:8,length:16,data:x)* */

#define TAG_THREADID 1
#define TAG_EXISTS 2
#define TAG_DISPLAY 4
#define TAG_THREADNAME 8
#define TAG_MOREDISPLAY 16 

int
remote_upk_thread_info_response(char *pkt,
				threadref *expectedref ,
				struct gdb_ext_thread_info *info)
{
    int mask, length ;
    unsigned int tag ;
    threadref ref ;
    char * limit = pkt + 500 ; /* plausable parsing limit */
    int retval = 1 ;

    PKT_TRACE(("upk-threadinfo %s\n", pkt));

    /* info->threadid = 0 ; FIXME: implement zero_threadref */
    info->active = 0 ;
    info->display[0] = '\0' ;
    info->shortname[0] = '\0' ;
    info->more_display[0] = '\0' ;

    /* Assume the characters indicating the packet type have been stripped */
    mask = __unpack_nibbles(&pkt, 8);  /* arg mask */
    pkt = unpack_threadid(pkt , &ref) ;
                      
    if (memcmp(&ref, expectedref, sizeof(threadref))) {
	/* This is an answer to a different request */
	bsp_printf("FAIL Thread mismatch\n");
	bsp_printf("ref %s\n", threadid_to_str(&ref));
	bsp_printf("expected %s\n", threadid_to_str(&expectedref));
	return 0 ;
    }
    memcpy(&info->threadid, &ref, sizeof(threadref));
  
    /* Loop on tagged fields , try to bail if somthing goes wrong */
    if (mask==0)
	bsp_printf("OOPS NO MASK \n") ;

    /* packets are terminated with nulls */
    while ((pkt < limit) && mask && *pkt) {
	tag = __unpack_nibbles(&pkt, 8) ;            /* tag */
	length = __unpack_nibbles(&pkt, 2) ;   /* length */
	if (! (tag & mask)) {
	    /* tags out of synch with mask */
	    bsp_printf("FAIL: threadinfo tag mismatch\n") ;
	    retval = 0 ;
	    break ;
	}
	if (tag == TAG_THREADID) {
	    bsp_printf("unpack THREADID\n") ;
	    if (length != 16) {
		bsp_printf("FAIL: length of threadid is not 16\n") ;
		retval = 0 ;
		break ;
	    }
	    pkt = unpack_threadid(pkt,&ref) ;
	    mask = mask & ~ TAG_THREADID ;
	    continue ;
	}
	if (tag == TAG_EXISTS) {
	    info->active = __unpack_nibbles(&pkt, length);
	    mask = mask & ~(TAG_EXISTS) ;
	    if (length > 8) {
		bsp_printf("FAIL: 'exists' length too long\n") ;
		retval = 0 ;
		break ;
	    }
	    continue ;
	}
	if (tag == TAG_THREADNAME) {
	    pkt = unpack_string(pkt,&info->shortname[0],length) ;
	    mask = mask & ~TAG_THREADNAME ;
	    continue ;
	}
	if (tag == TAG_DISPLAY) { 
	    pkt = unpack_string(pkt,&info->display[0],length) ;
	    mask = mask & ~TAG_DISPLAY ;
	    continue ;
	}
	if (tag == TAG_MOREDISPLAY) { 
	    pkt = unpack_string(pkt,&info->more_display[0],length) ;
	    mask = mask & ~TAG_MOREDISPLAY ;
	    continue ;
	}
	bsp_printf("FAIL: unknown info tag\n") ;
	break ; /* Not a tag we know about */
    }
    return retval  ;
}


/* ---- REMOTE_PACK_CURRTHREAD_REQUEST ---------------------------- */
/* This is a request to emit the T packet */

/* FORMAT: 'q':8,'C' */

static char *
remote_pack_currthread_request(char * pkt)
{
    *pkt++ = 'q' ;
    *pkt++ = 'C' ;
    *pkt = '\0' ;
    return pkt ;
} /* remote_pack_currthread_request */


/* ------- REMOTE_UPK_CURTHREAD_RESPONSE ----------------------- */
/* Unpack the interesting part of a T packet */


/* Parse a T packet */
static int
remote_upk_currthread_response(unsigned char * pkt, int *thr)
{
    int retval = 0 ;

    PKT_TRACE(("upk-currthreadresp %s\n", pkt));

#if 0
    {
	static char threadtag[8] =  "thread" ;
	int retval = 0 ;
	int i , found ;
	unsigned char ch ;
	unsigned long quickid ;

	/* Unpack as a t packet */
	/* scan for : thread */
        /* stop at end of packet */
	while ((ch = *pkt++) != ':'  &&  ch != '\0') {
	    found = 0 ;
	    i = 0 ;
	    while ((ch = *pkt++) == threadtag[i++])
		;
	    if (i == 8) { /* string match "thread" */
		__unpack_ulong(&pkt, &quickid);
		retval = 1;
		break ;
	    }
	    retval = 0 ;
	}
    }
#else
    pkt = unpack_threadid(pkt, thr) ;
    retval = 1 ;
#endif  
    return retval ;
}

/* -------- REMOTE_UPK-SIMPLE_ACK --------------------------------- */
/* Decode a response which is eother "OK" or "Enn"
   fillin error code,  fillin pkfag-1== undef, 0==nak, 1 == ack ;
   return advanced packet pointer */

static char *
remote_upk_simple_ack(char * buf, int * pkflag, int * errcode)
{
    int lclerr = 0 ;
    char ch = *buf++ ;
    int retval = -1 ;  /* Undefined ACK , a protocol error */
    if (ch == 'E') {   /* NAK */
	lclerr = __unpack_nibbles(&buf, 2);
	retval = 0 ;   /* transaction failed, explicitly */
    } else if ((ch == 'O') && (*buf++ == 'K')) /* ACK */
	retval = 1 ; /* transaction succeeded */
    *pkflag = retval ;
    *errcode = lclerr ;
    return buf ;
}

/* -------- PACK_THREADALIVE_REQUEST ------------------------------- */
static char *
pack_threadalive_request(char * buf, threadref * threadid)
{
    *buf++ = 'T' ;
    buf = pack_threadid(buf,threadid) ;
    *buf = '\0' ;
    return buf ;
}
#endif /* GDB_MOCKUP */

/* ---------------------------------------------------------------------- */
/* UNIT_TESTS SUBSECTION                                                  */
/* ---------------------------------------------------------------------- */


#if UNIT_TEST
static char test_req[400] ;
static char t_response[400] ;

/* ----- DISPLAY_THREAD_INFO ---------------------------------------------- */
/*  Use local cygmon string output utiities */

void
display_thread_info(struct gdb_ext_thread_info * info)
{
    bsp_printf("Threadid: %s\n", threadid_to_str(&info->threadid));
    bsp_printf("  Name:  %s\n", info->shortname);
    bsp_printf("  State: %s\n", info->display);
    bsp_printf("  Other: %s\n\n", info->more_display);
}


/* --- CURRTHREAD-TEST -------------------------------------------- */
static int
currthread_test(threadref * thread)
{
    int result ;
    int threadid ;

    bsp_printf("TEST: currthread\n");
    remote_pack_currthread_request(test_req);
    stub_pkt_currthread(test_req+2,t_response,STUB_BUF_MAX) ;

    result = remote_upk_currthread_response(t_response+2, &threadid);
    if (result) {
	bsp_printf("PASS getcurthread\n") ;
	/* FIXME: print the thread */
    }
    else
	bsp_printf("FAIL getcurrthread\n") ;
    return result ;
}


/* ------ SETTHREAD_TEST ------------------------------------------- */
  /* use a known thread from previous test */
static int
setthread_test(threadref *thread)
{
    int result, errcode ;
    bsp_printf("TEST: setthread\n");
  
    pack_setthread_request(test_req,'p',1,thread);
    _gdb_changethread(test_req,t_response,STUB_BUF_MAX);
    remote_upk_simple_ack(t_response,&result,&errcode);

    switch (result) {
      case 0:
	bsp_printf("FAIL setthread\n");
	break ;
      case 1:
	bsp_printf("PASS setthread\n") ;
	break ;
      default :
	bsp_printf("FAIL setthread -unrecognized response\n") ;
	break ;
    }
    return result ;
}


/* ------ THREADACTIVE_TEST ---------------------- */
  /* use known thread */
  /* pack threadactive packet */
  /* process threadactive packet */
  /* parse threadactive response */
  /* check results */
int
threadactive_test(threadref *thread)
{
    int result ;
    int errcode ;

    bsp_printf("TEST: threadactive\n") ;
    pack_threadalive_request(test_req,thread) ;
    stub_pkt_thread_alive(test_req+1,t_response,STUB_BUF_MAX);
    remote_upk_simple_ack(t_response,&result,&errcode) ;
    switch (result) {
      case 0 :
	bsp_printf("FAIL threadalive\n") ;
	break ;
      case 1 :
	bsp_printf("PASS threadalive\n") ;
	break ;
      default :
	bsp_printf("FAIL threadalive -unrecognized response\n") ;
	break ;
    }
    return result ;
}

/* ------ REMOTE_GET_THREADINFO -------------------------------------- */
static int
remote_get_threadinfo(threadref *threadid, int fieldset,
		      struct gdb_ext_thread_info * info)
{
    int result ;
    pack_threadinfo_request(test_req,fieldset,threadid) ;
    stub_pkt_getthreadinfo(test_req+2,t_response,STUB_BUF_MAX) ;
    result = remote_upk_thread_info_response(t_response+2,threadid,info) ;
    return result ;
}


static struct gdb_ext_thread_info test_info ;

static int
get_and_display_threadinfo(threadref * thread)
{
    int mode ;
    int result ;
    /* bsp_printf("TEST: get and display threadinfo\n") ; */

    mode = TAG_THREADID | TAG_EXISTS | TAG_THREADNAME
	| TAG_MOREDISPLAY | TAG_DISPLAY ;
    result = remote_get_threadinfo(thread,mode,&test_info) ;
    if (result)
	display_thread_info(&test_info) ;
#if 0  /* silent subtest */
    if (result)
	bsp_printf("PASS: get_and_display threadinfo\n") ;
    else
	bsp_printf("FAIL: get_and_display threadinfo\n") ;
#endif  
    return result ;
}


/* ----- THREADLIST_TEST ------------------------------------------ */
#define TESTLISTSIZE 16
#define TLRSIZ 2
static threadref test_threadlist[TESTLISTSIZE] ;

static int
threadlist_test(void)
{
    int done, i , result_count ;
    int startflag = 1 ;
    int result = 1 ;
    int loopcount = 0 ;
    static threadref nextthread ;
    static threadref echo_nextthread ;

    bsp_printf("TEST: threadlist\n") ;

    done = 0 ;
    while (!done) {
	if (loopcount++ > 10) {
	    result = 0 ;
	    bsp_printf("FAIL: Threadlist test -infinite loop-\n") ;
	    break ;
	}
	pack_threadlist_request(test_req,startflag,TLRSIZ,&nextthread) ;
	startflag = 0 ; /* clear for later iterations */
	stub_pkt_getthreadlist(test_req+2,t_response,STUB_BUF_MAX);
	result_count = parse_threadlist_response(t_response+2,
						 &echo_nextthread,
						 &test_threadlist[0],&done) ;
	if (memcmp(&echo_nextthread, &nextthread, sizeof(threadref))) {
	    bsp_printf("FAIL: threadlist did not echo arg thread\n");
	    result = 0 ;
	    break ;
	}
	if (result_count <= 0) {
	    if (done != 0) {
		bsp_printf("FAIL threadlist_test, failed to get list");
	        result = 0 ;
	    }
	    break ;
	}
	if (result_count > TLRSIZ) {
	    bsp_printf("FAIL: threadlist response longer than requested\n") ;
	    result = 0 ;
	    break ;
	}
	/* Setup to resume next batch of thread references , set nestthread */
	memcpy(&nextthread, &test_threadlist[result_count-1], sizeof(threadref));
	/* print_threadid("last-of-batch",&nextthread) ; */
	i = 0 ;
	while (result_count--) {
	    if (0)  /* two display alternatives */
		print_threadid("truncatedisplay",&test_threadlist[i++]) ;
	    else
		get_and_display_threadinfo(&test_threadlist[i++]) ; 
	}
    }
    bsp_printf("%s: threadlist test\n", result ? "PASS" : "FAIL");

    return result ;
}


static threadref testthread ;

int
test_thread_support(void)
{
    int result = 1 ;
    bsp_printf("TESTING Thread support infrastructure\n") ;
    stub_pack_Tpkt_threadid(test_req) ;
    PKT_TRACE(("packing the threadid  -> %s\n", test_req));
    result &= currthread_test(&testthread) ;
    result &= get_and_display_threadinfo(&testthread) ;
    result &= threadlist_test() ;
    result &= setthread_test(&testthread) ;
    if (result)
	bsp_printf("PASS: UNITTEST Thread support\n") ;
    else
        bsp_printf("FAIL: UNITTEST Thread support\n") ;
    return result ;
}
#endif /* UNIT_TEST */

#endif
