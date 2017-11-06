#ifndef __SRM_IPC_H__
#define __SRM_IPC_H__

#include "com.h"
#include "shm.h"

/*---- PROTOTYPE DECLARATION -------------------------------------------------*/
int srm_getipc( void );
int srm_getshm_cmd( struct ST_SR_SHMINFO*, struct ST_SR_IPC_DATA* );
int srm_dtcshm( void* );

#endif

