#include "ftp.h"
#include "shm.h"

unsigned long long dir_size = 0;
unsigned char dir_level = 0;
unsigned char re_download = 3;
unsigned char tcp_retry = 3;
unsigned char timeout = 10;
int sem_id = 0;
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

/*test code ------> start*/


int file_count = 0;
int control_retry = 0;
int data_retry = 0;
int recv_timeout_retry = 0;
int compare_retry  = 0;


/*test code ------> end*/
