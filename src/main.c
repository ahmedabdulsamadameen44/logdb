#include "hashtable.h"
#include "command.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr,"not enough arguments\n");
        return 1;
    }
 
    

    if (strcmp(argv[1],"query") == 0)
    {
       int query_done = query(argv[2]);
       return query_done;
    }


    else if(strcmp(argv[1],"load") == 0 )
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
    //TODO load , query
}