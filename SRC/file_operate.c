#include "ftp.h"
#include "shm.h"

/*****************************************************************
 *   NAME: make_dir
 *   PARAMETER: pathname
 *   RETURN VALUE:
 *       0    : make dir success
 *       other: make dir fail
 *   DESCRIPTION:
 *      make dir
 *
 ******************************************************************/
int
make_dir(char* pathname)
{
    int rev = 0;
    int len = 0;
    int ind = 0;
    char mk_path[128];
    memset(mk_path, '\0', 128);

    if(pathname == NULL)
        return 0x29; /* memory or process's error occured */
    if(pathname[0] != '/')
    {
        return 0x2d; /* directory is not correct */
    }

    len = strlen(pathname);
    while(ind <= len)
    {
        if(ind == len)
        {
            memcpy(mk_path, pathname, ind+1);
            rev = mkdir(mk_path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if(rev == -1)
            {
                if(errno != EEXIST)
                    return 0x16; /* create directory NG */
            }
            ind++;
            continue;
        }
        if(pathname[ind] == '/')
        {
            if(ind == 0)
            {
                ind++;
                continue;
            }
            else
            {
                memcpy(mk_path, pathname, ind+1);
                rev = mkdir(mk_path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                if(rev == -1)
                {
                    if(errno != EEXIST)
                        return 0x16; /* create directory NG */
                }
            }
        }
        ind++;
    }
    return 0;
}

/*****************************************************************
 *   NAME: distill_path
 *   PARAMETER: filename : filename
 *              pathname_1 : temporary directory
 *              pathname_2 : temporary directory
 *   RETURN VALUE:
 *       0    : distill path success
 *       other: fail
 *   DESCRIPTION:
 *      distill path from filename
 *
 ******************************************************************/
int
distill_path(char* filename, char* pathname_1, char* pathname_2)
{
    int offset = 0;
    char* p = NULL;
    char file_name[65];

    if(pathname_1 == NULL)
        return 0x29; /* memory or process's error occured */
    if(pathname_2 == NULL)
        return 0x29; /* memory or process's error occured */
    if(filename == NULL)
        return 0x29; /* memory or process's error occured */
    memset(pathname_1, '\0', 128);
    memset(pathname_2, '\0', 128);
    memcpy(file_name, filename, 65);
    memcpy(pathname_1, TEMP_DIR1, strlen(TEMP_DIR1));
    memcpy(pathname_2, TEMP_DIR2, strlen(TEMP_DIR2));
    p = strrchr(file_name, '/');
    if(p == NULL)
        return 0x2d; /* directory is not correct */
    memcpy((pathname_1 + strlen(TEMP_DIR1)), file_name, (p - file_name + 1));
    memcpy((pathname_2 + strlen(TEMP_DIR2)), file_name, (p - file_name + 1));
    offset = strlen(TEMP_DIR2) + (p - file_name + 1);
    if(offset >= 128)
        return 0x2d; /* directory is not correct */
    pathname_1[offset] = '\0';
    pathname_2[offset] = '\0';
    return 0;
}
/*****************************************************************
 *   NAME: make_temp_dir
 *   PARAMETER: filename
 *   RETURN VALUE:
 *       0    : make dir success
 *       other: make dir fail
 *   DESCRIPTION:
 *      make tmp dir
 *
 ******************************************************************/
int
make_temp_dir(char* filename)
{
    int rev = 0;
    char pathname_1[128];
    char pathname_2[128];
    if(filename == NULL)
        return 0x29; /* memory or process's error occured */
    memset(pathname_1, '\0', 128);
    memset(pathname_2, '\0', 128);

    rev = distill_path(filename, pathname_1, pathname_2);
    if(rev)
        return rev;
    rev = make_dir(pathname_1);
    if(rev)
        return rev;

    rev = make_dir(pathname_2);
    if(rev)
        return rev;

    return 0;
}
/*****************************************************************
 *   NAME: size_compare
 *   PARAMETER:
 *             pathname_1: file name1
 *             pathname_2: file name2
 *   RETURN VALUE:
 *       0    : filename1 the same size and filename2
 *       other: filename1 the diffent size and filename2
 *   DESCRIPTION:
 *      The number of bytes compared document
 *
 ******************************************************************/
int
size_compare(char* pathname_1, char* pathname_2)
{
    int rev = 0;
    struct stat file_status;
    long long int size_1 = 0, size_2 = 0;
    if(pathname_1 == NULL)
        return 0x29; /* memory or process's error occured */
    if(pathname_2 == NULL)
        return 0x29; /* memory or process's error occured */
    /*For the status of pathname_1*/
    rev = stat(pathname_1, &file_status);
    if(rev < 0)
    {
        if(errno == ENOENT)
            return 0x20; /* downloaded file is not exist */
        else
        {
            write_log("[ERROR INFO] stat() error occured in size_compare()\n");
            return 0x29; /* memory or process's error occured */
        }

    }
    size_1 = file_status.st_size;
    /*For the status of pathname_2*/
    rev = stat(pathname_2, &file_status);
    if(rev < 0)
    {
        if(errno == ENOENT)
            return 0x26; /* downloaded file is not exist */
        else
        {
            write_log("[ERROR INFO] stat() error occured in size_compare()\n");
            return 0x29; /* memory or process's error occured */
        }
    }
    size_2 = file_status.st_size;

    if(size_1 != size_2)
        return 0x22;  /* compare file's size NG */

    return 0;
}
/*****************************************************************
 *   NAME: binary_compare
 *   PARAMETER:
 *             pathname_1: file name1
 *             pathname_2: file name2
 *   RETURN VALUE:
 *       0    : filename1 the same Character and filename2
 *       other: filename1 the diffent Character and filename2
 *   DESCRIPTION:
 *      The number of Character compared document
 *
 ******************************************************************/
int
binary_compare(char* pathname_1, char* pathname_2)
{
    int fp_1 = 0, fp_2 = 0;
    long long int lSize;
    char * buffer_1 = NULL, * buffer_2 = NULL;
    long long int result = 0;
    int rev = 0;
    if(pathname_1 == NULL)
        return 0x29; /* memory or process's error occured */
    if(pathname_2 == NULL)
        return 0x29; /* memory or process's error occured */
    /*Open pathname_1*/
    fp_1 = open(pathname_1, O_RDONLY);
    if (fp_1 < 0)
    {
        if(errno == ENOENT)
            return 0x20; /* downloaded file is not exist */
        else
        {
            write_log("[ERROR INFO] open() error occured in binary_compare()\n");
            return 0x29; /* memory or process's error occured */
        }
    }
    /*Open pathname_2*/
    fp_2 = open(pathname_2, O_RDONLY);
    if (fp_2 < 0)
    {
        close(fp_1);
        if(errno == ENOENT)
            return 0x20; /* downloaded file is not exist */
        else
        {
            write_log("[ERROR INFO] open() error occured in binary_compare()\n");
            return 0x29; /* memory or process's error occured */
        }
    }

    /* obtain file size:*/
    lseek(fp_1 , 0, SEEK_END);
    lSize = lseek(fp_1, 0, SEEK_CUR);
    if(lSize < 0)
    {
        close(fp_1);
        close(fp_2);
        return 0x20;
    }

    lseek(fp_1, 0, SEEK_SET);
    /* allocate memory to contain the whole file:*/
    buffer_1 = (char*) see_malloc (sizeof(char) * lSize);
    if (buffer_1 == NULL)
    {
        close(fp_1);
        close(fp_2);
        write_log("[ERROR INFO] malloc() error occured in binary_compare()\n");
        return 0x29; /* memory or process's error occured */
    }
    memset(buffer_1, 0, lSize);
    /* copy the file into the buffer:*/
    result = read(fp_1, buffer_1, lSize);
    if (result < 0)
    {
        close(fp_1);
        close(fp_2);
        see_free(buffer_1);
        write_log("[ERROR INFO] read() error occured in binary_compare()\n");
        return 0x29; /* memory or process's error occured */
    }

    lseek(fp_2 , 0 , SEEK_END);
    lSize = lseek(fp_2, 0, SEEK_CUR);
    if(lSize < 0)
    {
        see_free(buffer_1);
        close(fp_1);
        close(fp_2);
        return 0x20;
    }
    lseek(fp_2, 0, SEEK_SET);
    /* allocate memory to contain the whole file:*/
    buffer_2 = (char*) see_malloc (sizeof(char) * lSize);
    if (buffer_2 == NULL)
    {
        close(fp_1);
        close(fp_2);
        see_free(buffer_1);
        write_log("[ERROR INFO] malloc() error occured in binary_compare()\n");
        return 0x29; /* memory or process's error occured */
    }
    memset(buffer_2, 0, lSize);
    /* copy the file into the buffer:*/
    result = read(fp_2, buffer_2, lSize);
    if (result < 0)
    {
        close(fp_1);
        close(fp_2);
        see_free(buffer_1);
        see_free(buffer_2);
        write_log("[ERROR INFO] read() error occured in binary_compare()\n");
        return 0x29; /* memory or process's error occured */
    }
    rev = memcmp(buffer_1, buffer_2, lSize);
    if(rev)
    {
        close(fp_1);
        close(fp_2);
        see_free(buffer_1);
        see_free(buffer_2);
        return 0x21; /* compare file's binary NG */
    }

    close(fp_1);
    close(fp_2);
    see_free(buffer_1);
    see_free(buffer_2);
    return 0;
}
/*****************************************************************
 *   NAME: file_compare
 *   PARAMETER:
 *             pathname_1: file name1
 *             pathname_2: file name2
 *   RETURN VALUE:
 *       0    : filename1 and filename2 equal
 *       other: filename1 and filename2 not equal
 *   DESCRIPTION:
 *      Comparing the two files
 *
 ******************************************************************/
int
file_compare(char* pathname_1, char* pathname_2)
{
    int rev = 0;
    if(pathname_1 == NULL)
        return 0x29; /* memory or process's error occured */
    if(pathname_2 == NULL)
        return 0x29; /* memory or process's error occured */
    rev = size_compare(pathname_1, pathname_2);
    if(rev)
        return rev;

    rev = binary_compare(pathname_1, pathname_2);
    if(rev)
        return rev;

    return 0;
}
/*****************************************************************
 *   NAME: file_update
 *   PARAMETER:
 *             source_path: Source file
 *             destination_path: destination file
 *   RETURN VALUE:
 *       0    : copy file success
 *       other: copy file fail
 *   DESCRIPTION:
 *      copy file
 *
 ******************************************************************/
int
file_update(char* source_path, char* destination_path)
{
    int fp_src = 0;
    DIR* fp_des = NULL;
    int rev = 0;
    char* p = NULL;
    char command[256];
    char path_name[128];
    char file_name[128];
    if(source_path == NULL)
        return 0x29; /* memory or process's error occured */
    if(destination_path == NULL)
        return 0x29; /* memory or process's error occured */
    memset(command, '\0', 256);
    memset(path_name, '\0', 128);
    memset(file_name, '\0', 128);
    memcpy(file_name, destination_path, strlen(destination_path));
    p = strrchr(file_name, '/');
    if(p == NULL)
        return 0x29; /* memory or process's error occured */
    memcpy(path_name, file_name, (p - file_name + 1 ));
    /*open source_path file*/
    fp_src = access(source_path, R_OK);
    if(fp_src < 0)
    {
        if(errno == EACCES || errno == ENOENT)
            return 0x27; /* update file NG */
        write_log("[ERROR INFO] access() error occured in file_update()\n");
        return 0x29; /* memory or process's error occured */
    }

    /*open destination_path file*/
    fp_des = opendir(path_name);
    if(fp_des == NULL)
    {
        if(errno == EACCES || errno == ENOENT)
        {
            make_dir(path_name);
            fp_des = opendir(path_name);
            if(fp_des == NULL)
                return 0x28; /* CF access NG */
        }
        else
        {
            write_log("[ERROR INFO] opendir() error occured in file_update()\n");
            return 0x29; /* memory or process's error occured */
        }
    }

    rev = closedir(fp_des);
    if(rev)
    {
        write_log("[ERROR INFO] closedir() error occured in file_update()\n");
        return 0x29; /* memory or process's error occured */
    }

    /*Executive Order copy*/
    sprintf(command, "%s%s %s", CPFILE, source_path, destination_path);
    rev = system(command);
    if(rev < 0)
    {
        write_log("[ERROR INFO] system() error occured in file_update()\n");
        return 0x29; /* memory or process's error occured */
    }

    else if(rev >0)
        return 0x28; /* CF CARD access NG*/
    return 0;
}
/*****************************************************************
 *   NAME: cf_update
 *   PARAMETER:
 *             pathname   : Source Directory
 *             cf_pathname: destination Directory
 *   RETURN VALUE:
 *       0    : copy  success
 *       other: copy  fail
 *   DESCRIPTION:
 *      copy dir
 *
 ******************************************************************/
int
cf_update(char* pathname, char* cf_pathname)
{
    int rev = 0;
    DIR* dir_s = NULL;
    DIR* dir_d = NULL;
    char command[256];
    if(pathname == NULL)
        return 0x29; /* memory or process's error occured */
    if(cf_pathname == NULL)
        return 0x29; /* memory or process's error occured */
    memset(command, '\0', 256);
    /*open pathname Directory*/
    dir_s = opendir(pathname);
    if(dir_s == NULL)
    {
        if(errno == EACCES || errno == ENOENT)
            return 0x27; /* update file NG */
        write_log("[ERROR INFO] opendir() error occured in cf_update()\n");
        return 0x29; /* memory or process's error occured */
    }
    /*close pathname Directory*/
    rev = closedir(dir_s);
    if(rev)
    {
        write_log("[ERROR INFO] closedir() error occured in cf_update()\n");
        return 0x29; /* memory or process's error occured */
    }

    /*open cf_pathname Directory*/
    dir_d = opendir(cf_pathname);
    if(dir_d == NULL)
    {
        if(errno == EACCES || errno == ENOENT)
            return 0x28; /* CF access NG */
        write_log("[ERROR INFO] opendir() error occured in cf_update()\n");
        return 0x29; /* memory or process's error occured */
    }
    /*close cf_pathname Directory*/
    rev = closedir(dir_d);
    if(rev)
    {
        write_log("[ERROR INFO] closedir() error occured in cf_update()\n");
        return 0x29; /* memory or process's error occured */
    }
     /*Executive Order copy*/
    sprintf(command, "%s%s %s", CPDIR, pathname, cf_pathname);
    rev = system(command);
    if(rev < 0)
    {
        write_log("[ERROR INFO] system() error occured in cf_update()\n");
        return 0x29; /* memory or process's error occured */
    }
    else if(rev >0)
        return 0x28; /* CF CARD access NG*/
    return 0;
}

/*****************************************************************
 *   NAME: dir_delete
 *   PARAMETER:
 *             pathname   : Delete dir name
 *   RETURN VALUE:
 *       0    : delete dir success
 *       other: delete dir fail
 *   DESCRIPTION:
 *      delete dir
 *
 ******************************************************************/
int
dir_delete(char* pathname)
{
    int rev = 0;
    DIR* dir = NULL;
    char command[256];
    if(pathname == NULL)
        return 0x29; /* memory or process's error occured */

    memset(command, '\0', 256);
    /*open pathname dir*/
    dir = opendir(pathname);
    if(dir == NULL)
    {
        if(errno == EACCES || errno == ENOENT)
            return 0x27; /* update file NG */
        write_log("[ERROR INFO] opendir() error occured in dir_delete()\n");
        return 0x29; /* memory or process's error occured */
    }
    /*close pathname dir*/
    rev = closedir(dir);
    if(rev)
    {
        write_log("[ERROR INFO] closedir() error occured in dir_delete()\n");
        return 0x29; /* memory or process's error occured */
    }
     /*Executive Order delete*/
    sprintf(command, "%s%s", DELCMD, pathname);
    rev = system(command);
    if(rev < 0)
    {
        write_log("[ERROR INFO] system() error occured in dir_delete()\n");
        return 0x29; /* memory or process's error occured */
    }
    else if(rev > 0)
        return 0x1b; /* delete temp directory NG*/

    return 0;
}
/*****************************************************************
 *   NAME: local_size
 *   PARAMETER:
 *             NONE
 *   RETURN VALUE:
 *       0    :  Space meet
 *       other: Lack of space
 *   DESCRIPTION:
 *      Calculation of the remaining space
 *
 ******************************************************************/
int
local_size(void)
{
    int rev = 0;
    struct statfs fs_status;
    unsigned long long free_size = 0;
    rev = statfs(TEMP_DIR, &fs_status);
    free_size = (fs_status.f_bavail) * (fs_status.f_bsize);
    if(free_size <= (2*dir_size))
        return 0x32; /*HDD space is not enough to save temp files*/
    return 0;
}
