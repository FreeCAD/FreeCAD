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

       //#define YYSTYPE yystype
       #define yyparse ExpressionParser_yyparse
       #define yyerror ExpressionParser_yyerror

#line 75 "ExpressionParser.tab.c" /* yacc.c:339  */

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
    RSTRING = 275,
    STRING = 276,
    MINUSSIGN = 277,
    PROPERTY_REF = 278,
    DOCUMENT = 279,
    OBJECT = 280,
    EXPONENT = 281,
    NUM_AND_UNIT = 282,
    NEG = 283,
    POS = 284
  };
#endif

/* Value type.  */


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_EXPRESSIONPARSER_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 151 "ExpressionParser.tab.c" /* yacc.c:358  */

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
#define YYFINAL  61
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   636

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  48
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  36
/* YYNRULES -- Number of rules.  */
#define YYNRULES  118
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  201

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   284

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    47,     2,    32,     2,     2,
      37,    38,    30,    29,    40,     2,    39,    31,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    28,    41,
       2,    46,     2,    27,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    42,     2,    43,    34,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    44,     2,    45,     2,     2,     2,     2,
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
      25,    26,    33,    35,    36
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    67,    67,    68,    72,    73,    74,    75,    76,    77,
      78,    81,    82,    83,    84,    85,    86,    89,    90,    96,
     105,   106,   107,   108,   109,   110,   114,   115,   116,   120,
     121,   124,   125,   126,   127,   128,   132,   133,   134,   138,
     139,   140,   141,   142,   146,   147,   148,   152,   153,   154,
     159,   160,   161,   165,   166,   170,   172,   173,   176,   176,
     178,   179,   182,   183,   184,   185,   188,   189,   192,   193,
     194,   197,   198,   201,   202,   203,   206,   207,   208,   209,
     210,   211,   214,   215,   218,   219,   220,   221,   224,   227,
     228,   229,   230,   231,   232,   235,   239,   248,   253,   265,
     273,   282,   283,   286,   287,   290,   291,   292,   295,   296,
     297,   298,   299,   300,   303,   304,   307,   308,   311
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ONE", "NUM", "IDENTIFIER", "UNIT",
  "INTEGER", "CONSTANT", "CELLADDRESS", "EQ", "NEQ", "LT", "GT", "GTE",
  "LTE", "AND_OP", "AND_OP2", "OR_OP", "OR_OP2", "RSTRING", "STRING",
  "MINUSSIGN", "PROPERTY_REF", "DOCUMENT", "OBJECT", "EXPONENT", "'?'",
  "':'", "'+'", "'*'", "'/'", "'%'", "NUM_AND_UNIT", "'^'", "NEG", "POS",
  "'('", "')'", "'.'", "','", "';'", "'['", "']'", "'{'", "'}'", "'='",
  "'#'", "$accept", "input", "primary_exp", "indexable", "callable",
  "unary_exp", "power_exp", "multiply_exp", "additive_exp",
  "relational_exp", "equality_exp", "logical_and_exp", "logical_or_exp",
  "conditional_exp", "exp", "id_or_cell", "sep", "items2", "tuple",
  "items", "list", "dict1", "dict", "arg", "args", "num", "range",
  "unit_exp", "identifier", "integer", "path", "subpath", "indexer",
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
     275,   276,   277,   278,   279,   280,   281,    63,    58,    43,
      42,    47,    37,   282,    94,   283,   284,    40,    41,    46,
      44,    59,    91,    93,   123,   125,    61,    35
};
# endif

#define YYPACT_NINF -94

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-94)))

#define YYTABLE_NINF -118

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     565,   -94,   -94,   -94,   -35,   -94,   -94,   -94,   -94,    36,
     592,   592,   193,    26,   373,   123,    25,   -94,    64,    70,
     -94,    -7,   146,    35,   121,   100,   125,    49,   -94,   -94,
      52,   -94,   -94,    76,   -94,     5,    48,     3,    27,    -2,
      45,   400,   -35,   220,   -94,   -94,   -94,   -94,   108,    75,
     151,    43,   -94,   -94,    58,    81,   -94,   -94,   110,   -94,
     119,   -94,   247,   113,    23,   -94,   289,   113,   -94,   592,
     592,   565,   592,   592,   592,   592,   592,   592,   592,   592,
     592,   592,   592,   592,   592,   592,   -94,   166,   -94,     5,
     120,     5,     5,    15,   331,   -94,    41,    26,   442,   -94,
      24,   -94,   178,   -94,   -94,   484,   -94,   -94,   511,   -94,
     113,   -94,   113,   -94,   538,   592,   -94,   198,   -94,   -94,
     -94,   118,   116,   142,   -20,   -94,   205,   -94,   -94,    -7,
      -7,   120,    -7,   146,   146,    35,    35,    35,    35,   121,
     121,   100,   100,   125,   125,   135,   -94,   138,   120,   120,
     116,   -94,   -94,   225,   -94,   -94,   165,    58,   167,   592,
     -94,   -18,   -94,   113,   592,   -94,   400,   -94,   -94,   -94,
     -94,   -94,    58,   -94,   -94,   -94,   -94,   -94,   147,   -94,
      10,   -94,   -94,   592,   592,   -94,   -94,   113,   113,   -94,
     -94,   -94,   -94,   -94,   -94,   164,   -94,   -94,    58,    58,
     -94
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    84,    85,    56,    89,    86,    87,    57,     7,     6,
       0,     0,     0,     0,     0,     0,     0,    26,     9,    10,
      29,    31,    36,    39,    44,    47,    50,    53,    55,     2,
     103,    12,    13,     0,    14,     4,     3,     8,    95,     0,
       0,     0,     0,     0,    27,    28,    58,    59,     0,     0,
       0,     0,   118,   105,    97,     0,    68,    66,     0,    73,
       0,     1,     0,     0,     0,    15,     0,     0,    24,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    74,     0,    89,     0,
       5,     0,     0,     0,     0,   104,     0,     0,     0,    76,
     103,    82,     0,    79,    11,     0,    62,    64,     0,    94,
       0,   107,     0,    69,     0,     0,    20,     0,    16,   102,
     101,     0,     0,     0,     0,    22,     0,    25,    30,    32,
      33,    34,    35,    38,    37,    40,    41,    42,    43,    45,
      46,    48,    49,    51,    52,     0,    75,     0,    91,    90,
       0,    92,    17,     0,   116,   117,     0,    99,     0,     0,
      78,   103,    80,     0,     0,    19,     0,    63,    60,    65,
      61,   106,    96,    70,    67,    71,    21,   112,     0,   113,
       0,   108,    23,     0,     0,    93,    18,     0,     0,    81,
      88,    77,    83,   110,   109,     0,    54,    72,   100,    98,
     111
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -94,   -94,   -94,   -94,   -94,    -4,   111,    51,    62,    77,
     112,   137,   -94,    29,     0,    -8,    -9,   -94,   -94,   -94,
     -94,   -94,   -94,    65,   -46,   -94,   149,     1,   -94,   -84,
     -94,   -93,    42,   -94,   148,   161
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    99,    30,   166,    50,    31,    58,
      32,    33,    34,   101,   102,    35,   103,    51,    37,   124,
      38,    54,   111,    39,    40,    55
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      29,    36,    41,    49,   157,    53,    44,    45,   180,   151,
     163,    88,    48,   119,    57,    60,   117,   120,   119,   172,
     126,  -117,   120,   181,    87,    61,   119,    69,     3,  -115,
     120,     3,     7,   100,    49,     7,    90,   150,   178,   105,
      94,   108,    89,    48,   121,    96,     3,    52,   153,   114,
       7,   122,   163,   194,   100,   118,   123,    73,   100,   127,
      65,    68,   154,  -117,    74,   128,   185,    83,    84,    64,
     164,  -115,   131,    91,    92,  -116,    85,    93,    91,    92,
      95,   109,    93,  -114,    97,   145,   100,   147,   155,    53,
     161,  -117,   148,   149,   198,   199,   195,   110,   160,  -115,
      64,    62,   171,    63,    53,   168,    64,    66,   170,    67,
      79,    80,    64,   106,   174,   175,    46,    47,     3,   119,
     112,    86,     7,   120,   133,   134,     1,     2,     3,    42,
       5,     6,     7,    75,    76,    77,    78,   135,   136,   137,
     138,    81,    82,     8,     9,    10,   104,   115,    46,    47,
      46,    47,    11,   113,    93,   190,   139,   140,   100,   189,
      43,   177,    13,   183,   191,    14,   184,    15,    59,     1,
       2,     3,    42,     5,     6,     7,    70,    71,    72,    53,
      53,   129,   130,   132,   197,   179,     8,     9,    10,   107,
     193,    46,    47,   141,   142,    11,     1,     2,     3,     4,
       5,     6,     7,    43,   187,    13,   188,   200,    14,     0,
      15,   146,   196,     8,     9,    10,   165,     0,    46,    47,
     143,   144,    11,     1,     2,     3,    42,     5,     6,     7,
      12,   192,    13,    46,    47,    14,   176,    15,    46,    47,
       8,     9,    10,   182,   156,    46,    47,   162,     0,    11,
       1,     2,     3,    42,     5,     6,     7,    43,   158,    13,
      46,    47,    14,   186,    15,    46,    47,     8,     9,    10,
       0,     0,     0,     0,     0,     0,    11,    98,     0,     0,
       0,     0,     0,     0,    43,   116,    13,     0,     0,    14,
       0,    15,     1,     2,     3,    42,     5,     6,     7,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,     0,     0,     0,     0,     0,     0,    11,    98,
       0,     0,     0,     0,     0,     0,    43,   125,    13,     0,
       0,    14,     0,    15,     1,     2,     3,    42,     5,     6,
       7,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,     0,     0,     0,     0,     0,     0,
      11,    98,     0,     0,     0,     0,     0,     0,    43,   152,
      13,     0,     0,    14,     0,    15,     1,     2,     3,    42,
       5,     6,     7,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     8,     9,    10,     0,     0,     0,     0,
       0,     0,    11,     1,     2,     3,    42,     5,     6,     7,
      43,     0,    13,     0,     0,    14,    56,    15,     0,     0,
       8,     9,    10,     0,     0,     0,     0,     0,     0,    11,
      98,     0,     0,     0,     0,     0,     0,    43,     0,    13,
       0,     0,    14,     0,    15,     1,     2,     3,    42,     5,
       6,     7,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     8,     9,    10,     0,     0,     0,     0,     0,
       0,    11,   159,     0,     0,     0,     0,     0,     0,    43,
       0,    13,     0,     0,    14,     0,    15,     1,     2,     3,
      42,     5,     6,     7,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     8,     9,    10,     0,     0,     0,
       0,     0,     0,    11,     1,     2,     3,    42,     5,     6,
       7,    43,   167,    13,     0,     0,    14,     0,    15,     0,
       0,     8,     9,    10,     0,     0,     0,     0,     0,     0,
      11,     1,     2,     3,    42,     5,     6,     7,    43,   169,
      13,     0,     0,    14,     0,    15,     0,     0,     8,     9,
      10,     0,     0,     0,     0,     0,     0,    11,     1,     2,
       3,     4,     5,     6,     7,    43,     0,    13,     0,     0,
      14,   173,    15,     0,     0,     8,     9,    10,     0,     0,
       0,     0,     0,     0,    11,     1,     2,     3,    42,     5,
       6,     7,    12,     0,    13,     0,     0,    14,     0,    15,
       0,     0,     8,     9,    10,     0,     0,     0,     0,     0,
       0,    11,     0,     0,     0,     0,     0,     0,     0,    43,
       0,    13,     0,     0,    14,     0,    15
};

static const yytype_int16 yycheck[] =
{
       0,     0,    37,    12,    97,    13,    10,    11,    28,    93,
      28,     6,    12,     3,    14,    15,    62,     7,     3,   112,
      66,    39,     7,    43,    33,     0,     3,    34,     5,    47,
       7,     5,     9,    41,    43,     9,    35,    22,   122,    48,
      37,    50,    37,    43,    21,    47,     5,    21,    94,    58,
       9,    28,    28,    43,    62,    63,    64,    22,    66,    67,
      18,    19,    21,    39,    29,    69,   150,    18,    19,    42,
      46,    47,    71,    30,    31,    39,    27,    34,    30,    31,
      38,    38,    34,    47,    39,    85,    94,    87,    96,    97,
      98,    39,    91,    92,   187,   188,   180,    39,    98,    47,
      42,    37,   110,    39,   112,   105,    42,    37,   108,    39,
      10,    11,    42,    38,   114,   115,    40,    41,     5,     3,
      39,    45,     9,     7,    73,    74,     3,     4,     5,     6,
       7,     8,     9,    12,    13,    14,    15,    75,    76,    77,
      78,    16,    17,    20,    21,    22,    38,    28,    40,    41,
      40,    41,    29,    43,    34,   163,    79,    80,   166,   159,
      37,    43,    39,    28,   164,    42,    28,    44,    45,     3,
       4,     5,     6,     7,     8,     9,    30,    31,    32,   187,
     188,    70,    71,    72,   184,    43,    20,    21,    22,    38,
      43,    40,    41,    81,    82,    29,     3,     4,     5,     6,
       7,     8,     9,    37,    39,    39,    39,    43,    42,    -1,
      44,    45,   183,    20,    21,    22,    38,    -1,    40,    41,
      83,    84,    29,     3,     4,     5,     6,     7,     8,     9,
      37,   166,    39,    40,    41,    42,    38,    44,    40,    41,
      20,    21,    22,    38,    96,    40,    41,    98,    -1,    29,
       3,     4,     5,     6,     7,     8,     9,    37,    97,    39,
      40,    41,    42,    38,    44,    40,    41,    20,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    -1,    -1,
      -1,    -1,    -1,    -1,    37,    38,    39,    -1,    -1,    42,
      -1,    44,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,
      -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    39,    -1,
      -1,    42,    -1,    44,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    20,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      29,    30,    -1,    -1,    -1,    -1,    -1,    -1,    37,    38,
      39,    -1,    -1,    42,    -1,    44,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    20,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    29,     3,     4,     5,     6,     7,     8,     9,
      37,    -1,    39,    -1,    -1,    42,    43,    44,    -1,    -1,
      20,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    29,
      30,    -1,    -1,    -1,    -1,    -1,    -1,    37,    -1,    39,
      -1,    -1,    42,    -1,    44,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    29,    30,    -1,    -1,    -1,    -1,    -1,    -1,    37,
      -1,    39,    -1,    -1,    42,    -1,    44,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    20,    21,    22,    -1,    -1,    -1,
      -1,    -1,    -1,    29,     3,     4,     5,     6,     7,     8,
       9,    37,    38,    39,    -1,    -1,    42,    -1,    44,    -1,
      -1,    20,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      29,     3,     4,     5,     6,     7,     8,     9,    37,    38,
      39,    -1,    -1,    42,    -1,    44,    -1,    -1,    20,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    29,     3,     4,
       5,     6,     7,     8,     9,    37,    -1,    39,    -1,    -1,
      42,    43,    44,    -1,    -1,    20,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    29,     3,     4,     5,     6,     7,
       8,     9,    37,    -1,    39,    -1,    -1,    42,    -1,    44,
      -1,    -1,    20,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    29,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,
      -1,    39,    -1,    -1,    42,    -1,    44
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    20,    21,
      22,    29,    37,    39,    42,    44,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    66,    68,    69,    70,    73,    75,    76,    78,    81,
      82,    37,     6,    37,    53,    53,    40,    41,    62,    64,
      65,    75,    21,    63,    79,    83,    43,    62,    67,    45,
      62,     0,    37,    39,    42,    80,    37,    39,    80,    34,
      30,    31,    32,    22,    29,    12,    13,    14,    15,    10,
      11,    16,    17,    18,    19,    27,    45,    64,     6,    37,
      75,    30,    31,    34,    37,    80,    47,    39,    30,    62,
      63,    71,    72,    74,    38,    64,    38,    38,    64,    38,
      39,    80,    39,    43,    64,    28,    38,    72,    63,     3,
       7,    21,    28,    63,    77,    38,    72,    63,    53,    54,
      54,    75,    54,    55,    55,    56,    56,    56,    56,    57,
      57,    58,    58,    59,    59,    62,    45,    62,    75,    75,
      22,    77,    38,    72,    21,    63,    82,    79,    83,    30,
      62,    63,    74,    28,    46,    38,    64,    38,    62,    38,
      62,    63,    79,    43,    62,    62,    38,    43,    77,    43,
      28,    43,    38,    28,    28,    77,    38,    39,    39,    62,
      63,    62,    71,    43,    43,    77,    61,    62,    79,    79,
      43
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    48,    49,    49,    50,    50,    50,    50,    50,    50,
      50,    51,    51,    51,    51,    51,    51,    52,    52,    52,
      52,    52,    52,    52,    52,    52,    53,    53,    53,    54,
      54,    55,    55,    55,    55,    55,    56,    56,    56,    57,
      57,    57,    57,    57,    58,    58,    58,    59,    59,    59,
      60,    60,    60,    61,    61,    62,    63,    63,    64,    64,
      65,    65,    66,    66,    66,    66,    67,    67,    68,    68,
      68,    69,    69,    70,    70,    70,    71,    71,    71,    71,
      71,    71,    72,    72,    73,    73,    73,    73,    74,    75,
      75,    75,    75,    75,    75,    76,    76,    76,    76,    76,
      76,    77,    77,    78,    78,    79,    79,    79,    80,    80,
      80,    80,    80,    80,    81,    81,    82,    82,    83
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     2,     1,     1,     1,     1,
       1,     3,     1,     1,     1,     2,     3,     3,     4,     4,
       3,     4,     3,     4,     2,     3,     1,     2,     2,     1,
       3,     1,     3,     3,     3,     3,     1,     3,     3,     1,
       3,     3,     3,     3,     1,     3,     3,     1,     3,     3,
       1,     3,     3,     1,     5,     1,     1,     1,     1,     1,
       3,     3,     3,     4,     3,     4,     1,     3,     2,     3,
       4,     4,     5,     2,     2,     3,     1,     3,     2,     1,
       2,     3,     1,     3,     1,     1,     1,     1,     3,     1,
       3,     3,     3,     4,     3,     1,     4,     2,     5,     3,
       5,     1,     1,     1,     2,     1,     3,     2,     3,     4,
       4,     5,     3,     3,     1,     1,     1,     1,     1
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
          case 49: /* input  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1206 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 50: /* primary_exp  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1212 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 51: /* indexable  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1218 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 52: /* callable  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1224 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 53: /* unary_exp  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1230 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 54: /* power_exp  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1236 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 55: /* multiply_exp  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1242 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 56: /* additive_exp  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1248 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 57: /* relational_exp  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1254 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 58: /* equality_exp  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1260 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 59: /* logical_and_exp  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1266 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 60: /* logical_or_exp  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1272 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 61: /* conditional_exp  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1278 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 62: /* exp  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1284 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 65: /* items2  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { for(auto expr : ((*yyvaluep).arguments)) {delete expr;} }
#line 1290 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 66: /* tuple  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1296 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 67: /* items  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { for(auto expr : ((*yyvaluep).arguments)) {delete expr;} }
#line 1302 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 68: /* list  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1308 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 69: /* dict1  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1314 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 70: /* dict  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1320 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 71: /* arg  */
#line 61 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).named_argument).second; }
#line 1326 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 72: /* args  */
#line 60 "ExpressionParser.y" /* yacc.c:1257  */
      { for(auto &v : ((*yyvaluep).named_arguments)) {delete v.second;} }
#line 1332 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 73: /* num  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1338 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 74: /* range  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1344 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 75: /* unit_exp  */
#line 58 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1350 "ExpressionParser.tab.c" /* yacc.c:1257  */
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
#line 67 "ExpressionParser.y" /* yacc.c:1646  */
    { ScanResult = (yyvsp[0].expr); valueExpression = true; (yyval.expr)=0;                                       }
#line 1614 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 68 "ExpressionParser.y" /* yacc.c:1646  */
    { ScanResult = (yyvsp[0].expr); unitExpression = true; (yyval.expr)=0;                                        }
#line 1620 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 72 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1626 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 73 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-1].expr), OperatorExpression::UNIT, (yyvsp[0].expr));  }
#line 1632 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 74 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new StringExpression(DocumentObject, (yyvsp[0].string)); }
#line 1638 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 75 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new StringExpression(DocumentObject, (yyvsp[0].string), true); }
#line 1644 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 76 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new VariableExpression(DocumentObject, (yyvsp[0].path));                                }
#line 1650 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 77 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1656 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 78 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1662 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 81 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 1668 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 82 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1674 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 83 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1680 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 84 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1686 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 85 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-1].expr)->addComponent((yyvsp[0].component)); (yyval.expr) = (yyvsp[-1].expr); }
#line 1692 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 86 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].expr)->addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string))); (yyval.expr) = (yyvsp[-2].expr); }
#line 1698 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 89 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, new VariableExpression(DocumentObject,(yyvsp[-2].path))); }
#line 1704 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 90 "ExpressionParser.y" /* yacc.c:1646  */
    { 
                                                    (yyval.expr) = CallableExpression::create(DocumentObject, (yyvsp[-3].path), (yyvsp[-1].named_arguments)); 
                                                    if(!(yyval.expr)) { 
                                                        YYABORT;
                                                    } 
                                                }
#line 1715 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 96 "ExpressionParser.y" /* yacc.c:1646  */
    {   // This rule exists because of possible name clash of 
                                                    // function and unit, e.g. min
                                                    ObjectIdentifier name(DocumentObject);
                                                    name << ObjectIdentifier::SimpleComponent((yyvsp[-3].quantity).unitStr);
                                                    (yyval.expr) = CallableExpression::create(DocumentObject, name, (yyvsp[-1].named_arguments));
                                                    if(!(yyval.expr)) { 
                                                        YYABORT;
                                                    } 
                                                }
#line 1729 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 105 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-2].expr)); }
#line 1735 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 106 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-3].expr), (yyvsp[-1].named_arguments)); }
#line 1741 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 107 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-2].expr)); }
#line 1747 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 108 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-3].expr), (yyvsp[-1].named_arguments)); }
#line 1753 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 109 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-1].expr)->addComponent((yyvsp[0].component)); (yyval.expr) = (yyvsp[-1].expr); }
#line 1759 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 110 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].expr)->addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string))); (yyval.expr) = (yyvsp[-2].expr); }
#line 1765 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 114 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1771 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 115 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))); }
#line 1777 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 116 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::POS, new NumberExpression(DocumentObject, Quantity(1))); }
#line 1783 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 120 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1789 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 121 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, (yyvsp[0].expr));   }
#line 1795 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 124 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1801 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 125 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 1807 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 126 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1813 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 127 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1819 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 128 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MOD, (yyvsp[0].expr));   }
#line 1825 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 132 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1831 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 133 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::ADD, (yyvsp[0].expr));   }
#line 1837 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 134 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::SUB, (yyvsp[0].expr));   }
#line 1843 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 138 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1849 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 139 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LT, (yyvsp[0].expr));    }
#line 1855 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 140 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GT, (yyvsp[0].expr));    }
#line 1861 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 141 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GTE, (yyvsp[0].expr));   }
#line 1867 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 142 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LTE, (yyvsp[0].expr));   }
#line 1873 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 146 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1879 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 147 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::EQ, (yyvsp[0].expr));    }
#line 1885 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 148 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::NEQ, (yyvsp[0].expr));   }
#line 1891 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 152 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1897 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 153 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::AND_OP, (yyvsp[0].expr));   }
#line 1903 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 154 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::AND_OP2, (yyvsp[0].expr));   }
#line 1909 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 159 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1915 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 160 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::OR_OP, (yyvsp[0].expr));   }
#line 1921 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 161 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::OR_OP2, (yyvsp[0].expr));   }
#line 1927 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 165 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1933 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 166 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ConditionalExpression(DocumentObject, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr));                     }
#line 1939 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 170 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1945 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 172 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string) = (yyvsp[0].string); }
#line 1951 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 173 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string) = (yyvsp[0].string); }
#line 1957 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 178 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.arguments).push_back((yyvsp[-2].expr)); (yyval.arguments).push_back((yyvsp[0].expr)); }
#line 1963 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 179 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].arguments).push_back((yyvsp[0].expr)); (yyval.arguments).swap((yyvsp[-2].arguments)); }
#line 1969 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 182 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject); }
#line 1975 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 183 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject, (yyvsp[-2].expr)); }
#line 1981 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 184 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject, (yyvsp[-1].arguments)); }
#line 1987 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 65:
#line 185 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject, (yyvsp[-2].arguments)); }
#line 1993 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 188 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.arguments).push_back((yyvsp[0].expr)); }
#line 1999 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 189 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].arguments).push_back((yyvsp[0].expr)); (yyval.arguments).swap((yyvsp[-2].arguments)); }
#line 2005 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 68:
#line 192 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ListExpression(DocumentObject); }
#line 2011 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 193 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ListExpression(DocumentObject, (yyvsp[-1].arguments)); }
#line 2017 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 194 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ListExpression(DocumentObject, (yyvsp[-2].arguments)); }
#line 2023 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 197 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new DictExpression(DocumentObject, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2029 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 198 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<DictExpression*>((yyvsp[-4].expr))->addItem((yyvsp[-2].expr),(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-4].expr); }
#line 2035 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 201 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new DictExpression(DocumentObject); }
#line 2041 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 202 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2047 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 203 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-2].expr); }
#line 2053 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 76:
#line 206 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first.clear(); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2059 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 77:
#line 207 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = (yyvsp[-2].string); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2065 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 208 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = "*"; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2071 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 209 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first.clear(); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2077 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 80:
#line 210 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = '*'; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2083 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 211 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = "**"; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2089 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 214 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_arguments).push_back((yyvsp[0].named_argument)); }
#line 2095 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 215 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].named_arguments).push_back((yyvsp[0].named_argument)); (yyval.named_arguments).swap((yyvsp[-2].named_arguments)); }
#line 2101 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 218 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 2107 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 219 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 2113 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 220 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue)));                }
#line 2119 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 87:
#line 221 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ConstantExpression(DocumentObject, (yyvsp[0].constant).name, Quantity((yyvsp[0].constant).fvalue));      }
#line 2125 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 88:
#line 224 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new RangeExpression(DocumentObject, (yyvsp[-2].string), (yyvsp[0].string));                               }
#line 2131 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 227 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 2137 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 90:
#line 228 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 2143 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 91:
#line 229 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 2149 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 92:
#line 230 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))));   }
#line 2155 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 93:
#line 231 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-3].expr), OperatorExpression::POW, new OperatorExpression(DocumentObject, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))));   }
#line 2161 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 94:
#line 232 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr);                                                                        }
#line 2167 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 95:
#line 235 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of the current object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 2176 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 96:
#line 239 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of a sub-object of the current object*/
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  if(_Reader) {
                                                    (yyvsp[-2].string_or_identifier) = ObjectIdentifier::String(
                                                        PropertyLinkBase::importSubName(*_Reader,(yyvsp[-2].string_or_identifier).getString().c_str()),true);
                                                  }
                                                  (yyval.path).setDocumentObjectName(DocumentObject,false,(yyvsp[-2].string_or_identifier));
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 2190 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 97:
#line 248 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of the current document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentObjectName(DocumentObject);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 2200 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 98:
#line 253 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of a sub-object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  if(_Reader) {
                                                    (yyvsp[-2].string_or_identifier) = ObjectIdentifier::String(
                                                        PropertyLinkBase::importSubName(*_Reader,(yyvsp[-2].string_or_identifier).getString().c_str()),true);
                                                    if(!(yyvsp[-4].string_or_identifier).isRealString())
                                                        (yyvsp[-4].string_or_identifier) = ObjectIdentifier::String(_Reader->getName((yyvsp[-4].string_or_identifier).getString().c_str()));
                                                  }
                                                  (yyval.path).setDocumentObjectName((yyvsp[-4].string_or_identifier), true, (yyvsp[-2].string_or_identifier));
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                  (yyval.path).resolveAmbiguity();
                                                }
#line 2217 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 99:
#line 265 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of a given document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  if(_Reader && !(yyvsp[-2].string_or_identifier).isRealString())
                                                      (yyvsp[-2].string_or_identifier) = ObjectIdentifier::String(_Reader->getName((yyvsp[-2].string_or_identifier).getString().c_str()));
                                                  (yyval.path) << ObjectIdentifier::SimpleComponent((yyvsp[-2].string_or_identifier));
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                  (yyval.path).resolveAmbiguity();
                                                }
#line 2230 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 273 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property from an external document, within a named document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentName((yyvsp[-4].string_or_identifier), true);
                                                  (yyval.path).setDocumentObjectName((yyvsp[-2].string_or_identifier), true);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                  (yyval.path).resolveAmbiguity();
                                                }
#line 2242 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 282 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.ivalue) = (yyvsp[0].ivalue); }
#line 2248 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 283 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.ivalue) = (yyvsp[0].fvalue); }
#line 2254 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 103:
#line 286 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_back(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));}
#line 2260 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 104:
#line 287 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-1].components).push_back((yyvsp[0].component)); (yyval.components).swap((yyvsp[-1].components));}
#line 2266 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 105:
#line 290 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_back(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));}
#line 2272 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 106:
#line 291 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].components).push_back(ObjectIdentifier::SimpleComponent((yyvsp[0].string))); (yyval.components).swap((yyvsp[-2].components)); }
#line 2278 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 107:
#line 292 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-1].components).push_back((yyvsp[0].component)); (yyval.components).swap((yyvsp[-1].components));}
#line 2284 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 108:
#line 295 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = ObjectIdentifier::ArrayComponent((yyvsp[-1].ivalue));   }
#line 2290 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 109:
#line 296 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = ObjectIdentifier::RangeComponent((yyvsp[-2].ivalue)); }
#line 2296 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 110:
#line 297 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = ObjectIdentifier::RangeComponent(0,(yyvsp[-1].ivalue)); }
#line 2302 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 111:
#line 298 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = ObjectIdentifier::RangeComponent((yyvsp[-3].ivalue),(yyvsp[-1].ivalue));}
#line 2308 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 112:
#line 299 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = ObjectIdentifier::MapComponent(ObjectIdentifier::String((yyvsp[-1].string), true));}
#line 2314 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 113:
#line 300 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = ObjectIdentifier::MapComponent((yyvsp[-1].string));}
#line 2320 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 114:
#line 303 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 2326 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 115:
#line 304 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string));       }
#line 2332 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 116:
#line 307 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 2338 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 117:
#line 308 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string));       }
#line 2344 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 118:
#line 311 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 2350 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;


#line 2354 "ExpressionParser.tab.c" /* yacc.c:1646  */
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
#line 314 "ExpressionParser.y" /* yacc.c:1906  */

