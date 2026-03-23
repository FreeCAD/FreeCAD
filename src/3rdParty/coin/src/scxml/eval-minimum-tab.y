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
 * bison -Dapi.prefix=scxml_minimum_ -o eval-minimum-tab.cpp eval-minimum-tab.y
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cstdio>

#include "scxml/eval-minimum.h"
#include <Inventor/scxml/ScXMLMinimumEvaluator.h>

namespace {
static ScXMLDataObj * root = NULL;
}
#define yyalloc scxml_minimum_alloc
%}


%union {
  char * stringptr;
  ScXMLDataObj * scxmlobj;
}

%token SCXML_MINIMUM_PAREN_OPEN SCXML_MINIMUM_PAREN_CLOSE
// %token <stringptr> SCXML_MINIMUM_STRING
%token <stringptr> SCXML_MINIMUM_IDENTIFIER
%token SCXML_MINIMUM_IN_FUNC

%defines

%type <scxmlobj> rootexpr
%type <scxmlobj> expr
%type <scxmlobj> evalexpr
%type <scxmlobj> infuncexpr

%start rootexpr

%%

rootexpr            : expr { $$ = $1; root = $$; }
                    ;

expr                : evalexpr { $$ = $1; }
                    ;

evalexpr            : infuncexpr { $$ = $1; }
                    ;

infuncexpr          : SCXML_MINIMUM_IN_FUNC SCXML_MINIMUM_PAREN_OPEN SCXML_MINIMUM_IDENTIFIER SCXML_MINIMUM_PAREN_CLOSE
                      { $$ = ScXMLInExprDataObj::createFor($3); }
                    ;

%%

ScXMLDataObj *
scxml_minimum_get_root_obj(void)
{
  return root;
}

void
scxml_minimum_clear_root_obj(void)
{
  root = NULL;
}
