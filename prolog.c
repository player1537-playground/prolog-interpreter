#include "prolog.h"
#include "mpc/mpc.h"
#include <assert.h>

#define NEW(type, num) ((type*)malloc(sizeof(type) * (num)))
#define RENEW(orig, type, num) ((type*)realloc((orig), sizeof(type) * (num)))
#define MAX_PARAMS 10
#define ENLARGE_FACTOR 2

/*****************************************
 * AST Functions
 *****************************************/
int parse_file(mpc_result_t *r, const char *filename) {
  mpc_parser_t	*Constant  = mpc_new("constant");
  mpc_parser_t	*Variable  = mpc_new("variable");
  mpc_parser_t	*Ident	   = mpc_new("ident");
  mpc_parser_t	*Params	   = mpc_new("params");
  mpc_parser_t	*Predicate = mpc_new("predicate");
  mpc_parser_t  *Union     = mpc_new("union");
  mpc_parser_t	*Fact	   = mpc_new("fact");
  mpc_parser_t  *Query     = mpc_new("query");
  mpc_parser_t	*Lang	   = mpc_new("lang");

  int return_value;

  mpca_lang(MPCA_LANG_DEFAULT,
	    " constant  : /[a-z0-9_]+/;                            "
	    " variable  : /[A-Z][a-z0-9_]*/;                       "
	    " ident     : <constant> | <variable>;                 "
	    " params    : <ident> (',' <ident>)*;                  "
	    " predicate : <ident> '(' <params> ')';                "
	    " union     : <predicate> (',' <predicate>)*;          "
	    " fact      : <union> '.';                             "
	    " query     : \"?-\" <union> '.';                      "
	    " lang      : /^/ (<fact> | <query>)+ /$/;             ",
	    Constant, Variable, Ident, Params, Predicate, Union, Fact, Query,
	    Lang, NULL);

  printf("Constant:  "); mpc_print(Constant);
  printf("Variable:  "); mpc_print(Variable);
  printf("Ident:     "); mpc_print(Ident);
  printf("Params:    "); mpc_print(Params);
  printf("Predicate: "); mpc_print(Predicate);
  printf("Union:     "); mpc_print(Union);
  printf("Fact:      "); mpc_print(Fact);
  printf("Query:     "); mpc_print(Query);
  printf("Lang:      "); mpc_print(Lang);

  if (filename != NULL) {
    return_value = mpc_parse_contents(filename, Lang, r);
  } else {
    return_value = mpc_parse_pipe("<stdin>", stdin, Lang, r);
  }

  if (!return_value) {
    mpc_err_print(r->error);
    mpc_err_delete(r->error);

    goto cleanup;
  }

 cleanup:
  mpc_cleanup(9,
	      Constant, Variable, Ident, Params, Predicate,
	      Union,    Fact,     Query, Lang
	      );

  return return_value;
}

void print_tags(const mpc_ast_t *ast, const int depth) {
  int i;

  for (i=0; i<depth; ++i) {
    printf("  ");
  }
  printf("%s: '%s'\n", ast->tag, ast->contents);

  for (i=0; i<ast->children_num; ++i) {
    print_tags(ast->children[i], depth + 1);
  }
}

void initialize_tag_state(find_tag_state_t *state, const mpc_ast_t *ast) {
  state->ast = ast;
  state->child = 0;
}

const mpc_ast_t *find_tag(const mpc_ast_t *ast, const char *tag) {
  find_tag_state_t state;

  initialize_tag_state(&state, ast);
  return find_tag_next(&state, tag);
}

int has_tag(const mpc_ast_t *ast, const char *tag) {
  return strstr(ast->tag, tag) != NULL;
}

const mpc_ast_t *find_tag_next(find_tag_state_t *state, const char *tag) {
  int i,
      j,
      children_num;
  const mpc_ast_t *child_ast,
                  *parent = state->ast,
                  *return_value;

  if (parent == NULL) {
    return NULL;
  }

  children_num = parent->children_num;

  for (i=state->child; i<children_num; ++i) {
    child_ast = parent->children[i];
    if (has_tag(child_ast, tag)) {
      state->child = i + 1;
      return child_ast;
    }

    for (j=0; j<child_ast->children_num; ++j) {
      state->ast = child_ast;
      state->child = 0;

      return_value = find_tag_next(state, tag);
      if (return_value != NULL) {
	return return_value;
      }
    }

    state->ast = parent;
  }

  state->ast = NULL;
  return NULL;
}

/*****************************************
 * Symbol Table Functions
 *****************************************/

/* ==== Symbol Table ==== */
void initialize_symbol_table(symbol_table_t *table) {
  table->num_symbols = 0;
  table->num_allocated = 1;
  table->symbols = NEW(symbol_table_node_t *, table->num_allocated);
}

void symbol_table_add(symbol_table_t *table, symbol_table_node_t *node) {
  if (table->num_symbols >= table->num_allocated) {
    symbol_table_enlarge(table);
  }

  table->symbols[table->num_symbols++] = node;
}

void symbol_table_enlarge(symbol_table_t *table) {
  table->num_allocated *= ENLARGE_FACTOR;
  table->symbols = RENEW(table->symbols,
			 symbol_table_node_t *,
			 table->num_allocated);
}

void destroy_symbol_table(symbol_table_t *table) {
  int i;

  for (i=0; i<table->num_symbols; ++i) {
    destroy_symbol_table_node(table->symbols[i]);
    free(table->symbols[i]);
  }

  table->num_symbols = table->num_allocated = 0;
  free(table->symbols);
  table->symbols = NULL;
}

symbol_table_node_t *symbol_table_find(symbol_table_t *table,
				       const char *name) {
  int i;
  symbol_table_node_t *node;

  for (i=0; i<table->num_symbols; ++i) {
    node = table->symbols[i];
    if (strcmp(name, node->name) == 0) {
      return node;
    }
  }

  return NULL;
}

symbol_table_node_t *symbol_table_find_or_add(symbol_table_t *table,
					      const char *name) {
  symbol_table_node_t *node = symbol_table_find(table, name);

  if (node == NULL) {
    node = NEW(symbol_table_node_t, 1);
    initialize_symbol_table_node(node, name);
    symbol_table_add(table, node);
  }

  return node;
}

/* ==== Symbol Table Node ==== */
void initialize_symbol_table_node(symbol_table_node_t *node, const char *name) {
  node->num_link = 0;
  node->num_allocated = 1;
  node->links = NEW(symbol_table_to_predicate_t *, node->num_allocated);
  node->name = name;
}

void symbol_table_node_enlarge(symbol_table_node_t *node) {
  node->num_allocated *= ENLARGE_FACTOR;
  node->links = RENEW(node->links,
		      symbol_table_to_predicate_t *,
		      node->num_allocated);
}

void destroy_symbol_table_node(symbol_table_node_t *node) {
  int i;

  for (i=0; i<node->num_link; ++i) {
    destroy_symbol_table_to_predicate(node->links[i]);
    free(node->links[i]);
  }

  free(node->links);
  node->num_link = node->num_allocated = 0;
  node->name = NULL;
}

void symbol_table_node_add(symbol_table_node_t *node,
			   int pos,
			   predicate_table_node_t *predicate,
			   predicate_table_to_symbol_t *ref) {
  symbol_table_to_predicate_t *link = NEW(symbol_table_to_predicate_t, 1);

  initialize_symbol_table_to_predicate(link, pos, predicate, ref);

  if (node->num_link >= node->num_allocated) {
    symbol_table_node_enlarge(node);
  }

  node->links[node->num_link++] = link;
}

/* ==== Symbol Table to Predicate ==== */
void initialize_symbol_table_to_predicate(symbol_table_to_predicate_t *link,
					  int pos,
					  predicate_table_node_t *predicate,
					  predicate_table_to_symbol_t *ref) {
  link->position = pos;
  link->predicate = predicate;
  link->link = ref;
}

void destroy_symbol_table_to_predicate(symbol_table_to_predicate_t *link) {
  link->position = 0;
  link->predicate = NULL;
  link->link = NULL;
}

/*****************************************
 * Predicate Table Functions
 *****************************************/

/* ==== Predicate Table ==== */
void initialize_predicate_table(predicate_table_t *table) {
  table->num_predicates = 0;
  table->num_allocated = 1;
  table->predicates = NEW(predicate_table_node_t *, table->num_allocated);
}

predicate_table_node_t *predicate_table_add(predicate_table_t *table,
					    predicate_table_node_t *node) {
  if (table->num_predicates >= table->num_allocated) {
    predicate_table_enlarge(table);
  }

  table->predicates[table->num_predicates++] = node;
  return node;
}

void predicate_table_enlarge(predicate_table_t *table) {
  table->num_allocated *= ENLARGE_FACTOR;
  table->predicates = RENEW(table->predicates,
			    predicate_table_node_t *,
			    table->num_allocated);
}

void destroy_predicate_table(predicate_table_t *table) {
  int i;

  for (i=0; i<table->num_predicates; ++i) {
    destroy_predicate_table_node(table->predicates[i]);
    free(table->predicates[i]);
  }

  table->num_allocated = table->num_predicates = 0;
  free(table->predicates);
  table->predicates = NULL;
}

predicate_table_node_t *predicate_table_find(predicate_table_t *table,
					     const char *name) {
  int i;
  predicate_table_node_t *node;

  for (i=0; i<table->num_predicates; ++i) {
    node = table->predicates[i];
    if (strcmp(name, node->name) == 0) {
      return node;
    }
  }

  return NULL;
}

predicate_table_node_t *predicate_table_find_or_add(predicate_table_t *table,
						    const char *name) {
  predicate_table_node_t *node = predicate_table_find(table, name);

  if (node == NULL) {
    node = NEW(predicate_table_node_t, 1);
    assert(node != NULL);
    initialize_predicate_table_node(node, name);
    predicate_table_add(table, node);
  }

  return node;
}

/* ==== Predicate Table Node ==== */
void initialize_predicate_table_node(predicate_table_node_t *node,
				     const char *name) {
  node->num_link = 0;
  node->num_allocated = 1;
  node->name = name;
  node->links = NEW(predicate_table_to_symbol_t *, node->num_allocated);
}

predicate_table_to_symbol_t *predicate_table_node_add(
    predicate_table_node_t *node,
    int arity,
    symbol_table_node_t **nodes) {
  predicate_table_to_symbol_t *link = NEW(predicate_table_to_symbol_t, 1);
  int i;

  link->arity = arity;
  link->nodes = NEW(symbol_table_node_t *, arity);

  for (i=0; i<arity; ++i) {
    link->nodes[i] = nodes[i];
  }

  if (node->num_link >= node->num_allocated) {
    predicate_table_node_enlarge(node);
  }

  node->links[node->num_link++] = link;
  return link;
}

void predicate_table_node_enlarge(predicate_table_node_t *node) {
  node->num_allocated *= ENLARGE_FACTOR;
  node->links = RENEW(node->links,
		      predicate_table_to_symbol_t *,
		      node->num_allocated);
}

void destroy_predicate_table_node(predicate_table_node_t *node) {
  int i;

  for (i=0; i<node->num_link; ++i) {
    destroy_predicate_table_to_symbol(node->links[i]);
    free(node->links[i]);
  }

  node->num_link = node->num_allocated = 0;
  free(node->links);
  node->links = NULL;
}

/* ==== Predicate Table to Symbol ==== */
void initialize_predicate_table_to_symbol(predicate_table_to_symbol_t *link,
					  int arity) {
  link->arity = arity;
  link->nodes = NEW(symbol_table_node_t *, arity);
}

void destroy_predicate_table_to_symbol(predicate_table_to_symbol_t *link) {
  free(link->nodes);
  link->arity = 0;
}

/*****************************************
 * Solve Functions
 *****************************************/
void initialize_solve(solve_t *solve) {
  solve->num_goals = 0;
  solve->num_allocated = 1;
  solve->goals = NEW(solve_goal_t *, solve->num_allocated);
  solve->states = NEW(solve_goal_state_t *, solve->num_allocated);
}

void solve_add(solve_t *solve, solve_goal_t *goal) {
  if (solve->num_goals >= solve->num_allocated) {
    solve_enlarge(solve);
  }

  solve->goals[solve->num_goals] = goal;
  initialize_solve_goal_state(solve->states[solve->num_goals], goal);
  solve->num_goals++;
}

void solve_enlarge(solve_t *solve) {
  solve->num_allocated *= ENLARGE_FACTOR;
  solve->goals = RENEW(solve->goals,
		       solve_goal_t *,
		       solve->num_allocated);
  solve->states = RENEW(solve->states,
			solve_goal_state_t *,
			solve->num_allocated);
}

void initialize_solve_goal_state(solve_goal_state_t *state,
				 solve_goal_t *goal) {
  state->goal = goal;
  state->subgoal_index = 0;
  state->candidate = NULL;
  state->candidate_index = 0;
}

void initialize_solve_goal(solve_goal_t *goal,
			   predicate_table_node_t *predicate) {
  goal->predicate = predicate;
  goal->num_subgoals = 0;
  goal->num_allocated = 1;
  goal->subgoals = NEW(solve_subgoal_t *, goal->num_allocated);
}

void solve_goal_enlarge(solve_goal_t *goal) {
  goal->num_allocated *= ENLARGE_FACTOR;
  goal->subgoals = RENEW(goal->subgoals,
			 solve_subgoal_t *,
			 goal->num_allocated);
}

void solve_goal_add(solve_goal_t *goal, solve_subgoal_t *subgoal) {
  if (goal->num_subgoals >= goal->num_allocated) {
    solve_goal_enlarge(goal);
  }

  goal->subgoals[goal->num_subgoals] = subgoal;
}

void initialize_solve_subgoal(solve_subgoal_t *subgoal,
			      int pos,
			      solve_condition_t *condition) {
  subgoal->pos = pos;
  subgoal->condition = condition;
}

void initialize_solve_condition_constant(solve_condition_t *condition,
					 symbol_table_node_t *symbol) {
  condition->type = CONSTANT;
  condition->symbol = symbol;
}

void initialize_solve_condition_variable(solve_condition_t *condition,
					 symbol_table_node_t *symbol) {
  condition->type = VARIABLE;
  condition->symbol = symbol;
}

void initialize_solve_variable_table(solve_variable_table_t *table) {
  table->num_variables = 0;
  table->num_allocated = 1;
  table->conditions = NEW(solve_condition_t *, table->num_allocated);
}

void solve_variable_table_add(solve_variable_table_t *table,
			      solve_condition_t *condition) {
  if (table->num_variables >= table->num_allocated) {
    solve_variable_table_enlarge(table);
  }

  table->conditions[table->num_variables++] = condition;
}

void solve_variable_table_enlarge(solve_variable_table_t *table) {
  table->num_allocated *= ENLARGE_FACTOR;
  table->conditions = RENEW(table->conditions,
			    solve_condition_t *,
			    table->num_allocated);
}

solve_condition_t *solve_variable_table_find(solve_variable_table_t *table,
					     const char *name) {
  int i;
  solve_condition_t *condition;

  for (i=0; i<table->num_variables; ++i) {
    condition = table->conditions[i];
    if (strcmp(condition->symbol->name, name) == 0) {
      return condition;
    }
  }

  return NULL;
}

solve_condition_t *solve_variable_table_find_or_add(
    solve_variable_table_t *table,
    const char *name) {
  symbol_table_node_t *symbol;
  solve_condition_t *condition = solve_variable_table_find(table, name);

  if (condition == NULL) {
    symbol = NEW(symbol_table_node_t, 1);
    assert(symbol != NULL);
    initialize_symbol_table_node(symbol, name);

    condition = NEW(solve_condition_t, 1);
    assert(condition != NULL);
    initialize_solve_condition_variable(condition, symbol);

    solve_variable_table_add(table, condition);
  }

  return condition;
}

/*****************************************
 * Rule Functions
 *****************************************/
void define_facts(const mpc_ast_t *ast,
		  symbol_table_t *symbol_table,
		  predicate_table_t *predicate_table) {
  int ident_number,
      i;
  const char *params[MAX_PARAMS];
  find_tag_state_t fact_state,
                   predicate_state,
                   ident_state;
  const mpc_ast_t *fact,
                  *predicate,
                  *ident;

  initialize_tag_state(&fact_state, ast);
  while ((fact = find_tag_next(&fact_state, "fact")) != NULL) {
    initialize_tag_state(&predicate_state, fact);
    while ((predicate = find_tag_next(&predicate_state, "predicate")) != NULL) {
      ident_number = 0;

      for (i=0; i<MAX_PARAMS; ++i) {
	params[i] = NULL;
      }

      initialize_tag_state(&ident_state, predicate);
      while ((ident = find_tag_next(&ident_state, "ident")) != NULL) {
	params[ident_number++] = ident->contents;
      }

      rule_add(symbol_table,
	       predicate_table,
	       params[0],
	       ident_number - 1,
	       &params[1]);
    }
  }
}

void execute_queries(const mpc_ast_t *ast,
		     symbol_table_t *symbol_table,
		     predicate_table_t *predicate_table) {
  find_tag_state_t query_state;
  const mpc_ast_t *query;

  initialize_tag_state(&query_state, ast);
  while ((query = find_tag_next(&query_state, "query")) != NULL) {
    execute_query(query, symbol_table, predicate_table);
  }
}

void execute_query(const mpc_ast_t *ast,
		   symbol_table_t *symbol_table,
		   predicate_table_t *predicate_table) {
  find_tag_state_t predicate_state;
  const mpc_ast_t *predicate;
  solve_variable_table_t variables;
  solve_t solve;
  solve_goal_t *goal;

  initialize_solve(&solve);
  initialize_solve_variable_table(&variables);
  predicate_table = predicate_table;

  initialize_tag_state(&predicate_state, ast);
  while ((predicate = find_tag_next(&predicate_state, "predicate")) != NULL) {
    goal = execute_query_build_goal(predicate,
				    symbol_table,
				    predicate_table,
				    &variables);
    solve_add(&solve, goal);
  }

  /* unify(&solve, &variables); */
}

solve_goal_t *execute_query_build_goal(const mpc_ast_t *ast,
				       symbol_table_t *symbol_table,
				       predicate_table_t *predicate_table,
				       solve_variable_table_t *variables) {
  int ident_number;
  const char *name;
  find_tag_state_t ident_state;
  const mpc_ast_t *ident;
  solve_goal_t *goal = NEW(solve_goal_t, 1);
  predicate_table_node_t *predicate;
  solve_subgoal_t *subgoal;
  solve_condition_t *condition;
  symbol_table_node_t *symbol;

  goal = NEW(solve_goal_t, 1);
  assert(goal != NULL);

  ident_number = 0;
  initialize_tag_state(&ident_state, ast);
  {
    ident = find_tag_next(&ident_state, "ident");

    name = ident->contents;
    predicate = predicate_table_find_or_add(predicate_table, name);
    initialize_solve_goal(goal, predicate);
  }

  while ((ident = find_tag_next(&ident_state, "ident")) != NULL) {
    name = ident->contents;
    if (has_tag(ident, "variable")) {
      condition = solve_variable_table_find_or_add(variables, name);
    } else /* has_tag(ident, "constant") */ {
      symbol = symbol_table_find_or_add(symbol_table, name);

      condition = NEW(solve_condition_t, 1);
      assert(condition != NULL);

      initialize_solve_condition_constant(condition, symbol);
    }

    subgoal = NEW(solve_subgoal_t, 1);
    assert(subgoal != NULL);

    initialize_solve_subgoal(subgoal, ident_number++, condition);
    solve_goal_add(goal, subgoal);
  }

  return goal;
}

void rule_add(symbol_table_t *symbol_table,
	      predicate_table_t *predicate_table,
	      const char *pred_name,
	      const int arity,
	      const char **strings) {
  int i;
  predicate_table_node_t *predicate;
  predicate_table_to_symbol_t *link;
  symbol_table_node_t *symbols[MAX_PARAMS];

  predicate = predicate_table_find_or_add(predicate_table, pred_name);

  for (i=0; i<arity; ++i) {
    symbols[i] = symbol_table_find_or_add(symbol_table, strings[i]);
  }

  link = predicate_table_node_add(predicate, arity, symbols);

  for (i=0; i<arity; ++i) {
    symbol_table_node_add(symbols[i], i, predicate, link);
  }
}

void print_symbols(symbol_table_t *table) {
  int i,
      j;
  symbol_table_node_t *node;
  symbol_table_to_predicate_t *link;
  predicate_table_node_t *predicate;

  printf("Symbol Table:\n");
  for (i=0; i<table->num_symbols; ++i) {
    node = table->symbols[i];
    printf("'%s':\n", node->name);
    for (j=0; j<node->num_link; ++j) {
      link = node->links[j];
      predicate = link->predicate;
      printf("\t'%s': %d\n", predicate->name, link->position);
    }
  }
}

void print_predicates(predicate_table_t *table) {
  int i,
      j,
      k;
  predicate_table_node_t *node;
  predicate_table_to_symbol_t *link;
  symbol_table_node_t *symbol;

  printf("Predicate Table:\n");
  for (i=0; i<table->num_predicates; ++i) {
    node = table->predicates[i];
    for (j=0; j<node->num_link; ++j) {
      link = node->links[j];
      printf("%s(", node->name);
      for (k=0; k<link->arity; ++k) {
	symbol = link->nodes[k];
	if (k != 0) {
	  printf(",");
	}
	printf("%s", symbol->name);
      }
      printf(").\n");
    }
  }
}

void print_rules(symbol_table_t *symbol_table,
		 predicate_table_t *predicate_table) {
  print_symbols(symbol_table);
  print_predicates(predicate_table);
}

/*****************************************
 * Main function
 *****************************************/
int main(int argc, char **argv) {
  mpc_result_t r;
  symbol_table_t symbol_table;
  predicate_table_t predicate_table;
  int return_value;
  const char *filename = argc > 1 ? argv[1] : NULL;

  return_value = parse_file(&r, filename);
  if (!return_value) {
    return 1;
  }

  initialize_symbol_table(&symbol_table);
  initialize_predicate_table(&predicate_table);

  print_tags(r.output, 0);
  define_facts(r.output, &symbol_table, &predicate_table);

  print_rules(&symbol_table, &predicate_table);

  destroy_symbol_table(&symbol_table);
  destroy_predicate_table(&predicate_table);
  mpc_ast_delete(r.output);

  return 0;
}
