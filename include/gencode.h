#ifndef GENCODE_H
#define GENCODE_H

#include "ast.h"
#include "symtable.h"

typedef struct LoopContext {
  int loop_id;
  struct LoopContext *prev; // punta al ciclo più esterno se sono annidati
} LoopContext;

int gencode(FILE *fd, Program *program);
int gencode_node(FILE *fd, Node *node, var_table *table);

#endif
