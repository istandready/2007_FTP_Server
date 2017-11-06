/*The program is a FTP Client for (NEC)SR-MGC copying *
 *DM-data from (NEC)PBX.                              */
#include "ftp.h"

char log_filename[64];
int FileSize = 2*1024*1024;
int ByteSize  = 128;
unsigned char   re_download = 3;
unsigned char   tcp_retry = 3;
unsigned char   timeout = 10;
int cmd_shm_id = 0;
int sta_shm_id = 0;
int cmd_sem_id = 0;
int sta_sem_id = 0;
int log_fp = 0;
unsigned char cancel_flag = 0;
unsigned char fileget_flag = 0;
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

/*****************************************************************
 *   NAME: FTP Client process
 *   PARAMETER: NONE
 *   RETURN VALUE:
 *       Upon successful completion, main() shall return 0;
 *       otherwise, -1 shall be returned
 *   DESCRIPTION:
 *       step 1 : set environment
 *       step 2 : create shread memory thread
 *
 ******************************************************************/
int
main(int argc, char** argv)
{
    int rev = 0;
    pthread_t shm_tid = 0;

    rev = get_env_var();
    if(rev)
        return -1;
    //set process status
    set_status(SETENV);
    rev = info_return(STASHM, 0, 0, 0, 0x51);
    if(rev)
    {
        return rev;
    }
    /*set environment for data copy and make a log file*/
	if((rev = set_env()) != 0)
	{
        return -1;
	}

	record_log(LSTARTUP, NULL, LOK);
    //set process status
    set_status(EMPTY);
    rev = info_return(STASHM, 0, 0, 0, 0x52);
    if(rev)
    {
        record_log(LPROCESSERROR, NULL, 0x29);
    }
    /*create shm_thread */
    if(pthread_create(&shm_tid,NULL,(void *)SHM_thread,NULL) != 0)
    {
        record_log(LPROCESSERROR, NULL, 0x29);
    }
    if(pthread_join(shm_tid,NULL) != 0)
    {
        record_log(LPROCESSERROR, NULL, 0x29);
    }

    record_log(LPROCESSEXIT, NULL, LOK);
    FtpCloseLogFile(log_fp);
    return 0;
}

