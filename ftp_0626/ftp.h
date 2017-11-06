#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <netdb.h>
#include <sys/vfs.h>
#include <sys/sem.h>
#include <dirent.h>


#define TEMP_DIR1 "/var/ftp/DATA1"
#define TEMP_DIR2 "/var/ftp/DATA2"
#define FTP_LOG  "/usr/local/log/"
#define LOGNAME  "ftplog_"
#define FTP_PATH "/kong/"
#define PROJ_ID  'f'
#define BUFSIZE  512
#define SENDSIZE 256
#define ERROR    -1
#define ONEFILE  1
#define DIRE     2
#define BYTE1    1
#define INT_3    3
#define FIRST    1
#define SECOND   2
#define UNCOMPRESS 1
#define COMPRESS   2
#define SHUTDOWN 100
#define DUMMYSIZE  64
#define DUMMY0  0
#define DUMMY1  1
#define DUMMY2  2
#define DUMMY3  3
#define DUMMY4  4
#define DUMMY5  5
#define DATESIZE 16
#define CMDSHM  1
#define STASHM  2
#define COPYCMD 0
#define CANCELCMD 1
#define EXITCMD 2
#define DELCMD "rm -rf "
#define CPDIR  "cp -a "
#define CPFILE "cp -f "
#define CF_DIR "/root/kong"
#define EMPTY 0
#define SETENV 1
#define COPYSTART 2
#define COPYING 3
#define UPDATING 4
#define CANCELING 5
#define MAX 1024     //log filename max length
#define MAXBZ  1024
#define MAXFZ  10*1024*1024
#define TIMECHAR "[TIME] "
#define OPTCHAR " [OPERATION] "
#define FILENAMECHAR " [FILENAME] "
#define RESULTCHAR " [RESULT] "
#define LSTARTUP 0   //"process start-up "
#define LPROCESSEXITSTART 1   //"process exit start"
#define LPROCESSEXIT 2   //"process exit"
#define LCOPYSTART 3   //"data copy start "
#define LCOPYEND 4   //"data copy end "
#define LCANCELSTART 5   //"copy cancel "
#define LCANCELEND 6   //"copy cancel end "
#define LPROCESSERROR 7
#define LOK 0  //"sucessed "
#define LNG 1  //"NG "
#define SUCCESSED "success"
#define LERROR "NG "
#define DELELIST 0
#define CLOSESOCK 1
#define DELEDIR 2

union semun
{
    int val;
    struct semid_ds* buf;
    unsigned short* array;
};

struct list
{
    char data[256];
    struct list * next;
};

struct COPY_INFO
{
    char    userName[12];
    char    pwd[12];
    unsigned char   itype;
    char    ipAddress[15];
    unsigned short  port;
    char    FileName[64];
    char    Dir[30];
    unsigned short  max_length;
    unsigned char   re_download;
    unsigned char   tcp_retry;
    unsigned char   timeout;
};

struct RE
{
    unsigned char	excute_flag;
	unsigned char   result;
    unsigned char   error;
};

struct CANCEL_INFO
{
	unsigned char   cancel;
};

struct CAN_RE
{
    unsigned char	excute_flag;
	unsigned char   result;

};
struct EXIT_INFO
{
	unsigned char   ftp_exit;
};


struct ST_COPY               /*copy information*/
{
	struct COPY_INFO    copy_info;
    struct RE   copy_re;
};

struct ST_CANCEL            /*cancel information*/
{
	struct CANCEL_INFO  cancel_info;
    struct CAN_RE   cancel_re;
};

struct ST_EXIT               /*exit  information*/
{
	struct EXIT_INFO  exit_info;
    struct RE    exit_re;
};

struct ST_SHM
{
	struct ST_COPY  copy_cmd;     /*copy information*/
    struct ST_CANCEL    cancel_cmd;    /*cancel information*/
    struct ST_EXIT  exit_cmd;     /*exit  information*/
};

struct ST_SOCK
{
    int sid;
    struct sockaddr_in sin;
};

struct ST_DATASOCK
{
    int sid_data;
    int sid_file;
    int savefd;
    struct sockaddr_in sin_data;
};

struct COPY_DATA
{
    char    username[13];
    char    pwd[13];
    unsigned char   itype;
    char    ipaddress[16];
    unsigned short  port;
    char    filename[65];
    char    dir[31];
    unsigned short  max_length;
    unsigned char   re_download;
    unsigned char   tcp_retry;
    unsigned char   timeout;
};

struct ST_REMOTEFILEINFO
{
	char *			name_;
	unsigned long	size_;
	time_t			uptime_;
	char			isFile_;
};

struct ST_STATUS
{
    unsigned char status ;
};

struct store_list
{
    struct list * head;
    struct store_list * next;
};

extern char log_filename[64];
extern int FileSize;
extern int ByteSize;
extern struct store_list head_list;
extern unsigned char process_status;
extern int cmd_shm_id;
extern int sta_shm_id;
extern int cmd_sem_id;
extern int sta_sem_id;
extern int log_fp;
extern unsigned char cancel_flag;
extern unsigned char fileget_flag;
extern unsigned char  timeout;
extern unsigned char  re_download;
extern unsigned char  tcp_retry;
extern pthread_mutex_t status_lock;
extern pthread_mutex_t cancel_lock;
extern pthread_mutex_t fileget_lock;
extern void* SHM_thread(void* arg);
extern void* FILEGET_thread(void* arg);
extern int set_env(void);
extern int info_return(int shm_category, int cmd_category ,unsigned char excute_flag,unsigned char result , unsigned char code);
extern int make_temp_dir(char* filename);
extern int file_compare(char* pathname_1, char* pathname_2);
extern int local_size(unsigned long file_size);
extern int analyse_list(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr, struct list * head, char* pwd);
extern int traversal_dir(char* name, struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr);
extern int get_file(char* filename, struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr);
extern int download_a_file(char* filename, unsigned long file_size, struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr);
extern int creat_socket(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr);
extern int lock_semaphore(int semid);
extern int unlock_semaphore(int semid);
extern int cf_update(char* pathname, char* cf_pathname);
extern int dir_delete(char* pathname);
extern int accept_list(struct ST_DATASOCK * datasock_ptr, char* dir_name, unsigned short length, struct list * head);
extern int file_update(char* source_path, char* destination_path);
extern int delete_list(struct list * head);
extern int get_env_var(void);
extern void set_status(unsigned char);
extern void read_status(unsigned char* status);
extern int estimate_cancel(void);
extern void copy_shutdown(int flag, struct ST_SOCK * sock_ptr);
extern int make_dir(char* pathname);
extern void set_fileget_flag(unsigned char flag);
extern void read_fileget_flag(unsigned char* flag);
extern void get_current_time(char* time_str);
extern void write_log(char* input_info);
extern void record_log(int opt, char* target_name, unsigned char result);
extern int FtpCloseLogFile(int fileid);
extern void reclaim_resource(int opt, struct ST_SOCK * sock_ptr);
