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
  YYSYMBOL_exp_cond = 46,                  /* exp_cond  */
  YYSYMBOL_exp_bool = 47,                  /* exp_bool  */
  YYSYMBOL_exp_arith = 48,                 /* exp_arith  */
  YYSYMBOL_exp_term = 49,                  /* exp_term  */
  YYSYMBOL_exp_factor = 50,                /* exp_factor  */
  YYSYMBOL_exp_unary = 51,                 /* exp_unary  */
  YYSYMBOL_exp_prim = 52,                  /* exp_prim  */
  YYSYMBOL_num = 53,                       /* num  */
  YYSYMBOL_args = 54,                      /* args  */
  YYSYMBOL_range = 55,                     /* range  */
  YYSYMBOL_us_building_unit = 56,          /* us_building_unit  */
  YYSYMBOL_other_unit = 57,                /* other_unit  */
  YYSYMBOL_unit_exp = 58,                  /* unit_exp  */
  YYSYMBOL_integer = 59,                   /* integer  */
  YYSYMBOL_id_or_cell = 60,                /* id_or_cell  */
  YYSYMBOL_identifier = 61,                /* identifier  */
  YYSYMBOL_iden = 62,                      /* iden  */
  YYSYMBOL_indexer = 63,                   /* indexer  */
  YYSYMBOL_indexable = 64,                 /* indexable  */
  YYSYMBOL_document = 65,                  /* document  */
  YYSYMBOL_object = 66                     /* object  */
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
#define YYFINAL  47
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   227

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  42
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  25
/* YYNRULES -- Number of rules.  */
#define YYNRULES  83
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  147

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
       0,    81,    81,    82,    85,    86,    89,    92,    93,    96,
      97,    98,    99,   100,   101,   102,   105,   106,   107,   110,
     111,   112,   113,   114,   117,   118,   121,   122,   123,   126,
     127,   128,   129,   130,   131,   132,   135,   136,   137,   138,
     140,   141,   142,   143,   144,   145,   148,   152,   153,   155,
     156,   157,   158,   159,   160,   161,   164,   165,   169,   170,
     174,   175,   179,   184,   189,   195,   202,   209,   216,   220,
     221,   222,   223,   224,   225,   226,   227,   231,   232,   233,
     237,   238,   242,   243
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
  "exp", "exp_cond", "exp_bool", "exp_arith", "exp_term", "exp_factor",
  "exp_unary", "exp_prim", "num", "args", "range", "us_building_unit",
  "other_unit", "unit_exp", "integer", "id_or_cell", "identifier", "iden",
  "indexer", "indexable", "document", "object", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-23)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-84)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      91,   153,   -23,   -23,   -11,   -23,   -23,   -23,   -23,   -23,
      24,   189,   189,    91,    40,    43,   -23,   -23,   -23,    57,
     170,   115,    59,   -23,   -23,    -3,   -23,   -23,    94,    65,
      44,    80,    53,    73,    89,   153,   -23,     8,   -23,   -18,
     -23,   -23,    82,    77,   -23,    92,   -23,   -23,   153,   153,
     153,   153,   153,   153,   153,   153,   153,   153,    91,   153,
     153,    -3,   110,   101,    -3,    -3,    10,    30,   -23,   128,
     143,   -23,    46,    71,   -23,   153,   153,    -5,   -23,   -23,
      -5,   129,    35,    35,    35,    35,    35,    35,   115,   115,
      59,    59,   101,    59,   -23,   147,   101,   101,   -23,   -23,
      79,   -23,   142,   -17,   -23,   -23,   -23,   -23,   131,   132,
     -23,   -23,   -23,   -23,   -23,   -23,   -23,   153,   -23,   -23,
     153,   -16,    12,   -23,    74,    -5,   -23,   133,   153,   -23,
     153,   -23,   -15,   135,   -23,   -23,   -23,   134,   137,   153,
     -23,    -5,   -23,   -23,   140,   -23,   -23
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,    36,    37,    58,    48,    47,    38,    39,    59,
      31,     0,     0,     0,     0,     0,    30,     2,     6,     7,
       9,    16,    19,    24,    26,    29,    50,    49,     3,    60,
      32,    61,    33,     0,     0,     0,    40,     0,    41,    60,
      27,    28,     0,     0,    58,     0,    63,     1,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    50,     4,     0,     0,     0,     0,    77,     0,
       0,    78,     0,     0,    34,     0,     0,     0,    35,    55,
       0,     0,    10,    11,    12,    13,    14,    15,    18,    17,
      20,    21,    23,    22,    25,     0,    52,    51,    57,    56,
       0,    53,     0,     0,    68,    79,    82,    83,     0,     0,
      65,    42,    44,    43,    45,    46,    62,     0,     5,    54,
       0,     0,     0,    69,     0,     0,     8,     0,     0,    71,
       0,    70,     0,     0,    66,    64,    72,     0,     0,     0,
      73,     0,    75,    74,     0,    67,    76
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -23,   -23,   -23,    11,   -23,   -23,   152,    14,    78,   -23,
      95,   114,   -23,    90,   -22,   -23,     2,    97,    -1,   -23,
     -23,   158,   -23,   -23,   136
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,    15,    16,    42,    18,    19,    20,    21,    22,    23,
      24,    25,    37,    38,    26,    27,    43,   101,    29,    30,
      31,    68,    32,    33,    34
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      39,    44,    28,    62,     5,     6,     9,    77,   122,   128,
     139,    17,    36,    46,    98,     1,     2,     3,     4,    99,
     -83,     7,     8,     9,   123,   129,   140,    63,   -81,   100,
      10,    11,    61,     1,     2,     3,     4,   130,    12,     7,
       8,     9,    74,    47,    75,    76,    44,    35,    10,    11,
      14,     9,    44,   131,    55,   102,    12,     9,    45,    81,
      92,    56,   -82,   -80,   106,    35,    96,    97,    14,    88,
      89,   107,   110,   118,    39,    39,   115,    44,   103,   116,
      44,    48,     9,    98,    67,     9,   111,   113,    99,   109,
      60,    70,   133,    67,     1,     2,     3,     4,     5,     6,
       7,     8,     9,   -83,    64,    65,    40,    41,    66,    10,
      11,    79,    72,   121,     2,     3,    78,    12,    69,     7,
       8,    64,    65,   134,   135,    66,    13,    73,   126,    14,
      80,   127,    66,   132,   104,    90,    91,    93,    94,   137,
     145,   138,    57,    58,    59,     1,     2,     3,     4,   105,
     144,     7,     8,     9,   117,     6,     1,     2,     3,     4,
      10,    11,     7,     8,     9,   112,   114,   120,    12,   124,
     125,    10,    11,   141,   136,   142,    95,    35,   143,    12,
      14,   146,    49,    50,    51,    52,    53,    54,    35,    55,
      71,    14,     1,     2,     3,     4,    56,   119,     7,     8,
       9,    82,    83,    84,    85,    86,    87,    10,   108,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    35,     0,     0,    14
};

static const yytype_int16 yycheck[] =
{
       1,     6,     0,    25,     7,     8,    11,    25,    25,    25,
      25,     0,     1,    14,     4,     3,     4,     5,     6,     9,
      38,     9,    10,    11,    41,    41,    41,    25,    39,    19,
      18,    19,    35,     3,     4,     5,     6,    25,    26,     9,
      10,    11,    34,     0,    36,    37,     6,    35,    18,    19,
      38,    11,     6,    41,    19,    25,    26,    11,    18,    48,
      58,    26,    38,    39,    18,    35,    64,    65,    38,    55,
      56,    72,    73,    95,    75,    76,    77,     6,    67,    80,
       6,    24,    11,     4,    40,    11,    75,    76,     9,    18,
      31,    38,    18,    40,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    38,    27,    28,    11,    12,    31,    18,
      19,    34,    39,   102,     4,     5,    34,    26,    38,     9,
      10,    27,    28,   124,   125,    31,    35,    38,   117,    38,
      38,   120,    31,   122,     6,    57,    58,    59,    60,   128,
     141,   130,    27,    28,    29,     3,     4,     5,     6,     6,
     139,     9,    10,    11,    25,     8,     3,     4,     5,     6,
      18,    19,     9,    10,    11,    75,    76,    25,    26,    38,
      38,    18,    19,    38,    41,    41,    62,    35,    41,    26,
      38,    41,    12,    13,    14,    15,    16,    17,    35,    19,
      32,    38,     3,     4,     5,     6,    26,   100,     9,    10,
      11,    49,    50,    51,    52,    53,    54,    18,    72,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    35,    -1,    -1,    38
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      18,    19,    26,    35,    38,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    56,    57,    58,    60,
      61,    62,    64,    65,    66,    35,    45,    54,    55,    60,
      52,    52,    45,    58,     6,    18,    60,     0,    24,    12,
      13,    14,    15,    16,    17,    19,    26,    27,    28,    29,
      31,    35,    56,    58,    27,    28,    31,    40,    63,    38,
      38,    63,    39,    38,    34,    36,    37,    25,    34,    34,
      38,    45,    48,    48,    48,    48,    48,    48,    49,    49,
      50,    50,    58,    50,    50,    53,    58,    58,     4,     9,
      19,    59,    25,    45,     6,     6,    18,    60,    66,    18,
      60,    45,    55,    45,    55,    60,    60,    25,    56,    59,
      25,    45,    25,    41,    38,    38,    45,    45,    25,    41,
      25,    41,    45,    18,    60,    60,    41,    45,    45,    25,
      41,    38,    41,    41,    45,    60,    41
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    42,    43,    43,    44,    44,    45,    46,    46,    47,
      47,    47,    47,    47,    47,    47,    48,    48,    48,    49,
      49,    49,    49,    49,    50,    50,    51,    51,    51,    52,
      52,    52,    52,    52,    52,    52,    53,    53,    53,    53,
      54,    54,    54,    54,    54,    54,    55,    56,    57,    58,
      58,    58,    58,    58,    58,    58,    59,    59,    60,    60,
      61,    61,    62,    62,    62,    62,    62,    62,    62,    63,
      63,    63,    63,    63,    63,    63,    63,    64,    64,    64,
      65,    65,    66,    66
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     2,     4,     1,     1,     5,     1,
       3,     3,     3,     3,     3,     3,     1,     3,     3,     1,
       3,     3,     3,     3,     1,     3,     1,     2,     2,     1,
       1,     1,     1,     1,     3,     3,     1,     1,     1,     1,
       1,     1,     3,     3,     3,     3,     3,     1,     1,     1,
       1,     3,     3,     3,     4,     3,     1,     1,     1,     1,
       1,     1,     4,     2,     5,     3,     5,     7,     3,     3,
       4,     4,     5,     5,     6,     6,     7,     2,     2,     3,
       1,     1,     1,     1
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
#line 984 "Expression.tab.c"
        break;

    case YYSYMBOL_exp_cond: /* exp_cond  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 990 "Expression.tab.c"
        break;

    case YYSYMBOL_exp_bool: /* exp_bool  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 996 "Expression.tab.c"
        break;

    case YYSYMBOL_exp_arith: /* exp_arith  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 1002 "Expression.tab.c"
        break;

    case YYSYMBOL_exp_term: /* exp_term  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 1008 "Expression.tab.c"
        break;

    case YYSYMBOL_exp_factor: /* exp_factor  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 1014 "Expression.tab.c"
        break;

    case YYSYMBOL_exp_unary: /* exp_unary  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 1020 "Expression.tab.c"
        break;

    case YYSYMBOL_exp_prim: /* exp_prim  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 1026 "Expression.tab.c"
        break;

    case YYSYMBOL_num: /* num  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 1032 "Expression.tab.c"
        break;

    case YYSYMBOL_args: /* args  */
#line 75 "Expression.y"
            { std::vector<Expression*>::const_iterator i = ((*yyvaluep).arguments).begin(); while (i != ((*yyvaluep).arguments).end()) { delete *i; ++i; } }
#line 1038 "Expression.tab.c"
        break;

    case YYSYMBOL_range: /* range  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 1044 "Expression.tab.c"
        break;

    case YYSYMBOL_unit_exp: /* unit_exp  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 1050 "Expression.tab.c"
        break;

    case YYSYMBOL_indexer: /* indexer  */
#line 74 "Expression.y"
            { delete ((*yyvaluep).component); }
#line 1056 "Expression.tab.c"
        break;

    case YYSYMBOL_indexable: /* indexable  */
#line 73 "Expression.y"
            { delete ((*yyvaluep).expr); }
#line 1062 "Expression.tab.c"
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
#line 1332 "Expression.tab.c"
    break;

  case 3: /* input: unit_exp  */
#line 82 "Expression.y"
                                                { ScanResult = (yyvsp[0].expr); unitExpression = true;                                         }
#line 1338 "Expression.tab.c"
    break;

  case 4: /* unit_num: num unit_exp  */
#line 85 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-1].expr), OperatorExpression::UNIT, (yyvsp[0].expr));  }
#line 1344 "Expression.tab.c"
    break;

  case 5: /* unit_num: num us_building_unit num us_building_unit  */
#line 86 "Expression.y"
                                                                         { (yyval.expr) = new OperatorExpression(DocumentObject, new OperatorExpression(DocumentObject, (yyvsp[-3].expr), OperatorExpression::UNIT, (yyvsp[-2].expr)), OperatorExpression::ADD, new OperatorExpression(DocumentObject, (yyvsp[-1].expr), OperatorExpression::UNIT, (yyvsp[0].expr)));}
#line 1350 "Expression.tab.c"
    break;

  case 6: /* exp: exp_cond  */
#line 89 "Expression.y"
                                                { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1356 "Expression.tab.c"
    break;

  case 7: /* exp_cond: exp_bool  */
#line 92 "Expression.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 1362 "Expression.tab.c"
    break;

  case 8: /* exp_cond: exp_bool '?' exp ':' exp  */
#line 93 "Expression.y"
                                                { (yyval.expr) = new ConditionalExpression(DocumentObject, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr));                     }
#line 1368 "Expression.tab.c"
    break;

  case 9: /* exp_bool: exp_arith  */
#line 96 "Expression.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 1374 "Expression.tab.c"
    break;

  case 10: /* exp_bool: exp_arith EQ exp_arith  */
#line 97 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::EQ, (yyvsp[0].expr));    }
#line 1380 "Expression.tab.c"
    break;

  case 11: /* exp_bool: exp_arith NEQ exp_arith  */
#line 98 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::NEQ, (yyvsp[0].expr));   }
#line 1386 "Expression.tab.c"
    break;

  case 12: /* exp_bool: exp_arith LT exp_arith  */
#line 99 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LT, (yyvsp[0].expr));    }
#line 1392 "Expression.tab.c"
    break;

  case 13: /* exp_bool: exp_arith GT exp_arith  */
#line 100 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GT, (yyvsp[0].expr));    }
#line 1398 "Expression.tab.c"
    break;

  case 14: /* exp_bool: exp_arith GTE exp_arith  */
#line 101 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GTE, (yyvsp[0].expr));   }
#line 1404 "Expression.tab.c"
    break;

  case 15: /* exp_bool: exp_arith LTE exp_arith  */
#line 102 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LTE, (yyvsp[0].expr));   }
#line 1410 "Expression.tab.c"
    break;

  case 16: /* exp_arith: exp_term  */
#line 105 "Expression.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 1416 "Expression.tab.c"
    break;

  case 17: /* exp_arith: exp_arith '+' exp_term  */
#line 106 "Expression.y"
                                                 { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::ADD, (yyvsp[0].expr));   }
#line 1422 "Expression.tab.c"
    break;

  case 18: /* exp_arith: exp_arith MINUSSIGN exp_term  */
#line 107 "Expression.y"
                                                 { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::SUB, (yyvsp[0].expr));   }
#line 1428 "Expression.tab.c"
    break;

  case 19: /* exp_term: exp_factor  */
#line 110 "Expression.y"
                                                 { (yyval.expr) = (yyvsp[0].expr); }
#line 1434 "Expression.tab.c"
    break;

  case 20: /* exp_term: exp_term '*' exp_factor  */
#line 111 "Expression.y"
                                               { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 1440 "Expression.tab.c"
    break;

  case 21: /* exp_term: exp_term '/' exp_factor  */
#line 112 "Expression.y"
                                               { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1446 "Expression.tab.c"
    break;

  case 22: /* exp_term: exp_term '%' exp_factor  */
#line 113 "Expression.y"
                                               { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MOD, (yyvsp[0].expr));   }
#line 1452 "Expression.tab.c"
    break;

  case 23: /* exp_term: exp_term '/' unit_exp  */
#line 114 "Expression.y"
                                               { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1458 "Expression.tab.c"
    break;

  case 24: /* exp_factor: exp_unary  */
#line 117 "Expression.y"
                                                 { (yyval.expr) = (yyvsp[0].expr); }
#line 1464 "Expression.tab.c"
    break;

  case 25: /* exp_factor: exp_factor '^' exp_factor  */
#line 118 "Expression.y"
                                                 { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, (yyvsp[0].expr));   }
#line 1470 "Expression.tab.c"
    break;

  case 26: /* exp_unary: exp_prim  */
#line 121 "Expression.y"
                                                 { (yyval.expr) = (yyvsp[0].expr); }
#line 1476 "Expression.tab.c"
    break;

  case 27: /* exp_unary: MINUSSIGN exp_prim  */
#line 122 "Expression.y"
                                                 { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))); }
#line 1482 "Expression.tab.c"
    break;

  case 28: /* exp_unary: '+' exp_prim  */
#line 123 "Expression.y"
                                                 { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::POS, new NumberExpression(DocumentObject, Quantity(1))); }
#line 1488 "Expression.tab.c"
    break;

  case 29: /* exp_prim: num  */
#line 126 "Expression.y"
                                                { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1494 "Expression.tab.c"
    break;

  case 30: /* exp_prim: unit_num  */
#line 127 "Expression.y"
                                                { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1500 "Expression.tab.c"
    break;

  case 31: /* exp_prim: STRING  */
#line 128 "Expression.y"
                                                { (yyval.expr) = new StringExpression(DocumentObject, (yyvsp[0].string));                                  }
#line 1506 "Expression.tab.c"
    break;

  case 32: /* exp_prim: identifier  */
#line 129 "Expression.y"
                                                { (yyval.expr) = new VariableExpression(DocumentObject, (yyvsp[0].path));                                }
#line 1512 "Expression.tab.c"
    break;

  case 33: /* exp_prim: indexable  */
#line 130 "Expression.y"
                                                { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1518 "Expression.tab.c"
    break;

  case 34: /* exp_prim: FUNC args ')'  */
#line 131 "Expression.y"
                                                { (yyval.expr) = new FunctionExpression(DocumentObject, (yyvsp[-2].func).first, std::move((yyvsp[-2].func).second), (yyvsp[-1].arguments));}
#line 1524 "Expression.tab.c"
    break;

  case 35: /* exp_prim: '(' exp ')'  */
#line 132 "Expression.y"
                                                { (yyval.expr) = (yyvsp[-1].expr); }
#line 1530 "Expression.tab.c"
    break;

  case 36: /* num: ONE  */
#line 135 "Expression.y"
                                                { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 1536 "Expression.tab.c"
    break;

  case 37: /* num: NUM  */
#line 136 "Expression.y"
                                                { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 1542 "Expression.tab.c"
    break;

  case 38: /* num: INTEGER  */
#line 137 "Expression.y"
                                                { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue)));                }
#line 1548 "Expression.tab.c"
    break;

  case 39: /* num: CONSTANT  */
#line 138 "Expression.y"
                                                { (yyval.expr) = new ConstantExpression(DocumentObject, (yyvsp[0].constant).name, Quantity((yyvsp[0].constant).fvalue));      }
#line 1554 "Expression.tab.c"
    break;

  case 40: /* args: exp  */
#line 140 "Expression.y"
                                                { (yyval.arguments).push_back((yyvsp[0].expr));                                                               }
#line 1560 "Expression.tab.c"
    break;

  case 41: /* args: range  */
#line 141 "Expression.y"
                                                { (yyval.arguments).push_back((yyvsp[0].expr));                                                               }
#line 1566 "Expression.tab.c"
    break;

  case 42: /* args: args ',' exp  */
#line 142 "Expression.y"
                                                { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1572 "Expression.tab.c"
    break;

  case 43: /* args: args ';' exp  */
#line 143 "Expression.y"
                                                { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1578 "Expression.tab.c"
    break;

  case 44: /* args: args ',' range  */
#line 144 "Expression.y"
                                                { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1584 "Expression.tab.c"
    break;

  case 45: /* args: args ';' range  */
#line 145 "Expression.y"
                                                { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1590 "Expression.tab.c"
    break;

  case 46: /* range: id_or_cell ':' id_or_cell  */
#line 148 "Expression.y"
                                                { (yyval.expr) = new RangeExpression(DocumentObject, (yyvsp[-2].string), (yyvsp[0].string));                               }
#line 1596 "Expression.tab.c"
    break;

  case 47: /* us_building_unit: USUNIT  */
#line 152 "Expression.y"
                                                { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 1602 "Expression.tab.c"
    break;

  case 48: /* other_unit: UNIT  */
#line 153 "Expression.y"
                                                { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 1608 "Expression.tab.c"
    break;

  case 49: /* unit_exp: other_unit  */
#line 155 "Expression.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 1614 "Expression.tab.c"
    break;

  case 50: /* unit_exp: us_building_unit  */
#line 156 "Expression.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 1620 "Expression.tab.c"
    break;

  case 51: /* unit_exp: unit_exp '/' unit_exp  */
#line 157 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1626 "Expression.tab.c"
    break;

  case 52: /* unit_exp: unit_exp '*' unit_exp  */
#line 158 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 1632 "Expression.tab.c"
    break;

  case 53: /* unit_exp: unit_exp '^' integer  */
#line 159 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))));   }
#line 1638 "Expression.tab.c"
    break;

  case 54: /* unit_exp: unit_exp '^' MINUSSIGN integer  */
#line 160 "Expression.y"
                                                { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-3].expr), OperatorExpression::POW, new OperatorExpression(DocumentObject, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))));   }
#line 1644 "Expression.tab.c"
    break;

  case 55: /* unit_exp: '(' unit_exp ')'  */
#line 161 "Expression.y"
                                                { (yyval.expr) = (yyvsp[-1].expr);                                                                        }
#line 1650 "Expression.tab.c"
    break;

  case 56: /* integer: INTEGER  */
#line 164 "Expression.y"
                 { (yyval.ivalue) = (yyvsp[0].ivalue); }
#line 1656 "Expression.tab.c"
    break;

  case 57: /* integer: ONE  */
#line 165 "Expression.y"
             { (yyval.ivalue) = (yyvsp[0].fvalue); }
#line 1662 "Expression.tab.c"
    break;

  case 58: /* id_or_cell: IDENTIFIER  */
#line 169 "Expression.y"
                                            { (yyval.string) = std::move((yyvsp[0].string)); }
#line 1668 "Expression.tab.c"
    break;

  case 59: /* id_or_cell: CELLADDRESS  */
#line 170 "Expression.y"
                                            { (yyval.string) = std::move((yyvsp[0].string)); }
#line 1674 "Expression.tab.c"
    break;

  case 60: /* identifier: id_or_cell  */
#line 174 "Expression.y"
                                            { (yyval.path) = ObjectIdentifier(DocumentObject); (yyval.path) << ObjectIdentifier::SimpleComponent((yyvsp[0].string)); }
#line 1680 "Expression.tab.c"
    break;

  case 61: /* identifier: iden  */
#line 175 "Expression.y"
                                            { (yyval.path) = std::move((yyvsp[0].path)); }
#line 1686 "Expression.tab.c"
    break;

  case 62: /* iden: '.' STRING '.' id_or_cell  */
#line 179 "Expression.y"
                                            { /* Path to property of a sub-object of the current object*/
                                                (yyval.path) = ObjectIdentifier(DocumentObject,true);
                                                (yyval.path).setDocumentObjectName(DocumentObject,false,ObjectIdentifier::String(std::move((yyvsp[-2].string)),true),true);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                            }
#line 1696 "Expression.tab.c"
    break;

  case 63: /* iden: '.' id_or_cell  */
#line 184 "Expression.y"
                                            { /* Path to property of the current document object */
                                                (yyval.path) = ObjectIdentifier(DocumentObject,true);
                                                (yyval.path).setDocumentObjectName(DocumentObject);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                            }
#line 1706 "Expression.tab.c"
    break;

  case 64: /* iden: object '.' STRING '.' id_or_cell  */
#line 189 "Expression.y"
                                            { /* Path to property of a sub-object */
                                                (yyval.path) = ObjectIdentifier(DocumentObject);
                                                (yyval.path).setDocumentObjectName(std::move((yyvsp[-4].string_or_identifier)), true, ObjectIdentifier::String(std::move((yyvsp[-2].string)),true),true);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                                (yyval.path).resolveAmbiguity();
                                            }
#line 1717 "Expression.tab.c"
    break;

  case 65: /* iden: object '.' id_or_cell  */
#line 195 "Expression.y"
                                            { /* Path to property of a given document object */
                                                (yyval.path) = ObjectIdentifier(DocumentObject);
                                                (yyvsp[-2].string_or_identifier).checkImport(DocumentObject);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[-2].string_or_identifier)));
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                                (yyval.path).resolveAmbiguity();
                                            }
#line 1729 "Expression.tab.c"
    break;

  case 66: /* iden: document '#' object '.' id_or_cell  */
#line 202 "Expression.y"
                                            { /* Path to property from an external document, within a named document object */
                                                (yyval.path) = ObjectIdentifier(DocumentObject);
                                                (yyval.path).setDocumentName(std::move((yyvsp[-4].string_or_identifier)), true);
                                                (yyval.path).setDocumentObjectName(std::move((yyvsp[-2].string_or_identifier)), true);
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                                (yyval.path).resolveAmbiguity();
                                            }
#line 1741 "Expression.tab.c"
    break;

  case 67: /* iden: document '#' object '.' STRING '.' id_or_cell  */
#line 210 "Expression.y"
                                            {   (yyval.path) = ObjectIdentifier(DocumentObject);
                                                (yyval.path).setDocumentName(std::move((yyvsp[-6].string_or_identifier)), true);
                                                (yyval.path).setDocumentObjectName(std::move((yyvsp[-4].string_or_identifier)), true, ObjectIdentifier::String(std::move((yyvsp[-2].string)),true));
                                                (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));
                                                (yyval.path).resolveAmbiguity();
                                            }
#line 1752 "Expression.tab.c"
    break;

  case 68: /* iden: iden '.' IDENTIFIER  */
#line 216 "Expression.y"
                                            { (yyval.path)= std::move((yyvsp[-2].path)); (yyval.path).addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string))); }
#line 1758 "Expression.tab.c"
    break;

  case 69: /* indexer: '[' exp ']'  */
#line 220 "Expression.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-1].expr));   }
#line 1764 "Expression.tab.c"
    break;

  case 70: /* indexer: '[' exp ':' ']'  */
#line 221 "Expression.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-2].expr),0,0,true); }
#line 1770 "Expression.tab.c"
    break;

  case 71: /* indexer: '[' ':' exp ']'  */
#line 222 "Expression.y"
                                            { (yyval.component) = Expression::createComponent(0,(yyvsp[-1].expr)); }
#line 1776 "Expression.tab.c"
    break;

  case 72: /* indexer: '[' ':' ':' exp ']'  */
#line 223 "Expression.y"
                                            { (yyval.component) = Expression::createComponent(0,0,(yyvsp[-1].expr)); }
#line 1782 "Expression.tab.c"
    break;

  case 73: /* indexer: '[' exp ':' exp ']'  */
#line 224 "Expression.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-3].expr),(yyvsp[-1].expr));}
#line 1788 "Expression.tab.c"
    break;

  case 74: /* indexer: '[' exp ':' ':' exp ']'  */
#line 225 "Expression.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-4].expr),0,(yyvsp[-1].expr)); }
#line 1794 "Expression.tab.c"
    break;

  case 75: /* indexer: '[' ':' exp ':' exp ']'  */
#line 226 "Expression.y"
                                            { (yyval.component) = Expression::createComponent(0,(yyvsp[-3].expr),(yyvsp[-1].expr)); }
#line 1800 "Expression.tab.c"
    break;

  case 76: /* indexer: '[' exp ':' exp ':' exp ']'  */
#line 227 "Expression.y"
                                            { (yyval.component) = Expression::createComponent((yyvsp[-5].expr),(yyvsp[-3].expr),(yyvsp[-1].expr));}
#line 1806 "Expression.tab.c"
    break;

  case 77: /* indexable: identifier indexer  */
#line 231 "Expression.y"
                                            { (yyval.expr) = new VariableExpression(DocumentObject,(yyvsp[-1].path)); (yyval.expr)->addComponent((yyvsp[0].component)); }
#line 1812 "Expression.tab.c"
    break;

  case 78: /* indexable: indexable indexer  */
#line 232 "Expression.y"
                                            { (yyvsp[-1].expr)->addComponent(std::move((yyvsp[0].component))); (yyval.expr) = (yyvsp[-1].expr); }
#line 1818 "Expression.tab.c"
    break;

  case 79: /* indexable: indexable '.' IDENTIFIER  */
#line 233 "Expression.y"
                                            { (yyvsp[-2].expr)->addComponent(Expression::createComponent((yyvsp[0].string))); (yyval.expr) = (yyvsp[-2].expr); }
#line 1824 "Expression.tab.c"
    break;

  case 80: /* document: STRING  */
#line 237 "Expression.y"
                                            { (yyval.string_or_identifier) = ObjectIdentifier::String(std::move((yyvsp[0].string)), true); }
#line 1830 "Expression.tab.c"
    break;

  case 81: /* document: IDENTIFIER  */
#line 238 "Expression.y"
                                            { (yyval.string_or_identifier) = ObjectIdentifier::String(std::move((yyvsp[0].string)), false, true);}
#line 1836 "Expression.tab.c"
    break;

  case 82: /* object: STRING  */
#line 242 "Expression.y"
                                            { (yyval.string_or_identifier) = ObjectIdentifier::String(std::move((yyvsp[0].string)), true); }
#line 1842 "Expression.tab.c"
    break;

  case 83: /* object: id_or_cell  */
#line 243 "Expression.y"
                                            { (yyval.string_or_identifier) = ObjectIdentifier::String(std::move((yyvsp[0].string)), false);}
#line 1848 "Expression.tab.c"
    break;


#line 1852 "Expression.tab.c"

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

#line 246 "Expression.y"

