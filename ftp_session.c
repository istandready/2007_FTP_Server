#include "ftp.h"
#include "shm.h"
#include "com.h"
/*****************************************************************
 *   NAME: check_reply_code
 *   PARAMETER:
 *               reply_code : reply code from ftp server
 *               flag       : check code by different bound
 *   RETURN VALUE:
 *              OK:reply code is right code
 *              NG:reply code is not right code
 *            0x18:ftp server down
 *   DESCRIPTION:
 *     check code by different bound
 *
 ******************************************************************/
int
check_reply_code(int reply_code, unsigned char flag)
{
    if(reply_code == 421)
        return 0x18; /* connect FTP Server NG */
    switch(flag)
    {
        case 1:
            if(reply_code >= 100 && reply_code < 200)
                return OK;
            else
                return NG;
        case 2:
            if(reply_code >= 200 && reply_code < 300)
                return OK;
            else
                return NG;
        case 3:
            if(reply_code >= 300 && reply_code < 400)
                return OK;
            else
                return NG;
        default:
            break;
    }
    return OK;
}


/*****************************************************************
 *   NAME: estimate_cancel
 *   PARAMETER: NONE
 *   RETURN VALUE:
 *              OK: not cancel practice
 *               1: cancel need to be implemented
 *   DESCRIPTION:
 *     whether cancel or not
 *
 ******************************************************************/
int
estimate_cancel(void)
{
    /*lock memory*/
    pthread_mutex_lock(&status_lock);
    if(cancel_flag == 1)
    {
        pthread_mutex_unlock(&status_lock);
        return 1;
    }
    /*unlock memory */
    pthread_mutex_unlock(&status_lock);
    return OK;
}
/*****************************************************************
 *   NAME: set_file_info
 *   PARAMETER:
 *            rply_msg : ftp server respose msg
 *            pFileInfo: list information has benn analysed
 *   RETURN VALUE:
 *               OK   : execute success
 *               other: execute filaure
 *   DESCRIPTION:
 *            analyse list information and detach list
 *
 ******************************************************************/
int
set_file_info(char* rply_msg, struct ST_REMOTEFILEINFO * pFileInfo)
{
    unsigned long size = 0L;
    char name[BUFSIZE];
    char month[DATESIZE];
    char day[DATESIZE];
    char hour_min[DATESIZE];
    char dummy[DUMMY5][DUMMYSIZE];
    char permission[BUFSIZE];
    char buf[BUFSIZE];
    char *str     = buf;
    char *token   = NULL;
    char *delim = " ";
    char *saveptr = NULL;
    int i = 0;
    int j = 0;

    if(rply_msg == NULL)
        return 0x29; /* memory or process's error occured */
    if(pFileInfo == NULL)
        return 0x29; /* memory or process's error occured */
    pFileInfo->name_ = "";
    pFileInfo->size_ = 0;
    pFileInfo->uptime_ = -1;
    pFileInfo->isFile_ = 1;

    strncpy(buf, rply_msg, 256);

    for(i = 0; ; i++, str = NULL)
    {
        /*split string into tokens*/
        /*tokens separated by <space>s.*/
        token = strtok_r(str, delim, &saveptr);
        if (token == NULL)
            break;

        for (j = 0; j < DUMMY4; j++)
        {
            strncpy(dummy[j], dummy[j+1], DUMMYSIZE);
        }
        strncpy(dummy[DUMMY4], token, DUMMYSIZE);

        if (i == 0)
        {
            strncpy(permission, token, BUFSIZE);
        }
    }
    /*convert string to a long integer */
    size = strtol(dummy[DUMMY0], NULL, 10);

    strncpy(month,    dummy[DUMMY1], DATESIZE);
    strncpy(day,      dummy[DUMMY2], DATESIZE);
    strncpy(hour_min, dummy[DUMMY3], DATESIZE);
    strncpy(name,     dummy[DUMMY4], BUFSIZE);
    if ((strlen(name) > 0) &&
        (name[strlen(name) - 1] == '\n'))
    {
        name[strlen(name) - 1] = '\0';
        if ((strlen(name) > 0) &&
            (name[strlen(name) - 1] == '\r'))
        {
            name[strlen(name) - 1] = '\0';
        }
    }

    if ( permission[0] == 'd' )
    {
        pFileInfo->isFile_ = 0;
    }
    pFileInfo->name_ = name;
    pFileInfo->size_ = size;
    return OK;
}

/*****************************************************************
 *   NAME: get_replycode
 *   PARAMETER:
 *            response   :datagram
 *            reply_code :reply code in the datagram
 *   RETURN VALUE:
 *               OK   : success
 *               other: PARAMETER is NULL
 *   DESCRIPTION:
 *              get reply code
 *
 ******************************************************************/
int
get_replycode(char* response, int* reply_code)
{
    char code[4];
    if(response == NULL)
        return 0x29; /* memory or process's error occured */
    if(reply_code == NULL)
        return 0x29; /* memory or process's error occured */
    strncpy(code, response, 3);
    code[3] = '\0';
    *reply_code = atoi(code);
    return OK;
}
/*****************************************************************
 *   NAME: send_command
 *   PARAMETER:
 *            sock_ptr   : socket resource
 *            cmd        : send msg
 *   RETURN VALUE:
 *               OK   : send msg success
 *               other: send msg fail
 *   DESCRIPTION:
 *              Send command to ftp server
 *
 ******************************************************************/
int
send_command(struct ST_SOCK * sock_ptr, char* cmd)
{
    int rev = 0;
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(cmd == NULL)
        return 0x29; /* memory or process's error occured */
    /*send msg to ftp server*/
    rev = send(sock_ptr->sid, cmd, strlen(cmd), 0);
    if ( rev == ERROR )
    {
        write_log("[ERROR INFO] send() error occured in send_command()\n");
        return 0x29;  /* memory or process's error occured */
    }

    return OK;
}
/*****************************************************************
 *   NAME: read_line
 *   PARAMETER:
 *            sid      :socket descriptor
 *            response :Ftp server response msg
 *            length   :msg length
 *   RETURN VALUE:
 *               OK   : read msg success
 *               other: read msg fail
 *   DESCRIPTION:
 *              Recv FTP server one line information from response msg
 *
 ******************************************************************/
int
read_line(int sid, char* response, int* length)
{
    int recv_cnt = 0;
    unsigned char flag = 0;
    unsigned char retry_ind = 0;
    if(response == NULL)
        return 0x29; /* memory or process's error occured */
    if(length == NULL)
        return 0x29; /* memory or process's error occured */
    while(flag == 0)
    {
        if(retry_ind > tcp_retry)
        {
            /*test code ------> start*/
            #ifdef _TEST_

            recv_timeout_retry -= 1;

            #endif
            /*test code ------> end*/

            return 0x14; /*timed out*/
        }
        /*recv Ftp server response msg*/
        recv_cnt = recv(sid, response, BYTE1, 0);
        /*Timerout*/
        if(recv_cnt == ERROR && (errno == ETIMEDOUT || errno == EAGAIN))
        {
            retry_ind++;
            /*test code ------> start*/
            #ifdef _TEST_

            recv_timeout_retry++;

            #endif
            /*test code ------> end*/
            continue;
        }
        if(recv_cnt == ERROR && (errno != ETIMEDOUT && errno != EAGAIN))
        {
            write_log("[ERROR INFO] recv() error occured in read_line()\n");
            return 0x29; /* memory or process's error occured */
        }

        if(recv_cnt == 0)
            return SHUTDOWN;

        if( *response == '\n' )
            flag = 1;

        response++;
        (*length)++;
    }
    *response = '\0';
    return OK;
}
/*****************************************************************
 *   NAME: read_response
 *   PARAMETER:
 *            sock_ptr : socket resource
 *            response : Ftp server response msg
 *   RETURN VALUE:
 *               OK   : Read Ftp server response msg success
 *               other: read Ftp server response msg fail
 *   DESCRIPTION:
 *              Read ftp server response msg
 *
 ******************************************************************/
int
read_response(struct ST_SOCK * sock_ptr, char* response)
{
    int recv_cnt = 0;
    int rev = 0;
    int flag = 0;
    char first_line[BUFSIZE];
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(response == NULL)
        return 0x29; /* memory or process's error occured */
    memset(first_line, '\0' , BUFSIZE);
    while(flag == 0)
    {
        /*Read ftp server response msg*/
        rev = read_line(sock_ptr->sid, response, &recv_cnt);
        if(rev)
            return rev;
		/*printf("read info = %s\n", response);*/
        if ((recv_cnt > 4) && (isdigit(response[0])) &&
            (isdigit(response[1])) && (isdigit(response[2])))
        {
            /*multi-line replies*/
            if (response[3] == '-')
            {
                if (strlen(first_line) == 0)
                {
                    strncpy(first_line, response, BUFSIZE);
                }
            }
            /*The last line*/
            else if (response[3] == ' ')
            {
                if ((strlen(first_line) > 0) &&
                    (strncmp(response, first_line, INT_3) != 0))
                {
                    continue;
                }
                flag = 1;
            }
        }
    }
    return OK;
}

/*****************************************************************
 *   NAME: user_login
 *   PARAMETER:
 *            copy_ptr:copy information
 *            sock_ptr:socket resource
 *   RETURN VALUE:
 *               OK   :  user login success
 *               other:  user login fail
 *   DESCRIPTION:
 *        login ftp server
 *
 ******************************************************************/
int
user_login(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(copy_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    /* execute User command */
    rev = user_cmd(sock_ptr, copy_ptr->username);
    if(rev)
        return rev;
    /* execute pass command */
    rev = pass_cmd(sock_ptr, copy_ptr->pwd);
    if(rev)
        return rev;

    return OK;
}

/*****************************************************************
 *   NAME: ftp_login
 *   PARAMETER:
 *            copy_ptr:copy information
 *            sock_ptr:socket resource
 *   RETURN VALUE:
 *               OK   :  login ftp server success
 *               other:  login ftp server fail
 *   DESCRIPTION:
 *        login ftp server
 *
 ******************************************************************/
int
ftp_login(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0, ret = 0, flag = 0, fcn_val = 0, reply_code = 0, error = 0;
    int retry_ind = 0;
    socklen_t len = 0;
    fd_set rset, wset;
    struct timeval tval;
    char response[BUFSIZE];
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(copy_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    memset(response, '\0', BUFSIZE);

    while(flag == 0)
    {
        fcn_val = fcntl(sock_ptr->sid, F_GETFL, 0);
        if(fcn_val == -1)
        {
            write_log("[ERROR INFO] fcntl() error occured in ftp_login()\n");
            return 0x29;
        }
        /*set NON-BLOCK mode for watching timedout*/
        rev = fcntl(sock_ptr->sid, F_SETFL, fcn_val | O_NONBLOCK);
        if(rev == -1)
        {
            write_log("[ERROR INFO] fcntl() error occured in ftp_login()\n");
            return 0x29;
        }
        /*select ---- time out*/
        /* connect ftp server */
        rev = connect(sock_ptr->sid,(struct sockaddr *)&(sock_ptr->sin),
                                                sizeof((sock_ptr->sin)));
        if(rev < 0 && errno != EINPROGRESS)
        {
            if(errno == EADDRNOTAVAIL)
            {
                write_log("[ERROR INFO] connect() error occured in ftp_login()\n");
                return 0x14; /* Network NG */
            }

            else if(errno == EAFNOSUPPORT)
            {
                write_log("[ERROR INFO] connect() error occured in ftp_login()\n");
                return 0x0c; /*ip address is not correct*/
            }
            else if(errno == ENETUNREACH)
            {
                write_log("[ERROR INFO] connect() error occured in ftp_login()\n");
                return 0x10; /*SR-MGC's internet service occured error*/
            }
            else
            {
                write_log("[ERROR INFO] connect() error occured in ftp_login()\n");
                return 0x29;
            }
        }
        if(rev == 0)
        {
            if(fcntl(sock_ptr->sid, F_SETFL, fcn_val) < 0)
            {
                write_log("[ERROR INFO] fcntl() error occured in ftp_login()\n");
                return 0x29;
            }
            /*exit while loop*/
            flag = 1;
        }
        FD_ZERO(&rset);
        FD_SET(sock_ptr->sid, &rset);
        wset = rset;
        tval.tv_sec = copy_ptr->timeout;
        tval.tv_usec = 0;
        /*watch timedout*/
        rev = select(sock_ptr->sid + 1, &rset, &wset, NULL, &tval);
        if(rev == 0)
        {
            /*retry to connect*/
            retry_ind++;
            if(retry_ind > tcp_retry)
                return 0x18; /* connect FTP Server NG */

            /*test code ------> start*/
            #ifdef _TEST_

            control_retry++;

            #endif
            /*test code ------> end*/

            close(sock_ptr->sid);
            sock_ptr->sid = 0;
            ret = creat_socket(copy_ptr, sock_ptr);
            if(ret)
                return ret;

            continue;
        }
        else if(rev < 0)
        {
            write_log("[ERROR INFO] select() error occured in ftp_login()\n");
            return 0x29;
        }

        else
        {
            if(FD_ISSET(sock_ptr->sid, &rset) || FD_ISSET(sock_ptr->sid, &wset))
            {
                len = sizeof(error);
                if(getsockopt(sock_ptr->sid, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
                {
                    write_log("[ERROR INFO] getsockopt() error occured in ftp_login()\n");
                    return 0x29;
                }
            }
            else
            {
                write_log("[ERROR INFO] FD_ISSET() error occured in ftp_login()\n");
                return 0x29;
            }
        }

        rev = fcntl(sock_ptr->sid, F_SETFL, fcn_val);
        if(rev == -1)
        {
            write_log("[ERROR INFO] fcntl() error occured in ftp_login()\n");
            return 0x29;
        }
        /*exit while loop*/
        flag++;
    }

    /*receive welcome message*/
    ret = read_response(sock_ptr, response);
    if(ret)
        return ret;

    ret = get_replycode(response, &reply_code);
    if(ret)
        return ret;

    /*check connect's code*/
    ret = check_reply_code(reply_code, 2);
    if(ret)
    {
        if(ret == -1)
            return 0x18; /* connect FTP Server NG */
        return ret;
    }

    /*login ftp server*/
    ret = user_login(copy_ptr, sock_ptr);
    if(ret)
        return ret;

    return OK;
}


/*****************************************************************
 *   NAME: ftp_quit
 *   PARAMETER:
 *            sock_ptr:socket resource
 *   RETURN VALUE:
 *             OK : execute quit command success
 *          other : execute quit command fail
 *   DESCRIPTION:
 *        execute quit command
 *
 ******************************************************************/
int
ftp_quit(struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    /*execute quit command */
    rev = quit_cmd(sock_ptr);
    if(rev)
        return rev;

    return OK;
}


/*****************************************************************
 *   NAME: create_data_conn
 *   PARAMETER:
 *            sock_ptr    :socket resource
 *            ip          :ip address
 *            port        :port
 *            timeout     :timeout value
 *            datasock_ptr:data-port socket resource
 *   RETURN VALUE:
 *             OK : create data connection success
 *          other : create data connection fail
 *   DESCRIPTION:
 *         create a  socket for data connection
 *
 ******************************************************************/
int
create_data_conn(struct ST_SOCK * sock_ptr, char* ip, unsigned short port,
                                    unsigned char timeout, struct ST_DATASOCK * datasock_ptr)
{
    int rev = 0, flag = 1;
    socklen_t len;
    len = sizeof(struct sockaddr_in);
    struct sockaddr_in address;
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(datasock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(ip == NULL)
        return 0x29;
    /*create a socket for data connection*/
    if((datasock_ptr->sid_data = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        write_log("[ERROR INFO] socket() error occured in create_data_conn()\n");
        return 0x29;
    }
    /*data connection port*/
    (datasock_ptr->sin_data).sin_port = htons(port + 1);
    (datasock_ptr->sin_data).sin_family = AF_INET;
    getsockname(sock_ptr->sid, (struct sockaddr *)&address, &len);
    (datasock_ptr->sin_data).sin_addr = address.sin_addr;
    if(((datasock_ptr->sin_data).sin_addr.s_addr) == INADDR_NONE)
    {
        close(datasock_ptr->sid_data);
        datasock_ptr->sid_data = 0;
        write_log("[ERROR INFO] getsockname() error occured in create_data_conn()\n");
        return 0x29; /* memory or process's error occured */
    }

    /* set socket option */
    if((setsockopt(datasock_ptr->sid_data, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) == -1)
    {
        close(datasock_ptr->sid_data);
        datasock_ptr->sid_data = 0;
        write_log("[ERROR INFO] setsockopt() error occured in create_data_conn()\n");
        return 0x29; /* memory or process's error occured */
    }

    if((setsockopt(datasock_ptr->sid_data, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv))) == -1)
    {
        close(datasock_ptr->sid_data);
        datasock_ptr->sid_data = 0;
        write_log("[ERROR INFO] setsockopt() error occured in create_data_conn()\n");
        return 0x29; /* memory or process's error occured */
    }
    /*reuse port*/
    if(setsockopt(datasock_ptr->sid_data, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1)
    {
        close(datasock_ptr->sid_data);
        datasock_ptr->sid_data = 0;
        write_log("[ERROR INFO] setsockopt() error occured in create_data_conn()\n");
        return 0x29; /* memory or process's error occured */
    }

    /*create a data connection*/
    rev = bind(datasock_ptr->sid_data, (struct sockaddr *)&(datasock_ptr->sin_data),
                                        sizeof(datasock_ptr->sin_data));
    if(rev < 0)
    {
        close(datasock_ptr->sid_data);
        datasock_ptr->sid_data = 0;
        write_log("[ERROR INFO] bind() error occured in create_data_conn()\n");
        return 0x29; /* memory or process's error occured */
    }

    rev = listen(datasock_ptr->sid_data, 1);
    if(rev < 0)
    {
        close(datasock_ptr->sid_data);
        datasock_ptr->sid_data = 0;
        write_log("[ERROR INFO] listen() error occured in create_data_conn()\n");
        return 0x29; /* memory or process's error occured */
    }

    return OK;
}


/*****************************************************************
 *   NAME: accept_file
 *   PARAMETER:
 *            datasock_ptr:data-port socket resource
 *            filename    :file name
 *            flag        :file get times
 *            length      :buffer length
 *   RETURN VALUE:
 *              OK:  accept file success
 *           other:  failure
 *   DESCRIPTION:
 *        accept a file from server
 *
 ******************************************************************/
int
accept_file(struct ST_DATASOCK * datasock_ptr, char* filename, unsigned short length, int flag)
{
    char pathname[128];
    char* DATABUF = NULL;
    struct timeval tv;
    int rev = 0 ,len = 0, wr_flag = 0, retry_ind = 0;
    socklen_t size_sockaddr = 0;
    fd_set rset, wset;
    if(datasock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(filename == NULL)
        return 0x29; /* memory or process's error occured */
    size_sockaddr = sizeof(datasock_ptr->sin_data);
    memset(pathname, '\0', 128);

    FD_ZERO(&rset);
    FD_SET(datasock_ptr->sid_data, &rset);
    wset = rset;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    /*watch event of connecting*/
    rev = select(datasock_ptr->sid_data + 1, &rset, &wset, NULL, &tv);
    if(rev > 0)
    {
        datasock_ptr->sid_file = accept(datasock_ptr->sid_data,
                                    (struct sockaddr *)&(datasock_ptr->sin_data),&size_sockaddr);
    }
    else if(rev == 0)
    {
        /*timeout and retry*/
        return REACCEPT;
    }
    else
    {
        write_log("[ERROR INFO] select() error occured in accept_file()\n");
        return 0x29; //memory or process's error occured
    }

    if(flag == FIRST)
    {
        sprintf(pathname, "%s%s", TEMP_DIR1, filename);
    }
    else if(flag == SECOND)
    {
        sprintf(pathname, "%s%s", TEMP_DIR2, filename);
    }
    else
    {
        close(datasock_ptr->sid_file);
        datasock_ptr->sid_file = 0;
        write_log("[ERROR INFO] parameter error occured in accept_file()\n");
        return 0x29; /* memory or process's error occured */
    }
    /*open a file to save the accepted data*/
    datasock_ptr->savefd = open(pathname, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG);
    if(datasock_ptr->savefd < 0)
    {
        close(datasock_ptr->sid_file);
        datasock_ptr->sid_file = 0;
        write_log("[ERROR INFO] open() error occured in accept_file()\n");
        return 0x17; /* create file NG */
    }

    DATABUF = (char* )see_malloc(length);
    if(DATABUF == NULL)
    {
        close(datasock_ptr->sid_file);
        datasock_ptr->sid_file = 0;
        close(datasock_ptr->savefd);
        datasock_ptr->savefd = 0;
        write_log("[ERROR INFO] malloc() error occured in accept_file()\n");
        return 0x29;
    }

    do
    {
        if(retry_ind > tcp_retry)
        {
            /*test code ------> start*/
            #ifdef _TEST_

            recv_timeout_retry -= 1;

            #endif
            /*test code ------> end*/
            close(datasock_ptr->sid_file);
            datasock_ptr->sid_file = 0;
            close(datasock_ptr->savefd);
            datasock_ptr->savefd = 0;
            see_free(DATABUF);
            return 0x14; /* Network NG */
        }

        memset(DATABUF, '\0', length);
        len = recv(datasock_ptr->sid_file, DATABUF, sizeof(DATABUF), 0);
        if(len == 0)
        {
            wr_flag = 1;
        }
        else if(len < 0)
        {
            if(errno == ETIMEDOUT || errno == EAGAIN)
            {
                retry_ind++;
                /*test code ------> start*/
                #ifdef _TEST_

                recv_timeout_retry++;

                #endif
                /*test code ------> end*/
                continue;
            }
            else
            {
                close(datasock_ptr->sid_file);
                datasock_ptr->sid_file = 0;
                close(datasock_ptr->savefd);
                datasock_ptr->savefd = 0;
                see_free(DATABUF);
                write_log("[ERROR INFO] recv() error occured in accept_file()\n");
                return 0x29; /* memory or process's error occured */
            }
        }
        else
        {
            /*write file data from server to local file*/
            rev = write(datasock_ptr->savefd, DATABUF, len);
            if(rev < 0)
            {
                close(datasock_ptr->sid_file);
                datasock_ptr->sid_file = 0;
                close(datasock_ptr->savefd);
                datasock_ptr->savefd = 0;
                see_free(DATABUF);
                return 0x17;  /* create file NG */
            }
        }
    }while(wr_flag == 0);

    close(datasock_ptr->sid_file);
    datasock_ptr->sid_file = 0;
    close(datasock_ptr->savefd);
    datasock_ptr->savefd = 0;
    see_free(DATABUF);
    return OK;
}

/*****************************************************************
 *   NAME: data_fin
 *   PARAMETER:
 *            sock_ptr:socket resource
 *   RETURN VALUE:
 *             OK : execute success
 *          other : execute failure
 *   DESCRIPTION:
 *     receive data transfer finished message
 *
 ******************************************************************/
int
data_fin(struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    int reply_code = 0;
    char response[BUFSIZE];
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    memset(response,'\0',BUFSIZE);
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response, &reply_code);
    if(rev < 0)
        return rev;
    /*check fin's code*/
    rev = check_reply_code(reply_code, 2);
    if(rev)
    {
        if(rev == -1)
            return 0x1d; /* operate fail on the FTP Server(download) */
        return rev;
    }

    return OK;
}

/*****************************************************************
 *   NAME: get_file_data
 *   PARAMETER:
 *            sock_ptr:scoket resource
 *            copy_ptr:copy command's information
 *            filename:filename
 *            flag    :download times
 *            datasock_ptr:data-port socket resource
 *   RETURN VALUE:
 *             OK : execute success
 *          other : failure
 *   DESCRIPTION:
 *          get file from server twice
 *
 ******************************************************************/
int
get_file_data(struct ST_SOCK * sock_ptr, struct COPY_DATA * copy_ptr, char* filename,
                               struct ST_DATASOCK * datasock_ptr, int flag)
{
    int rev = 0, retry_ind = 0;
    int offset = 0;
    char* p = NULL;
    unsigned char* addr_t = NULL;
    unsigned char* port_t = NULL;
    char pathname[128];
    char file_name[65];
    char response[BUFSIZE];
    memset(response, 0 ,BUFSIZE);

    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(filename == NULL)
        return 0x29; /* memory or process's error occured */
    if(datasock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(copy_ptr == NULL)
        return 0x29;
    memset(pathname, '\0' , 128);
    memcpy(file_name, filename, 65);
    p = strrchr(file_name, '/');
    if(p == NULL)
    {
        write_log("[ERROR INFO] strrchr() error occured in get_file_data()\n");
        return 0x2e; /* filename is not correct */
    }

    memcpy(pathname, file_name, ( p - file_name + 1 ));
    offset = ( p - file_name + 1 );
    if(offset >= 128)
        return 0x2e; /* filename is not correct */

    /* execute cwd command  */
    rev = cwd_cmd(sock_ptr, pathname);
    if(rev)
        return rev;

    for(retry_ind = 0; retry_ind <= tcp_retry;)
    {
        /* create data connection  */
        rev = create_data_conn(sock_ptr, copy_ptr->ipaddress, copy_ptr->port,
                                                        copy_ptr->timeout, datasock_ptr);
        if(rev)
            return rev;

        addr_t = (unsigned char *)&(datasock_ptr->sin_data).sin_addr;
        port_t = (unsigned char *)&(datasock_ptr->sin_data).sin_port;

        rev = port_cmd(sock_ptr, addr_t, port_t);
        if(rev)
        {
            close(datasock_ptr->sid_data);
            datasock_ptr->sid_data = 0;
            /*execute port command failure*/
            if(rev == 0x2c)
            {
                retry_ind++;
                continue;
            }
            return rev;
        }
        /*get file command*/
        rev = retr_cmd(sock_ptr, (p+1));
        if(rev)
        {
            close(datasock_ptr->sid_data);
            datasock_ptr->sid_data = 0;
            /*execute retr command failure*/
            if(rev == 0x2e)
            {
                retry_ind++;
                continue;
            }
            return rev;
        }
        /*accept file from server*/
        rev = accept_file(datasock_ptr, filename, copy_ptr->max_length, flag);
		close(datasock_ptr->sid_data);
		datasock_ptr->sid_data = 0;
        if(rev)
        {
            /*retry to getting file*/
            if(rev == REACCEPT)
            {
                retry_ind++;
                rev = read_response(sock_ptr, response);
                if(rev)
                    return rev;
                continue;
            }
            return rev;
        }
        break;
    }

    /*test code ------> start*/
    #ifdef _TEST_

    data_retry += retry_ind;

    #endif
    /*test code ------> end*/

    if(retry_ind > tcp_retry)
        return 0x1e; /* connect FTP Server NG(download) */

    rev = data_fin(sock_ptr);
    if(rev)
        return rev;
    return OK;
}
/*****************************************************************
 *   NAME: get_file
 *   PARAMETER:
 *            filename:filename
 *            copy_ptr:copy information
 *            sock_ptr:socket resource
 *   RETURN VALUE:
 *             OK : execute success
 *          other : execute failure
 *   DESCRIPTION:
 *          get a file from server
 ******************************************************************/
int
get_file(char* filename, struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    struct ST_DATASOCK data_sock;
    char pathname_1[128];
    char pathname_2[128];
    if(copy_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(filename == NULL)
        return 0x29; /* memory or process's error occured */
    memset(&data_sock, 0, sizeof(struct ST_DATASOCK));
    memset(pathname_1, '\0', 128);
    memset(pathname_2, '\0', 128);

    /* estimate whether cancel or not  */
    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(3, sock_ptr);
        return CANCELRET;
    }

    /*get file first*/
    rev = get_file_data(sock_ptr, copy_ptr, filename, &data_sock, FIRST);
    if(rev)
        return rev;

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(3, sock_ptr);
        return CANCELRET;
    }
    /*get file second*/
    rev = get_file_data(sock_ptr, copy_ptr, filename, &data_sock, SECOND);
    if(rev)
        return rev;

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(3, sock_ptr);
        return CANCELRET;
    }

    sprintf(pathname_1, "%s%s", TEMP_DIR1, filename);
    sprintf(pathname_2, "%s%s", TEMP_DIR2, filename);

    /* compare the first file data with the second file data*/
    rev = file_compare(pathname_1, pathname_2);
    if(rev)
    {
        if(rev == 0x21 || rev == 0x22)
        {
            /*re_download files*/
            return REDOWNLOAD;
        }
        else
            return rev;
    }
    return OK;
}


/*****************************************************************
 *   NAME: download_a_file
 *   PARAMETER:
 *            filename: file name
 *            copy_ptr:copy information
 *            sock_ptr:scoket resource
 *   RETURN VALUE:
 *             OK :  get a file success
 *          other :  get a file fail
 *   DESCRIPTION:
 *          download a file
 *
 ******************************************************************/
int
download_a_file(char* filename, struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0, re_down = 0;
    if(filename == NULL)
        return 0x29; /* memory or process's error occured */
    if(copy_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */

    if(strlen(filename) == 0)
    {
        write_log("[ERROR INFO] parameter error occured in download_a_file()\n");
        return 0x29;
    }
    /*make local directory (for 2 download)*/
    rev = make_temp_dir(filename);
    if(rev)
        return rev;

    for(re_down = 0; re_down <= re_download; )
    {
        rev = get_file(filename, copy_ptr, sock_ptr);
        if(rev)
        {
            /*retry to get file*/
            if(rev == REDOWNLOAD)
            {
                re_down++;
                continue;
            }
            return rev;
        }
        break;
    }

    /*test code ------> start*/
    #ifdef _TEST_

    compare_retry += re_down;

    #endif
    /*test code ------> end*/

    if(re_down > re_download)
        return 0x21;

    return OK;
}
/*****************************************************************
 *   NAME: get_file_size
 *   PARAMETER:
 *            name:file name
 *            head:LIST information
 *            file_size:file size
 *   RETURN VALUE:
 *             OK :  get a file size success
 *          other :  get a file size fail
 *   DESCRIPTION:
 *       get size of a file
 *
 ******************************************************************/
int
get_file_size(char* name, struct list * head, unsigned long* file_size)
{
    int rev = 0;
    struct ST_REMOTEFILEINFO st_rfi;
    struct list * p = NULL;
    struct list * r = NULL;
    if(name == NULL)
        return 0x29; /* memory or process's error occured */
    if(head == NULL)
        return 0x29; /* memory or process's error occured */
    if(file_size == NULL)
        return 0x29; /* memory or process's error occured */
    p = head->next;
    r = head->next;
    memset(&st_rfi, 0, sizeof(struct ST_REMOTEFILEINFO));

    if(head->next == NULL)
        return 0x29; /* memory or process's error occured */

    while(p != NULL)
    {
        r = p;
        /*delete trash data*/
        while((p->data[strlen(p->data)-1]) >= 0 && (p->data[strlen(p->data)-1]) <= 32)
        {
            (p->data[strlen(p->data)-1]) = '\0';
        }

        if(p->data[0] == '-')
        {
            /*collect information from list*/
            rev = set_file_info(p->data, &st_rfi);
            if(rev)
                return rev;
            if(strcmp(st_rfi.name_, name) == 0)
            {
                *file_size = st_rfi.size_ ;
                return OK;
            }
        }
        p = r->next;
    }
    return 0x2e; /* filename is not correct */
}
/*****************************************************************
 *   NAME: copy_onefile
 *   PARAMETER:
 *            copy_ptr: copy information
 *            sock_ptr: socket resource
 *   RETURN VALUE:
 *             OK :  download file success
 *          other :  download file fail
 *   DESCRIPTION:
 *          Download a file
 *
 ******************************************************************/
int
copy_onefile(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0, retry_ind = 0;
    int offset = 0;
    unsigned long file_size = 0;
    char* p = NULL;
    char path_name[128];
    char file_name[65];
    char name[64];
    char response[BUFSIZE];
    unsigned char* addr_t = NULL;
    unsigned char* port_t = NULL;
    struct ST_DATASOCK data_sock;
    struct list head;
    if(copy_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    memset(path_name, '\0' , 128);
    memcpy(file_name, copy_ptr->filename, 65);
    memset(name, '\0', 64);
	memset(head.data, 0, 256);
	memset(response, 0 , BUFSIZE);
	head.next = NULL;
    p = strrchr(file_name, '/');
    if(p == NULL)
    {
        write_log("[ERROR INFO] strrchr() error occured in copy_onefile()\n");
        return 0x2e;  /* filename is not correct */
    }
    memcpy(path_name, file_name, (p - file_name + 1));
    memcpy(name, p + 1, file_name + strlen(file_name) - p - 1);
    offset = (p - file_name + 1);
    if(offset >= 128)
        return 0x2e; /* filename is not correct */

    rev = type_cmd(sock_ptr, 'I');
    if(rev)
        return rev;
    /* Send cwd command to ftp server */
    rev = cwd_cmd(sock_ptr, path_name);
    if(rev)
        return rev;

    /* Create data connection */
    for(retry_ind = 0; retry_ind <= tcp_retry;)
    {
        rev = create_data_conn(sock_ptr, copy_ptr->ipaddress, copy_ptr->port, copy_ptr->timeout, &data_sock);
        if(rev)
            return rev;

        addr_t = (unsigned char *)&(data_sock.sin_data).sin_addr;
        port_t = (unsigned char *)&(data_sock.sin_data).sin_port;

        rev = port_cmd(sock_ptr, addr_t, port_t);
        if(rev)
        {
            close(data_sock.sid_data);
            data_sock.sid_data = 0;
            /*execute port command fail*/
            if(rev == 0x2c)
            {
                retry_ind++;
                continue;
            }
            return rev;
        }
        /* Send list command to ftp server */
        rev = list_cmd(sock_ptr);
        if(rev)
        {
            close(data_sock.sid_data);
            data_sock.sid_data = 0;
            /*execute list command failure*/
            if(rev == 0x19)
            {
                retry_ind++;
                continue;
            }
            return rev;
        }
        /*get list infomation */
        rev = accept_list(&data_sock, path_name, &head);
		close(data_sock.sid_data);
		data_sock.sid_data = 0;
        if(rev)
        {
            if(rev == REACCEPT)
            {
                retry_ind++;
                rev = read_response(sock_ptr, response);
                if(rev)
                    return rev;
                continue;
            }
            return rev;
        }
        break;
    }
    if(retry_ind > tcp_retry)
        return 0x1e; /* connect FTP Server NG(download) */

    rev = data_fin(sock_ptr);
    if(rev)
        return rev;
    /* get file size*/
    rev = get_file_size(name, &head, &file_size);
    if(rev)
        return rev;

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(3, sock_ptr);
        return CANCELRET;
    }

    /*download a file */
    rev = download_a_file(copy_ptr->filename, copy_ptr, sock_ptr);
    if(rev)
        return rev;

    return OK;
}
/*****************************************************************
 *   NAME: creat_socket
 *   PARAMETER:
 *            copy_ptr: copy information
 *            sock_ptr: scoket resource
 *   RETURN VALUE:
 *             OK :  Create socket success
 *          other :  create socket fail
 *   DESCRIPTION:
 *          Create a socket
 *
 ******************************************************************/
int
creat_socket(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0, flag = 1;
    struct timeval tv;
    struct sockaddr_in my_addr; /*local network information*/
    if(copy_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    tv.tv_sec = copy_ptr->timeout;
    tv.tv_usec = 0;

    (sock_ptr->sin).sin_family = AF_INET; /*internet IPV4 address*/
    (sock_ptr->sin).sin_port = htons(21); /*ftp port no = 21*/
    /*get the ip address of ftp server*/
    (sock_ptr->sin).sin_addr.s_addr = inet_addr(copy_ptr->ipaddress);
    if(((sock_ptr->sin).sin_addr.s_addr) == INADDR_NONE)
    {
        write_log("[ERROR INFO] inet_addr() error occured in creat_socket()\n");
        return 0x29; /* memory or process's error occured */
    }

    /*TCP Protocol*/
    /*create a  socket for control connection*/
    sock_ptr->sid = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_ptr->sid == -1)
    {
        write_log("[ERROR INFO] socket() error occured in creat_socket()\n");
        return 0x29; /* memory or process's error occured */
    }
    /* set a socket option */
    rev = setsockopt(sock_ptr->sid, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if(rev)
    {
        close(sock_ptr->sid);
        write_log("[ERROR INFO] setsockopt() error occured in creat_socket()\n");
        return 0x29; /* memory or process's error occured */
    }

    /* set a socket option */
    rev = setsockopt(sock_ptr->sid, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    if(rev)
    {
        close(sock_ptr->sid);
        write_log("[ERROR INFO] setsockopt() error occured in creat_socket()\n");
        return 0x29; /* memory or process's error occured */
    }

    rev = setsockopt(sock_ptr->sid, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    if(rev)
    {
        close(sock_ptr->sid);
        write_log("[ERROR INFO] setsockopt() error occured in creat_socket()\n");
        return 0x29; /* memory or process's error occured */
    }
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(copy_ptr->port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(my_addr.sin_zero), 8);
    if(bind(sock_ptr->sid, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        close(sock_ptr->sid);
        write_log("[ERROR INFO] bind() error occured in creat_socket()\n");
        return 0x29; /* memory or process's error occured */
    }
    return OK;
}

/*****************************************************************
 *   NAME: traversal_dir
 *   PARAMETER:
 *            dir_name: dir name
 *            copy_ptr: copy information
 *            sock_ptr: socket resource
 *   RETURN VALUE:
 *             OK : execute success
 *          other : execute failure
 *   DESCRIPTION:
 *          traversal directory and get all information of files
 *
 ******************************************************************/
int
traversal_dir(char* dir_name, struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    char pwd[256];
    struct list head;
    if(copy_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(dir_name == NULL)
        return 0x29; /* memory or process's error occured */
    memset(pwd, '\0', 256);
    memset(head.data, '\0', 256);
    head.next = NULL;

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(3, sock_ptr);
        return CANCELRET;
    }

    dir_level += 1;
    /*Send cwd command to ftp server */
    rev = cwd_cmd(sock_ptr, dir_name);
    if(rev)
        return rev;

    strncpy(pwd, dir_name, strlen(dir_name));

    /*get information of files by LIST command*/
    rev = get_list(copy_ptr, sock_ptr, dir_name, &head);
    if(rev)
        return rev;

    /*analyse information of list*/
    rev = analyse_list(copy_ptr, sock_ptr, &head, pwd);
    if(rev)
        return rev;

    dir_level -= 1;

    return OK;
}

/*****************************************************************
 *   NAME: copy_dir_file
 *   PARAMETER:
 *            copy_ptr: copy information
 *            sock_ptr: socket resource
 *   RETURN VALUE:
 *             OK : execute success
 *          other : execute failure
 *   DESCRIPTION:
 *          read file information from file list and get file
 *
 ******************************************************************/
int
copy_dir_file(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    struct file_list * p = file_list.next, * r = file_list.next;
    if(copy_ptr == NULL)
        return 0x29;
    if(sock_ptr == NULL)
        return 0x29;

    if(strlen(file_list.file_pathname) == 0)
        return OK;
    rev = download_a_file(file_list.file_pathname, copy_ptr, sock_ptr);
    if(rev)
        return rev;

    /*get information of files from file list and copy files*/
    if(file_list.next == NULL)
        return OK;
    while(p != NULL)
    {
        r = p;
        rev = download_a_file(p->file_pathname, copy_ptr, sock_ptr);
        if(rev)
            return rev;

        p = r->next;
    }
    return OK;
}

/*****************************************************************
 *   NAME: copy_dir
 *   PARAMETER:
 *            copy_ptr:copy information
 *            sock_ptr:socket resource
 *   RETURN VALUE:
 *             OK :execute success
 *          other :execute failure
 *   DESCRIPTION:
 *       copy all files in the directory
 *
 ******************************************************************/
int
copy_dir(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    if(copy_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */

    rev = type_cmd(sock_ptr, 'I');
    if(rev)
        return rev;

    /*traversal target directory and get all files information of directory*/
    rev = traversal_dir(copy_ptr->dir, copy_ptr, sock_ptr);
    if(rev)
        return rev;
    /*judge whether free space is enough to save files or not*/
    rev = local_size();
    if(rev)
        return rev;
    /*execute copying files*/
    rev = copy_dir_file(copy_ptr, sock_ptr);
    if(rev)
        return rev;

    return OK;
}
/*****************************************************************
 *   NAME: init_copydata
 *   PARAMETER:
 *            copy_ptr:copying information
 *            info_ptr:copying information need to be initiliazed
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *         initialize data
 *
 ******************************************************************/
int
init_copydata(struct COPY_DATA * copy_ptr, struct COPY_INFO * info_ptr)
{
    if(copy_ptr == NULL)
        return 0x29;
    if(info_ptr == NULL)
        return 0x29;
    memcpy(copy_ptr->username, info_ptr->userName, 16);
    copy_ptr->username[16] = '\0';

    memcpy(copy_ptr->pwd, info_ptr->pwd, 16);
    copy_ptr->pwd[16] = '\0';

    copy_ptr->itype = info_ptr->itype ;

    memcpy(copy_ptr->ipaddress, info_ptr->ipAddress, 16);
    copy_ptr->ipaddress[16] = '\0';

    copy_ptr->port = info_ptr->port ;

    memcpy(copy_ptr->filename, info_ptr->FileName, 68);
    copy_ptr->filename[68] = '\0';

    memcpy(copy_ptr->dir, info_ptr->Dir, 32);
    copy_ptr->dir[32] = '\0';

    copy_ptr->max_length = info_ptr->max_length ;
    copy_ptr->re_download = info_ptr->re_download ;
    copy_ptr->tcp_retry = info_ptr->tcp_retry ;
    copy_ptr->timeout = info_ptr->timeout ;
    return OK;
}
/*****************************************************************
 *   NAME: copy_shutdown
 *   PARAMETER:
 *            flag: shutdown option
 *            sock_ptr:socket resource
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *      terminate file download
 *
 ******************************************************************/
void
copy_shutdown(int flag, struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    pthread_mutex_lock(&cancel_lock);

    /*cancel start*/
    rev = info_return(STASHM, 0, 0, 0, 0x58);
    if(rev)
    {
        write_log("[ERROR INFO] info_return() error occured in copy_shutdown()\n");
        record_log(LSHMERROR, NULL, 0x29);
    }

    switch(flag)
    {
        case 1: /*close socket and write information to shared memory */
            reclaim_resource(CLOSESOCK, sock_ptr);
            rev = info_return(STASHM, 0, 0, 0, 0x59);
            if(rev)
            {
                write_log("[ERROR INFO] info_return() error occured in copy_shutdown()\n");
                record_log(LSHMERROR, NULL, 0x29);
            }

            rev = info_return(CMDSHM, CANCELCMD, 2, 1, 0x70);
            if(rev)
            {
                write_log("[ERROR INFO] info_return() error occured in copy_shutdown()\n");
                record_log(LSHMERROR, NULL, 0x29);
            }

            break;
        case 2: /*delete temp files ,free memory and write information to shared memory */
            reclaim_resource(DELEDIR, NULL);
            reclaim_resource(DELELIST, NULL);
            rev = info_return(STASHM, 0, 0, 0, 0x59);
            if(rev)
            {
                write_log("[ERROR INFO] info_return() error occured in copy_shutdown()\n");
                record_log(LSHMERROR, NULL, 0x29);
            }

            rev = info_return(CMDSHM, CANCELCMD, 2, 1, 0x70);
            if(rev)
            {
                write_log("[ERROR INFO] info_return() error occured in copy_shutdown()\n");
                record_log(LSHMERROR, NULL, 0x29);
            }

            break;
        case 3: /*delete temp files, free memory, close socket */
                /* and write information to shared memory */
            reclaim_resource(CLOSESOCK, sock_ptr);
            reclaim_resource(DELELIST, NULL);
            reclaim_resource(DELEDIR, NULL);
            rev = info_return(STASHM, 0, 0, 0, 0x59);
            if(rev)
            {
                write_log("[ERROR INFO] info_return() error occured in copy_shutdown()\n");
                record_log(LSHMERROR, NULL, 0x29);
            }

            rev = info_return(CMDSHM, CANCELCMD, 2, 1, 0x70);
            if(rev)
            {
                write_log("[ERROR INFO] info_return() error occured in copy_shutdown()\n");
                record_log(LSHMERROR, NULL, 0x29);
            }

            break;
        default:
            write_log("[ERROR INFO] info_return() error occured in copy_shutdown()\n");
            record_log(LPROCESSERROR, NULL, 0x29);
            break;
    }
    cancel_flag = 0;
    pthread_mutex_unlock(&cancel_lock);
    set_fileget_flag(0);
    set_status(EMPTY);
    /*  write cancel finished information to log */
    record_log(LCANCELEND, NULL, LOK);
    return;
}

/*****************************************************************
 *   NAME: reclaim_resource
 *   PARAMETER:
 *            opt: option
 *            sock_ptr:socket resource
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *         reclaim resource
 ******************************************************************/
void
reclaim_resource(int opt, struct ST_SOCK * sock_ptr)
{
    switch(opt)
    {
        /*free memory has been malloced*/
        case DELELIST:
            delete_headlist();
            delete_filelist();
            break;
        /*close socket*/
        case CLOSESOCK:
            if(sock_ptr->sid > 0)
            {
                shutdown(sock_ptr->sid, SHUT_RDWR);
                close(sock_ptr->sid);
                sock_ptr->sid = 0;
            }
            break;
        /*delete temp directory*/
        case DELEDIR:
            dir_delete(TEMP_DIR1);
            dir_delete(TEMP_DIR2);
            break;
        default:
            break;
    }
    return ;
}
/*****************************************************************
 *   NAME: set_fileget_flag
 *   PARAMETER:
 *            flag: fileget_thread flag
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *            set fileget_thread flag
 *
 ******************************************************************/
void
set_fileget_flag(unsigned char flag)
{
    pthread_mutex_lock(&fileget_lock);
    fileget_flag = flag;
    pthread_mutex_unlock(&fileget_lock);
    return;
}
/*****************************************************************
 *   NAME: read_fileget_flag
 *   PARAMETER:
 *            flag: fileget_thread flag
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *          Read fileget_thread flag
 *
 ******************************************************************/
void
read_fileget_flag(unsigned char* flag)
{
    pthread_mutex_lock(&fileget_lock);
    *flag = fileget_flag;
    pthread_mutex_unlock(&fileget_lock);
    return;
}
/*****************************************************************
 *   NAME: FILEGET_thread
 *   PARAMETER:
 *            _copy_info:the information has been gotten from shared memory
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *      Ftp get file thread
 *
 ******************************************************************/
void*
FILEGET_thread(void* _copy_info)
{

    struct ST_SOCK sock;
    struct COPY_DATA copy_data;
    struct COPY_INFO * info_ptr;
    int rev = 0;
    int shm_ret = 0;
    char source_path[256]; /*temporary directory's pathname*/
    char destination_path[256];/*cf card's pathname*/
	set_fileget_flag(1); /*note that fileget_thread has been created*/
    head_list.head = NULL;
    head_list.next = NULL;
    file_list.next = NULL;
    memset(file_list.file_pathname, 0, 128);
    memset(source_path, '\0', 256);
    memset(&sock, 0, sizeof(struct ST_SOCK));
    memset(&copy_data, 0, sizeof(copy_data));
    memset(source_path, '\0', 256);
    memset(destination_path, '\0', 256);
    dir_level = 0;
    dir_size = 0;
    info_ptr = (struct COPY_INFO *)_copy_info;

    if(file_or_dir == DIRE)
        record_log(LCOPYSTART, copy_data.dir, 0);
    else
        record_log(LCOPYSTART, copy_data.filename, 0);

    /*initial information of copy*/
    rev = init_copydata(&copy_data, info_ptr);
    if(rev)
    {
         write_log("[ERROR INFO] init_copydata() error occured in FILEGET_thread()\n");
         record_log(LPROCESSERROR, NULL, 0x29);
         return ((void* )NG);
    }
    /*set process status --- copy start*/
    set_status(COPYSTART);
    rev = info_return(STASHM, 0, 0, 0, 0x53);
    if(rev)
    {
        write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
        record_log(LSHMERROR, NULL, 0x29);
    }

    rev = creat_socket(&copy_data, &sock);
    if(rev)
    {
        /*return err code*/
        set_fileget_flag(0);
        set_status(EMPTY);
        record_log(LPROCESSERROR, NULL, 0x29);
        shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
        if(shm_ret)
        {
            write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
            record_log(LSHMERROR, NULL, 0x29);
        }

        shm_ret = info_return(STASHM, 0, 0, 0, 0x55);
        if(shm_ret)
        {
            write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
            record_log(LSHMERROR, NULL, 0x29);
        }

        return ((void* )NG);
    }
    /*login ftp server*/
    rev = ftp_login(&copy_data, &sock);
    if(rev)
    {
        /*reclaim resource*/
        reclaim_resource(CLOSESOCK, &sock);
        set_fileget_flag(0);
        set_status(EMPTY);
        /*return err code*/
        shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
        if(shm_ret)
        {
            write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
            record_log(LSHMERROR, NULL, 0x29);
        }

        shm_ret = info_return(STASHM, 0, 0, 0, 0x55);
        if(shm_ret)
        {
            write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
            record_log(LSHMERROR, NULL, 0x29);
        }

        if(file_or_dir == DIRE)
            record_log(LCOPYEND, copy_data.dir, rev);
        else
            record_log(LCOPYEND, copy_data.filename, rev);

        return ((void* )NG);
    }

    tcp_retry = copy_data.tcp_retry;
    re_download = copy_data.re_download;
    timeout = copy_data.timeout;

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(1, &sock);
        return ((void* )NG);
    }

    /*set process status -- copying*/
    set_status(COPYING);
    rev = info_return(STASHM, 0, 0, 0, 0x54);
    if(rev)
    {
        write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
        record_log(LSHMERROR, NULL, 0x29);
    }

    if(file_or_dir == ONEFILE)
    {
        /*copy a file*/
        rev = copy_onefile(&copy_data, &sock);
        if(rev)
        {
            if(rev == CANCELRET)
                return ((void* )NG);
            set_fileget_flag(0);
            set_status(EMPTY);
            reclaim_resource(CLOSESOCK, &sock);
            reclaim_resource(DELELIST, NULL);
            reclaim_resource(DELEDIR, NULL);
            record_log(LCOPYEND, copy_data.filename, rev);
            /*return err code*/
            shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
            if(shm_ret)
            {
                write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
                record_log(LSHMERROR, NULL, 0x29);
            }

            shm_ret = info_return(STASHM, 0, 0, 0, 0x55);
            if(shm_ret)
            {
                write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
                record_log(LSHMERROR, NULL, 0x29);
            }

            return ((void* )NG);
        }
    }
    else if(file_or_dir == DIRE)
    {
        /*copy all files in target directory*/
        rev = copy_dir(&copy_data, &sock);
        if(rev)
        {
            if(rev == CANCELRET)
                return ((void* )NG);
            set_fileget_flag(0);
            set_status(EMPTY);
            reclaim_resource(CLOSESOCK, &sock);
            reclaim_resource(DELELIST, NULL);
            reclaim_resource(DELEDIR, NULL);
            record_log(LCOPYEND, copy_data.dir, rev);
            /*return err code*/
            shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
            if(shm_ret)
            {
                write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
                record_log(LSHMERROR, NULL, 0x29);
            }

            shm_ret = info_return(STASHM, 0, 0, 0, 0x55);
            if(shm_ret)
            {
                write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
                record_log(LSHMERROR, NULL, 0x29);
            }

            return ((void* )NG);
        }
    }
    else
    {
        set_fileget_flag(0);
        record_log(LPROCESSERROR, NULL, 0x29);
        reclaim_resource(CLOSESOCK, &sock);
        return ((void* )NG);
    }

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(3, &sock);
        return ((void* )NG);
    }

    /*exit ftp server*/
    rev = ftp_quit(&sock);
    if(rev)
    {
        write_log("[ERROR INFO] ftp_quit() error occured in FILEGET_thread()\n");
    }

    reclaim_resource(CLOSESOCK, &sock);

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(2, &sock);
        return ((void* )NG);
    }

    /*free memory*/
    reclaim_resource(DELELIST, NULL);
    /*check CF CARD access*/
    /*CF CARD UPDATE*/
    set_status(UPDATING);
    rev = info_return(STASHM, 0, 0, 0, 0x56);
    if(rev)
    {
        write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
        record_log(LSHMERROR, NULL, 0x29);
    }

    /*CF update*/
    if(file_or_dir == DIRE)
    {
        sprintf(source_path, "%s%s", TEMP_DIR1, copy_data.dir);
        /*update all files of target directory for cf card*/
        rev = cf_update(source_path, CF_DIR);
        if(rev)
        {
            set_fileget_flag(0);
            set_status(EMPTY);
            reclaim_resource(DELEDIR, NULL);
            record_log(LCOPYEND, copy_data.dir, rev);
            shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
            if(shm_ret)
            {
                write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
                record_log(LSHMERROR, NULL, 0x29);
            }

            shm_ret = info_return(STASHM, 0, 0, 0, 0x55);
            if(shm_ret)
            {
                write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
                record_log(LSHMERROR, NULL, 0x29);
            }

            return ((void* )NG);
        }
    }
    else if(file_or_dir == ONEFILE)
    {
        sprintf(source_path, "%s%s", TEMP_DIR1, copy_data.filename);
        sprintf(destination_path, "%s%s", CF_DIR, copy_data.filename);
        /*update a file for cf card*/
        rev = file_update(source_path, destination_path);
        if(rev)
        {
            set_fileget_flag(0);
            set_status(EMPTY);
            reclaim_resource(DELEDIR, NULL);
            record_log(LCOPYEND, copy_data.filename, rev);
            shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
            if(shm_ret)
            {
                write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
                record_log(LSHMERROR, NULL, 0x29);
            }

            shm_ret = info_return(STASHM, 0, 0, 0, 0x55);
            if(shm_ret)
            {
                write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
                record_log(LSHMERROR, NULL, 0x29);
            }

            return ((void* )NG);
        }
    }
    else
    {
        set_fileget_flag(0);
        set_status(EMPTY);
        reclaim_resource(DELEDIR, NULL);
        record_log(LPROCESSERROR, NULL, 0x29);
        return ((void* ) -1);
    }

    /*clear temp files*/
    rev = dir_delete(TEMP_DIR1);
    if(rev)
    {
        set_fileget_flag(0);
        set_status(EMPTY);
        reclaim_resource(DELEDIR, NULL);
        if(file_or_dir == DIRE)
            record_log(LCOPYEND, copy_data.dir, rev);
        else if(file_or_dir == ONEFILE)
            record_log(LCOPYEND, copy_data.filename, rev);

        shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
        if(shm_ret)
        {
            write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
            record_log(LSHMERROR, NULL, 0x29);
        }

        shm_ret = info_return(STASHM, 0, 0, 0, 0x55);
        if(shm_ret)
        {
            write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
            record_log(LSHMERROR, NULL, 0x29);
        }

        return ((void* )NG);
    }
    /*clear temp files*/
    rev = dir_delete(TEMP_DIR2);
    if(rev)
    {
        set_fileget_flag(0);
        set_status(EMPTY);
        reclaim_resource(DELEDIR, NULL);
        if(file_or_dir == DIRE)
            record_log(LCOPYEND, copy_data.dir, rev);
        else if(file_or_dir == ONEFILE)
            record_log(LCOPYEND, copy_data.filename, rev);

        shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
        if(shm_ret)
        {
            write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
            record_log(LSHMERROR, NULL, 0x29);
        }

        shm_ret = info_return(STASHM, 0, 0, 0, 0x55);
        if(shm_ret)
        {
            write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
            record_log(LSHMERROR, NULL, 0x29);
        }

        return ((void* )NG);
    }

    rev = estimate_cancel();
    if(rev)
    {
        set_status(EMPTY);
        set_fileget_flag(0);
        rev = info_return(CMDSHM, CANCELCMD, 2, 2, 0);
        if(rev)
        {
            write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
            record_log(LSHMERROR, NULL, 0x29);
        }
    }

    /*set process status*/
    set_status(EMPTY);
    rev = info_return(STASHM, 0, 0, 0, 0x57);
    if(rev)
    {
        write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
        record_log(LSHMERROR, NULL, 0x29);
    }

    rev = info_return(CMDSHM, COPYCMD, 2, 1, 0);
    if(rev)
    {
        write_log("[ERROR INFO] info_return() error occured in FILEGET_thread()\n");
        record_log(LSHMERROR, NULL, 0x29);
    }

    if(file_or_dir == DIRE)
        record_log(LCOPYEND, copy_data.dir, LOK);
    else if(file_or_dir == ONEFILE)
        record_log(LCOPYEND, copy_data.filename, LOK);

    set_fileget_flag(0);
    return ((void* )OK);
}

