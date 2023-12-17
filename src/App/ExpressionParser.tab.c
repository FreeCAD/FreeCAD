/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 28 "ExpressionParser.y"


#define YYSTYPE App::ExpressionParser::semantic_type

std::stack<FunctionExpression::Function> functions;                /**< Function identifier */

       //#define YYSTYPE yystype
       #define yyparse ExpressionParser_yyparse
       #define yyerror ExpressionParser_yyerror

#line 82 "ExpressionParser.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "ExpressionParser.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_FUNC = 3,                       /* FUNC  */
  YYSYMBOL_ONE = 4,                        /* ONE  */
  YYSYMBOL_NUM = 5,                        /* NUM  */
  YYSYMBOL_IDENTIFIER = 6,                 /* IDENTIFIER  */
  YYSYMBOL_UNIT = 7,                       /* UNIT  */
  YYSYMBOL_UNITINCH = 8,                   /* UNITINCH  */
  YYSYMBOL_UNITFOOT = 9,                   /* UNITFOOT  */
  YYSYMBOL_UNITDEG = 10,                   /* UNITDEG  */
  YYSYMBOL_UNITARCMIN = 11,                /* UNITARCMIN  */
  YYSYMBOL_UNITARCSEC = 12,                /* UNITARCSEC  */
  YYSYMBOL_INTEGER = 13,                   /* INTEGER  */
  YYSYMBOL_CONSTANT = 14,                  /* CONSTANT  */
  YYSYMBOL_CELLADDRESS = 15,               /* CELLADDRESS  */
  YYSYMBOL_EQ = 16,                        /* EQ  */
  YYSYMBOL_NEQ = 17,                       /* NEQ  */
  YYSYMBOL_LT = 18,                        /* LT  */
  YYSYMBOL_GT = 19,                        /* GT  */
  YYSYMBOL_GTE = 20,                       /* GTE  */
  YYSYMBOL_LTE = 21,                       /* LTE  */
  YYSYMBOL_STRING = 22,                    /* STRING  */
  YYSYMBOL_MINUSSIGN = 23,                 /* MINUSSIGN  */
  YYSYMBOL_PROPERTY_REF = 24,              /* PROPERTY_REF  */
  YYSYMBOL_DOCUMENT = 25,                  /* DOCUMENT  */
  YYSYMBOL_OBJECT = 26,                    /* OBJECT  */
  YYSYMBOL_EXPONENT = 27,                  /* EXPONENT  */
  YYSYMBOL_28_ = 28,                       /* ':'  */
  YYSYMBOL_29_ = 29,                       /* '+'  */
  YYSYMBOL_30_ = 30,                       /* '*'  */
  YYSYMBOL_31_ = 31,                       /* '/'  */
  YYSYMBOL_32_ = 32,                       /* '%'  */
  YYSYMBOL_NUM_AND_UNIT = 33,              /* NUM_AND_UNIT  */
  YYSYMBOL_34_ = 34,                       /* '^'  */
  YYSYMBOL_NEG = 35,                       /* NEG  */
  YYSYMBOL_POS = 36,                       /* POS  */
  YYSYMBOL_37_ = 37,                       /* ')'  */
  YYSYMBOL_38_ = 38,                       /* '?'  */
  YYSYMBOL_39_ = 39,                       /* '('  */
  YYSYMBOL_40_ = 40,                       /* ','  */
  YYSYMBOL_41_ = 41,                       /* ';'  */
  YYSYMBOL_42_ = 42,                       /* '.'  */
  YYSYMBOL_43_ = 43,                       /* '#'  */
  YYSYMBOL_44_ = 44,                       /* '['  */
  YYSYMBOL_45_ = 45,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 46,                  /* $accept  */
  YYSYMBOL_input = 47,                     /* input  */
  YYSYMBOL_unit_num = 48,                  /* unit_num  */
  YYSYMBOL_exp = 49,                       /* exp  */
  YYSYMBOL_num = 50,                       /* num  */
  YYSYMBOL_args = 51,                      /* args  */
  YYSYMBOL_range = 52,                     /* range  */
  YYSYMBOL_cond = 53,                      /* cond  */
  YYSYMBOL_unit_inch = 54,                 /* unit_inch  */
  YYSYMBOL_unit_foot = 55,                 /* unit_foot  */
  YYSYMBOL_unit_degree = 56,               /* unit_degree  */
  YYSYMBOL_unit_arcminute = 57,            /* unit_arcminute  */
  YYSYMBOL_unit_arcsecond = 58,            /* unit_arcsecond  */
  YYSYMBOL_unit_other = 59,                /* unit_other  */
  YYSYMBOL_unit_exp = 60,                  /* unit_exp  */
  YYSYMBOL_integer = 61,                   /* integer  */
  YYSYMBOL_id_or_cell = 62,                /* id_or_cell  */
  YYSYMBOL_identifier = 63,                /* identifier  */
  YYSYMBOL_iden = 64,                      /* iden  */
  YYSYMBOL_indexer = 65,                   /* indexer  */
  YYSYMBOL_indexable = 66,                 /* indexable  */
  YYSYMBOL_document = 67,                  /* document  */
  YYSYMBOL_object = 68                     /* object  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  50
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   455

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  46
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  23
/* YYNRULES -- Number of rules.  */
#define YYNRULES  90
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  167

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   285


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    43,     2,    32,     2,     2,
      39,    37,    30,    29,    40,     2,    42,    31,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    28,    41,
       2,     2,     2,    38,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    44,     2,    45,    34,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    33,    35,    36
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    85,    85,    86,    89,    91,    93,    95,    97,    98,
      99,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   121,   122,
     123,   124,   126,   127,   128,   129,   130,   131,   134,   137,
     138,   139,   140,   141,   142,   143,   146,   147,   148,   149,
     150,   151,   153,   154,   155,   156,   157,   158,   159,   160,
     161,   162,   163,   166,   167,   171,   172,   176,   177,   181,
     186,   191,   197,   204,   211,   218,   222,   223,   224,   225,
     226,   227,   228,   229,   233,   234,   235,   239,   240,   244,
     245
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "FUNC", "ONE", "NUM",
  "IDENTIFIER", "UNIT", "UNITINCH", "UNITFOOT", "UNITDEG", "UNITARCMIN",
  "UNITARCSEC", "INTEGER", "CONSTANT", "CELLADDRESS", "EQ", "NEQ", "LT",
  "GT", "GTE", "LTE", "STRING", "MINUSSIGN", "PROPERTY_REF", "DOCUMENT",
  "OBJECT", "EXPONENT", "':'", "'+'", "'*'", "'/'", "'%'", "NUM_AND_UNIT",
  "'^'", "NEG", "POS", "')'", "'?'", "'('", "','", "';'", "'.'", "'#'",
  "'['", "']'", "$accept", "input", "unit_num", "exp", "num", "args",
  "range", "cond", "unit_inch", "unit_foot", "unit_degree",
  "unit_arcminute", "unit_arcsecond", "unit_other", "unit_exp", "integer",
  "id_or_cell", "identifier", "iden", "indexer", "indexable", "document",
  "object", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-99)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-91)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     167,   209,   -99,   -99,   -41,   -99,   -99,   -99,   -99,   -99,
     -99,   -99,   -99,   -99,   -30,   209,   209,   167,    99,    19,
     -99,   411,   232,   -24,   -99,   -99,   -99,   -99,   -99,   -99,
     174,    -6,   -15,    -4,   -36,     4,    11,   209,   411,    52,
     -99,   -27,   -99,   -99,   370,    14,   127,   -99,    32,   -99,
     -99,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     167,   209,   209,    92,    42,    82,    82,    41,   209,    92,
      92,    31,    17,   -99,    88,   101,   -99,   100,   102,   -99,
     209,   209,     1,   -99,   -99,   -99,     1,   411,   411,   411,
     411,   411,   411,   153,   153,    77,    77,    41,    77,   -99,
      82,    29,   105,   392,    41,    41,   -99,   -99,     5,   -99,
     188,   236,   -99,   -99,   -99,   -99,    71,    76,   -99,   411,
     -99,   411,   -99,   -99,   -99,   117,    82,   103,   -99,    82,
     209,   -99,   209,   256,   124,   -99,   104,     1,   -99,   117,
      82,   120,   421,   294,   209,   -99,   209,   -99,   275,    91,
     -99,   -99,   -99,   117,   -99,   -99,   313,   332,   209,   -99,
       1,   -99,   -99,   -99,   351,   -99,   -99
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,    28,    29,    65,    51,    46,    47,    48,    49,
      50,    30,    31,    66,    13,     0,     0,     0,     0,     0,
      12,     2,    11,     0,    53,    54,    55,    56,    57,    52,
       3,    67,    14,    68,    24,     0,     0,     0,    32,     0,
      33,    67,    15,    16,     0,     0,     0,    65,     0,    70,
       1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    54,    55,     4,     0,     0,
       0,     0,     0,    84,     0,     0,    85,     0,     0,    25,
       0,     0,     0,    27,    45,    62,     0,    39,    40,    41,
      42,    43,    44,    18,    17,    19,    20,    22,    21,    23,
       0,     0,     0,     0,    59,    58,    64,    63,     0,    60,
       0,     0,    75,    86,    89,    90,     0,     0,    72,    34,
      36,    35,    37,    38,    69,     0,     0,     0,     7,    10,
       0,    61,     0,     0,     0,    76,     0,     0,     8,     0,
       0,     0,    26,     0,     0,    78,     0,    77,     0,     0,
      73,    71,     6,     0,     9,    79,     0,     0,     0,    80,
       0,     5,    82,    81,     0,    74,    83
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -99,   -99,   -99,    10,   -17,   -99,   -23,   -13,   -98,   119,
     121,    43,     7,   -99,    28,    47,    -1,   -99,   -99,   115,
     -99,   -99,    73
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,    19,    20,    44,    22,    39,    40,    23,    24,    25,
      26,    27,    28,    29,    46,   109,    31,    32,    33,    73,
      34,    35,    36
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      41,    82,   -88,   128,    45,    64,    75,    47,    72,   106,
      21,    38,   -89,   -87,    68,   -90,    13,    49,   107,    50,
       1,     2,     3,     4,    45,    42,    43,   138,    30,    72,
      11,    12,    13,     2,     3,   106,   -90,     6,    74,    14,
      15,   152,    11,    12,   107,   110,    16,    77,   101,   102,
      67,    84,    68,    78,   108,   161,    37,   120,   122,    18,
     126,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    98,    99,   100,    86,    71,   115,   118,   103,    41,
      41,   123,   111,   125,   127,   124,     2,     3,    97,    79,
     119,   121,    80,    81,   112,    11,    12,   104,   105,     5,
       6,     7,     8,     9,    10,    47,    47,   113,    47,   139,
      47,    62,   141,   136,    13,    13,     9,    13,   137,    13,
     133,    48,   114,   153,   117,     6,   149,     1,     2,     3,
       4,    63,    10,   160,   140,   150,   151,    11,    12,    13,
     142,    65,   143,    66,   148,   129,    14,    15,   154,    76,
     116,     0,   146,    16,   156,   131,   157,    69,    70,   165,
       0,    71,     0,    37,    85,     0,    18,     0,   164,   147,
       1,     2,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    59,    60,    61,     0,    62,     0,    14,
      15,     1,     2,     3,     4,     0,    16,     0,     0,     0,
       0,    11,    12,    13,    69,    70,    17,     0,    71,    18,
      14,    15,     1,     2,     3,     4,   132,    16,     0,     0,
       0,     0,    11,    12,    13,     0,     0,    37,     0,     0,
      18,    14,    15,     0,     0,     0,     2,     3,    16,     5,
       6,     7,     8,     9,    10,    11,    12,     0,    37,     0,
       0,    18,    51,    52,    53,    54,    55,    56,     0,    57,
       0,     0,     0,     0,   134,    58,    59,    60,    61,     0,
      62,    63,    51,    52,    53,    54,    55,    56,     0,    57,
       0,   135,     0,     0,   144,    58,    59,    60,    61,     0,
      62,    51,    52,    53,    54,    55,    56,     0,    57,     0,
       0,   145,     0,   158,    58,    59,    60,    61,     0,    62,
      51,    52,    53,    54,    55,    56,     0,    57,     0,     0,
     159,     0,     0,    58,    59,    60,    61,     0,    62,    51,
      52,    53,    54,    55,    56,     0,    57,     0,     0,   155,
       0,     0,    58,    59,    60,    61,     0,    62,    51,    52,
      53,    54,    55,    56,     0,    57,     0,     0,   162,     0,
       0,    58,    59,    60,    61,     0,    62,    51,    52,    53,
      54,    55,    56,     0,    57,     0,     0,   163,     0,     0,
      58,    59,    60,    61,     0,    62,    51,    52,    53,    54,
      55,    56,     0,    57,     0,     0,   166,     0,     0,    58,
      59,    60,    61,     0,    62,     0,     0,    83,    51,    52,
      53,    54,    55,    56,     0,    57,     0,     0,     0,     0,
     130,    58,    59,    60,    61,     0,    62,    51,    52,    53,
      54,    55,    56,     0,    57,     0,     0,     0,     0,     0,
      58,    59,    60,    61,    57,    62,     0,     0,     0,     0,
      58,    59,    60,    61,     0,    62
};

static const yytype_int16 yycheck[] =
{
       1,    28,    43,   101,    17,    22,    42,     6,    44,     4,
       0,     1,    42,    43,    38,    42,    15,    18,    13,     0,
       3,     4,     5,     6,    37,    15,    16,   125,     0,    44,
      13,    14,    15,     4,     5,     4,    42,     8,    42,    22,
      23,   139,    13,    14,    13,    28,    29,    43,    65,    66,
      22,    37,    38,    42,    23,   153,    39,    80,    81,    42,
      31,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    31,    42,    34,    77,    78,    68,    80,
      81,    82,    72,   100,   101,    86,     4,     5,    60,    37,
      80,    81,    40,    41,     6,    13,    14,    69,    70,     7,
       8,     9,    10,    11,    12,     6,     6,     6,     6,   126,
       6,    34,   129,    42,    15,    15,    11,    15,    42,    15,
     110,    22,    22,   140,    22,     8,    22,     3,     4,     5,
       6,    39,    12,    42,    31,   136,   137,    13,    14,    15,
     130,    22,   132,    22,   134,   102,    22,    23,   141,    34,
      77,    -1,    28,    29,   144,   108,   146,    30,    31,   160,
      -1,    34,    -1,    39,    37,    -1,    42,    -1,   158,    45,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    30,    31,    32,    -1,    34,    -1,    22,
      23,     3,     4,     5,     6,    -1,    29,    -1,    -1,    -1,
      -1,    13,    14,    15,    30,    31,    39,    -1,    34,    42,
      22,    23,     3,     4,     5,     6,    28,    29,    -1,    -1,
      -1,    -1,    13,    14,    15,    -1,    -1,    39,    -1,    -1,
      42,    22,    23,    -1,    -1,    -1,     4,     5,    29,     7,
       8,     9,    10,    11,    12,    13,    14,    -1,    39,    -1,
      -1,    42,    16,    17,    18,    19,    20,    21,    -1,    23,
      -1,    -1,    -1,    -1,    28,    29,    30,    31,    32,    -1,
      34,    39,    16,    17,    18,    19,    20,    21,    -1,    23,
      -1,    45,    -1,    -1,    28,    29,    30,    31,    32,    -1,
      34,    16,    17,    18,    19,    20,    21,    -1,    23,    -1,
      -1,    45,    -1,    28,    29,    30,    31,    32,    -1,    34,
      16,    17,    18,    19,    20,    21,    -1,    23,    -1,    -1,
      45,    -1,    -1,    29,    30,    31,    32,    -1,    34,    16,
      17,    18,    19,    20,    21,    -1,    23,    -1,    -1,    45,
      -1,    -1,    29,    30,    31,    32,    -1,    34,    16,    17,
      18,    19,    20,    21,    -1,    23,    -1,    -1,    45,    -1,
      -1,    29,    30,    31,    32,    -1,    34,    16,    17,    18,
      19,    20,    21,    -1,    23,    -1,    -1,    45,    -1,    -1,
      29,    30,    31,    32,    -1,    34,    16,    17,    18,    19,
      20,    21,    -1,    23,    -1,    -1,    45,    -1,    -1,    29,
      30,    31,    32,    -1,    34,    -1,    -1,    37,    16,    17,
      18,    19,    20,    21,    -1,    23,    -1,    -1,    -1,    -1,
      28,    29,    30,    31,    32,    -1,    34,    16,    17,    18,
      19,    20,    21,    -1,    23,    -1,    -1,    -1,    -1,    -1,
      29,    30,    31,    32,    23,    34,    -1,    -1,    -1,    -1,
      29,    30,    31,    32,    -1,    34
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    22,    23,    29,    39,    42,    47,
      48,    49,    50,    53,    54,    55,    56,    57,    58,    59,
      60,    62,    63,    64,    66,    67,    68,    39,    49,    51,
      52,    62,    49,    49,    49,    53,    60,     6,    22,    62,
       0,    16,    17,    18,    19,    20,    21,    23,    29,    30,
      31,    32,    34,    39,    50,    55,    56,    60,    38,    30,
      31,    34,    44,    65,    42,    42,    65,    43,    42,    37,
      40,    41,    28,    37,    37,    37,    42,    49,    49,    49,
      49,    49,    49,    49,    49,    49,    49,    60,    49,    49,
      31,    50,    50,    49,    60,    60,     4,    13,    23,    61,
      28,    49,     6,     6,    22,    62,    68,    22,    62,    49,
      52,    49,    52,    62,    62,    50,    31,    50,    54,    57,
      28,    61,    28,    49,    28,    45,    42,    42,    54,    50,
      31,    50,    49,    49,    28,    45,    28,    45,    49,    22,
      62,    62,    54,    50,    58,    45,    49,    49,    28,    45,
      42,    54,    45,    45,    49,    62,    45
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    46,    47,    47,    48,    48,    48,    48,    48,    48,
      48,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      49,    49,    49,    49,    49,    49,    49,    49,    50,    50,
      50,    50,    51,    51,    51,    51,    51,    51,    52,    53,
      53,    53,    53,    53,    53,    53,    54,    55,    56,    57,
      58,    59,    60,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    60,    61,    61,    62,    62,    63,    63,    64,
      64,    64,    64,    64,    64,    64,    65,    65,    65,    65,
      65,    65,    65,    65,    66,    66,    66,    67,    67,    68,
      68
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     2,     7,     6,     4,     5,     6,
       4,     1,     1,     1,     1,     2,     2,     3,     3,     3,
       3,     3,     3,     3,     1,     3,     5,     3,     1,     1,
       1,     1,     1,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     3,
       3,     4,     3,     1,     1,     1,     1,     1,     1,     4,
       2,     5,     3,     5,     7,     3,     3,     4,     4,     5,
       5,     6,     6,     7,     2,     2,     3,     1,     1,     1,
       1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_exp: /* exp  */
#line 77 "ExpressionParser.y"
            { delete ((*yyvaluep).expr); }
#line 1044 "ExpressionParser.tab.c"
        break;

    case YYSYMBOL_num: /* num  */
#line 77 "ExpressionParser.y"
            { delete ((*yyvaluep).expr); }
#line 1050 "ExpressionParser.tab.c"
        break;

    case YYSYMBOL_args: /* args  */
#line 79 "ExpressionParser.y"
            { std::vector<Expression*>::const_iterator i = ((*yyvaluep).arguments).begin(); while (i != ((*yyvaluep).arguments).end()) { delete *i; ++i; } }
#line 1056 "ExpressionParser.tab.c"
        break;

    case YYSYMBOL_range: /* range  */
#line 77 "ExpressionParser.y"
            { delete ((*yyvaluep).expr); }
#line 1062 "ExpressionParser.tab.c"
        break;

    case YYSYMBOL_cond: /* cond  */
#line 77 "ExpressionParser.y"
            { delete ((*yyvaluep).expr); }
#line 1068 "ExpressionParser.tab.c"
        break;

    case YYSYMBOL_unit_exp: /* unit_exp  */
#line 77 "ExpressionParser.y"
            { delete ((*yyvaluep).expr); }
#line 1074 "ExpressionParser.tab.c"
        break;

    case YYSYMBOL_indexer: /* indexer  */
#line 78 "ExpressionParser.y"
            { delete ((*yyvaluep).component); }
#line 1080 "ExpressionParser.tab.c"
        break;

    case YYSYMBOL_indexable: /* indexable  */
#line 77 "ExpressionParser.y"
            { delete ((*yyvaluep).expr); }
#line 1086 "ExpressionParser.tab.c"
        break;

      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* input: exp  */
#line 85 "ExpressionParser.y"
                                                { ScanResult = (yyvsp[0].expr); valueExpression = true;                                        }
#line 1356 "ExpressionParser.tab.c"
    break;

  case 3: /* input: unit_exp  */
#line 86 "ExpressionParser.y"
                                                { ScanResult = (yyvsp[0].expr); unitExpression = true;                                         }
#line 1362 "ExpressionParser.tab.c"
    break;

  case 4: /* unit_num: num unit_exp  */
#line 89 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-1].expr), OperatorExpression::UNIT, (yyvsp[0].expr));  }
#line 1368 "ExpressionParser.tab.c"
    break;

  case 5: /* unit_num: num unit_foot num num '/' num unit_inch  */
#line 91 "ExpressionParser.y"
                                                                           { (yyval.expr) = new ImperialDistance(DocumentObject, (yyvsp[-6].expr), (yyvsp[-4].expr), (yyvsp[-3].expr), (yyvsp[-1].expr));  }
#line 1374 "ExpressionParser.tab.c"
    break;

  case 6: /* unit_num: num unit_foot num '/' num unit_inch  */
#line 93 "ExpressionParser.y"
                                                                       { (yyval.expr) = new ImperialDistance(DocumentObject, (yyvsp[-5].expr), new NumberExpression(DocumentObject, Quantity(0)), (yyvsp[-3].expr), (yyvsp[-1].expr));  }
#line 1380 "ExpressionParser.tab.c"
    break;

  case 7: /* unit_num: num unit_foot num unit_inch  */
#line 95 "ExpressionParser.y"
                                                               { (yyval.expr) = new ImperialDistance(DocumentObject, (yyvsp[-3].expr), (yyvsp[-1].expr), new NumberExpression(DocumentObject, Quantity(0)), new NumberExpression(DocumentObject, Quantity(1)));  }
#line 1386 "ExpressionParser.tab.c"
    break;

  case 8: /* unit_num: num num '/' num unit_inch  */
#line 97 "ExpressionParser.y"
                                                             { (yyval.expr) = new ImperialDistance(DocumentObject, new NumberExpression(DocumentObject, Quantity(0)), (yyvsp[-4].expr), (yyvsp[-3].expr), (yyvsp[-1].expr));  }
#line 1392 "ExpressionParser.tab.c"
    break;

  case 9: /* unit_num: num unit_degree num unit_arcminute num unit_arcsecond  */
#line 98 "ExpressionParser.y"
                                                                                     { (yyval.expr) = new ImperialAngle(DocumentObject, (yyvsp[-5].expr), (yyvsp[-3].expr), (yyvsp[-1].expr));}
#line 1398 "ExpressionParser.tab.c"
    break;

  case 10: /* unit_num: num unit_degree num unit_arcminute  */
#line 99 "ExpressionParser.y"
                                                                  { (yyval.expr) = new ImperialAngle(DocumentObject, (yyvsp[-3].expr), (yyvsp[-1].expr), new NumberExpression(DocumentObject, Quantity(0)));}
#line 1404 "ExpressionParser.tab.c"
    break;

  case 11: /* exp: num  */
#line 102 "ExpressionParser.y"
                                                { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1410 "ExpressionParser.tab.c"
    break;

  case 12: /* exp: unit_num  */
#line 103 "ExpressionParser.y"
                                                { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1416 "ExpressionParser.tab.c"
    break;

  case 13: /* exp: STRING  */
#line 104 "ExpressionParser.y"
                                                { (yyval.expr) = new StringExpression(DocumentObject, (yyvsp[0].string));                                  }
#line 1422 "ExpressionParser.tab.c"
    break;

  case 14: /* exp: identifier  */
#line 105 "ExpressionParser.y"
                                                { (yyval.expr) = new VariableExpression(DocumentObject, (yyvsp[0].path));                                }
#line 1428 "ExpressionParser.tab.c"
    break;

  case 15: /* exp: MINUSSIGN exp  */
#line 106 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))); }
#line 1434 "ExpressionParser.tab.c"
    break;

  case 16: /* exp: '+' exp  */
#line 107 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::POS, new NumberExpression(DocumentObject, Quantity(1))); }
#line 1440 "ExpressionParser.tab.c"
    break;

  case 17: /* exp: exp '+' exp  */
#line 108 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::ADD, (yyvsp[0].expr));   }
#line 1446 "ExpressionParser.tab.c"
    break;

  case 18: /* exp: exp MINUSSIGN exp  */
#line 109 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::SUB, (yyvsp[0].expr));   }
#line 1452 "ExpressionParser.tab.c"
    break;

  case 19: /* exp: exp '*' exp  */
#line 110 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 1458 "ExpressionParser.tab.c"
    break;

  case 20: /* exp: exp '/' exp  */
#line 111 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1464 "ExpressionParser.tab.c"
    break;

  case 21: /* exp: exp '%' exp  */
#line 112 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MOD, (yyvsp[0].expr));   }
#line 1470 "ExpressionParser.tab.c"
    break;

  case 22: /* exp: exp '/' unit_exp  */
#line 113 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1476 "ExpressionParser.tab.c"
    break;

  case 23: /* exp: exp '^' exp  */
#line 114 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, (yyvsp[0].expr));   }
#line 1482 "ExpressionParser.tab.c"
    break;

  case 24: /* exp: indexable  */
#line 115 "ExpressionParser.y"
                                                { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1488 "ExpressionParser.tab.c"
    break;

  case 25: /* exp: FUNC args ')'  */
#line 116 "ExpressionParser.y"
                                                { (yyval.expr) = new FunctionExpression(DocumentObject, (yyvsp[-2].func).first, std::move((yyvsp[-2].func).second), (yyvsp[-1].arguments));}
#line 1494 "ExpressionParser.tab.c"
    break;

  case 26: /* exp: cond '?' exp ':' exp  */
#line 117 "ExpressionParser.y"
                                                { (yyval.expr) = new ConditionalExpression(DocumentObject, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr));                     }
#line 1500 "ExpressionParser.tab.c"
    break;

  case 27: /* exp: '(' exp ')'  */
#line 118 "ExpressionParser.y"
                                                { (yyval.expr) = (yyvsp[-1].expr); }
#line 1506 "ExpressionParser.tab.c"
    break;

  case 28: /* num: ONE  */
#line 121 "ExpressionParser.y"
                                                { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 1512 "ExpressionParser.tab.c"
    break;

  case 29: /* num: NUM  */
#line 122 "ExpressionParser.y"
                                                { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 1518 "ExpressionParser.tab.c"
    break;

  case 30: /* num: INTEGER  */
#line 123 "ExpressionParser.y"
                                                { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue)));                }
#line 1524 "ExpressionParser.tab.c"
    break;

  case 31: /* num: CONSTANT  */
#line 124 "ExpressionParser.y"
                                                { (yyval.expr) = new ConstantExpression(DocumentObject, (yyvsp[0].constant).name, Quantity((yyvsp[0].constant).fvalue));      }
#line 1530 "ExpressionParser.tab.c"
    break;

  case 32: /* args: exp  */
#line 126 "ExpressionParser.y"
                                                { (yyval.arguments).push_back((yyvsp[0].expr));                                                               }
#line 1536 "ExpressionParser.tab.c"
    break;

  case 33: /* args: range  */
#line 127 "ExpressionParser.y"
                                                { (yyval.arguments).push_back((yyvsp[0].expr));                                                               }
#line 1542 "ExpressionParser.tab.c"
    break;

  case 34: /* args: args ',' exp  */
#line 128 "ExpressionParser.y"
                                                { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1548 "ExpressionParser.tab.c"
    break;

  case 35: /* args: args ';' exp  */
#line 129 "ExpressionParser.y"
                                                { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1554 "ExpressionParser.tab.c"
    break;

  case 36: /* args: args ',' range  */
#line 130 "ExpressionParser.y"
                                                { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1560 "ExpressionParser.tab.c"
    break;

  case 37: /* args: args ';' range  */
#line 131 "ExpressionParser.y"
                                                { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1566 "ExpressionParser.tab.c"
    break;

  case 38: /* range: id_or_cell ':' id_or_cell  */
#line 134 "ExpressionParser.y"
                                                { (yyval.expr) = new RangeExpression(DocumentObject, (yyvsp[-2].string), (yyvsp[0].string));                               }
#line 1572 "ExpressionParser.tab.c"
    break;

  case 39: /* cond: exp EQ exp  */
#line 137 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::EQ, (yyvsp[0].expr));    }
#line 1578 "ExpressionParser.tab.c"
    break;

  case 40: /* cond: exp NEQ exp  */
#line 138 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::NEQ, (yyvsp[0].expr));   }
#line 1584 "ExpressionParser.tab.c"
    break;

  case 41: /* cond: exp LT exp  */
#line 139 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LT, (yyvsp[0].expr));    }
#line 1590 "ExpressionParser.tab.c"
    break;

  case 42: /* cond: exp GT exp  */
#line 140 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GT, (yyvsp[0].expr));    }
#line 1596 "ExpressionParser.tab.c"
    break;

  case 43: /* cond: exp GTE exp  */
#line 141 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GTE, (yyvsp[0].expr));   }
#line 1602 "ExpressionParser.tab.c"
    break;

  case 44: /* cond: exp LTE exp  */
#line 142 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LTE, (yyvsp[0].expr));   }
#line 1608 "ExpressionParser.tab.c"
    break;

  case 45: /* cond: '(' cond ')'  */
#line 143 "ExpressionParser.y"
                                                { (yyval.expr) = (yyvsp[-1].expr); }
#line 1614 "ExpressionParser.tab.c"
    break;

  case 46: /* unit_inch: UNITINCH  */
#line 146 "ExpressionParser.y"
                                                { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 1620 "ExpressionParser.tab.c"
    break;

  case 47: /* unit_foot: UNITFOOT  */
#line 147 "ExpressionParser.y"
                                                { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 1626 "ExpressionParser.tab.c"
    break;

  case 48: /* unit_degree: UNITDEG  */
#line 148 "ExpressionParser.y"
                                                { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 1632 "ExpressionParser.tab.c"
    break;

  case 49: /* unit_arcminute: UNITARCMIN  */
#line 149 "ExpressionParser.y"
                                                { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 1638 "ExpressionParser.tab.c"
    break;

  case 50: /* unit_arcsecond: UNITARCSEC  */
#line 150 "ExpressionParser.y"
                                                { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 1644 "ExpressionParser.tab.c"
    break;

  case 51: /* unit_other: UNIT  */
#line 151 "ExpressionParser.y"
                                                { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 1650 "ExpressionParser.tab.c"
    break;

  case 52: /* unit_exp: unit_other  */
#line 153 "ExpressionParser.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 1656 "ExpressionParser.tab.c"
    break;

  case 53: /* unit_exp: unit_inch  */
#line 154 "ExpressionParser.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 1662 "ExpressionParser.tab.c"
    break;

  case 54: /* unit_exp: unit_foot  */
#line 155 "ExpressionParser.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 1668 "ExpressionParser.tab.c"
    break;

  case 55: /* unit_exp: unit_degree  */
#line 156 "ExpressionParser.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 1674 "ExpressionParser.tab.c"
    break;

  case 56: /* unit_exp: unit_arcminute  */
#line 157 "ExpressionParser.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 1680 "ExpressionParser.tab.c"
    break;

  case 57: /* unit_exp: unit_arcsecond  */
#line 158 "ExpressionParser.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 1686 "ExpressionParser.tab.c"
    break;

  case 58: /* unit_exp: unit_exp '/' unit_exp  */
#line 159 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1692 "ExpressionParser.tab.c"
    break;

  case 59: /* unit_exp: unit_exp '*' unit_exp  */
#line 160 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 1698 "ExpressionParser.tab.c"
    break;

  case 60: /* unit_exp: unit_exp '^' integer  */
#line 161 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))));   }
#line 1704 "ExpressionParser.tab.c"
    break;

  case 61: /* unit_exp: unit_exp '^' MINUSSIGN integer  */
#line 162 "ExpressionParser.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-3].expr), OperatorExpression::POW, new OperatorExpression(DocumentObject, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))));   }
#line 1710 "ExpressionParser.tab.c"
    break;

  case 62: /* unit_exp: '(' unit_exp ')'  */
#line 163 "ExpressionParser.y"
                                                { (yyval.expr) = (yyvsp[-1].expr);                                                                        }
#line 1716 "ExpressionParser.tab.c"
    break;

  case 63: /* integer: INTEGER  */
#line 166 "ExpressionParser.y"
                 { (yyval.ivalue) = (yyvsp[0].ivalue); }
#line 1722 "ExpressionParser.tab.c"
    break;

  case 64: /* integer: ONE  */
#line 167 "ExpressionParser.y"
             { (yyval.ivalue) = (yyvsp[0].fvalue); }
#line 1728 "ExpressionParser.tab.c"
    break;

  case 65: /* id_or_cell: IDENTIFIER  */
#line 171 "ExpressionParser.y"
                                            { (yyval.string) = std::move((yyvsp[0].string)); }
#line 1734 "ExpressionParser.tab.c"
    break;

  case 66: /* id_or_cell: CELLADDRESS  */
#line 172 "ExpressionParser.y"
                                            { (yyval.string) = std::move((yyvsp[0].string)); }
#line 1740 "ExpressionParser.tab.c"
    break;

  case 67: /* identifier: id_or_cell  */
#line 176 "ExpressionParser.y"
                                            { (yyval.path) = ObjectIdentifier(DocumentObject); (yyval.path) << ObjectIdentifier::SimpleComponent((yyvsp[0].string)); }
#line 1746 "ExpressionParser.tab.c"
    break;

  case 68: /* identifier: iden  */
#line 177 "ExpressionParser.y"
                                            { (yyval.path) = std::move((yyvsp[0].path)); }
#line 1752 "ExpressionParser.tab.c"
    break;

  case 69: /* iden: '.' STRING '.' id_or_cell  */
#line 181 "ExpressionParser.y"
                                            { /* Path to property of a sub-object of the current object*/
                                                (yyval.path) = ObjectIdentifier(DocumentObject,true);
                                                (yyval.path).setDocumentObjectName(DocumentObject,false,ObjectIdentifier::String(std::move((yyvsp[-2].string)),true),true);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                            }
#line 1762 "ExpressionParser.tab.c"
    break;

  case 70: /* iden: '.' id_or_cell  */
#line 186 "ExpressionParser.y"
                                            { /* Path to property of the current document object */
                                                (yyval.path) = ObjectIdentifier(DocumentObject,true);
                                                (yyval.path).setDocumentObjectName(DocumentObject);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                            }
#line 1772 "ExpressionParser.tab.c"
    break;

  case 71: /* iden: object '.' STRING '.' id_or_cell  */
#line 191 "ExpressionParser.y"
                                            { /* Path to property of a sub-object */
                                                (yyval.path) = ObjectIdentifier(DocumentObject);
                                                (yyval.path).setDocumentObjectName(std::move((yyvsp[-4].string_or_identifier)), true, ObjectIdentifier::String(std::move((yyvsp[-2].string)),true),true);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                                (yyval.path).resolveAmbiguity();
                                            }
#line 1783 "ExpressionParser.tab.c"
    break;

  case 72: /* iden: object '.' id_or_cell  */
#line 197 "ExpressionParser.y"
                                            { /* Path to property of a given document object */
                                                (yyval.path) = ObjectIdentifier(DocumentObject);
                                                (yyvsp[-2].string_or_identifier).checkImport(DocumentObject);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[-2].string_or_identifier)));
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                                (yyval.path).resolveAmbiguity();
                                            }
#line 1795 "ExpressionParser.tab.c"
    break;

  case 73: /* iden: document '#' object '.' id_or_cell  */
#line 204 "ExpressionParser.y"
                                            { /* Path to property from an external document, within a named document object */
                                                (yyval.path) = ObjectIdentifier(DocumentObject);
                                                (yyval.path).setDocumentName(std::move((yyvsp[-4].string_or_identifier)), true);
                                                (yyval.path).setDocumentObjectName(std::move((yyvsp[-2].string_or_identifier)), true);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                                (yyval.path).resolveAmbiguity();
                                            }
#line 1807 "ExpressionParser.tab.c"
    break;

  case 74: /* iden: document '#' object '.' STRING '.' id_or_cell  */
#line 212 "ExpressionParser.y"
                                            {   (yyval.path) = ObjectIdentifier(DocumentObject);
                                                (yyval.path).setDocumentName(std::move((yyvsp[-6].string_or_identifier)), true);
                                                (yyval.path).setDocumentObjectName(std::move((yyvsp[-4].string_or_identifier)), true, ObjectIdentifier::String(std::move((yyvsp[-2].string)),true));
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                                (yyval.path).resolveAmbiguity();
                                            }
#line 1818 "ExpressionParser.tab.c"
    break;

  case 75: /* iden: iden '.' IDENTIFIER  */
#line 218 "ExpressionParser.y"
                                            { (yyval.path)= std::move((yyvsp[-2].path)); (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string))); }
#line 1824 "ExpressionParser.tab.c"
    break;

  case 76: /* indexer: '[' exp ']'  */
#line 222 "ExpressionParser.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-1].expr));   }
#line 1830 "ExpressionParser.tab.c"
    break;

  case 77: /* indexer: '[' exp ':' ']'  */
#line 223 "ExpressionParser.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-2].expr),0,0,true); }
#line 1836 "ExpressionParser.tab.c"
    break;

  case 78: /* indexer: '[' ':' exp ']'  */
#line 224 "ExpressionParser.y"
                                            { (yyval.component) = Expression::createComponent(0,(yyvsp[-1].expr)); }
#line 1842 "ExpressionParser.tab.c"
    break;

  case 79: /* indexer: '[' ':' ':' exp ']'  */
#line 225 "ExpressionParser.y"
                                            { (yyval.component) = Expression::createComponent(0,0,(yyvsp[-1].expr)); }
#line 1848 "ExpressionParser.tab.c"
    break;

  case 80: /* indexer: '[' exp ':' exp ']'  */
#line 226 "ExpressionParser.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-3].expr),(yyvsp[-1].expr));}
#line 1854 "ExpressionParser.tab.c"
    break;

  case 81: /* indexer: '[' exp ':' ':' exp ']'  */
#line 227 "ExpressionParser.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-4].expr),0,(yyvsp[-1].expr)); }
#line 1860 "ExpressionParser.tab.c"
    break;

  case 82: /* indexer: '[' ':' exp ':' exp ']'  */
#line 228 "ExpressionParser.y"
                                            { (yyval.component) = Expression::createComponent(0,(yyvsp[-3].expr),(yyvsp[-1].expr)); }
#line 1866 "ExpressionParser.tab.c"
    break;

  case 83: /* indexer: '[' exp ':' exp ':' exp ']'  */
#line 229 "ExpressionParser.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-5].expr),(yyvsp[-3].expr),(yyvsp[-1].expr));}
#line 1872 "ExpressionParser.tab.c"
    break;

  case 84: /* indexable: identifier indexer  */
#line 233 "ExpressionParser.y"
                                            { (yyval.expr) = new VariableExpression(DocumentObject,(yyvsp[-1].path)); (yyval.expr)->addComponent((yyvsp[0].component)); }
#line 1878 "ExpressionParser.tab.c"
    break;

  case 85: /* indexable: indexable indexer  */
#line 234 "ExpressionParser.y"
                                            { (yyvsp[-1].expr)->addComponent(std::move((yyvsp[0].component))); (yyval.expr) = (yyvsp[-1].expr); }
#line 1884 "ExpressionParser.tab.c"
    break;

  case 86: /* indexable: indexable '.' IDENTIFIER  */
#line 235 "ExpressionParser.y"
                                            { (yyvsp[-2].expr)->addComponent(Expression::createComponent((yyvsp[0].string))); (yyval.expr) = (yyvsp[-2].expr); }
#line 1890 "ExpressionParser.tab.c"
    break;

  case 87: /* document: STRING  */
#line 239 "ExpressionParser.y"
                                            { (yyval.string_or_identifier) = ObjectIdentifier::String(std::move((yyvsp[0].string)), true); }
#line 1896 "ExpressionParser.tab.c"
    break;

  case 88: /* document: IDENTIFIER  */
#line 240 "ExpressionParser.y"
                                            { (yyval.string_or_identifier) = ObjectIdentifier::String(std::move((yyvsp[0].string)), false, true);}
#line 1902 "ExpressionParser.tab.c"
    break;

  case 89: /* object: STRING  */
#line 244 "ExpressionParser.y"
                                            { (yyval.string_or_identifier) = ObjectIdentifier::String(std::move((yyvsp[0].string)), true); }
#line 1908 "ExpressionParser.tab.c"
    break;

  case 90: /* object: id_or_cell  */
#line 245 "ExpressionParser.y"
                                            { (yyval.string_or_identifier) = ObjectIdentifier::String(std::move((yyvsp[0].string)), false);}
#line 1914 "ExpressionParser.tab.c"
    break;


#line 1918 "ExpressionParser.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 248 "ExpressionParser.y"

