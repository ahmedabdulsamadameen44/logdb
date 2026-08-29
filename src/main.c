#include "hashtable.h"
#include <string.h>
#include "stdio.h"


int main (void)
{
    HashTable *table = create_table(2048);


    insert(table, "404", 6);
    insert(table, "307", 6);
    insert(table, "200", 60);

    printf("table size : %d  , table count : %d", table->size , table->count );



    unsigned long idx = hash("404", table->size);
    Entry *e = table->buckets[idx];
    while (e != NULL)
    {
        if (strcmp(e->key, "404") == 0)
        {
            printf("404 -> row_count=%d: ", e->row_count);
        
        for (int i = 0; i < e->row_count; i++) 
        {
            printf("%d ", e->rows[i]);
        }
        printf("\n");
    }
    e = e->next;
    }
    return 0 ;

}