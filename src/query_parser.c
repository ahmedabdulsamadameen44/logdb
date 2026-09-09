// AI assistance (Claude, Anthropic): Claude proposed the initial tokenizer
// skeleton/structure. I wrote the core scanning and classification logic
// myself. Claude reviewed and found 2 bugs: (1) infinite loop when the
// operator branch didn't advance the scan index, (2) uninitialized op[]
// buffer never null-terminated causing nondeterministic strcmp behavior.


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
            while (i < query_len && j < TOKEN_MAX - 1 && query[i] != ' ' && query[i] != '\t' && query[i] != '\n' && query[i] != '=' && query[i] != '<' && query[i] != '>') 
            {
                token_text[j++] = query[i++];
            }
            token_text[j] = '\0';

            if (strcmp(token_text, "SELECT") == 0 || strcmp(token_text, "WHERE") == 0 || strcmp(token_text, "AND") == 0 || strcmp(token_text, "GROUP") == 0 || strcmp(token_text, "BY") == 0 || strcmp(token_text, "COUNT") == 0  || strcmp(token_text, "DESC") == 0 || strcmp(token_text, "ASC") == 0) 
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


int parse_query(Token tokens[], int token_count, ParsedQuery *out_query) 
{
    if (token_count < 1 || tokens[0].type != TOKEN_KEYWORD || strcmp(tokens[0].text, "SELECT") != 0) 
    {
        return -1;
    }

    int i = 1;
    out_query->condition_count = 0;
    out_query->has_group_by = 0;
    out_query->sort_order = SORT_NONE;

    if (i < token_count && tokens[i].type == TOKEN_STAR) 
    {
        out_query->mode = MODE_SELECT_STAR;
        i++;
    } 
    else if (i < token_count && tokens[i].type == TOKEN_KEYWORD && strcmp(tokens[i].text, "COUNT") == 0) 
    {
        out_query->mode = MODE_SELECT_COUNT;
        i++;
        if (i < token_count && tokens[i].type == TOKEN_FIELD) 
        {
            strcpy(out_query->count_field, tokens[i].text);
            i++;
        } 
        else 
        {
            return -1;
        }
    } 
    else 
    {
        return -1;
    }

    if (i < token_count && tokens[i].type == TOKEN_KEYWORD && strcmp(tokens[i].text, "WHERE") == 0) 
    {
        i++;
        while (i + 3 <= token_count && out_query->condition_count < MAX_CONDITIONS) 
        {
            if (tokens[i].type != TOKEN_FIELD || tokens[i + 1].type != TOKEN_OP || tokens[i + 2].type != TOKEN_VALUE) 
            {
                return -1;
            }
            strcpy(out_query->conditions[out_query->condition_count].field, tokens[i].text);
            strcpy(out_query->conditions[out_query->condition_count].op, tokens[i + 1].text);
            strcpy(out_query->conditions[out_query->condition_count].value, tokens[i + 2].text);
            out_query->condition_count++;
            i += 3;

            if (i < token_count && tokens[i].type == TOKEN_KEYWORD && strcmp(tokens[i].text, "AND") == 0) 
            {
                i++;
            } 
            else 
            {
                break;
            }
        }
    }
    if (i < token_count && tokens[i].type == TOKEN_KEYWORD && strcmp(tokens[i].text, "GROUP") == 0) 
    {
        i++;
        if (!(i < token_count && tokens[i].type == TOKEN_KEYWORD && strcmp(tokens[i].text, "BY") == 0))
        {
            return -1;
        }
        i++;

        if (!(i < token_count && tokens[i].type == TOKEN_FIELD))
        {
            return -1;
        }
        strcpy(out_query->group_by_field, tokens[i].text);
        out_query->has_group_by = 1;
        i++;

        if (i < token_count && tokens[i].type == TOKEN_KEYWORD && (strcmp(tokens[i].text, "DESC") == 0 || strcmp(tokens[i].text, "ASC") == 0)) 
        {
            if (strcmp(tokens[i].text, "DESC") == 0) 
            {
                out_query->sort_order = SORT_DESC;
            } 
            else 
            {
                out_query->sort_order = SORT_ASC;
            }
            i++;
        }
    }
    if (i != token_count) 
    {
        return -1;
    }

    if (out_query->mode == MODE_SELECT_STAR && out_query->has_group_by) {
    return -1;
    }

    
    return 0;
}