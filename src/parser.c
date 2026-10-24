#include "../include/parser.h"

// store first syntax error to avoid cascading diagnostics
static void parser_set_error(Parser *parser, Token tok, Token next_tok, const char *msg) {
  if (parser->error.has_error) return;
  parser->error.has_error = true;
  parser->error.token = tok;
  parser->error.next_token = next_tok;
  snprintf(parser->error.message, sizeof(parser->error.message), "%s", msg);
}

Token peek(Parser *parser){
  return parser->tokens[parser->current_token];
}

Token peek_next(Parser *parser){
  if (parser->current_token + 1 >= parser->token_number) {
    // return eof token once index is out of bounds
    return parser->tokens[parser->token_number - 1]; 
  }
  return parser->tokens[parser->current_token + 1];
}

Token advance(Parser *parser){
  Token current = parser->tokens[parser->current_token];
  if (parser->current_token < parser->token_number - 1) {
    parser->current_token++;
  }
  return current;
}

bool check(Parser *parser, Token_type type){
  return (peek(parser)).type == type;
}

bool match(Parser *parser, Token_type type){
  if((peek(parser)).type == type){
    advance(parser);
    return true;
  }
  return false;
}

bool expect(Parser *parser, Token_type type, const char *msg) {
  if (check(parser, type)) {
    advance(parser);
    return true;
  }

  Token curr = peek(parser);
  Token prev = (parser->current_token > 0) ? parser->tokens[parser->current_token - 1] : curr;
  
  parser_set_error(parser, prev, curr, msg);
  return false;
}


void program_destroy(Program *program) {
  if (!program || !program->statements) return;
  for (int i = 0; i < program->count; i++) {
    ast_destroy(program->statements[i]);
  }
  free(program->statements);
  program->statements = NULL;
  program->count = 0;
}


Program parse_program(Parser *parser) {
    Program program;
    program.statements = NULL;
    program.count = 0;
    program.error = (Token_error){0};

    while (!check(parser, TOKEN_EOF)) {
        Node *node = parse_statement(parser);
        if (node == NULL) {
          program.count = -1;
          program.error = parser->error;
          break;
        }

        program.count++;
        program.statements = realloc(program.statements, program.count * sizeof(Node*));
        program.statements[program.count - 1] = node;
    }

    return program;
}

Node *parse_block(Parser *parser) {
  String msg_lbrace = str_from_fmt("Syntax error, expected '{', (%d,%d)\n", peek(parser).position.line, peek(parser).position.col);
  if (!expect(parser, L_BRACE, msg_lbrace.str)) {
    str_destroy(&msg_lbrace);
    return NULL;
  }
  str_destroy(&msg_lbrace);

  Node *head = NULL;
  Node *current = NULL;

  while (!check(parser, R_BRACE) && !check(parser, TOKEN_EOF)) {
    Node *stmt = parse_statement(parser);
    if (stmt == NULL) {
      ast_destroy(head);
      return NULL;
    }

    Node *block_node = create_node(NODE_BLOCK, stmt->token);
    block_node->left = stmt;
    block_node->right = NULL;

    if (head == NULL) {
      head = block_node;
      current = head;
    } else {
      current->right = block_node;
      current = block_node;
    }
  }

  String msg_rbrace = str_from_fmt("Syntax error, expected '}', (%d,%d)\n", peek(parser).position.line, peek(parser).position.col);
  if (!expect(parser, R_BRACE, msg_rbrace.str)) {
    str_destroy(&msg_rbrace);
    ast_destroy(head);
    return NULL;
  }
  str_destroy(&msg_rbrace);

  if (head == NULL) {
    head = create_node(NODE_BLOCK, (Token){0});
  }

  return head;
}

Node *parse_statement(Parser *parser){
  Node *node = NULL; 
  
  if (check(parser, IDENTIFIER) && peek_next(parser).type == EQUALS){
    Node *left = create_node(NODE_IDENTIFIER, advance(parser));
    Token equal_token = advance(parser);
    Node *right = parse_expression(parser);
    if(right == NULL){
      ast_destroy(left);
      return NULL;
    }

    node = create_node(NODE_ASSIGN, equal_token);
    node->left = left;
    node->right = right;
  } else if(check(parser, TOKEN_PRINT)){
    node = create_node(NODE_PRINT, advance(parser));

    String msg_l = str_from_fmt("Syntax error, expected '(', (%d,%d)\n", peek(parser).position.line, peek(parser).position.col);
    if(!expect(parser, L_PAREN, msg_l.str)){
      str_destroy(&msg_l);
      ast_destroy(node);
      return NULL;
    }
    str_destroy(&msg_l);

    Node *curr = node;
    if (check(parser, TOKEN_STRING)) {
      curr->left = create_node(NODE_STRING, advance(parser));
    } else {
      curr->left = parse_expression(parser);
      if (curr->left == NULL) {
        ast_destroy(node);
        return NULL;
      }
    }

    while (check(parser, COMMA)) {
      advance(parser);

      Node *next_arg = create_node(NODE_PRINT, curr->token);
      if (check(parser, TOKEN_STRING)) {
        next_arg->left = create_node(NODE_STRING, advance(parser));
      } else {
        next_arg->left = parse_expression(parser);
        if (next_arg->left == NULL) {
          ast_destroy(next_arg);
          ast_destroy(node);
          return NULL;
        }
      }
      curr->right = next_arg;
      curr = next_arg;
    }

    String msg_r = str_from_fmt("Syntax error, expected ')', (%d,%d)\n", peek(parser).position.line, peek(parser).position.col);
    if(!expect(parser, R_PAREN, msg_r.str)){
      str_destroy(&msg_r);
      ast_destroy(node);
      return NULL;
    }
    str_destroy(&msg_r);
  }
  else if (check(parser, TOKEN_IF)) {
    Token if_token = advance(parser);
    node = create_node(NODE_IF, if_token);

    // expect opening parenthesis before condition
    String msg_lparen = str_from_fmt("Syntax error, expected '(', (%d,%d)\n", peek(parser).position.line, peek(parser).position.col);
    if (!expect(parser, L_PAREN, msg_lparen.str)) {
      str_destroy(&msg_lparen);
      ast_destroy(node);
      return NULL;
    }
    str_destroy(&msg_lparen);

    Node *cond = parse_expression(parser);
    if (cond == NULL) {
      ast_destroy(node);
      return NULL;
    }
    node->left = cond;

    String msg_rparen = str_from_fmt("Syntax error, expected ')', (%d,%d)\n", peek(parser).position.line, peek(parser).position.col);
    if (!expect(parser, R_PAREN, msg_rparen.str)) {
      str_destroy(&msg_rparen);
      ast_destroy(node);
      return NULL;
    }
    str_destroy(&msg_rparen);

    Node *branches = create_node(NODE_IF_BRANCHES, if_token);
    node->right = branches;

    Node *then_branch = parse_block(parser);
    if (then_branch == NULL) {
      ast_destroy(node);
      return NULL;
    }
    branches->left = then_branch;

    if (check(parser, TOKEN_ELSE)) {
      // consume else keyword before parsing branch body
      advance(parser); 

      Node *else_branch = parse_block(parser);
      if (else_branch == NULL) {
        ast_destroy(node);
        return NULL;
      }
      branches->right = else_branch;
    }

    return node;
  } else if (check(parser, TOKEN_SCAN)) {
    node = create_node(NODE_SCAN, advance(parser));

    String msg_lparen = str_from_fmt("Syntax error, expected '(', (%d,%d)\n", peek(parser).position.line, peek(parser).position.col);
    if (!expect(parser, L_PAREN, msg_lparen.str)) {
      str_destroy(&msg_lparen);
      ast_destroy(node);
      return NULL;
    }
    str_destroy(&msg_lparen);

    if (!check(parser, IDENTIFIER)) {
      Token curr = peek(parser);
      char err_msg[256];
      snprintf(err_msg, sizeof(err_msg), "Syntax error: expected variable name inside scan, found '%s'", curr.value.str);
      parser_set_error(parser, curr, curr, err_msg);
      ast_destroy(node);
      return NULL;
    }
    node->left = create_node(NODE_IDENTIFIER, advance(parser));

    String msg_rparen = str_from_fmt("Syntax error, expected ')', (%d,%d)\n", peek(parser).position.line, peek(parser).position.col);
    if (!expect(parser, R_PAREN, msg_rparen.str)) {
      str_destroy(&msg_rparen);
      ast_destroy(node);
      return NULL;
    }
    str_destroy(&msg_rparen);
  }else if (check(parser, TOKEN_WHILE)) {
    Token while_token = advance(parser);
    node = create_node(NODE_WHILE, while_token);

    String msg_lparen = str_from_fmt("Syntax error, expected '(', (%d,%d)\n", peek(parser).position.line, peek(parser).position.col);
    if (!expect(parser, L_PAREN, msg_lparen.str)) {
      str_destroy(&msg_lparen);
      ast_destroy(node);
      return NULL;
    }
    str_destroy(&msg_lparen);

    Node *cond = parse_expression(parser);
    if (cond == NULL) {
      ast_destroy(node);
      return NULL;
    }
    node->left = cond;

    String msg_rparen = str_from_fmt("Syntax error, expected ')', (%d,%d)\n", peek(parser).position.line, peek(parser).position.col);
    if (!expect(parser, R_PAREN, msg_rparen.str)) {
      str_destroy(&msg_rparen);
      ast_destroy(node);
      return NULL;
    }
    str_destroy(&msg_rparen);

    Node *body = parse_block(parser);
    if (body == NULL) {
      ast_destroy(node);
      return NULL;
    }
    node->right = body;

    return node;
  } else if (check(parser, TOKEN_BREAK)) {
    node = create_node(NODE_BREAK, advance(parser));
  } else if (check(parser, TOKEN_CONTINUE)) {
    node = create_node(NODE_CONTINUE, advance(parser));
  }
  else if (check(parser, TOKEN_FN)) {
    advance(parser);

    if (!check(parser, IDENTIFIER)) {
      Token curr = peek(parser);
      parser_set_error(parser, curr, curr, "Expected function name");
      return NULL;
    }
    Token fn_name = advance(parser);
    node = create_node(NODE_FN_DEF, fn_name);

    String msg_lp = str_from_fmt("Syntax error, expected '(' after function name, (%d,%d)\n", peek(parser).position.line, peek(parser).position.col);
    if (!expect(parser, L_PAREN, msg_lp.str)) {
      str_destroy(&msg_lp);
      ast_destroy(node);
      return NULL;
    }
    str_destroy(&msg_lp);

    Node *params_head = NULL;
    Node *params_curr = NULL;
    if (check(parser, IDENTIFIER)) {
      params_head = create_node(NODE_PARAM, advance(parser));
      params_curr = params_head;

      while (check(parser, COMMA)) {
        advance(parser);
      if (!check(parser, IDENTIFIER)) {
        Token curr = peek(parser);
        parser_set_error(parser, curr, curr, "Expected parameter name after ','");
        ast_destroy(params_head);
        ast_destroy(node);
        return NULL;
      }
        Node *next_p = create_node(NODE_PARAM, advance(parser));
        params_curr->right = next_p;
        params_curr = next_p;
      }
    }

    String msg_rp = str_from_fmt("Syntax error, expected ')' after parameters, (%d,%d)\n", peek(parser).position.line, peek(parser).position.col);
    if (!expect(parser, R_PAREN, msg_rp.str)) {
      str_destroy(&msg_rp);
      ast_destroy(params_head);
      ast_destroy(node);
      return NULL;
    }
    str_destroy(&msg_rp);

    node->left = params_head;

    Node *body = parse_block(parser);
    if (body == NULL) {
      ast_destroy(node);
      return NULL;
    }
    node->right = body;

    return node;
  }
  else if (check(parser, TOKEN_RETURN)) {
    Token ret_tok = advance(parser);
    node = create_node(NODE_RETURN, ret_tok);
    node->left = parse_expression(parser);
    if (node->left == NULL) {
      ast_destroy(node);
      return NULL;
    }
  }

  else {
    node = parse_expression(parser);
    if(node == NULL){
      return NULL;
    }
  }
  
  String msg = str_from_fmt("Syntax error, expected ';' at the end of statement, found '%s' at (%d,%d).\n", peek(parser).value.str, peek(parser).position.line, peek(parser).position.col);
  if (!expect(parser, SEMICOLON, msg.str)){
    str_destroy(&msg);
    ast_destroy(node);
    return NULL;
  }
  str_destroy(&msg);
  return node;
}


Node *parse_expression(Parser *parser) {
  Node *left = parse_logical_and(parser);
  if (left == NULL) {
    return NULL;
  }

  while (check(parser, LOGICAL_OR)) {
    Token op_token = advance(parser);
    Node *right = parse_logical_and(parser);
    if (right == NULL) {
      ast_destroy(left);
      return NULL;
    }

    Node *node = create_node(NODE_LOGICAL_OR, op_token);
    node->left = left;
    node->right = right;
    left = node;
  }

  return left;
}

Node *parse_logical_and(Parser *parser) {
  Node *left = parse_comparison(parser);
  if (left == NULL) {
    return NULL;
  }

  while (check(parser, LOGICAL_AND)) {
    Token op_token = advance(parser);
    Node *right = parse_comparison(parser);
    if (right == NULL) {
      ast_destroy(left);
      return NULL;
    }

    Node *node = create_node(NODE_LOGICAL_AND, op_token);
    node->left = left;
    node->right = right;
    left = node;
  }

  return left;
}

Node *parse_comparison(Parser *parser) {
  Node *left = parse_additive(parser);
  if (left == NULL) {
    return NULL;
  }

  if (check(parser, COMP_EQ) || check(parser, COMP_NOT_EQ) || 
      check(parser, COMP_GR) || check(parser, COMP_GREQ) || 
      check(parser, COMP_LS) || check(parser, COMP_LSEQ)) {
    Token op_token = advance(parser);
    Node *right = parse_additive(parser);
    if (right == NULL) {
      ast_destroy(left);
      return NULL;
    }

    Node *node = create_node(NODE_OP, op_token);
    node->left = left;
    node->right = right;
    left = node;
  }

  return left;
}

Node *parse_additive(Parser *parser){
  Node *left = parse_term(parser);
  if(left == NULL){
    return NULL;
  }

  while (check(parser, PLUS) || check(parser, MINUS)) {
    Token op_token = advance(parser);
    Node *right = parse_term(parser);
    if(right == NULL){
      ast_destroy(left);
      return NULL;
    }

    Node *node = create_node(NODE_OP, op_token);
    node->left = left;
    node->right = right;

    left = node;
  }

  return left;
}

Node *parse_term(Parser *parser){
  Node *left = parse_factor(parser);
  if(left == NULL){
    return NULL;
  }

  while (check(parser, TIMES) || check(parser, DIVISION) || check(parser, PERCENT)) {
    Token op_token = advance(parser);
    Node *right = parse_factor(parser);
    if(right == NULL){
      return NULL;
    }

    // construct binary operator node chaining factors
    Node *node = create_node(NODE_OP, op_token);
    node->left = left;
    node->right = right;

    left = node;
  }

  return left;

}

Node *parse_factor(Parser *parser){
  Node *node = NULL;

  if(check(parser, MINUS)){
    node = create_node(NODE_UNARY_OP, advance(parser));
    node->left = parse_power(parser);

  }else{
    return parse_power(parser);
  }
  return node;
}

Node *parse_power(Parser *parser){
  Node *left = parse_postfix(parser);
  if(left == NULL){
    return NULL;
  }

  if(check(parser, CARROT)){
    Token token_power = advance(parser);

    Node *right = parse_power(parser);
    if(right == NULL){
      ast_destroy(left);
      return NULL;
    }

    Node *node = create_node(NODE_OP, token_power);
    node->left = left;
    node->right = right;
    return node;
  }

  return left;
}

Node *parse_postfix(Parser *parser){
  Node *left = parse_primary(parser);
  if(left == NULL){
    return NULL;
  }

  while (check(parser, ESCALMATION)) {
    Token token_factorial = advance(parser);
    Node *node = create_node(NODE_POSTFIX_OP, token_factorial);
    node->left = left;
    left = node;
  }
  return left;
}

Node *parse_primary(Parser *parser){
  Node *node = NULL; 

  if(check(parser, INT) || check(parser, FLOAT)){
    node = create_node(NODE_NUMBER, advance(parser)); 
  } else if (check(parser, IDENTIFIER)) {
    if (peek_next(parser).type == L_PAREN) {
      Token fn_tok = advance(parser);
      advance(parser);
      node = create_node(NODE_CALL, fn_tok);

      Node *args_head = NULL;
      Node *args_curr = NULL;

      if (!check(parser, R_PAREN)) {
        Node *first_arg = parse_expression(parser);
        if (first_arg == NULL) {
          ast_destroy(node);
          return NULL;
        }
        args_head = create_node(NODE_CALL_ARG, (Token){0});
        args_head->left = first_arg;
        args_curr = args_head;

        while (check(parser, COMMA)) {
          advance(parser);
          Node *next_arg = parse_expression(parser);
          if (next_arg == NULL) {
            ast_destroy(args_head);
            ast_destroy(node);
            return NULL;
          }
          Node *arg_node = create_node(NODE_CALL_ARG, (Token){0});
          arg_node->left = next_arg;
          args_curr->right = arg_node;
          args_curr = arg_node;
        }
      }

      String msg_rp = str_from_fmt("Syntax error, expected ')' after function arguments, (%d,%d)\n", peek(parser).position.line, peek(parser).position.col);
      if (!expect(parser, R_PAREN, msg_rp.str)) {
        str_destroy(&msg_rp);
        ast_destroy(args_head);
        ast_destroy(node);
        return NULL;
      }
      str_destroy(&msg_rp);

      node->left = args_head;
    } else {
      node = create_node(NODE_IDENTIFIER, advance(parser));
    }
  }else if(check(parser, L_PAREN)){
    advance(parser);
    node = parse_expression(parser);

    String msg = str_from_fmt("Syntax error, unexpected token '%s' at (%d,%d).\n", peek(parser).value.str, peek(parser).position.line, peek(parser).position.col);
    if(!expect(parser, R_PAREN, msg.str)){
      str_destroy(&msg);
      ast_destroy(node);
      return NULL;
    }
    str_destroy(&msg);
  }
  
  else if (check(parser, TOKEN_TIME)) {
    // advance past time keyword and verify empty argument parens
    Token time_tok = advance(parser); 
    expect(parser, L_PAREN, "Expected '(' after 'time'");
    expect(parser, R_PAREN, "Expected ')' after 'time('");
    node = create_node(NODE_TIME, time_tok);
  }
  else {
    Token curr = peek(parser);
    char err_msg[256];
    snprintf(err_msg, sizeof(err_msg), "Syntax error: expected expression, found '%s'", curr.value.str);
    parser_set_error(parser, curr, curr, err_msg);
    return NULL;
  }

  return node;
}

