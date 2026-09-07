// AI assistance (Claude, Anthropic): I wrote main()'s argument dispatch
// myself. Claude caught two wiring bugs during review: a missing load
// branch that had been accidentally deleted, and a missing
// #include "query_parser.h" in command.h that was needed for the
// ParsedQuery type to resolve correctly.



#include "hashtable.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "command.h"
#include "query_parser.h" 

int main (int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr,"not enough arguments\n");
        return 1;
    }
 
    
    if (strcmp(argv[1], "query") == 0)
    {
        Token tokens[MAX_TOKENS];
        int token_count = tokenize(argv[2], tokens);
        if (token_count < 0) {
            fprintf(stderr, "Error tokenizing query\n");
            return 1;
        }

        ParsedQuery pq;
        if (parse_query(tokens, token_count, &pq) != 0) {
            fprintf(stderr, "Error parsing query\n");
            return 1;
        }

        int query_done = query(&pq);
        return query_done;
    }

    else if (strcmp(argv[1], "load") == 0)
    {
        int load_done = load(argv[2]);
        return load_done;
    }
    
    else
    {
        fprintf(stderr,"not a command");
        return 1;
    }
    

    return 0;
}