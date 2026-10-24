#include "../include/ast.h"

Node *create_node(Node_type type, Token token){
  Node *node = malloc(sizeof(Node));

  if (!node){
    printf("[ERROR] Can't allocate memory for the node\n");
    return NULL;
  }

  node->type = type;
  node->token = token;
  node->left = NULL;
  node->right = NULL;
  return node;
}

void ast_destroy(Node *node){

  if (node == NULL){
    return;
  }
  ast_destroy(node->left);
  ast_destroy(node->right);
  free(node);
}

// helper to convert ast node enum to readable string
static const char *node_type_to_str(Node_type type) {
  switch (type) {
    case NODE_NUMBER:      return "NUMBER";
    case NODE_IDENTIFIER:  return "IDENTIFIER";
    case NODE_OP:          return "OP";
    case NODE_UNARY_OP:    return "UNARY_OP";
    case NODE_POSTFIX_OP:  return "POSTFIX_OP";
    case NODE_ASSIGN:      return "ASSIGN";
    case NODE_PRINT:       return "PRINT";
    case NODE_IF:          return "IF";             
    case NODE_SCAN:        return "SCAN";
    case NODE_IF_BRANCHES: return "IF_BRANCHES"; 
    case NODE_WHILE:       return "WHILE";
    case NODE_BREAK:       return "BREAK";
    case NODE_CONTINUE:    return "CONTINUE";  
    case NODE_BLOCK:       return "BLOCK";
    case NODE_LOGICAL_AND: return "AND";
    case NODE_LOGICAL_OR:  return "OR";
    case NODE_STRING:      return "STRING";
    case NODE_FN_DEF:      return "FN_DEF";
    case NODE_PARAM:       return "PARAM";
    case NODE_RETURN:      return "RETURN";
    case NODE_CALL:        return "CALL";
    case NODE_CALL_ARG:    return "CALL_ARG";
    case NODE_TIME:    return "TIME";
    default:               return "UNKNOWN";
  }
}

void print_ast_node(const Node *node, int depth, const char *side) {
  if (node == NULL) {
    return;
  }

  for (int i = 0; i < depth; i++) {
    printf("  │ ");
  }

  if (side != NULL) {
    printf("%s: ", side);
  }

  printf("[%s] '%s'\n", node_type_to_str(node->type), node->token.value.str);

  if (node->type == NODE_UNARY_OP || node->type == NODE_POSTFIX_OP) {
    print_ast_node(node->left, depth + 1, "arg");
  } else {
    print_ast_node(node->left, depth + 1, "L");
    print_ast_node(node->right, depth + 1, "R");
  }
}

void print_ast_program(const Program *program) {
  if (!program) return;

  printf("\nAST TREE\n");
  for (int i = 0; i < program->count; i++) {
    printf("\nStatement %d\n", i + 1);
    print_ast_node(program->statements[i], 0, NULL);
  }
}
