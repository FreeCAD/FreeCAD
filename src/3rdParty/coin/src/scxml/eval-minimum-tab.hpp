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

#ifndef YY_SCXML_MINIMUM_EVAL_MINIMUM_TAB_HPP_INCLUDED
# define YY_SCXML_MINIMUM_EVAL_MINIMUM_TAB_HPP_INCLUDED
/* Enabling traces.  */
#ifndef SCXML_MINIMUM_DEBUG
# if defined YYDEBUG
#  if YYDEBUG
#   define SCXML_MINIMUM_DEBUG 1
#  else
#   define SCXML_MINIMUM_DEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define SCXML_MINIMUM_DEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined SCXML_MINIMUM_DEBUG */
#if SCXML_MINIMUM_DEBUG
extern int scxml_minimum_debug;
#endif

/* Tokens.  */
#ifndef SCXML_MINIMUM_TOKENTYPE
# define SCXML_MINIMUM_TOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum scxml_minimum_tokentype {
     SCXML_MINIMUM_PAREN_OPEN = 258,
     SCXML_MINIMUM_PAREN_CLOSE = 259,
     SCXML_MINIMUM_IDENTIFIER = 260,
     SCXML_MINIMUM_IN_FUNC = 261
   };
#endif


#if ! defined SCXML_MINIMUM_STYPE && ! defined SCXML_MINIMUM_STYPE_IS_DECLARED
typedef union SCXML_MINIMUM_STYPE
{
/* Line 2058 of yacc.c  */
#line 56 "eval-minimum-tab.y"

  char * stringptr;
  ScXMLDataObj * scxmlobj;


/* Line 2058 of yacc.c  */
#line 77 "eval-minimum-tab.hpp"
} SCXML_MINIMUM_STYPE;
# define SCXML_MINIMUM_STYPE_IS_TRIVIAL 1
# define scxml_minimum_stype SCXML_MINIMUM_STYPE /* obsolescent; will be withdrawn */
# define SCXML_MINIMUM_STYPE_IS_DECLARED 1
#endif

extern SCXML_MINIMUM_STYPE scxml_minimum_lval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int scxml_minimum_parse (void *YYPARSE_PARAM);
#else
int scxml_minimum_parse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int scxml_minimum_parse (void);
#else
int scxml_minimum_parse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_SCXML_MINIMUM_EVAL_MINIMUM_TAB_HPP_INCLUDED  */
