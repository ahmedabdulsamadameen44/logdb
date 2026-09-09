#include <stdio.h>
#include <stdlib.h>


int write_index_entry(const char *key, int *rows, int row_count, FILE *file);
int read_index_entry(char **key_out, int **rows_out, int *row_count_out, FILE *file);