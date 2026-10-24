#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "ast.h"

typedef struct {
  Token *tokens;
  int token_number;
  int current_token;
  Token_error error;
} Parser;


Token peek(Parser *parser);
Token peek_next(Parser *parser);
Token advance(Parser *parser);
bool check(Parser *parser, Token_type type);
bool match(Parser *parser, Token_type type);
bool expect(Parser *parser, Token_type type, const char *msg);
void program_destroy(Program *program);
Program parse_program(Parser *parser);
Node *parse_statement(Parser *parser);
Node *parse_block(Parser *parser);
Node *parse_additive(Parser *parser);
Node *parse_expression(Parser *parser);
Node *parse_term(Parser *parser);
Node *parse_factor(Parser *parser);
Node *parse_power(Parser *parser);
Node *parse_postfix(Parser *parser);
Node *parse_primary(Parser *parser);
Node *parse_logical_and(Parser *parser);
Node *parse_comparison(Parser *parser);
#endif
