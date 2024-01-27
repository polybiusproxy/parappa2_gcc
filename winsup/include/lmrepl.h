#ifndef _LMREPL_H
#define _LMREPL_H
#ifdef __cplusplus
extern "C" {
#endif
#define REPL_ROLE_EXPORT 1
#define REPL_ROLE_IMPORT 2
#define REPL_ROLE_BOTH 3
#define REPL_INTERVAL_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+0)
#define REPL_PULSE_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+1)
#define REPL_GUARDTIME_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+2)
#define REPL_RANDOM_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+3)
#define REPL_UNLOCK_NOFORCE 0
#define REPL_UNLOCK_FORCE 1
#define REPL_STATE_OK 0
#define REPL_STATE_NO_MASTER 1
#define REPL_STATE_NO_SYNC 2
#define REPL_STATE_NEVER_REPLICATED 3
#define REPL_INTEGRITY_FILE 1
#define REPL_INTEGRITY_TREE 2
#define REPL_EXTENT_FILE 1
#define REPL_EXTENT_TREE 2
#define REPL_EXPORT_INTEGRITY_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+0)
#define REPL_EXPORT_EXTENT_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+1)
typedef struct _REPL_INFO_0 {
	DWORD rp0_role;
	LPTSTR rp0_exportpath;
	LPTSTR rp0_exportlist;
	LPTSTR rp0_importpath;
	LPTSTR rp0_importlist;
	LPTSTR rp0_logonusername;
	DWORD rp0_interval;
	DWORD rp0_pulse;
	DWORD rp0_guardtime;
	DWORD rp0_random;
} REPL_INFO_0,*PREPL_INFO_0,*LPREPL_INFO_0;
typedef struct _REPL_INFO_1000 { DWORD rp1000_interval; } REPL_INFO_1000,*PREPL_INFO_1000,*LPREPL_INFO_1000;
typedef struct _REPL_INFO_1001 { DWORD rp1001_pulse; } REPL_INFO_1001,*PREPL_INFO_1001,*LPREPL_INFO_1001;
typedef struct _REPL_INFO_1002 { DWORD rp1002_guardtime; } REPL_INFO_1002,*PREPL_INFO_1002,*LPREPL_INFO_1002;
typedef struct _REPL_INFO_1003 { DWORD rp1003_random; } REPL_INFO_1003,*PREPL_INFO_1003,*LPREPL_INFO_1003;

NET_API_STATUS WINAPI NetReplGetInfo(LPTSTR,DWORD,PBYTE*);
NET_API_STATUS WINAPI NetReplSetInfo(LPTSTR,DWORD,PBYTE,PDWORD);
typedef struct _REPL_EDIR_INFO_0 {
	LPTSTR rped0_dirname;
} REPL_EDIR_INFO_0,*PREPL_EDIR_INFO_0,*LPREPL_EDIR_INFO_0;
typedef struct _REPL_EDIR_INFO_1 {
	LPTSTR rped1_dirname;
	DWORD rped1_integrity;
	DWORD rped1_extent;
} REPL_EDIR_INFO_1,*PREPL_EDIR_INFO_1,*LPREPL_EDIR_INFO_1;
typedef struct _REPL_EDIR_INFO_2 {
	LPTSTR rped2_dirname;
	DWORD rped2_integrity;
	DWORD rped2_extent;
	DWORD rped2_lockcount;
	DWORD rped2_locktime;
} REPL_EDIR_INFO_2,*PREPL_EDIR_INFO_2,*LPREPL_EDIR_INFO_2;
typedef struct _REPL_EDIR_INFO_1000 {
	DWORD rped1000_integrity;
} REPL_EDIR_INFO_1000,*PREPL_EDIR_INFO_1000,*LPREPL_EDIR_INFO_1000;
typedef struct _REPL_EDIR_INFO_1001 {
	DWORD rped1001_extent;
} REPL_EDIR_INFO_1001,*PREPL_EDIR_INFO_1001,*LPREPL_EDIR_INFO_1001;
typedef struct _REPL_IDIR_INFO_0 { LPTSTR rpid0_dirname; } REPL_IDIR_INFO_0,*PREPL_IDIR_INFO_0,*LPREPL_IDIR_INFO_0;
typedef struct _REPL_IDIR_INFO_1 {
	LPTSTR rpid1_dirname;
	DWORD rpid1_state;
	LPTSTR rpid1_mastername;
	DWORD rpid1_last_update_time;
	DWORD rpid1_lockcount;
	DWORD rpid1_locktime;
} REPL_IDIR_INFO_1,*PREPL_IDIR_INFO_1,*LPREPL_IDIR_INFO_1;
NET_API_STATUS WINAPI NetReplExportDirAdd(LPTSTR,DWORD,PBYTE,PDWORD);
NET_API_STATUS WINAPI NetReplExportDirDel(LPTSTR,LPTSTR);
NET_API_STATUS WINAPI NetReplExportDirEnum(LPTSTR,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,PDWORD);
NET_API_STATUS WINAPI NetReplExportDirGetInfo(LPTSTR,LPTSTR,DWORD,PBYTE*);
NET_API_STATUS WINAPI NetReplExportDirSetInfo(LPTSTR,LPTSTR,DWORD,PBYTE,PDWORD);
NET_API_STATUS WINAPI NetReplExportDirLock(LPTSTR,LPTSTR);
NET_API_STATUS WINAPI NetReplExportDirUnlock(LPTSTR,LPTSTR,DWORD);
NET_API_STATUS WINAPI NetReplImportDirAdd(LPTSTR,DWORD,PBYTE,PDWORD);
NET_API_STATUS WINAPI NetReplImportDirDel(LPTSTR,LPTSTR);
NET_API_STATUS WINAPI NetReplImportDirEnum(LPTSTR,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,PDWORD);
NET_API_STATUS WINAPI NetReplImportDirGetInfo(LPTSTR,LPTSTR,DWORD,PBYTE*);
NET_API_STATUS WINAPI NetReplImportDirLock(LPTSTR,LPTSTR);
NET_API_STATUS WINAPI NetReplImportDirUnlock(LPTSTR,LPTSTR,DWORD);
#ifdef __cplusplus
}
#endif
#endif 