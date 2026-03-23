%top{
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

/* Run 'flex eval-minimum.l' to generate the compilable source files */

#include <cmath>
#include <cfloat>
#include "scxml/eval-minimum.h"
#ifndef SCXML_MINIMUM_OP_ADD
#include "scxml/eval-minimum-tab.hpp"
#else
#endif // !SCXML_MINIMUM_OP_ADD
}

%option outfile="eval-minimum.cpp"
%option prefix="scxml_minimum_"
%option case-sensitive
%option never-interactive
%option noyywrap
%option nounput
%option noinput

DIGITSET            0123456789
LETTERSET           aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ
DIGIT               [0123456789]
DIGITS              {DIGIT}+
LETTER              [{LETTERSET}]
LETTERS             {LETTER}+

SIGN                ([+-])?
EXP                 [eE]{SIGN}{DIGITS}
FLOAT               {SIGN}{DIGITS}("."{DIGITS})?({EXP})?
WHITE               [ \t]*
WS                  [ \t]{WHITE}
LINE                [^\n]*

ID                  {LETTER}[{LETTERSET}{DIGITSET}_]+

TRUE                ("TRUE"|"True"|"true")
FALSE               ("FALSE"|"False"|"false")

%%

{WS}                { }
"("                 { return SCXML_MINIMUM_PAREN_OPEN; }
")"                 { return SCXML_MINIMUM_PAREN_CLOSE; }

"In"                { return SCXML_MINIMUM_IN_FUNC; }
{ID}                { scxml_minimum_lval.stringptr = yytext;
                      return SCXML_MINIMUM_IDENTIFIER; }
%%

// "'"[^']*"'"         { scxml_minimum_lval.stringptr = yytext; return SCXML_MINIMUM_STRING; }

int
scxml_minimum_error(const char * error)
{
  printf("\nscxml_minimum error: %s\n", error);
  return 0;
}

ScXMLDataObj *
scxml_minimum_parse(const char * buffer)
{
  YY_BUFFER_STATE yybuf = scxml_minimum__scan_string(buffer);
  scxml_minimum__switch_to_buffer(yybuf);
  scxml_minimum_parse(); // the bison function
  scxml_minimum__delete_buffer(yybuf);
  ScXMLDataObj * dataobj = scxml_minimum_get_root_obj();
  scxml_minimum_clear_root_obj();
  return dataobj;
}

