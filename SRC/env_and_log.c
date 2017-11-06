#include "ftp.h"
#include "shm.h"
#include "srm_ipc.h"
/*****************************************************************
 *   NAME: get_current_time
 *   PARAMETER: time_str: The current time
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *      Get the current time
 *
 ******************************************************************/
int
get_current_time(char* time_str)
{
    time_t timep;
    struct tm * p = NULL;
    if(time_str == NULL)
        return 0x29;
    time(&timep);
    /*get the current time*/
    p = localtime(&timep);
    if(p == NULL)
    {
        write_log("[ERROR INFO] localtime() error occured in get_current_time()\n");
        return 0x29;
    }
    sprintf(time_str,"%dy%dm%dd%dh%dm",1900+p->tm_year,p->tm_mon,p->tm_mday,p->tm_hour,p->tm_min);
    return OK;
}
/*****************************************************************
 *   NAME: get_current_date
 *   PARAMETER: The current date
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *      Get the current date
 *
 ******************************************************************/
int
get_current_date(char* date)
{
    time_t timep;
    struct tm * p = NULL;
    if(date == NULL)
        return 0x29;
    time(&timep);
    /*Get the local time*/
    p = localtime(&timep);
    if(p == NULL)
    {
        write_log("[ERROR INFO] localtime() error occured in get_current_time()\n");
        return 0x29;
    }
    sprintf(date,"%dy%dm%dd",1900+p->tm_year,p->tm_mon,p->tm_mday);
    return OK;
}
/*****************************************************************
 *   NAME: CreatLogFile
 *   PARAMETER: filename: log file name
 *   RETURN VALUE:
 *      0x29: PARAMETER is NULL
 *      0x17: open log file fail
 *      0: creat log file success
 *   DESCRIPTION:
 *      Create log file
 *
 ******************************************************************/
int
CreatLogFile(char* filename)
{
    int fd = 0;
    unsigned char index = 1;
    if(filename == NULL)
        return 0x29;
    while(index < 255)
    {
        sprintf(filename, "%s_%d", log_filename, index);
        fd = open(filename, O_CREAT | O_EXCL);
        if(fd == NG)
        {
            if(errno == EEXIST)
            {
                index++;
                continue;
            }
            else
                return 0x17; /*create file NG*/
        }
        close(fd);
        return OK;
    }
    return 0x17; /*create file NG*/
}
/*****************************************************************
 *   NAME: FtpCreatLogFile
 *   PARAMETER: filename: file name
 *   RETURN VALUE:
 *      0x29: PARAMETER is NULL
 *      0x17: open log file fail
 *      0: creat log file success
 *   DESCRIPTION:
 *      Create log file
 *
 ******************************************************************/
int
FtpCreatLogFile(char* filename)
{
    int rev = 0;
    char stime[MAX];
    int fd = 0;
    memset(&stime, '\0', MAX);
    if(filename == NULL)
        return 0x29; /* memory or process's error occured */
    /*get current date*/
    rev = get_current_date(stime);
    if(rev)
        return rev;

    if(strlen(stime) <= 0)
    {
        write_log("[ERROR INFO] get_current_date() error occured in FtpCreatLogFile()\n");
        return 0x29; /* memory or process's error occured */
    }

    sprintf(filename, "%s%s%s", FTP_LOG, LOGNAME , stime);
    /*creat log file*/
    fd = open(filename, O_CREAT | O_APPEND);
    if(fd == NG)
         return 0x17; /* create file NG */
    close(fd);
    memcpy(log_filename, filename, strlen(filename));
    return OK;
}
/*****************************************************************
 *   NAME: FtpOpenLogFile
 *   PARAMETER: filename: log file name
 *   RETURN VALUE:
 *      NG   : open log file fail
 *      other: open log file success
 *   DESCRIPTION:
 *      Open log file
 *
 ******************************************************************/
int
FtpOpenLogFile(char* filename)
{
    int iRet = 0;
    if(filename == NULL)
        return NG;
    /*Open log file*/
    iRet = open(filename, O_RDWR);
    if(iRet < 0)
        return NG;
    return iRet;
}
/*****************************************************************
 *   NAME: FtpCloseLogFile
 *   PARAMETER: fileid: log file Descriptor
 *   RETURN VALUE:
 *      0    : close log file success
 *      other: close log file fail
 *   DESCRIPTION:
 *      Close log file
 *
 ******************************************************************/
int
FtpCloseLogFile(int fileid)
{
    int iRet = 0;
    struct flock lock;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_type = F_UNLCK;
    /*Unlock the files*/
    fcntl(fileid, F_SETLKW, &lock);
    /*Close log file*/
    if(fileid > 0)
    {
        iRet = close(fileid);
        if(iRet < 0)
            return iRet;
    }
    return OK;
}
/*****************************************************************
 *   NAME: FtpCountBuff
 *   PARAMETER: buff: Write a string
 *   RETURN VALUE:
 *      NG    : PARAMETER is NULL
 *      other : number of bytes buff
 *   DESCRIPTION:
 *      number of bytes buff
 *
 ******************************************************************/
int
FtpCountBuff(char* buff)
{
    int iRet = 0;
    if(buff == NULL)
        return NG;

    iRet = sizeof(buff);

    return iRet;
}
/*****************************************************************
 *   NAME: FtpCountFile
 *   PARAMETER: filed:log file Descriptor
 *   RETURN VALUE:
 *      NG    : PARAMETER is NULL
 *      other : log file size
 *   DESCRIPTION:
 *      Calculate the size of log files
 *
 ******************************************************************/
int
FtpCountFile(int filed)
{
    int iRet = 0;
    struct stat buf;

    iRet = fstat(filed,&buf);

    if(iRet == 0)
    {
        iRet = buf.st_size;
        return iRet;
    }

    return NG;
}
/*****************************************************************
 *   NAME: formatstr
 *   PARAMETER: buff:Write a string
 *   RETURN VALUE:
 *      NG    : PARAMETER is NULL
 *      other : The length of buff
 *   DESCRIPTION:
 *      The length of buff
 *
 ******************************************************************/
int
formatstr(char* buff)
{
    int iRet = 0;
    if(buff == NULL)
        return NG;

    iRet = strlen(buff);
    if(iRet <= 0)
        return NG;

    return iRet;
}

/*****************************************************************
 *   NAME: FWrite
 *   PARAMETER:
 *            fd: file log Descriptor
 *            buff : write a string
 *   RETURN VALUE:
 *      NG    : PARAMETER is NULL
 *      other : write log success
 *   DESCRIPTION:
 *      Write log
 *
 ******************************************************************/
int
FWrite(int fd, char* buff)
{
    int iRet = 0;
    int i = 0;
    int ibyte = 0;
    char str[MAXBZ];
    char word[MAXBZ*2];
    char pp[MAX];
    char fill[MAXBZ];

    memset(str, '\0', MAXBZ);
    memset(word, '\0', MAXBZ*2);
    memset(pp, '\0', MAX);
    memset(fill, '\0', MAXBZ);

    if(buff == NULL)
        return NG;
    /*
    fp = fdopen(fd,"a+");

    if(fp == NULL)
        return NG;
    */
    lseek(fd, 0, SEEK_END);
    /*bytes of size*/
    iRet = formatstr(buff);
    if(iRet > ByteSize)
    {

        while(ibyte != strlen(buff))
        {
            memset(str, 0, sizeof(str));
            i = strlen(buff) - ibyte;
            if(i < ByteSize)
            {
                memcpy(str, &(buff[ibyte]), i);
            }
            else
            {
                memcpy(str, &(buff[ibyte]), ByteSize);
            }
            ibyte += strlen(str);
            iRet = write(fd, str, strlen(str));
            if(iRet == NG)
                return NG;
            iRet = write(fd, "\n", 1);
            if(iRet == NG)
                return NG;
        }
    }
    else
    {
        iRet = 0;
        iRet = write(fd, buff, strlen(buff));
        if(iRet == NG)
            return NG;
        iRet = write(fd, "\n", 1);
        if(iRet == NG)
            return NG;
    }
    return OK;
}
/*****************************************************************
 *   NAME: FtpLogWrite
 *   PARAMETER:
 *             fd: log file Descriptor
 *           buff: write a string
 *   RETURN VALUE:
 *      NG    : PARAMETER is NULL
 *      -2    : lock file fail
 *      other : write log success
 *   DESCRIPTION:
 *      Write log
 *
 ******************************************************************/
int
FtpLogWrite(int* fd, char* buff)
{
     int iRet = 0;
     int iResult = 0;
     int retry_ind = 0;
     char filename[MAX + 4];
     struct flock lock;
     memset(filename, 0, sizeof(filename));
     lock.l_whence = SEEK_SET;
     lock.l_start = 0;
     lock.l_len = 0;
     lock.l_type =  F_WRLCK;

     if(fd == NULL)
         return NG;
     if(buff == NULL)
         return NG;

     for(retry_ind = 0; retry_ind < 11; retry_ind++)
     {
         if(retry_ind == 10)
            return -2; /*log file is locked*/
         if((iRet = fcntl(*fd, F_SETLK, &lock)) >= 0)
            break;
     }
     /*log_file_lock has not been unlocked*/
     /*bytes of buff*/
     iRet = FtpCountBuff(buff);
     /*bytes of log file*/
     iResult = FtpCountFile(*fd);
     /* log file over FILESIZE */
     if(iRet+iResult > FILESIZE)
     {
        /*Close log file*/
        iRet = FtpCloseLogFile(*fd);
        /*error*/
        if(iRet == NG)
           return NG;
        /*Creat new log file*/
        iRet = CreatLogFile(filename);
        if(iRet)
            return NG;
         /*Open new log file*/
        *fd = FtpOpenLogFile(filename);
        /*error*/
        if(*fd == NG)
           return NG;
        /*Write log*/
        iResult = FWrite(*fd, buff);
    }
    else
        iResult = FWrite(*fd, buff);

    if(iResult != NG)
        return OK;

    return NG;
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
/*****************************************************************
 *   NAME: write_log
 *   PARAMETER: input_info: input log information
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *      Write log
 *
 ******************************************************************/
void
write_log(char* input_info)
{
    int rev = 0;
    char stime[16];
    char log_str[280];
    struct flock lock;
    memset(stime, '\0', 16);
    memset(log_str, '\0', 280);
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_type = F_UNLCK;

    rev = get_current_time(stime);
    if(rev)
        return;

    sprintf(log_str, "%s%s %s", TIMECHAR, stime, input_info);
    /*Write log*/
    rev = FtpLogWrite(&log_fp, log_str);
    if(rev == -2)
        return;
    fcntl(log_fp, F_SETLK, &lock);
    return;
}
/*****************************************************************
 *   NAME: get_sem
 *   PARAMETER: key:get semaphore
 *            semid:shared memory's semaphore
 *   RETURN VALUE: NONE
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
 *   DESCRIPTION:
 *      set and get environment variables
 *
 ******************************************************************/
int
get_env_var(void)
{
    int rev = 0;
    int shm_sem_id = 0;
   // int shmid = 0;
   // key_t shm_key = 0;

    rev = srm_getipc();
    if(rev)
        return NG;

    rev = get_sem(sr_shmdat.key, &shm_sem_id);
    if(rev)
        return NG;

    sem_id = shm_sem_id;

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
 *       0    : creat tmpdir success
 *       other: creat tmpdir fail
 *   DESCRIPTION:
 *      Create tmp dir
 *
 ******************************************************************/
int
create_tmpdir(void)
{
    int rev = 0;

    /*if there are temp files in the temp dir, delete them*/
    reclaim_resource(DELEDIR, NULL);

    /*Creat TEMP_DIR1 */
    rev = make_dir(TEMP_DIR1);
    if(rev)
        return rev;
    /*Creat TEMP_DIR2 */
    rev = make_dir(TEMP_DIR2);
    if(rev)
        return rev;

    return OK;
}
/*****************************************************************
 *   NAME: make_logdir
 *   PARAMETER: NONE
 *   RETURN VALUE:
 *       0    : creat logdir success
 *       other: creat logdir fail
 *   DESCRIPTION:
 *      Creat log dir
 *
 ******************************************************************/
int
make_logdir(void)
{
    int rev = 0;
    /*Creat FTPLOG dir*/
    rev = make_dir(FTP_LOG);
    if(rev)
        return rev;
    return OK;
}
/*****************************************************************
 *   NAME: set_env
 *   PARAMETER: NONE
 *   RETURN VALUE:
 *       0    : set log and env success
 *       other: set log and env fail
 *   DESCRIPTION:
 *      Set log and env
 *
 ******************************************************************/
int
set_env(void)
{
    int rev = 0; /*return's value*/
    char filename[MAX];
    memset(filename, '\0', MAX);
    /*Creat log dir*/
    rev = make_logdir();
    if(rev)
        return NG;
    /*Creat log file*/
    rev = FtpCreatLogFile(filename);
    if(rev)
        return NG;
    /*Open log file*/
    log_fp = FtpOpenLogFile(filename);
    if(log_fp < 0)
        return NG;
    rev = create_tmpdir();
    if(rev)
    {
        record_log(LPROCESSERROR, NULL, 0x29);
        return NG;
    }
    return OK;
}
