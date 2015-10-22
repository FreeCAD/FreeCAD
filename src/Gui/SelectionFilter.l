%{
/* Lexer for the FreeCAD selection filter language */
/* (c) 2010 Juergen Riegel  LGPL                    */

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
       yylval.string = new std::string(yytext);
       return TIDENTIFIER;
      }
[0-9]+                  {
       long n = strtol (yytext, NULL, 10);
       yylval.number = n;
       return TNUMBER;
     }



 
