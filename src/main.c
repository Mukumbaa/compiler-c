#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#define LIBSTR_IMPLEMENTATION
#include "../../c-assets/libstr/standalone_lib/libstr.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/gencode.h"
#define LIBARGV_IMPLEMENTATION
#include "../../c-assets/libargv/standalone_lib/libargv.h"

bool g_debug = false;

static void print_diagnostic(const char *source_path, const Token_error *err) {
  if (source_path == NULL || err == NULL || !err->has_error) {
    return;
  }

  int target_line = err->token.position.line;
  int start_col   = err->token.position.col;
  if (target_line <= 0) target_line = 1;
  if (start_col < 0) start_col = 0;

  // compute ending column boundary for underline cursor
  int end_col = start_col + (err->token.value.len > 0 ? (int)err->token.value.len : 1);
  if (err->next_token.position.line == target_line && err->next_token.position.col >= start_col) {
    int next_end = err->next_token.position.col + (err->next_token.value.len > 0 ? (int)err->next_token.value.len : 1);
    if (next_end > end_col) {
      end_col = next_end;
    }
  }

  // print error banner with ansi red formatting
  fprintf(stderr, "\033[1;31merror\033[0m\033[1m: %s\033[0m\n", err->message);
  fprintf(stderr, "\033[1;34m  --> \033[0m%s:%d:%d\n", source_path, target_line, start_col + 1);
  fprintf(stderr, "\033[1;34m   |\033[0m\n");

  // reread input file to display original context line
  FILE *f = fopen(source_path, "r");
  if (f == NULL) {
    return;
  }

  char line_buf[1024];
  int current_line = 1;

  while (fgets(line_buf, sizeof(line_buf), f) != NULL) {
    if (current_line == target_line) {
      // strip line endings before printing
      size_t len = strlen(line_buf);
      while (len > 0 && (line_buf[len - 1] == '\n' || line_buf[len - 1] == '\r')) {
        line_buf[--len] = '\0';
      }

      // print target source line prefixed with line number
      fprintf(stderr, "\033[1;34m%2d | \033[0m%s\n", target_line, line_buf);
      fprintf(stderr, "\033[1;34m   | \033[1;31m");

      // pad with spaces or tabs up to the starting column
      for (int c = 0; c < start_col; c++) {
        if (c < (int)len && line_buf[c] == '\t') {
          fputc('\t', stderr);
        } else {
          fputc(' ', stderr);
        }
      }

      // draw caret underline marking the offending token range
      int span = end_col - start_col;
      if (span <= 1) {
        fprintf(stderr, "^\033[0m\n");
      } else {
        fputc('^', stderr);
        for (int c = 1; c < span - 1; c++) {
          fputc('~', stderr);
        }
        fprintf(stderr, "^\033[0m\n");
      }
      break;
    }
    current_line++;
  }

  fclose(f);
  fprintf(stderr, "\033[1;34m   |\033[0m\n\n");
}





int main(int argc, char **argv){

  // parse command line flags and positional parameters
  const char *source_file = NULL;
  const char *file_out = NULL;
  bool save_asm = false;
  bool print_tokens = false;
  bool print_ast = false;

  CliParser p;
  argv_init(&p, "compilerc");
  
  argv_bind_pos(&p, "source", &source_file, true, NULL, "Source file to compile.");
  argv_bind_str(&p, "-o", "--output", &file_out, false, NULL, "Output file name.");
  argv_bind_bool(&p, "-s", "--save-asm", &save_asm, "Save asm file.");
  argv_bind_bool(&p, "-l", "--print-lexer", &print_tokens, "Print tokens produced by the lexer.");
  argv_bind_bool(&p, "-a", "--print-ast", &print_ast, "Print the AST graph produced by the parser.");

  CliStatus status = argv_parse(&p, argc, argv);
  if (status == ARGV_HELP_REQUESTED) {
    argv_free(&p);
    return 0;
  }
  if (status != ARGV_OK) {
    fprintf(stderr, "Error in command line: %s\nUse '%s --help'.\n", argv_get_error(&p), argv[0]);
    argv_free(&p);
    return 1;
  }






  FILE *fd = fopen(source_file, "r");
  if (fd == NULL){
    printf("[ERROR] Not able to open %s file.\n", source_file);
    argv_free(&p);
    return -1;
  }

  // LEXER
  Lexer lexer = lexer_gen(fd);
  // close input file immediately after reading tokens
  fclose(fd); 

  if (lexer.error.has_error || (lexer.tokens == NULL && lexer.token_number == -1)){
    print_diagnostic(source_file, &lexer.error);
    lexer_destroy(&lexer);
    argv_free(&p);
    return -1;
  }else{
    if (print_tokens){
      print_lexer(lexer);
      printf("\n");
    }

    printf("[DONE LEXER]\n");
  }
  

  // PARSER
  Parser parser = {
    .tokens = lexer.tokens,
    .token_number = lexer.token_number,
    .current_token = 0,
    .error = (Token_error){0}
  };

  Program program = parse_program(&parser);
  if (program.count == -1 || program.error.has_error){
    print_diagnostic(source_file, &program.error);
    program_destroy(&program);
    lexer_destroy(&lexer);
    argv_free(&p);
    return -1;
  }else{
    if (print_ast){
      print_ast_program(&program);
      printf("\n");
    }
    printf("[DONE PARSER]\n");
  }

  
  // GENCODE
  String file_name_asm;
  String file_name_out;

  if (file_out != NULL){
    file_name_asm = str_init(file_out);
    str_append_fmt(&file_name_asm, ".s");
    file_name_out = str_init(file_out); 
  } else {
    String src_str = str_init(source_file);
    ptrdiff_t dot = str_find_last(&src_str, ".", CASE_SENSITIVE);
    if (dot > 0) {
      file_name_out = str_substr(&src_str, 0, (size_t)dot);
    } else {
      file_name_out = str_init(source_file);
    }
    str_destroy(&src_str);

    file_name_asm = str_init(file_name_out.str);
    str_append_fmt(&file_name_asm, ".s");
  }

  FILE *out = fopen(file_name_asm.str, "w");
  if (out == NULL){
    printf("[ERROR] Not able to open %s file.\n", file_name_asm.str);
    str_destroy(&file_name_asm);
    str_destroy(&file_name_out);
    program_destroy(&program);
    lexer_destroy(&lexer);
    argv_free(&p);
    return -1;
  }

  int res_1 = gencode(out, &program);
  fclose(out);

  if (res_1 == -1){
    if (program.error.has_error) {
      print_diagnostic(source_file, &program.error);
    }

    remove(file_name_asm.str);
    str_destroy(&file_name_asm);
    str_destroy(&file_name_out);
    program_destroy(&program);
    lexer_destroy(&lexer);
    argv_free(&p);
    return -1;
  }
  printf("[DONE GENCODE]\n");

  String gcc_comand = str_from_fmt("gcc %s -lm -o %s", file_name_asm.str, file_name_out.str);
  int res_2 = system(gcc_comand.str);
  if (res_2 == 0){
    printf("[DONE COMPILATION]\n");
  }

  if (!save_asm){
    remove(file_name_asm.str);
  }

  // CLEANUP
  str_destroy(&gcc_comand);
  str_destroy(&file_name_asm);
  str_destroy(&file_name_out);
  program_destroy(&program);
  lexer_destroy(&lexer);
  argv_free(&p);
  return 0;
}
