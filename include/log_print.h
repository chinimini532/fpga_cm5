#ifndef _LOG_PRINT_H_
#define _LOG_PRINT_H_

#include <stdio.h>
#include <stdint.h>
#include <sys/syscall.h>

#define LOG_FILE_PATH		    "/var/log"
#define DEFAULT_LOG_FILE_SIZE   0x64000     /* 400 kb */
#define MAX_LOG_FILE_SIZE	    0x100000	/* 1 Mb */
#define DEFAULT_LOG_FILES		2
#define MAX_LOG_FILES		    32
#define MAX_LOG_STR_LEN			4096

#define DBG_LEVEL1	0x00000001
#define DBG_LEVEL2	0x00000002
#define DBG_LEVEL3	0x00000004
#define DBG_LEVEL4	0x00000008
#define DBG_LEVEL5	0x00000010
#define DBG_LEVEL6	0x00000020
#define DBG_LEVEL7	0x00000040
#define DBG_LEVEL8	0x00000080
#define DBG_LEVEL9	0x00000100
#define DBG_LEVEL10	0x00000200
#define DBG_LEVEL11	0x00000400
#define DBG_LEVEL12	0x00000800
#define DBG_LEVEL13	0x00001000
#define DBG_LEVEL14	0x00002000
#define DBG_LEVEL15	0x00004000
#define DBG_LEVEL16	0x00008000

#define DERR	DBG_LEVEL1
#define DDBG	DBG_LEVEL2
#define DINFO	DBG_LEVEL3
#define DPLD    DBG_LEVEL5
#define DIPC	DBG_LEVEL4
#define DAPI	DBG_LEVEL6
#define DLOOP	DBG_LEVEL7

#define GETTID()            syscall(SYS_gettid) 

#define NGT_LOGFF(L, X...)  {set_log_ff(1); ngt_log_print(L, __func__, X); set_log_ff(0);}
#define NGT_LOG(L, X...)	ngt_log_print(L, __func__, X)
#define MAX_LOG_COUNT   	16

#define IS_ALNUM(ch) ((ch >='a' && ch <='z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || (ch >= '-' && ch <= '.') || (ch == '#') || (ch == '*'))

struct ngt_log_info_t {
	char name[20];
	char full_name[40];
	uint8_t max_files;
	uint32_t max_file_size;
	int dlevel;	/* log print level */
	FILE *fd;
};

#ifdef __cplusplus
extern "C" {
#endif
void ngt_log_print(int level, const char *func, const char *form, ...);
void ngt_log_init_file_descriptor();
void ngt_log_set_level(int cmd, int level);
int ngt_log_get_level();
void init_ngt_log(const char *name, int level, unsigned char max, int size);
char* url_decode( const char* str );
void set_fflush(int on_off);
void set_log_ff(int on_off);
#ifdef __cplusplus
}
#endif

#endif  /* _LOG_PRINT_H_ */
