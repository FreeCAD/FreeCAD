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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* !COIN_INTERNAL */

/*
 * Lexical scanner for SoCalculator. Compile into lexical scanner
 * lex.so_eval.c with
 *
 *             flex -oso_eval.ic -L evaluator.l
 *
 * (Note: flex version should be 2.5.4, since this is what is
 * installed on nfs.sim.no).
 *
 * Flex 2.5.4 generates code that includes unitstd.h. unistd.h does
 * not exist on the windows platform. You therefore need to apply
 * a patch after running flex:
 *
 *             patch -p0 < so_eval.diff
 *
 */

#include <Inventor/C/basic.h>
#include <float.h>
#include <stdlib.h>
#include "engines/evaluator.h"
%}

%option noyywrap
%option prefix="so_eval"

Digit                           [0-9]
Space                           [ \t\n]
Exp                             [eE][+-]?{Digit}+

%%
{Space}+                        ;
"<="                            { yylval.id = ID_LEQ; return LEX_COMPARE; }
">="                            { yylval.id = ID_GEQ; return LEX_COMPARE; }
"<"                             { yylval.id = ID_LT; return LEX_COMPARE; }
">"                             { yylval.id = ID_GT; return LEX_COMPARE; }
"=="                            return LEX_EQ;
"!="                            return LEX_NEQ;
"&&"                            return LEX_AND;
"||"                            return LEX_OR;
"-"                             return '-';
[!+*/=\[\]%?:(),;]              return(yytext[0]);
MAXFLOAT                        { yylval.value = FLT_MAX; return LEX_VALUE; }
MINFLOAT                        { yylval.value = FLT_MIN; return LEX_VALUE; }
M_E                             { yylval.value = (float)M_E; return LEX_VALUE; }
M_LOG2E                         { yylval.value = (float)M_LOG2E; return LEX_VALUE; }
M_LOG10E                        { yylval.value = (float)M_LOG10E; return LEX_VALUE; }
M_LN2                           { yylval.value = (float)M_LN2; return LEX_VALUE; }
M_PI                            { yylval.value = (float)M_PI; return LEX_VALUE; }
M_SQRT2                         { yylval.value = (float)M_SQRT2; return LEX_VALUE; }
M_SQRT1_2                       { yylval.value = (float)M_SQRT1_2; return LEX_VALUE; }
cos                             { yylval.id = ID_COS; return LEX_FLTFUNC; }
sin                             { yylval.id = ID_SIN; return LEX_FLTFUNC; }
tan                             { yylval.id = ID_TAN; return LEX_FLTFUNC; }
acos                            { yylval.id = ID_ACOS; return LEX_FLTFUNC; }
asin                            { yylval.id = ID_ASIN; return LEX_FLTFUNC; }
atan                            { yylval.id = ID_ATAN; return LEX_FLTFUNC; }
atan2                           return LEX_ATAN2;
cosh                            { yylval.id = ID_COSH; return LEX_FLTFUNC; }
sinh                            { yylval.id = ID_SINH; return LEX_FLTFUNC; }
tanh                            { yylval.id = ID_TANH; return LEX_FLTFUNC; }
sqrt                            { yylval.id = ID_SQRT; return LEX_FLTFUNC; }
pow                             return LEX_POW;
exp                             { yylval.id = ID_EXP; return LEX_FLTFUNC; }
log                             { yylval.id = ID_LOG; return LEX_FLTFUNC; }
log10                           { yylval.id = ID_LOG10; return LEX_FLTFUNC; }
ceil                            { yylval.id = ID_CEIL; return LEX_FLTFUNC; }
floor                           { yylval.id = ID_FLOOR; return LEX_FLTFUNC; }
fabs                            { yylval.id = ID_FABS; return LEX_FLTFUNC; }
fmod                            return LEX_FMOD;
rand                            { yylval.id = ID_RAND; return LEX_FLTFUNC; }
cross                           { return LEX_CROSS; }
dot                             { return LEX_DOT; }
length                          { return LEX_LEN; }
normalize                       { return LEX_NORMALIZE; }
vec3f                           { return LEX_VEC3F; }
{Digit}*"."{Digit}*{Exp}?  { yylval.value = (float)atof(yytext); return LEX_VALUE; }
{Digit}+{Exp}?             { yylval.value = (float)atof(yytext); return LEX_VALUE; }
t[a-h]                          { yylval.reg = yytext[1]; return LEX_TMP_FLT_REG; }
o[a-d]                          { yylval.reg = yytext[1]; return LEX_OUT_FLT_REG; }
[a-h]                           { yylval.reg = yytext[0]; return LEX_IN_FLT_REG; }
t[A-H]                          { yylval.reg = yytext[1]; return LEX_TMP_VEC_REG; }
o[A-D]                          { yylval.reg = yytext[1]; return LEX_OUT_VEC_REG; }
[A-H]                           { yylval.reg = yytext[0]; return LEX_IN_VEC_REG; }
.                               { return LEX_ERROR; }
%%
