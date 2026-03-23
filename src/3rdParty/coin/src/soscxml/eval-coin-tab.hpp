/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
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

#ifndef YY_SCXML_COIN_EVAL_COIN_TAB_HPP_INCLUDED
# define YY_SCXML_COIN_EVAL_COIN_TAB_HPP_INCLUDED
/* Enabling traces.  */
#ifndef SCXML_COIN_DEBUG
# if defined YYDEBUG
#  if YYDEBUG
#   define SCXML_COIN_DEBUG 1
#  else
#   define SCXML_COIN_DEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define SCXML_COIN_DEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined SCXML_COIN_DEBUG */
#if SCXML_COIN_DEBUG
extern int scxml_coin_debug;
#endif

/* Tokens.  */
#ifndef SCXML_COIN_TOKENTYPE
# define SCXML_COIN_TOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum scxml_coin_tokentype {
     SCXML_COIN_PAREN_OPEN = 258,
     SCXML_COIN_PAREN_CLOSE = 259,
     SCXML_COIN_EVENT_SCOPE = 260,
     SCXML_COIN_BOOL_TRUE = 261,
     SCXML_COIN_BOOL_FALSE = 262,
     SCXML_COIN_REAL = 263,
     SCXML_COIN_STRING = 264,
     SCXML_COIN_SBVALUE = 265,
     SCXML_COIN_IDENTIFIER = 266,
     SCXML_COIN_IN_FUNC = 267,
     SCXML_COIN_LENGTH_FUNC = 268,
     SCXML_COIN_OP_NOT = 269,
     SCXML_COIN_OP_AND = 270,
     SCXML_COIN_OP_OR = 271,
     SCXML_COIN_OP_EQUALS = 272,
     SCXML_COIN_OP_NOT_EQUALS = 273,
     SCXML_COIN_OP_ADD = 274,
     SCXML_COIN_OP_SUBTRACT = 275,
     SCXML_COIN_OP_MULTIPLY = 276,
     SCXML_COIN_OP_DIVIDE = 277,
     SCXML_COIN_OP_NEGATE = 278,
     SCXML_COIN_OP_APPEND = 279,
     SCXML_COIN_END = 280
   };
#endif


#if ! defined SCXML_COIN_STYPE && ! defined SCXML_COIN_STYPE_IS_DECLARED
typedef union SCXML_COIN_STYPE
{
/* Line 2058 of yacc.c  */
#line 56 "eval-coin-tab.y"

  double real;
  char * stringptr;
  ScXMLDataObj * scxmlobj;


/* Line 2058 of yacc.c  */
#line 97 "eval-coin-tab.hpp"
} SCXML_COIN_STYPE;
# define SCXML_COIN_STYPE_IS_TRIVIAL 1
# define scxml_coin_stype SCXML_COIN_STYPE /* obsolescent; will be withdrawn */
# define SCXML_COIN_STYPE_IS_DECLARED 1
#endif

extern SCXML_COIN_STYPE scxml_coin_lval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int scxml_coin_parse (void *YYPARSE_PARAM);
#else
int scxml_coin_parse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int scxml_coin_parse (void);
#else
int scxml_coin_parse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_SCXML_COIN_EVAL_COIN_TAB_HPP_INCLUDED  */
