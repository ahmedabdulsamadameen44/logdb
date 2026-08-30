
#include "index.h"
#include <stdio.h>
#include <string.h>



int write_index_entry(const char *key, int *rows, int row_count, FILE *file) {
    int key_len = strlen(key);

    if (fwrite(&key_len, sizeof(int), 1, file) != 1) {
        perror("fwrite key_len failed");
        return 1;
    }

    if (fwrite(key, sizeof(char), (size_t)key_len, file) != (size_t)key_len) {
        perror("fwrite key failed");
        return 1;
    }

    if (fwrite(&row_count, sizeof(int), 1, file) != 1) {
        perror("fwrite row_count failed");
        return 1;
    }

    if (fwrite(rows, sizeof(int), (size_t)row_count, file) != (size_t)row_count) {
        perror("fwrite rows failed");
        return 1;
    }

    return 0;
}


int read_index_entry(char **key_out, int **rows_out, int *row_count_out, FILE *file)
{
    int key_len;
    if (fread(&key_len, sizeof(int), 1, file) != 1) {
        if (feof(file)) {
            return 1;
        }
        perror("fread key_len failed");
        return -2;
    }

    char *key = malloc(key_len + 1);
    if (key == NULL) {
        fprintf(stderr, "malloc failed for key\n");
        return -2;
    }
    if (fread(key, sizeof(char), (size_t)key_len, file) != (size_t)key_len) {
        perror("fread key failed");
        free(key);
        return -2;
    }
    key[key_len] = '\0';

    int row_count;
    if (fread(&row_count, sizeof(int), 1, file) != 1) {
        perror("fread row_count failed");
        free(key);
        return -2;
    }

    int *rows = malloc(sizeof(int) * row_count);
    if (rows == NULL) {
        fprintf(stderr, "malloc failed for rows\n");
        free(key);
        return -2;
    }
    if (fread(rows, sizeof(int), (size_t)row_count, file) != (size_t)row_count) {
        perror("fread rows failed");
        free(key);
        free(rows);
        return -2;
    }

    *key_out = key;
    *rows_out = rows;
    *row_count_out = row_count;
    return 0;
}