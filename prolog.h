#ifndef PROLOG_H_
#define PROLOG_H_

#include "mpc/mpc.h"

typedef struct find_tag_state_t {
  const mpc_ast_t *ast;
  int child;
} find_tag_state_t;

/**
 * Table stubs
 */
struct symbol_table_to_predicate_t;
struct symbol_table_node_t;
struct symbol_table_t;
struct predicate_table_to_symbol_t;
struct predicate_table_node_t;
struct predicate_table_t;

/**
 * Symbol Table
 */
typedef struct symbol_table_to_predicate_t {
  int position;
  struct predicate_table_to_symbol_t *predicate;
} symbol_table_to_predicate_t;

typedef struct symbol_table_node_t {
  const char *name;
  int num_link;
  int num_allocated;
  struct symbol_table_to_predicate_t **links;
} symbol_table_node_t;

typedef struct symbol_table_t {
  int num_symbols;
  int num_allocated;
  struct symbol_table_node_t **symbols;
} symbol_table_t;

/**
 * Predicate Table
 */
typedef struct predicate_table_to_symbol_t {
  int arity;
  struct symbol_table_node_t **nodes;
} predicate_table_to_symbol_t;

typedef struct predicate_table_node_t {
  const char *name;
  int num_link;
  int num_allocated;
  struct predicate_table_to_symbol_t **links;
} predicate_table_node_t;

typedef struct predicate_table_t {
  int num_predicates;
  int num_allocated;
  struct predicate_table_node_t **predicates;
} predicate_table_t;

/**
 * Functions
 */
void print_tags(const mpc_ast_t *, const int);
const mpc_ast_t *find_tag(const mpc_ast_t *ast, const char *tag);
void initialize_tag_state(find_tag_state_t *, const mpc_ast_t *);
const mpc_ast_t *find_tag_next(find_tag_state_t *, const char *);
void define_facts(const mpc_ast_t *);
int main(int, char **);

#endif
