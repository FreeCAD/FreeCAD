%{
/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

/*
 * Syntax analyzer for SoCalculator expressions.
 *
 * Compile with
 *
 *         bison -Dapi.prefix=so_eval -o evaluator_tab.c -l evaluator.y
 *
 * ..with GNU bison version 1.27 (which is what we have on nfs.sim.no)
 * then patch the resulting evaluator_tab.c file with
 *
 *         patch -p0 < evaluator_tab.diff
 *
 * The patch is explained at the top of the diff file.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_IO_H
/* isatty() on windows */
#include <io.h>
#endif /* HAVE_IO_H */
#include <Inventor/C/basic.h>
#include "engines/evaluator.h"
#define yyalloc so_evalalloc
%}

%union
{
  int id;
  float value;
  char  reg;
  so_eval_node *node;
}

/* tokens that have some associated value */
%token <value> LEX_VALUE
%token <reg> LEX_TMP_FLT_REG LEX_IN_FLT_REG LEX_OUT_FLT_REG
%token <reg> LEX_TMP_VEC_REG LEX_IN_VEC_REG LEX_OUT_VEC_REG
%token <id> LEX_COMPARE LEX_FLTFUNC

/* tokens that do not have an associated value */
%token LEX_ATAN2 LEX_POW LEX_FMOD LEX_LEN
%token LEX_CROSS LEX_DOT LEX_NORMALIZE LEX_VEC3F
%token ',' '[' ']' '(' ')' ';'

/* error token */
%token LEX_ERROR

/* typedefs of non-terminals */
%type <node> expression subexpression fltlhs veclhs
%type <node> fltstatement
%type <node> vecstatement
%type <node> boolstatement

/* operator priorities (based on C operator precedence and associativity */
%right '='
%right '?' ':'
%left LEX_OR
%left LEX_AND
%left LEX_EQ LEX_NEQ
%left LEX_COMPARE
%left '+' '-'
%left '*' '/' '%'
%left '!' UNARY

%start expression

%{
  static char * get_regname(char reg, int regtype);
  enum { REGTYPE_IN, REGTYPE_OUT, REGTYPE_TMP };
  static so_eval_node *root_node;
  static int so_evalerror(char *);
  static int so_evallex(void);
%}

%%

expression    : expression ';' subexpression
              { root_node = so_eval_create_binary(ID_SEPARATOR, $1, $3); $$ = root_node; }
              | subexpression { root_node = $1; $$ = $1; }
              ;

subexpression : { $$ = NULL; }
              | fltlhs '=' fltstatement { $$ = so_eval_create_binary(ID_ASSIGN_FLT, $1, $3); }
              | veclhs '=' vecstatement { $$ = so_eval_create_binary(ID_ASSIGN_VEC, $1, $3); }
              ;

fltlhs        : LEX_TMP_FLT_REG { $$ = so_eval_create_reg(get_regname($1, REGTYPE_TMP)); }
              | LEX_OUT_FLT_REG { $$ = so_eval_create_reg(get_regname($1, REGTYPE_OUT)); }
              | LEX_TMP_VEC_REG '[' LEX_VALUE ']'
              { $$ = so_eval_create_reg_comp(get_regname($1, REGTYPE_TMP), (int) $3); }
              | LEX_OUT_VEC_REG '[' LEX_VALUE ']'
              { $$ = so_eval_create_reg_comp(get_regname($1, REGTYPE_OUT), (int) $3); }
              ;

veclhs        : LEX_TMP_VEC_REG { $$ = so_eval_create_reg(get_regname($1, REGTYPE_TMP));}
              | LEX_OUT_VEC_REG { $$ = so_eval_create_reg(get_regname($1, REGTYPE_OUT));}
              ;

fltstatement  : boolstatement '?' fltstatement ':' fltstatement
              { $$ = so_eval_create_ternary(ID_FLT_COND, $1, $3, $5); }
              | fltstatement '?' fltstatement ':' fltstatement
              { $$ = so_eval_create_ternary(ID_FLT_COND, so_eval_create_unary(ID_TEST_FLT, $1), $3, $5); }
              | vecstatement '?' fltstatement ':' fltstatement
              { $$ = so_eval_create_ternary(ID_FLT_COND, so_eval_create_unary(ID_TEST_VEC, $1), $3, $5); }

              | fltstatement '+' fltstatement { $$ = so_eval_create_binary(ID_ADD, $1, $3); }
              | fltstatement '-' fltstatement { $$ = so_eval_create_binary(ID_SUB, $1, $3); }
              | fltstatement '/' fltstatement { $$ = so_eval_create_binary(ID_DIV, $1, $3); }
              | fltstatement '*' fltstatement { $$ = so_eval_create_binary(ID_MUL, $1, $3); }
              | fltstatement '%' fltstatement { $$ = so_eval_create_binary(ID_FMOD, $1, $3); }
              | LEX_ATAN2 '(' fltstatement ',' fltstatement ')'
              { $$ = so_eval_create_binary(ID_ATAN2, $3, $5); }
              | LEX_POW '(' fltstatement ',' fltstatement ')'
              { $$ = so_eval_create_binary(ID_POW, $3, $5); }
              | LEX_FMOD '(' fltstatement ',' fltstatement ')'
              { $$ = so_eval_create_binary(ID_FMOD, $3, $5); }

              | '-' fltstatement %prec UNARY { $$ = so_eval_create_unary(ID_NEG, $2); }
              | '(' fltstatement ')' { $$ = $2; }
              | LEX_FLTFUNC '(' fltstatement ')' { $$ = so_eval_create_unary($1, $3);}
              | LEX_LEN '(' vecstatement ')' { $$ = so_eval_create_unary(ID_LEN, $3);}
              | LEX_DOT '(' vecstatement ',' vecstatement ')'
              { $$ = so_eval_create_binary(ID_DOT, $3, $5); }
              | LEX_TMP_FLT_REG { $$ = so_eval_create_reg(get_regname($1, REGTYPE_TMP));}
              | LEX_OUT_FLT_REG { $$ = so_eval_create_reg(get_regname($1, REGTYPE_OUT));}
              | LEX_IN_FLT_REG { $$ = so_eval_create_reg(get_regname($1, REGTYPE_IN));}
              | LEX_TMP_VEC_REG '[' LEX_VALUE ']'
              { $$ = so_eval_create_reg_comp(get_regname($1, REGTYPE_TMP), (int) $3);}
              | LEX_IN_VEC_REG '[' LEX_VALUE ']'
              { $$ = so_eval_create_reg_comp(get_regname($1, REGTYPE_IN), (int) $3);}
              | LEX_OUT_VEC_REG '[' LEX_VALUE ']'
              { $$ = so_eval_create_reg_comp(get_regname($1, REGTYPE_OUT), (int) $3);}
              | LEX_VALUE
              { $$ = so_eval_create_flt_val($1); }
              ;

vecstatement  : boolstatement '?' vecstatement ':' vecstatement
              { $$ = so_eval_create_ternary(ID_VEC_COND, $1, $3, $5); }
              | fltstatement '?' vecstatement ':' vecstatement
              { $$ = so_eval_create_ternary(ID_VEC_COND, so_eval_create_unary(ID_TEST_FLT, $1), $3, $5); }
              | vecstatement '?' vecstatement ':' vecstatement
              { $$ = so_eval_create_ternary(ID_VEC_COND, so_eval_create_unary(ID_TEST_VEC, $1), $3, $5); }

              | vecstatement '+' vecstatement  { $$ = so_eval_create_binary(ID_ADD_VEC, $1, $3); }
              | vecstatement '-' vecstatement { $$ = so_eval_create_binary(ID_SUB_VEC, $1, $3); }
              | vecstatement '*' fltstatement { $$ = so_eval_create_binary(ID_MUL_VEC_FLT, $1, $3); }
              | vecstatement '/' fltstatement { $$ = so_eval_create_binary(ID_DIV_VEC_FLT, $1, $3); }
              | fltstatement '*' vecstatement { $$ = so_eval_create_binary(ID_MUL_VEC_FLT, $3, $1); }
              | LEX_CROSS '(' vecstatement ',' vecstatement ')'
              { $$ = so_eval_create_binary(ID_CROSS, $3, $5); }

              |'-' vecstatement %prec UNARY { $$ = so_eval_create_unary(ID_NEG_VEC, $2); }
              | '(' vecstatement ')' { $$ = $2; }
              | LEX_NORMALIZE '(' vecstatement ')'
              { $$ = so_eval_create_unary(ID_NORMALIZE, $3); }

              | LEX_TMP_VEC_REG { $$ = so_eval_create_reg(get_regname($1, REGTYPE_TMP));}
              | LEX_OUT_VEC_REG { $$ = so_eval_create_reg(get_regname($1, REGTYPE_OUT));}
              | LEX_IN_VEC_REG { $$ = so_eval_create_reg(get_regname($1, REGTYPE_IN));}
              | LEX_VEC3F '(' fltstatement ',' fltstatement ',' fltstatement ')'
              { $$ = so_eval_create_ternary(ID_VEC3F, $3, $5, $7); }
              ;

boolstatement : fltstatement LEX_EQ fltstatement { $$ = so_eval_create_binary(ID_EQ, $1, $3); }
              | fltstatement LEX_NEQ fltstatement { $$ = so_eval_create_binary(ID_NEQ, $1, $3); }
              | vecstatement LEX_EQ vecstatement { $$ = so_eval_create_binary(ID_EQ, $1, $3); }
              | vecstatement LEX_NEQ vecstatement { $$ = so_eval_create_binary(ID_NEQ, $1, $3); }
              | fltstatement LEX_COMPARE fltstatement { $$ = so_eval_create_binary($2, $1, $3); }
              | boolstatement LEX_AND boolstatement { $$ = so_eval_create_binary(ID_AND, $1, $3); }
              | boolstatement LEX_OR boolstatement { $$ = so_eval_create_binary(ID_OR, $1, $3); }
              | '!' boolstatement %prec UNARY { $$ = so_eval_create_unary(ID_NOT, $2); }
              | '(' boolstatement ')' { $$ = $2; }
              ;
%%

/*
 * Creates a register name from the register type and register char.
 *
 * Note: don't "const" the return type, as that will trigger a bug in
 * Microsoft Visual C++ v6.0. 20000606 mortene.
*/
static char *
get_regname(char reg, int regtype)
{
  static char buf[3];
  buf[2] = 0;

  if (regtype != REGTYPE_IN) {
    if (regtype == REGTYPE_TMP) buf[0] = 't';
    else if (regtype == REGTYPE_OUT) buf[0] = 'o';
    buf[1] = reg;
    buf[2] = 0;
  }
  else {
    buf[0] = reg;
    buf[1] = 0;
  }
  return buf;
}



#include "so_eval.ic" /* our lexical scanner */

/* some very simple error handling for now :) */
static char *myerrorptr;
static char myerrorbuf[512];

/*
 * parse the text string into a tree structure.
 */
so_eval_node *
so_eval_parse(const char *buffer)
{
  /* FIXME: better error handling is obviously needed */
  YY_BUFFER_STATE state;
  myerrorptr = NULL;
  root_node = NULL;
  state = so_eval_scan_string(buffer); /* flex routine */
  so_evalparse(); /* start parsing */
  so_eval_delete_buffer(state); /* flex routine */
  if (myerrorptr) return NULL;
  return root_node;
}

/*
 * Returns current error message or NULL if none.
 *
 * Note: don't "const" the return type, as that will trigger a bug in
 * Microsoft Visual C++ v6.0. 20000606 mortene.
 */
char *
so_eval_error(void)
{
  return myerrorptr;
}

/*
 * Called by bison parser upon lexical/syntax error.
 */
int
so_evalerror(char *myerr)
{
  strncpy(myerrorbuf, myerr, 512);
  myerrorbuf[511] = 0; /* just in case string was too long */
  myerrorptr = myerrorbuf; /* signal error */
  so_eval_delete(root_node); /* free memory used */
  root_node = NULL;
  return 0;
}
