// AI assistance (Claude, Anthropic): I designed and wrote row_matches(),
// update_groups(), sort_groups(), and the load()/query() pipeline myself.
// Claude helped shape the dispatch structure in query() — deciding to
// branch on whether a status condition exists (index-narrow path) vs.
// falling back to a full linear scan (no status condition) — and reviewed
// row_matches()'s condition-checking loop for correctness.



#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"
#include "row.h"
#include "index.h"
#include "parser.h"
#include <string.h>
#include "query_parser.h"



#define MAX_GROUPS 64




typedef struct 
{
    char value[VALUE_MAX];
    int count;
} GroupCount;







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
    int is_first_line = 1;

    while (fgets(line, sizeof(line), csv_fp) != NULL) {
        if (is_first_line) {
            is_first_line = 0;
            continue;
        }

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










static void update_groups(Row *row, ParsedQuery *pq, GroupCount groups[], int *group_count)
{
    if (!pq->has_group_by) {
        return;
    }

    char value[VALUE_MAX];

    if (strcmp(pq->group_by_field, "status") == 0) {
        sprintf(value, "%d", row->status);
    }
    else if (strcmp(pq->group_by_field, "bytes") == 0) {
        sprintf(value, "%d", row->bytes);
    }
    else if (strcmp(pq->group_by_field, "path") == 0) {
        strcpy(value, row->path);
    }
    else if (strcmp(pq->group_by_field, "host") == 0) {
        strcpy(value, row->host);
    }
    else {
        return;  // unknown field, shouldn't happen if parser validated it
    }

    for (int i = 0; i < *group_count; i++) {
        if (strcmp(groups[i].value, value) == 0) {
            groups[i].count++;
            return;
        }
    }

    if (*group_count < MAX_GROUPS) {
        strcpy(groups[*group_count].value, value);
        groups[*group_count].count = 1;
        (*group_count)++;
    }
}















static void handle_match(Row *row, ParsedQuery *pq, int *count) {
    if (pq->mode == MODE_SELECT_STAR) {
        printf("%s\x1F%s\x1F%d\x1F%d\n", row->host, row->path, row->status, row->bytes);
    } 
    
    else {
        (*count)++;
    }
}








static void sort_groups(GroupCount groups[], int group_count, SortOrder sort_order) {
    if (sort_order == SORT_NONE) {
        return;
    }

    for (int i = 0; i < group_count - 1; i++) {
        int target_idx = i;
        for (int j = i + 1; j < group_count; j++) {
            if (sort_order == SORT_DESC && groups[j].count > groups[target_idx].count) {
                target_idx = j;
            }
            else if (sort_order == SORT_ASC && groups[j].count < groups[target_idx].count) {
                target_idx = j;
            }
        }
        if (target_idx != i) {
            GroupCount temp = groups[i];
            groups[i] = groups[target_idx];
            groups[target_idx] = temp;
        }
    }
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
    GroupCount groups[MAX_GROUPS];
    int group_count = 0;

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
                            if (pq->has_group_by)
                            {
                            update_groups(&row, pq, groups, &group_count);
                            }
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
                if (pq->has_group_by) {
                    update_groups(&row, pq, groups, &group_count);
                }
            }
        }
    }

    fclose(row_fp);

    if (pq->mode == MODE_SELECT_COUNT) {
        printf("COUNT\x1F%d\n", count);   
    }


    if (pq->has_group_by) 
    {
        sort_groups(groups, group_count, pq->sort_order);
        for (int i = 0; i < group_count; i++) 
        {
            printf("GROUP\x1F%s\x1F%d\n", groups[i].value, groups[i].count);
        }
    }

    return 0;
}