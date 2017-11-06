#include "ftp.h"

int
FtpLogByteSizeSet(int Bytesize)
{
    if(Bytesize > MAXBZ)
        return 0;
    ByteSize = Bytesize;
        return 1;
}

void
get_current_time(char* time_str)
{
    time_t timep;
    struct tm * p;
    time(&timep);
    p = localtime(&timep);

    sprintf(time_str,"%d%d%d%d%d",1900+p->tm_year,p->tm_mon,p->tm_mday,p->tm_hour,p->tm_min);
    return;
}

void
get_current_date(char* date)
{
    time_t timep;
    struct tm * p;
    time(&timep);
    p = localtime(&timep);

    sprintf(date,"%d%d%d",1900+p->tm_year,p->tm_mon,p->tm_mday);
    return;
}

int
FtpCreatLogFile(char* filename)
{
    char stime[MAX];

    memset(&stime, '\0', MAX);
    int fd = 0;

    if(filename == NULL)
        return 0x29;
    get_current_date(stime);
    if(strlen(stime) <= 0)
        return 0x29;
    sprintf(filename, "%s%s%s", FTP_LOG, LOGNAME , stime);

    fd = open(filename, O_CREAT | O_APPEND);//problem here
    if(fd == -1)
	 	return 0x17;
    close(fd);
    return 0;
}

int
FtpOpenLogFile(char* filename)
{
    int iRet = 0;
    iRet = open(filename, O_RDWR);
    if(iRet < 0)
        return -1;
    return iRet;
}

int
FtpCloseLogFile(int fileid)
{
    int iRet = 0;

	struct flock lock;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
	lock.l_type = F_UNLCK;
    fcntl(fileid, F_SETLKW, &lock);
    iRet = close(fileid);
    if(iRet < 0)
        return iRet;
    return 0;
}

int
FtpCountBuff(char *buff)
{
    int iRet = 0;

	if(buff == NULL)
		return -1;

	iRet = sizeof(buff);

	return iRet;
}

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

    return -1;
}

int
spacecoute(char* buff)
{
    int iRet = 0;

    if(buff == NULL)
        return -1;

    iRet = strlen(buff);
    if(iRet <= 0)
        return -1;
    if(isalnum(buff[iRet-2])&&isalnum(buff[iRet-1])&& (iRet > ByteSize))
    {
        iRet--;
        while(isalnum(buff[iRet]))
            iRet--;
        buff[++iRet]=0;
        iRet = strlen(buff);
    }
    return iRet;
}

int
formatstr(char* buff)
{
    int iRet = 0;

	if(buff == NULL)
		return -1;

	iRet = strlen(buff);
	if(iRet <= 0)
        return -1;

	return iRet;

}

int
countenter(char *buff)
{
    int iRet = 0;
	int iResult = 0;

	if(buff == NULL)
		return -1;

	while(buff[iResult])
    {
       if(buff[iResult]=='\n')
	   	  iRet++;
	   iResult++;
    }
    return iRet;
}

int
count(char *buff)
{
    int iRet = 0;
    int iResult = 0;

	if(buff == NULL)
		return -1;

	iRet = sizeof(buff);
	if(iRet <= 0)
        return -1;
    iRet--;
	while(buff[iRet]!='\n')
	{
	    iResult++;
	    iRet--;
    }
	return iResult;
}

int
FWrite(int fd, char* buff)
{
    int iRet = 0;
    int i = 0;
    int ibyte = 0;
    char str[MAXBZ];
    char word[MAXBZ*2];
    char pp[MAX];
    FILE *fp;
    char fill[MAXBZ];

    memset(str, '\0', MAXBZ);
    memset(word, '\0', MAXBZ*2);
    memset(pp, '\0', MAX);
    memset(fill, '\0', MAXBZ);

    if(buff == NULL)
        return -1;

    fp = fdopen(fd,"a+");

    if(fp == NULL)
        return -1;

    fseek(fp,0,SEEK_END);

    iRet = formatstr(buff);
    if(iRet>ByteSize)
    {

        while(ibyte != strlen(buff))
        {
            memset(str, 0, sizeof(str));
            i = strlen(buff) - ibyte;
            if(i < ByteSize)
            {
                memcpy(str,&(buff[ibyte]),i);
            }
            else
            {
                memcpy(str,&(buff[ibyte]),ByteSize);
            }
            ibyte += strlen(str);
            iRet = write(fd,str,strlen(str));
            write(fd,"\n",1);
            if(iRet == -1)
                return -1;

        }
    }
    else
    {
        iRet = 0;
        iRet = write(fd,buff,strlen(buff));
        if( iRet  == -1)
            return -1;
        write(fd,"\n",1);
        if( iRet  == -1)
            return -1;
    }
    return 0;
}

int
FtpLogWrite(int* fd, char* buff)
{
     int iRet = 0;
	 int iResult = 0;
	 char filename[MAX];
	 struct flock lock;
	 memset(filename,0, sizeof(filename));
     lock.l_whence = SEEK_SET;
     lock.l_start = 0;
     lock.l_len = 0;
	 lock.l_type =  F_WRLCK;

     if(fd == NULL)
	 	return -1;
	 if(buff == NULL)
	 	return -1;
     if( (iRet = fcntl(*fd, F_SETLKW, &lock)) < 0 )
        return -1;
	 iRet = FtpCountBuff(buff);
	 iResult = FtpCountFile(*fd);

     if(iRet+iResult > FileSize)
     {
        iRet=FtpCloseLogFile(*fd);
        //error
		if(iRet == -1)
	    {
           return -1;
		}

        iRet = FtpCreatLogFile(filename);
		if(iRet == -1 )
	    {
            return -1;
		}

		*fd = FtpOpenLogFile(filename);
        //error
		if(*fd == -1)
	    {
           return -1;
		}
		iResult = FWrite(*fd,buff);

    }
    else
    {
        iResult = FWrite(*fd,buff);
    }

    if(iResult != -1)
        return 0;
    return iRet;
}

void
record_log(int opt, char* target_name, unsigned char result)
{
    char stime[MAX];
    char log_info[512];
    memset(log_info, '\0', 512);
    memset(stime, '\0', MAX);

    get_current_time(stime);
    switch(opt)
    {
        case LSTARTUP:
            sprintf(log_info, "%s%s%s%s\n", TIMECHAR, stime, OPTCHAR, "ftp process start-up");
            write_log(log_info);
            break;
        case LPROCESSEXITSTART:
            sprintf(log_info, "%s%s%s%s\n", TIMECHAR, stime, OPTCHAR, "process exit start");
            write_log(log_info);
            break;
        case LPROCESSEXIT:
            if(result == 0)
            {
                sprintf(log_info, "%s%s%s%s\n", TIMECHAR, stime, OPTCHAR, "process exit");
            }
            else
            {
                sprintf(log_info, "%s%s%s%s%s%s%0x\n", TIMECHAR, stime, OPTCHAR, "process exit",
                                                RESULTCHAR, LERROR, result);
            }
            write_log(log_info);
            break;
        case LCOPYSTART:
            sprintf(log_info, "%s%s%s%s%s%s\n", TIMECHAR, stime, OPTCHAR, "copy start",
                                                FILENAMECHAR, target_name);
            write_log(log_info);
            break;
        case LCOPYEND:
            if(result == 0)
            {
                sprintf(log_info, "%s%s%s%s%s%s%s%s\n", TIMECHAR, stime, OPTCHAR, "copy end",
                                FILENAMECHAR, target_name, RESULTCHAR, SUCCESSED);
            }
            else
            {
                sprintf(log_info, "%s%s%s%s%s%s%s%s%0x\n", TIMECHAR, stime, OPTCHAR, "copy end",
                                FILENAMECHAR, target_name, RESULTCHAR, LERROR, result);
            }
            write_log(log_info);
            break;
        case LCANCELSTART:
            sprintf(log_info, "%s%s%s%s\n", TIMECHAR, stime, OPTCHAR, "cancel start");
            write_log(log_info);
            break;
        case LCANCELEND:
            if(result == 0)
            {
                sprintf(log_info, "%s%s%s%s%s%s\n", TIMECHAR, stime, OPTCHAR, "cancel end",
                                RESULTCHAR, SUCCESSED);
            }
            else
            {
                sprintf(log_info, "%s%s%s%s%s%s%0x\n", TIMECHAR, stime, OPTCHAR, "cancel end",
                                RESULTCHAR, LERROR, result);
            }
            write_log(log_info);
            break;
        case LPROCESSERROR:
                sprintf(log_info, "%s%s%s%s%s%s%0x\n", TIMECHAR, stime, OPTCHAR, "process error occured",
                                RESULTCHAR, LERROR, result);
            write_log(log_info);
            break;
        default:
            break;
    }
    return;
}

void
write_log(char* input_info)
{
    struct flock lock;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
	lock.l_type = F_UNLCK;

    if(input_info == NULL)
        return;

    FtpLogWrite(&log_fp, input_info);
    fcntl(log_fp, F_SETLKW, &lock);
    return;
}

int
get_sem(key_t key, int* semid)
{
    union semun sem ;
    sem.val = 0;
    *semid = semget(key, 0, 0);
    if (*semid == -1)
        return -1;
    return 0;
}

int
get_env_var(void)
{
    int rev = 0;
    int cmd_sem = 0 , sta_sem = 0;
    int shmid = 0 ,sta_id = 0;
    key_t shm_key = 0 , sta_key = 0;

    if((shm_key = ftok( FTP_PATH , 0 )) == (key_t)-1) //ftp's key create
    {
        return -1;
    }
    if((shmid = shmget( shm_key , sizeof(struct ST_SHM) , 0 )) == -1)
    {
        return -1;
    }
    cmd_shm_id = shmid;

    if((sta_key = ftok( FTP_PATH , 1 )) == (key_t)-1) //ftp's key create
    {
        return -1;
    }
    if((sta_id = shmget( sta_key , sizeof(struct ST_STATUS) , 0 )) == -1)
    {
        return -1;
    }
    sta_shm_id = sta_id;

    rev = get_sem(shm_key, &cmd_sem);
    if(rev)
        return rev;
    cmd_sem_id = cmd_sem;

    rev = get_sem(sta_key, &sta_sem);
    if(rev)
        return rev;
    sta_sem_id = sta_sem;

    if(pthread_mutex_init(&status_lock, NULL) != 0)
    {
        return 0x29;
    }
    if(pthread_mutex_init(&cancel_lock, NULL) != 0)
    {
        return 0x29;
    }
    if(pthread_mutex_init(&fileget_lock, NULL) != 0)
    {
        return 0x29;
    }
    return 0;
}


int
create_tmpdir(void)
{
    int rev = 0;

    rev = make_dir(TEMP_DIR1);
    if(rev)
        return rev;

    rev = make_dir(TEMP_DIR2);
    if(rev)
        return rev;

    return 0;
}

int
make_logdir(void)
{
    int rev = 0;
    rev = make_dir(FTP_LOG);
    if(rev)
        return rev;
    return 0;
}

int
set_env(void)
{
    int rev = 0; /*return's value*/
    char filename[MAX];
    memset(filename, '\0', MAX);
    rev = make_logdir();
    if(rev)
        return rev;

    rev = FtpCreatLogFile(filename);
    if(rev)
        return rev;
    log_fp = FtpOpenLogFile(filename);
    if(log_fp < 0)
        return 0x17;
    rev = create_tmpdir();
    if(rev)
        return rev;
    return 0;
}
