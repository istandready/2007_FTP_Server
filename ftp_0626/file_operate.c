#include "ftp.h"

int
make_dir(char* pathname)
{
    int rev = 0;
    int len = strlen(pathname);
    int ind = 0;
    char mk_path[128];
    memset(mk_path, '\0', 128);
    if(pathname[0] != '/')
    {
        return 0x2d;
    }
    while(ind <= len)
    {
        if(ind == len)
        {
            memcpy(mk_path, pathname, ind+1);
            rev = mkdir(mk_path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if(rev == -1)
            {
                if(errno != EEXIST)
                    return 0x16;
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
                        return 0x16;
                }
            }
        }
        ind++;
    }
    return 0;
}

int
distill_path(char* filename, char* pathname_1, char* pathname_2)
{
    int offset = 0;
    char* p;
    char file_name[65];
    memset(pathname_1, '\0', 128);
    memset(pathname_2, '\0' , 128);
    memcpy(file_name, filename, 65);
    memcpy(pathname_1, TEMP_DIR1, strlen(TEMP_DIR1));
    memcpy(pathname_2, TEMP_DIR2, strlen(TEMP_DIR2));
    p = strrchr(file_name, '/');
    memcpy((pathname_1 + strlen(TEMP_DIR1)), file_name, ( p - file_name + 1 ));
    memcpy((pathname_2 + strlen(TEMP_DIR2)), file_name, ( p - file_name + 1 ));
    offset = strlen(TEMP_DIR2) + ( p - file_name + 1 );
    if(offset >= 128)
        return 0x2d;
    pathname_1[offset] = '\0';
    pathname_2[offset] = '\0';
    return 0;
}

int
make_temp_dir(char* filename)
{
    int rev = 0;
    char pathname_1[128];
    char pathname_2[128];
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

int
size_compare(char* pathname_1, char* pathname_2)
{
    int rev = 0;
    struct stat file_status;
    int size_1 = 0, size_2 = 0;
    rev = stat(pathname_1, &file_status);
    if(rev < 0)
    {
        if(errno == ENOENT)
            return 0x20;
        else
            return 0x29;
    }
    size_1 = file_status.st_size;

    rev = stat(pathname_2, &file_status);
    if(rev < 0)
    {
        if(errno == ENOENT)
            return 0x26;
        else
            return 0x29;
    }
    size_2 = file_status.st_size;

    if(size_1 != size_2)
        return 0x22;

    return 0;
}

int
binary_compare(char* pathname_1, char* pathname_2)
{
    FILE * fp_1, *fp_2;
    long lSize;
    char * buffer_1 , * buffer_2;
    size_t result;
    int rev = 0;
    fp_1 = fopen ( pathname_1 , "rb" );
    if (fp_1==NULL)
    {
        if(errno == ENOENT)
            return 0x20;
        else
            return 0x29;
    }

    fp_2 = fopen ( pathname_2, "rb" );
    if (fp_2==NULL)
    {
        if(errno == ENOENT)
            return 0x20;
        else
            return 0x29;
    }

    // obtain file size:
    fseek (fp_1 , 0 , SEEK_END);
    lSize = ftell (fp_1);
    rewind (fp_1);
    // allocate memory to contain the whole file:
    buffer_1 = (char*) malloc (sizeof(char)*lSize);
    if (buffer_1 == NULL)
    {
        return 0x29;
    }

    // copy the file into the buffer:
    result = fread (buffer_1,1,lSize,fp_1);
    if (result != lSize)
    {
        return 0x29;
    }

    fseek (fp_2 , 0 , SEEK_END);
    lSize = ftell (fp_2);
    rewind (fp_2);
    // allocate memory to contain the whole file:
    buffer_2 = (char*) malloc (sizeof(char)*lSize);
    if (buffer_2 == NULL)
    {
        return 0x29;
    }
    // copy the file into the buffer:
    result = fread (buffer_2,1,lSize,fp_2);
    if (result != lSize)
    {
        return 0x29;
    }
    rev = memcmp(buffer_1, buffer_2, lSize);
    if(rev)
    {
        return 0x21;
    }

    fclose (fp_1);
    fclose (fp_2);
    free (buffer_1);
    free (buffer_2);
    return 0;
}

int
file_compare(char* pathname_1, char* pathname_2)
{
    int rev = 0;
    rev = size_compare(pathname_1, pathname_2);
    if(rev)
        return rev;

    rev = binary_compare(pathname_1, pathname_2);
    if(rev)
        return rev;

    return 0;
}

int
file_update(char* source_path, char* destination_path)
{
    FILE* fp_src = NULL, * fp_des = NULL;
    int rev = 0;
    char command[256];
    memset(command, '\0', 256);

    fp_src = fopen(source_path, "rb");
    if(fp_src == NULL)
    {
        if(errno == EACCES || errno == ENOENT)
            return 0x27;
        return 0x29;
    }

    rev = fclose(fp_src);
    if(rev)
        return 0x29;

    fp_des = fopen(destination_path, "rb");
    if(fp_des == NULL)
    {
        if(errno == EACCES || errno == ENOENT)
            return 0x28;
        return 0x29;
    }

    rev = fclose(fp_des);
    if(rev)
        return 0x29;

    sprintf(command, "%s%s %s", CPFILE, source_path, destination_path);
    system(command);

    return 0;
}

int
cf_update(char* pathname, char* cf_pathname)
{
    int rev = 0;
    DIR* dir_s = NULL;
    DIR* dir_d = NULL;
    char command[256];
    memset(command, '\0', 256);
    dir_s = opendir(pathname);
    if(dir_s == NULL)
    {
        if(errno == EACCES || errno == ENOENT)
            return 0x27;
        return 0x29;
    }
    rev = closedir(dir_s);
    if(rev)
        return 0x29;

    dir_d = opendir(cf_pathname);
    if(dir_d == NULL)
    {
        if(errno == EACCES || errno == ENOENT)
            return 0x28;
        return 0x29;
    }
    rev = closedir(dir_d);
    if(rev)
        return 0x29;

    sprintf(command, "%s%s %s", CPDIR, pathname, cf_pathname);
    system(command);
    return 0;
}

int
dir_delete(char* pathname)
{
    int rev = 0;
    DIR* dir = NULL;
    char command[256];
    memset(command, '\0', 256);
    dir = opendir(pathname);
    if(dir == NULL)
    {
        if(errno == EACCES || errno == ENOENT)
            return 0x27;
        return 0x29;
    }
    rev = closedir(dir);
    if(rev)
        return 0x29;
    sprintf(command, "%s%s", DELCMD, pathname);
    system(command);

    return 0;
}

int
local_size(unsigned long file_size)
{
    int rev = 0;
    struct statfs fs_status;
    rev = statfs(FTP_PATH, &fs_status);
    if(fs_status.f_bfree <= (2*file_size))
        return 0x32;
    return 0;
}
