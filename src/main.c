#include "hashtable.h"
#include <string.h>
#include <stdio.h>

int main(void)
{
    HashTable *table = create_table(2048);

    insert(table, "404", 6);
    insert(table, "404", 99);
    insert(table, "307", 6);
    insert(table, "200", 60);

    printf("table size : %d  , table count : %d\n", table->size, table->count);

    Entry *found = lookup(table, "404");
    if (found != NULL) {
        printf("404 -> row_count=%d: ", found->row_count);
        for (int i = 0; i < found->row_count; i++) {
            printf("%d ", found->rows[i]);
        }
        printf("\n");
    } else {
        printf("404 not found\n");
    }

    return 0;
}