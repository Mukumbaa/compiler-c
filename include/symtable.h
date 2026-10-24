#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "../../c-assets/libstr/standalone_lib/libstr.h"

typedef struct{
  String value;
  int offset;
}var;

typedef struct{
  var *vars;
  int next_offset;
  int count;
}var_table;

void init_var_table(var_table *var_table);
int add_var(var_table *var_table, String value);
int find_var(var_table *var_table, String value);
int get_or_add_var(var_table *var_table, String value);
void free_var_table(var_table *var_table);

#endif
