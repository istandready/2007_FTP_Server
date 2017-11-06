#include "ftp.h"
#include "shm.h"
char log_filename[MAX];
unsigned long long dir_size = 0;
unsigned char dir_level = 0;
const int ByteSize = 128;
unsigned char re_download = 3;
unsigned char tcp_retry = 3;
unsigned char timeout = 10;
int sem_id = 0;
int log_fp = 0;
unsigned char cancel_flag = 0;
unsigned char fileget_flag = 0;
unsigned char file_or_dir = 0;
/**************************
*    process status:
*    0 : empty
*    1 : set environment
*    2 : copy start
*    3 : copying
*    4 : update
*    5 : canceling
***************************/
unsigned char process_status = 0;

pthread_mutex_t status_lock;
pthread_mutex_t cancel_lock;
pthread_mutex_t fileget_lock;

struct store_list head_list;
struct file_list file_list;
int count = 0;
