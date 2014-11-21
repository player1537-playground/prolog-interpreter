#ifndef PROLOG_H_
#define PROLOG_H_

#include "mpc/mpc.h"

#define MAX_PARAMS 10

typedef struct find_tag_state_t {
  const mpc_ast_t *ast;
  int child;
} find_tag_state_t;

void print_tags(const mpc_ast_t *, const int);
void initialize_tag_state(find_tag_state_t *, const mpc_ast_t *);
const mpc_ast_t *find_tag_next(find_tag_state_t *, const char *);
void define_facts(const mpc_ast_t *);
int main(int, char **);

#endif
