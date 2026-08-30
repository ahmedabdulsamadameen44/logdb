// Portions of this  implementation were developed with 
// assistance from Claude (Anthropic)


#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"
#include "row.h"
#include "index.h"
#include "parser.h"
#include <string.h>





int load(const char *csv_path) {
    FILE *csv_fp = fopen(csv_path, "r");
    if (csv_fp == NULL) {
        perror("fopen csv");
        return 1;
    }

    FILE *row_fp = fopen("rows.dat", "wb");
    if (row_fp == NULL) {
        perror("fopen rows.dat");
        fclose(csv_fp);
        return 1;
    }

    HashTable *table = create_table(2048);
    if (table == NULL) {
        perror("create_table");
        fclose(row_fp);
        fclose(csv_fp);
        return 1;
    }

    char line[512];
    int row_count = 0;

    while (fgets(line, sizeof(line), csv_fp) != NULL) {
        Row row;
        if (parse_line(line, &row) != 0) {
            continue;
        }

        if (write_row(&row, row_fp) != 0) {
            fclose(csv_fp);
            fclose(row_fp);
            exit(-2);
        }

        char status_key[16];
        sprintf(status_key, "%d", row.status);
        insert(table, status_key, row_count);

        row_count++;
    }

    fclose(csv_fp);
    fclose(row_fp);

    FILE *index_fp = fopen("index.dat", "wb");
    if (index_fp == NULL) {
        perror("fopen index.dat");
        return 1;
    }

    for (int i = 0; i < table->size; i++) {
        Entry *curr = table->buckets[i];
        while (curr != NULL) {
            int check_write = write_index_entry(curr->key, curr->rows, curr->row_count, index_fp);
            curr = curr->next;
            if (check_write == 1) {
                fclose(index_fp);
                return 1;
            }
        }
    }

    fclose(index_fp);
    return 0;
}



int query(const char *query_str) {
    FILE *index_fp = fopen("index.dat", "rb");
    if (index_fp == NULL) {
        perror("fopen index.dat");
        return 1;
    }

    char *key;
    int *rows;
    int row_count;

    while (read_index_entry(&key, &rows, &row_count, index_fp) == 0) {
        if (strcmp(key, query_str) == 0) {
            printf("Found %d rows for status %s:\n", row_count, key);
            for (int i = 0; i < row_count; i++) {
                printf("Row %d\n", rows[i]);
            }
            free(key);
            free(rows);
            fclose(index_fp);
            return 0;
        }
        free(key);
        free(rows);
    }

    fclose(index_fp);
    printf("No rows found for status %s\n", query_str);
    return 0;
}