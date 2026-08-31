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
#include <errno.h>
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



int query(const char *query_str) {
    Token tokens[MAX_TOKENS];
    int token_count = tokenize(query_str, tokens);
    if (token_count < 0) {
        fprintf(stderr, "Error tokenizing query\n");
        return 1;
    }

    ParsedQuery parsed_query;
    int parse_result = parse_query(tokens, token_count, &parsed_query);
    if (parse_result != 0) {
        fprintf(stderr, "Error parsing query\n");
        return 1;
    }

    // For demonstration purposes, we will just print the parsed query
    printf("Parsed Query:\n");
    printf("Mode: %s\n", parsed_query.mode == MODE_SELECT_STAR ? "SELECT *" : "SELECT COUNT");
    if (parsed_query.mode == MODE_SELECT_COUNT) {
        printf("Count Field: %s\n", parsed_query.count_field);
    }
    printf("Conditions (%d):\n", parsed_query.condition_count);
    for (int i = 0; i < parsed_query.condition_count; i++) {
        printf("  %s %s %s\n", parsed_query.conditions[i].field, parsed_query.conditions[i].op, parsed_query.conditions[i].value);
    }
    if (parsed_query.has_group_by) {
        printf("Group By: %s\n", parsed_query.group_by_field);
        printf("Sort Order: %s\n", parsed_query.sort_order == SORT_ASC ? "ASC" : (parsed_query.sort_order == SORT_DESC ? "DESC" : "NONE"));
    }

    return 0;
}