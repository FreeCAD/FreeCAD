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

/* Run 'flex eval-coin.l' to generate the compilable source files */

#include <cmath>
#include <cfloat>
#include "soscxml/eval-coin.h"
#ifndef SCXML_COIN_OP_ADD
#include "soscxml/eval-coin-tab.hpp"
#else
#endif // !SCXML_COIN_OP_ADD
}

%option outfile="eval-coin.cpp"
%option prefix="scxml_coin_"
%option case-sensitive
%option never-interactive
%option noyywrap
%option nounput
%option noinput

DIGIT               [0123456789]
DIGITS              ({DIGIT})+
LETTER              [aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ]
LETTERS             ({LETTER})+

SIGN                ([+-])?
EXP                 [eE]{SIGN}{DIGITS}
FLOAT               {SIGN}{DIGITS}("."{DIGITS})?({EXP})?
WHITE               [ \t]*
WS                  [ \t]{WHITE}
LINE                [^\n]*

ID                  {LETTER}({LETTER}|{DIGIT}|[_])*("()")?
MLID                {ID}("."{ID})*

TRUE                ("TRUE"|"True"|"true")
FALSE               ("FALSE"|"False"|"false")
EVENTSCOPE          "_event."
DATASCOPE           "_data."
CAMERASCOPE         "coin:camera."

%%

{WS}                { }
{TRUE}              { return SCXML_COIN_BOOL_TRUE; }
{FALSE}             { return SCXML_COIN_BOOL_FALSE; }
{FLOAT}             { scxml_coin_lval.real = atof(yytext); return SCXML_COIN_REAL; }
M_PI                { scxml_coin_lval.real = M_PI; return SCXML_COIN_REAL; }
M_PI_2              { scxml_coin_lval.real = M_PI_2; return SCXML_COIN_REAL; }
FLT_MAX             { scxml_coin_lval.real = FLT_MAX; return SCXML_COIN_REAL; }
FLT_MIN             { scxml_coin_lval.real = FLT_MIN; return SCXML_COIN_REAL; }
M_E                 { scxml_coin_lval.real = M_E; return SCXML_COIN_REAL; }
M_LOG2E             { scxml_coin_lval.real = M_LOG2E; return SCXML_COIN_REAL; }
M_LOG10E            { scxml_coin_lval.real = M_LOG10E; return SCXML_COIN_REAL; }
M_LN2               { scxml_coin_lval.real = M_LN2; return SCXML_COIN_REAL; }
M_SQRT2             { scxml_coin_lval.real = M_SQRT2; return SCXML_COIN_REAL; }
M_SQRT1_2           { scxml_coin_lval.real = M_SQRT1_2; return SCXML_COIN_REAL; }
Sb[^(]*"("[^)]*")"  { scxml_coin_lval.stringptr = yytext; return SCXML_COIN_SBVALUE; }
"'"[^']*"'"         { scxml_coin_lval.stringptr = yytext; return SCXML_COIN_STRING; }
"("                 { return SCXML_COIN_PAREN_OPEN; }
")"                 { return SCXML_COIN_PAREN_CLOSE; }

In                  { return SCXML_COIN_IN_FUNC; }
Length              { return SCXML_COIN_LENGTH_FUNC; }
_event"."{MLID}     { scxml_coin_lval.stringptr = yytext;
                      return SCXML_COIN_IDENTIFIER; }
_data"."{ID}        { scxml_coin_lval.stringptr = yytext;
                      return SCXML_COIN_IDENTIFIER; }
coin:camera"."{MLID} { scxml_coin_lval.stringptr = yytext;
                      return SCXML_COIN_IDENTIFIER; }
coin:temp"."{ID}    { scxml_coin_lval.stringptr = yytext;
                      return SCXML_COIN_IDENTIFIER; }
{ID}                { scxml_coin_lval.stringptr = yytext;
                      return SCXML_COIN_IDENTIFIER; }

"=="                { return SCXML_COIN_OP_EQUALS; }
"!="                { return SCXML_COIN_OP_NOT_EQUALS; }
"||"                { return SCXML_COIN_OP_OR; }
"&&"                { return SCXML_COIN_OP_AND; }
"+"                 { return SCXML_COIN_OP_ADD; }
"-"                 { return SCXML_COIN_OP_SUBTRACT; }
"*"                 { return SCXML_COIN_OP_MULTIPLY; }
"/"                 { return SCXML_COIN_OP_DIVIDE; }
"!"                 { return SCXML_COIN_OP_NOT; }

 /* <<EOF>>             { return SCXML_COIN_END; } */

%%


int
scxml_coin_error(const char * error)
{
  printf("\nscxml_coin error: %s\n", error);
  return 0;
}

ScXMLDataObj *
scxml_coin_parse(const char * buffer)
{
  YY_BUFFER_STATE yybuf = scxml_coin__scan_string(buffer);
  scxml_coin__switch_to_buffer(yybuf);
  scxml_coin_parse(); // the bison function
  scxml_coin__delete_buffer(yybuf);
  ScXMLDataObj * dataobj = scxml_coin_get_root_obj();
  scxml_coin_clear_root_obj();
  return dataobj;
}

