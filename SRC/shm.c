#include "ftp.h"
#include "shm.h"
//#include ""
/*****************************************************************
 *   NAME: wait_sem
 *   PARAMETER:
 *          semid: share memory's semaphore id
 *          flag: WAIT or NOWAIT
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *       not wait until semaphore to be unlocked
 *
 ******************************************************************/
int
wait_sem(int semid, int flag)
{
    int rev = 0;
    if(flag == WAIT)
    {
        struct sembuf sops = {0, 0, 0};
        rev = semop(semid, &sops, 1);
        if(rev == -1)
            return -1;
        return 0;
    }
    else if(flag == NOWAIT)
    {
        struct sembuf sops = {0, 0, IPC_NOWAIT};
        rev = semop(semid, &sops, 1);
        if(rev == -1 && errno == EAGAIN)
            return 1; /*lock fail*/
        else if(rev == -1 && errno != EAGAIN)
            return -1;

        return 0;
    }
    else
        return -1;
}
/*****************************************************************
 *   NAME: lock_sem
 *   PARAMETER: semid:share memory's semaphore id
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *       make use of semaphore to lock shared memory
 *
 ******************************************************************/
int
lock_sem(int semid)
{
    struct sembuf sops = {0, +1, SEM_UNDO};
    return (semop(semid, &sops, 1));
}
/*****************************************************************
 *   NAME: unlock_sem
 *   PARAMETER: semid:share memory's semaphore id
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *       make use of semaphore to unlock shared memory
 *
 ******************************************************************/
int
unlock_sem(int semid)
{
    struct sembuf sops = {0, -1, SEM_UNDO};
    return (semop(semid, &sops, 1));
}

/*****************************************************************
 *   NAME: check_info
 *   PARAMETER:
 *            info_ptr   :copy information
 *   RETURN VALUE:
 *               0: infomation valid
 *          other : infomation invalid
 *   DESCRIPTION:
 *        Check information
 *
 ******************************************************************/
int
check_info(struct COPY_INFO * info_ptr)
{
    if(info_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(strlen(info_ptr->userName) == 0)
        return 0x2a; /* username is not correct */
    if(strlen(info_ptr->pwd) == 0)
        return 0x2b; /* password is not correct */
    if(info_ptr->itype != UNCOMPRESS && info_ptr->itype != COMPRESS)
        return 0x2f; /* mode of transimission is not correct */
    if(strlen(info_ptr->ipAddress) == 0)
        return 0x0c; /*ip address is not correct*/
    if(!(info_ptr->port >= 60061 && info_ptr->port <= 60069))
        return 0x2c; /* port is not correct */
    if(!(info_ptr->max_length >= 1024 && info_ptr->max_length <= 4096))
        info_ptr->max_length = 4096;
    if(!(info_ptr->re_download <= 5))
        info_ptr->re_download = 3;
    if(!(info_ptr->tcp_retry <= 5))
        info_ptr->tcp_retry = 3;
    if(!(info_ptr->timeout >= 5 && info_ptr->timeout <= 30))
        info_ptr->timeout = 10;
    if(strlen(info_ptr->FileName) == 0 && strlen(info_ptr->Dir) == 0)
        return 0x2d; /* directory is not correct */
    if(strlen(info_ptr->FileName) != 0 )
    {
        if(strlen(info_ptr->Dir) != 0)
            file_or_dir = DIRE;
        else
            file_or_dir = ONEFILE;
    }
    if(strlen(info_ptr->Dir) != 0)
    {
        file_or_dir = DIRE;
        if(info_ptr->Dir[strlen(info_ptr->Dir)-1] == '/')
            info_ptr->Dir[strlen(info_ptr->Dir)-1] = '\0';
    }
    return 0;
}

/*****************************************************************
 *   NAME: file_copy_cancel
 *   PARAMETER: NONE
 *   RETURN VALUE: 0: execute file copy cancel
 *                -1: can't execute file copy cancel
 *   DESCRIPTION:
 *       set copy cancel flag
 *
 ******************************************************************/
int
file_copy_cancel(void)
{
    unsigned char status = 0;
    read_status(&status);
    if(status == COPYSTART || status == COPYING)
    {
        pthread_mutex_lock(&cancel_lock);
        cancel_flag = 1;
        pthread_mutex_unlock(&cancel_lock);
        set_status(CANCELING);
    }
    else
    {
        return -1;
    }
    return 0;
}

/*****************************************************************
 *   NAME: set_status
 *   PARAMETER: status: ftp client status
 *   RETURN VALUE:NONE
 *   DESCRIPTION:
 *       set ftp clietn status
 *
 ******************************************************************/
void
set_status(unsigned char status)
{
    pthread_mutex_lock(&status_lock);
    process_status = status;
    pthread_mutex_unlock(&status_lock);
    return;
}
/*****************************************************************
 *   NAME: read_status
 *   PARAMETER: status: ftp clietn status
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *       read ftp clietn status
 *
 ******************************************************************/
void
read_status(unsigned char* status)
{
    pthread_mutex_lock(&status_lock);
    *status = process_status;
    pthread_mutex_unlock(&status_lock);
    return;
}

/*********************************************************************
 *   NAME: return result to shared memory
 *   PARAMETER:
 *       shm_category : 1: command shared memory 2: status shared memory
 *       cmd_category :
 *       result : command completion result
 *       code : if shm_category = 1, code is error code.
 *              if shm_category = 2, code is codestatus information .
 *   RETURN VALUE:
 *       Upon successful completion, main() shall return 0;
 *       otherwise, -1 shall be returned
 *   DESCRIPTION:
 *       write data to shared memory
 *
 *********************************************************************/
int
info_return(int shm_category, int cmd_category ,unsigned char excute_flag,
                                    unsigned char result , unsigned char code)
{
    //int rev = 0;
    //int i = 0;
    struct ST_SHM * ptr = NULL;

    if(sr_shmdat.adr == NULL)
        return -1;

    ptr = (struct ST_SHM *)sr_shmdat.adr;

    /*wait cmd_sharedmemory's semaphore*/
    /*retry 10 times*/
    //for(i = 0; i < 11;)
    //{
    //    if(i == 10)
    //    {
    //        return -1;
    //    }
    //   rev = wait_sem(sem_id, NOWAIT);
    //    if(rev == 1)
     //   {
    //        i++;
   //         continue;
    //    }
   //     else if(rev == 0)
   //         break;
   //     else
   //     {
   //         return -1;
    //    }
   // }
    /*lock cmd_sharedmemory's semaphore*/
   // lock_sem(sem_id);

    if(shm_category == 1)
    {

        switch (cmd_category)
        {
            case COPYCMD: /*copy command's return*/
                if(excute_flag == 2)
                {
                    ptr->copy_cmd.copy_re.excute_flag = excute_flag;
                    ptr->copy_cmd.copy_re.result = result;
                    ptr->copy_cmd.copy_re.error = code;
                }
                break;
            case CANCELCMD: /*cancel command's return*/
                if(excute_flag == 2)
                {
                    if(result == 1) /*if success*/
                    {
                        ptr->cancel_cmd.cancel_re.excute_flag = excute_flag;
                        ptr->cancel_cmd.cancel_re.result = result;
                        ptr->copy_cmd.copy_re.error = code;
                        ptr->copy_cmd.copy_re.excute_flag = excute_flag;
                        ptr->copy_cmd.copy_re.result = 2;
                    }
                    else if(result == 2) /*if error occured*/
                    {
                        ptr->cancel_cmd.cancel_re.excute_flag = excute_flag;
                        ptr->cancel_cmd.cancel_re.result = result;
                    }
                    else
                    {
                 //       unlock_sem(sem_id);
                        return -1;
                    }
                }
                break;
            case EXITCMD: /*exit command's return*/
                ptr->exit_cmd.exit_re.excute_flag = excute_flag;
                ptr->exit_cmd.exit_re.result = result;
                ptr->exit_cmd.exit_re.error = code;
                break;
            default:
            //    unlock_sem(sem_id);
                return -1;
        }
       // unlock_sem(sem_id);
    }
    else if(shm_category == 2)  /*process status*/
    {
        /*set prcess status */
        (ptr->ftp_status).status = code;

        /*unlock semaphore;*/
       // unlock_sem(sem_id);
    }
    else
        return -1;
    return 0;
}

/*********************************************************************
 *   NAME: estimate data in command shared memory.
 *   PARAMETER:
 *       struct ST_SHM * : data has been obtained from command shared memory
 *   RETURN VALUE:
 *       exit command should be excuted , return 2.
 *       copy command should be excuted , return 0.
 *       copy command and cancel command appear at once time,return 3.
 *       can't execute command , return 100.
 *       otherwise , return -1.
 *   DESCRIPTION:
 *       step 0 : estimate cancel flag
 *       step 1 : estimate exit command
 *       step 2 : estimate copy command
 *       step 3 : estimate cancel command
 *
 *********************************************************************/
int
estimate_cmd(struct ST_SHM * ptr)
{
    unsigned char fileget = 0;
    if(ptr == NULL)
        return INVALIDCMD;

    if((ptr->exit_cmd).exit_info.ftp_exit == 1)
    {
        (ptr->exit_cmd).exit_re.excute_flag = 1;
        return EXITCMD; /*execute exit command*/
    }

    pthread_mutex_lock(&cancel_lock);
    if(cancel_flag == 1)
    {
        pthread_mutex_unlock(&cancel_lock);
        return INVALIDCMD;
    }
    pthread_mutex_unlock(&cancel_lock);

    if(((ptr->copy_cmd).copy_info.itype != 0 )&&
       ((ptr->copy_cmd).copy_re.excute_flag == 0))
    {
        read_fileget_flag(&fileget);
        if(fileget == 1)
            return INVALIDCMD;
        if((ptr->cancel_cmd).cancel_info.cancel == 0)
        {
            (ptr->copy_cmd).copy_re.excute_flag = 1;
            return COPYCMD; /*execute copy command*/
        }
        else if((ptr->cancel_cmd).cancel_info.cancel == 1 &&
                    (ptr->cancel_cmd).cancel_re.excute_flag == 0)
        {
            (ptr->cancel_cmd).cancel_re.excute_flag = 2;
            (ptr->copy_cmd).copy_re.excute_flag = 2;
            (ptr->copy_cmd).copy_re.error = 0x70;
            return INVALIDCMD;
        }
    }

    if((ptr->cancel_cmd).cancel_info.cancel == 1 &&
                    (ptr->cancel_cmd).cancel_re.excute_flag == 0)
    {
        if((ptr->copy_cmd).copy_re.excute_flag != 1)
        {
            (ptr->cancel_cmd).cancel_re.excute_flag = 2;
            return INVALIDCMD;
        }
        else
        {
            (ptr->cancel_cmd).cancel_re.excute_flag = 1;
            return CANCELCMD; /*execute cancel command*/
        }
    }
    return INVALIDCMD;
}
/*****************************************************************
 *   NAME: SHM_thread
 *   PARAMETER: NONE
 *   RETURN VALUE: NONE
 *   DESCRIPTION:The thread  gets command from command shared memory
 *               each 10s.
 *
 ******************************************************************/
void*
SHM_thread(void* arg)
{
    pthread_t fileget_tid = 0;
    struct ST_SHM * ptr = NULL;
    unsigned char fileget = 0;
    int opt = 0;
    int rev = 0, ret = 0;
    struct COPY_INFO _copy_info;
    memset(&_copy_info, 0, sizeof(struct COPY_INFO));

    ptr = (struct ST_SHM *)sr_shmdat.adr;

    while( 1 )
    {
        read_fileget_flag(&fileget);
        if(fileget == 0 && fileget_tid != 0)
        {
             pthread_detach(fileget_tid);
             fileget_tid = 0;
        }
        /*wait semaphore*/
        //wait_sem(sem_id, WAIT);
        //lock_sem(sem_id);
        /*analyse information of command shared memory*/
        opt = estimate_cmd(ptr);
        if(opt == 0)
        {   /*get copy information from shared memory*/
            memcpy(&_copy_info, &((ptr->copy_cmd).copy_info), sizeof(_copy_info));
        }
        /*unlock*/
        //unlock_sem(sem_id);
        switch( opt )
        {
            case COPYCMD:       /*create File thread*/
                 /*check info whether is valid or not*/
                rev = check_info(&_copy_info);
                if(rev)
                {
                    /*return err code*/
                    ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
                    if(ret)
                        record_log(LSHMERROR, NULL, 0x29);

                    break;
                }
                pthread_mutex_lock(&cancel_lock);
                cancel_flag = 0;
                pthread_mutex_unlock(&cancel_lock);
                if(pthread_create(&fileget_tid , NULL, (void* )FILEGET_thread, (void* )&_copy_info ))
                {
                    info_return(CMDSHM, COPYCMD, 2, 2, 0x29);
                    write_log("[ERROR INFO] create fileget_thread error occured in SHM_thread()\n");
                    record_log(LPROCESSERROR, NULL, 0x29);
                }
                break;
            case CANCELCMD:        /*Cancel command*/
                record_log(LCANCELSTART, NULL, LOK);
                read_fileget_flag(&fileget);
                if(fileget == 1)
                {
                    rev = file_copy_cancel();
                    if(rev)
                    {
                        info_return(CMDSHM, CANCELCMD, 2, 2, 0);
                        record_log(LCANCELEND, NULL, 0x71);
                    }
                }
                else if(fileget == 0)
                {
                    info_return(CMDSHM, CANCELCMD, 2, 1, 0);
                    record_log(LCANCELEND, NULL, LOK);
                }
                break;
            case EXITCMD:        /*exit command*/
                record_log(LPROCESSEXITSTART, NULL, LOK);
                read_fileget_flag(&fileget);
                if(fileget == 1)
                {
                    file_copy_cancel();
                    if( pthread_join(fileget_tid, NULL) != 0 )
                    {
                        write_log("[ERROR INFO] join fileget_thread error occured in SHM_thread()\n");
                        info_return(CMDSHM, EXITCMD, 2, 2, 0x29);
                        record_log(LPROCESSERROR, NULL, 0x29);
                        return ((void* ) 0);
                    }
                    info_return(CMDSHM, EXITCMD, 2, 1, 0);
                    record_log(LPROCESSEXIT, NULL, LOK);
                    return ((void* ) 0);
                }
                else if(fileget == 0)
                {
                    info_return(CMDSHM, EXITCMD, 2, 1, 0);
                    record_log(LPROCESSEXIT, NULL, LOK);
                    return ((void* ) 0);
                }
            default:
                break;
        }
        sleep(10);
    }

    return ((void* ) 0);
}
