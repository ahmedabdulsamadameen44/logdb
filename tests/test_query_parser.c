// this is fully by claude

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
void test_single_condition_where(void) {
    Token tokens[MAX_TOKENS];
    int token_count = tokenize("SELECT * WHERE status = 404", tokens);

    ParsedQuery pq;
    int result = parse_query(tokens, token_count, &pq);

    assert(result == 0);
    assert(pq.mode == MODE_SELECT_STAR);
    assert(pq.condition_count == 1);
    assert(strcmp(pq.conditions[0].field, "status") == 0);
    assert(strcmp(pq.conditions[0].op, "=") == 0);
    assert(strcmp(pq.conditions[0].value, "404") == 0);

    printf("test_single_condition_where passed\n");
}

void test_select_star_no_where(void) {
    Token tokens[MAX_TOKENS];
    int token_count = tokenize("SELECT *", tokens);

    ParsedQuery pq;
    int result = parse_query(tokens, token_count, &pq);

    assert(result == 0);
    assert(pq.mode == MODE_SELECT_STAR);
    assert(pq.condition_count == 0);

    printf("test_select_star_no_where passed\n");
}

void test_count_missing_field_fails(void) {
    Token tokens[MAX_TOKENS];
    int token_count = tokenize("SELECT COUNT WHERE status = 404", tokens);

    ParsedQuery pq;
    int result = parse_query(tokens, token_count, &pq);

    assert(result == -1);

    printf("test_count_missing_field_fails passed\n");
}
void test_count_with_field(void) {
    Token tokens[MAX_TOKENS];
    int token_count = tokenize("SELECT COUNT status WHERE status = 404", tokens);

    ParsedQuery pq;
    int result = parse_query(tokens, token_count, &pq);

    assert(result == 0);
    assert(pq.mode == MODE_SELECT_COUNT);
    assert(strcmp(pq.count_field, "status") == 0);
    assert(pq.condition_count == 1);
    assert(strcmp(pq.conditions[0].field, "status") == 0);
    assert(strcmp(pq.conditions[0].op, "=") == 0);
    assert(strcmp(pq.conditions[0].value, "404") == 0);

    printf("test_count_with_field passed\n");
}
void test_count_with_group_by(void) {
    Token tokens[MAX_TOKENS];
    int token_count = tokenize("SELECT COUNT status WHERE status = 404 GROUP BY host", tokens);

    ParsedQuery pq;
    int result = parse_query(tokens, token_count, &pq);

    assert(result == 0);
    assert(pq.mode == MODE_SELECT_COUNT);
    assert(strcmp(pq.count_field, "status") == 0);
    assert(pq.condition_count == 1);
    assert(strcmp(pq.conditions[0].field, "status") == 0);
    assert(strcmp(pq.conditions[0].op, "=") == 0);
    assert(strcmp(pq.conditions[0].value, "404") == 0);
    assert(pq.has_group_by == 1);
    assert(strcmp(pq.group_by_field, "host") == 0);

    printf("test_count_with_group_by passed\n");
}
void test_count_with_group_by_and_sort(void) {
    Token tokens[MAX_TOKENS];
    int token_count = tokenize("SELECT COUNT status WHERE status = 404 GROUP BY host DESC", tokens);

    ParsedQuery pq;
    int result = parse_query(tokens, token_count, &pq);

    assert(result == 0);
    assert(pq.mode == MODE_SELECT_COUNT);
    assert(strcmp(pq.count_field, "status") == 0);
    assert(pq.condition_count == 1);
    assert(strcmp(pq.conditions[0].field, "status") == 0);
    assert(strcmp(pq.conditions[0].op, "=") == 0);
    assert(strcmp(pq.conditions[0].value, "404") == 0);
    assert(pq.has_group_by == 1);
    assert(strcmp(pq.group_by_field, "host") == 0);
    assert(pq.sort_order == SORT_DESC);

    printf("test_count_with_group_by_and_sort passed\n");
}
void test_count_with_group_by_and_sort_asc(void) {
    Token tokens[MAX_TOKENS];
    int token_count = tokenize("SELECT COUNT status WHERE status = 404 GROUP BY host ASC", tokens);

    ParsedQuery pq;
    int result = parse_query(tokens, token_count, &pq);

    assert(result == 0);
    assert(pq.mode == MODE_SELECT_COUNT);
    assert(strcmp(pq.count_field, "status") == 0);
    assert(pq.condition_count == 1);
    assert(strcmp(pq.conditions[0].field, "status") == 0);
    assert(strcmp(pq.conditions[0].op, "=") == 0);
    assert(strcmp(pq.conditions[0].value, "404") == 0);
    assert(pq.has_group_by == 1);
    assert(strcmp(pq.group_by_field, "host") == 0);
    assert(pq.sort_order == SORT_ASC);

    printf("test_count_with_group_by_and_sort_asc passed\n");
}
void test_group_by_without_field_fails(void) {
    Token tokens[MAX_TOKENS];
    int token_count = tokenize("SELECT COUNT status WHERE status = 404 GROUP BY", tokens);

    ParsedQuery pq;
    int result = parse_query(tokens, token_count, &pq);

    assert(result == -1);

    printf("test_group_by_without_field_fails passed\n");
}
void test_group_by_missing_by_fails(void) {
    Token tokens[MAX_TOKENS];
    int token_count = tokenize("SELECT * WHERE status = 404 GROUP host", tokens);

    ParsedQuery pq;
    int result = parse_query(tokens, token_count, &pq);

    assert(result == -1);

    printf("test_group_by_missing_by_fails passed\n");
}

int main(void) {
    test_single_condition();
    test_and_chain_two_char_op();
    test_single_condition_where();
    test_select_star_no_where();
    test_count_missing_field_fails();
    test_count_with_field();
    test_count_with_group_by();
    test_count_with_group_by_and_sort();
    test_count_with_group_by_and_sort_asc();
    test_group_by_without_field_fails();
    test_group_by_missing_by_fails();
    printf("all tests passed\n");
    return 0;
}