// Portions of this implementation were developed with
// assistance from Claude (Anthropic)

#include "../src/query_parser.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

void test_single_condition(void) {
    Token tokens[MAX_TOKENS];
    int count = tokenize("SELECT * WHERE status = 404", tokens);

    assert(count == 6);
    assert(tokens[0].type == TOKEN_KEYWORD);
    assert(strcmp(tokens[0].text, "SELECT") == 0);
    assert(tokens[1].type == TOKEN_STAR);
    assert(strcmp(tokens[1].text, "*") == 0);
    assert(tokens[2].type == TOKEN_KEYWORD);
    assert(strcmp(tokens[2].text, "WHERE") == 0);
    assert(tokens[3].type == TOKEN_FIELD);
    assert(strcmp(tokens[3].text, "status") == 0);
    assert(tokens[4].type == TOKEN_OP);
    assert(strcmp(tokens[4].text, "=") == 0);
    assert(tokens[5].type == TOKEN_VALUE);
    assert(strcmp(tokens[5].text, "404") == 0);

    printf("test_single_condition passed\n");
}

void test_and_chain_two_char_op(void) {
    Token tokens[MAX_TOKENS];
    int count = tokenize("SELECT * WHERE bytes >= 1000 AND status = 404", tokens);

    assert(count == 10);
    assert(tokens[4].type == TOKEN_OP);
    assert(strcmp(tokens[4].text, ">=") == 0);
    assert(tokens[6].type == TOKEN_KEYWORD);
    assert(strcmp(tokens[6].text, "AND") == 0);

    printf("test_and_chain_two_char_op passed\n");
}

int main(void) {
    test_single_condition();
    test_and_chain_two_char_op();
    printf("all tests passed\n");
    return 0;
}