/*---- INCLUDE DECLARATION ---------------------------------------------------*/

/*---- SYSTEM ----*/
#include <string.h>
#include <sys/types.h>

/*---- USER ----*/
#include "srm_ipc.h"


/*---- GLOBAL VARIABLE DECLARATION -------------------------------------------*/

struct ST_SR_IPC_DATA sr_shmdat = { ( key_t )-1, ( int )-1, NULL };

