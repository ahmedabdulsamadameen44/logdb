#include "row.h"
#include <stdio.h>

int write_row(Row *row , FILE *file)
{
    size_t written = fwrite(row,sizeof(Row),1,file);
    if (written != 1)
    {
        perror("fwrite failed");
        return 1;
    }
    return 0;
}

int read_row(Row *row , FILE *file)
{
    size_t read = fread(row,sizeof(Row),1,file);
    if (feof(file))
    {
        return 1;
    }
    
    if (read != 1)
    {
        perror("fread failed");
        return -2;
    }
    return 0;
}