#ifndef TOKEN_H
#define TOKEN_H

#include "../../c-assets/libstr/standalone_lib/libstr.h"


typedef enum{
  IDENTIFIER,
  INT,
  FLOAT,
  EQUALS,
  COMP_EQ,
  COMP_GR,
  COMP_LS,
  COMP_GREQ,
  COMP_LSEQ,
  COMP_NOT_EQ,
  PLUS,
  MINUS,
  TIMES,
  DIVISION,
  L_PAREN,
  R_PAREN,
  L_BRACE,
  R_BRACE,
  SEMICOLON,
  DOT,
  CARROT,
  ESCALMATION,
  TOKEN_PRINT,
  TOKEN_SCAN,
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_WHILE,
  TOKEN_BREAK,
  TOKEN_CONTINUE,
  LOGICAL_AND,
  LOGICAL_OR,
  TOKEN_STRING,
  COMMA,
  TOKEN_FN,
  TOKEN_RETURN,
  TOKEN_TIME,
  PERCENT,
  TOKEN_EOF
} Token_type;

typedef struct{
  int line;
  int col;
} Position;

typedef struct{
  Token_type type;
  String value;
  Position position;
} Token;

typedef struct {
  bool has_error; 
  Token token;
  Token next_token;
  char message[256];
} Token_error;

String token_type_to_string(Token_type type);

#endif
