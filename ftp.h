#ifndef __FTP_H__
#define __FTP_H__

#define _TEST_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <netdb.h>
#include <sys/vfs.h>
#include <sys/sem.h>
#include <dirent.h>


#define TEMP_DIR1           "/var/ftp/DATA1"
#define TEMP_DIR2           "/var/ftp/DATA2"
#define TEMP_DIR            "/var/ftp/"
#define CF_DIR              "/root/kong"
#define BUFSIZE             512
#define SENDSIZE            256
#define ERROR               -1
#define ONEFILE             1
#define DIRE                2
#define BYTE1               1
#define INT_3               3
#define FIRST               1
#define SECOND              2
#define UNCOMPRESS          1
#define COMPRESS            2
#define SHUTDOWN            0x18
#define DUMMYSIZE           64
#define DUMMY0              0
#define DUMMY1              1
#define DUMMY2              2
#define DUMMY3              3
#define DUMMY4              4
#define DUMMY5              5
#define DATESIZE            16
#define CMDSHM              1
#define STASHM              2
#define COPYCMD             0
#define CANCELCMD           1
#define EXITCMD             2
#define DELCMD              "rm -rf "
#define CPDIR               "cp -a "
#define CPFILE              "cp -f "
#define EMPTY               0
#define SETENV              1
#define COPYSTART           2
#define COPYING             3
#define UPDATING            4
#define CANCELING           5
#define OPTCHAR             " [OPERATION] "
#define FILENAMECHAR        " [FILENAME] "
#define RESULTCHAR          " [RESULT] "
#define LSTARTUP            0   /*"process start-up "*/
#define LPROCESSEXITSTART   1   /*"process exit start"*/
#define LPROCESSEXIT        2   /*"process exit"*/
#define LCOPYSTART          3   /*"data copy start "*/
#define LCOPYEND            4   /*"data copy end "*/
#define LCANCELSTART        5   /*"copy cancel "*/
#define LCANCELEND          6   /*"copy cancel end "*/
#define LPROCESSERROR       7
#define LSHMERROR           8
#define LERROR              "NG "
#define LOK                 0   /*"sucessed "*/
#define LNG                 1   /*"NG "*/
#define SUCCESSED           "success"
#define DELELIST            0
#define CLOSESOCK           1
#define DELEDIR             2
#define MAXDIRLEVEL         4
#define WAIT                0
#define NOWAIT              1
#define INVALIDCMD          100
#define CANCELRET           -100
#define REDOWNLOAD          101
#define REACCEPT            102


union semun
{
    int val;
    struct semid_ds* buf;
    unsigned short* array;
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
                char    username[17];
                char    pwd[17];
    unsigned    char    itype;
                char    nop1[3];
                char    ipaddress[17];
    unsigned    short   port;
                short   nop2;
                char    filename[69];
                char    dir[33];
    unsigned    short   max_length;
                short   nop3;
    unsigned    char    re_download;
    unsigned    char    tcp_retry;
    unsigned    char    timeout;
                char    nop4;
};

struct ST_REMOTEFILEINFO
{
	char *			name_;
	unsigned long	size_;
	time_t			uptime_;
	char			isFile_;
};

/*the struct of saving the head address of the LIST information list*/
struct store_list
{
    struct list * head;
    struct store_list * next;
};

/*the struct of saving files information*/
struct file_list
{
    char file_pathname[256];
    struct file_list * next;
};

/*the struct of saving LIST information*/
struct list
{
    char data[256];
    struct list * next;
};


extern unsigned long long dir_size;
extern unsigned char dir_level;
extern struct store_list head_list;
extern struct file_list file_list;
extern unsigned char process_status;
extern int sem_id;
extern unsigned char cancel_flag;
extern unsigned char fileget_flag;
extern unsigned char  timeout;
extern unsigned char  re_download;
extern unsigned char  tcp_retry;
extern unsigned char  file_or_dir;
extern pthread_mutex_t status_lock;
extern pthread_mutex_t cancel_lock;
extern pthread_mutex_t fileget_lock;
extern void* SHM_thread(void* arg);
extern void* FILEGET_thread(void* arg);
extern int set_env(void);
extern int info_return(int shm_category, int cmd_category, unsigned char excute_flag, unsigned char result , unsigned char code);
extern int make_temp_dir(char* filename);
extern int file_compare(char* pathname_1, char* pathname_2);
extern int local_size(void);
extern int analyse_list(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr, struct list * head, char* pwd);
extern int traversal_dir(char* name, struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr);
extern int get_file(char* filename, struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr);
extern int download_a_file(char* filename, struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr);
extern int creat_socket(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr);
extern int lock_semaphore(int semid);
extern int unlock_semaphore(int semid);
extern int cf_update(char* pathname, char* cf_pathname);
extern int dir_delete(char* pathname);
extern int accept_list(struct ST_DATASOCK * datasock_ptr, char* dir_name, struct list * head);
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
extern int write_log(const char* input_info,...);
extern void record_log(int opt, char* target_name, unsigned char result);
extern void reclaim_resource(int opt, struct ST_SOCK * sock_ptr);
extern int list_cmd(struct ST_SOCK * sock_ptr);
extern int type_cmd(struct ST_SOCK * sock_ptr, char type_mode);
extern int cwd_cmd(struct ST_SOCK * sock_ptr, char* dir);
extern int pass_cmd(struct ST_SOCK * sock_ptr, char* pwd);
extern int user_cmd(struct ST_SOCK * sock_ptr, char* username);
extern int port_cmd(struct ST_SOCK * sock_ptr, unsigned char* addr,unsigned char* port);
extern int retr_cmd(struct ST_SOCK * sock_ptr, char* filename);
extern int send_command(struct ST_SOCK * sock_ptr, char* cmd);
extern int read_response(struct ST_SOCK * sock_ptr, char* response);
extern int get_replycode(char* response, int* reply_code);
extern int check_reply_code(int reply_code, unsigned char flag);
extern int quit_cmd(struct ST_SOCK * sock_ptr);
extern int get_list(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr, char* dir_name, struct list * head);
extern void delete_headlist(void);
extern int read_line(int sid, char* response, int* length);
extern int data_fin(struct ST_SOCK * sock_ptr);
extern int create_data_conn(struct ST_SOCK * sock_ptr, char* ip, unsigned short port,
                                unsigned char timeout, struct ST_DATASOCK * datasock_ptr);
extern void delete_filelist(void);
extern int set_file_info(char * rply_msg, struct ST_REMOTEFILEINFO * pFileInfo);
extern char* see_malloc(int size);
extern void see_free(char* addr);




/*test code ------> start*/


extern int file_count;
extern int control_retry;
extern int data_retry;
extern int compare_retry;
extern int recv_timeout_retry;
extern struct ST_TEST_SHM sr_test_shm;
extern int test_getipc(void);

struct ST_TEST
{
    int file_count;
    int control_retry;
    int data_retry;
    int compare_retry;
    int recv_timeout_retry;
    unsigned long long dir_size;
};

struct ST_TEST_SHM
{
    key_t  key;        /* IPC KEY-ID   */
    int    id;         /* IPC SHM-ID   */
    void*  adr;        /* IPC SHM-ADDR */
};
#endif

/*test code ------> end*/
