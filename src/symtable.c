#include "../include/symtable.h"

void init_var_table(var_table *var_table){
  var_table->vars = NULL;
  var_table->next_offset = -8;
  var_table->count = 0;
}

int add_var(var_table *var_table, String value) {
  var_table->vars = realloc(var_table->vars, (var_table->count + 1) * sizeof(var));

  var_table->vars[var_table->count].value = value;
  var_table->vars[var_table->count].offset = var_table->next_offset;

  int assigned_offset = var_table->next_offset;

  var_table->count++;
  var_table->next_offset -= 8;

  return assigned_offset;
}

int find_var(var_table *var_table, String value){
  for(int i = 0; i < var_table->count; i++){
    if(str_equals(&value, &var_table->vars[i].value)){
      return var_table->vars[i].offset;
    }
  }
  return 0;
}

int get_or_add_var(var_table *var_table, String value){
  int offset = find_var(var_table, value);
  if (offset != 0) {
    return offset;
  }
  return add_var(var_table, value);
}

void free_var_table(var_table *var_table){
  if (var_table->vars) {
    free(var_table->vars);
    var_table->vars = NULL;
  }
  var_table->count = 0;
}
