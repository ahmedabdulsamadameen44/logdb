// Portions of this  implementation were developed with 
// assistance from Claude (Anthropic)


#include "row.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>



int parse_line(char *line, Row *row) {
    char buf[512];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *token;

    strtok(buf, ",");                          // index — skip
    token = strtok(NULL, ",");                 // host
    if (token == NULL) return 1;
    strncpy(row->host, token, HOST_MAX - 1);
    row->host[HOST_MAX - 1] = '\0';

    strtok(NULL, ",");                         // time — skip
    strtok(NULL, ",");                         // method — skip

    token = strtok(NULL, ",");                 // url -> path
    if (token == NULL) return 1;
    strncpy(row->path, token, LOG_PATH_MAX - 1);
    row->path[LOG_PATH_MAX - 1] = '\0';

    token = strtok(NULL, ",");                 // response -> status
    if (token == NULL) return 1;
    row->status = atoi(token);

    token = strtok(NULL, ",\n");                // bytes (strip trailing newline too)
    if (token == NULL) return 1;
    row->bytes = atoi(token);

    return 0;
}