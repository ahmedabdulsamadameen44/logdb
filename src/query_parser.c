#include "query_parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


int tokenize(const char *query, Token out_tokens[MAX_TOKENS]) 
{
    int query_len = strlen(query);
    int token_count = 0;
    int i = 0;
    char op[3];
    while (i < query_len)
    {
        if (token_count >= MAX_TOKENS) return -1;

        if (query[i] == '=' || query[i] == '<' || query[i] == '>')
        {
            int j = 0;
            op[j++] = query[i];

            if (query[i + 1] == '=')
            {
                op[j++] = '=';
            }
            op[j] = '\0';

            if (strcmp(op, "=") == 0 || strcmp(op, ">") == 0 || strcmp(op, "<") == 0 || strcmp(op, ">=") == 0 || strcmp(op, "<=") == 0) 
            {
                out_tokens[token_count].type = TOKEN_OP;
                strcpy(out_tokens[token_count].text, op);
                token_count++;
            }
            i += j;
        }  
        else if (query[i] == ' ' || query[i] == '\t' || query[i] == '\n') 
        {
            i++;
            continue;
        } 
        else 
        {
            int j = 0;
            char token_text[TOKEN_MAX];
            while (i < query_len && query[i] != ' ' && query[i] != '\t' && query[i] != '\n' && query[i] != '=' && query[i] != '<' && query[i] != '>') 
            {
                token_text[j++] = query[i++];
            }
            token_text[j] = '\0';

            if (strcmp(token_text, "SELECT") == 0 || strcmp(token_text, "WHERE") == 0 || strcmp(token_text, "AND") == 0 || strcmp(token_text, "GROUP") == 0 || strcmp(token_text, "BY") == 0 || strcmp(token_text, "COUNT") == 0) 
            {
                out_tokens[token_count].type = TOKEN_KEYWORD;
            } 
            else if (strcmp(token_text, "status") == 0 || strcmp(token_text, "path") == 0 || strcmp(token_text, "bytes") == 0 || strcmp(token_text, "host") == 0) 
            {
                out_tokens[token_count].type = TOKEN_FIELD;
            } 
            else if (strcmp(token_text, "*") == 0) 
            {
                out_tokens[token_count].type = TOKEN_STAR;
            } 
            else 
            {
                out_tokens[token_count].type = TOKEN_VALUE;
            }

            strcpy(out_tokens[token_count].text, token_text);
            token_count++;
        }
    }
    return token_count;
}