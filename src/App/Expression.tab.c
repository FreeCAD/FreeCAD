// clang-format off
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
#line 26 "Expression.y"

#define YYSTYPE App::ExpressionParser::semantic_type

std::stack<FunctionExpression::Function> functions;                /**< Function identifier */

#define yyparse ExpressionParser_yyparse
#define yyerror ExpressionParser_yyerror

#line 80 "Expression.tab.c"

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

#include "Expression.tab.h"
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
  YYSYMBOL_USUNIT = 8,                     /* USUNIT  */
  YYSYMBOL_INTEGER = 9,                    /* INTEGER  */
  YYSYMBOL_CONSTANT = 10,                  /* CONSTANT  */
  YYSYMBOL_CELLADDRESS = 11,               /* CELLADDRESS  */
  YYSYMBOL_EQ = 12,                        /* EQ  */
  YYSYMBOL_NEQ = 13,                       /* NEQ  */
  YYSYMBOL_LT = 14,                        /* LT  */
  YYSYMBOL_GT = 15,                        /* GT  */
  YYSYMBOL_GTE = 16,                       /* GTE  */
  YYSYMBOL_LTE = 17,                       /* LTE  */
  YYSYMBOL_STRING = 18,                    /* STRING  */
  YYSYMBOL_MINUSSIGN = 19,                 /* MINUSSIGN  */
  YYSYMBOL_PROPERTY_REF = 20,              /* PROPERTY_REF  */
  YYSYMBOL_DOCUMENT = 21,                  /* DOCUMENT  */
  YYSYMBOL_OBJECT = 22,                    /* OBJECT  */
  YYSYMBOL_EXPONENT = 23,                  /* EXPONENT  */
  YYSYMBOL_24_ = 24,                       /* '?'  */
  YYSYMBOL_25_ = 25,                       /* ':'  */
  YYSYMBOL_26_ = 26,                       /* '+'  */
  YYSYMBOL_27_ = 27,                       /* '*'  */
  YYSYMBOL_28_ = 28,                       /* '/'  */
  YYSYMBOL_29_ = 29,                       /* '%'  */
  YYSYMBOL_NUM_AND_UNIT = 30,              /* NUM_AND_UNIT  */
  YYSYMBOL_31_ = 31,                       /* '^'  */
  YYSYMBOL_NEG = 32,                       /* NEG  */
  YYSYMBOL_POS = 33,                       /* POS  */
  YYSYMBOL_34_ = 34,                       /* ')'  */
  YYSYMBOL_35_ = 35,                       /* '('  */
  YYSYMBOL_36_ = 36,                       /* ','  */
  YYSYMBOL_37_ = 37,                       /* ';'  */
  YYSYMBOL_38_ = 38,                       /* '.'  */
  YYSYMBOL_39_ = 39,                       /* '#'  */
  YYSYMBOL_40_ = 40,                       /* '['  */
  YYSYMBOL_41_ = 41,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 42,                  /* $accept  */
  YYSYMBOL_input = 43,                     /* input  */
  YYSYMBOL_unit_num = 44,                  /* unit_num  */
  YYSYMBOL_exp = 45,                       /* exp  */
  YYSYMBOL_num = 46,                       /* num  */
  YYSYMBOL_args = 47,                      /* args  */
  YYSYMBOL_range = 48,                     /* range  */
  YYSYMBOL_us_building_unit = 49,          /* us_building_unit  */
  YYSYMBOL_other_unit = 50,                /* other_unit  */
  YYSYMBOL_unit_exp = 51,                  /* unit_exp  */
  YYSYMBOL_integer = 52,                   /* integer  */
  YYSYMBOL_id_or_cell = 53,                /* id_or_cell  */
  YYSYMBOL_identifier = 54,                /* identifier  */
  YYSYMBOL_iden = 55,                      /* iden  */
  YYSYMBOL_indexer = 56,                   /* indexer  */
  YYSYMBOL_indexable = 57,                 /* indexable  */
  YYSYMBOL_document = 58,                  /* document  */
  YYSYMBOL_object = 59                     /* object  */
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
#define YYFINAL  40
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   432

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  42
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  18
/* YYNRULES -- Number of rules.  */
#define YYNRULES  76
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  140

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   281


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
       2,     2,     2,     2,     2,    39,     2,    29,     2,     2,
      35,    34,    27,    26,    36,     2,    38,    28,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    25,    37,
       2,     2,     2,    24,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    40,     2,    41,    31,     2,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21,    22,    23,    30,
      32,    33
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    81,    81,    82,    85,    86,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   114,
     115,   116,   117,   119,   120,   121,   122,   123,   124,   127,
     131,   132,   134,   135,   136,   137,   138,   139,   140,   143,
     144,   148,   149,   153,   154,   158,   163,   168,   174,   181,
     188,   195,   199,   200,   201,   202,   203,   204,   205,   206,
     210,   211,   212,   216,   217,   221,   222
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
  "IDENTIFIER", "UNIT", "USUNIT", "INTEGER", "CONSTANT", "CELLADDRESS",
  "EQ", "NEQ", "LT", "GT", "GTE", "LTE", "STRING", "MINUSSIGN",
  "PROPERTY_REF", "DOCUMENT", "OBJECT", "EXPONENT", "'?'", "':'", "'+'",
  "'*'", "'/'", "'%'", "NUM_AND_UNIT", "'^'", "NEG", "POS", "')'", "'('",
  "','", "';'", "'.'", "'#'", "'['", "']'", "$accept", "input", "unit_num",
  "exp", "num", "args", "range", "us_building_unit", "other_unit",
  "unit_exp", "integer", "id_or_cell", "identifier", "iden", "indexer",
  "indexable", "document", "object", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-32)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-77)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     100,   203,   -32,   -32,   -31,   -32,   -32,   -32,   -32,   -32,
      90,   203,   203,   100,    34,    17,   -32,   401,     7,   -32,
     -32,    93,   -10,   -19,    19,    16,    -5,    49,   203,   401,
      78,   -32,    -9,   -32,   -32,   358,    -4,   -32,    64,   -32,
     -32,   203,   203,   203,   203,   203,   203,   203,   203,   203,
     203,   100,   203,   203,     7,    84,     6,     7,     7,     3,
     156,   -32,   107,   119,   -32,    44,    52,   -32,   203,   203,
      25,   -32,   -32,    25,    20,    20,    20,    20,    20,    20,
      68,   381,    68,    59,    59,     6,    59,   -32,   122,     6,
       6,   -32,   -32,    57,   -32,   167,   218,   -32,   -32,   -32,
     -32,    94,    96,   -32,   401,   -32,   401,   -32,   -32,   -32,
     203,   -32,   -32,   203,   238,     0,   -32,    80,    25,   401,
     278,   203,   -32,   203,   -32,   258,    98,   -32,   -32,   -32,
     298,   318,   203,   -32,    25,   -32,   -32,   338,   -32,   -32
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,    29,    30,    51,    41,    40,    31,    32,    52,
       8,     0,     0,     0,     0,     0,     7,     2,     6,    43,
      42,     3,    53,     9,    54,    25,     0,     0,     0,    33,
       0,    34,    53,    10,    11,     0,     0,    51,     0,    56,
       1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    43,     4,     0,     0,     0,
       0,    70,     0,     0,    71,     0,     0,    26,     0,     0,
       0,    28,    48,     0,    19,    20,    21,    22,    23,    24,
      13,     0,    12,    14,    15,    17,    16,    18,     0,    45,
      44,    50,    49,     0,    46,     0,     0,    61,    72,    75,
      76,     0,     0,    58,    35,    37,    36,    38,    39,    55,
       0,     5,    47,     0,     0,     0,    62,     0,     0,    27,
       0,     0,    64,     0,    63,     0,     0,    59,    57,    65,
       0,     0,     0,    66,     0,    68,    67,     0,    60,    69
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -32,   -32,   -32,    32,    76,   -32,    54,   -17,   -32,     2,
      46,    -1,   -32,   -32,   112,   -32,   -32,    75
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,    15,    16,    35,    18,    30,    31,    19,    20,    36,
      94,    22,    23,    24,    61,    25,    26,    27
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      32,    55,    21,     1,     2,     3,     4,    91,   -74,     7,
       8,     9,    92,    39,     5,     6,    70,    40,    10,    11,
      56,    60,    93,    57,    58,   123,    12,    59,   -76,   -76,
      72,    37,    17,    29,    65,    28,     9,    59,    14,    47,
      37,   124,    54,    33,    34,     9,    49,    50,    51,    52,
      37,    53,    38,    85,    63,     9,    60,    62,    37,    89,
      90,    91,    99,     9,   100,   103,    92,    32,    32,   108,
     102,   111,   109,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    86,    87,    37,    66,     2,     3,
      53,     9,    96,     7,     8,    50,    51,    52,   126,    53,
     104,   106,    73,     1,     2,     3,     4,     5,     6,     7,
       8,     9,    67,    97,    68,    69,   127,   128,    10,    11,
      57,    58,   105,   107,    59,    98,    12,   114,   -75,   -73,
       6,    88,   117,   138,   118,    13,   134,    64,    14,   112,
     101,     0,   119,     0,     0,   120,     0,   125,     0,     0,
       0,     0,     0,   130,     0,   131,     0,     0,     0,     1,
       2,     3,     4,     0,   137,     7,     8,     9,     0,     0,
       1,     2,     3,     4,    10,    11,     7,     8,     9,     0,
       0,    95,    12,     0,     0,    10,    11,     0,     0,     0,
       0,    28,   113,    12,    14,     0,     0,     0,     0,     0,
       0,     0,    28,     0,     0,    14,     1,     2,     3,     4,
       0,     0,     7,     8,     9,     0,     0,     0,     0,     0,
       0,    10,    11,     0,     0,     0,     0,     0,     0,    12,
      41,    42,    43,    44,    45,    46,     0,    47,    28,     0,
       0,    14,    48,   115,    49,    50,    51,    52,     0,    53,
      41,    42,    43,    44,    45,    46,     0,    47,     0,   116,
       0,     0,    48,   121,    49,    50,    51,    52,     0,    53,
      41,    42,    43,    44,    45,    46,     0,    47,     0,   122,
       0,     0,    48,   132,    49,    50,    51,    52,     0,    53,
      41,    42,    43,    44,    45,    46,     0,    47,     0,   133,
       0,     0,    48,     0,    49,    50,    51,    52,     0,    53,
      41,    42,    43,    44,    45,    46,     0,    47,     0,   129,
       0,     0,    48,     0,    49,    50,    51,    52,     0,    53,
      41,    42,    43,    44,    45,    46,     0,    47,     0,   135,
       0,     0,    48,     0,    49,    50,    51,    52,     0,    53,
      41,    42,    43,    44,    45,    46,     0,    47,     0,   136,
       0,     0,    48,     0,    49,    50,    51,    52,     0,    53,
      41,    42,    43,    44,    45,    46,     0,    47,     0,   139,
       0,     0,    48,     0,    49,    50,    51,    52,     0,    53,
       0,     0,    71,    41,    42,    43,    44,    45,    46,     0,
      47,     0,     0,     0,     0,    48,   110,    49,    50,    51,
      52,     0,    53,    41,    42,    43,    44,    45,    46,     0,
      47,     0,     0,     0,     0,    48,     0,    49,    50,    51,
      52,     0,    53
};

static const yytype_int16 yycheck[] =
{
       1,    18,     0,     3,     4,     5,     6,     4,    39,     9,
      10,    11,     9,    14,     7,     8,    25,     0,    18,    19,
      18,    40,    19,    27,    28,    25,    26,    31,    38,    38,
      34,     6,     0,     1,    39,    35,    11,    31,    38,    19,
       6,    41,    35,    11,    12,    11,    26,    27,    28,    29,
       6,    31,    18,    51,    38,    11,    40,    38,     6,    57,
      58,     4,    18,    11,    65,    66,     9,    68,    69,    70,
      18,    88,    73,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,     6,    38,     4,     5,
      31,    11,    60,     9,    10,    27,    28,    29,    18,    31,
      68,    69,    38,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    34,     6,    36,    37,   117,   118,    18,    19,
      27,    28,    68,    69,    31,     6,    26,    95,    38,    39,
       8,    55,    38,   134,    38,    35,    38,    25,    38,    93,
      65,    -1,   110,    -1,    -1,   113,    -1,   115,    -1,    -1,
      -1,    -1,    -1,   121,    -1,   123,    -1,    -1,    -1,     3,
       4,     5,     6,    -1,   132,     9,    10,    11,    -1,    -1,
       3,     4,     5,     6,    18,    19,     9,    10,    11,    -1,
      -1,    25,    26,    -1,    -1,    18,    19,    -1,    -1,    -1,
      -1,    35,    25,    26,    38,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    35,    -1,    -1,    38,     3,     4,     5,     6,
      -1,    -1,     9,    10,    11,    -1,    -1,    -1,    -1,    -1,
      -1,    18,    19,    -1,    -1,    -1,    -1,    -1,    -1,    26,
      12,    13,    14,    15,    16,    17,    -1,    19,    35,    -1,
      -1,    38,    24,    25,    26,    27,    28,    29,    -1,    31,
      12,    13,    14,    15,    16,    17,    -1,    19,    -1,    41,
      -1,    -1,    24,    25,    26,    27,    28,    29,    -1,    31,
      12,    13,    14,    15,    16,    17,    -1,    19,    -1,    41,
      -1,    -1,    24,    25,    26,    27,    28,    29,    -1,    31,
      12,    13,    14,    15,    16,    17,    -1,    19,    -1,    41,
      -1,    -1,    24,    -1,    26,    27,    28,    29,    -1,    31,
      12,    13,    14,    15,    16,    17,    -1,    19,    -1,    41,
      -1,    -1,    24,    -1,    26,    27,    28,    29,    -1,    31,
      12,    13,    14,    15,    16,    17,    -1,    19,    -1,    41,
      -1,    -1,    24,    -1,    26,    27,    28,    29,    -1,    31,
      12,    13,    14,    15,    16,    17,    -1,    19,    -1,    41,
      -1,    -1,    24,    -1,    26,    27,    28,    29,    -1,    31,
      12,    13,    14,    15,    16,    17,    -1,    19,    -1,    41,
      -1,    -1,    24,    -1,    26,    27,    28,    29,    -1,    31,
      -1,    -1,    34,    12,    13,    14,    15,    16,    17,    -1,
      19,    -1,    -1,    -1,    -1,    24,    25,    26,    27,    28,
      29,    -1,    31,    12,    13,    14,    15,    16,    17,    -1,
      19,    -1,    -1,    -1,    -1,    24,    -1,    26,    27,    28,
      29,    -1,    31
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      18,    19,    26,    35,    38,    43,    44,    45,    46,    49,
      50,    51,    53,    54,    55,    57,    58,    59,    35,    45,
      47,    48,    53,    45,    45,    45,    51,     6,    18,    53,
       0,    12,    13,    14,    15,    16,    17,    19,    24,    26,
      27,    28,    29,    31,    35,    49,    51,    27,    28,    31,
      40,    56,    38,    38,    56,    39,    38,    34,    36,    37,
      25,    34,    34,    38,    45,    45,    45,    45,    45,    45,
      45,    45,    45,    45,    45,    51,    45,    45,    46,    51,
      51,     4,     9,    19,    52,    25,    45,     6,     6,    18,
      53,    59,    18,    53,    45,    48,    45,    48,    53,    53,
      25,    49,    52,    25,    45,    25,    41,    38,    38,    45,
      45,    25,    41,    25,    41,    45,    18,    53,    53,    41,
      45,    45,    25,    41,    38,    41,    41,    45,    53,    41
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    42,    43,    43,    44,    44,    45,    45,    45,    45,
      45,    45,    45,    45,    45,    45,    45,    45,    45,    45,
      45,    45,    45,    45,    45,    45,    45,    45,    45,    46,
      46,    46,    46,    47,    47,    47,    47,    47,    47,    48,
      49,    50,    51,    51,    51,    51,    51,    51,    51,    52,
      52,    53,    53,    54,    54,    55,    55,    55,    55,    55,
      55,    55,    56,    56,    56,    56,    56,    56,    56,    56,
      57,    57,    57,    58,    58,    59,    59
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     2,     4,     1,     1,     1,     1,
       2,     2,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     1,     3,     5,     3,     1,
       1,     1,     1,     1,     1,     3,     3,     3,     3,     3,
       1,     1,     1,     1,     3,     3,     3,     4,     3,     1,
       1,     1,     1,     1,     1,     4,     2,     5,     3,     5,
       7,     3,     3,     4,     4,     5,     5,     6,     6,     7,
       2,     2,     3,     1,     1,     1,     1
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
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 1010 "Expression.tab.c"
        break;

    case YYSYMBOL_num: /* num  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 1016 "Expression.tab.c"
        break;

    case YYSYMBOL_args: /* args  */
#line 75 "Expression.y"
            { std::vector<Expression*>::const_iterator i = ((*yyvaluep).arguments).begin(); while (i != ((*yyvaluep).arguments).end()) { delete *i; ++i; } }
#line 1022 "Expression.tab.c"
        break;

    case YYSYMBOL_range: /* range  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 1028 "Expression.tab.c"
        break;

    case YYSYMBOL_unit_exp: /* unit_exp  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 1034 "Expression.tab.c"
        break;

    case YYSYMBOL_indexer: /* indexer  */
#line 74 "Expression.y"
            { delete ((*yyvaluep).component); }
#line 1040 "Expression.tab.c"
        break;

    case YYSYMBOL_indexable: /* indexable  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 1046 "Expression.tab.c"
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
#line 81 "Expression.y"
                                                { ScanResult = (yyvsp[0].expr); valueExpression = true;                                        }
#line 1316 "Expression.tab.c"
    break;

  case 3: /* input: unit_exp  */
#line 82 "Expression.y"
                                                { ScanResult = (yyvsp[0].expr); unitExpression = true;                                         }
#line 1322 "Expression.tab.c"
    break;

  case 4: /* unit_num: num unit_exp  */
#line 85 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-1].expr), OperatorExpression::UNIT, (yyvsp[0].expr));  }
#line 1328 "Expression.tab.c"
    break;

  case 5: /* unit_num: num us_building_unit num us_building_unit  */
#line 86 "Expression.y"
                                                                         { (yyval.expr) = new OperatorExpression(DocumentObject, new OperatorExpression(DocumentObject, (yyvsp[-3].expr), OperatorExpression::UNIT, (yyvsp[-2].expr)), OperatorExpression::ADD, new OperatorExpression(DocumentObject, (yyvsp[-1].expr), OperatorExpression::UNIT, (yyvsp[0].expr)));}
#line 1334 "Expression.tab.c"
    break;

  case 6: /* exp: num  */
#line 89 "Expression.y"
                                                { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1340 "Expression.tab.c"
    break;

  case 7: /* exp: unit_num  */
#line 90 "Expression.y"
                                                { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1346 "Expression.tab.c"
    break;

  case 8: /* exp: STRING  */
#line 91 "Expression.y"
                                                { (yyval.expr) = new StringExpression(DocumentObject, (yyvsp[0].string));                                  }
#line 1352 "Expression.tab.c"
    break;

  case 9: /* exp: identifier  */
#line 92 "Expression.y"
                                                { (yyval.expr) = new VariableExpression(DocumentObject, (yyvsp[0].path));                                }
#line 1358 "Expression.tab.c"
    break;

  case 10: /* exp: MINUSSIGN exp  */
#line 93 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))); }
#line 1364 "Expression.tab.c"
    break;

  case 11: /* exp: '+' exp  */
#line 94 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::POS, new NumberExpression(DocumentObject, Quantity(1))); }
#line 1370 "Expression.tab.c"
    break;

  case 12: /* exp: exp '+' exp  */
#line 95 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::ADD, (yyvsp[0].expr));   }
#line 1376 "Expression.tab.c"
    break;

  case 13: /* exp: exp MINUSSIGN exp  */
#line 96 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::SUB, (yyvsp[0].expr));   }
#line 1382 "Expression.tab.c"
    break;

  case 14: /* exp: exp '*' exp  */
#line 97 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 1388 "Expression.tab.c"
    break;

  case 15: /* exp: exp '/' exp  */
#line 98 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1394 "Expression.tab.c"
    break;

  case 16: /* exp: exp '%' exp  */
#line 99 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MOD, (yyvsp[0].expr));   }
#line 1400 "Expression.tab.c"
    break;

  case 17: /* exp: exp '/' unit_exp  */
#line 100 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1406 "Expression.tab.c"
    break;

  case 18: /* exp: exp '^' exp  */
#line 101 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, (yyvsp[0].expr));   }
#line 1412 "Expression.tab.c"
    break;

  case 19: /* exp: exp EQ exp  */
#line 102 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::EQ, (yyvsp[0].expr));    }
#line 1418 "Expression.tab.c"
    break;

  case 20: /* exp: exp NEQ exp  */
#line 103 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::NEQ, (yyvsp[0].expr));   }
#line 1424 "Expression.tab.c"
    break;

  case 21: /* exp: exp LT exp  */
#line 104 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LT, (yyvsp[0].expr));    }
#line 1430 "Expression.tab.c"
    break;

  case 22: /* exp: exp GT exp  */
#line 105 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GT, (yyvsp[0].expr));    }
#line 1436 "Expression.tab.c"
    break;

  case 23: /* exp: exp GTE exp  */
#line 106 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GTE, (yyvsp[0].expr));   }
#line 1442 "Expression.tab.c"
    break;

  case 24: /* exp: exp LTE exp  */
#line 107 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LTE, (yyvsp[0].expr));   }
#line 1448 "Expression.tab.c"
    break;

  case 25: /* exp: indexable  */
#line 108 "Expression.y"
                                                { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1454 "Expression.tab.c"
    break;

  case 26: /* exp: FUNC args ')'  */
#line 109 "Expression.y"
                                                { (yyval.expr) = new FunctionExpression(DocumentObject, (yyvsp[-2].func).first, std::move((yyvsp[-2].func).second), (yyvsp[-1].arguments));}
#line 1460 "Expression.tab.c"
    break;

  case 27: /* exp: exp '?' exp ':' exp  */
#line 110 "Expression.y"
                                                { (yyval.expr) = new ConditionalExpression(DocumentObject, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr));                     }
#line 1466 "Expression.tab.c"
    break;

  case 28: /* exp: '(' exp ')'  */
#line 111 "Expression.y"
                                                { (yyval.expr) = (yyvsp[-1].expr); }
#line 1472 "Expression.tab.c"
    break;

  case 29: /* num: ONE  */
#line 114 "Expression.y"
                                                { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 1478 "Expression.tab.c"
    break;

  case 30: /* num: NUM  */
#line 115 "Expression.y"
                                                { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 1484 "Expression.tab.c"
    break;

  case 31: /* num: INTEGER  */
#line 116 "Expression.y"
                                                { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue)));                }
#line 1490 "Expression.tab.c"
    break;

  case 32: /* num: CONSTANT  */
#line 117 "Expression.y"
                                                { (yyval.expr) = new ConstantExpression(DocumentObject, (yyvsp[0].constant).name, Quantity((yyvsp[0].constant).fvalue));      }
#line 1496 "Expression.tab.c"
    break;

  case 33: /* args: exp  */
#line 119 "Expression.y"
                                                { (yyval.arguments).push_back((yyvsp[0].expr));                                                               }
#line 1502 "Expression.tab.c"
    break;

  case 34: /* args: range  */
#line 120 "Expression.y"
                                                { (yyval.arguments).push_back((yyvsp[0].expr));                                                               }
#line 1508 "Expression.tab.c"
    break;

  case 35: /* args: args ',' exp  */
#line 121 "Expression.y"
                                                { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1514 "Expression.tab.c"
    break;

  case 36: /* args: args ';' exp  */
#line 122 "Expression.y"
                                                { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1520 "Expression.tab.c"
    break;

  case 37: /* args: args ',' range  */
#line 123 "Expression.y"
                                                { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1526 "Expression.tab.c"
    break;

  case 38: /* args: args ';' range  */
#line 124 "Expression.y"
                                                { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1532 "Expression.tab.c"
    break;

  case 39: /* range: id_or_cell ':' id_or_cell  */
#line 127 "Expression.y"
                                                { (yyval.expr) = new RangeExpression(DocumentObject, (yyvsp[-2].string), (yyvsp[0].string));                               }
#line 1538 "Expression.tab.c"
    break;

  case 40: /* us_building_unit: USUNIT  */
#line 131 "Expression.y"
                                                { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 1544 "Expression.tab.c"
    break;

  case 41: /* other_unit: UNIT  */
#line 132 "Expression.y"
                                                { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 1550 "Expression.tab.c"
    break;

  case 42: /* unit_exp: other_unit  */
#line 134 "Expression.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 1556 "Expression.tab.c"
    break;

  case 43: /* unit_exp: us_building_unit  */
#line 135 "Expression.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 1562 "Expression.tab.c"
    break;

  case 44: /* unit_exp: unit_exp '/' unit_exp  */
#line 136 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1568 "Expression.tab.c"
    break;

  case 45: /* unit_exp: unit_exp '*' unit_exp  */
#line 137 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 1574 "Expression.tab.c"
    break;

  case 46: /* unit_exp: unit_exp '^' integer  */
#line 138 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))));   }
#line 1580 "Expression.tab.c"
    break;

  case 47: /* unit_exp: unit_exp '^' MINUSSIGN integer  */
#line 139 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-3].expr), OperatorExpression::POW, new OperatorExpression(DocumentObject, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))));   }
#line 1586 "Expression.tab.c"
    break;

  case 48: /* unit_exp: '(' unit_exp ')'  */
#line 140 "Expression.y"
                                                { (yyval.expr) = (yyvsp[-1].expr);                                                                        }
#line 1592 "Expression.tab.c"
    break;

  case 49: /* integer: INTEGER  */
#line 143 "Expression.y"
                 { (yyval.ivalue) = (yyvsp[0].ivalue); }
#line 1598 "Expression.tab.c"
    break;

  case 50: /* integer: ONE  */
#line 144 "Expression.y"
             { (yyval.ivalue) = (yyvsp[0].fvalue); }
#line 1604 "Expression.tab.c"
    break;

  case 51: /* id_or_cell: IDENTIFIER  */
#line 148 "Expression.y"
                                            { (yyval.string) = std::move((yyvsp[0].string)); }
#line 1610 "Expression.tab.c"
    break;

  case 52: /* id_or_cell: CELLADDRESS  */
#line 149 "Expression.y"
                                            { (yyval.string) = std::move((yyvsp[0].string)); }
#line 1616 "Expression.tab.c"
    break;

  case 53: /* identifier: id_or_cell  */
#line 153 "Expression.y"
                                            { (yyval.path) = ObjectIdentifier(DocumentObject); (yyval.path) << ObjectIdentifier::SimpleComponent((yyvsp[0].string)); }
#line 1622 "Expression.tab.c"
    break;

  case 54: /* identifier: iden  */
#line 154 "Expression.y"
                                            { (yyval.path) = std::move((yyvsp[0].path)); }
#line 1628 "Expression.tab.c"
    break;

  case 55: /* iden: '.' STRING '.' id_or_cell  */
#line 158 "Expression.y"
                                            { /* Path to property of a sub-object of the current object*/
                                                (yyval.path) = ObjectIdentifier(DocumentObject,true);
                                                (yyval.path).setDocumentObjectName(DocumentObject,false,ObjectIdentifier::String(std::move((yyvsp[-2].string)),true),true);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                            }
#line 1638 "Expression.tab.c"
    break;

  case 56: /* iden: '.' id_or_cell  */
#line 163 "Expression.y"
                                            { /* Path to property of the current document object */
                                                (yyval.path) = ObjectIdentifier(DocumentObject,true);
                                                (yyval.path).setDocumentObjectName(DocumentObject);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                            }
#line 1648 "Expression.tab.c"
    break;

  case 57: /* iden: object '.' STRING '.' id_or_cell  */
#line 168 "Expression.y"
                                            { /* Path to property of a sub-object */
                                                (yyval.path) = ObjectIdentifier(DocumentObject);
                                                (yyval.path).setDocumentObjectName(std::move((yyvsp[-4].string_or_identifier)), true, ObjectIdentifier::String(std::move((yyvsp[-2].string)),true),true);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                                (yyval.path).resolveAmbiguity();
                                            }
#line 1659 "Expression.tab.c"
    break;

  case 58: /* iden: object '.' id_or_cell  */
#line 174 "Expression.y"
                                            { /* Path to property of a given document object */
                                                (yyval.path) = ObjectIdentifier(DocumentObject);
                                                (yyvsp[-2].string_or_identifier).checkImport(DocumentObject);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[-2].string_or_identifier)));
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                                (yyval.path).resolveAmbiguity();
                                            }
#line 1671 "Expression.tab.c"
    break;

  case 59: /* iden: document '#' object '.' id_or_cell  */
#line 181 "Expression.y"
                                            { /* Path to property from an external document, within a named document object */
                                                (yyval.path) = ObjectIdentifier(DocumentObject);
                                                (yyval.path).setDocumentName(std::move((yyvsp[-4].string_or_identifier)), true);
                                                (yyval.path).setDocumentObjectName(std::move((yyvsp[-2].string_or_identifier)), true);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                                (yyval.path).resolveAmbiguity();
                                            }
#line 1683 "Expression.tab.c"
    break;

  case 60: /* iden: document '#' object '.' STRING '.' id_or_cell  */
#line 189 "Expression.y"
                                            {   (yyval.path) = ObjectIdentifier(DocumentObject);
                                                (yyval.path).setDocumentName(std::move((yyvsp[-6].string_or_identifier)), true);
                                                (yyval.path).setDocumentObjectName(std::move((yyvsp[-4].string_or_identifier)), true, ObjectIdentifier::String(std::move((yyvsp[-2].string)),true));
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                                (yyval.path).resolveAmbiguity();
                                            }
#line 1694 "Expression.tab.c"
    break;

  case 61: /* iden: iden '.' IDENTIFIER  */
#line 195 "Expression.y"
                                            { (yyval.path)= std::move((yyvsp[-2].path)); (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string))); }
#line 1700 "Expression.tab.c"
    break;

  case 62: /* indexer: '[' exp ']'  */
#line 199 "Expression.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-1].expr));   }
#line 1706 "Expression.tab.c"
    break;

  case 63: /* indexer: '[' exp ':' ']'  */
#line 200 "Expression.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-2].expr),0,0,true); }
#line 1712 "Expression.tab.c"
    break;

  case 64: /* indexer: '[' ':' exp ']'  */
#line 201 "Expression.y"
                                            { (yyval.component) = Expression::createComponent(0,(yyvsp[-1].expr)); }
#line 1718 "Expression.tab.c"
    break;

  case 65: /* indexer: '[' ':' ':' exp ']'  */
#line 202 "Expression.y"
                                            { (yyval.component) = Expression::createComponent(0,0,(yyvsp[-1].expr)); }
#line 1724 "Expression.tab.c"
    break;

  case 66: /* indexer: '[' exp ':' exp ']'  */
#line 203 "Expression.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-3].expr),(yyvsp[-1].expr));}
#line 1730 "Expression.tab.c"
    break;

  case 67: /* indexer: '[' exp ':' ':' exp ']'  */
#line 204 "Expression.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-4].expr),0,(yyvsp[-1].expr)); }
#line 1736 "Expression.tab.c"
    break;

  case 68: /* indexer: '[' ':' exp ':' exp ']'  */
#line 205 "Expression.y"
                                            { (yyval.component) = Expression::createComponent(0,(yyvsp[-3].expr),(yyvsp[-1].expr)); }
#line 1742 "Expression.tab.c"
    break;

  case 69: /* indexer: '[' exp ':' exp ':' exp ']'  */
#line 206 "Expression.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-5].expr),(yyvsp[-3].expr),(yyvsp[-1].expr));}
#line 1748 "Expression.tab.c"
    break;

  case 70: /* indexable: identifier indexer  */
#line 210 "Expression.y"
                                            { (yyval.expr) = new VariableExpression(DocumentObject,(yyvsp[-1].path)); (yyval.expr)->addComponent((yyvsp[0].component)); }
#line 1754 "Expression.tab.c"
    break;

  case 71: /* indexable: indexable indexer  */
#line 211 "Expression.y"
                                            { (yyvsp[-1].expr)->addComponent(std::move((yyvsp[0].component))); (yyval.expr) = (yyvsp[-1].expr); }
#line 1760 "Expression.tab.c"
    break;

  case 72: /* indexable: indexable '.' IDENTIFIER  */
#line 212 "Expression.y"
                                            { (yyvsp[-2].expr)->addComponent(Expression::createComponent((yyvsp[0].string))); (yyval.expr) = (yyvsp[-2].expr); }
#line 1766 "Expression.tab.c"
    break;

  case 73: /* document: STRING  */
#line 216 "Expression.y"
                                            { (yyval.string_or_identifier) = ObjectIdentifier::String(std::move((yyvsp[0].string)), true); }
#line 1772 "Expression.tab.c"
    break;

  case 74: /* document: IDENTIFIER  */
#line 217 "Expression.y"
                                            { (yyval.string_or_identifier) = ObjectIdentifier::String(std::move((yyvsp[0].string)), false, true);}
#line 1778 "Expression.tab.c"
    break;

  case 75: /* object: STRING  */
#line 221 "Expression.y"
                                            { (yyval.string_or_identifier) = ObjectIdentifier::String(std::move((yyvsp[0].string)), true); }
#line 1784 "Expression.tab.c"
    break;

  case 76: /* object: id_or_cell  */
#line 222 "Expression.y"
                                            { (yyval.string_or_identifier) = ObjectIdentifier::String(std::move((yyvsp[0].string)), false);}
#line 1790 "Expression.tab.c"
    break;


#line 1794 "Expression.tab.c"

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

#line 225 "Expression.y"

