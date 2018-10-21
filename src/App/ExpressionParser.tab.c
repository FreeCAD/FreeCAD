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

/* Copy the second part of user declarations.  */

#line 169 "ExpressionParser.tab.c" /* yacc.c:358  */

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
#define YYFINAL  101
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1024

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  66
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  53
/* YYNRULES -- Number of rules.  */
#define YYNRULES  172
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  313

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   302

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    65,     2,    50,     2,     2,
      56,    57,    48,    47,    58,     2,    55,    49,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    46,    60,
       2,    59,     2,    45,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    61,     2,    62,    52,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    63,     2,    64,     2,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      51,    53,    54
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    75,    75,    76,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    91,    92,    93,    94,    95,    96,    97,
      98,   101,   107,   113,   122,   123,   124,   125,   129,   130,
     131,   135,   136,   139,   140,   141,   142,   143,   147,   148,
     149,   153,   154,   155,   156,   157,   161,   162,   163,   167,
     168,   169,   174,   175,   176,   180,   181,   185,   186,   190,
     191,   195,   196,   197,   198,   199,   203,   204,   208,   209,
     210,   211,   212,   216,   217,   224,   228,   229,   230,   231,
     232,   236,   237,   238,   239,   243,   244,   247,   248,   252,
     253,   257,   260,   263,   264,   267,   267,   269,   270,   271,
     272,   275,   276,   279,   280,   281,   282,   285,   286,   289,
     290,   291,   292,   296,   297,   301,   302,   305,   305,   307,
     308,   309,   310,   313,   314,   315,   316,   317,   320,   321,
     322,   323,   326,   327,   330,   331,   332,   333,   334,   335,
     338,   339,   342,   343,   344,   345,   348,   351,   352,   353,
     354,   355,   356,   357,   360,   363,   372,   377,   389,   397,
     406,   407,   410,   411,   414,   415,   416,   417,   420,   421,
     424,   425,   428
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ONE", "NUM", "IDENTIFIER", "UNIT",
  "INTEGER", "CONSTANT", "CELLADDRESS", "EQ", "NEQ", "LT", "GT", "GTE",
  "LTE", "AND_OP", "AND_OP2", "OR_OP", "OR_OP2", "MUL_ASSIGN",
  "DIV_ASSIGN", "MOD_ASSIGN", "ADD_ASSIGN", "SUB_ASSIGN", "RSTRING",
  "STRING", "MINUSSIGN", "PROPERTY_REF", "DOCUMENT", "OBJECT", "EXPONENT",
  "EXPAND", "NEWLINE", "INDENT", "DEDENT", "IF", "ELIF", "ELSE", "WHILE",
  "FOR", "BREAK", "CONTINUE", "RETURN", "IN", "'?'", "':'", "'+'", "'*'",
  "'/'", "'%'", "NUM_AND_UNIT", "'^'", "NEG", "POS", "'.'", "'('", "')'",
  "','", "'='", "';'", "'['", "']'", "'{'", "'}'", "'#'", "$accept",
  "input", "primary_exp", "indexable", "callable", "unary_exp",
  "power_exp", "multiply_exp", "additive_exp", "relational_exp",
  "equality_exp", "logical_and_exp", "logical_or_exp", "exp", "id_list",
  "assignment_exp1", "assignment_exp2", "assignment_exp", "small_stmt",
  "simple_stmt", "compound_stmt", "stmt", "statement", "suite", "if_stmt",
  "while_stmt", "for_stmt", "id_or_cell", "sep", "item", "items2", "tuple",
  "items", "list", "comprehension0", "comprehension", "dict_expand",
  "dict1", "dict", "idict1", "idict", "arg", "args", "num", "range",
  "unit_exp", "identifier", "integer", "subpath", "indexer", "document",
  "object", "subname", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,    63,    58,    43,    42,    47,
      37,   300,    94,   301,   302,    46,    40,    41,    44,    61,
      59,    91,    93,   123,   125,    35
};
# endif

#define YYPACT_NINF -155

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-155)))

#define YYTABLE_NINF -172

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     330,  -155,  -155,   165,   -37,  -155,  -155,  -155,  -155,    46,
     961,   961,   961,    21,  -155,  -155,   961,  -155,   961,    23,
     445,   660,   190,    38,  -155,    70,    77,  -155,    -7,   142,
      32,   188,   118,   224,    -1,  -155,   200,    19,  -155,  -155,
    -155,   -12,  -155,   420,  -155,   283,  -155,    84,    98,  -155,
    -155,    91,  -155,   161,  -155,    17,   183,    31,    72,   105,
     961,   961,   961,   961,   961,   721,    80,   -37,   516,  -155,
     117,   120,  -155,    22,  -155,  -155,  -155,  -155,  -155,   125,
     127,   961,  -155,  -155,   157,    61,   163,    90,    82,  -155,
      57,  -155,   178,  -155,   223,   -25,  -155,  -155,  -155,    43,
     864,  -155,   228,   577,   875,  -155,   231,   588,  -155,   961,
     961,   890,   961,   961,   961,   961,   961,   961,   961,   961,
     961,   961,   961,   961,   961,   961,   242,   961,   961,  -155,
     201,     2,  -155,   961,   204,   209,  -155,   268,  -155,    24,
    -155,    17,   214,    17,    17,    30,   649,  -155,    73,    23,
    -155,  -155,  -155,  -155,  -155,    54,   961,   961,  -155,  -155,
     239,  -155,   506,   506,   961,   256,   103,  -155,  -155,  -155,
     103,  -155,   731,  -155,   792,  -155,    21,    94,   206,  -155,
     803,   961,   961,   218,   961,  -155,  -155,  -155,   244,   961,
      29,  -155,  -155,   249,  -155,    -7,    -7,   214,    -7,   142,
     142,    32,    32,    32,    32,   188,   188,   118,   118,   224,
     224,   238,  -155,  -155,  -155,  -155,  -155,   240,   506,   506,
    -155,   246,   961,   255,  -155,   263,   214,   214,  -155,  -155,
     174,  -155,  -155,   253,  -155,  -155,   248,   125,   257,   961,
    -155,  -155,  -155,  -155,   721,   274,    14,  -155,  -155,   284,
    -155,   125,  -155,  -155,  -155,  -155,  -155,  -155,    37,   961,
      21,  -155,  -155,  -155,  -155,   178,  -155,  -155,  -155,   280,
     951,  -155,  -155,   961,   506,  -155,  -155,   961,  -155,   961,
     961,  -155,  -155,   103,   103,  -155,  -155,   420,  -155,   506,
     961,  -155,    42,   281,  -155,  -155,   285,  -155,  -155,  -155,
    -155,  -155,   125,   125,   -12,   375,  -155,  -155,   961,  -155,
    -155,  -155,  -155
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,   142,   143,    93,   147,   144,   145,    94,     7,     6,
       0,     0,     0,     0,    71,    72,     0,   148,     0,     0,
       0,     0,     0,     0,    28,     9,    10,    31,    33,    38,
      41,    46,    49,    52,    55,    69,     0,    66,    67,    68,
      73,    85,    82,    86,     2,    76,    78,    79,   154,    14,
      15,     0,    16,     0,    17,     4,     3,     8,     0,     0,
       0,     0,     0,     0,     0,     0,    93,     0,     0,    29,
       0,     0,    57,     0,    70,    30,    93,   172,   162,   156,
       0,     0,    95,    96,    97,   154,     0,     0,     0,    98,
       0,   109,    97,   107,     0,    93,   118,   117,   123,     0,
       0,     1,     0,     0,     0,    19,     0,     0,    11,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    81,
      75,     0,    84,     0,     0,     0,   124,     0,   132,     0,
     147,     0,     5,     0,     0,     0,     0,    18,     0,     0,
      61,    62,    63,    64,    65,    93,     0,     0,   134,   140,
       0,   137,     0,     0,     0,     0,     0,    99,   100,    13,
       0,   103,     0,   105,     0,   153,     0,   115,     0,   110,
       0,     0,     0,     0,     0,   120,    20,    24,     0,     0,
       0,    12,    26,     0,    32,    34,    35,    36,    37,    40,
      39,    42,    43,    44,    45,    47,    48,    50,    51,    53,
      54,     0,    58,    59,    60,    74,    83,     0,     0,     0,
     125,     0,     0,     0,   133,     0,   150,   149,   161,   160,
       0,   151,    21,     0,   170,   171,     0,   158,     0,     0,
     139,   136,   138,    23,     0,     0,     0,    89,    91,     0,
     163,   155,   146,   104,    97,   101,   106,   102,     0,     0,
       0,   112,   111,   108,   128,   119,   126,   129,    25,     0,
       0,   164,    27,     0,     0,    77,    80,     0,   122,     0,
       0,   152,    22,     0,     0,   135,   141,     0,    87,     0,
       0,   116,     0,     0,   166,   165,     0,    56,    90,   121,
     130,   131,   159,   157,     0,     0,    92,   113,     0,   127,
     167,    88,   114
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -155,  -155,  -155,  -155,  -155,    12,   207,   212,   173,   208,
     219,   220,  -155,   -10,    -8,  -155,  -155,  -155,   216,     0,
     -40,    62,  -155,  -154,  -155,  -155,  -155,    -5,    16,     4,
    -155,  -155,  -155,  -155,  -155,   -92,    74,  -155,  -155,  -155,
    -155,   104,   -76,  -155,   -61,    13,  -155,   121,  -125,   -18,
    -155,   202,   203
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,   246,
      42,    43,    44,   247,    45,    46,    47,    48,   244,    87,
      88,    49,    94,    50,   177,   178,   100,    51,    52,    53,
      54,   159,   160,    55,    89,    90,    57,   231,    79,   105,
      58,    59,    80
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      41,    70,    71,   132,   161,    73,    74,   183,   108,   248,
      84,    92,    99,    56,    78,    85,    85,   123,   124,    65,
     168,   129,    69,   140,   237,    93,    72,   188,    76,   223,
      75,   193,     7,   228,   181,   216,    86,   229,   101,   147,
    -169,   251,   161,   131,   125,   109,   161,   288,   130,    77,
     150,   151,   152,   153,   154,   158,    96,   230,    84,   113,
      85,    17,   130,    85,   275,   276,   164,   137,   142,   139,
     233,   167,    97,   141,   130,   270,    85,   128,    76,   114,
     126,   290,     7,   176,    86,   161,   308,   146,   224,   182,
     185,   271,   104,   158,   190,   126,   242,   158,    85,   234,
     126,  -170,    85,   172,   174,   143,   144,   170,    76,   145,
     180,  -168,     7,   239,   175,   211,  -171,   213,   214,  -169,
     298,   194,   135,   217,   197,   102,   103,   221,   119,   120,
     259,   104,   106,   107,   260,   306,   158,   148,   104,   173,
      82,    85,    83,   235,    78,  -169,   240,   241,    82,    82,
      83,    83,    85,  -171,   249,   136,   226,   227,   302,   303,
     149,    78,   254,   162,   254,   252,   163,    85,   258,    85,
     254,   264,   265,   293,   267,    85,   255,   228,   257,   269,
     165,   229,   166,   161,   263,    60,    61,    62,    63,    64,
     110,   111,   112,     1,     2,    95,    67,     5,     6,     7,
     115,   116,   117,   118,     1,     2,     3,    67,     5,     6,
       7,   222,   278,   225,   169,     8,     9,    10,   176,    82,
     171,    83,    96,   -57,   -57,   138,     8,     9,    10,   285,
    -169,   143,   144,   186,   158,   145,   191,    18,    97,    85,
     121,   122,    14,    15,    16,    19,    68,   212,    18,   291,
     218,    21,   292,    22,    98,   219,    19,    68,   126,   127,
     296,   250,    21,   297,    22,   132,   145,   299,   261,   300,
     301,     1,     2,    66,    67,     5,     6,     7,    78,    78,
     307,    82,   266,    83,   273,   179,   274,   304,   201,   202,
     203,   204,   277,     8,     9,    10,   243,    82,   312,    83,
      96,   268,    82,   283,    83,   131,   272,    82,   287,    83,
     282,    82,   284,    83,   279,    18,    97,   195,   196,   198,
     133,   134,   280,    19,    68,   199,   200,   205,   206,    21,
     289,    22,   220,     1,     2,     3,     4,     5,     6,     7,
     207,   208,   294,   209,   210,   309,   215,   310,   286,   305,
     236,   281,   238,     0,     0,     8,     9,    10,     0,     0,
       0,     0,     0,     0,     0,     0,    11,     0,     0,    12,
      13,    14,    15,    16,    17,     0,     0,    18,     1,     2,
       3,    67,     5,     6,     7,    19,    20,     0,     0,     0,
       0,    21,     0,    22,     0,     0,     0,     0,     0,     0,
       8,     9,    10,     0,     0,     0,     0,     0,     0,     0,
     311,    11,     0,     0,    12,    13,    14,    15,    16,     0,
       0,     0,    18,     1,     2,     3,    67,     5,     6,     7,
      19,    68,     0,     0,     0,     0,    21,     0,    22,     0,
       0,     0,     0,     0,     0,     8,     9,    10,     1,     2,
      66,     4,     5,     6,     7,     0,    11,     0,     0,    12,
      13,    14,    15,    16,     0,     0,     0,    18,     0,     0,
       8,     9,    10,     0,     0,    19,    68,     0,     0,     0,
       0,    21,     0,    22,     0,     0,     0,     0,     0,    17,
       0,     0,    18,    81,     0,     0,     0,     0,     0,     0,
      19,    20,     0,    82,     0,    83,    21,     0,    22,     1,
       2,     3,    67,     5,     6,     7,     0,     0,     0,     1,
       2,    66,    67,     5,     6,     7,     0,     0,     0,     0,
       0,     8,     9,    10,     0,     0,     0,     0,     0,   245,
       0,     8,     9,    10,     0,     0,     0,    14,    15,    16,
       0,     0,     0,    18,     0,     0,     0,     0,     0,     0,
       0,    19,    68,    18,    81,     0,     0,    21,     0,    22,
       0,    19,    68,     0,    82,     0,    83,    21,     0,    22,
       1,     2,   155,    67,     5,     6,     7,     0,     0,     0,
       0,     1,     2,   155,    67,     5,     6,     7,     0,     0,
       0,     0,     8,     9,    10,     0,     0,     0,     0,   156,
       0,     0,     0,     8,     9,    10,     0,     0,     0,     0,
     156,     0,     0,     0,    18,   157,     0,     0,     0,     0,
       0,     0,    19,    68,   187,    18,   157,     0,    21,     0,
      22,     0,     0,    19,    68,   192,     0,     0,     0,    21,
       0,    22,     1,     2,   155,    67,     5,     6,     7,     0,
       0,     0,     0,     1,     2,    66,    67,     5,     6,     7,
       0,     0,     0,     0,     8,     9,    10,     0,     0,     0,
       0,   156,     0,     0,     0,     8,     9,    10,     0,     0,
       0,     0,     0,     0,     0,     0,    18,   157,     0,     0,
       0,     0,     0,     0,    19,    68,   232,    18,    81,     0,
      21,     0,    22,     0,     0,    19,    68,     0,     0,     0,
       0,    21,    91,    22,     1,     2,   155,    67,     5,     6,
       7,     0,     0,     0,     1,     2,    66,    67,     5,     6,
       7,     0,     0,     0,     0,     0,     8,     9,    10,     0,
       0,     0,     0,   156,     0,     0,     8,     9,    10,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    18,   157,
       0,     0,     0,     0,     0,     0,    19,    68,    18,    81,
       0,     0,    21,     0,    22,     0,    19,    68,   253,     0,
       0,     0,    21,     0,    22,     1,     2,    66,    67,     5,
       6,     7,     0,     0,     0,     0,     1,     2,    66,    67,
       5,     6,     7,     0,     0,     0,     0,     8,     9,    10,
       0,     0,     0,     0,     0,     0,     0,     0,     8,     9,
      10,     0,     0,     0,     0,     0,     0,     0,     0,    18,
      81,     0,     0,     0,     0,     0,     0,    19,    68,   256,
      18,    81,     0,    21,     0,    22,     0,     0,    19,    68,
       0,     0,     0,     0,    21,   262,    22,     1,     2,    66,
      67,     5,     6,     7,     0,     0,     0,     0,     1,     2,
      66,    67,     5,     6,     7,     0,     0,     0,     0,     8,
       9,    10,     0,     1,     2,    66,     4,     5,     6,     7,
       8,     9,    10,     0,     0,     0,     0,     0,     0,     0,
       0,    18,     0,     0,     0,     8,     9,    10,     0,    19,
      68,   189,    18,   184,     0,    21,     0,    22,     0,     0,
      19,    68,     0,     0,    17,     0,    21,    18,    22,     0,
       0,     0,     0,     0,     0,    19,    20,     0,     0,     0,
       0,    21,     0,    22,     1,     2,    66,    67,     5,     6,
       7,     0,     0,     0,     1,     2,    66,    67,     5,     6,
       7,     0,     0,     0,     0,     0,     8,     9,    10,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    18,     0,
       0,     0,     0,     0,     0,     0,    19,    68,    18,     0,
       0,     0,    21,   295,    22,     0,    19,    68,     0,     0,
       0,     0,    21,     0,    22
};

static const yytype_int16 yycheck[] =
{
       0,    11,    12,    43,    65,    13,    16,    99,    26,   163,
      20,    21,    22,     0,    19,    20,    21,    18,    19,    56,
      81,    33,    10,     6,   149,    21,     5,   103,     5,     5,
      18,   107,     9,     3,    59,    33,    20,     7,     0,    57,
      65,   166,   103,    43,    45,    52,   107,    33,    60,    26,
      60,    61,    62,    63,    64,    65,    32,    27,    68,    27,
      65,    44,    60,    68,   218,   219,    44,    51,    55,    53,
     146,    81,    48,    56,    60,    46,    81,    58,     5,    47,
      58,    44,     9,    40,    68,   146,    44,    56,    64,    46,
     100,    62,    61,   103,   104,    58,   157,   107,   103,    26,
      58,    55,   107,    87,    88,    48,    49,    46,     5,    52,
      94,    65,     9,    59,    57,   125,    55,   127,   128,    65,
     274,   109,    38,   133,   111,    55,    56,   137,    10,    11,
      36,    61,    55,    56,    40,   289,   146,    65,    61,    57,
      58,   146,    60,   148,   149,    65,   156,   157,    58,    58,
      60,    60,   157,    55,   164,    64,   143,   144,   283,   284,
      55,   166,   172,    46,   174,   170,    46,   172,   176,   174,
     180,   181,   182,   265,   184,   180,   172,     3,   174,   189,
      55,     7,    55,   244,   180,    20,    21,    22,    23,    24,
      48,    49,    50,     3,     4,     5,     6,     7,     8,     9,
      12,    13,    14,    15,     3,     4,     5,     6,     7,     8,
       9,   137,   222,   139,    57,    25,    26,    27,    40,    58,
      57,    60,    32,    58,    59,    64,    25,    26,    27,   239,
      65,    48,    49,     5,   244,    52,     5,    47,    48,   244,
      16,    17,    41,    42,    43,    55,    56,     5,    47,   259,
      46,    61,   260,    63,    64,    46,    55,    56,    58,    59,
     270,     5,    61,   273,    63,   305,    52,   277,    62,   279,
     280,     3,     4,     5,     6,     7,     8,     9,   283,   284,
     290,    58,    64,    60,    46,    62,    46,   287,   115,   116,
     117,   118,    46,    25,    26,    27,    57,    58,   308,    60,
      32,    57,    58,    55,    60,   305,    57,    58,    34,    60,
      57,    58,    55,    60,    59,    47,    48,   110,   111,   112,
      37,    38,    59,    55,    56,   113,   114,   119,   120,    61,
      46,    63,    64,     3,     4,     5,     6,     7,     8,     9,
     121,   122,    62,   123,   124,    64,   130,    62,   244,   287,
     148,   230,   149,    -1,    -1,    25,    26,    27,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    36,    -1,    -1,    39,
      40,    41,    42,    43,    44,    -1,    -1,    47,     3,     4,
       5,     6,     7,     8,     9,    55,    56,    -1,    -1,    -1,
      -1,    61,    -1,    63,    -1,    -1,    -1,    -1,    -1,    -1,
      25,    26,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      35,    36,    -1,    -1,    39,    40,    41,    42,    43,    -1,
      -1,    -1,    47,     3,     4,     5,     6,     7,     8,     9,
      55,    56,    -1,    -1,    -1,    -1,    61,    -1,    63,    -1,
      -1,    -1,    -1,    -1,    -1,    25,    26,    27,     3,     4,
       5,     6,     7,     8,     9,    -1,    36,    -1,    -1,    39,
      40,    41,    42,    43,    -1,    -1,    -1,    47,    -1,    -1,
      25,    26,    27,    -1,    -1,    55,    56,    -1,    -1,    -1,
      -1,    61,    -1,    63,    -1,    -1,    -1,    -1,    -1,    44,
      -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    -1,    -1,
      55,    56,    -1,    58,    -1,    60,    61,    -1,    63,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    25,    26,    27,    -1,    -1,    -1,    -1,    -1,    33,
      -1,    25,    26,    27,    -1,    -1,    -1,    41,    42,    43,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    55,    56,    47,    48,    -1,    -1,    61,    -1,    63,
      -1,    55,    56,    -1,    58,    -1,    60,    61,    -1,    63,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      -1,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    -1,    25,    26,    27,    -1,    -1,    -1,    -1,    32,
      -1,    -1,    -1,    25,    26,    27,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    47,    48,    -1,    -1,    -1,    -1,
      -1,    -1,    55,    56,    57,    47,    48,    -1,    61,    -1,
      63,    -1,    -1,    55,    56,    57,    -1,    -1,    -1,    61,
      -1,    63,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    -1,    25,    26,    27,    -1,    -1,    -1,
      -1,    32,    -1,    -1,    -1,    25,    26,    27,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    48,    -1,    -1,
      -1,    -1,    -1,    -1,    55,    56,    57,    47,    48,    -1,
      61,    -1,    63,    -1,    -1,    55,    56,    -1,    -1,    -1,
      -1,    61,    62,    63,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    -1,
      -1,    -1,    -1,    32,    -1,    -1,    25,    26,    27,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    47,    48,
      -1,    -1,    -1,    -1,    -1,    -1,    55,    56,    47,    48,
      -1,    -1,    61,    -1,    63,    -1,    55,    56,    57,    -1,
      -1,    -1,    61,    -1,    63,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    -1,    25,    26,    27,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,
      27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    47,
      48,    -1,    -1,    -1,    -1,    -1,    -1,    55,    56,    57,
      47,    48,    -1,    61,    -1,    63,    -1,    -1,    55,    56,
      -1,    -1,    -1,    -1,    61,    62,    63,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,    25,
      26,    27,    -1,     3,     4,     5,     6,     7,     8,     9,
      25,    26,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    47,    -1,    -1,    -1,    25,    26,    27,    -1,    55,
      56,    46,    47,    59,    -1,    61,    -1,    63,    -1,    -1,
      55,    56,    -1,    -1,    44,    -1,    61,    47,    63,    -1,
      -1,    -1,    -1,    -1,    -1,    55,    56,    -1,    -1,    -1,
      -1,    61,    -1,    63,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    55,    56,    47,    -1,
      -1,    -1,    61,    62,    63,    -1,    55,    56,    -1,    -1,
      -1,    -1,    61,    -1,    63
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    25,    26,
      27,    36,    39,    40,    41,    42,    43,    44,    47,    55,
      56,    61,    63,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    90,    91,    92,    93,    97,
      99,   103,   104,   105,   106,   109,   111,   112,   116,   117,
      20,    21,    22,    23,    24,    56,     5,     6,    56,    71,
      79,    79,     5,    80,    79,    71,     5,    26,    93,   114,
     118,    48,    58,    60,    79,    93,    94,    95,    96,   110,
     111,    62,    79,    95,    98,     5,    32,    48,    64,    79,
     102,     0,    55,    56,    61,   115,    55,    56,   115,    52,
      48,    49,    50,    27,    47,    12,    13,    14,    15,    10,
      11,    16,    17,    18,    19,    45,    58,    59,    58,    33,
      60,    85,    86,    37,    38,    38,    64,    94,    64,    94,
       6,    56,   111,    48,    49,    52,    56,   115,    65,    55,
      79,    79,    79,    79,    79,     5,    32,    48,    79,   107,
     108,   110,    46,    46,    44,    55,    55,    79,   110,    57,
      46,    57,    94,    57,    94,    57,    40,   100,   101,    62,
      94,    59,    46,   101,    59,    79,     5,    57,   108,    46,
      79,     5,    57,   108,    71,    72,    72,   111,    72,    73,
      73,    74,    74,    74,    74,    75,    75,    76,    76,    77,
      77,    79,     5,    79,    79,    84,    33,    79,    46,    46,
      64,    79,   102,     5,    64,   102,   111,   111,     3,     7,
      27,   113,    57,   108,    26,    93,   117,   114,   118,    59,
      79,    79,   110,    57,    94,    33,    85,    89,    89,    79,
       5,   114,    93,    57,    79,    95,    57,    95,    80,    36,
      40,    62,    62,    95,    79,    79,    64,    79,    57,    79,
      46,    62,    57,    46,    46,    89,    89,    46,    79,    59,
      59,   113,    57,    55,    55,    79,   107,    34,    33,    46,
      44,    79,    80,   101,    62,    62,    79,    79,    89,    79,
      79,    79,   114,   114,    85,    87,    89,    79,    44,    64,
      62,    35,    79
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    66,    67,    67,    68,    68,    68,    68,    68,    68,
      68,    68,    68,    69,    69,    69,    69,    69,    69,    69,
      69,    70,    70,    70,    70,    70,    70,    70,    71,    71,
      71,    72,    72,    73,    73,    73,    73,    73,    74,    74,
      74,    75,    75,    75,    75,    75,    76,    76,    76,    77,
      77,    77,    78,    78,    78,    79,    79,    80,    80,    81,
      81,    82,    82,    82,    82,    82,    83,    83,    84,    84,
      84,    84,    84,    85,    85,    85,    86,    86,    86,    86,
      86,    87,    87,    87,    87,    88,    88,    89,    89,    90,
      90,    91,    92,    93,    93,    94,    94,    95,    95,    95,
      95,    96,    96,    97,    97,    97,    97,    98,    98,    99,
      99,    99,    99,   100,   100,   101,   101,   102,   102,   103,
     103,   103,   103,   104,   104,   104,   104,   104,   105,   105,
     105,   105,   106,   106,   107,   107,   107,   107,   107,   107,
     108,   108,   109,   109,   109,   109,   110,   111,   111,   111,
     111,   111,   111,   111,   112,   112,   112,   112,   112,   112,
     113,   113,   114,   114,   115,   115,   115,   115,   116,   116,
     117,   117,   118
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     2,     1,     1,     1,     1,
       1,     2,     3,     3,     1,     1,     1,     1,     2,     2,
       3,     3,     4,     4,     3,     4,     3,     4,     1,     2,
       2,     1,     3,     1,     3,     3,     3,     3,     1,     3,
       3,     1,     3,     3,     3,     3,     1,     3,     3,     1,
       3,     3,     1,     3,     3,     1,     5,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     1,     1,     1,
       2,     1,     1,     1,     3,     2,     1,     4,     1,     1,
       4,     2,     1,     3,     2,     1,     1,     2,     4,     4,
       5,     4,     6,     1,     1,     1,     1,     1,     1,     2,
       2,     3,     3,     3,     4,     3,     4,     1,     3,     2,
       3,     4,     4,     4,     5,     1,     3,     1,     1,     4,
       3,     5,     4,     2,     2,     3,     4,     6,     4,     4,
       5,     5,     2,     3,     1,     3,     2,     1,     2,     2,
       1,     3,     1,     1,     1,     1,     3,     1,     1,     3,
       3,     3,     4,     3,     1,     4,     2,     5,     3,     5,
       1,     1,     1,     3,     3,     4,     4,     5,     1,     1,
       1,     1,     1
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
          case 67: /* input  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1367 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 68: /* primary_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1373 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 69: /* indexable  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1379 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 70: /* callable  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1385 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 71: /* unary_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1391 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 72: /* power_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1397 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 73: /* multiply_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1403 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 74: /* additive_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1409 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 75: /* relational_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1415 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 76: /* equality_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1421 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 77: /* logical_and_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1427 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 78: /* logical_or_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1433 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 79: /* exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1439 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 81: /* assignment_exp1  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1445 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 82: /* assignment_exp2  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1451 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 83: /* assignment_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1457 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 84: /* small_stmt  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1463 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 85: /* simple_stmt  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1469 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 86: /* compound_stmt  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1475 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 87: /* stmt  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1481 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 88: /* statement  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1487 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 89: /* suite  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1493 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 90: /* if_stmt  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1499 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 91: /* while_stmt  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1505 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 92: /* for_stmt  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1511 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 95: /* item  */
#line 68 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).named_argument).second; }
#line 1517 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 96: /* items2  */
#line 67 "ExpressionParser.y" /* yacc.c:1257  */
      { for(auto &v : ((*yyvaluep).named_arguments)) {delete v.second;} }
#line 1523 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 97: /* tuple  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1529 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 98: /* items  */
#line 67 "ExpressionParser.y" /* yacc.c:1257  */
      { for(auto &v : ((*yyvaluep).named_arguments)) {delete v.second;} }
#line 1535 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 99: /* list  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1541 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 100: /* comprehension0  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1547 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 101: /* comprehension  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1553 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 103: /* dict1  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1559 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 104: /* dict  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1565 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 105: /* idict1  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1571 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 106: /* idict  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1577 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 107: /* arg  */
#line 68 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).named_argument).second; }
#line 1583 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 108: /* args  */
#line 67 "ExpressionParser.y" /* yacc.c:1257  */
      { for(auto &v : ((*yyvaluep).named_arguments)) {delete v.second;} }
#line 1589 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 109: /* num  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1595 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 110: /* range  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1601 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 111: /* unit_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1607 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 115: /* indexer  */
#line 69 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).component).e1; delete ((*yyvaluep).component).e2; }
#line 1613 "ExpressionParser.tab.c" /* yacc.c:1257  */
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
#line 75 "ExpressionParser.y" /* yacc.c:1646  */
    { ScanResult = (yyvsp[0].expr); valueExpression = true; (yyval.expr)=0;                                       }
#line 1877 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 76 "ExpressionParser.y" /* yacc.c:1646  */
    { ScanResult = (yyvsp[0].expr); unitExpression = true; (yyval.expr)=0;                                        }
#line 1883 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 80 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1889 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 81 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-1].expr), OperatorExpression::UNIT, (yyvsp[0].expr));  }
#line 1895 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 82 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new StringExpression(DocumentObject, (yyvsp[0].string)); }
#line 1901 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 83 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new StringExpression(DocumentObject, (yyvsp[0].string), true); }
#line 1907 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 84 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new VariableExpression(DocumentObject, (yyvsp[0].path)); }
#line 1913 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 85 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1919 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 86 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1925 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 87 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-1].expr)->addComponent((yyvsp[0].component)); (yyval.expr) = (yyvsp[-1].expr); }
#line 1931 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 88 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].expr)->addComponent((yyvsp[0].string)); (yyval.expr) = (yyvsp[-2].expr); }
#line 1937 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 91 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 1943 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 92 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1949 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 93 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1955 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 94 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1961 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 95 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1967 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 96 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new VariableExpression(DocumentObject,(yyvsp[-1].path)); (yyval.expr)->addComponent((yyvsp[0].component)); }
#line 1973 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 97 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-1].expr)->addComponent((yyvsp[0].component)); (yyval.expr) = (yyvsp[-1].expr); }
#line 1979 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 98 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].expr)->addComponent((yyvsp[0].string)); (yyval.expr) = (yyvsp[-2].expr); }
#line 1985 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 101 "ExpressionParser.y" /* yacc.c:1646  */
    { 
                                                    (yyval.expr) = CallableExpression::create(DocumentObject,(yyvsp[-2].path));
                                                    if(!(yyval.expr)) { 
                                                        YYABORT;
                                                    } 
                                                }
#line 1996 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 107 "ExpressionParser.y" /* yacc.c:1646  */
    { 
                                                    (yyval.expr) = CallableExpression::create(DocumentObject, (yyvsp[-3].path), (yyvsp[-1].named_arguments)); 
                                                    if(!(yyval.expr)) { 
                                                        YYABORT;
                                                    } 
                                                }
#line 2007 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 113 "ExpressionParser.y" /* yacc.c:1646  */
    {   // This rule exists because of possible name clash of 
                                                    // function and unit, e.g. min
                                                    ObjectIdentifier name(DocumentObject);
                                                    name << ObjectIdentifier::SimpleComponent((yyvsp[-3].quantity).unitStr);
                                                    (yyval.expr) = CallableExpression::create(DocumentObject, name, (yyvsp[-1].named_arguments));
                                                    if(!(yyval.expr)) { 
                                                        YYABORT;
                                                    } 
                                                }
#line 2021 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 122 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-2].expr)); }
#line 2027 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 123 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-3].expr), (yyvsp[-1].named_arguments)); }
#line 2033 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 124 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-2].expr)); }
#line 2039 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 125 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-3].expr), (yyvsp[-1].named_arguments)); }
#line 2045 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 129 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2051 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 130 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))); }
#line 2057 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 131 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::POS, new NumberExpression(DocumentObject, Quantity(1))); }
#line 2063 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 135 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2069 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 136 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, (yyvsp[0].expr));   }
#line 2075 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 139 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2081 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 140 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 2087 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 141 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 2093 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 142 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 2099 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 143 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MOD, (yyvsp[0].expr));   }
#line 2105 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 147 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2111 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 148 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::ADD, (yyvsp[0].expr));   }
#line 2117 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 149 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::SUB, (yyvsp[0].expr));   }
#line 2123 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 153 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2129 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 154 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LT, (yyvsp[0].expr));    }
#line 2135 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 155 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GT, (yyvsp[0].expr));    }
#line 2141 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 156 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GTE, (yyvsp[0].expr));   }
#line 2147 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 157 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LTE, (yyvsp[0].expr));   }
#line 2153 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 161 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2159 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 162 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::EQ, (yyvsp[0].expr));    }
#line 2165 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 163 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::NEQ, (yyvsp[0].expr));   }
#line 2171 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 167 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2177 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 168 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::AND_OP, (yyvsp[0].expr));   }
#line 2183 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 169 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::AND_OP2, (yyvsp[0].expr));   }
#line 2189 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 174 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2195 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 175 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::OR_OP, (yyvsp[0].expr));   }
#line 2201 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 176 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::OR_OP2, (yyvsp[0].expr));   }
#line 2207 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 180 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2213 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 181 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ConditionalExpression(DocumentObject, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr));                     }
#line 2219 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 185 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_list).push_back((yyvsp[0].string)); }
#line 2225 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 186 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].string_list).push_back((yyvsp[0].string)); (yyval.string_list).swap((yyvsp[-2].string_list)); }
#line 2231 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 190 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new AssignmentExpression(DocumentObject,(yyvsp[-2].string_list),(yyvsp[0].expr)); }
#line 2237 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 191 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<AssignmentExpression*>((yyvsp[-2].expr))->add((yyvsp[0].expr)); (yyval.expr) = (yyvsp[-2].expr); }
#line 2243 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 195 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new AssignmentExpression(DocumentObject,(yyvsp[-2].string),OperatorExpression::MUL,(yyvsp[0].expr)); }
#line 2249 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 196 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new AssignmentExpression(DocumentObject,(yyvsp[-2].string),OperatorExpression::DIV,(yyvsp[0].expr)); }
#line 2255 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 197 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new AssignmentExpression(DocumentObject,(yyvsp[-2].string),OperatorExpression::MOD,(yyvsp[0].expr)); }
#line 2261 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 198 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new AssignmentExpression(DocumentObject,(yyvsp[-2].string),OperatorExpression::ADD,(yyvsp[0].expr)); }
#line 2267 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 65:
#line 199 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new AssignmentExpression(DocumentObject,(yyvsp[-2].string),OperatorExpression::SUB,(yyvsp[0].expr)); }
#line 2273 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 203 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2279 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 204 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2285 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 68:
#line 208 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2291 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 209 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2297 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 210 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new JumpStatement(DocumentObject,JumpStatement::JUMP_RETURN,(yyvsp[0].expr)); }
#line 2303 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 211 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new JumpStatement(DocumentObject,JumpStatement::JUMP_BREAK); }
#line 2309 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 212 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new JumpStatement(DocumentObject,JumpStatement::JUMP_CONTINUE); }
#line 2315 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 216 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2321 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 217 "ExpressionParser.y" /* yacc.c:1646  */
    { 
                                                    SimpleStatement *stmt = dynamic_cast<SimpleStatement*>((yyvsp[-2].expr)); 
                                                    if(!stmt)
                                                        stmt = new SimpleStatement(DocumentObject, (yyvsp[-2].expr));
                                                    stmt->add((yyvsp[0].expr)); 
                                                    (yyval.expr) = stmt;
                                                 }
#line 2333 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 224 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2339 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 76:
#line 228 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2345 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 77:
#line 229 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<IfStatement*>((yyvsp[-3].expr))->addElse((yyvsp[0].expr)); (yyval.expr)=(yyvsp[-3].expr); }
#line 2351 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 230 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2357 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 231 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2363 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 80:
#line 232 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<ForStatement*>((yyvsp[-3].expr))->addElse((yyvsp[0].expr)); (yyval.expr)=(yyvsp[-3].expr); }
#line 2369 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 236 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new Statement(DocumentObject, (yyvsp[-1].expr)); }
#line 2375 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 237 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new Statement(DocumentObject, (yyvsp[0].expr)); }
#line 2381 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 238 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<Statement*>((yyvsp[-2].expr))->add((yyvsp[-1].expr)); (yyval.expr) = (yyvsp[-2].expr); }
#line 2387 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 239 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<Statement*>((yyvsp[-1].expr))->add((yyvsp[0].expr)); (yyval.expr) = (yyvsp[-1].expr); }
#line 2393 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 243 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2399 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 244 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2405 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 87:
#line 247 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2411 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 88:
#line 248 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2417 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 252 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new IfStatement(DocumentObject,(yyvsp[-2].expr),(yyvsp[0].expr)); }
#line 2423 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 90:
#line 253 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<IfStatement*>((yyval.expr))->addElseIf((yyvsp[-2].expr),(yyvsp[0].expr)); (yyval.expr)=(yyvsp[-4].expr); }
#line 2429 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 91:
#line 257 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new WhileStatement(DocumentObject,(yyvsp[-2].expr),(yyvsp[0].expr)); }
#line 2435 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 92:
#line 260 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ForStatement(DocumentObject,(yyvsp[-4].string_list),(yyvsp[-2].expr),(yyvsp[0].expr)); }
#line 2441 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 93:
#line 263 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string) = (yyvsp[0].string); }
#line 2447 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 94:
#line 264 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string) = (yyvsp[0].string); }
#line 2453 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 97:
#line 269 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first.clear(); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2459 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 98:
#line 270 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first.clear(); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2465 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 99:
#line 271 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = "*"; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2471 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 272 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = "*"; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2477 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 275 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_arguments).push_back((yyvsp[-2].named_argument)); (yyval.named_arguments).push_back((yyvsp[0].named_argument)); }
#line 2483 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 276 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].named_arguments).push_back((yyvsp[0].named_argument)); (yyval.named_arguments).swap((yyvsp[-2].named_arguments)); }
#line 2489 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 103:
#line 279 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject); }
#line 2495 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 104:
#line 280 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject, (yyvsp[-2].named_argument)); }
#line 2501 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 105:
#line 281 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject, (yyvsp[-1].named_arguments)); }
#line 2507 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 106:
#line 282 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject, (yyvsp[-2].named_arguments)); }
#line 2513 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 107:
#line 285 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_arguments).push_back((yyvsp[0].named_argument)); }
#line 2519 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 108:
#line 286 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].named_arguments).push_back((yyvsp[0].named_argument)); (yyval.named_arguments).swap((yyvsp[-2].named_arguments)); }
#line 2525 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 109:
#line 289 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ListExpression(DocumentObject); }
#line 2531 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 110:
#line 290 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ListExpression(DocumentObject, (yyvsp[-1].named_arguments)); }
#line 2537 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 111:
#line 291 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ListExpression(DocumentObject, (yyvsp[-2].named_arguments)); }
#line 2543 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 112:
#line 292 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<ComprehensionExpression*>((yyvsp[-1].expr))->setExpr((yyvsp[-2].expr)); (yyval.expr) = (yyvsp[-1].expr); }
#line 2549 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 113:
#line 296 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ComprehensionExpression(DocumentObject,(yyvsp[-2].string_list),(yyvsp[0].expr)); }
#line 2555 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 114:
#line 297 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<ComprehensionExpression*>((yyvsp[-4].expr))->add((yyvsp[-2].string_list),(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-4].expr); }
#line 2561 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 115:
#line 301 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2567 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 116:
#line 302 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<ComprehensionExpression*>((yyvsp[-2].expr))->setCondition((yyvsp[0].expr)); (yyval.expr) = (yyvsp[-2].expr); }
#line 2573 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 119:
#line 307 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new DictExpression(DocumentObject, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2579 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 120:
#line 308 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new DictExpression(DocumentObject, 0, (yyvsp[0].expr)); }
#line 2585 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 121:
#line 309 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<DictExpression*>((yyvsp[-4].expr))->addItem((yyvsp[-2].expr),(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-4].expr); }
#line 2591 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 122:
#line 310 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<DictExpression*>((yyvsp[-3].expr))->addItem(0,(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-3].expr); }
#line 2597 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 123:
#line 313 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new DictExpression(DocumentObject); }
#line 2603 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 124:
#line 314 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2609 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 125:
#line 315 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-2].expr); }
#line 2615 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 126:
#line 316 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<ComprehensionExpression*>((yyvsp[-1].expr))->setExpr((yyvsp[-2].expr),0,false); (yyval.expr)=(yyvsp[-1].expr); }
#line 2621 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 127:
#line 317 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<ComprehensionExpression*>((yyvsp[-1].expr))->setExpr((yyvsp[-4].expr),(yyvsp[-2].expr),false); (yyval.expr)=(yyvsp[-1].expr); }
#line 2627 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 128:
#line 320 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new IDictExpression(DocumentObject, (yyvsp[-2].string), (yyvsp[0].expr)); }
#line 2633 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 129:
#line 321 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new IDictExpression(DocumentObject, "**", (yyvsp[0].expr)); }
#line 2639 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 130:
#line 322 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<IDictExpression*>((yyvsp[-4].expr))->addItem((yyvsp[-2].string),(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-4].expr); }
#line 2645 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 131:
#line 323 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<IDictExpression*>((yyvsp[-4].expr))->addItem("**",(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-4].expr); }
#line 2651 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 132:
#line 326 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2657 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 133:
#line 327 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-2].expr); }
#line 2663 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 134:
#line 330 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first.clear(); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2669 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 135:
#line 331 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = (yyvsp[-2].string); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2675 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 136:
#line 332 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = "*"; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2681 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 137:
#line 333 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first.clear(); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2687 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 138:
#line 334 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = '*'; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2693 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 139:
#line 335 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = "**"; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2699 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 140:
#line 338 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_arguments).push_back((yyvsp[0].named_argument)); }
#line 2705 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 141:
#line 339 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].named_arguments).push_back((yyvsp[0].named_argument)); (yyval.named_arguments).swap((yyvsp[-2].named_arguments)); }
#line 2711 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 142:
#line 342 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 2717 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 143:
#line 343 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 2723 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 144:
#line 344 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue)));                }
#line 2729 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 145:
#line 345 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ConstantExpression(DocumentObject, (yyvsp[0].constant).name, Quantity((yyvsp[0].constant).fvalue));      }
#line 2735 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 146:
#line 348 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new RangeExpression(DocumentObject, (yyvsp[-2].string), (yyvsp[0].string));                               }
#line 2741 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 147:
#line 351 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 2747 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 148:
#line 352 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new UnitExpression(DocumentObject, Quantity::Inch, (yyvsp[0].string));                }
#line 2753 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 149:
#line 353 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 2759 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 150:
#line 354 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 2765 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 151:
#line 355 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))));   }
#line 2771 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 152:
#line 356 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-3].expr), OperatorExpression::POW, new OperatorExpression(DocumentObject, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))));   }
#line 2777 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 153:
#line 357 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr);                                                                        }
#line 2783 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 154:
#line 360 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of the current object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject) << ObjectIdentifier::SimpleComponent((yyvsp[0].string));
                                                }
#line 2791 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 155:
#line 363 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of a sub-object of the current object*/
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  if(_Reader) {
                                                    (yyvsp[-2].string_or_identifier) = ObjectIdentifier::String(
                                                        PropertyLinkBase::importSubName(*_Reader,(yyvsp[-2].string_or_identifier).getString().c_str()),true);
                                                  }
                                                  (yyval.path).setDocumentObjectName(DocumentObject,false,(yyvsp[-2].string_or_identifier));
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 2805 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 156:
#line 372 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of the current document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentObjectName(DocumentObject);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 2815 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 157:
#line 377 "ExpressionParser.y" /* yacc.c:1646  */
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
#line 2832 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 158:
#line 389 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of a given document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  if(_Reader && !(yyvsp[-2].string_or_identifier).isRealString())
                                                      (yyvsp[-2].string_or_identifier) = ObjectIdentifier::String(_Reader->getName((yyvsp[-2].string_or_identifier).getString().c_str()));
                                                  (yyval.path) << ObjectIdentifier::SimpleComponent((yyvsp[-2].string_or_identifier));
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                  (yyval.path).resolveAmbiguity();
                                                }
#line 2845 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 159:
#line 397 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property from an external document, within a named document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentName((yyvsp[-4].string_or_identifier), true);
                                                  (yyval.path).setDocumentObjectName((yyvsp[-2].string_or_identifier), true);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                  (yyval.path).resolveAmbiguity();
                                                }
#line 2857 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 160:
#line 406 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.ivalue) = (yyvsp[0].ivalue); }
#line 2863 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 161:
#line 407 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.ivalue) = (yyvsp[0].fvalue); }
#line 2869 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 162:
#line 410 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_back(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));}
#line 2875 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 163:
#line 411 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].components).push_back(ObjectIdentifier::SimpleComponent((yyvsp[0].string))); (yyval.components).swap((yyvsp[-2].components)); }
#line 2881 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 164:
#line 414 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = Expression::Component((yyvsp[-1].expr));   }
#line 2887 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 165:
#line 415 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = Expression::Component((yyvsp[-2].expr),0,true); }
#line 2893 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 166:
#line 416 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = Expression::Component(0,(yyvsp[-1].expr)); }
#line 2899 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 167:
#line 417 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = Expression::Component((yyvsp[-3].expr),(yyvsp[-1].expr));}
#line 2905 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 168:
#line 420 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 2911 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 169:
#line 421 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string));       }
#line 2917 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 170:
#line 424 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 2923 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 171:
#line 425 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string));       }
#line 2929 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 172:
#line 428 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 2935 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;


#line 2939 "ExpressionParser.tab.c" /* yacc.c:1646  */
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
#line 431 "ExpressionParser.y" /* yacc.c:1906  */

