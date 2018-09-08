/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 7 "ExpressionParser.y" /* yacc.c:339  */


#define YYSTYPE App::ExpressionParser::semantic_type

std::stack<FunctionExpression::Function> functions;                /**< Function identifier */

       //#define YYSTYPE yystype
       #define yyparse ExpressionParser_yyparse
       #define yyerror ExpressionParser_yyerror

#line 77 "ExpressionParser.tab.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "ExpressionParser.tab.h".  */
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
    FUNC = 258,
    ONE = 259,
    NUM = 260,
    IDENTIFIER = 261,
    UNIT = 262,
    INTEGER = 263,
    CONSTANT = 264,
    CELLADDRESS = 265,
    EQ = 266,
    NEQ = 267,
    LT = 268,
    GT = 269,
    GTE = 270,
    LTE = 271,
    STRING = 272,
    MINUSSIGN = 273,
    PROPERTY_REF = 274,
    DOCUMENT = 275,
    OBJECT = 276,
    EXPONENT = 277,
    NUM_AND_UNIT = 278,
    NEG = 279,
    POS = 280
  };
#endif

/* Value type.  */


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_EXPRESSIONPARSER_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 149 "ExpressionParser.tab.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

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

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
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


#if ! defined yyoverflow || YYERROR_VERBOSE

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
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
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
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
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
#define YYLAST   259

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  40
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  16
/* YYNRULES -- Number of rules.  */
#define YYNRULES  89
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  166

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   280

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    37,     2,     2,     2,     2,
      32,    33,    26,    25,    34,     2,    36,    27,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    24,    35,
       2,     2,     2,    23,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    38,     2,    39,    29,     2,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21,    22,    28,    30,
      31
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    68,    68,    69,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    89,
      90,    91,    92,    94,    95,    96,    97,    98,    99,   102,
     103,   106,   107,   108,   109,   112,   113,   114,   115,   116,
     117,   120,   121,   122,   123,   124,   125,   128,   132,   137,
     142,   147,   152,   157,   162,   170,   171,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   206,   207,   210,   211,   214,   215
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "FUNC", "ONE", "NUM", "IDENTIFIER",
  "UNIT", "INTEGER", "CONSTANT", "CELLADDRESS", "EQ", "NEQ", "LT", "GT",
  "GTE", "LTE", "STRING", "MINUSSIGN", "PROPERTY_REF", "DOCUMENT",
  "OBJECT", "EXPONENT", "'?'", "':'", "'+'", "'*'", "'/'", "NUM_AND_UNIT",
  "'^'", "NEG", "POS", "'('", "')'", "','", "';'", "'.'", "'#'", "'['",
  "']'", "$accept", "input", "exp", "num", "args", "args0", "range",
  "cond", "unit_exp", "identifier", "integer", "path", "subpath",
  "document", "object", "subname", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,    63,    58,    43,    42,    47,   278,    94,
     279,   280,    40,    41,    44,    59,    46,    35,    91,    93
};
# endif

#define YYPACT_NINF -58

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-58)))

#define YYTABLE_NINF -90

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      78,   155,   -58,   -58,   138,   -58,   -58,   -58,   -12,   -22,
     180,   180,    78,    60,     8,   230,    -4,     6,    63,   -58,
     -58,     1,    36,   -20,    33,   180,   230,   159,   -58,     3,
     100,   -58,   -58,   188,   122,    55,    39,   -58,   -58,    40,
     -58,   180,   180,   180,   180,   180,   180,   180,   180,   180,
      78,   180,    -4,    65,   180,    -4,    -4,    19,   101,    60,
      24,    26,   -58,   155,   155,   121,    99,    39,    69,   -58,
      80,   -58,    82,   -58,    83,    89,   -58,   -58,    16,    29,
     230,   230,   230,   230,   230,   230,    86,    86,   118,   118,
      65,   -58,   211,    65,    65,    54,   -58,   -58,   -58,   107,
     -58,   120,   -58,   -58,   -58,   -58,   230,   -58,   230,   -58,
     -58,   174,   114,    16,   128,    29,   126,   131,   133,   -58,
     -58,   -58,   -58,   180,   -58,    29,    29,   -58,    16,   162,
     -58,   132,   139,   142,   -58,    16,    16,    16,   115,   -58,
     -58,   -58,   143,   156,   157,   141,   175,   182,   -58,   -58,
     -58,   183,   184,   192,    16,    16,    16,    16,    16,    16,
     -58,   -58,   -58,   -58,   -58,   -58
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,    19,    20,    57,    41,    21,    22,    58,     6,
       0,     0,     0,     0,     0,     2,     4,     0,     3,     7,
      47,     0,     0,    57,    58,     0,    23,     0,    24,     0,
       0,     8,     9,     0,     0,    57,    58,    88,    49,     0,
       1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     5,     0,     0,     0,     0,     0,     0,
       0,     0,    17,     0,     0,     0,    66,    68,    67,    65,
       0,    56,     0,    55,     0,     0,    16,    46,     0,     0,
      35,    36,    37,    38,    39,    40,    11,    10,    12,    13,
      14,    15,     0,    43,    42,     0,    44,    86,    53,     0,
      52,     0,    34,    33,    32,    31,    25,    27,    26,    28,
      29,     0,    83,     0,     0,     0,    62,    61,    59,    68,
      67,    58,    48,     0,    45,     0,     0,    30,     0,     0,
      75,     0,     0,     0,    50,     0,     0,     0,    18,    54,
      51,    82,     0,     0,     0,    72,    71,    69,    64,    63,
      60,    80,    78,    76,     0,     0,     0,     0,     0,     0,
      74,    73,    70,    81,    79,    77
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -58,   -58,     0,   -58,   145,   -58,     4,   -58,     5,   -58,
     -55,    -6,   -57,   -58,     7,   -28
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    14,    26,    16,    27,   112,    28,    17,    34,    19,
      75,    20,    69,    21,    22,    39
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      15,    70,    96,     5,    60,    18,    65,    38,    40,    66,
      31,    32,    33,    67,   -86,   -84,    29,   -85,    30,    65,
      68,    53,    66,    71,   -87,    33,   119,    73,    52,    54,
     102,   101,   104,   120,   103,    35,   105,    95,    58,   121,
     124,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    91,    98,   100,    92,    90,   130,    61,    71,   133,
      93,    94,    73,   106,   108,    99,    35,   107,   109,   -87,
      36,   141,    59,   122,   144,   -89,    79,    37,   148,   149,
     150,     1,     2,     3,     4,     5,     6,     7,     8,    55,
      56,    78,    57,    30,    57,     9,    10,   160,   161,   162,
     163,   164,   165,    11,    71,   -88,    72,    35,    73,   134,
      12,     8,    49,    50,    13,    51,   115,    74,    97,   139,
     140,   116,   117,   138,     1,     2,     3,    23,   118,     6,
       7,    24,    71,    47,   131,   113,    73,   114,     9,    10,
      48,    49,    50,   125,    51,   132,    11,    51,    55,    56,
     128,    57,   129,    25,   110,    77,   126,    13,     1,     2,
       3,    23,   135,     6,     7,    24,    71,   136,   142,   137,
      73,   145,     9,    10,    29,   -85,    30,   154,   146,   143,
      11,   147,   151,     1,     2,     3,     4,    25,     6,     7,
       8,    13,    62,    63,    64,   152,   153,     9,    10,    41,
      42,    43,    44,    45,    46,    11,    47,   127,    63,    64,
     111,   155,    25,    48,    49,    50,    13,    51,   156,   157,
     158,    76,    41,    42,    43,    44,    45,    46,   159,    47,
       0,     0,     0,     0,     0,   123,    48,    49,    50,     0,
      51,    41,    42,    43,    44,    45,    46,     0,    47,     0,
       0,     0,     0,     0,     0,    48,    49,    50,     0,    51
};

static const yytype_int16 yycheck[] =
{
       0,    29,    57,     7,    24,     0,     3,    13,     0,     6,
      10,    11,    12,    10,    36,    37,    36,    37,    38,     3,
      17,    16,     6,     4,    36,    25,    10,     8,    32,    23,
       6,    59,     6,    17,    10,     6,    10,    18,    37,    10,
      95,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    58,    59,    54,    50,   113,    24,     4,   114,
      55,    56,     8,    63,    64,    58,     6,    63,    64,    36,
      10,   128,    36,    79,   129,    36,    36,    17,   135,   136,
     137,     3,     4,     5,     6,     7,     8,     9,    10,    26,
      27,    36,    29,    38,    29,    17,    18,   154,   155,   156,
     157,   158,   159,    25,     4,    36,     6,     6,     8,   115,
      32,    10,    26,    27,    36,    29,    36,    17,    17,   125,
     126,    39,    39,   123,     3,     4,     5,     6,    39,     8,
       9,    10,     4,    18,     6,    36,     8,    38,    17,    18,
      25,    26,    27,    36,    29,    17,    25,    29,    26,    27,
      36,    29,    38,    32,    33,    33,    36,    36,     3,     4,
       5,     6,    36,     8,     9,    10,     4,    36,     6,    36,
       8,    39,    17,    18,    36,    37,    38,    36,    39,    17,
      25,    39,    39,     3,     4,     5,     6,    32,     8,     9,
      10,    36,    33,    34,    35,    39,    39,    17,    18,    11,
      12,    13,    14,    15,    16,    25,    18,    33,    34,    35,
      65,    36,    32,    25,    26,    27,    36,    29,    36,    36,
      36,    33,    11,    12,    13,    14,    15,    16,    36,    18,
      -1,    -1,    -1,    -1,    -1,    24,    25,    26,    27,    -1,
      29,    11,    12,    13,    14,    15,    16,    -1,    18,    -1,
      -1,    -1,    -1,    -1,    -1,    25,    26,    27,    -1,    29
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    17,
      18,    25,    32,    36,    41,    42,    43,    47,    48,    49,
      51,    53,    54,     6,    10,    32,    42,    44,    46,    36,
      38,    42,    42,    42,    48,     6,    10,    17,    51,    55,
       0,    11,    12,    13,    14,    15,    16,    18,    25,    26,
      27,    29,    32,    48,    23,    26,    27,    29,    37,    36,
      24,    24,    33,    34,    35,     3,     6,    10,    17,    52,
      55,     4,     6,     8,    17,    50,    33,    33,    36,    36,
      42,    42,    42,    42,    42,    42,    42,    42,    42,    42,
      48,    42,    42,    48,    48,    18,    50,    17,    51,    54,
      51,    55,     6,    10,     6,    10,    42,    46,    42,    46,
      33,    44,    45,    36,    38,    36,    39,    39,    39,    10,
      17,    10,    51,    24,    50,    36,    36,    33,    36,    38,
      52,     6,    17,    50,    51,    36,    36,    36,    42,    51,
      51,    52,     6,    17,    50,    39,    39,    39,    52,    52,
      52,    39,    39,    39,    36,    36,    36,    36,    36,    36,
      52,    52,    52,    52,    52,    52
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    40,    41,    41,    42,    42,    42,    42,    42,    42,
      42,    42,    42,    42,    42,    42,    42,    42,    42,    43,
      43,    43,    43,    44,    44,    44,    44,    44,    44,    45,
      45,    46,    46,    46,    46,    47,    47,    47,    47,    47,
      47,    48,    48,    48,    48,    48,    48,    49,    49,    49,
      49,    49,    49,    49,    49,    50,    50,    51,    51,    51,
      51,    51,    51,    51,    51,    51,    52,    52,    52,    52,
      52,    52,    52,    52,    52,    52,    52,    52,    52,    52,
      52,    52,    52,    52,    53,    53,    54,    54,    55,    55
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     2,     1,     1,     2,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     5,     1,
       1,     1,     1,     1,     1,     3,     3,     3,     3,     1,
       2,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     1,     3,     3,     3,     4,     3,     1,     4,     2,
       5,     5,     3,     3,     5,     1,     1,     1,     1,     4,
       6,     4,     4,     6,     6,     3,     1,     1,     1,     4,
       6,     4,     4,     6,     6,     3,     5,     7,     5,     7,
       5,     7,     4,     2,     1,     1,     1,     1,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
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

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



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

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
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
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yytype)
    {
          case 42: /* exp  */
#line 61 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1097 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 44: /* args  */
#line 62 "ExpressionParser.y" /* yacc.c:1257  */
      { std::vector<Expression*>::const_iterator i = ((*yyvaluep).arguments).begin(); while (i != ((*yyvaluep).arguments).end()) { delete *i; ++i; } }
#line 1103 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 47: /* cond  */
#line 61 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1109 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 48: /* unit_exp  */
#line 61 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1115 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;


      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
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
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
| yyreduce -- Do a reduction.  |
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
        case 2:
#line 68 "ExpressionParser.y" /* yacc.c:1646  */
    { ScanResult = (yyvsp[0].expr); valueExpression = true;                                        }
#line 1379 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 69 "ExpressionParser.y" /* yacc.c:1646  */
    { ScanResult = (yyvsp[0].expr); unitExpression = true;                                         }
#line 1385 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 72 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1391 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 73 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-1].expr), OperatorExpression::UNIT, (yyvsp[0].expr));  }
#line 1397 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 74 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new StringExpression(DocumentObject, (yyvsp[0].string));                                  }
#line 1403 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 75 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new VariableExpression(DocumentObject, (yyvsp[0].path));                                }
#line 1409 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 76 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))); }
#line 1415 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 77 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::POS, new NumberExpression(DocumentObject, Quantity(1))); }
#line 1421 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 78 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::ADD, (yyvsp[0].expr));   }
#line 1427 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 79 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::SUB, (yyvsp[0].expr));   }
#line 1433 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 80 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 1439 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 81 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1445 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 82 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1451 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 83 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, (yyvsp[0].expr));   }
#line 1457 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 84 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr);                                                                        }
#line 1463 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 85 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new FunctionExpression(DocumentObject, (yyvsp[-2].func).second, (yyvsp[-1].arguments));                   }
#line 1469 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 86 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ConditionalExpression(DocumentObject, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr));                     }
#line 1475 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 89 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 1481 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 90 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 1487 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 91 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue)));                }
#line 1493 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 92 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ConstantExpression(DocumentObject, (yyvsp[0].constant).name, Quantity((yyvsp[0].constant).fvalue));      }
#line 1499 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 94 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.arguments).push_back((yyvsp[0].expr));                                                               }
#line 1505 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 95 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.arguments).push_back((yyvsp[0].expr));                                                               }
#line 1511 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 96 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1517 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 97 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1523 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 98 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1529 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 99 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].arguments).push_back((yyvsp[0].expr));  (yyval.arguments) = (yyvsp[-2].arguments);                                                     }
#line 1535 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 102 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.arguments).clear(); }
#line 1541 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 103 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.arguments) = (yyvsp[-1].arguments); }
#line 1547 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 106 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new RangeExpression(DocumentObject, (yyvsp[-2].string), (yyvsp[0].string));                               }
#line 1553 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 107 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new RangeExpression(DocumentObject, (yyvsp[-2].string), (yyvsp[0].string));                               }
#line 1559 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 108 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new RangeExpression(DocumentObject, (yyvsp[-2].string), (yyvsp[0].string));                               }
#line 1565 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 109 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new RangeExpression(DocumentObject, (yyvsp[-2].string), (yyvsp[0].string));                               }
#line 1571 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 112 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::EQ, (yyvsp[0].expr));    }
#line 1577 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 113 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::NEQ, (yyvsp[0].expr));   }
#line 1583 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 114 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LT, (yyvsp[0].expr));    }
#line 1589 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 115 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GT, (yyvsp[0].expr));    }
#line 1595 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 116 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GTE, (yyvsp[0].expr));   }
#line 1601 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 117 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LTE, (yyvsp[0].expr));   }
#line 1607 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 120 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 1613 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 121 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1619 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 122 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 1625 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 123 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))));   }
#line 1631 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 124 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-3].expr), OperatorExpression::POW, new OperatorExpression(DocumentObject, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))));   }
#line 1637 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 125 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr);                                                                        }
#line 1643 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 128 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property within document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 1652 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 132 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of a sub-object of the current object*/
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentObjectName(DocumentObject,false,(yyvsp[-2].string_or_identifier));
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 1662 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 137 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of the current document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentObjectName(DocumentObject);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 1672 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 142 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of a sub-object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentObjectName(ObjectIdentifier::String((yyvsp[-4].string),false,true), true, (yyvsp[-2].string_or_identifier));
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 1682 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 147 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of a sub-object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentObjectName((yyvsp[-4].string_or_identifier), true, (yyvsp[-2].string_or_identifier));
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 1692 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 152 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property within document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentObjectName((yyvsp[-2].string_or_identifier), true);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 1702 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 157 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property from an external document, within a named document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentName((yyvsp[-2].string_or_identifier), true);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 1712 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 162 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property from an external document, within a named document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentName((yyvsp[-4].string_or_identifier), true);
                                                  (yyval.path).setDocumentObjectName((yyvsp[-2].string_or_identifier), true);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 1723 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 170 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.ivalue) = (yyvsp[0].ivalue); }
#line 1729 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 171 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.ivalue) = (yyvsp[0].fvalue); }
#line 1735 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 175 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::SimpleComponent((yyvsp[0].string)));                         }
#line 1741 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 176 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::SimpleComponent((yyvsp[0].string)));                         }
#line 1747 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 177 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::ArrayComponent((yyvsp[-3].string), (yyvsp[-1].ivalue)));                      }
#line 1753 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 178 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[0].components).push_front(ObjectIdentifier::Component::ArrayComponent((yyvsp[-5].string), (yyvsp[-3].ivalue))); (yyval.components) = (yyvsp[0].components);             }
#line 1759 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 179 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::MapComponent((yyvsp[-3].string), ObjectIdentifier::String((yyvsp[-1].string), true)));          }
#line 1765 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 180 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::MapComponent((yyvsp[-3].string), (yyvsp[-1].string)));                        }
#line 1771 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 181 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[0].components).push_front(ObjectIdentifier::Component::MapComponent((yyvsp[-5].string), ObjectIdentifier::String((yyvsp[-3].string), true))); (yyval.components) = (yyvsp[0].components); }
#line 1777 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 182 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[0].components).push_front(ObjectIdentifier::Component::MapComponent((yyvsp[-5].string), (yyvsp[-3].string))); (yyval.components) = (yyvsp[0].components);               }
#line 1783 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 65:
#line 183 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[0].components).push_front(ObjectIdentifier::Component::SimpleComponent((yyvsp[-2].string))); (yyval.components) = (yyvsp[0].components);                }
#line 1789 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 186 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::SimpleComponent((yyvsp[0].string)));                         }
#line 1795 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 187 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::SimpleComponent((yyvsp[0].string)));                         }
#line 1801 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 68:
#line 188 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::SimpleComponent((yyvsp[0].string)));                         }
#line 1807 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 189 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::ArrayComponent((yyvsp[-3].string), (yyvsp[-1].ivalue)));                      }
#line 1813 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 190 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[0].components).push_front(ObjectIdentifier::Component::ArrayComponent((yyvsp[-5].string), (yyvsp[-3].ivalue))); (yyval.components) = (yyvsp[0].components);             }
#line 1819 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 191 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::MapComponent((yyvsp[-3].string), ObjectIdentifier::String((yyvsp[-1].string), true)));          }
#line 1825 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 192 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::MapComponent((yyvsp[-3].string), (yyvsp[-1].string)));                        }
#line 1831 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 193 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[0].components).push_front(ObjectIdentifier::Component::MapComponent((yyvsp[-5].string), ObjectIdentifier::String((yyvsp[-3].string), true))); (yyval.components) = (yyvsp[0].components); }
#line 1837 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 194 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[0].components).push_front(ObjectIdentifier::Component::MapComponent((yyvsp[-5].string), (yyvsp[-3].string))); (yyval.components) = (yyvsp[0].components);               }
#line 1843 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 195 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[0].components).push_front(ObjectIdentifier::Component::SimpleComponent((yyvsp[-2].string))); (yyval.components) = (yyvsp[0].components);                }
#line 1849 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 76:
#line 196 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::CallableComponent((yyvsp[-4].func).first, (yyvsp[-3].arguments), (yyvsp[-1].ivalue)));}
#line 1855 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 77:
#line 197 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[0].components).push_front(ObjectIdentifier::Component::CallableComponent((yyvsp[-6].func).first, (yyvsp[-5].arguments), (yyvsp[-3].ivalue))); (yyval.components) = (yyvsp[0].components);}
#line 1861 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 198 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::CallableComponent((yyvsp[-4].func).first, (yyvsp[-3].arguments), ObjectIdentifier::String((yyvsp[-1].string), true)));          }
#line 1867 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 199 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[0].components).push_front(ObjectIdentifier::Component::CallableComponent((yyvsp[-6].func).first, (yyvsp[-5].arguments), ObjectIdentifier::String((yyvsp[-3].string), true))); (yyval.components) = (yyvsp[0].components); }
#line 1873 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 80:
#line 200 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::CallableComponent((yyvsp[-4].func).first, (yyvsp[-3].arguments), (yyvsp[-1].string)));}
#line 1879 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 201 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[0].components).push_front(ObjectIdentifier::Component::CallableComponent((yyvsp[-6].func).first, (yyvsp[-5].arguments), (yyvsp[-3].string))); (yyval.components) = (yyvsp[0].components);}
#line 1885 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 202 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[0].components).push_front(ObjectIdentifier::Component::CallableComponent((yyvsp[-3].func).first,(yyvsp[-2].arguments))); (yyval.components) = (yyvsp[0].components);}
#line 1891 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 203 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_front(ObjectIdentifier::Component::CallableComponent((yyvsp[-1].func).first,(yyvsp[0].arguments)));}
#line 1897 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 206 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 1903 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 207 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string));       }
#line 1909 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 210 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 1915 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 87:
#line 211 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 1921 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 88:
#line 214 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 1927 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 215 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 1933 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;


#line 1937 "ExpressionParser.tab.c" /* yacc.c:1646  */
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
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

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

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
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
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
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
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 218 "ExpressionParser.y" /* yacc.c:1906  */

