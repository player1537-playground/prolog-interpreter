#include "mpc/mpc.h"

int main(int argc, char **argv) {

  mpc_parser_t	*Ident	   = mpc_new("ident");
  mpc_parser_t	*Params	   = mpc_new("params");
  mpc_parser_t	*Predicate = mpc_new("predicate");
  mpc_parser_t	*Fact	   = mpc_new("fact");
  mpc_parser_t	*Lang	   = mpc_new("lang");

  mpca_lang(MPCA_LANG_PREDICTIVE,
	    " ident     : /[a-z0-9_]+/;                            "
	    " params    : <ident> (',' <ident>)*;                  "
	    " predicate : <ident> '(' <params> ')';                "
	    " fact      : <predicate> '.';                         "
	    " lang      : <fact> (<fact>)*;                        ",
	    Ident, Params, Predicate, Fact, Lang, NULL);

  printf("Ident:     "); mpc_print(Ident);
  printf("Params:    "); mpc_print(Params);
  printf("Predicate: "); mpc_print(Predicate);
  printf("Fact:      "); mpc_print(Fact);
  printf("Lang:      "); mpc_print(Lang);

  if (argc > 1) {

    mpc_result_t r;
    if (mpc_parse_contents(argv[1], Lang, &r)) {
      mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

  } else {

    mpc_result_t r;
    if (mpc_parse_pipe("<stdin>", stdin, Lang, &r)) {
      mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

  }

  mpc_cleanup(5, Ident, Params, Predicate, Fact, Lang);

  return 0;

}
