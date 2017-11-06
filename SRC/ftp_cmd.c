#include "ftp.h"
#include "shm.h"

/*****************************************************************
 *   NAME: type_cmd
 *   PARAMETER:
 *            sock_ptr : socket source
 *            type_mode: type mode
 *   RETURN VALUE:
 *               0    : send type command success
 *               other: sne type command fail
 *   DESCRIPTION:
 *          send type command to ftp server
 *
 ******************************************************************/
int
type_cmd(struct ST_SOCK * sock_ptr, char type_mode)
{
    int rev = 0;
    char command[SENDSIZE];
    char response[BUFSIZE];
    int reply_code = 0;
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */

    memset(command, '\0', SENDSIZE);
    memset(response, '\0', BUFSIZE);
    /*Assembly type command */
    sprintf(command,"TYPE %c\r\n", type_mode);
    /*Send type command*/
    rev = send_command(sock_ptr, command);
    if(rev)
        return rev;
    /* Read type command response msg*/
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev)
        return rev;
    /*check type's code*/
    rev = check_reply_code(reply_code, 2);
    if(rev)
    {
        if(rev == -1)
            return 0x19; /* operate fail on the FTP Server */
        return rev;
    }

    return 0;
}

/*****************************************************************
 *   NAME: cwd_cmd
 *   PARAMETER:
 *            sock_ptr:socket resource
 *            dir     :target directory
 *   RETURN VALUE:
 *               0    : The successful implementation of cwd command
 *               other: The fail implementation of cwd command
 *   DESCRIPTION:
 *         Execute Order cwd command
 *
 ******************************************************************/

int
cwd_cmd(struct ST_SOCK * sock_ptr, char* dir)
{
    char command[SENDSIZE];
    char response[BUFSIZE];
    int rev = 0;
    int reply_code = 0;
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(dir == NULL)
        return 0x29; /* memory or process's error occured */
    memset(command, '\0', SENDSIZE);
    memset(response, '\0', BUFSIZE);

    sprintf(command,"CWD %s\r\n",dir);
    /*Send cwd command to Ftp server*/
    rev = send_command(sock_ptr, command);
    if(rev)
        return rev;
    /*recv Ftp server response msg*/
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev)
        return rev;
    /*check cwd's code*/
    rev = check_reply_code(reply_code, 2);
    if(rev)
    {
        if(rev == -1)
            return 0x2d; /* directory is not correct */
        return rev;
    }

    return 0;
}

/*****************************************************************
 *   NAME: pass_cmd
 *   PARAMETER:
 *            sock_ptr:socket resource
 *            pwd     :pass word
 *   RETURN VALUE:
 *               0    : Send pass word to Ftp server success
 *               other: Send pass word to Ftp server fail
 *   DESCRIPTION:
 *         execute Order pass command
 *
 ******************************************************************/
int
pass_cmd(struct ST_SOCK * sock_ptr, char* pwd)
{
    char command[SENDSIZE];
    char response[BUFSIZE];
    int rev = 0;
    int reply_code = 0;
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(pwd == NULL)
        return 0x29; /* memory or process's error occured */
    memset(command, '\0', SENDSIZE);
    memset(response, '\0', BUFSIZE);

    sprintf(command,"PASS %s\r\n",pwd);
    /*Send pass command to Ftp server*/
    rev = send_command(sock_ptr, command);
    if(rev)
        return rev;
    /*Recv Ftp server response msg*/
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev)
        return rev;
    /*check PASS's code*/
    rev = check_reply_code(reply_code, 2);
    if(rev)
    {
        if(rev == -1)
            return 0x31; /* login FTP Server NG */
        return rev;
    }

    return 0;
}

/*****************************************************************
 *   NAME: user_cmd
 *   PARAMETER:
 *            sock_ptr:  socket resource
 *            username:  user name
 *   RETURN VALUE:
 *               0    :  Execute User command success
 *               other:  Execute User command fail
 *   DESCRIPTION:
 *        execute User command
 *
 ******************************************************************/
int
user_cmd(struct ST_SOCK * sock_ptr, char* username)
{
    char command[SENDSIZE];
    char response[BUFSIZE];
    int rev = 0;
    int reply_code = 0;
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(username == NULL)
        return 0x29; /* memory or process's error occured */
    memset(command, '\0', SENDSIZE);
    memset(response, '\0', BUFSIZE);

    sprintf(command,"USER %s\r\n",username);
    /*Send user command to Ftp server*/
    rev = send_command(sock_ptr, command);
    if(rev)
        return rev;
    /*Recv ftp server response msg*/
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev)
        return rev;
    /*check USER's code*/
    rev = check_reply_code(reply_code, 3);
    if(rev)
    {
        if(rev == -1)
            return 0x2a; /* username is not correct */
        return rev;
    }
    return 0;
}

/*****************************************************************
 *   NAME: list_cmd
 *   PARAMETER:
 *            sock_ptr:socket resource
 *   RETURN VALUE:
 *               0:  execute list  command success
 *          other :  execute list  command fail
 *   DESCRIPTION:
 *     execute list  command
 *
 ******************************************************************/
int
list_cmd(struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    char command[SENDSIZE];
    char response[BUFSIZE];
    int reply_code = 0;

    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    memset(response, '\0', BUFSIZE);
    memset(command, '\0', SENDSIZE);

    sprintf(command,"LIST\r\n");
    /*Send list command to ftp server */
    rev = send_command(sock_ptr, command);
    if(rev)
        return rev;
    /* Recv ftp server response */
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev < 0)
        return rev;
    /*check list's code*/
    rev = check_reply_code(reply_code, 1);
    if(rev)
    {
        if(rev == -1)
            return 0x19; /* operate fail on the FTP Server */
        return rev;
    }

    return 0;
}
/*****************************************************************
 *   NAME: port_cmd
 *   PARAMETER:
 *            sock_ptr:socket resource
 *            addr    :ip address
 *            port    :port
 *   RETURN VALUE:
 *               0: execute port cmd success
 *          other : execute port cmd fail
 *   DESCRIPTION:
 *         execute port command
 *
 ******************************************************************/
int
port_cmd(struct ST_SOCK * sock_ptr, unsigned char* addr,unsigned char* port)
{
    int rev = 0;
    char command[SENDSIZE];
    char response[BUFSIZE];
    int reply_code = 0;
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(addr == NULL)
        return 0x29; /* memory or process's error occured */
    if(port == NULL)
        return 0x29; /* memory or process's error occured */
    memset(command, '\0', SENDSIZE);
    memset(response, '\0', BUFSIZE);
    /*send ip address and port to ftp server.*/
    sprintf(command,"PORT %d,%d,%d,%d,%d,%d\r\n",
                    addr[0],addr[1],addr[2],addr[3],port[0],port[1]);
    /*Send port command to ftp server */
    rev = send_command(sock_ptr, command);

    if(rev)
        return rev;
    /* recv ftp server responser msg */
    rev = read_response(sock_ptr, response);

    if(rev)
        return rev;
    rev = get_replycode(response,&reply_code);
    if(rev < 0)
        return rev;
    /*check port's code*/
    rev = check_reply_code(reply_code, 2);
    if(rev)
    {
        if(rev == -1)
            return 0x2c; /* port is not correct */
        return rev;
    }

    return 0;
}

/*****************************************************************
 *   NAME: retr_cmd
 *   PARAMETER:
 *            sock_ptr:scoket resource
 *            filename:file name
 *   RETURN VALUE:
 *               0:  execute retr command success
 *          other :  execute retr command fail
 *   DESCRIPTION:
 *         execute retr command
 *
 ******************************************************************/
int
retr_cmd(struct ST_SOCK * sock_ptr, char* filename)
{
    int rev = 0;
    char command[SENDSIZE];
    char response[BUFSIZE];
    int reply_code = 0;
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    if(filename == NULL)
        return 0x29; /* memory or process's error occured */
    sprintf(command,"RETR %s\r\n",filename);
    /*Send retr command to ftp server */
    rev = send_command(sock_ptr, command);
    if(rev)
        return rev;
    /* recv ftp server responser msg */
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;
    rev = get_replycode(response, &reply_code);
    if(rev)
        return rev;

    /*check retr's code*/
    rev = check_reply_code(reply_code, 1);
    if(rev)
    {
        if(rev == -1)
            return 0x2d; /* directory is not correct */
        return rev;
    }

    return 0;
}

/*****************************************************************
 *   NAME: quit_cmd
 *   PARAMETER:
 *            sock_ptr:socket resource
 *   RETURN VALUE:
 *               0: execute quit command success
 *          other : execute quit command fail
 *   DESCRIPTION:
 *        Execute quit command
 *
 ******************************************************************/
int
quit_cmd(struct ST_SOCK * sock_ptr)
{
    int rev = 0;
    char command[SENDSIZE];
    char response[BUFSIZE];
    int reply_code = 0;
    if(sock_ptr == NULL)
        return 0x29; /* memory or process's error occured */
    memset(command, '\0', SENDSIZE);
    memset(response, '\0', BUFSIZE);

    sprintf(command,"QUIT\r\n");
    /* Send quit command to ftp server */
    rev = send_command(sock_ptr, command);
    if(rev)
        return rev;
    /* recv ftp server response msg */
    rev = read_response(sock_ptr, response);
    if(rev)
        return rev;

    rev = get_replycode(response, &reply_code);
    if(rev < 0)
        return rev;
    /*check quit's code*/
    rev = check_reply_code(reply_code, 2);
    if(rev)
        return rev;

    return 0;
}

