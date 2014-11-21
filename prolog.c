#include "prolog.h"
#include "mpc/mpc.h"
#include <assert.h>

#define NEW(type, num) ((type*)malloc(sizeof((type)) * (num)))
#define TYPE_SUB_POINTER(var) (typeof(*(typeof(orig))0))
#define RENEW(orig, num) ((typeof(orig))realloc((orig),			\
						sizeof(TYPE_SUB_POINTER(orig)) * (num)))
#define MAX_PARAMS 10
#define ENLARGE_FACTOR 2

/*****************************************
 * AST Functions
 *****************************************/
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
    if (strstr(child_ast->tag, tag) != NULL) {
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

void define_facts(const mpc_ast_t *ast) {
  int ident_number,
      i;
  char *params[MAX_PARAMS];
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

      for (i=0; i<ident_number; ++i) {
	if (i == 0) {
	  printf("%s(", params[i]);
	} else if (i == ident_number - 1) {
	  printf("%s)\n", params[i]);
	} else {
	  printf("%s,", params[i]);
	}
      }
    }
    printf("\n");
  }
}

/*****************************************
 * Symbol Table Functions
 *****************************************/
void initialize_symbol_table(symbol_table_t *table) {
  table->num_symbols = table->num_allocated = 0;
  table->symbols = NULL;
}

void initialize_symbol_table_node(symbol_table_node_t *node, const char *name) {
  node->num_link = node->num_allocated = 0;
  node->name = name;
  node->links = NULL;
}

void initialize_symbol_table_to_predicate(symbol_table_to_predicate_t *link,
					  int pos,
					  predicate_table_to_symbol_t *ref) {
  link->position = pos;
  link->predicate = ref;
}

symbol_table_node_t *symbol_table_add(symbol_table_t *table, const char *name) {
  symbol_table_node_t *node = NEW(symbol_table_node_t, 1);

  initialize_symbol_table_node(node, name);

  if (table->num_symbols >= table->num_allocated) {
    symbol_table_enlarge(table);
  }

  table->symbols[table->num_symbols]++ = node;
  return node;
}

void symbol_table_link_add(symbol_table_node_t *node,
			   int pos,
			   predicate_table_to_symbol_t *predicate) {
  symbol_table_to_predicate_t *link = NEW(symbol_table_to_predicate_t, 1);

  initialize_symbol_table_to_predicate(link, pos, predicate);

  if (node->num_link >= node->num_allocated) {
    symbol_table_node_enlarge(node);
  }

  node->links[node->num_link++] = link;
}

void symbol_table_enlarge(symbol_table_t *table) {
  table->num_allocated *= ENLARGE_FACTOR;
  table->symbols = RENEW(table->symbols, table->num_allocated);
}

void symbol_table_node_enlarge(symbol_table_node_t *node) {
  node->num_allocated *= ENLARGE_FACTOR;
  node->links = RENEW(node->links
}

/*****************************************
 * Predicate Table Functions
 *****************************************/
void initialize_predicate_table(predicate_table_t *table) {
  table->num_predicates = table->num_allocated = 0;
  table->predicates = NULL;
}

void initialize_predicate_table_node(predicate_table_node_t *node, const char *name) {
  node->num_link = node->num_allocated = 0;
  node->name = name;
  node->links = NULL;
}

void initialize_predicate_table_to_symbol(predicate_table_to_symbol_t *link,
					  int arity,
					  ...) {
  va_list argp;
  symbol_table_node_t *symbol;
  int i;

  va_start(argp, arity);

  link->position = pos;
  link->predicate = ref;
  link->nodes = NEW(symbol_table_node_t *, arity);

  for (i=0; i<arity; ++i) {
    link->nodes[i] = va_arg(argp, symbol_table_node_t *);
  }

  va_end(argp);
}


int main(int argc, char **argv) {

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
  mpc_result_t r;

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

  if (argc > 1) {
    return_value = mpc_parse_contents(argv[1], Lang, &r);
  } else {
    return_value = mpc_parse_pipe("<stdin>", stdin, Lang, &r);
  }

  if (!return_value) {
    mpc_err_print(r.error);
    mpc_err_delete(r.error);

    goto cleanup;
  }

  print_tags(r.output, 0);
  define_facts(r.output);

  mpc_ast_delete(r.output);

 cleanup:
  mpc_cleanup(5, Ident, Params, Predicate, Fact, Lang);

  return 0;

}
