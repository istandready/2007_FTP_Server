#include "ftp.h"
#include "shm.h"

char*
see_malloc(int size)
{
#ifdef SEE_MALLOC
    count++;
    /*printf("count++ == %d\n", count);*/
#endif
    return (malloc(size));
}

void
see_free(char* addr)
{
#ifdef SEE_MALLOC
    count--;
    /*printf("count-- == %d\n", count);*/
#endif
    free(addr);
    return;
}
/*****************************************************************
 *   NAME: accept_list
 *   PARAMETER:
 *            datasock_ptr:data-port socket resource
 *            dirname     :directory name
 *            head        :list head address
 *   RETURN VALUE:
 *               0: execute success
 *          other : execute failure
 *   DESCRIPTION:
 *          accept list information
 *
 ******************************************************************/
int
accept_list(struct ST_DATASOCK * datasock_ptr, char* dir_name, struct list * head)
{
    int rev = 0;
    struct list * s = NULL, * r = NULL;
    struct store_list * m = NULL, * n = NULL, * l = NULL;
    int flag = 0, head_flag = 0, len = 0;
    int size_sockaddr = 0;
    char* DATABUF = NULL;
    fd_set rset, wset;
    struct timeval tv;
    if(datasock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(dir_name == NULL)
        return 0x29; /* memory or process's error occured */
    if(head == NULL)
        return 0x29;
    size_sockaddr = sizeof(datasock_ptr->sin_data);
    FD_ZERO(&rset);
    FD_SET(datasock_ptr->sid_data, &rset);
    wset = rset;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    rev = select(datasock_ptr->sid_data + 1, &rset, &wset, NULL, &tv);
    if(rev > 0)
    {
        datasock_ptr->sid_file = accept(datasock_ptr->sid_data,
                                    (struct sockaddr *)&(datasock_ptr->sin_data),&size_sockaddr);
    }
    else if(rev == 0)
    {
        return REACCEPT;
    }
    else
    {
        write_log("[ERROR INFO] select() error occured in accept_list()\n");
        return 0x29; /*memory or process's error occured*/
    }

    DATABUF = (char* )see_malloc(256);
    if(DATABUF == NULL)
    {
		close(datasock_ptr->sid_file);
		datasock_ptr->sid_file = 0;
        write_log("[ERROR INFO] malloc() error occured in accept_list()\n");
        return 0x29;
    }

    while(flag == 0)
    {
        memset(DATABUF, '\0' , 256);
        /* Read ftp server response msg */
        rev = read_line(datasock_ptr->sid_file, DATABUF, &len);
        if(rev == 0)
        {
            s = (struct list *)see_malloc(sizeof(struct list));
            memcpy(s->data, DATABUF, 256);
            if(head->next == NULL)
            {
                head->next = s;
                if(head_list.head == NULL)
                {
                    head_list.head = s;
                }
                else
                {
                    n = (struct store_list *)see_malloc(sizeof(struct store_list));
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
            see_free(DATABUF);
            close(datasock_ptr->sid_file);
            datasock_ptr->sid_file = 0;
            return rev;
        }

    }
    if (r != NULL)
        r->next = NULL;

    see_free(DATABUF);
    close(datasock_ptr->sid_file);
    datasock_ptr->sid_file = 0;

    return 0;
}
/*****************************************************************
 *   NAME: get_list
 *   PARAMETER:
 *            datasock_ptr:data-port socket resource
 *            sock_ptr    :socket resource
 *            dir_name    :directory name
 *            head        :list head address
 *   RETURN VALUE:
 *               0: execute success
 *          other : execute failure
 *   DESCRIPTION:
 *         get LIST information from server
 *
 ******************************************************************/
int
get_list(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr,
                                        char* dir_name, struct list * head)
{
    int rev = 0, retry_ind = 0;
    unsigned char* addr_t = NULL;
    unsigned char* port_t = NULL;
    struct ST_DATASOCK data_sock;
    if(copy_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(dir_name == NULL)
        return 0x29; /* memory or process's error occured */
    if(head == NULL)
        return 0x29; /* memory or process's error occured */
    memset(&data_sock, 0, sizeof(struct ST_DATASOCK));

    for(retry_ind = 0; retry_ind <= tcp_retry;)
    {
        /*Creat data connect */
        rev = create_data_conn(sock_ptr, copy_ptr->ipaddress, copy_ptr->port,
                                                    copy_ptr->timeout, &data_sock);
        if(rev)
            return rev;

        addr_t = (unsigned char *)&(data_sock.sin_data).sin_addr;
        port_t = (unsigned char *)&(data_sock.sin_data).sin_port;

        /* Send port command to ftp server */
        rev = port_cmd(sock_ptr, addr_t, port_t);
        if(rev)
        {
            close(data_sock.sid_data);
            data_sock.sid_data = 0;
            return rev;
        }
        /* send list command to ftp server */
        rev = list_cmd(sock_ptr);
        if(rev)
        {
            close(data_sock.sid_data);
            data_sock.sid_data = 0;
            return rev;
        }

        rev = accept_list(&data_sock, dir_name, head);
		close(data_sock.sid_data);
		data_sock.sid_data = 0;
        if(rev)
        {
            if(rev == REACCEPT)
            {
                retry_ind++;
                continue;
            }
            return rev;
        }
        break;
    }
    if(retry_ind > tcp_retry)
    {
        return 0x1e; /* connect FTP Server NG(download) */
    }

    rev = data_fin(sock_ptr);
    if(rev)
        return rev;
    return 0;
}
/*****************************************************************
 *   NAME: delete_filelist
 *   PARAMETER:
 *            head: file_list head
 *   RETURN VALUE:
 *               0: delete list success
 *          other : delete list fail
 *   DESCRIPTION:
 *          Delete file_list
 *
 ******************************************************************/
void
delete_filelist(void)
{
    struct file_list * p , * r;
    if(file_list.next == NULL)
    {
        memset(file_list.file_pathname, 0, 256);
        return;
    }
    p = file_list.next;
    r = file_list.next;
    while(p != NULL)
    {
        r = p;
        p = r->next;
        see_free((char*)r);
        r = NULL;
    }
    memset(file_list.file_pathname, 0, 256);
    file_list.next = NULL;
    return;
}
/*****************************************************************
 *   NAME: delete_list
 *   PARAMETER:
 *            head: list head
 *   RETURN VALUE:
 *               0: delete list success
 *          other : delete list fail
 *   DESCRIPTION:
 *          Delete list
 *
 ******************************************************************/
int
delete_list(struct list * head)
{
    struct list * p , * r;
    if(head == NULL)
        return 0;

    p = head->next;
    r = head->next;
    while(p != NULL)
    {
        r = p;
        p = r->next;
        see_free((char*)r);
        r = NULL;
    }

	see_free((char*)head);
    return 0;
}

/*****************************************************************
 *   NAME: analyse_list
 *   PARAMETER:
 *            copy_ptr :copy information
 *            sock_ptr :socket resource
 *            head     :list information
 *            pwd      :current directory
 *   RETURN VALUE:
 *               0    : execute success
 *               other: execute failure
 *   DESCRIPTION:
 *              analyse list information and copy file
 *
 ******************************************************************/
int
analyse_list(struct COPY_DATA * copy_ptr, struct ST_SOCK * sock_ptr, struct list * head, char* pwd)
{
    int rev = 0;
    unsigned char flag = 0;
    struct ST_REMOTEFILEINFO st_rfi;
    char temp[256];
    char dummy[260];
    struct list * p = NULL;
    struct list * r = NULL;
    if(copy_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(head == NULL)
        return 0x29; /* memory or process's error occured */
    if(pwd == NULL)
        return 0x29; /* memory or process's error occured */
    p = head->next;
    r = head->next;
    struct file_list * fl_p = NULL;
    struct file_list * fl_r = NULL;
    struct file_list * fl_n = NULL;
    memset(dummy, '\0', 260);
    memset(&st_rfi, 0, sizeof(struct ST_REMOTEFILEINFO));

    if(head->next == NULL)
        return 0;

    while(p != NULL)
    {
        r = p;
        memset(temp, '\0' ,256);
        memcpy(temp, pwd, strlen(pwd));
        /*delete space , CR and LF*/
        while((p->data[strlen(p->data)-1]) >= 0 && (p->data[strlen(p->data)-1]) <= 32)
        {
            (p->data[strlen(p->data)-1]) = '\0';
        }
        /*target is a directory*/
        if(p->data[0] == 'd')
        {
            if(dir_level >= MAXDIRLEVEL)
            {
                p = r->next;
                continue;
            }
            rev = set_file_info(p->data, &st_rfi);
            if(rev)
                return rev;
            strcat(temp,"/");
            strcat(temp, st_rfi.name_);
            memcpy(dummy, temp, 256);
            strcat(dummy, "/TXT");
            /*Creat temp dir*/
            rev = make_temp_dir(dummy);
            if(rev)
                return rev;
            rev = traversal_dir(temp, copy_ptr, sock_ptr);
            if(rev)
                return rev;
        }
        /*target is a file*/
        else if(p->data[0] == '-')
        {
            rev = set_file_info(p->data, &st_rfi);
            if(rev)
                return rev;
            if(strlen(st_rfi.name_) == 0)
            {
                p = r->next;
                continue;
            }
            dir_size += st_rfi.size_;
            strcat(temp, "/");
            strcat(temp, st_rfi.name_);
            if(strlen(file_list.file_pathname) == 0)
            {
                memcpy(file_list.file_pathname, temp, 256);
            }
            else
            {
                fl_n = (struct file_list *)see_malloc(sizeof(struct file_list));
                memcpy(fl_n->file_pathname, temp, 256);
                fl_n->next = NULL;
                if(file_list.next == NULL)
                {
                    file_list.next = fl_n;
                }
                else
                {
                    flag = 0;
                    fl_p = file_list.next;
                    fl_r = file_list.next;
                    while(flag == 0)
                    {
                        if(fl_p->next == NULL)
                        {
                            fl_p->next = fl_n;
                            flag = 1;
                        }
                        else
                        {
                            fl_p = fl_r->next;
                            fl_r = fl_p;
                        }
                    }
                }

            }
        }
        p = r->next;
    }
    return 0;
}

/*****************************************************************
 *   NAME: delete_headlist
 *   PARAMETER: NONE
 *   RETURN VALUE: NONE
 *   DESCRIPTION:
 *      Delete list
 *
 ******************************************************************/
void
delete_headlist(void)
{
    struct store_list * m = NULL, * n = NULL;
    if(head_list.head == NULL)
        return;

    if(head_list.head != NULL)
        delete_list(head_list.head);

    m = head_list.next;
    n = head_list.next;
    /*delete list*/
    while(m != NULL)
    {
        n = m;
        m = n->next;
        delete_list(n->head);
        see_free((char*)n);
        n = NULL;
    }

    head_list.head = NULL;
    head_list.next = NULL;
    return;

}

