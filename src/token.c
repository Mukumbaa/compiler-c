#include "../include/token.h"


String token_type_to_string(Token_type type){
  String str;
  switch (type) {
    case IDENTIFIER:
      str = str_init("IDENTIFIER");
      break;
    case INT:
      str = str_init("INT");
      break;
    case FLOAT:
      str = str_init("FLOAT");
      break;
    case EQUALS:
      str = str_init("EQUALS");
      break;
    case COMP_EQ:
      str = str_init("COMP_EQ");
      break;
    case COMP_NOT_EQ:
      str = str_init("COMP_NOT_EQ");
      break;
    case COMP_GR:
      str = str_init("COMP_GR");
      break;
    case COMP_GREQ:
      str = str_init("COMP_GREQ");
      break;
    case COMP_LS:
      str = str_init("COMP_LS");
      break;
    case COMP_LSEQ:
      str = str_init("COMP_LSEQ");
      break;
    case PLUS:
      str = str_init("PLUS");
      break;
    case MINUS:
      str = str_init("MINUS");
      break;
    case CARROT:
      str = str_init("CARROT");
      break;
    case ESCALMATION:
      str = str_init("ESCALMATION");
      break;
    case TIMES:
      str = str_init("TIMES");
      break;
    case DIVISION:
      str = str_init("DIVISION");
      break;
    case L_PAREN:
      str = str_init("L_PAREN");
      break;
    case R_PAREN:
      str = str_init("R_PAREN");
      break;
    case L_BRACE:
      str = str_init("L_BRACE");
      break;
    case R_BRACE:
      str = str_init("R_BRACE");
      break;
    case SEMICOLON:
      str = str_init("SEMICOLON");
      break;
    case DOT:
      str = str_init("DOT");
      break;
    case TOKEN_PRINT:
      str = str_init("TOKEN_PRINT");
      break;
    case TOKEN_IF:
      str = str_init("TOKEN_IF");
      break;
    case TOKEN_SCAN:
      str = str_init("TOKEN_SCAN");
      break;
    case TOKEN_ELSE:
      str = str_init("TOKEN_ELSE");
      break;
    case TOKEN_WHILE:
      str = str_init("TOKEN_WHILE");
      break;
    case TOKEN_EOF:
      str = str_init("TOKEN_EOF");
      break;
    case TOKEN_BREAK:
      str = str_init("TOKEN_BREAK");
      break;
    case TOKEN_CONTINUE:
      str = str_init("TOKEN_CONTINUE");
      break;
    case LOGICAL_AND:
      str = str_init("LOGICAL_AND");
      break;
    case LOGICAL_OR:
      str = str_init("LOGICAL_OR");
      break;
    case TOKEN_STRING:
      str = str_init("TOKEN_STRING");
      break;
    case COMMA:
      str = str_init("COMMA");
      break;
    case TOKEN_FN:
      str = str_init("TOKEN_FN");
      break;
    case TOKEN_RETURN:
      str = str_init("TOKEN_RETURN");
      break;
    case TOKEN_TIME:
      str = str_init("TOKEN_TIME");
      break;
    case PERCENT:
      str = str_init("PERCENT");
      break;
    default:
      str = str_init("UNKNOWN");
      break;
  }

  return str;
}
