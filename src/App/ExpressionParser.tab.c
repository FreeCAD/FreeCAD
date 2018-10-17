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
    EXPAND = 282,
    NUM_AND_UNIT = 283,
    NEG = 284,
    POS = 285
  };
#endif

/* Value type.  */


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_EXPRESSIONPARSER_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 152 "ExpressionParser.tab.c" /* yacc.c:358  */

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
#define YYFINAL  72
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   805

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  49
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  40
/* YYNRULES -- Number of rules.  */
#define YYNRULES  133
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  229

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   285

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    48,     2,    33,     2,     2,
      38,    39,    31,    30,    41,     2,    40,    32,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    29,    42,
       2,    47,     2,    28,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    43,     2,    44,    35,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    45,     2,    46,     2,     2,     2,     2,
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
      25,    26,    27,    34,    36,    37
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    68,    68,    69,    73,    74,    75,    76,    77,    78,
      79,    82,    83,    84,    85,    86,    87,    88,    91,    97,
     103,   112,   113,   114,   115,   116,   117,   121,   122,   123,
     127,   128,   131,   132,   133,   134,   135,   139,   140,   141,
     145,   146,   147,   148,   149,   153,   154,   155,   159,   160,
     161,   166,   167,   168,   172,   173,   177,   179,   180,   183,
     183,   185,   186,   187,   188,   191,   192,   195,   196,   197,
     198,   201,   202,   205,   206,   207,   210,   210,   212,   213,
     214,   215,   218,   219,   220,   223,   224,   225,   226,   229,
     230,   233,   234,   235,   236,   237,   238,   241,   242,   245,
     246,   247,   248,   251,   254,   255,   256,   257,   258,   259,
     262,   266,   275,   280,   292,   300,   309,   310,   313,   314,
     317,   318,   319,   322,   323,   324,   325,   326,   327,   330,
     331,   334,   335,   338
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
  "MINUSSIGN", "PROPERTY_REF", "DOCUMENT", "OBJECT", "EXPONENT", "EXPAND",
  "'?'", "':'", "'+'", "'*'", "'/'", "'%'", "NUM_AND_UNIT", "'^'", "NEG",
  "POS", "'('", "')'", "'.'", "','", "';'", "'['", "']'", "'{'", "'}'",
  "'='", "'#'", "$accept", "input", "primary_exp", "indexable", "callable",
  "unary_exp", "power_exp", "multiply_exp", "additive_exp",
  "relational_exp", "equality_exp", "logical_and_exp", "logical_or_exp",
  "conditional_exp", "exp", "id_or_cell", "sep", "item", "items2", "tuple",
  "items", "list", "dict_expand", "dict1", "dict", "idict1", "idict",
  "arg", "args", "num", "range", "unit_exp", "identifier", "integer",
  "path", "subpath", "indexer", "document", "object", "subname", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,    63,    58,
      43,    42,    47,    37,   283,    94,   284,   285,    40,    41,
      46,    44,    59,    91,    93,   123,   125,    61,    35
};
# endif

#define YYPACT_NINF -110

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-110)))

#define YYTABLE_NINF -133

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     732,  -110,  -110,  -110,   -28,  -110,  -110,  -110,  -110,    13,
     760,   760,   302,    14,   517,   214,    57,  -110,    12,   122,
    -110,    41,   241,    34,   185,    16,    70,    90,  -110,  -110,
      30,  -110,  -110,   131,  -110,   134,  -110,     2,   150,    59,
      67,    71,    75,   560,   -28,   345,  -110,  -110,   760,  -110,
    -110,    91,    -4,    98,   142,   174,  -110,   104,  -110,  -110,
     121,   101,  -110,  -110,  -110,   166,  -110,  -110,  -110,   116,
     119,   149,  -110,   388,    19,    72,  -110,   431,    19,  -110,
     760,   760,   732,   760,   760,   760,   760,   760,   760,   760,
     760,   760,   760,   760,   760,   760,   760,  -110,   258,  -110,
      58,  -110,     2,   112,     2,     2,    44,   474,  -110,    79,
      14,   760,   760,  -110,    73,  -110,   186,  -110,  -110,  -110,
    -110,    19,  -110,   603,  -110,   646,  -110,    19,  -110,    19,
    -110,   689,   760,   760,   760,  -110,  -110,   198,  -110,  -110,
    -110,   130,    96,   144,   -11,  -110,   207,  -110,  -110,    41,
      41,   112,    41,   241,   241,    34,    34,    34,    34,   185,
     185,    16,    16,    70,    70,   164,  -110,   177,   760,  -110,
     167,   179,   112,   112,    96,  -110,  -110,   229,  -110,  -110,
     123,   121,   138,  -110,  -110,  -110,   760,  -110,   560,  -110,
    -110,  -110,  -110,  -110,  -110,   121,  -110,  -110,  -110,  -110,
    -110,  -110,  -110,   151,  -110,    10,  -110,  -110,   760,   760,
    -110,   760,   760,  -110,  -110,    19,    19,  -110,  -110,  -110,
    -110,   180,  -110,  -110,  -110,  -110,   121,   121,  -110
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    99,   100,    57,   104,   101,   102,    58,     7,     6,
       0,     0,     0,     0,     0,     0,     0,    27,     9,    10,
      30,    32,    37,    40,    45,    48,    51,    54,    56,     2,
     118,    12,    13,     0,    14,     0,    15,     4,     3,     8,
     110,     0,     0,     0,     0,     0,    28,    29,     0,    59,
      60,    61,   118,     0,     0,     0,    62,     0,   133,   120,
     112,     0,    73,    61,    71,     0,    77,    76,    82,     0,
     118,     0,     1,     0,     0,     0,    16,     0,     0,    25,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    83,     0,    89,
       0,   104,     0,     5,     0,     0,     0,     0,   119,     0,
       0,     0,     0,    91,   118,    97,     0,    94,    63,    64,
      11,     0,    67,     0,    69,     0,   109,     0,   122,     0,
      74,     0,     0,     0,     0,    79,    21,     0,    17,   117,
     116,     0,     0,     0,     0,    23,     0,    26,    31,    33,
      34,    35,    36,    39,    38,    41,    42,    43,    44,    46,
      47,    49,    50,    52,    53,     0,    84,     0,     0,    90,
       0,     0,   106,   105,     0,   107,    18,     0,   131,   132,
       0,   114,     0,    96,    93,    95,     0,    20,     0,   103,
      68,    65,    70,    66,   121,   111,    75,    72,    78,    85,
      86,    22,   127,     0,   128,     0,   123,    24,     0,     0,
      81,     0,     0,   108,    19,     0,     0,    92,    98,   125,
     124,     0,    55,    80,    87,    88,   115,   113,   126
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -110,  -110,  -110,  -110,  -110,    -8,   148,   106,   115,   152,
     158,   161,  -110,    39,     0,    17,     4,    -9,  -110,  -110,
    -110,  -110,   -18,  -110,  -110,  -110,  -110,    50,   -66,  -110,
     -39,    46,  -110,  -100,  -110,  -109,     3,  -110,   160,   143
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,   113,    30,   188,    54,    55,    31,
      65,    32,    71,    33,    34,    35,    36,   115,   116,    37,
      56,    57,    39,   144,    40,    60,   128,    41,    42,    61
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      29,   181,    46,    47,   117,    64,   175,   137,   101,   119,
      43,   146,    51,   139,    63,    69,    53,   140,   205,     3,
     195,    76,    79,     7,     3,   121,    90,    91,     7,    52,
      59,    52,    70,   206,   117,    58,  -132,    98,   117,   100,
     102,   177,   203,   108,  -130,    51,    38,   139,   118,    53,
      73,   140,    74,  -131,   220,    75,    84,    72,   123,   125,
     114,  -129,    52,     3,    85,    52,   174,     7,   117,   131,
    -132,   135,   148,   185,   213,   139,    80,     3,  -130,   140,
     168,     7,   171,   103,     3,    66,    92,    93,     7,    67,
     114,   138,   143,   141,   114,   147,   165,   107,   167,   139,
     178,   142,   121,   140,   169,   221,   226,   227,    94,    95,
      75,   183,   184,  -132,   191,   110,   193,   170,    96,   109,
     186,  -130,   197,    63,   114,    63,   179,    59,   151,    52,
     120,    63,   198,   199,   200,   104,   105,   122,   189,   106,
      52,   129,    52,   126,   194,   132,    59,   106,    52,   117,
     172,   173,     1,     2,     3,    44,     5,     6,     7,  -132,
      77,   127,    78,   215,    75,    75,   133,  -130,   210,     8,
       9,    10,    49,    50,   202,    49,    50,    97,   216,    11,
      99,   104,   105,    49,    50,   106,   217,    45,   204,    13,
     153,   154,    14,   208,    15,   219,   134,    86,    87,    88,
      89,   155,   156,   157,   158,   114,   209,    49,    50,   223,
     130,   224,   225,   124,   211,    49,    50,     1,     2,     3,
      44,     5,     6,     7,   228,   187,   212,    49,    50,   149,
     150,   152,    59,    59,     8,     9,    10,   201,   218,    49,
      50,    66,   159,   160,    11,    67,   207,   222,    49,    50,
     161,   162,    45,   182,    13,   163,   164,    14,     0,    15,
      68,     1,     2,     3,    44,     5,     6,     7,   214,   180,
      49,    50,    81,    82,    83,     0,     0,     0,     8,     9,
      10,     0,     0,     0,     0,    66,     0,     0,    11,    67,
       0,     0,     0,     0,     0,     0,    45,     0,    13,     0,
       0,    14,     0,    15,   166,     1,     2,     3,     4,     5,
       6,     7,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     8,     9,    10,     0,     0,     0,     0,     0,
       0,     0,    11,    48,     0,     0,     0,     0,     0,     0,
      12,     0,    13,    49,    50,    14,     0,    15,     1,     2,
       3,    44,     5,     6,     7,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,     0,     0,
       0,     0,     0,     0,     0,    11,    48,     0,     0,     0,
       0,     0,     0,    45,     0,    13,    49,    50,    14,     0,
      15,     1,     2,     3,    44,     5,     6,     7,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     8,     9,
      10,     0,     0,     0,     0,   111,     0,     0,    11,   112,
       0,     0,     0,     0,     0,     0,    45,   136,    13,     0,
       0,    14,     0,    15,     1,     2,     3,    44,     5,     6,
       7,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,     0,     0,     0,     0,   111,     0,
       0,    11,   112,     0,     0,     0,     0,     0,     0,    45,
     145,    13,     0,     0,    14,     0,    15,     1,     2,     3,
      44,     5,     6,     7,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     8,     9,    10,     0,     0,     0,
       0,   111,     0,     0,    11,   112,     0,     0,     0,     0,
       0,     0,    45,   176,    13,     0,     0,    14,     0,    15,
       1,     2,     3,    44,     5,     6,     7,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
       0,     0,     0,     0,     0,     0,     0,    11,    48,     0,
       0,     0,     0,     0,     0,    45,     0,    13,     0,     0,
      14,    62,    15,     1,     2,     3,    44,     5,     6,     7,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       8,     9,    10,     0,     0,     0,     0,   111,     0,     0,
      11,   112,     0,     0,     0,     0,     0,     0,    45,     0,
      13,     0,     0,    14,     0,    15,     1,     2,     3,    44,
       5,     6,     7,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     8,     9,    10,     0,     0,     0,     0,
       0,     0,     0,    11,    48,     0,     0,     0,     0,     0,
       0,    45,   190,    13,     0,     0,    14,     0,    15,     1,
       2,     3,    44,     5,     6,     7,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,     0,
       0,     0,     0,     0,     0,     0,    11,    48,     0,     0,
       0,     0,     0,     0,    45,   192,    13,     0,     0,    14,
       0,    15,     1,     2,     3,    44,     5,     6,     7,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,     0,     0,     0,     0,     0,     0,     0,    11,
      48,     0,     0,     0,     0,     0,     0,    45,     0,    13,
       0,     0,    14,   196,    15,     1,     2,     3,     4,     5,
       6,     7,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     8,     9,    10,     0,     0,     0,     0,     0,
       0,     0,    11,     1,     2,     3,    44,     5,     6,     7,
      12,     0,    13,     0,     0,    14,     0,    15,     0,     0,
       8,     9,    10,     0,     0,     0,     0,     0,     0,     0,
      11,     0,     0,     0,     0,     0,     0,     0,    45,     0,
      13,     0,     0,    14,     0,    15
};

static const yytype_int16 yycheck[] =
{
       0,   110,    10,    11,    43,    14,   106,    73,     6,    48,
      38,    77,    12,     3,    14,    15,    12,     7,    29,     5,
     129,    18,    19,     9,     5,    29,    10,    11,     9,    12,
      13,    14,    15,    44,    73,    21,    40,    33,    77,    35,
      38,   107,   142,    40,    48,    45,     0,     3,    48,    45,
      38,     7,    40,    40,    44,    43,    22,     0,    54,    55,
      43,    48,    45,     5,    30,    48,    22,     9,   107,    65,
      40,    71,    80,   112,   174,     3,    35,     5,    48,     7,
      98,     9,   100,    37,     5,    27,    16,    17,     9,    31,
      73,    74,    75,    21,    77,    78,    96,    38,    98,     3,
      21,    29,    29,     7,    46,   205,   215,   216,    18,    19,
      43,   111,   112,    40,   123,    40,   125,   100,    28,    48,
      47,    48,   131,   123,   107,   125,   109,   110,    82,   112,
      39,   131,   132,   133,   134,    31,    32,    39,   121,    35,
     123,    40,   125,    39,   127,    29,   129,    35,   131,   188,
     104,   105,     3,     4,     5,     6,     7,     8,     9,    40,
      38,    40,    40,    40,    43,    43,    47,    48,   168,    20,
      21,    22,    41,    42,    44,    41,    42,    46,    40,    30,
      46,    31,    32,    41,    42,    35,   186,    38,    44,    40,
      84,    85,    43,    29,    45,    44,    47,    12,    13,    14,
      15,    86,    87,    88,    89,   188,    29,    41,    42,   209,
      44,   211,   212,    39,    47,    41,    42,     3,     4,     5,
       6,     7,     8,     9,    44,    39,    47,    41,    42,    81,
      82,    83,   215,   216,    20,    21,    22,    39,   188,    41,
      42,    27,    90,    91,    30,    31,    39,   208,    41,    42,
      92,    93,    38,   110,    40,    94,    95,    43,    -1,    45,
      46,     3,     4,     5,     6,     7,     8,     9,    39,   109,
      41,    42,    31,    32,    33,    -1,    -1,    -1,    20,    21,
      22,    -1,    -1,    -1,    -1,    27,    -1,    -1,    30,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,    40,    -1,
      -1,    43,    -1,    45,    46,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      38,    -1,    40,    41,    42,    43,    -1,    45,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    20,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    30,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    38,    -1,    40,    41,    42,    43,    -1,
      45,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    21,
      22,    -1,    -1,    -1,    -1,    27,    -1,    -1,    30,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    38,    39,    40,    -1,
      -1,    43,    -1,    45,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    20,    21,    22,    -1,    -1,    -1,    -1,    27,    -1,
      -1,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    38,
      39,    40,    -1,    -1,    43,    -1,    45,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    20,    21,    22,    -1,    -1,    -1,
      -1,    27,    -1,    -1,    30,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    38,    39,    40,    -1,    -1,    43,    -1,    45,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    30,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    38,    -1,    40,    -1,    -1,
      43,    44,    45,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      20,    21,    22,    -1,    -1,    -1,    -1,    27,    -1,    -1,
      30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,
      40,    -1,    -1,    43,    -1,    45,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    20,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    30,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    38,    39,    40,    -1,    -1,    43,    -1,    45,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    20,    21,    22,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    30,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    38,    39,    40,    -1,    -1,    43,
      -1,    45,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    30,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,    40,
      -1,    -1,    43,    44,    45,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    30,     3,     4,     5,     6,     7,     8,     9,
      38,    -1,    40,    -1,    -1,    43,    -1,    45,    -1,    -1,
      20,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      30,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,
      40,    -1,    -1,    43,    -1,    45
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    20,    21,
      22,    30,    38,    40,    43,    45,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    68,    70,    72,    73,    74,    75,    78,    80,    81,
      83,    86,    87,    38,     6,    38,    54,    54,    31,    41,
      42,    63,    64,    65,    66,    67,    79,    80,    21,    64,
      84,    88,    44,    63,    66,    69,    27,    31,    46,    63,
      64,    71,     0,    38,    40,    43,    85,    38,    40,    85,
      35,    31,    32,    33,    22,    30,    12,    13,    14,    15,
      10,    11,    16,    17,    18,    19,    28,    46,    65,    46,
      65,     6,    38,    80,    31,    32,    35,    38,    85,    48,
      40,    27,    31,    63,    64,    76,    77,    79,    63,    79,
      39,    29,    39,    65,    39,    65,    39,    40,    85,    40,
      44,    65,    29,    47,    47,    63,    39,    77,    64,     3,
       7,    21,    29,    64,    82,    39,    77,    64,    54,    55,
      55,    80,    55,    56,    56,    57,    57,    57,    57,    58,
      58,    59,    59,    60,    60,    63,    46,    63,    71,    46,
      64,    71,    80,    80,    22,    82,    39,    77,    21,    64,
      87,    84,    88,    63,    63,    79,    47,    39,    65,    64,
      39,    66,    39,    66,    64,    84,    44,    66,    63,    63,
      63,    39,    44,    82,    44,    29,    44,    39,    29,    29,
      63,    47,    47,    82,    39,    40,    40,    63,    76,    44,
      44,    82,    62,    63,    63,    63,    84,    84,    44
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    49,    50,    50,    51,    51,    51,    51,    51,    51,
      51,    52,    52,    52,    52,    52,    52,    52,    53,    53,
      53,    53,    53,    53,    53,    53,    53,    54,    54,    54,
      55,    55,    56,    56,    56,    56,    56,    57,    57,    57,
      58,    58,    58,    58,    58,    59,    59,    59,    60,    60,
      60,    61,    61,    61,    62,    62,    63,    64,    64,    65,
      65,    66,    66,    66,    66,    67,    67,    68,    68,    68,
      68,    69,    69,    70,    70,    70,    71,    71,    72,    72,
      72,    72,    73,    73,    73,    74,    74,    74,    74,    75,
      75,    76,    76,    76,    76,    76,    76,    77,    77,    78,
      78,    78,    78,    79,    80,    80,    80,    80,    80,    80,
      81,    81,    81,    81,    81,    81,    82,    82,    83,    83,
      84,    84,    84,    85,    85,    85,    85,    85,    85,    86,
      86,    87,    87,    88
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     2,     1,     1,     1,     1,
       1,     3,     1,     1,     1,     1,     2,     3,     3,     4,
       4,     3,     4,     3,     4,     2,     3,     1,     2,     2,
       1,     3,     1,     3,     3,     3,     3,     1,     3,     3,
       1,     3,     3,     3,     3,     1,     3,     3,     1,     3,
       3,     1,     3,     3,     1,     5,     1,     1,     1,     1,
       1,     1,     1,     2,     2,     3,     3,     3,     4,     3,
       4,     1,     3,     2,     3,     4,     1,     1,     4,     3,
       5,     4,     2,     2,     3,     4,     4,     5,     5,     2,
       3,     1,     3,     2,     1,     2,     2,     1,     3,     1,
       1,     1,     1,     3,     1,     3,     3,     3,     4,     3,
       1,     4,     2,     5,     3,     5,     1,     1,     1,     2,
       1,     3,     2,     3,     4,     4,     5,     3,     3,     1,
       1,     1,     1,     1
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
          case 50: /* input  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1253 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 51: /* primary_exp  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1259 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 52: /* indexable  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1265 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 53: /* callable  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1271 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 54: /* unary_exp  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1277 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 55: /* power_exp  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1283 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 56: /* multiply_exp  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1289 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 57: /* additive_exp  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1295 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 58: /* relational_exp  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1301 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 59: /* equality_exp  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1307 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 60: /* logical_and_exp  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1313 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 61: /* logical_or_exp  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1319 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 62: /* conditional_exp  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1325 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 63: /* exp  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1331 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 66: /* item  */
#line 62 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).named_argument).second; }
#line 1337 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 67: /* items2  */
#line 61 "ExpressionParser.y" /* yacc.c:1257  */
      { for(auto &v : ((*yyvaluep).named_arguments)) {delete v.second;} }
#line 1343 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 68: /* tuple  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1349 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 69: /* items  */
#line 61 "ExpressionParser.y" /* yacc.c:1257  */
      { for(auto &v : ((*yyvaluep).named_arguments)) {delete v.second;} }
#line 1355 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 70: /* list  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1361 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 72: /* dict1  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1367 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 73: /* dict  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1373 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 74: /* idict1  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1379 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 75: /* idict  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1385 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 76: /* arg  */
#line 62 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).named_argument).second; }
#line 1391 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 77: /* args  */
#line 61 "ExpressionParser.y" /* yacc.c:1257  */
      { for(auto &v : ((*yyvaluep).named_arguments)) {delete v.second;} }
#line 1397 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 78: /* num  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1403 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 79: /* range  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1409 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 80: /* unit_exp  */
#line 59 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1415 "ExpressionParser.tab.c" /* yacc.c:1257  */
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
    { ScanResult = (yyvsp[0].expr); valueExpression = true; (yyval.expr)=0;                                       }
#line 1679 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 69 "ExpressionParser.y" /* yacc.c:1646  */
    { ScanResult = (yyvsp[0].expr); unitExpression = true; (yyval.expr)=0;                                        }
#line 1685 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 73 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1691 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 74 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-1].expr), OperatorExpression::UNIT, (yyvsp[0].expr));  }
#line 1697 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 75 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new StringExpression(DocumentObject, (yyvsp[0].string)); }
#line 1703 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 76 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new StringExpression(DocumentObject, (yyvsp[0].string), true); }
#line 1709 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 77 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = VariableExpression::create(DocumentObject, (yyvsp[0].path)); }
#line 1715 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 78 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1721 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 79 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1727 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 82 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 1733 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 83 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1739 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 84 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1745 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 85 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1751 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 86 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1757 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 87 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-1].expr)->addComponent((yyvsp[0].component)); (yyval.expr) = (yyvsp[-1].expr); }
#line 1763 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 88 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].expr)->addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string))); (yyval.expr) = (yyvsp[-2].expr); }
#line 1769 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 91 "ExpressionParser.y" /* yacc.c:1646  */
    { 
                                                    (yyval.expr) = CallableExpression::create(DocumentObject,(yyvsp[-2].path));
                                                    if(!(yyval.expr)) { 
                                                        YYABORT;
                                                    } 
                                                }
#line 1780 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 97 "ExpressionParser.y" /* yacc.c:1646  */
    { 
                                                    (yyval.expr) = CallableExpression::create(DocumentObject, (yyvsp[-3].path), (yyvsp[-1].named_arguments)); 
                                                    if(!(yyval.expr)) { 
                                                        YYABORT;
                                                    } 
                                                }
#line 1791 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 103 "ExpressionParser.y" /* yacc.c:1646  */
    {   // This rule exists because of possible name clash of 
                                                    // function and unit, e.g. min
                                                    ObjectIdentifier name(DocumentObject);
                                                    name << ObjectIdentifier::SimpleComponent((yyvsp[-3].quantity).unitStr);
                                                    (yyval.expr) = CallableExpression::create(DocumentObject, name, (yyvsp[-1].named_arguments));
                                                    if(!(yyval.expr)) { 
                                                        YYABORT;
                                                    } 
                                                }
#line 1805 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 112 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-2].expr)); }
#line 1811 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 113 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-3].expr), (yyvsp[-1].named_arguments)); }
#line 1817 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 114 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-2].expr)); }
#line 1823 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 115 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-3].expr), (yyvsp[-1].named_arguments)); }
#line 1829 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 116 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-1].expr)->addComponent((yyvsp[0].component)); (yyval.expr) = (yyvsp[-1].expr); }
#line 1835 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 117 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].expr)->addComponent(ObjectIdentifier::SimpleComponent((yyvsp[0].string))); (yyval.expr) = (yyvsp[-2].expr); }
#line 1841 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 121 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1847 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 122 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))); }
#line 1853 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 123 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::POS, new NumberExpression(DocumentObject, Quantity(1))); }
#line 1859 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 127 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1865 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 128 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, (yyvsp[0].expr));   }
#line 1871 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 131 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1877 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 132 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 1883 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 133 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1889 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 134 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 1895 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 135 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MOD, (yyvsp[0].expr));   }
#line 1901 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 139 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1907 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 140 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::ADD, (yyvsp[0].expr));   }
#line 1913 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 141 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::SUB, (yyvsp[0].expr));   }
#line 1919 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 145 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1925 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 146 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LT, (yyvsp[0].expr));    }
#line 1931 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 147 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GT, (yyvsp[0].expr));    }
#line 1937 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 148 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GTE, (yyvsp[0].expr));   }
#line 1943 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 149 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LTE, (yyvsp[0].expr));   }
#line 1949 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 153 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1955 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 154 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::EQ, (yyvsp[0].expr));    }
#line 1961 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 155 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::NEQ, (yyvsp[0].expr));   }
#line 1967 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 159 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1973 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 160 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::AND_OP, (yyvsp[0].expr));   }
#line 1979 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 161 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::AND_OP2, (yyvsp[0].expr));   }
#line 1985 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 166 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1991 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 167 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::OR_OP, (yyvsp[0].expr));   }
#line 1997 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 168 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::OR_OP2, (yyvsp[0].expr));   }
#line 2003 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 172 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2009 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 173 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ConditionalExpression(DocumentObject, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr));                     }
#line 2015 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 177 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2021 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 179 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string) = (yyvsp[0].string); }
#line 2027 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 180 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string) = (yyvsp[0].string); }
#line 2033 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 185 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first.clear(); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2039 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 186 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first.clear(); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2045 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 187 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = "*"; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2051 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 188 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = "*"; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2057 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 65:
#line 191 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_arguments).push_back((yyvsp[-2].named_argument)); (yyval.named_arguments).push_back((yyvsp[0].named_argument)); }
#line 2063 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 192 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].named_arguments).push_back((yyvsp[0].named_argument)); (yyval.named_arguments).swap((yyvsp[-2].named_arguments)); }
#line 2069 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 195 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject); }
#line 2075 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 68:
#line 196 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject, (yyvsp[-2].named_argument)); }
#line 2081 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 197 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject, (yyvsp[-1].named_arguments)); }
#line 2087 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 198 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject, (yyvsp[-2].named_arguments)); }
#line 2093 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 201 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_arguments).push_back((yyvsp[0].named_argument)); }
#line 2099 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 202 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].named_arguments).push_back((yyvsp[0].named_argument)); (yyval.named_arguments).swap((yyvsp[-2].named_arguments)); }
#line 2105 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 205 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ListExpression(DocumentObject); }
#line 2111 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 206 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ListExpression(DocumentObject, (yyvsp[-1].named_arguments)); }
#line 2117 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 207 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ListExpression(DocumentObject, (yyvsp[-2].named_arguments)); }
#line 2123 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 212 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new DictExpression(DocumentObject, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2129 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 213 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new DictExpression(DocumentObject, 0, (yyvsp[0].expr)); }
#line 2135 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 80:
#line 214 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<DictExpression*>((yyvsp[-4].expr))->addItem((yyvsp[-2].expr),(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-4].expr); }
#line 2141 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 215 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<DictExpression*>((yyvsp[-3].expr))->addItem(0,(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-3].expr); }
#line 2147 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 218 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new DictExpression(DocumentObject); }
#line 2153 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 219 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2159 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 220 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-2].expr); }
#line 2165 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 223 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new IDictExpression(DocumentObject, (yyvsp[-2].string), (yyvsp[0].expr)); }
#line 2171 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 224 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new IDictExpression(DocumentObject, "**", (yyvsp[0].expr)); }
#line 2177 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 87:
#line 225 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<IDictExpression*>((yyvsp[-4].expr))->addItem((yyvsp[-2].string),(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-4].expr); }
#line 2183 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 88:
#line 226 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<IDictExpression*>((yyvsp[-4].expr))->addItem("**",(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-4].expr); }
#line 2189 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 229 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2195 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 90:
#line 230 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-2].expr); }
#line 2201 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 91:
#line 233 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first.clear(); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2207 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 92:
#line 234 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = (yyvsp[-2].string); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2213 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 93:
#line 235 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = "*"; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2219 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 94:
#line 236 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first.clear(); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2225 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 95:
#line 237 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = '*'; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2231 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 96:
#line 238 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = "**"; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2237 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 97:
#line 241 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_arguments).push_back((yyvsp[0].named_argument)); }
#line 2243 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 98:
#line 242 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].named_arguments).push_back((yyvsp[0].named_argument)); (yyval.named_arguments).swap((yyvsp[-2].named_arguments)); }
#line 2249 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 99:
#line 245 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 2255 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 246 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 2261 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 247 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue)));                }
#line 2267 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 248 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ConstantExpression(DocumentObject, (yyvsp[0].constant).name, Quantity((yyvsp[0].constant).fvalue));      }
#line 2273 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 103:
#line 251 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new RangeExpression(DocumentObject, (yyvsp[-2].string), (yyvsp[0].string));                               }
#line 2279 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 104:
#line 254 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 2285 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 105:
#line 255 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 2291 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 106:
#line 256 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 2297 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 107:
#line 257 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))));   }
#line 2303 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 108:
#line 258 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-3].expr), OperatorExpression::POW, new OperatorExpression(DocumentObject, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))));   }
#line 2309 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 109:
#line 259 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr);                                                                        }
#line 2315 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 110:
#line 262 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of the current object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 2324 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 111:
#line 266 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of a sub-object of the current object*/
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  if(_Reader) {
                                                    (yyvsp[-2].string_or_identifier) = ObjectIdentifier::String(
                                                        PropertyLinkBase::importSubName(*_Reader,(yyvsp[-2].string_or_identifier).getString().c_str()),true);
                                                  }
                                                  (yyval.path).setDocumentObjectName(DocumentObject,false,(yyvsp[-2].string_or_identifier));
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 2338 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 112:
#line 275 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of the current document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentObjectName(DocumentObject);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 2348 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 113:
#line 280 "ExpressionParser.y" /* yacc.c:1646  */
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
#line 2365 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 114:
#line 292 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of a given document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  if(_Reader && !(yyvsp[-2].string_or_identifier).isRealString())
                                                      (yyvsp[-2].string_or_identifier) = ObjectIdentifier::String(_Reader->getName((yyvsp[-2].string_or_identifier).getString().c_str()));
                                                  (yyval.path) << ObjectIdentifier::SimpleComponent((yyvsp[-2].string_or_identifier));
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                  (yyval.path).resolveAmbiguity();
                                                }
#line 2378 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 115:
#line 300 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property from an external document, within a named document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentName((yyvsp[-4].string_or_identifier), true);
                                                  (yyval.path).setDocumentObjectName((yyvsp[-2].string_or_identifier), true);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                  (yyval.path).resolveAmbiguity();
                                                }
#line 2390 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 116:
#line 309 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.ivalue) = (yyvsp[0].ivalue); }
#line 2396 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 117:
#line 310 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.ivalue) = (yyvsp[0].fvalue); }
#line 2402 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 118:
#line 313 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_back(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));}
#line 2408 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 119:
#line 314 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-1].components).push_back((yyvsp[0].component)); (yyval.components).swap((yyvsp[-1].components));}
#line 2414 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 120:
#line 317 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_back(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));}
#line 2420 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 121:
#line 318 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].components).push_back(ObjectIdentifier::SimpleComponent((yyvsp[0].string))); (yyval.components).swap((yyvsp[-2].components)); }
#line 2426 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 122:
#line 319 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-1].components).push_back((yyvsp[0].component)); (yyval.components).swap((yyvsp[-1].components));}
#line 2432 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 123:
#line 322 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = ObjectIdentifier::ArrayComponent((yyvsp[-1].ivalue));   }
#line 2438 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 124:
#line 323 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = ObjectIdentifier::RangeComponent((yyvsp[-2].ivalue)); }
#line 2444 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 125:
#line 324 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = ObjectIdentifier::RangeComponent(0,(yyvsp[-1].ivalue)); }
#line 2450 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 126:
#line 325 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = ObjectIdentifier::RangeComponent((yyvsp[-3].ivalue),(yyvsp[-1].ivalue));}
#line 2456 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 127:
#line 326 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = ObjectIdentifier::MapComponent(ObjectIdentifier::String((yyvsp[-1].string), true));}
#line 2462 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 128:
#line 327 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = ObjectIdentifier::MapComponent((yyvsp[-1].string));}
#line 2468 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 129:
#line 330 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 2474 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 130:
#line 331 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string));       }
#line 2480 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 131:
#line 334 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 2486 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 132:
#line 335 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string));       }
#line 2492 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 133:
#line 338 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 2498 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;


#line 2502 "ExpressionParser.tab.c" /* yacc.c:1646  */
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
#line 341 "ExpressionParser.y" /* yacc.c:1906  */

