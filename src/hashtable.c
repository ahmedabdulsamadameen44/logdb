// Portions of this hash table implementation were developed with 
// assistance from Claude (Anthropic) — struct design, djb2 hash 
// function, and dynamic array growth logic.
#include <string.h>
#include "hashtable.h"



struct  hashtable 
{
    int size;
    int count;
};


unsigned long hash(const char *key, int table_size){

    unsigned long hash_val = 5381 ;
    int c;
    while ((c = *key++))
    {
        hash_val = hash_val * 33 + c;
    }
    return hash_val % table_size;
}



