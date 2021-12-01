%{
/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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

/* Lexer for the FreeCAD selection filter language */

/*  flex -o lex.SelectionFilter.c SelectionFilter.l */

#include <string>

#include "stdio.h"


/* This disables inclusion of unistd.h, which is not available under Visual C++
 * on Win32. The C++ scanner uses STL streams instead. */
#define YY_NO_UNISTD_H

%}

/*** Flex Declarations and Options ***/

/* change the name of the scanner class. results in "SelectionFilterFlexLexer" */
%option prefix="SelectionFilter"

/* the manual says "somewhat more optimized" */
%option batch

/* no support for include files is planned */
%option noyywrap nounput


%% /*** Filter language Part ***/


[ \t]                   ;
[\n]+                   ;
"SUBELEMENT"            return TSUB;
"SELECT"                return TSELECT;
"COUNT"                 return TCOUNT;
".."					return TSLICE;
"::"					return TNAMESPACE;

[a-zA-Z_][a-zA-Z0-9_]*  {
       yylval.string = StringFactory::New(yytext);
       return TIDENTIFIER;
      }
[0-9]+                  {
       long n = strtol (yytext, NULL, 10);
       yylval.number = n;
       return TNUMBER;
     }




