#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"
#include "row.h"
#include "index.h"
#include "parser.h"
#include <string.h>
#include "query_parser.h"


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


static int row_matches(Row *row, ParsedQuery *pq) {
    for (int i = 0; i < pq->condition_count; i++) {
        Condition *cond = &pq->conditions[i];
        int cmp_result;

        if (strcmp(cond->field, "status") == 0) {
            int val = atoi(cond->value);
            cmp_result = row->status - val;
        }
        else if (strcmp(cond->field, "bytes") == 0) {
            int val = atoi(cond->value);
            cmp_result = row->bytes - val;
        }
        else if (strcmp(cond->field, "path") == 0) {
            if (strcmp(cond->op, "=") != 0) return 0;
            cmp_result = strcmp(row->path, cond->value);
        }
        else if (strcmp(cond->field, "host") == 0) {
            if (strcmp(cond->op, "=") != 0) return 0;
            cmp_result = strcmp(row->host, cond->value);
        }
        else {
            return 0;
        }

        if (strcmp(cond->op, "=") == 0 && cmp_result != 0) return 0;
        if (strcmp(cond->op, ">") == 0 && cmp_result <= 0) return 0;
        if (strcmp(cond->op, "<") == 0 && cmp_result >= 0) return 0;
        if (strcmp(cond->op, ">=") == 0 && cmp_result < 0) return 0;
        if (strcmp(cond->op, "<=") == 0 && cmp_result > 0) return 0;
    }
    return 1;
}


static void handle_match(Row *row, ParsedQuery *pq, int *count) {
    if (pq->mode == MODE_SELECT_STAR) {
        printf("%s %s %d %d\n", row->host, row->path, row->status, row->bytes);
    } else {
        (*count)++;
    }
    // GROUP BY aggregation hooks in here later: instead of a flat count,
    // this would increment a per-group-value counter instead.
}


int query(ParsedQuery *pq) {
    int status_cond_index = -1;
    for (int i = 0; i < pq->condition_count; i++) {
        if (strcmp(pq->conditions[i].field, "status") == 0) {
            status_cond_index = i;
            break;
        }
    }

    FILE *row_fp = fopen("rows.dat", "rb");
    if (row_fp == NULL) {
        perror("fopen rows.dat");
        return 1;
    }

    int count = 0;

    if (status_cond_index != -1) {
        FILE *index_fp = fopen("index.dat", "rb");
        if (index_fp == NULL) {
            perror("fopen index.dat");
            fclose(row_fp);
            return 1;
        }

        char *key;
        int *rows;
        int row_count;
        int found = 0;

        while (read_index_entry(&key, &rows, &row_count, index_fp) == 0) {
            if (!found && strcmp(key, pq->conditions[status_cond_index].value) == 0) {
                found = 1;
                for (int i = 0; i < row_count; i++) {
                    Row row;
                    fseek(row_fp, (long)rows[i] * sizeof(Row), SEEK_SET);
                    if (read_row(&row, row_fp) == 0) {
                        if (row_matches(&row, pq)) {
                            handle_match(&row, pq, &count);
                        }
                    }
                }
            }
            free(key);
            free(rows);
        }

        fclose(index_fp);
    }
    else {
        Row row;
        while (read_row(&row, row_fp) == 0) {
            if (row_matches(&row, pq)) {
                handle_match(&row, pq, &count);
            }
        }
    }

    fclose(row_fp);

    if (pq->mode == MODE_SELECT_COUNT) {
        printf("COUNT: %d\n", count);
    }

    return 0;
}