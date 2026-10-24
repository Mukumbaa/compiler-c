#ifndef AST_H
#define AST_H

#include "token.h"


typedef enum {
  NODE_OP,
  NODE_ASSIGN,
  NODE_UNARY_OP,
  NODE_POSTFIX_OP,
  NODE_NUMBER,
  NODE_IDENTIFIER,
  NODE_PRINT,
  NODE_IF,
  NODE_SCAN,
  NODE_IF_BRANCHES,
  NODE_BLOCK,
  NODE_WHILE,
  NODE_BREAK,
  NODE_CONTINUE,
  NODE_LOGICAL_AND,
  NODE_LOGICAL_OR,
  NODE_STRING, 
  NODE_FN_DEF,
  NODE_PARAM,
  NODE_RETURN,
  NODE_CALL,
  NODE_CALL_ARG,
  NODE_TIME,
} Node_type;

typedef struct Node {
  Node_type type;
  Token token;
  struct Node *left;
  struct Node *right;
} Node;

typedef struct {
  Node **statements;
  int count;
  Token_error error;
} Program;

Node *create_node(Node_type type, Token token);
void ast_destroy(Node *node);
void print_ast_node(const Node *node, int depth, const char *side);
void print_ast_program(const Program *program);

#endif
