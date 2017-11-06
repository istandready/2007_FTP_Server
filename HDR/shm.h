#ifndef __SHM_H__
#define __SHM_H__

#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>

#define     KEYPROC_PATH_CMD        "/usr/local/sbin/PROG/SR_COPY/sr_copy"
#define     PROJ_ID_CMD             'F'

#define     MAX_PATHLEN             ( size_t )128
#define     SHM_SIZE_CMD            ( size_t )256


/*---- EXTERN PROTOTYPE DECLARATION ------------------------------------------*/

/*-- VARIABLE  --*/
extern struct ST_SR_IPC_DATA sr_shmdat;

/*-- PROTOTYPE --*/
extern int srm_getipc( void );
extern int srm_dtcshm( void* );


/*---- STRUCT DECLARATION ----------------------------------------------------*/

struct ST_SR_IPC_DATA
{
    key_t  key;        /* IPC KEY-ID   */
    int    id;         /* IPC SHM-ID   */
    void*  adr;        /* IPC SHM-ADDR */
};

struct ST_SR_SHMINFO
{
    char    path[MAX_PATHLEN];  /* Key-file path */
    char    proj_id;            /* Project ID    */
    size_t  size;               /* IPC SHM-size  */
};


/*
 * base data size = ( 1data-type % 4byte = 0 )
 */
struct COPY_INFO
{
                char    userName[16];
                char    pwd[16];
    unsigned    char    itype;
                char    nop1[3];
                char    ipAddress[16];
    unsigned    short   port;
                short   nop2;
                char    FileName[68];
                char    Dir[32];
    unsigned    short   max_length;
                short   nop3;
    unsigned    char    re_download;
    unsigned    char    tcp_retry;
    unsigned    char    timeout;
                char    nop4;
};

struct RE
{
    unsigned    char    excute_flag;
    unsigned    char    result;
    unsigned    char    error;
                char    nop;
};

struct CANCEL_INFO
{
    unsigned    char    cancel;
                char    nop[3];
};

struct CAN_RE
{
    unsigned    char    excute_flag;
    unsigned    char    result;
                char    nop[2];
};
struct EXIT_INFO
{
    unsigned    char    ftp_exit;
                char    nop[3];
};

struct ST_COPY          /* copy information */
{
    struct COPY_INFO    copy_info;
    struct RE           copy_re;
};

struct ST_CANCEL        /* cancel information */
{
    struct CANCEL_INFO  cancel_info;
    struct CAN_RE       cancel_re;
};

struct ST_EXIT          /* exit  information */
{
    struct EXIT_INFO    exit_info;
    struct RE           exit_re;
};

struct ST_STATUS
{
    unsigned    char    status;
                char    nop[3];
};

struct ST_SHM
{
    struct ST_COPY      copy_cmd;           /* copy   information */
    struct ST_CANCEL    cancel_cmd;         /* cancel information */
    struct ST_EXIT      exit_cmd;           /* exit   information */
    struct ST_STATUS    ftp_status;         /* status information */
};


#endif

