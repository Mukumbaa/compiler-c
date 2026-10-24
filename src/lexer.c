#include "../include/lexer.h"
#include <ctype.h>

Lexer lexer_gen(FILE *fd){


  String line = str_init_with_cap(256);
  Token *tokens = NULL;
  // initialize error tracker to zero before scanning
  Token_error error = {0}; 

  int token_number = 0;
  int line_number = 0;
  bool in_multiline_comment = false;

  while(fgets(line.str, 256, fd)){
    line_number++;

    int i = 0;
    while(line.str[i] != '\n' && line.str[i] != '\0'){
      
      // keep scanning until closing delimiter if inside a block comment
      if (in_multiline_comment) {
        if (line.str[i] == '*' && line.str[i + 1] == '/') {
          in_multiline_comment = false;
          // advance past closing slash and asterisk
          i += 2; 
        } else {
          i++;
        }
        continue;
      }

      if (line.str[i] == ' ' || line.str[i] == '\t' || line.str[i] == '\r') {
          i++;
          continue;
      }

      Token_type type;
      String value;
      Position position;

      switch (line.str[i]) {
        case '=':
          if(line.str[i+1] == '='){
            type = COMP_EQ;
            value = str_init("==");
            position = (Position){.line = line_number, .col = i};
            i+=2;
            break;
          }
          type = EQUALS;
          value = str_init("=");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;
        case '<':
          if(line.str[i+1] == '='){
            type = COMP_LSEQ;
            value = str_init("<=");
            position = (Position){.line = line_number, .col = i};
            i+=2;
            break;
          }
          type = COMP_LS;
          value = str_init("<");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;
        case '>':
          if(line.str[i+1] == '='){
            type = COMP_GREQ;
            value = str_init(">=");
            position = (Position){.line = line_number, .col = i};
            i+=2;
            break;
          }
          type = COMP_GR;
          value = str_init(">");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;
        case '+':
          type = PLUS;
          value = str_init("+");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;
        case '-':
          type = MINUS;
          value = str_init("-");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;
        case '^':
          type = CARROT;
          value = str_init("^");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;
        case '!':
          if(line.str[i+1] == '='){
            type = COMP_NOT_EQ;
            value = str_init("!=");
            position = (Position){.line = line_number, .col = i};
            i+=2;
            break;
          }
          type = ESCALMATION;
          value = str_init("!");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;
        case '*':
          type = TIMES;
          value = str_init("*");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;
        case '/':
          // skip remainder of line when double slashes are encountered
          if (line.str[i + 1] == '/') {
            while (line.str[i] != '\n' && line.str[i] != '\0') {
              i++;
            }
            continue;
          }

          // enter block comment mode and advance past opening marker
          if (line.str[i + 1] == '*') {
            in_multiline_comment = true;
            i += 2; 
            continue;
          }
          type = DIVISION;
          value = str_init("/");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;

        case '%':
          type = PERCENT;
          value = str_init("%");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;
        case '(':
          type = L_PAREN;
          value = str_init("(");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;
        case ')':
          type = R_PAREN;
          value = str_init(")");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;
        case '{':
          type = L_BRACE;
          value = str_init("{");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;
        case '}':
          type = R_BRACE;
          value = str_init("}");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;
        case ';':
          type = SEMICOLON;
          value = str_init(";");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;
        case '.':
          // treat dot as floating point number if next char is a digit
          if (!isdigit((unsigned char)line.str[i + 1])) {
              type = DOT;
              value = str_init(".");
              position = (Position){.line = line_number, .col = i};
              i++;
              break;
          }
        case '&':
          if (line.str[i + 1] == '&') {
            type = LOGICAL_AND;
            value = str_init("&&");
            position = (Position){.line = line_number, .col = i};
            i += 2;
            break;
          } else {
            // reject isolated ampersand since bitwise operations are unsupported
            error.has_error = true;
            error.token = (Token){.type = TOKEN_EOF, .value = str_init("&"), .position = (Position){.line = line_number, .col = i}};
            error.next_token = error.token;
            snprintf(error.message, sizeof(error.message), "Invalid token '&'. Expected '&&'");

            str_destroy(&line);
            for (int k = 0; k < token_number; k++) str_destroy(&tokens[k].value);
            free(tokens);
            return (Lexer){.tokens = NULL, .token_number = -1, .error = error};
          }
        case '|':
          if (line.str[i + 1] == '|') {
            type = LOGICAL_OR;
            value = str_init("||");
            position = (Position){.line = line_number, .col = i};
            i += 2;
            break;
          } else {
            // reject isolated pipe since bitwise operations are unsupported
            error.has_error = true;
            error.token = (Token){.type = TOKEN_EOF, .value = str_init("|"), .position = (Position){.line = line_number, .col = i}};
            error.next_token = error.token;
            snprintf(error.message, sizeof(error.message), "Invalid token '|'. Expected '||'");

            str_destroy(&line);
            for (int k = 0; k < token_number; k++) str_destroy(&tokens[k].value);
            free(tokens);
            return (Lexer){.tokens = NULL, .token_number = -1, .error = error};
          }
        case ',':
          type = COMMA;
          value = str_init(",");
          position = (Position){.line = line_number, .col = i};
          i++;
          break;

        case '"': {
          value = str_empty();
          type = TOKEN_STRING;
          position = (Position){.line = line_number, .col = i};
          i++; 

          while (line.str[i] != '"' && line.str[i] != '\0' && line.str[i] != '\n') {
            if (line.str[i] == '\\' && line.str[i + 1] != '\0') {
              str_push_char(&value, line.str[i]);
              str_push_char(&value, line.str[i + 1]);
              i += 2;
            } else {
              str_push_char(&value, line.str[i]);
              i++;
            }
          }

          if (line.str[i] != '"') {
            // handle unterminated string literal when newline or eof is reached
            error.has_error = true;
            error.token = (Token){.type = TOKEN_STRING, .value = str_init("\""), .position = position};
            error.next_token = (Token){.type = TOKEN_STRING, .value = str_init("\""), .position = (Position){.line = line_number, .col = i}};
            snprintf(error.message, sizeof(error.message), "Unterminated string literal");

            str_destroy(&value);
            str_destroy(&line);
            for (int k = 0; k < token_number; k++) str_destroy(&tokens[k].value);
            free(tokens);
            return (Lexer){.tokens = NULL, .token_number = -1, .error = error};
          }
          i++;
          break;
        }
        default:
            
          if (isdigit((unsigned char)line.str[i]) || line.str[i] == '.') {
            value = str_empty();
            type = INT;
            position = (Position){.line = line_number, .col = i};
            bool has_dot = false;

            if (line.str[i] == '.') {
                type = FLOAT;
                has_dot = true;
                str_push_char(&value, '0');
                str_push_char(&value, '.');
                i++;
            }

            while (isdigit((unsigned char)line.str[i]) || (line.str[i] == '.' && !has_dot)) {
                if (line.str[i] == '.') {
                    type = FLOAT;
                    has_dot = true;
                }
                str_push_char(&value, line.str[i]);
                i++;
            }

            if (value.len > 0 && value.str[value.len - 1] == '.') {
                str_push_char(&value, '0');
            }
            if (isalpha((unsigned char)line.str[i]) || line.str[i] == '_') {
              while (isalnum((unsigned char)line.str[i]) || line.str[i] == '_') {
                str_push_char(&value, line.str[i]);
                i++;
              }

              // fail when an identifier begins with numeric digits
              error.has_error = true;
              error.token = (Token){.type = TOKEN_EOF, .value = str_init(value.str), .position = position};
              error.next_token = error.token;
              snprintf(error.message, sizeof(error.message), "Identifiers cannot start with a digit: '%s'", value.str);

              str_destroy(&value);
              str_destroy(&line);
              for (int k = 0; k < token_number; k++) str_destroy(&tokens[k].value);
              free(tokens);
              return (Lexer){.tokens = NULL, .token_number = -1, .error = error};
            }

          } else if (isalpha((unsigned char)line.str[i]) || line.str[i] == '_') {
            value = str_empty();
            position = (Position){.line = line_number, .col = i};

            while (isalnum((unsigned char)line.str[i]) || line.str[i] == '_') {
              str_push_char(&value, line.str[i]);
              i++;
            }

            if (str_equals_cstr(&value, "print")) {
              type = TOKEN_PRINT;
            } else if (str_equals_cstr(&value, "scan")) {
              type = TOKEN_SCAN;
            } else if (str_equals_cstr(&value, "if")) {
              type = TOKEN_IF;
            } else if (str_equals_cstr(&value, "else")) {
              type = TOKEN_ELSE;
            }else if (str_equals_cstr(&value, "while")){
              type = TOKEN_WHILE;
            }else if (str_equals_cstr(&value, "break")){
              type = TOKEN_BREAK;
            }else if (str_equals_cstr(&value, "continue")){
              type = TOKEN_CONTINUE;
            } else if (str_equals_cstr(&value, "fn")) {
              type = TOKEN_FN;
            } else if (str_equals_cstr(&value, "return")) {
              type = TOKEN_RETURN;
            } else if (str_equals_cstr(&value, "time")) {
              type = TOKEN_TIME;
            } else {
              type = IDENTIFIER;
            }

          }else{
            // halt on encountering unrecognized symbols
            char bad_char[2] = { line.str[i], '\0' };
            error.has_error = true;
            error.token = (Token){.type = TOKEN_EOF, .value = str_init(bad_char), .position = (Position){.line = line_number, .col = i}};
            error.next_token = error.token;
            snprintf(error.message, sizeof(error.message), "Invalid character '%c'", line.str[i]);

            str_destroy(&line);
            for (int k = 0; k < token_number; k++) str_destroy(&tokens[k].value);
            free(tokens);
            return (Lexer){.tokens = NULL, .token_number = -1, .error = error};
          }

          break;
      }

      token_number++;
      tokens = realloc(tokens, token_number * sizeof(Token));
      add_token(tokens, token_number, type, value, position);

    }

    str_reset(&line);
  }

  str_destroy(&line);

  token_number++;
  tokens = realloc(tokens, token_number * sizeof(Token));
  add_token(tokens, token_number, TOKEN_EOF, str_init("EOF"), (Position){.line = line_number, .col = 0});

  return (Lexer){.tokens = tokens, .token_number = token_number, .error = error};
}

void add_token(Token *tokens, int token_number, Token_type token_type, String value, Position posiotion){
  tokens[token_number - 1].type = token_type;
  tokens[token_number - 1].value = value;
  tokens[token_number - 1].position = posiotion;
}

void print_lexer(Lexer lexer){
  printf("\nTOKENS\n\n");
  for(int i = 0; i < lexer.token_number; i++){
    if(lexer.tokens[i].type == TOKEN_EOF){
      printf("\n%s(%s)", (token_type_to_string(lexer.tokens[i].type)).str, lexer.tokens[i].value.str);
    }else{
      printf("%s(%s) ", (token_type_to_string(lexer.tokens[i].type)).str, lexer.tokens[i].value.str);
    }
  }
  printf("\n");
}

void lexer_destroy(Lexer *lexer) {
    if (!lexer) return;
    
    // release allocated strings stored in error diagnostics
    if (lexer->error.has_error) {
      str_destroy(&lexer->error.token.value);
      if (lexer->error.next_token.value.str != lexer->error.token.value.str) {
        str_destroy(&lexer->error.next_token.value);
      }
    }

    if (lexer->tokens) {
      for (int i = 0; i < lexer->token_number; i++) {
          str_destroy(&lexer->tokens[i].value);
      }
      free(lexer->tokens);
      lexer->tokens = NULL;
    }
    lexer->token_number = 0;
}
