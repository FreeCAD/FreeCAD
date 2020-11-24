%{
/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

/* Lexer for the FreeCAD  Units language   */

/* use this file to generate the file 'QuantityLexer.c' using the program flex
 * the command for this operation is:
 * flex --outfile=QuantityLexer.c QuantityParser.l
 * (flex for Windows is available here:
 *  https://sourceforge.net/projects/winflexbison/
 *  (you must then change 'flex' to 'win_flex' in the command)) */

/* This disables inclusion of unistd.h, which is not available under Visual C++
 * on Win32. The C++ scanner uses STL streams instead. */
#define YY_NO_UNISTD_H

%}

/*** Flex Declarations and Options ***/


/* the manual says "somewhat more optimized" */
%option batch
%option never-interactive


/* no support for include files is planned */
%option noyywrap nounput

DIGIT    [0-9]
ID       [a-z][a-z0-9]*
EXPO     [eE][-+]?[0-9]+
DGRP     '\.'[0-9][0-9][0-9]
CGRP     '\,'[0-9][0-9][0-9]

%x C_COMMENT

%% /*** Filter language Part ***/

"\["            { BEGIN(C_COMMENT); }
<C_COMMENT>"\]" { BEGIN(INITIAL); }
<C_COMMENT>.    { ;}


[ \t]       ;
[\n]+       ;

[+()=/*^]  { return *yytext; }

"-"                                                return MINUSSIGN;
"\xe2\x88\x92"                                     return MINUSSIGN;

"1"        yylval = Quantity(1.0);                 return ONE;
{DIGIT}+"."?{DIGIT}*{EXPO}?                         {  yylval = Quantity(num_change(yytext,'.',','));return NUM;  }
"."?{DIGIT}+{EXPO}?                                 {  yylval = Quantity(num_change(yytext,'.',','));return NUM;  }
{DIGIT}+","?{DIGIT}*{EXPO}?                         {  yylval = Quantity(num_change(yytext,',','.'));return NUM;  }
","?{DIGIT}+{EXPO}?                                 {  yylval = Quantity(num_change(yytext,',','.'));return NUM;  }


"pi"                   {yylval = Quantity(M_PI)          ; return NUM;} // constant pi
"e"                    {yylval = Quantity(M_E)           ; return NUM;} // constant e

"acos"                 return ACOS;
"asin"                 return ASIN;
"atan"                 return ATAN;
"atan2"                return ATAN2;
"cos"                  return COS;
"exp"                  return EXP;
"abs"                  return ABS;
"mod"                  return MOD;
"log"                  return LOG;
"log10"                return LOG10;
"pow"                  return POW;
"sin"                  return SIN;
"sinh"                 return SINH;
"tan"                  return TAN;
"tanh"                 return TANH;
"sqrt"                 return SQRT;

'|\"|[^[:space:][:digit:][:punct:]]+   { if(!Quantity::fromUnitString(yylval, yytext)) {return *yytext;} return UNIT;}

.                      return *yytext;
