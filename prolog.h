#ifndef PROLOG_H_
#define PROLOG_H_

#include "mpc/mpc.h"

/*****************************************
 * Struct stubs
 *****************************************/
struct find_tag_state_t;
struct symbol_table_to_predicate_t;
struct symbol_table_node_t;
struct symbol_table_t;
struct predicate_table_to_symbol_t;
struct predicate_table_node_t;
struct predicate_table_t;

/*****************************************
 * AST Parsing
 *****************************************/
typedef struct find_tag_state_t {
  const mpc_ast_t *ast;
  int child;
} find_tag_state_t;

int parse_file(mpc_result_t *, const char *);
void print_tags(const mpc_ast_t *, const int);
const mpc_ast_t *find_tag(const mpc_ast_t *, const char *);
void initialize_tag_state(find_tag_state_t *, const mpc_ast_t *);
const mpc_ast_t *find_tag_next(find_tag_state_t *, const char *);

/*****************************************
 * Symbol Table
 *****************************************/
typedef struct symbol_table_to_predicate_t {
  int position;
  struct predicate_table_node_t *predicate;
  struct predicate_table_to_symbol_t *link;
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

/*****************************************
 * Predicate Table
 *****************************************/

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

/*****************************************
 * Symbol Table Functions
 *****************************************/
void initialize_symbol_table(symbol_table_t *);
void initialize_symbol_table_node(symbol_table_node_t *, const char *);
void initialize_symbol_table_to_predicate(symbol_table_to_predicate_t *,
					  int,
					  predicate_table_node_t *,
					  predicate_table_to_symbol_t *);
symbol_table_node_t *symbol_table_add(symbol_table_t *, const char *);
void symbol_table_link_add(symbol_table_node_t *,
			   int,
			   predicate_table_node_t *,
			   predicate_table_to_symbol_t *);
void symbol_table_enlarge(symbol_table_t *);
void symbol_table_node_enlarge(symbol_table_node_t *);
symbol_table_node_t *symbol_table_find(symbol_table_t *, const char *);

/*****************************************
 * Predicate Table Functions
 *****************************************/
void initialize_predicate_table(predicate_table_t *);
void initialize_predicate_table_node(predicate_table_node_t *, const char *);
void initialize_predicate_table_to_symbol(predicate_table_to_symbol_t *, int );
predicate_table_node_t *predicate_table_add(predicate_table_t *, const char *);
predicate_table_to_symbol_t *predicate_table_link_add(predicate_table_node_t *,
						      int,
						      symbol_table_node_t **);
void predicate_table_enlarge(predicate_table_t *);
void predicate_table_node_enlarge(predicate_table_node_t *);

/*****************************************
 * Rule Functions
 *****************************************/
void define_facts(const mpc_ast_t *, symbol_table_t *, predicate_table_t *);
void rule_add(symbol_table_t *,
	      predicate_table_t *,
	      const char *,
	      const int,
	      const char **);
void print_symbols(symbol_table_t *);
void print_predicates(predicate_table_t *);
void print_rules(symbol_table_t *, predicate_table_t *);
/*****************************************
 * Solve Functions
 *****************************************/
typedef struct solve_goal_state_t {
  struct solve_goal_t *goal;
  int subgoal_index;
  struct predicate_table_to_symbol_t *candidate;
  int candidate_index;
} solve_goal_state_t;

typedef struct solve_t {
  int num_goals;
  struct solve_goal_t **goals;
  struct solve_goal_state_t **states;
} solve_t;

typedef struct solve_goal_t {
  struct predicate_table_node_t *predicate;
  int num_subgoals;
  struct solve_subgoal_t **subgoals;
} solve_goal_t;

typedef struct solve_subgoal_t {
  int pos;
  struct solve_condition_t *condition;
} solve_subgoal_t;

typedef enum { CONSTANT, VARIABLE } solve_condition_type_t;

typedef struct solve_condition_t {
  const char *name;
  enum solve_condition_type_t type;
  struct symbol_table_node_t *symbol;
} solve_condition_t;


/*****************************************
 * Main Function
 *****************************************/
int main(int, char **);

#endif
