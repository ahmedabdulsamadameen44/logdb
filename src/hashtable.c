// Portions of this hash table implementation were developed with 
// assistance from Claude (Anthropic) — struct design, djb2 hash 
// function, and dynamic array growth logic.



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"



unsigned long hash(const char *key, int table_size){

    unsigned long hash_val = 5381 ;
    int c;
    while ((c = *key++))
    {
        hash_val = hash_val * 33 + c;
    }
    return hash_val % table_size;
}


HashTable *create_table(int size)
{
    HashTable *table = malloc(sizeof(HashTable));
    if (table == NULL)
    {
        fprintf(stderr, "malloc failed for table\n");
        exit(1);
    }



    table->buckets = malloc(sizeof(Entry *) * size);
    if (table->buckets == NULL)
    {
        fprintf(stderr, "malloc failed for buckets \n");
        exit(1);
    }
    for (int i = 0; i < size; i++)
    {
        table->buckets[i] = NULL;
    }
    


    table->size = size;
    table->count = 0;
    return table;

}


void insert(HashTable *table, const char *key, int row) {
    unsigned long idx = hash(key, table->size);
    Entry *curr = table->buckets[idx];

    // walk the chain looking for an existing entry with this key
    while (curr != NULL) {
        if (strcmp(key,curr->key) == 0) {
            if (curr->row_capacity == curr->row_count)
            {
               curr->row_capacity *= 2;
               curr->rows = realloc (curr->rows, sizeof(int) * curr->row_capacity);
            }
            curr->rows[curr->row_count] = row;
            curr->row_count++;
            ;
            return;
        }
        curr = curr->next;
    }

    // no existing entry found — create a new one, prepend to chain
    Entry *new_entry = malloc(sizeof(Entry));
    new_entry->key = strdup(key);
    new_entry->row_count = 1;
    new_entry->row_capacity = 4;
    new_entry->rows = malloc(sizeof(int) * new_entry->row_capacity);
    new_entry->rows[0] = row;
    new_entry->next = table->buckets[idx];
    table->buckets[idx] = new_entry;
    table->count++;
}