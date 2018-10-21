/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_EXPRESSIONPARSER_TAB_H_INCLUDED
# define YY_YY_EXPRESSIONPARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    ONE = 258,
    NUM = 259,
    IDENTIFIER = 260,
    UNIT = 261,
    INTEGER = 262,
    CONSTANT = 263,
    CELLADDRESS = 264,
    EQ = 265,
    NEQ = 266,
    LT = 267,
    GT = 268,
    GTE = 269,
    LTE = 270,
    AND_OP = 271,
    AND_OP2 = 272,
    OR_OP = 273,
    OR_OP2 = 274,
    MUL_ASSIGN = 275,
    DIV_ASSIGN = 276,
    MOD_ASSIGN = 277,
    ADD_ASSIGN = 278,
    SUB_ASSIGN = 279,
    RSTRING = 280,
    STRING = 281,
    MINUSSIGN = 282,
    PROPERTY_REF = 283,
    DOCUMENT = 284,
    OBJECT = 285,
    EXPONENT = 286,
    EXPAND = 287,
    NEWLINE = 288,
    INDENT = 289,
    DEDENT = 290,
    IF = 291,
    ELIF = 292,
    ELSE = 293,
    WHILE = 294,
    FOR = 295,
    BREAK = 296,
    CONTINUE = 297,
    RETURN = 298,
    IN = 299,
    NUM_AND_UNIT = 300,
    NEG = 301,
    POS = 302
  };
#endif

/* Value type.  */


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_EXPRESSIONPARSER_TAB_H_INCLUDED  */
