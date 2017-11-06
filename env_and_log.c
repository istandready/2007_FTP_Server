#include "ftp.h"
#include "shm.h"
#include "srm_ipc.h"
#include <string.h>
#include <sys/syslog.h>
#include <stdarg.h>
/*****************************************************************
 *   NAME: write_log
 *   PARAMETER: input_info: log information
 *   RETURN VALUE:
 *       Upon successful completion, write_log() shall return OK;
 *       otherwise, NG shall be returned
 *   DESCRIPTION:
 *      Write log information to syslog
 *
 ******************************************************************/
int
write_log(const char* input_info, ...)
{
    va_list ap = 0;
    int priority = 0;
    int option = 0;
    int facility = 0;

    if(input_info == NULL)
        return NG;

    va_start(ap, input_info);

    priority = LOG_INFO;
    option = LOG_PID;
    facility = LOG_LOCAL4;
    /*the name of process: SR_COPY*/
    openlog("SR_FTP", option, facility);
    vsyslog(priority, input_info, ap);
    closelog();

    va_end(ap);

    return OK;
}

/*****************************************************************
 *   NAME: get_sem
 *   PARAMETER: key:ipc's key
 *              semid:shared memory's semaphore
 *   RETURN VALUE:
 *       Upon successful completion, get_sem() shall return OK;
 *       otherwise, NG shall be returned
 *   DESCRIPTION:
 *      get semaphore by shared memory's key
 *
 ******************************************************************/
int
get_sem(key_t key, int* semid)
{
    union semun sem;
    if(semid == NULL)
        return NG;
    sem.val = 0;
    *semid = semget(key, 0, 0);
    if(*semid == NG)
        return NG;

    return OK;
}
/*****************************************************************
 *   NAME: get_env_var
 *   PARAMETER: NONE
 *   RETURN VALUE: NONE
 *       Upon successful completion, get_env_var() shall return OK;
 *       otherwise, NG shall be returned
 *   DESCRIPTION:
 *      set and get environment variables
 *
 ******************************************************************/
int
get_env_var(void)
{
    int rev = 0;
    int shm_sem_id = 0;

    rev = srm_getipc(); /*attach shared memory*/
    if(rev)
        return NG;

    /*test code ------> start*/
    #ifdef _TEST_

    rev = test_getipc();
    if(rev)
        return NG;

    #endif
    /*test code ------> end*/

    /*get semaphore*/
    rev = get_sem(sr_shmdat.key, &shm_sem_id);
    if(rev)
        return NG;
    /*save semaphore value to globle variable*/
    sem_id = shm_sem_id;

    /*thread lock*/
    if(pthread_mutex_init(&status_lock, NULL) != 0)
    {
        return NG;
    }

    if(pthread_mutex_init(&cancel_lock, NULL) != 0)
    {
        return NG;
    }

    if(pthread_mutex_init(&fileget_lock, NULL) != 0)
    {
        return NG;
    }

    return OK;
}

/*****************************************************************
 *   NAME: create_tmpdir
 *   PARAMETER: NONE
 *   RETURN VALUE:
 *       OK   : creat tmpdir success
 *       other: creat tmpdir fail
 *   DESCRIPTION:
 *      Create directroy for saving temp files
 *
 ******************************************************************/
int
create_tmpdir(void)
{
    int rev = 0;

    /*if there are temp files in the temp dir, delete them*/
    reclaim_resource(DELEDIR, NULL);

    /*Create TEMP_DIR1 */
    rev = make_dir(TEMP_DIR1);
    if(rev)
        return rev;
    /*Create TEMP_DIR2 */
    rev = make_dir(TEMP_DIR2);
    if(rev)
        return rev;

    return OK;
}

/*****************************************************************
 *   NAME: set_env
 *   PARAMETER: NONE
 *   RETURN VALUE:
 *       OK   : set log and env success
 *       other: set log and env fail
 *   DESCRIPTION:
 *      Set process's environment
 *
 ******************************************************************/
int
set_env(void)
{
    int rev = 0; /*return's value*/

    rev = create_tmpdir();
    if(rev)
    {
        record_log(LPROCESSERROR, NULL, 0x29);
        return NG;
    }
    return OK;
}

/*****************************************************************
 *   NAME: record_log
 *   PARAMETER:
 *            opt:option
 *    target_name:target of the copy
 *         result:copy result
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *      Log records
 *
 ******************************************************************/
void
record_log(int opt, char* target_name, unsigned char result)
{
    char log_info[256];
    memset(log_info, '\0', 256);

    switch(opt)
    {
        /*process start up*/
        case LSTARTUP:
            sprintf(log_info, "%s%s\n", OPTCHAR, "ftp process start-up");
            write_log(log_info);
            break;
        /*process exit start*/
        case LPROCESSEXITSTART:
            sprintf(log_info, "%s%s\n", OPTCHAR, "process exit start");
            write_log(log_info);
            break;
        /*process exit */
        case LPROCESSEXIT:
            if(result == 0)
            {
                sprintf(log_info, "%s%s\n", OPTCHAR, "process exit");
            }
            else
            {
                sprintf(log_info, "%s%s%s%s%0x\n", OPTCHAR, "process exit",
                                                RESULTCHAR, LERROR, result);
            }
            write_log(log_info);
            break;
        /*data copy start*/
        case LCOPYSTART:
            sprintf(log_info, "%s%s%s%s\n", OPTCHAR, "copy start",
                                                FILENAMECHAR, target_name);
            write_log(log_info);
            break;
        /*data copy end*/
        case LCOPYEND:
            if(result == 0)
            {
                sprintf(log_info, "%s%s%s%s%s%s\n", OPTCHAR, "copy end",
                                FILENAMECHAR, target_name, RESULTCHAR, SUCCESSED);
            }
            else
            {
                sprintf(log_info, "%s%s%s%s%s%s%0x\n", OPTCHAR, "copy end",
                                FILENAMECHAR, target_name, RESULTCHAR, LERROR, result);
            }
            write_log(log_info);
            break;
        /*Cancel*/
        case LCANCELSTART:
            sprintf(log_info, "%s%s\n", OPTCHAR, "cancel start");
            write_log(log_info);
            break;
        /*cancel end*/
        case LCANCELEND:
            if(result == 0)
            {
                sprintf(log_info, "%s%s%s%s\n", OPTCHAR, "cancel end",
                                RESULTCHAR, SUCCESSED);
            }
            else
            {
                sprintf(log_info, "%s%s%s%s%0x\n", OPTCHAR, "cancel end",
                                RESULTCHAR, LERROR, result);
            }
            write_log(log_info);
            break;
        case LPROCESSERROR:
            sprintf(log_info, "%s%s%s%s%0x\n", OPTCHAR,
                            "process memory's or API's error occured", RESULTCHAR, LERROR, result);
            write_log(log_info);
            break;
        /*occure error when access sharedmemory*/
        case LSHMERROR:
            sprintf(log_info, "%s%s%s%s%0x\n", OPTCHAR,
                    "shared memory error occured", RESULTCHAR, LERROR, result);
            write_log(log_info);
            break;
        default:
            break;
    }
    return;
}
