// SPDX-License-Identifier: LGPL-2.1-or-later

/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#pragma once
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    FUNC = 258,                    /* FUNC  */
    ONE = 259,                     /* ONE  */
    NUM = 260,                     /* NUM  */
    IDENTIFIER = 261,              /* IDENTIFIER  */
    UNIT = 262,                    /* UNIT  */
    USUNIT = 263,                  /* USUNIT  */
    INTEGER = 264,                 /* INTEGER  */
    CONSTANT = 265,                /* CONSTANT  */
    CELLADDRESS = 266,             /* CELLADDRESS  */
    EQ = 267,                      /* EQ  */
    NEQ = 268,                     /* NEQ  */
    LT = 269,                      /* LT  */
    GT = 270,                      /* GT  */
    GTE = 271,                     /* GTE  */
    LTE = 272,                     /* LTE  */
    STRING = 273,                  /* STRING  */
    MINUSSIGN = 274,               /* MINUSSIGN  */
    PROPERTY_REF = 275,            /* PROPERTY_REF  */
    DOCUMENT = 276,                /* DOCUMENT  */
    OBJECT = 277,                  /* OBJECT  */
    EXPONENT = 278,                /* EXPONENT  */
    NUM_AND_UNIT = 279,            /* NUM_AND_UNIT  */
    NEG = 280,                     /* NEG  */
    POS = 281                      /* POS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */


extern YYSTYPE yylval;


int yyparse (void);
