#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/timeb.h>
#include <sys/stat.h>

#include "log_print.h"

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct ngt_log_info_t log_info;
static int set_fflush_flag = 0;  // OFF(0)
static int log_ff = 0;  // OFF(0) -> 1 회용


// ON(1), OFF(0)
void set_fflush(int on_off)
{
	set_fflush_flag = on_off;
}

void set_log_ff(int on_off)
{
	log_ff = on_off;
}

#if 0
static void *task_delete_log()
{
	char com[400];

	sprintf(com, "for log in `/bin/ls -tdr %s.*`; do set `/bin/ls %s.*`; if [ $# -ge %d ]; then /bin/rm $log; fi; done", log_info.full_name, log_info.full_name, log_info.max_files);

	if (system(com) < 0) {
		printf("system(com) error\n");
	}
	log_info.task_flag = 0;
	pthread_exit(NULL);
}
#endif

static int ngt_log_free_file()
{
#if 1
	int log_count = 0; 
	char f_path[1024];
	DIR *dp;
	struct dirent *item;
	struct stat     finfo;
	int rc = 0;
	time_t oldest_file_mtime = 0;
	char oldest_file_path[1024];

	dp = opendir(LOG_FILE_PATH);

	if (dp != NULL) {
		for (;;) {
			item = readdir(dp);
			if (item == NULL) {
				if(log_count >= log_info.max_files) {
					unlink(oldest_file_path);
					if(log_count > log_info.max_files) {
						ngt_log_free_file();
					}
				}
				break;
			}
			else {
				if (strstr(item->d_name, log_info.name) != NULL) {
					log_count++;
					bzero(f_path, sizeof(f_path));
					sprintf(f_path, "%s/%s", LOG_FILE_PATH, item->d_name);
					if((rc = stat(f_path, &finfo)) < 0 ) {  //read last modify date on file name
						fprintf(stderr, "Log File Date Read Fail...%s:%s\n",item->d_name, strerror(errno));
						continue;
					}
					if(oldest_file_mtime == 0 || oldest_file_mtime > finfo.st_mtime) {
						bzero(oldest_file_path,sizeof(oldest_file_path));
						oldest_file_mtime = finfo.st_mtime;
						strcpy(oldest_file_path, f_path);
					}
				}
			}
		}
		closedir(dp);
	} else 
		fprintf(stderr, "Log Dir Open Fail...%s:%s\n", LOG_FILE_PATH, strerror(errno));
#else
	pthread_t tid;

	system("sync");

	if (log_info.task_flag)
		return 1;

	log_info.task_flag = 1;
	if (pthread_create(&tid, NULL, task_delete_log, NULL) < 0) {
		printf("[%s] pthread_create : %s\n", __func__, strerror(errno));
		log_info.task_flag = 0;
		return 0;
	}
	pthread_detach(tid);
#endif
	return 1;
}

void ngt_log_print(int level, const char *func, const char *form, ...)
{
	char str[MAX_LOG_STR_LEN] = {0, };
	char mvfile[60] = {0, };  /* 2022.01.14 by KKU - 컴파일 에러 보완 (40 -> 60) */
	va_list	pvar;
	struct timeb tmb;
	struct tm *tmp;

	if (!(log_info.dlevel & level))
		return;

	pthread_mutex_lock(&log_mutex);

	ftime(&tmb);
	tmp = localtime(&tmb.time);

	if (!log_info.fd) {
		if ((log_info.fd = fopen(log_info.full_name, "a")) == NULL)
			log_info.fd = stdout;
		fprintf(log_info.fd, "\n################# LOG START ######## %02d/%02d ########\n\n",
			tmp->tm_mon+1, tmp->tm_mday);
	}

	fprintf(log_info.fd, "%02d/%02d#%02d:%02d:%02d.%03d#[%ld]#%s#: ",
		tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec, tmb.millitm, GETTID(), func);

	va_start(pvar, form);
#if 1  /* 2022.01.14 by KKU - strlen(form) 의 오사용 보완 */
	vsnprintf(str, MAX_LOG_STR_LEN, form, pvar);
	if(str[strlen(str)-1] == '\n') {
		// 여기에서 '\n'을 제거하고 아래 출력에서 추가함
		// -> '\n'이 있던 없던 '\n'을 추가하게 됨
		str[strlen(str)-1] = '\0';
	}

	if(level == 0)  fprintf(log_info.fd,"\033[31m%s\033[0m\n", str);
	else {
		if (level == DERR)
			fprintf(log_info.fd, "%c[1;31m", 27);   /* light red */
		else if (level == DDBG)
			fprintf(log_info.fd, "%c[1;35m", 27);   /* light puple */
		else if (level == DINFO)
			fprintf(log_info.fd, "%c[0;36m", 27);   /* cyan */
		else if (level == DIPC)
			fprintf(log_info.fd, "%c[0;32m", 27);   /* green */
		else if (level == DBG_LEVEL6)
			fprintf(log_info.fd, "%c[1;34m", 27);   /* light Blue */
		else if (level == DBG_LEVEL7)
			fprintf(log_info.fd, "%c[0;33m", 27);   /* yellow */
		else if (level == DBG_LEVEL8)
			fprintf(log_info.fd, "%c[1;36m", 27);   /* light cyan */

		// ORG와동일하게 '\n'이 없어도 추가됨 -> 단, 맨끝으로 위치시킴
		fprintf(log_info.fd, "%s%c[0m\n", str, 27);
	}
#else  /* ORG */
	if (level == 0) {
		vsprintf(str, form, pvar);
		if (str[strlen(str) - 1] == '\n')
			str[strlen(str) - 1] = '\0';
		fprintf(log_info.fd,"\033[31m%s\033[0m\n", str);
	} else {
		if (level == DERR)
			fprintf(log_info.fd, "%c[1;31m", 27);   /* light red */
		else if (level == DDBG)
			fprintf(log_info.fd, "%c[1;35m", 27);   /* light puple */
		else if (level == DINFO)
			fprintf(log_info.fd, "%c[0;36m", 27);   /* cyan */
		else if (level == DIPC)
			fprintf(log_info.fd, "%c[0;32m", 27);   /* green */
//      else if (level == DPLD)
//          fprintf(log_info.fd, "%c[1;37m", 27);   /* white */
		else if (level == DBG_LEVEL6)
			fprintf(log_info.fd, "%c[1;34m", 27);   /* light Blue */
		else if (level == DBG_LEVEL7)
			fprintf(log_info.fd, "%c[0;33m", 27);   /* yellow */
		else if (level == DBG_LEVEL8)
			fprintf(log_info.fd, "%c[1;36m", 27);   /* light cyan */

		vfprintf(log_info.fd, form, pvar);
		if (form[strlen(form) - 1] != '\n')
			fprintf(log_info.fd, "\n");

		fprintf(log_info.fd, "%c[0m", 27);
	}
#endif
	va_end(pvar);
    
	if (log_info.fd != stdout && (ftell(log_info.fd) > (log_info.max_file_size))) {
		fprintf(log_info.fd, "\n################# LOG RESET ######## %02d/%02d ########\n\n",
			tmp->tm_mon+1, tmp->tm_mday);
		sprintf(mvfile, "%s.%02d%02d_%02d%02d%02d", log_info.full_name, tmp->tm_mon + 1,
			tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
		rename(log_info.full_name, mvfile);
		fclose(log_info.fd);
		log_info.fd = NULL;
		ngt_log_free_file();
	} else
		fflush(log_info.fd);

	if(set_fflush_flag)  fflush(log_info.fd);
	else {  // 1 회용
		if(log_ff)       fflush(log_info.fd);
	}

	pthread_mutex_unlock(&log_mutex);
}

void ngt_log_init_file_descriptor()
{
	if (log_info.fd == NULL) return;
	fclose(log_info.fd);
	log_info.fd = NULL;
}

#if 0
void ngt_log_set_file_name(char *name)
{
	memset(log_info.full_name, 0, sizeof(log_info.full_name));
	sprintf(log_info.full_name, "%s/%s", LOG_FILE_PATH, name);
}
#endif

void ngt_log_set_level(int cmd, int level)
{
	if (cmd == 0) {	/* clear */
		log_info.dlevel &= ~level;
	} else {	/* set */
		log_info.dlevel |= level;
	}
}

int ngt_log_get_level()
{
	return log_info.dlevel;
}

void init_ngt_log(const char *name, int level, unsigned char max, int size)
{
	ngt_log_init_file_descriptor();

	memset(&log_info, 0, sizeof(struct ngt_log_info_t));

	strcpy(log_info.name, name);
	sprintf(log_info.full_name, "%s/%s", LOG_FILE_PATH, name);

    if (max >= MAX_LOG_FILES) {
        log_info.max_files = MAX_LOG_FILES;
    } else {
        log_info.max_files = max == 0 ? DEFAULT_LOG_FILES : max;
    }

    if (size >= MAX_LOG_FILE_SIZE) {
        log_info.max_file_size = MAX_LOG_FILE_SIZE;
    } else {
        log_info.max_file_size = size == 0 ? DEFAULT_LOG_FILE_SIZE : size;
    }

	ngt_log_set_level(1, level);
	set_log_ff(0);  // OFF(0)
	set_fflush(0);  // OFF(0)
}

/* Caution : after using this function -> free memory */
char* url_decode( const char* str )
{

    int i, j = 0, len;
    char* tmp;
    char hex[3];

    len = strlen( str );
    hex[2] = 0;
    
    tmp = (char*)malloc( sizeof(char) * (len+1) );
    
    for( i = 0 ; i < len ; i++, j++ ){

        if( str[i] != '%' )
            tmp[j] = str[i];

        else{

            if( IS_ALNUM(str[i+1]) && IS_ALNUM(str[i+2]) && i < (len-2) ){

                hex[0] = str[i+1];
                hex[1] = str[i+2];
                tmp[j] = strtol( hex, NULL, 16 );

                i += 2;
                
            }
            else
                tmp[j] = '%';
            
        }
        
    }
    tmp[j] = 0;
    
    return tmp;
}
