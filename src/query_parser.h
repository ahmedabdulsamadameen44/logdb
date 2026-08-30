// Portions of this  implementation were developed with 
// assistance from Claude (Anthropic)


#ifndef QUERY_PARSER_H
#define QUERY_PARSER_H

#include "row.h"   // for LOG_PATH_MAX

#define TOKEN_MAX LOG_PATH_MAX
#define MAX_TOKENS 64

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

// returns number of tokens written into out_tokens, or -1 on error
int tokenize(const char *query, Token out_tokens[MAX_TOKENS]);

#endif