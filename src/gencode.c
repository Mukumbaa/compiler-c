#include "../include/gencode.h"

static LoopContext *g_current_loop = NULL;
// pointer to capture runtime or semantic errors in the program
static Token_error *g_error = NULL; 


int gencode(FILE *fd, Program *program){
  // bind program error structure so code generation can record issues
  g_error = &program->error;

  fprintf(fd, ".intel_syntax noprefix\n");
  fprintf(fd, ".globl main\n");
  fprintf(fd, ".extern printf\n");
  fprintf(fd, ".extern scanf\n");
  fprintf(fd, ".extern pow\n");
  fprintf(fd, ".extern time\n\n");
  fprintf(fd, ".extern fmod\n\n");

  fprintf(fd, ".section .data\n");
  fprintf(fd, "fmt_float:\n");
  fprintf(fd, ".string \"%%g\"\n");
  fprintf(fd, "fmt_str:\n");
  fprintf(fd, ".string \"%%s\"\n");
  fprintf(fd, "fmt_newline:\n");
  fprintf(fd, ".string \"\\n\"\n");
  fprintf(fd, "fmt_scan:\n");             
  fprintf(fd, ".string \"%%lf\"\n\n");

  fprintf(fd, ".section .text\n");

  // setup main stack frame and allocate local variable space
  fprintf(fd, "main:\n");
  fprintf(fd, "push rbp\n");
  fprintf(fd, "mov rbp, rsp\n");
  fprintf(fd, "sub rsp, 256\n\n");
  
  var_table main_table; 
  init_var_table(&main_table);
  
  for (int i = 0; i < program->count; i++){
    Node *stmt = program->statements[i];
    if (stmt->type == NODE_FN_DEF) continue;

    if (gencode_node(fd, stmt, &main_table) == -1) return -1;

    if (stmt->type != NODE_ASSIGN && 
        stmt->type != NODE_PRINT && 
        stmt->type != NODE_SCAN && 
        stmt->type != NODE_IF && 
        stmt->type != NODE_BLOCK && 
        stmt->type != NODE_WHILE && 
        stmt->type != NODE_BREAK && 
        stmt->type != NODE_CONTINUE &&
        stmt->type != NODE_RETURN) {
      fprintf(fd, "add rsp, 8\n");
    }
  }

  fprintf(fd, "\nmov rax, 0\n");
  fprintf(fd, "mov rsp, rbp\n");
  fprintf(fd, "pop rbp\n");
  fprintf(fd, "ret\n");

  free_var_table(&main_table);

  // emit subroutines after the main entry point exits
  for (int i = 0; i < program->count; i++){
    Node *stmt = program->statements[i];
    if (stmt->type == NODE_FN_DEF) {
      if (gencode_node(fd, stmt, NULL) == -1) {
        return -1;
      }
    }
  }

  return 0;
}

int gencode_node(FILE *fd, Node *node, var_table *table){
  if (!node) return 0;

  switch (node->type){
    case NODE_NUMBER: {
      static int num_label_count = 0;
      int id = num_label_count++;

      fprintf(fd, ".section .data\n");
      fprintf(fd, ".align 8\n");
      fprintf(fd, ".LC_num_%d:\n", id);
      fprintf(fd, ".double %s\n", node->token.value.str);
      fprintf(fd, ".section .text\n");
      fprintf(fd, "movsd xmm0, [rip + .LC_num_%d]\n", id);

      fprintf(fd, "sub rsp, 8\n");
      fprintf(fd, "movsd [rsp], xmm0\n");
      break;
    }

    case NODE_IDENTIFIER: {
      int offset = find_var(table, node->token.value);
      if (offset == 0) {
        // record diagnostic error when reading an uninitialized variable
        if (g_error != NULL) {
          g_error->has_error = true;
          g_error->token = node->token;
          g_error->next_token = node->token;
          snprintf(g_error->message, sizeof(g_error->message),
                   "Variable '%s' used before assignment", node->token.value.str);
        }
        return -1; 
      }
      fprintf(fd, "movsd xmm0, [rbp %d]\n", offset);
      fprintf(fd, "sub rsp, 8\n");
      fprintf(fd, "movsd [rsp], xmm0\n");
      break;
    }

    case NODE_ASSIGN:
      if (gencode_node(fd, node->right, table) == -1) return -1;
      fprintf(fd, "movsd xmm0, [rsp]\n");
      fprintf(fd, "add rsp, 8\n");
      fprintf(fd, "movsd [rbp %d], xmm0\n", get_or_add_var(table, node->left->token.value));
      break;

    case NODE_OP:
      if (gencode_node(fd, node->left, table) == -1) return -1;
      if (gencode_node(fd, node->right, table) == -1) return -1;

      fprintf(fd, "movsd xmm1, [rsp]\n");
      fprintf(fd, "add rsp, 8\n");

      fprintf(fd, "movsd xmm0, [rsp]\n");
      fprintf(fd, "add rsp, 8\n");

      switch (node->token.type){
        case PLUS:
          fprintf(fd, "addsd xmm0, xmm1\n");
          break;
        case MINUS:
          fprintf(fd, "subsd xmm0, xmm1\n");
          break;
        case TIMES:
          fprintf(fd, "mulsd xmm0, xmm1\n");
          break;
        case DIVISION:
          fprintf(fd, "divsd xmm0, xmm1\n");
          break;
        case CARROT:
          fprintf(fd, "call pow\n");
          break; 
        case PERCENT:
          fprintf(fd, "call fmod\n");
          break;

        case COMP_EQ:
        case COMP_NOT_EQ:
        case COMP_LS:
        case COMP_LSEQ:
        case COMP_GR:
        case COMP_GREQ:
          fprintf(fd, "ucomisd xmm0, xmm1\n");
          if (node->token.type == COMP_EQ) {
            fprintf(fd, "sete al\n");
          } else if (node->token.type == COMP_NOT_EQ) {
            fprintf(fd, "setne al\n");
          } else if (node->token.type == COMP_LS) {
            fprintf(fd, "setb al\n");
          } else if (node->token.type == COMP_LSEQ) {
            fprintf(fd, "setbe al\n");
          } else if (node->token.type == COMP_GR) {
            fprintf(fd, "seta al\n");
          } else if (node->token.type == COMP_GREQ) {
            fprintf(fd, "setae al\n");
          }
          fprintf(fd, "movzx rax, al\n");
          fprintf(fd, "cvtsi2sd xmm0, rax\n");
          break;

        default:
          break;
      }

      fprintf(fd, "sub rsp, 8\n");
      fprintf(fd, "movsd [rsp], xmm0\n");
      break;

    case NODE_UNARY_OP:
      if (gencode_node(fd, node->left, table) == -1) return -1;
      fprintf(fd, "movsd xmm0, [rsp]\n");
      fprintf(fd, "add rsp, 8\n");

      fprintf(fd, "xorpd xmm1, xmm1\n"); 
      fprintf(fd, "subsd xmm1, xmm0\n");
      fprintf(fd, "movsd xmm0, xmm1\n");

      fprintf(fd, "sub rsp, 8\n");
      fprintf(fd, "movsd [rsp], xmm0\n");
      break;

    case NODE_POSTFIX_OP: {
      static int fact_label_count = 0;
      int id = fact_label_count++;

      if (gencode_node(fd, node->left, table) == -1) return -1;
      fprintf(fd, "movsd xmm0, [rsp]\n");
      fprintf(fd, "add rsp, 8\n");

      fprintf(fd, "cvttsd2si rcx, xmm0\n");
      fprintf(fd, "mov rax, 1\n");

      fprintf(fd, ".L_fact_loop_%d:\n", id);
      fprintf(fd, "cmp rcx, 1\n");
      fprintf(fd, "jle .L_fact_end_%d\n", id);
      fprintf(fd, "imul rax, rcx\n");
      fprintf(fd, "sub rcx, 1\n");
      fprintf(fd, "jmp .L_fact_loop_%d\n", id);

      fprintf(fd, ".L_fact_end_%d:\n", id);
      fprintf(fd, "cvtsi2sd xmm0, rax\n");

      fprintf(fd, "sub rsp, 8\n");
      fprintf(fd, "movsd [rsp], xmm0\n");
      break;
    }

    case NODE_BLOCK:
      if (node->left) {
        if (gencode_node(fd, node->left, table) == -1) return -1;
        if (node->left->type != NODE_ASSIGN && 
            node->left->type != NODE_PRINT && 
            node->left->type != NODE_SCAN && 
            node->left->type != NODE_IF && 
            node->left->type != NODE_BLOCK && 
            node->left->type != NODE_WHILE && 
            node->left->type != NODE_BREAK && 
            node->left->type != NODE_CONTINUE && 
            node->left->type != NODE_RETURN) {
          fprintf(fd, "add rsp, 8\n");
        }
      }
      if (node->right) {
        if (gencode_node(fd, node->right, table) == -1) return -1;
      }
      break;

    case NODE_IF: {
      static int if_label_count = 0;
      int id = if_label_count++;

      if (gencode_node(fd, node->left, table) == -1) return -1;

      fprintf(fd, "movsd xmm0, [rsp]\n");
      fprintf(fd, "add rsp, 8\n");
      fprintf(fd, "xorpd xmm1, xmm1\n");
      fprintf(fd, "ucomisd xmm0, xmm1\n");

      Node *branches = node->right;
      bool has_else = (branches && branches->right != NULL);

      if (has_else) {
        fprintf(fd, "je .L_else_%d\n", id);
      } else {
        fprintf(fd, "je .L_end_if_%d\n", id);
      }

      if (branches && branches->left) {
        if (gencode_node(fd, branches->left, table) == -1) return -1;
      }

      if (has_else) {
        fprintf(fd, "jmp .L_end_if_%d\n", id);
        fprintf(fd, ".L_else_%d:\n", id);
        if (gencode_node(fd, branches->right, table) == -1) return -1;
      }

      fprintf(fd, ".L_end_if_%d:\n", id);
      break;
    }

    case NODE_IF_BRANCHES:
      break;

    case NODE_SCAN: {
      static int scan_prompt_count = 0;
      int id = scan_prompt_count++;

      fprintf(fd, ".section .data\n");
      fprintf(fd, ".LC_scan_prompt_%d:\n", id);
      fprintf(fd, ".string \"scan %s = \"\n", node->left->token.value.str);
      fprintf(fd, ".section .text\n");

      fprintf(fd, "lea rdi, [rip + .LC_scan_prompt_%d]\n", id);
      fprintf(fd, "xor eax, eax\n");
      fprintf(fd, "call printf\n");

      fprintf(fd, "lea rsi, [rbp %d]\n", get_or_add_var(table, node->left->token.value));
      fprintf(fd, "lea rdi, [rip + fmt_scan]\n");
      fprintf(fd, "xor eax, eax\n");
      fprintf(fd, "call scanf\n");
      break;
    }

    case NODE_WHILE: {
      static int while_label_count = 0;
      int id = while_label_count++;

      LoopContext ctx;
      ctx.loop_id = id;
      ctx.prev = g_current_loop;
      g_current_loop = &ctx;

      fprintf(fd, ".L_while_start_%d:\n", id);

      if (gencode_node(fd, node->left, table) == -1) return -1;

      fprintf(fd, "movsd xmm0, [rsp]\n");
      fprintf(fd, "add rsp, 8\n");
      fprintf(fd, "xorpd xmm1, xmm1\n");
      fprintf(fd, "ucomisd xmm0, xmm1\n");

      fprintf(fd, "je .L_while_end_%d\n", id);

      if (node->right) {
        if (gencode_node(fd, node->right, table) == -1) return -1;
      }

      fprintf(fd, "jmp .L_while_start_%d\n", id);
      fprintf(fd, ".L_while_end_%d:\n", id);

      g_current_loop = ctx.prev;
      break;
    }

    case NODE_BREAK:
      if (g_current_loop == NULL) {
        if (g_error != NULL) {
          g_error->has_error = true;
          g_error->token = node->token;
          g_error->next_token = node->token;
          snprintf(g_error->message, sizeof(g_error->message), "'break' used outside of a loop");
        }
        return -1;
      }
      fprintf(fd, "jmp .L_while_end_%d\n", g_current_loop->loop_id);
      break;

    case NODE_CONTINUE:
      if (g_current_loop == NULL) {
        if (g_error != NULL) {
          g_error->has_error = true;
          g_error->token = node->token;
          g_error->next_token = node->token;
          snprintf(g_error->message, sizeof(g_error->message), "'continue' used outside of a loop");
        }
        return -1;
      }
      fprintf(fd, "jmp .L_while_start_%d\n", g_current_loop->loop_id);
      break;

    case NODE_LOGICAL_AND: {
      static int and_label_count = 0;
      int id = and_label_count++;

      if (gencode_node(fd, node->left, table) == -1) return -1;

      fprintf(fd, "movsd xmm0, [rsp]\n");
      fprintf(fd, "add rsp, 8\n");
      fprintf(fd, "xorpd xmm1, xmm1\n");
      fprintf(fd, "ucomisd xmm0, xmm1\n");
      fprintf(fd, "je .L_and_false_%d\n", id);

      if (gencode_node(fd, node->right, table) == -1) return -1;

      fprintf(fd, "movsd xmm0, [rsp]\n");
      fprintf(fd, "add rsp, 8\n");
      fprintf(fd, "xorpd xmm1, xmm1\n");
      fprintf(fd, "ucomisd xmm0, xmm1\n");
      fprintf(fd, "je .L_and_false_%d\n", id);

      fprintf(fd, "mov rax, 1\n");
      fprintf(fd, "cvtsi2sd xmm0, rax\n");
      fprintf(fd, "jmp .L_and_end_%d\n", id);

      fprintf(fd, ".L_and_false_%d:\n", id);
      fprintf(fd, "xorpd xmm0, xmm0\n");

      fprintf(fd, ".L_and_end_%d:\n", id);
      fprintf(fd, "sub rsp, 8\n");
      fprintf(fd, "movsd [rsp], xmm0\n");
      break;
    }

    case NODE_LOGICAL_OR: {
      static int or_label_count = 0;
      int id = or_label_count++;

      if (gencode_node(fd, node->left, table) == -1) return -1;

      fprintf(fd, "movsd xmm0, [rsp]\n");
      fprintf(fd, "add rsp, 8\n");
      fprintf(fd, "xorpd xmm1, xmm1\n");
      fprintf(fd, "ucomisd xmm0, xmm1\n");
      fprintf(fd, "jne .L_or_true_%d\n", id);

      if (gencode_node(fd, node->right, table) == -1) return -1;

      fprintf(fd, "movsd xmm0, [rsp]\n");
      fprintf(fd, "add rsp, 8\n");
      fprintf(fd, "xorpd xmm1, xmm1\n");
      fprintf(fd, "ucomisd xmm0, xmm1\n");
      fprintf(fd, "jne .L_or_true_%d\n", id);

      fprintf(fd, "xorpd xmm0, xmm0\n");
      fprintf(fd, "jmp .L_or_end_%d\n", id);

      fprintf(fd, ".L_or_true_%d:\n", id);
      fprintf(fd, "mov rax, 1\n");
      fprintf(fd, "cvtsi2sd xmm0, rax\n");

      fprintf(fd, ".L_or_end_%d:\n", id);
      fprintf(fd, "sub rsp, 8\n");
      fprintf(fd, "movsd [rsp], xmm0\n");
      break;
    }

    case NODE_STRING: {
      static int str_label_count = 0;
      int id = str_label_count++;

      fprintf(fd, ".section .data\n");
      fprintf(fd, ".LC_str_%d:\n", id);
      fprintf(fd, ".string \"%s\"\n", node->token.value.str);
      fprintf(fd, ".section .text\n");

      fprintf(fd, "lea rsi, [rip + .LC_str_%d]\n", id);
      fprintf(fd, "lea rdi, [rip + fmt_str]\n");
      fprintf(fd, "xor eax, eax\n");
      fprintf(fd, "call printf\n");
      break;
    }

    case NODE_PRINT: {
      if (node->left->type == NODE_STRING) {
        if (gencode_node(fd, node->left, table) == -1) return -1;
      } else {
        if (gencode_node(fd, node->left, table) == -1) return -1;
        fprintf(fd, "movsd xmm0, [rsp]\n");
        fprintf(fd, "add rsp, 8\n");

        fprintf(fd, "lea rdi, [rip + fmt_float]\n");
        fprintf(fd, "mov eax, 1\n"); 
        fprintf(fd, "call printf\n");
      }

      if (node->right) {
        if (gencode_node(fd, node->right, table) == -1) return -1;
      } 
      break;
    }

    case NODE_FN_DEF: {
      var_table fn_table;
      init_var_table(&fn_table);

      fprintf(fd, "\n.globl %s\n", node->token.value.str);
      fprintf(fd, "%s:\n", node->token.value.str);
      fprintf(fd, "push rbp\n");
      fprintf(fd, "mov rbp, rsp\n");
      fprintf(fd, "sub rsp, 256\n");

      Node *param = node->left;
      int param_idx = 0;
      while (param != NULL) {
        int offset = get_or_add_var(&fn_table, param->token.value);
        if (param_idx < 8) {
          fprintf(fd, "movsd [rbp %d], xmm%d\n", offset, param_idx);
        }
        param_idx++;
        param = param->right;
      }

      if (node->right) {
        if (gencode_node(fd, node->right, &fn_table) == -1) return -1;
      }

      fprintf(fd, "mov rsp, rbp\n");
      fprintf(fd, "pop rbp\n");
      fprintf(fd, "ret\n");

      if (fn_table.vars) free(fn_table.vars);
      break;
    }

    case NODE_RETURN: {
      if (gencode_node(fd, node->left, table) == -1) return -1;

      fprintf(fd, "movsd xmm0, [rsp]\n");
      fprintf(fd, "add rsp, 8\n");

      fprintf(fd, "mov rsp, rbp\n");
      fprintf(fd, "pop rbp\n");
      fprintf(fd, "ret\n");
      break;
    }

    case NODE_CALL: {
      Node *arg = node->left;
      int arg_count = 0;
      while (arg != NULL) {
        if (gencode_node(fd, arg->left, table) == -1) return -1;
        arg_count++;
        arg = arg->right;
      }

      for (int i = arg_count - 1; i >= 0; i--) {
        if (i < 8) {
          fprintf(fd, "movsd xmm%d, [rsp]\n", i);
          fprintf(fd, "add rsp, 8\n");
        } else {
          fprintf(fd, "add rsp, 8\n");
        }
      }

      fprintf(fd, "call %s\n", node->token.value.str);

      fprintf(fd, "sub rsp, 8\n");
      fprintf(fd, "movsd [rsp], xmm0\n");
      break;
    }

    case NODE_TIME:
      fprintf(fd, "rdtsc\n");
      fprintf(fd, "shl rdx, 32\n");
      fprintf(fd, "or rax, rdx\n");
      fprintf(fd, "cvtsi2sd xmm0, rax\n");
      fprintf(fd, "sub rsp, 8\n");
      fprintf(fd, "movsd [rsp], xmm0\n");
      break;

    default:
      break;
  }
  return 0;
}

