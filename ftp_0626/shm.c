#include "ftp.h"

void
wait_sem(int semid)
{
    struct sembuf sops={0,0,0};
    semop(semid, &sops, 1);
}

int
lock_sem(int semid)
{
    struct sembuf sops = {0, +1, SEM_UNDO};
    return (semop(semid, &sops, 1));
}

int
unlock_sem(int semid)
{
    struct sembuf sops = {0, -1, SEM_UNDO};
    return (semop(semid, &sops, 1));
}

int
file_copy_cancel(void)
{
    pthread_mutex_lock(&status_lock);
    pthread_mutex_lock(&cancel_lock);
    if(process_status == 2 || process_status == 3)
    {
        cancel_flag = 1;
        process_status = CANCELING;
    }
    else
    {
        pthread_mutex_unlock(&status_lock);
        pthread_mutex_unlock(&cancel_lock);
        return -1;
    }
    pthread_mutex_unlock(&status_lock);
    pthread_mutex_unlock(&cancel_lock);
    return 0;
}

void
set_status(unsigned char status)
{
    pthread_mutex_lock(&status_lock);
    process_status = status;
    pthread_mutex_unlock(&status_lock);
    return;
}

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
 *       result : command completion result
 *       code : if shm_category = 1, code is error .
 *              if shm_category = 2, code is codestatus information .
 *   RETURN VALUE:
 *       Upon successful completion, main() shall return 0;
 *       otherwise, -1 shall be returned
 *   DESCRIPTION:
 *       step 1 :
 *       step 2 :
 *
 *********************************************************************/

int
info_return(int shm_category, int cmd_category ,unsigned char excute_flag,
                                    unsigned char result , unsigned char code)
{
    char* shm_addr = NULL;
    struct ST_SHM * ptr;
    struct ST_STATUS * sta_ptr;
    if(shm_category == 1)
    {
        if((shm_addr = (char* )shmat( cmd_shm_id , NULL , 0 )) == (char* )-1)
        {
            return -1;  /*need to investigate */
        }
        ptr = (struct ST_SHM *)shm_addr;
        //wait cmd_sharedmemory's semaphore
        wait_sem(cmd_sem_id);
        //lock cmd_sharedmemory's semaphore
        lock_sem(cmd_sem_id);
        switch (cmd_category)
        {
            case 0: //copy
                if(excute_flag == 1)
                {
                    ptr->copy_cmd.copy_re.excute_flag = excute_flag;
                }
                else if(excute_flag == 2)
                {
                    ptr->copy_cmd.copy_re.excute_flag = excute_flag;
                    ptr->copy_cmd.copy_re.result = result;
                    ptr->copy_cmd.copy_re.error = code;
                }
                else
                {
                     unlock_sem(cmd_sem_id);
                     return -3;
                }
                break;
            case 1: //cancel
                if(excute_flag == 1)
                {
                    ptr->cancel_cmd.cancel_re.excute_flag = excute_flag;
                }
                else if(excute_flag == 2)
                {
                    if(result == 1)
                    {
                        ptr->cancel_cmd.cancel_re.excute_flag = excute_flag;
                        ptr->cancel_cmd.cancel_re.result = result;
                        ptr->copy_cmd.copy_re.error = code;
                        ptr->copy_cmd.copy_re.excute_flag = excute_flag;
                        ptr->copy_cmd.copy_re.result = 2;
                    }
                    else if(result == 2)
                    {
                        ptr->cancel_cmd.cancel_re.excute_flag = excute_flag;
                        ptr->cancel_cmd.cancel_re.result = result;
                    }
                    else
                    {
                         unlock_sem(cmd_sem_id);
                         return -3;
                    }
                }
                break;
            case 2: //exit
                ptr->exit_cmd.exit_re.excute_flag = excute_flag;
                ptr->exit_cmd.exit_re.result = result;
                ptr->exit_cmd.exit_re.error = code;
                break;
            default:
                unlock_sem(cmd_sem_id);
                return -3;
        }
        unlock_sem(cmd_sem_id);
    }
    else if(shm_category == 2)  //process status
    {
        if((shm_addr = (char* )shmat( sta_shm_id , NULL , 0 )) == (char* )-1)
        {
            return -1;  /*need to investigate */
        }
        sta_ptr = (struct ST_STATUS * )shm_addr;

        //wait semaphore
        wait_sem(sta_sem_id);
        //lock semaphore
        lock_sem(sta_sem_id);
        sta_ptr->status = code;

        //unlock semaphore;
        unlock_sem(sta_sem_id);
    }
    else
        return -3;
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
 *       otherwise , return -1.
 *   DESCRIPTION:
 *       step 1 : estimate exit command
 *       step 2 : estimate copy command
 *       step 3 : estimate cancel command
 *
 *********************************************************************/

int
estimate_cmd(struct ST_SHM * ptr)
{
    pthread_mutex_lock(&cancel_lock);
    if(cancel_flag == 1)
    {
        pthread_mutex_unlock(&cancel_lock);
        return 100;
    }
    pthread_mutex_unlock(&cancel_lock);

    if((ptr->exit_cmd).exit_info.ftp_exit == 1)
    {
        (ptr->exit_cmd).exit_re.excute_flag = 1;
        return 2;
    }

    if(((ptr->copy_cmd).copy_info.itype != 0 )&&
       ((ptr->copy_cmd).copy_re.excute_flag == 0) )
    {
        if((ptr->cancel_cmd).cancel_info.cancel == 0)
        {
            (ptr->copy_cmd).copy_re.excute_flag = 1;
            return 0;
        }
        else if((ptr->cancel_cmd).cancel_info.cancel == 1)
        {
            (ptr->cancel_cmd).cancel_re.excute_flag = 2;
            (ptr->copy_cmd).copy_re.excute_flag = 2;
            return 100;
        }
    }

    if((ptr->cancel_cmd).cancel_info.cancel == 1)
    {
        if((ptr->copy_cmd).copy_re.excute_flag != 1)
        {
            (ptr->cancel_cmd).cancel_re.excute_flag = 2;
            return 100;
        }
        else
        {
            (ptr->cancel_cmd).cancel_re.excute_flag = 1;
            return 1;
        }
    }
    return -1;
}

void*
SHM_thread(void* arg)
{
    pthread_t fileget_tid = 0;
    char* shm_addr = NULL;
    struct ST_SHM * ptr;
    unsigned char fileget = 0;
    int opt = 0;
    struct COPY_INFO _copy_info;
    memset(&_copy_info, 0, sizeof(struct COPY_INFO));

    while( 1 )
    {
        if((shm_addr = (char* )shmat( cmd_shm_id , NULL , 0 )) == (char* )-1)
	    {
	        record_log(LPROCESSERROR, NULL, 0x29);
            return ((void *) -1);  /*need to investigate */
        }
        ptr = (struct ST_SHM *)shm_addr;
        //estimate command from the shared memory
        //wait semaphore
        wait_sem(cmd_sem_id);
        lock_sem(cmd_sem_id);
        opt = estimate_cmd(ptr);
        if(opt == 0)
        {
            memcpy(&_copy_info,&((ptr->copy_cmd).copy_info),sizeof(_copy_info));
        }
        //unlock
        unlock_sem(cmd_sem_id);
        switch( opt )
        {
            case 0:       //Get File thread
                pthread_mutex_lock(&cancel_lock);
                cancel_flag = 0;
                pthread_mutex_unlock(&cancel_lock);
	    		if( pthread_create( &fileget_tid , NULL , (void* )FILEGET_thread , (void* )&_copy_info ))
				{
				    record_log(LPROCESSERROR, NULL, 0x29);
					return ((void* ) -1);
				}
	    		break;
	    	case 1:		//Cancel command
                read_fileget_flag(&fileget);
                record_log(LCANCELSTART, NULL, LOK);
                if(fileget == 1)
                {
                    file_copy_cancel();
                }
				else if(fileget == 0)
				{
				    info_return(CMDSHM, CANCELCMD, 2, 1, 0);
				    record_log(LCANCELEND, NULL, LOK);
                }
	    		break;
	    	case 2:		//exit
                read_fileget_flag(&fileget);
                record_log(LPROCESSEXITSTART, NULL, LOK);
                if(fileget == 1)
                {
                    file_copy_cancel();
                    if( pthread_join(fileget_tid, NULL) != 0 )
                    {
                        info_return(CMDSHM, EXITCMD, 2, 2, 0x29);
                        record_log(LPROCESSERROR, NULL, 0x29);
                        return ((void* )-1);
                    }
                    info_return(CMDSHM, EXITCMD, 2, 2, 0x29);
                    record_log(LPROCESSEXIT, NULL, LOK);
                    return ((void* )-1);
                }
				else if(fileget == 0)
				{
				    info_return(CMDSHM, EXITCMD, 2, 1, 0);
				    record_log(LPROCESSEXIT, NULL, LOK);
				    return ((void* )0);
                }
	    	default:
	    		break;
        }
        sleep(10);
    }
    return ((void* ) 0);
}
