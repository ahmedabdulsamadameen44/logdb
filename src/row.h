// AI assistance (Claude, Anthropic): reviewed struct layout and
// read/write logic in row.h/row.c.



#ifndef ROW_H
#define ROW_H

#define HOST_MAX 64
#define LOG_PATH_MAX 128
#include <stdio.h>
#include <string.h>

typedef struct {
    char host[HOST_MAX];
    char path[LOG_PATH_MAX];
    int status;
    int bytes;
} Row;

int write_row(Row *row , FILE *file);
int read_row(Row *row , FILE *file);

#endif