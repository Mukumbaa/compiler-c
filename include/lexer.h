#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "token.h"
#include "../../c-assets/libstr/standalone_lib/libstr.h"

typedef struct{
  Token *tokens;
  int token_number;
  Token_error error;
} Lexer;

Lexer lexer_gen(FILE *fd);
void add_token(Token *tokens, int token_number, Token_type token_type, String value, Position posiotion);
void print_lexer(Lexer lexer);
void lexer_destroy(Lexer *lexer);
#endif
