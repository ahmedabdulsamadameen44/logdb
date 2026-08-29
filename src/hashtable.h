   // Portions of this hash table implementation were developed with 
   // assistance from Claude (Anthropic) — struct design, djb2 hash 
   // function, and dynamic array growth logic.
#ifndef HASHTABLE_H
#define HASHTABLE_H





typedef struct Entry
{
    char *key;
    int *rows;
    int row_count;
    int row_capacity;
    struct Entry *next;
    
}Entry;




typedef struct 
{
    int size;
    int count;
    Entry **buckets;
}HashTable;



unsigned long hash(const char *key, int table_size);
HashTable *create_table (int size);
void insert(HashTable *table, const char *key, int row);
Entry *lookup( HashTable *table, const char *key);

#endif