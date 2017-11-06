#include "ftp.h"

void
delete_headlist(void)
{
    struct store_list * m = NULL, * n = NULL;
    if(head_list.head == NULL)
    {
        return;
    }
    if((head_list.head != NULL) && (head_list.next == NULL))
    {
        delete_list(head_list.head);
        return;
    }
    m = head_list.next;
    n = head_list.next;
    //delete list
    while(m != NULL)
    {
        n = m;
        m = n->next;
        delete_list(n->head);
        free(n);
        n = NULL;
    }

    head_list.head = NULL;
    head_list.next = NULL;
    return;

}

int
estimate_cancel(void)
{
    pthread_mutex_lock(&status_lock);
    if(cancel_flag == 1)
    {
        pthread_mutex_unlock(&status_lock);
        return 1;
    }
    pthread_mutex_unlock(&status_lock);

    return 0;
}

int
set_file_info(char * rply_msg, struct ST_REMOTEFILEINFO * pFileInfo)
{
	unsigned long size = 0L;
    char name[BUFSIZE];
    char month[DATESIZE];
    char day[DATESIZE];
    char hour_min[DATESIZE];
    char dummy[DUMMY5][DUMMYSIZE];
    char permission[BUFSIZE];

    pFileInfo->name_ = "";
    pFileInfo->size_ = 0;
    pFileInfo->uptime_ = -1;
    pFileInfo->isFile_ = 1;

    char buf[BUFSIZE];
    strncpy(buf, rply_msg, BUFSIZE);
    char *str     = buf;
    char *token   = NULL;
    char *delim = " ";
    char *saveptr = NULL;
    int i = 0;
    int j = 0;

    for (i = 0; ; i++, str = NULL)
    {
        token = strtok_r(str, delim, &saveptr);
        if (token == NULL)
        {
            break;
        }
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

    if (i < DUMMY5)
    {
        return 0x29;
    }
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
    return 0;
}

int
analyse_list(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr, struct list * head, char* pwd)
{
    int rev = 0;
    struct list * p = head->next;
    struct list * r = head->next;
    struct ST_REMOTEFILEINFO st_rfi;
    char temp[256];
    char dummy[260];
    memset(dummy, '\0', 260);
    memset(&st_rfi, 0, sizeof(struct ST_REMOTEFILEINFO));
    if(head->next == NULL)
    {
        return 0;
    }

    while(p != NULL)
    {
        r = p;
        memset(temp, '\0' ,256);
        memcpy(temp, pwd, strlen(pwd));
        while((p->data[strlen(p->data)-1]) >= 0 && (p->data[strlen(p->data)-1]) <= 32 )
        {
            (p->data[strlen(p->data)-1]) = '\0';
        }

        if(p->data[0] == 'd')
        {
            rev = set_file_info(p->data, &st_rfi);
            if(rev)
                return rev;
            strcat(temp,"/");
            strcat(temp, st_rfi.name_);
            memcpy(dummy, temp, 256);
            strcat(dummy, "/TXT");
            rev = make_temp_dir(dummy);
            if(rev)
                return rev;
            rev = traversal_dir(temp, copy_ptr, sock_ptr);
            if(rev)
                return rev;
        }
        else if(p->data[0] == '-')
        {
            rev = set_file_info(p->data, &st_rfi);
            if(rev)
                return rev;
            strcat(temp,"/");
            strcat(temp, st_rfi.name_);
            rev = download_a_file(temp, st_rfi.size_, copy_ptr, sock_ptr);
            if(rev)
                return rev;
        }
        p = r->next;
    }
    return 0;
}

int
get_replycode(char* response, int* reply_code)
{
    char code[4];
    strncpy(code, response, 3);
    code[3] = '\0';
    *reply_code = atoi(code);
    return 0;
}

int
send_command(struct ST_SOCK * sock_ptr, char* cmd)
{
    int rev = 0;
    rev = send(sock_ptr->sid, cmd, strlen(cmd), 0);
    if ( rev == ERROR )
    {
        // error
        return 0x29;  //return err_code;
    }
    return 0;
}

int
read_line(int sid, char* response, int* length)
{
    int recv_cnt = 0;
    unsigned char flag = 0;
    unsigned char retry_ind = 0;
    while(flag == 0)
    {
        if(retry_ind > tcp_retry)
        {
            return 0x14; //timed out
        }
        recv_cnt = recv(sid, response, BYTE1, 0);
        if(recv_cnt == ERROR && errno == ETIMEDOUT)
        {
            retry_ind++;
            continue;
        }
        if(recv_cnt == ERROR && errno != ETIMEDOUT)
        {
            perror("recv");
            return 0x29;
        }

        if(recv_cnt == 0)
        {
            return SHUTDOWN;
        }
        if( *response == '\n' )
        {
            flag = 1;
        }
        response++;
        (*length)++;
    }
    *response = '\0';
    return 0;
}

int
read_response(struct ST_SOCK * sock_ptr, char* response)
{
    int recv_cnt = 0;
    int rev = 0;
    int flag = 0;
    char first_line[BUFSIZE];
    memset(first_line, '\0' , BUFSIZE);
    while(flag == 0)
    {
        rev = read_line(sock_ptr->sid, response, &recv_cnt);
        if(rev)
        {
            return rev;
        }
        if ((recv_cnt > 4) &&(isdigit(response[0])) &&
            (isdigit(response[1])) &&(isdigit(response[2])))
        {
            if (response[3] == '-')
            {
                if (strlen(first_line) == 0)
                {
                    strncpy(first_line, response, BUFSIZE);
                }
            }
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
    return 0;
}

int
check_port_code(int reply_code)
{
    if(reply_code >= 200 && reply_code <300)
        return 0;
    else
        return 0x2c;
}

int
check_type_code(int reply_code)
{
    if(reply_code >= 200 && reply_code <300)
        return 0;
    else
        return 0x19;
}

int
type_cmd(struct ST_SOCK * sock_ptr, char type_mode)
{
    int rev = 0;
    char command[SENDSIZE];
    char response[BUFSIZE];
    int reply_code = 0;
    memset(command, '\0', SENDSIZE);
    memset(response, '\0', BUFSIZE);

    sprintf(command,"TYPE %c\r\n", type_mode);

    rev = send_command(sock_ptr, command);
    if(rev)
        return rev;
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev)
        return rev;
    //check type's code
    rev = check_type_code(reply_code);
    if(rev)
        return rev;

    return 0;

}

int
check_cwd_code(int reply_code)
{
    if(reply_code >= 200 && reply_code <300)
        return 0;
    else
        return 0x2d;
}

int
cwd_cmd(struct ST_SOCK * sock_ptr, char* dir)
{
    char command[SENDSIZE];
    char response[BUFSIZE];
    int rev = 0;
    int reply_code = 0;
    memset(command, '\0', SENDSIZE);
    memset(response, '\0', BUFSIZE);

    sprintf(command,"CWD %s\r\n",dir);
    rev = send_command(sock_ptr, command);
    if(rev)
        return rev;
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev)
        return rev;
    //check cwd's code
    rev = check_cwd_code(reply_code);
    if(rev)
        return rev;

    return 0;
}

int
check_pass_code(int reply_code)
{
    if(reply_code >= 200 && reply_code <300)
        return 0;
    else
        return 0x31;
}

int
pass_cmd(struct ST_SOCK * sock_ptr, char* pwd)
{
    char command[SENDSIZE];
    char response[BUFSIZE];
    int rev = 0;
    int reply_code = 0;
    memset(command, '\0', SENDSIZE);
    memset(response, '\0', BUFSIZE);

    sprintf(command,"PASS %s\r\n",pwd);
    rev = send_command(sock_ptr, command);
    if(rev)
        return rev;
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev)
        return rev;
    //check PASS's code
    rev = check_pass_code(reply_code);
    if(rev)
        return rev;
    return 0;
}

int
check_user_code(int reply_code)
{
    if(reply_code >= 300 && reply_code <400)
        return 0;
    else
        return 0x2a;
}

int
user_cmd(struct ST_SOCK * sock_ptr, char* username)
{
    char command[SENDSIZE];
    char response[BUFSIZE];
    int rev = 0;
    int reply_code = 0;
    memset(command, '\0', SENDSIZE);
    memset(response, '\0', BUFSIZE);

    sprintf(command,"USER %s\r\n",username);
    rev = send_command(sock_ptr, command);
    if(rev)
        return rev;
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev)
        return rev;
    //check USER's code
    rev = check_user_code(reply_code);
    if(rev)
        return rev;
    return 0;
}

int
user_login(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0;

    rev = user_cmd(sock_ptr, copy_ptr->username);
    if(rev)
        return rev;

    rev = pass_cmd(sock_ptr, copy_ptr->pwd);
    if(rev)
        return rev;

    return 0;
}

int
check_conn_code(int reply_code)
{
    if(reply_code >= 200 && reply_code <300)
        return 0;
    else
        return 0x18;
}

int
ftp_login(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    int ret = 0;
    int flag = 0;
    int fcn_val = 0;
    int reply_code = 0;
    int error = 0;
    int retry_ind = 0;
    int len = 0;
    fd_set rset, wset;
    struct timeval tval;
    char response[BUFSIZE];
    memset(response, '\0', BUFSIZE);

    while(flag == 0)
    {
        if(retry_ind > tcp_retry)
        {
            return 0x18;
        }

        fcn_val = fcntl(sock_ptr->sid, F_GETFL, 0);
        fcntl(sock_ptr->sid, F_SETFL, fcn_val | O_NONBLOCK);
              // select ---- time out
        rev = connect(sock_ptr->sid,(struct sockaddr *)&(sock_ptr->sin),
                                                sizeof((sock_ptr->sin)));
        if(rev < 0 && errno != EINPROGRESS)
        {
                if(errno == EADDRNOTAVAIL)
                    return 0x14;
                else if(errno == EAFNOSUPPORT)
                    return 0x0c;
                else if(errno == ENETUNREACH)
                    return 0x10;
                else
                    return 0x29;
        }
        if( rev == 0 )
        {
            if (fcntl(sock_ptr->sid, F_SETFL, flag) < 0)
            {
                return 0x29;
            }
            flag = 1;
        }
        FD_ZERO(&rset);
        FD_SET(sock_ptr->sid, &rset);
        wset = rset;
        tval.tv_sec = copy_ptr->timeout;
        tval.tv_usec = 0;

        rev = select(sock_ptr->sid + 1, &rset, &wset, NULL, &tval);
        if(rev == 0)
        {
            close(sock_ptr->sid);
            sock_ptr->sid = 0;
            errno = ETIMEDOUT;
            ret = creat_socket(copy_ptr, sock_ptr);
            if(ret)
                return ret;
            retry_ind++;
            continue;
        }
        else if(rev < 0)
        {
            return 0x29;
        }
        else
        {
            if(FD_ISSET (sock_ptr->sid, &rset) || FD_ISSET(sock_ptr->sid, &wset))
            {
                len = sizeof(error);
                if(getsockopt(sock_ptr->sid, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
                    return 0x29;
            }
            else
                return 0x29;
        }
        fcntl(sock_ptr->sid, F_SETFL, fcn_val);
        flag++;
    }

    //welcome message
    ret = read_response(sock_ptr, response);
    if(ret)
        return ret;
    ret = get_replycode(response, &reply_code);
    if(ret)
        return ret;

    //check connect's code
    rev = check_conn_code(reply_code);
    if(rev)
        return rev;

    ret = user_login(copy_ptr, sock_ptr);
    if(ret)
        return ret;

    return 0;
}

int
check_quit_code(int reply_code)
{
    if(reply_code >= 200 && reply_code <300)
        return 0;
    else
        return 0;
}

int
quit_cmd(struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    char command[SENDSIZE];
    char response[BUFSIZE];
    int reply_code = 0;
    memset(command, '\0', SENDSIZE);
    memset(response, '\0', BUFSIZE);

    sprintf(command,"QUIT\r\n");
    rev = send_command(sock_ptr, command);
    if(rev)
        return rev;
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev < 0)
        return rev;
    //check quit's code
    rev = check_quit_code(reply_code);
    if(rev)
        return rev;

    return 0;
}

int
ftp_quit(struct ST_SOCK * sock_ptr)
{

    int rev = 0;

    rev = quit_cmd(sock_ptr);
    if(rev)
        return rev;
    shutdown(sock_ptr->sid, SHUT_RDWR);
    if(sock_ptr->sid == 0)
        return -1;
    rev = close(sock_ptr->sid);
    sock_ptr->sid = 0;

    return 0;

}

int
check_info(struct COPY_INFO * info_ptr, unsigned char* file_or_dir)
{
    if(strlen(info_ptr->userName) == 0)
        return 0x2a;
    if(strlen(info_ptr->pwd) == 0)
        return 0x2b;
    if(info_ptr->itype != UNCOMPRESS && info_ptr->itype != COMPRESS)
        return 0x2f;
    if(strlen(info_ptr->ipAddress) == 0)
        return 0x0c;
    if(!(info_ptr->port >= 60061 && info_ptr->port <= 60069))
        return 0x2c;
    if(!(info_ptr->max_length >= 1024 && info_ptr->max_length <= 4096))
        info_ptr->max_length = 4096;
    if(!(info_ptr->re_download <= 5))
        info_ptr->re_download = 3;
    if(!(info_ptr->tcp_retry <= 5))
        info_ptr->tcp_retry = 3;
    if(!(info_ptr->timeout >= 5 && info_ptr->timeout <= 30))
        info_ptr->timeout = 10;
    if(strlen(info_ptr->FileName) == 0 && strlen(info_ptr->Dir) == 0)
        return 0x2d;
    if(strlen(info_ptr->FileName) != 0 )
    {
        if(strlen(info_ptr->Dir) != 0)
            *file_or_dir = DIRE;
        else
            *file_or_dir = ONEFILE;
    }
    if(strlen(info_ptr->Dir) != 0)
    {
        *file_or_dir = DIRE;
        if(info_ptr->Dir[strlen(info_ptr->Dir)-1] == '/')
            info_ptr->Dir[strlen(info_ptr->Dir)-1] = '\0';
    }
    return 0;
}

int
port_cmd(struct ST_SOCK * sock_ptr, unsigned char* addr,unsigned char* port)
{
    int rev = 0;
    char command[SENDSIZE];
    char response[BUFSIZE];
    int reply_code = 0;
    memset(command, '\0', SENDSIZE);
    memset(response, '\0', BUFSIZE);
    //send ip address and port to ftp server.
    sprintf(command,"PORT %d,%d,%d,%d,%d,%d\r\n",
                    addr[0],addr[1],addr[2],addr[3],port[0],port[1]);
    rev = send_command(sock_ptr, command);

    if(rev)
        return rev;
    rev = read_response(sock_ptr, response);

    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev < 0)
        return rev;
    //check port's code
    rev = check_port_code(reply_code);
    if(rev)
        return rev;
    return 0;
}

int
create_data_conn(struct ST_SOCK * sock_ptr, char* ip, unsigned short port, unsigned char timeout, struct ST_DATASOCK * datasock_ptr)
{
    //create a  socket for data connection
    int rev = 0;
    int flag = 1;
    socklen_t len;
    len = sizeof(struct sockaddr_in);
    struct sockaddr_in address;
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    datasock_ptr->sid_data = socket(AF_INET,SOCK_STREAM,0);
    (datasock_ptr->sin_data).sin_port = htons(port);
    (datasock_ptr->sin_data).sin_family = AF_INET;
    getsockname(sock_ptr->sid, (struct sockaddr *)&address, &len);
    (datasock_ptr->sin_data).sin_addr = address.sin_addr;
    if(((datasock_ptr->sin_data).sin_addr.s_addr) == INADDR_NONE)
    {
		//return err code
		return 0x29;
	}

    if((setsockopt(datasock_ptr->sid_data, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) == -1)
    {
        return 0x29;
    }

    if((setsockopt(datasock_ptr->sid_data, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv))) == -1)
    {
        return 0x29;
    }
    if( setsockopt(datasock_ptr->sid_data, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1)
    {
   		//return err code
		return 0x29;
    }
    //create a data connection
    rev = bind(datasock_ptr->sid_data, (struct sockaddr *)&(datasock_ptr->sin_data),
                                        sizeof(datasock_ptr->sin_data));

    if(rev < 0)
    {
        return 0x29;
    }

    rev = listen(datasock_ptr->sid_data, 1);
    if(rev < 0)
    {
        return 0x29;
    }

    unsigned char* addr_t = (unsigned char *)&(datasock_ptr->sin_data).sin_addr;
    unsigned char* port_t = (unsigned char *)&(datasock_ptr->sin_data).sin_port;

    rev = port_cmd(sock_ptr, addr_t, port_t);
    if(rev)
    {
       return rev;
    }
    return 0;
}

int
check_retr_code(int reply_code)
{
    if(reply_code == 150)
        return 0;
    else
        return 0x2d;
}

int
retr_cmd(struct ST_SOCK * sock_ptr, char* filename)
{
    int rev = 0;
    char command[SENDSIZE];
    char response[BUFSIZE];
    int reply_code = 0;
    sprintf(command,"RETR %s\r\n",filename);
    rev = send_command(sock_ptr, command);
    if(rev)
        return rev;
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev)
        return rev;

    //check retr's code
    rev = check_retr_code(reply_code);
    if(rev)
        return rev;

    return 0;
}

int
accept_file(struct ST_DATASOCK * datasock_ptr, char* filename, unsigned short length, int flag)
{
    char* DATABUF = (char* )malloc(length);
    char pathname[128];
    int rev = 0;
    int len = 0;
    int loop_flag = 0;
    int wr_flag = 0;
    int retry_ind = 0;
    int size_sockaddr = sizeof (datasock_ptr->sin_data);
    fd_set rset, wset;
    struct timeval tval;
    tval.tv_sec = timeout;
    tval.tv_usec = 0;
    memset(pathname, '\0' , 128);
    memset(DATABUF, '\0', length);

    while( loop_flag == 0 )
    {
        if(retry_ind > tcp_retry)
        {
            return 0x18;
        }
        FD_ZERO(&rset);
        FD_SET(datasock_ptr->sid_data, &rset);
        wset = rset;
        rev = select(datasock_ptr->sid_data + 1, &rset, &wset, NULL, &tval);
        if(rev > 0)
        {
            datasock_ptr->sid_file = accept(datasock_ptr->sid_data,
                                    (struct sockaddr *)&(datasock_ptr->sin_data),&size_sockaddr);
        }
        else if(rev == 0)
        {
            //timeout
            retry_ind++;
            continue;
        }
        else
        {
            return 0x29;
        }
        loop_flag++;
    }

    retry_ind = 0;
	if(flag == FIRST)
    {
        sprintf(pathname, "%s%s", TEMP_DIR1, filename);
    }
    else if(flag == SECOND)
    {
        sprintf(pathname, "%s%s", TEMP_DIR2, filename);
    }
    else
        return 0x29;
	//open a file to save the accepted data
    datasock_ptr->savefd = open(pathname,O_WRONLY|O_CREAT);
    if(datasock_ptr->savefd < 0)
    {
        return 0x17;
    }

    do
    {
        if(retry_ind > tcp_retry)
        {
            return 0x14;
        }
        len = recv(datasock_ptr->sid_file,DATABUF,sizeof(DATABUF),0);
        if( len == 0)
        {
            wr_flag = 1;
        }
        else if(len < 0)
        {
            if(errno == ETIMEDOUT)
            {
                retry_ind++;
                continue;
            }
            else
                return 0x29;
        }
        else
        {
            rev = write(datasock_ptr->savefd,DATABUF,len);
            if(rev < 0)
            {
                return 0x17;
            }
        }
    }while(wr_flag == 0);

	close(datasock_ptr->sid_data);
	datasock_ptr->sid_data = 0;
	close(datasock_ptr->savefd);
	datasock_ptr->savefd = 0;
    close(datasock_ptr->sid_file);
    datasock_ptr->sid_file = 0;
	free(DATABUF);
    return 0;
}

int
check_fin_code(int reply_code)
{
    if(reply_code >= 200 && reply_code <300)
        return 0;
    else
        return 0x1d;
}

int
data_fin(struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    int reply_code = 0;
    char response[BUFSIZE];
    memset(response,'\0',BUFSIZE);
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev < 0)
        return rev;
    //check fin's code
    rev = check_fin_code(reply_code);
    if(rev)
        return rev;
    return 0;
}

int
get_file_second(struct ST_SOCK * sock_ptr, char* ip, unsigned short port,
                               unsigned short length, char* filename, unsigned char timeout, struct ST_DATASOCK * datasock_ptr)
{
    int rev = 0;
    int offset = 0;
    char* p;
    char pathname[128];
    char file_name[65];
    memset(pathname, '\0' , 128);
    memcpy(file_name, filename, 65);
    //memcpy(pathname, TEMP_DIR1, strlen(TEMP_DIR1));
    p = strrchr(file_name, '/');
    memcpy(pathname, file_name, ( p - file_name + 1 ));
    offset = ( p - file_name + 1 );
    if(offset >= 128)
    {
        return -1;
    }

    rev = cwd_cmd(sock_ptr, pathname);
    if(rev)
        return rev;

    rev = create_data_conn(sock_ptr, ip, port, timeout, datasock_ptr);
    if(rev)
        return rev;

    rev = retr_cmd(sock_ptr, (p+1));
    if(rev)
    {
        if(rev == -1)
            return 0;
        else
            return rev;
    }

    rev = accept_file(datasock_ptr, filename, length, SECOND);
    if(rev)
        return rev;

    rev = data_fin(sock_ptr);
    if(rev)
        return rev;
    return 0;
}

int
get_file_first(struct ST_SOCK * sock_ptr, char* ip, unsigned short port,
                              unsigned short length, char* filename, unsigned char timeout, struct ST_DATASOCK * datasock_ptr)
{
    int rev = 0;
    int offset = 0;
    char* p;
    char pathname[128];
    char file_name[65];
    memset(pathname, '\0' , 128);
    memcpy(file_name, filename, 65);
    p = strrchr(file_name, '/');
    memcpy(pathname, file_name, ( p - file_name + 1 ));
    offset = ( p - file_name + 1 );
    if(offset >= 128)
    {
        //error
        return 0x2d;
    }

    rev = cwd_cmd(sock_ptr, pathname);
    if(rev)
        return rev;

    rev = create_data_conn(sock_ptr, ip, port, timeout, datasock_ptr);
    if(rev)
        return rev;

    rev = retr_cmd(sock_ptr, (p+1));
    if(rev)
    {
        if(rev == -1)
            return 0;
        else
            return rev;
    }

    rev = accept_file(datasock_ptr, filename, length, FIRST);
    if(rev)
        return rev;

    rev = data_fin(sock_ptr);
    if(rev)
        return rev;
    return 0;
}

int
get_file(char* filename, struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    int retry_ind = 0;
    struct ST_DATASOCK data_sock;
    char pathname_1[128];
    char pathname_2[128];
    memset(&data_sock, 0, sizeof(struct ST_DATASOCK));
    memset(pathname_1, '\0', 128);
    memset(pathname_2, '\0', 128);
    rev = type_cmd(sock_ptr, 'I');
    if(rev)
        return rev;
FILEDOWN:

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(3, sock_ptr);
    }

    rev = get_file_first(sock_ptr, copy_ptr->ipaddress, copy_ptr->port, copy_ptr->max_length,
                                            filename, copy_ptr->timeout, &data_sock);
    if(rev)
        return rev;

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(3, sock_ptr);
    }

    rev = get_file_second(sock_ptr, copy_ptr->ipaddress, copy_ptr->port, copy_ptr->max_length,
                                            filename, copy_ptr->timeout, &data_sock);
    if(rev)
        return rev;

    sprintf(pathname_1, "%s%s", TEMP_DIR1, filename);
    sprintf(pathname_2, "%s%s", TEMP_DIR2, filename);

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(3, sock_ptr);
    }

    rev = file_compare(pathname_1, pathname_2);
    if(rev)
    {
        if(rev == 0x21 || rev == 0x22)
        {
            retry_ind++;
            if(retry_ind > re_download)
            {
                return rev;
            }
            //it's need to delete the files have been downloaded
            goto FILEDOWN;
        }
    }
    return 0;
}

int
check_list_code(int reply_code)
{
    if(reply_code == 150)
        return 0;
    else
        return 0x19;
}
int
list_cmd(struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    char command[SENDSIZE];
    char response[BUFSIZE];
    int reply_code = 0;
    memset(response, '\0', BUFSIZE);
    memset(command, '\0', SENDSIZE);

    sprintf(command,"LIST\r\n");

    rev = send_command(sock_ptr, command);
    if(rev)
        return rev;
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev < 0)
        return rev;
    //check list's code
    rev = check_list_code(reply_code);
    if(rev)
        return rev;

    return 0;
}

int
download_a_file(char* filename, unsigned long file_size, struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0;

    rev = local_size(file_size);
    if(rev)
        return rev;


    //make local directory (for 2 download)
    rev = make_temp_dir(filename);
    if(rev)
        return rev;

    //get a file
    rev = get_file(filename, copy_ptr, sock_ptr);
    if(rev)
        return rev;
    return 0;
}

int
get_file_size(char* name, struct list * head, unsigned long * file_size)
{
    int rev = 0;
    struct list * p = head->next;
    struct list * r = head->next;
    struct ST_REMOTEFILEINFO st_rfi;
    memset(&st_rfi, 0, sizeof(struct ST_REMOTEFILEINFO));

    memset(&st_rfi, 0, sizeof(struct ST_REMOTEFILEINFO));
    if(head->next == NULL)
    {
        return 0x29;
    }

    while(p != NULL)
    {
        r = p;
        while((p->data[strlen(p->data)-1]) >= 0 && (p->data[strlen(p->data)-1]) <= 32 )
        {
            (p->data[strlen(p->data)-1]) = '\0';
        }

        if(p->data[0] == '-')
        {
            rev = set_file_info(p->data, &st_rfi);
            if(rev)
                return rev;
            if(st_rfi.name_ == name)
                *file_size = st_rfi.size_ ;
        }
        p = r->next;
    }
    return 0;
}

int
copy_onefile(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    int offset = 0;
    unsigned long file_size = 0;
    char* p;
    char path_name[128];
    char file_name[65];
    char name[64];
    struct ST_DATASOCK data_sock;
    struct list head;
    memset(path_name, '\0' , 128);
    memcpy(file_name, copy_ptr->filename, 65);
    memset(name, '\0', 64);
    p = strrchr(file_name, '/');
    memcpy(path_name, file_name, ( p - file_name + 1 ));
    memcpy(name, p + 1, file_name + strlen(file_name) - p - 1 );
    offset = ( p - file_name + 1 );
    if(offset >= 128)
    {
        return 0x2d;
    }

    rev = cwd_cmd(sock_ptr, path_name);
    if(rev)
        return rev;

    rev = create_data_conn(sock_ptr, copy_ptr->ipaddress, copy_ptr->port, timeout, &data_sock);

    if(rev)
        return rev;

    rev = list_cmd(sock_ptr);

    if(rev)
        return rev;

    rev = accept_list(&data_sock, path_name, copy_ptr->max_length, &head);

    if(rev)
        return rev;
    rev = data_fin(sock_ptr);
    if(rev)
        return rev;


    rev = get_file_size(name, &head, &file_size);
    if(rev)
        return rev;

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(3, sock_ptr);
    }

    rev = download_a_file(copy_ptr->filename, file_size, copy_ptr, sock_ptr);
    if(rev)
        return rev;

    return 0;
}

int
creat_socket(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    struct timeval tv;
    tv.tv_sec = copy_ptr->timeout;
    tv.tv_usec = 0;

    (sock_ptr->sin).sin_family = AF_INET; //internet IPV4 address
    //get the ip address of ftp server
    (sock_ptr->sin).sin_addr.s_addr = inet_addr(copy_ptr->ipaddress);
    if(((sock_ptr->sin).sin_addr.s_addr) == INADDR_NONE)
    {
		return 0x29;
	}
	(sock_ptr->sin).sin_port = htons(21); //ftp port no = 21
    //TCP Protocol
    //create a  socket for control connection
    sock_ptr->sid = socket(AF_INET, SOCK_STREAM, 0);

    if(sock_ptr->sid == -1)
    {
		return 0x29;
    }

    rev = setsockopt(sock_ptr->sid, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if(rev)
    {
        return 0x29;
    }

    rev = setsockopt(sock_ptr->sid, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    if(rev)
    {
        return 0x29;
    }

    return 0;
}

int
accept_list(struct ST_DATASOCK * datasock_ptr, char* dir_name, unsigned short length, struct list * head)
{
    int rev = 0;
    struct list * s = NULL, * r = NULL;
    struct store_list * m = NULL, * n = NULL, * l = NULL;
    char* DATABUF = (char* )malloc(length);
    int flag = 0;
    int loop_flag = 0;
    int head_flag = 0;
    int len = 0;
    int retry_ind = 0;
    int size_sockaddr = sizeof (datasock_ptr->sin_data);
    fd_set rset, wset;
    struct timeval tval;
    tval.tv_sec = timeout;
    tval.tv_usec = 0;

    while( loop_flag == 0)
    {
        if(retry_ind > tcp_retry)
        {
            return 0x18;
        }
        FD_ZERO(&rset);
        FD_SET(datasock_ptr->sid_data, &rset);
        wset = rset;
        rev = select(datasock_ptr->sid_data + 1, &rset, &wset, NULL, &tval);
        if(rev > 0)
        {
            datasock_ptr->sid_file = accept(datasock_ptr->sid_data,
                                    (struct sockaddr *)&(datasock_ptr->sin_data),&size_sockaddr);
        }
        else if(rev == 0)
        {
            //timeout
            retry_ind++;
            continue;
        }
        else
        {
            return 0x29;
        }
        loop_flag++;
    }


    while (flag == 0)
    {
        memset(DATABUF, '\0' , length);
        rev = read_line(datasock_ptr->sid_file, DATABUF, &len);
        if(rev == 0)
        {
            s = (struct list *)malloc(sizeof(struct list));

            memcpy(s->data, DATABUF, 256);
            strncpy(s->data, DATABUF, strlen(DATABUF));
            if(head->next == NULL)
            {
                head->next = s;
                if(head_list.head == NULL)
                {
                    head_list.head = s;
                }
                else
                {
                    n = (struct store_list *)malloc(sizeof(struct store_list));
                    n->head = NULL;
                    n->next = NULL;
                    if(head_list.next == NULL)
                    {
                        head_list.next = n;
                        n->head = s;
                        n->next = NULL;
                    }
                    else
                    {
                        m = head_list.next;
                        while(head_flag == 0)
                        {
                            if(m->head == NULL)
                            {
                                m->head = s;
                                head_flag = 1;
                                m->next = n;
                            }
                            else
                            {
                                if(m->next == NULL)
                                {
                                    m->next = n;
                                    n->head = s;
                                    head_flag = 1;
                                }
                                else
                                {
                                    l = m->next;
                                    m = l;
                                }

                            }
                        }
                    }
                }
            }
            else
            {
                r->next = s;
            }
            r = s;
        }
        if(rev == SHUTDOWN)
        {
            flag = 1;
        }
        if(rev != 0 && rev != SHUTDOWN)
        {
            return rev;
        }

    }
    if (r != NULL)
        r->next = NULL;

	free(DATABUF);
    close(datasock_ptr->sid_file);
    datasock_ptr->sid_file = 0;
    close(datasock_ptr->sid_data);
	datasock_ptr->sid_data = 0;

    return 0;
}


int
get_list(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr, char* dir_name, struct list * head)
{
    int rev = 0;
    struct ST_DATASOCK data_sock;
    memset(&data_sock, 0, sizeof(struct ST_DATASOCK));

    rev = create_data_conn(sock_ptr, copy_ptr->ipaddress, copy_ptr->port, copy_ptr->timeout, &data_sock);

    if(rev)
        return rev;
    rev = type_cmd(sock_ptr, 'I');
    if(rev)
        return 0;
    rev = list_cmd(sock_ptr);
    if(rev)
        return rev;

    rev = accept_list(&data_sock, dir_name, copy_ptr->max_length, head);
    if(rev)
        return rev;

    rev = data_fin(sock_ptr);
    if(rev)
        return rev;
    return 0;
}

int
delete_list(struct list * head)
{
    struct list * p , * r;
    if(head->next == NULL)
        return 0;
    p = head->next;
    r = head->next;
    while(p != NULL)
    {
        r = p;
        p = r->next;
        free(r);
        r = NULL;
    }
    return 0;
}

int
traversal_dir(char* dir_name, struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    char pwd[256];
    struct list head;
    memset(pwd, '\0', 256);
    memset(head.data, '\0', 256);
    head.next = NULL;

    rev = cwd_cmd(sock_ptr, dir_name);

    if(rev)
        return rev;

    strncpy(pwd, dir_name,strlen(dir_name));

    rev = get_list(copy_ptr, sock_ptr, dir_name, &head);
    if(rev)
        return rev;

    rev = analyse_list(copy_ptr, sock_ptr, &head, pwd);
    if(rev)
        return rev;

    return 0;
}

int
copy_dir(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    rev = traversal_dir(copy_ptr->dir, copy_ptr, sock_ptr);
    if(rev)
        return rev;
    return 0;
}

void
init_copydata(struct COPY_DATA * copy_ptr, struct COPY_INFO * info_ptr)
{
    memcpy(copy_ptr->username, info_ptr->userName, 12);
    copy_ptr->username[12] = '\0';

    memcpy(copy_ptr->pwd, info_ptr->pwd, 12);
    copy_ptr->pwd[12] = '\0';

    copy_ptr->itype = info_ptr->itype ;

    memcpy(copy_ptr->ipaddress, info_ptr->ipAddress, 15);
    copy_ptr->ipaddress[15] = '\0';

    copy_ptr->port = info_ptr->port ;

    memcpy(copy_ptr->filename, info_ptr->FileName, 64);
    copy_ptr->ipaddress[64] = '\0';

    memcpy(copy_ptr->dir, info_ptr->Dir, 30);
    copy_ptr->dir[30] = '\0';

    copy_ptr->max_length = info_ptr->max_length ;
    copy_ptr->re_download = info_ptr->re_download ;
    copy_ptr->tcp_retry = info_ptr->tcp_retry ;
    copy_ptr->timeout = info_ptr->timeout ;
}

void
copy_shutdown(int flag, struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    pthread_mutex_lock(&cancel_lock);
    switch(flag)
    {
        case 0:
            rev = info_return(STASHM, 0, 0, 0, 0x59);
            if(rev)
            {
                record_log(LPROCESSERROR, NULL, 0x29);
                goto EXIT;
            }
            rev = info_return(CMDSHM, CANCELCMD, 2, 1, 0x70);
            if(rev)
            {
                record_log(LPROCESSERROR, NULL, 0x29);
                goto EXIT;
            }
            break;
        case 1:
            reclaim_resource(CLOSESOCK, sock_ptr);
            rev = info_return(STASHM, 0, 0, 0, 0x59);
            if(rev)
            {
                record_log(LPROCESSERROR, NULL, 0x29);
                goto EXIT;
            }
            rev = info_return(CMDSHM, CANCELCMD, 2, 1, 0x70);
            if(rev)
            {
                record_log(LPROCESSERROR, NULL, 0x29);
                goto EXIT;
            }
            break;
        case 2:
            reclaim_resource(DELEDIR, NULL);
            rev = info_return(STASHM, 0, 0, 0, 0x59);
            if(rev)
            {
                record_log(LPROCESSERROR, NULL, 0x29);
                goto EXIT;
            }
            rev = info_return(CMDSHM, CANCELCMD, 2, 1, 0x70);
            if(rev)
            {
                record_log(LPROCESSERROR, NULL, 0x29);
                goto EXIT;
            }
            break;
        case 3:
            reclaim_resource(CLOSESOCK, sock_ptr);
            reclaim_resource(DELELIST, NULL);
            reclaim_resource(DELEDIR, NULL);
            rev = info_return(STASHM, 0, 0, 0, 0x59);
            if(rev)
            {
                record_log(LPROCESSERROR, NULL, 0x29);
                goto EXIT;
            }
            rev = info_return(CMDSHM, CANCELCMD, 2, 1, 0x70);
            if(rev)
            {
                record_log(LPROCESSERROR, NULL, 0x29);
                goto EXIT;
            }
            break;
        default:
            break;
    }
EXIT:
    cancel_flag = 0;
    pthread_mutex_unlock(&cancel_lock);
    set_fileget_flag(0);
    set_status(EMPTY);
    //write log
    record_log(LCANCELEND, NULL, LOK);
    pthread_exit((void*) 1);
}


void
reclaim_resource(int opt, struct ST_SOCK * sock_ptr)
{
    switch(opt)
    {
        case DELELIST:
            delete_headlist();
            break;
        case CLOSESOCK:
            if(sock_ptr->sid > 0)
            {
                shutdown(sock_ptr->sid, SHUT_RDWR);
                close(sock_ptr->sid);
                sock_ptr->sid = 0;
            }
            break;
        case DELEDIR:
            dir_delete(TEMP_DIR1);
            dir_delete(TEMP_DIR2);
            break;
        default:
            break;
    }
    return ;
}

void
set_fileget_flag(unsigned char flag)
{
    pthread_mutex_lock(&fileget_lock);
    fileget_flag = flag;
    pthread_mutex_unlock(&fileget_lock);
    return;
}

void
read_fileget_flag(unsigned char* flag)
{
    pthread_mutex_lock(&fileget_lock);
    *flag = fileget_flag;
    pthread_mutex_unlock(&fileget_lock);
    return;
}

void*
FILEGET_thread(void* _copy_info)
{
    struct ST_SOCK sock;
    struct COPY_DATA copy_data;
    struct COPY_INFO * info_ptr;
    int rev = 0;
    int shm_ret = 0;
    unsigned char file_or_dir = 0;
    char source_path[256];
    char destination_path[256];
    head_list.head = NULL;
    head_list.next = NULL;
    memset(source_path, '\0', 256);
    memset(&sock, 0, sizeof(struct ST_SOCK));
    memset(&copy_data, 0, sizeof(copy_data));
    memset(source_path, '\0', 256);
    memset(destination_path, '\0', 256);

    info_ptr = (struct COPY_INFO *)_copy_info;

    set_fileget_flag(1);

     //check info whether is valid
    rev = check_info(info_ptr, &file_or_dir);
    if(rev || (file_or_dir == 0))
    {
  		//return err code
        set_fileget_flag(0);
  		shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
  		if(shm_ret)
  		{
  		    record_log(LPROCESSERROR, NULL, 0x29);
        }
		return ((void* )-1);
    }

    init_copydata(&copy_data, info_ptr);

    //set process status
    set_status(COPYSTART);
    rev = info_return(STASHM, 0, 0, 0, 0x53);
    if(rev)
    {
        record_log(LPROCESSERROR, NULL, 0x29);
    }

    rev = creat_socket(&copy_data, &sock);
    if(rev)
    {
  		//return err code
        set_fileget_flag(0);
        record_log(LPROCESSERROR, NULL, 0x29);
  		shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
  		if(shm_ret)
  		{
  		    record_log(LPROCESSERROR, NULL, 0x29);
        }
		return ((void* )-1);
    }
    rev = ftp_login(&copy_data,&sock);
    if(rev)
    {
        //reclaim resource
        reclaim_resource(CLOSESOCK, &sock);
        set_fileget_flag(0);
  		//return err code
  		shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
  		if(shm_ret)
  		{
  		    record_log(LPROCESSERROR, NULL, 0x29);
        }
		return ((void* )-1);
    }


    tcp_retry = copy_data.tcp_retry;
    re_download = copy_data.re_download;
    timeout = copy_data.timeout;

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(0, NULL);
    }

    //set process status
    set_status(COPYING);
    rev = info_return(STASHM, 0, 0, 0, 0x54);
    if(rev)
    {
        record_log(LPROCESSERROR, NULL, 0x29);
    }

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(1, &sock);
    }

    if(file_or_dir == ONEFILE)
    {
        record_log(LCOPYSTART, copy_data.filename, 0);
        rev = copy_onefile(&copy_data, &sock);
        if(rev)
        {
            set_fileget_flag(0);
            reclaim_resource(CLOSESOCK, &sock);
            reclaim_resource(DELELIST, NULL);
            reclaim_resource(DELEDIR, NULL);
            record_log(LCOPYEND, copy_data.filename, rev);
            //return err code
            shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
            if(shm_ret)
            {
                record_log(LPROCESSERROR, NULL, 0x29);
            }
            return ((void* )-1);
        }
    }
    else if(file_or_dir == DIRE)
    {
        record_log(LCOPYSTART, copy_data.dir, 0);
        rev = copy_dir(&copy_data, &sock);
        if(rev)
        {
            set_fileget_flag(0);
            reclaim_resource(CLOSESOCK, &sock);
            reclaim_resource(DELELIST, NULL);
            reclaim_resource(DELEDIR, NULL);
            record_log(LCOPYEND, copy_data.dir, rev);
             //return err code
            shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
            if(shm_ret)
            {
                record_log(LPROCESSERROR, NULL, 0x29);
            }
            return ((void* )-1);
        }
    }
    else
    {
        set_fileget_flag(0);
        record_log(LPROCESSERROR, NULL, 0x29);
        reclaim_resource(CLOSESOCK, &sock);
        return ((void* )-1);
    }

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(3, &sock);
    }

    rev = ftp_quit(&sock);
    if(rev)
    {
        reclaim_resource(CLOSESOCK, &sock);
        reclaim_resource(DELELIST, NULL);
        reclaim_resource(DELEDIR, NULL);
        if(file_or_dir == DIRE)
        {
            record_log(LCOPYEND, copy_data.dir, rev);
        }
        else if(file_or_dir == ONEFILE)
        {
            record_log(LCOPYEND, copy_data.filename, rev);
        }
    }

    rev = estimate_cancel();
    if(rev)
    {
        copy_shutdown(2, NULL);
    }

    //set process status
    set_status(EMPTY);
    rev = info_return(STASHM, 0, 0, 0, 0x55);
    if(rev)
    {
        record_log(LPROCESSERROR, NULL, 0x29);
    }

    //free head list
    delete_headlist();
    //check CF CARD access
    //CF CARD UPDATE
    set_status(UPDATING);
    rev = info_return(STASHM, 0, 0, 0, 0x56);
    if(rev)
    {
        reclaim_resource(DELEDIR, NULL);
        record_log(LPROCESSERROR, NULL, 0x29);
    }

    //CF update
    if(file_or_dir == DIRE)
    {
        sprintf(source_path, "%s%s", TEMP_DIR1, copy_data.dir);
        rev = cf_update(source_path, CF_DIR);
        if(rev)
        {
            set_fileget_flag(0);
            reclaim_resource(DELEDIR, NULL);
            record_log(LCOPYEND, copy_data.dir, rev);
            shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
            if(shm_ret)
            {
                record_log(LPROCESSERROR, NULL, 0x29);
            }
            return ((void* )rev);
        }

    }
    else if(file_or_dir == ONEFILE)
    {
        sprintf(source_path, "%s%s", TEMP_DIR1, copy_data.filename);
        sprintf(destination_path, "%s%s", CF_DIR,  copy_data.filename);
        rev = file_update(source_path, destination_path);
        if(rev)
        {
            set_fileget_flag(0);
            reclaim_resource(DELEDIR, NULL);
            record_log(LCOPYEND, copy_data.filename, rev);
            shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
            if(shm_ret)
            {
                record_log(LPROCESSERROR, NULL, 0x29);
            }
            return ((void* )rev);
        }
    }
    else
    {
        set_fileget_flag(0);
        reclaim_resource(DELEDIR, NULL);
        record_log(LPROCESSERROR, NULL, 0x29);
        return ((void* ) -1);
    }

    rev = dir_delete(TEMP_DIR1);
    if(rev)
    {
        set_fileget_flag(0);
        reclaim_resource(DELEDIR, NULL);
        if(file_or_dir == DIRE)
        {
            record_log(LCOPYEND, copy_data.dir, rev);
        }
        else if(file_or_dir == ONEFILE)
        {
            record_log(LCOPYEND, copy_data.filename, rev);
        }
        shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
        if(shm_ret)
        {
            record_log(LPROCESSERROR, NULL, 0x29);
        }
        return ((void* )rev);
    }

    rev = dir_delete(TEMP_DIR2);
    if(rev)
    {
        set_fileget_flag(0);
        reclaim_resource(DELEDIR, NULL);
        if(file_or_dir == DIRE)
        {
            record_log(LCOPYEND, copy_data.dir, rev);
        }
        else if(file_or_dir == ONEFILE)
        {
            record_log(LCOPYEND, copy_data.filename, rev);
        }
        shm_ret = info_return(CMDSHM, COPYCMD, 2, 2, rev);
        if(shm_ret)
        {
            record_log(LPROCESSERROR, NULL, 0x29);
        }
        return ((void* )rev);
    }

    rev = estimate_cancel();
    if(rev)
    {
        set_status(EMPTY);
        set_fileget_flag(0);
        rev = info_return(CMDSHM, CANCELCMD, 2, 1, 0);
        if(rev)
        {
            record_log(LPROCESSERROR, NULL, 0x29);
        }
    }

    set_status(EMPTY);
    rev = info_return(STASHM, 0, 0, 0, 0x57);
    if(rev)
    {
        record_log(LPROCESSERROR, NULL, 0x29);
    }

    rev = info_return(CMDSHM, COPYCMD, 2, 1, 0);
    if(rev)
    {
        record_log(LPROCESSERROR, NULL, 0x29);
    }


    if(file_or_dir == DIRE)
    {
        record_log(LCOPYEND, copy_data.dir, LOK);
    }
    else if(file_or_dir == ONEFILE)
    {
        record_log(LCOPYEND, copy_data.filename, LOK);
    }
    set_fileget_flag(0);
    return ((void* )0);
}

