// Portions of this  implementation were developed with 
// assistance from Claude (Anthropic)


#ifndef QUERY_PARSER_H
#define QUERY_PARSER_H

#include "row.h"   // for LOG_PATH_MAX

#define TOKEN_MAX LOG_PATH_MAX
#define MAX_TOKENS 64
#define MAX_CONDITIONS 16
#define FIELD_NAME_MAX 8
#define VALUE_MAX LOG_PATH_MAX

typedef enum {
    SORT_NONE,
    SORT_ASC,
    SORT_DESC
} SortOrder;

typedef enum {
    TOKEN_KEYWORD,   // SELECT, WHERE, AND, GROUP, BY, COUNT
    TOKEN_FIELD,     // status, path, bytes, host
    TOKEN_OP,        // =, >, <, >=, <=
    TOKEN_VALUE,     // 404, /x, etc.
    TOKEN_STAR       // *
} TokenType;

typedef struct {
    TokenType type;
    char text[TOKEN_MAX];
} Token;

typedef enum 
{
    MODE_SELECT_STAR,
    MODE_SELECT_COUNT
}QueryMode;

typedef struct 
{
    char field [FIELD_NAME_MAX];
    char op[3];
    char value[VALUE_MAX];
}Condition;

typedef struct 
{
    SortOrder sort_order;
    QueryMode mode;
    Condition conditions [MAX_CONDITIONS];
    int condition_count;
    int has_group_by;
    char group_by_field [FIELD_NAME_MAX];
    char count_field [FIELD_NAME_MAX];
}ParsedQuery;



// returns number of tokens written into out_tokens, or -1 on error
int tokenize(const char *query, Token out_tokens[MAX_TOKENS]);
int parse_query(Token tokens[], int token_count ,ParsedQuery *out_query);

#endif