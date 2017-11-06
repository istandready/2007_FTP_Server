/*---- INCLUDE DECLARATION ---------------------------------------------------*/

/*---- SYSTEM ----*/
#include <string.h>
#include <sys/types.h>

/*---- USER ----*/
#include "srm_ipc.h"
#include "shm.h"

/*---- GLOBAL VARIABLE DECLARATION -------------------------------------------*/

//struct ST_SR_IPC_DATA sr_shmdat = { ( key_t )-1, ( int )-1, NULL };


/*#########################   Function Description   #########################*
 *                                                                            *
 *       Subject    :  Get IPC                                                *
 *       Input      :                                                         *
 *       Output     :   0 = OK                                                *
 *                     -1 = NG                                                *
 *       Revision   :                                                         *
 *                                                                            *
 *############################################################################*/
int srm_getipc( void )
{
/*---- VARIABLE DECLARATION --------------------------------------------------*/

    int     result = 0;

    struct ST_SR_SHMINFO shm_info;

/*---- OPERATION -------------------------------------------------------------*/

    memset( &shm_info, 0x00, sizeof( SHM_SIZE_CMD ));

    /*---- safety data setting ----*/
    strncpy(( char* )&shm_info.path,
            ( char* )&KEYPROC_PATH_CMD,
            ( MAX_PATHLEN -1 ));
    shm_info.proj_id = ( char )( PROJ_ID_CMD & 0xff );
    shm_info.size    = ( SHM_SIZE_CMD & 0xffff ); /* local size lock */


    if( sr_shmdat.adr == NULL )
    {
        /* non-get shm */
        result = srm_getshm_cmd( &shm_info, &sr_shmdat );

        if( result == NG )
        {
            return( NG );
        }
    }

    return( OK );
}


/*#########################   Function Description   #########################*
 *                                                                            *
 *       Subject    :  Get SHM( IPC )                                         *
 *       Input      :                                                         *
 *       Output     :   0 = OK                                                *
 *                     -1 = NG                                                *
 *       Revision   :                                                         *
 *                                                                            *
 *############################################################################*/
int srm_getshm_cmd( struct ST_SR_SHMINFO*  shm_info,
                    struct ST_SR_IPC_DATA* shmdat )
{
/*---- VARIABLE DECLARATION --------------------------------------------------*/

    int     bk_err   =  0;   /* temp : errno        */
    key_t   ipc_key  = -1;
    int     shm_id   = -1;
    void*   shm_adr  = NULL;
    char    flg_init = NG;   /* flag : exec initial */

/*---- OPERATION -------------------------------------------------------------*/

    /* GET KEY-ID */
    ipc_key = ftok( shm_info->path, shm_info->proj_id );
    bk_err = errno;

    if( ipc_key == NG )
    {
        /* NG : ftok() */
        return( NG );
    }

    /* GET SHM */
    shm_id = shmget( ipc_key,
                     shm_info->size,
                     ( IPC_CREAT | IPC_EXCL | O_RDWR | 0666 ));
    bk_err = errno;

    if( shm_id == NG )
    {
        /* NG : shmget( CREATE ) */

        /* RETRY */
        switch( bk_err )
        {
            case    EEXIST:    /* exist shm */
                /* shm non-create */
                shm_id = shmget( ipc_key,
                                 shm_info->size,
                                 ( O_RDWR | 0666 ));
                bk_err = errno;
                break;

            default:
                break;
        }

        if( shm_id == NG )
        {
            /* NG : shmget() */
            return( NG );
        }
    }
    else
    {
        flg_init = OK;    /* shm_id create : exec initial */
    }

    shm_adr = shmat( shm_id, NULL, 0 );
    bk_err = errno;

    if(( int )shm_adr == NG )
    {
        /* NG : shmat() */
        return( NG );
    }
    else if( flg_init == OK )
    {
        memset(( char* )shm_adr, 0x00, shm_info->size );
    }

    /*---- complete : save output memory ----*/
    shmdat->key = ipc_key;
    shmdat->id  = shm_id;
    shmdat->adr = shm_adr;

    return( OK );
}


/*#########################   Function Description   #########################*
 *                                                                            *
 *       Subject    :  detach SHM( IPC )                                      *
 *       Input      :  shm_adr                                                *
 *       Output     :   0 = OK                                                *
 *                     -1 = NG                                                *
 *       Revision   :                                                         *
 *                                                                            *
 *############################################################################*/
int srm_dtcshm( void* shm_adr )
{
/*---- VARIABLE DECLARATION --------------------------------------------------*/

/*---- OPERATION -------------------------------------------------------------*/

    if(( int )shm_adr < 1 )
    {
        return( NG );
    }

    /* detach */
    if( shmdt( shm_adr ) == NG )
    {
        /* NG : shmdt() */
        return( NG );
    }

    return( OK );
}

