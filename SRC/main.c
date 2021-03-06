/*The program is a FTP Client for (NEC)SR-MGC copying *
 *DM-data from (NEC)PBX.                              */
#include "ftp.h"
#include "shm.h"
#include "com.h"
/*****************************************************************
 *   NAME: FTP Client process
 *   PARAMETER: NONE
 *   RETURN VALUE:
 *       Upon successful completion, main() shall return 0;
 *       otherwise, NG shall be returned
 *   DESCRIPTION:
 *       FTP process's main function
 *
 ******************************************************************/
int
main(int argc, char** argv)
{
    int rev = 0;
    pthread_t shm_tid = 0;
    /*set environment variables*/
    rev = get_env_var();
    if(rev)
    {
        srm_dtcshm(sr_shmdat.adr);
        return NG;
    }
    /*set process status*/
    set_status(SETENV);
    info_return(STASHM, 0, 0, 0, 0x51);

    /*set environment for data copy and make a log file*/
    if((rev = set_env()) != 0)
    {
        srm_dtcshm(sr_shmdat.adr);
        return NG;
    }
    /*log start-up information*/
    record_log(LSTARTUP, NULL, LOK);
    /*set process status*/
    set_status(EMPTY);
    rev = info_return(STASHM, 0, 0, 0, 0x52);
    if(rev)
    {
        srm_dtcshm(sr_shmdat.adr);
        write_log("[ERROR INFO] operate status shm error occured in main()\n");
        record_log(LSHMERROR, NULL, 0x29);
    }
    /* create shm_thread */
    if(pthread_create(&shm_tid, NULL, (void *)SHM_thread, NULL) != 0)
    {
        srm_dtcshm(sr_shmdat.adr);
        write_log("[ERROR INFO] create shm_thread error occured in main()\n");
        record_log(LPROCESSERROR, NULL, 0x29);
        FtpCloseLogFile(log_fp);
        return NG;
    }
    /*waiting the termation of SHM_thread*/
    if(pthread_join(shm_tid, NULL) != 0)
    {
        srm_dtcshm(sr_shmdat.adr);
        write_log("[ERROR INFO] join shm_thread error occured in main()\n");
        record_log(LPROCESSERROR, NULL, 0x29);
        FtpCloseLogFile(log_fp);
        reclaim_resource(DELEDIR, NULL);
        reclaim_resource(DELELIST, NULL);
        return NG;
    }

    srm_dtcshm(sr_shmdat.adr);
    /*close log file descriptor*/
    FtpCloseLogFile(log_fp);
    /*reclaim resource*/
    reclaim_resource(DELEDIR, NULL);
    reclaim_resource(DELELIST, NULL);
    return OK;
}

