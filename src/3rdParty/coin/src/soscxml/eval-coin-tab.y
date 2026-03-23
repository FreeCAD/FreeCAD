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
 * Generate the compilable source files this way:
 *
 * bison -Dapi.prefix=scxml_coin_ -o eval-coin-tab.cpp eval-coin-tab.y
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cstdio>

#include "soscxml/eval-coin.h"
#include <Inventor/scxml/ScXMLCoinEvaluator.h>
#include <Inventor/scxml/ScXMLMinimumEvaluator.h>

namespace {
static ScXMLDataObj * root = NULL;
}
#define yyalloc scxml_coin_alloc
%}

%union {
  double real;
  char * stringptr;
  ScXMLDataObj * scxmlobj;
}

%token SCXML_COIN_PAREN_OPEN SCXML_COIN_PAREN_CLOSE
%token SCXML_COIN_EVENT_SCOPE

%token SCXML_COIN_BOOL_TRUE SCXML_COIN_BOOL_FALSE
%token <real> SCXML_COIN_REAL
%token <stringptr> SCXML_COIN_STRING SCXML_COIN_SBVALUE

%token <stringptr> SCXML_COIN_IDENTIFIER
%token SCXML_COIN_IN_FUNC
%token SCXML_COIN_LENGTH_FUNC

%token SCXML_COIN_OP_NOT
%token SCXML_COIN_OP_AND
%token SCXML_COIN_OP_OR
%token SCXML_COIN_OP_EQUALS
%token SCXML_COIN_OP_NOT_EQUALS

%token SCXML_COIN_OP_ADD
%token SCXML_COIN_OP_SUBTRACT
%token SCXML_COIN_OP_MULTIPLY
%token SCXML_COIN_OP_DIVIDE
%token SCXML_COIN_OP_NEGATE

%token SCXML_COIN_OP_APPEND

%token SCXML_COIN_END

%defines

%type <scxmlobj> rootexpr
%type <scxmlobj> expr
%type <scxmlobj> constexpr
%type <scxmlobj> constboolexpr
%type <scxmlobj> constrealexpr
%type <scxmlobj> conststringexpr
%type <scxmlobj> constsbvalexpr
%type <scxmlobj> refexpr
%type <scxmlobj> evalexpr
%type <scxmlobj> infuncexpr
%type <scxmlobj> lengthfuncexpr
%type <scxmlobj> logicalexpr
%type <scxmlobj> arithmeticexpr

%start rootexpr

%%

rootexpr            : expr { $$ = $1; root = $$; }
                    ;

expr                : SCXML_COIN_PAREN_OPEN expr SCXML_COIN_PAREN_CLOSE { $$ = $2; }
                    | evalexpr { $$ = $1; }
                    | logicalexpr { $$ = $1; }
                    | arithmeticexpr { $$ = $1; }
                    | refexpr { $$ = $1; }
                    | constexpr { $$ = $1; }
                    ;

constexpr           : constboolexpr { $$ = $1; }
                    | constrealexpr { $$ = $1; }
                    | conststringexpr { $$ = $1; }
                    | constsbvalexpr { $$ = $1; }
                    ;

constboolexpr       : SCXML_COIN_BOOL_TRUE
                      { $$ = ScXMLBoolDataObj::createFor(TRUE); }
                    | SCXML_COIN_BOOL_FALSE
                      { $$ = ScXMLBoolDataObj::createFor(FALSE); }
                    ;

constsbvalexpr      : SCXML_COIN_SBVALUE
                      { $$ = ScXMLSbDataObj::createFor($1); }
                    ;

constrealexpr       : SCXML_COIN_REAL
                      { $$ = ScXMLRealDataObj::createFor($1); }
                    ;

conststringexpr     : SCXML_COIN_STRING
                      { $$ = ScXMLStringDataObj::createFor($1); }
                    ;

refexpr             : SCXML_COIN_IDENTIFIER
                      { $$ = ScXMLReferenceDataObj::createFor($1); }
                    ;

evalexpr            : infuncexpr { $$ = $1; }
                    ;

infuncexpr          : SCXML_COIN_IN_FUNC SCXML_COIN_PAREN_OPEN SCXML_COIN_IDENTIFIER SCXML_COIN_PAREN_CLOSE
                      { $$ = ScXMLInExprDataObj::createFor($3); }
                    ;
lengthfuncexpr      : SCXML_COIN_LENGTH_FUNC SCXML_COIN_PAREN_OPEN arithmeticexpr SCXML_COIN_PAREN_CLOSE
                      { $$ = ScXMLCoinLengthFuncExprDataObj::createFor($3); }
                    ;

logicalexpr         : SCXML_COIN_PAREN_OPEN logicalexpr SCXML_COIN_PAREN_CLOSE
                      { $$ = $2; }
                    | constboolexpr
                      { $$ = $1; }
                    | logicalexpr SCXML_COIN_OP_OR logicalexpr
                      { $$ = ScXMLOrOpExprDataObj::createFor($1, $3); }
                    | logicalexpr SCXML_COIN_OP_AND logicalexpr
                      { $$ = ScXMLAndOpExprDataObj::createFor($1, $3); }
                    | conststringexpr SCXML_COIN_OP_EQUALS conststringexpr
                      { $$ = ScXMLCoinEqualsOpExprDataObj::createFor($1, $3); }
                    | refexpr SCXML_COIN_OP_EQUALS conststringexpr
                      { $$ = ScXMLCoinEqualsOpExprDataObj::createFor($1, $3); }
                    | logicalexpr SCXML_COIN_OP_EQUALS logicalexpr
                      { $$ = ScXMLCoinEqualsOpExprDataObj::createFor($1, $3); }
                    | arithmeticexpr SCXML_COIN_OP_EQUALS arithmeticexpr
                      { $$ = ScXMLCoinEqualsOpExprDataObj::createFor($1, $3); }
                    | refexpr SCXML_COIN_OP_NOT_EQUALS conststringexpr
                      { $$ = ScXMLNotOpExprDataObj::createFor(
                               ScXMLEqualsOpExprDataObj::createFor($1, $3)); }
                    | conststringexpr SCXML_COIN_OP_NOT_EQUALS conststringexpr
                      { $$ = ScXMLNotOpExprDataObj::createFor(
                               ScXMLEqualsOpExprDataObj::createFor($1, $3)); }
                    | logicalexpr SCXML_COIN_OP_NOT_EQUALS logicalexpr
                      { $$ = ScXMLNotOpExprDataObj::createFor(
                               ScXMLEqualsOpExprDataObj::createFor($1, $3)); }
                    | arithmeticexpr SCXML_COIN_OP_NOT_EQUALS arithmeticexpr
                      { $$ = ScXMLNotOpExprDataObj::createFor(
                               ScXMLEqualsOpExprDataObj::createFor($1, $3)); }
                    | SCXML_COIN_OP_NOT logicalexpr
                      { $$ = ScXMLNotOpExprDataObj::createFor($2); }
                    | SCXML_COIN_OP_NOT refexpr
                      { $$ = ScXMLNotOpExprDataObj::createFor($2); }
                    ;

arithmeticexpr      : constrealexpr
                      { $$ = $1; }
                    | constsbvalexpr
                      { $$ = $1; }
                    | refexpr
                      { $$ = $1; }
                    | lengthfuncexpr
                      { $$ = $1; }
                    | arithmeticexpr SCXML_COIN_OP_ADD arithmeticexpr
                      { $$ = ScXMLAddOpExprDataObj::createFor($1, $3); }
                    | arithmeticexpr SCXML_COIN_OP_SUBTRACT arithmeticexpr
                      { $$ = ScXMLSubtractOpExprDataObj::createFor($1, $3); }
                    | arithmeticexpr SCXML_COIN_OP_MULTIPLY arithmeticexpr
                      { $$ = ScXMLMultiplyOpExprDataObj::createFor($1, $3); }
                    | arithmeticexpr SCXML_COIN_OP_DIVIDE arithmeticexpr
                      { $$ = ScXMLDivideOpExprDataObj::createFor($1, $3); }
                    | SCXML_COIN_OP_NEGATE arithmeticexpr
                      { $$ = ScXMLNegateOpExprDataObj::createFor($2); }
                    ;

%%

ScXMLDataObj *
scxml_coin_get_root_obj(void)
{
  return root;
}

void
scxml_coin_clear_root_obj(void)
{
  root = NULL;
}
