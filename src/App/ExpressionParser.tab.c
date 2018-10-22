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
    PSTRING = 280,
    PRSTRING = 281,
    RSTRING = 282,
    STRING = 283,
    MINUSSIGN = 284,
    PROPERTY_REF = 285,
    DOCUMENT = 286,
    OBJECT = 287,
    EXPONENT = 288,
    EXPAND = 289,
    NEWLINE = 290,
    INDENT = 291,
    DEDENT = 292,
    IF = 293,
    ELIF = 294,
    ELSE = 295,
    WHILE = 296,
    FOR = 297,
    BREAK = 298,
    CONTINUE = 299,
    RETURN = 300,
    IN = 301,
    PY_BEGIN = 302,
    PY_END = 303,
    NUM_AND_UNIT = 304,
    NEG = 305,
    POS = 306
  };
#endif

/* Value type.  */


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_EXPRESSIONPARSER_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 173 "ExpressionParser.tab.c" /* yacc.c:358  */

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
#define YYFINAL  104
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1156

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  70
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  54
/* YYNRULES -- Number of rules.  */
#define YYNRULES  185
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  333

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   306

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    69,     2,    54,     2,     2,
      60,    61,    52,    51,    62,     2,    59,    53,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    50,    64,
       2,    63,     2,    49,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    65,     2,    66,    56,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    67,     2,    68,     2,     2,     2,     2,
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
      45,    46,    47,    48,    55,    57,    58
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    75,    75,    76,    80,    81,    82,    83,    84,    85,
      86,    87,    91,    92,    93,    94,    95,    96,    97,    98,
     101,   102,   103,   104,   105,   106,   107,   108,   111,   117,
     123,   132,   133,   134,   135,   139,   140,   141,   145,   146,
     149,   150,   151,   152,   153,   157,   158,   159,   163,   164,
     165,   166,   167,   171,   172,   173,   177,   178,   179,   184,
     185,   186,   190,   191,   195,   196,   200,   201,   205,   206,
     207,   208,   209,   213,   214,   218,   219,   220,   221,   222,
     223,   224,   228,   229,   236,   240,   241,   242,   243,   244,
     248,   249,   250,   251,   255,   256,   259,   260,   264,   265,
     269,   272,   275,   276,   279,   279,   281,   282,   283,   284,
     287,   288,   291,   292,   293,   294,   297,   298,   301,   302,
     303,   304,   308,   309,   313,   314,   317,   317,   319,   320,
     321,   322,   325,   326,   327,   328,   329,   332,   333,   334,
     335,   338,   339,   342,   343,   344,   345,   346,   347,   350,
     351,   354,   355,   356,   357,   360,   363,   364,   365,   366,
     367,   368,   369,   372,   375,   384,   389,   401,   409,   418,
     419,   422,   423,   426,   427,   428,   429,   430,   431,   432,
     433,   436,   437,   440,   441,   444
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
  "DIV_ASSIGN", "MOD_ASSIGN", "ADD_ASSIGN", "SUB_ASSIGN", "PSTRING",
  "PRSTRING", "RSTRING", "STRING", "MINUSSIGN", "PROPERTY_REF", "DOCUMENT",
  "OBJECT", "EXPONENT", "EXPAND", "NEWLINE", "INDENT", "DEDENT", "IF",
  "ELIF", "ELSE", "WHILE", "FOR", "BREAK", "CONTINUE", "RETURN", "IN",
  "PY_BEGIN", "PY_END", "'?'", "':'", "'+'", "'*'", "'/'", "'%'",
  "NUM_AND_UNIT", "'^'", "NEG", "POS", "'.'", "'('", "')'", "','", "'='",
  "';'", "'['", "']'", "'{'", "'}'", "'#'", "$accept", "input",
  "primary_exp", "string", "indexable", "callable", "unary_exp",
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
     295,   296,   297,   298,   299,   300,   301,   302,   303,    63,
      58,    43,    42,    47,    37,   304,    94,   305,   306,    46,
      40,    41,    44,    61,    59,    91,    93,   123,   125,    35
};
# endif

#define YYPACT_NINF -155

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-155)))

#define YYTABLE_NINF -185

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     201,  -155,  -155,   200,    22,  -155,  -155,  -155,  -155,  -155,
    -155,   -43,  1089,  1089,  1089,    12,  -155,  -155,  1089,  -155,
    -155,  -155,  1089,    62,   560,    80,   276,    47,  -155,   263,
      65,    91,  -155,     4,   300,    13,   306,   116,   121,    27,
    -155,   175,    30,  -155,  -155,  -155,   -26,  -155,   457,  -155,
     217,  -155,    73,    59,  -155,  -155,    95,  -155,   135,  -155,
       5,   159,    36,    79,    96,  1089,  1089,  1089,  1089,  1089,
     712,   103,    22,   777,  -155,   131,   134,  -155,    35,  -155,
    -155,  -155,  -155,  -155,   132,   160,  1089,  -155,   164,    -7,
     138,   189,  -155,   180,  -155,   192,  -155,   105,   -41,  -155,
    -155,  -155,   -13,   939,  -155,  -155,  -155,  -155,  -155,   226,
     625,  1004,  -155,   249,   636,  -155,  1089,  1089,  1014,  1089,
    1089,  1089,  1089,  1089,  1089,  1089,  1089,  1089,  1089,  1089,
    1089,  1089,  1089,   259,  1089,  1089,  -155,   533,     6,  -155,
    1089,   221,   223,  -155,  -155,  -155,   342,  -155,    10,  -155,
       5,   219,     5,     5,    20,   701,  -155,   107,    62,  -155,
    -155,  -155,  -155,  -155,    97,  1089,  1089,  -155,  -155,   245,
    -155,   484,   484,  1089,   273,   174,  -155,  -155,  -155,   174,
     788,  -155,   853,  -155,    12,   156,   220,  -155,   864,  1089,
    1089,   225,  1089,  -155,  -155,  -155,   250,  1079,    44,  -155,
    -155,   261,  -155,     4,     4,   219,     4,   300,   300,    13,
      13,    13,    13,   306,   306,   116,   116,   121,   121,   274,
    -155,  -155,  -155,  -155,  -155,   308,   484,   484,  -155,   309,
    1089,   232,  -155,   297,   219,   219,  -155,  -155,   210,  -155,
    -155,   278,  -155,  -155,   267,   132,   302,  1089,  -155,  -155,
    -155,  -155,   712,   326,    15,  -155,  -155,   313,  -155,   132,
    -155,  -155,  -155,  -155,  -155,  -155,    49,  1089,    12,  -155,
    -155,  -155,  -155,   192,  -155,  -155,  -155,  1089,    53,   929,
    -155,  -155,  1089,   484,  -155,  -155,  1089,  -155,  1089,  1089,
    -155,  -155,   174,   174,  -155,  -155,   457,  -155,   484,  1089,
    -155,    87,   296,   299,  1089,  -155,  1089,  -155,    86,  -155,
    -155,  -155,  -155,  -155,   132,   132,   -26,   408,  -155,  -155,
    1089,  -155,  -155,   307,   311,  1089,  -155,  -155,  -155,  -155,
    -155,   312,  -155
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,   151,   152,   102,   156,   153,   154,   103,    14,    15,
      13,    12,     0,     0,     0,     0,    78,    79,     0,   157,
      80,    81,     0,     0,     0,     0,     0,     0,    35,     6,
       8,     9,    38,    40,    45,    48,    53,    56,    59,    62,
      76,     0,    73,    74,    75,    82,    94,    91,    95,     2,
      85,    87,    88,   163,    21,    22,     0,    23,     0,    24,
       4,     3,     7,     0,     0,     0,     0,     0,     0,     0,
       0,   102,     0,     0,    36,     0,     0,    64,     0,    77,
      37,   102,   185,   171,   165,     0,     0,   112,   106,   163,
       0,     0,   107,     0,   118,   106,   116,     0,   102,   127,
     126,   132,     0,     0,     1,    18,    19,    17,    16,     0,
       0,     0,    26,     0,     0,    10,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    90,    84,     0,    93,
       0,     0,     0,   104,   105,   133,     0,   141,     0,   156,
       0,     5,     0,     0,     0,     0,    25,     0,     0,    68,
      69,    70,    71,    72,   102,     0,     0,   143,   149,     0,
     146,     0,     0,     0,     0,     0,   108,   109,    20,     0,
       0,   114,     0,   162,     0,   124,     0,   119,     0,     0,
       0,     0,     0,   129,    27,    31,     0,     0,     0,    11,
      33,     0,    39,    41,    42,    43,    44,    47,    46,    49,
      50,    51,    52,    54,    55,    57,    58,    60,    61,     0,
      65,    66,    67,    83,    92,     0,     0,     0,   134,     0,
       0,     0,   142,     0,   159,   158,   170,   169,     0,   160,
      28,     0,   183,   184,     0,   167,     0,     0,   148,   145,
     147,    30,     0,     0,     0,    98,   100,     0,   172,   164,
     155,   113,   106,   110,   115,   111,     0,     0,     0,   121,
     120,   117,   137,   128,   135,   138,    32,     0,     0,     0,
     173,    34,     0,     0,    86,    89,     0,   131,     0,     0,
     161,    29,     0,     0,   144,   150,     0,    96,     0,     0,
     125,     0,     0,     0,     0,   175,     0,   174,     0,    63,
      99,   130,   139,   140,   168,   166,     0,     0,   101,   122,
       0,   136,   176,     0,     0,     0,   177,    97,   123,   179,
     178,     0,   180
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -155,  -155,  -155,  -155,  -155,  -155,    -2,   238,   177,   207,
     211,   187,   203,  -155,   -12,   -10,  -155,  -155,  -155,   229,
       0,   -45,    76,  -155,  -154,  -155,  -155,  -155,     7,   -22,
       8,  -155,  -155,  -155,  -155,  -155,   -98,    68,  -155,  -155,
    -155,  -155,   122,   -89,  -155,   -51,    40,  -155,   137,  -151,
     -23,  -155,   222,   224
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
     254,    47,    48,    49,   255,    50,    51,    52,    53,   252,
      90,    91,    54,    97,    55,   185,   186,   103,    56,    57,
      58,    59,   168,   169,    60,    92,    93,    62,   239,    84,
     112,    63,    64,    85
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      46,    75,    76,   139,   191,    78,    79,   245,   115,   136,
      74,   149,    88,    95,   102,   231,  -183,    77,   256,   170,
      80,   196,   189,   236,   259,   201,  -181,   237,  -182,   184,
      83,    89,    89,    96,   146,   177,   148,   190,   137,   156,
      61,   224,   120,   179,    99,   130,   131,   104,   138,   238,
     297,    19,  -184,   159,   160,   161,   162,   163,   167,   170,
     116,    88,   100,   170,   121,   150,   241,    81,   180,   182,
     137,     7,   284,   285,   176,   188,   132,    89,   232,   137,
      89,   173,    70,     1,     2,    71,    72,     5,     6,     7,
      82,   193,   135,    89,   279,   299,   155,   133,   167,   198,
     151,   111,   167,   304,   170,     8,     9,    10,    11,    12,
     280,   133,    81,   142,   202,   250,     7,    89,  -184,   305,
     219,    89,   221,   222,   109,   110,   126,   127,   225,   310,
     111,    22,    86,   320,   229,   242,   325,   128,   129,    23,
      73,   314,   315,   167,   318,    25,    94,    26,   157,   133,
     113,   114,   326,   248,   249,   158,   111,   143,   205,   144,
     247,   257,    89,   145,   243,    83,  -182,   143,   262,   144,
     262,   187,  -182,    89,   266,   302,   262,   272,   273,    81,
     275,   171,    83,     7,   172,   278,   260,    89,   263,    89,
     265,   174,   234,   235,   267,    89,   271,   143,   268,   144,
     143,   170,   144,   147,     1,     2,     3,     4,     5,     6,
       7,   152,   153,   236,   230,   154,   233,   237,   287,   175,
      65,    66,    67,    68,    69,   178,     8,     9,    10,    11,
      12,   194,   152,   153,   184,   294,   154,   133,   134,    13,
     167,   183,    14,    15,    16,    17,    18,    19,    20,    21,
     181,   143,    22,   144,   199,   300,   140,   141,   301,    89,
      23,    24,   -64,   -64,   220,   303,    25,   308,    26,  -182,
     309,   226,   139,   227,   311,   154,   312,   313,   258,     1,
       2,    98,    72,     5,     6,     7,   269,   319,   105,   106,
     107,   108,   323,   274,   324,   288,   316,   207,   208,    83,
      83,     8,     9,    10,    11,    12,   251,   143,   328,   144,
      99,   276,   143,   331,   144,   215,   216,   138,   122,   123,
     124,   125,   281,   143,   282,   144,   292,    22,   100,   209,
     210,   211,   212,   217,   218,    23,    73,   213,   214,   291,
     143,    25,   144,    26,   101,     1,     2,    71,    72,     5,
       6,     7,   117,   118,   119,   203,   204,   206,   283,   286,
     289,   293,   296,   298,   321,   322,   223,     8,     9,    10,
      11,    12,   317,   329,   295,   290,    99,   330,   332,   244,
       0,     0,   246,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    22,   100,     0,     0,     0,     0,     0,
       0,    23,    73,     0,     0,     0,     0,    25,     0,    26,
     228,     1,     2,     3,    72,     5,     6,     7,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     8,     9,    10,    11,    12,     0,     0,
       0,     0,     0,     0,     0,   327,    13,     0,     0,    14,
      15,    16,    17,    18,     0,    20,    21,     0,     0,    22,
       1,     2,     3,    72,     5,     6,     7,    23,    73,     0,
       0,     0,     0,    25,     0,    26,     0,     0,     0,     0,
       0,     0,     8,     9,    10,    11,    12,     1,     2,     3,
      72,     5,     6,     7,     0,    13,     0,     0,    14,    15,
      16,    17,    18,     0,    20,    21,     0,     0,    22,     8,
       9,    10,    11,    12,     0,     0,    23,    73,     0,   253,
       0,     0,    25,     0,    26,     0,     0,    16,    17,    18,
       0,    20,    21,     0,     0,    22,     1,     2,     3,    72,
       5,     6,     7,    23,    73,     0,     0,     0,     0,    25,
       0,    26,     0,     0,     0,     0,     0,     0,     8,     9,
      10,    11,    12,     1,     2,    71,     4,     5,     6,     7,
       0,     0,     0,     0,     0,     0,    16,    17,    18,     0,
      20,    21,     0,     0,    22,     8,     9,    10,    11,    12,
       0,     0,    23,    73,     0,     0,     0,     0,    25,     0,
      26,     0,     0,     0,     0,     0,    19,     0,     0,     0,
       0,    22,    86,     0,     0,     0,     0,     0,     0,    23,
      24,    87,     0,     0,     0,    25,     0,    26,     1,     2,
     164,    72,     5,     6,     7,     0,     0,     0,     0,     1,
       2,   164,    72,     5,     6,     7,     0,     0,     0,     0,
       8,     9,    10,    11,    12,     0,     0,     0,     0,   165,
       0,     8,     9,    10,    11,    12,     0,     0,     0,     0,
     165,     0,     0,     0,     0,     0,    22,   166,     0,     0,
       0,     0,     0,     0,    23,    73,   195,    22,   166,     0,
      25,     0,    26,     0,     0,    23,    73,   200,     0,     0,
       0,    25,     0,    26,     1,     2,   164,    72,     5,     6,
       7,     0,     0,     0,     0,     1,     2,   164,    72,     5,
       6,     7,     0,     0,     0,     0,     8,     9,    10,    11,
      12,     0,     0,     0,     0,   165,     0,     8,     9,    10,
      11,    12,     0,     0,     0,     0,   165,     0,     0,     0,
       0,     0,    22,   166,     0,     0,     0,     0,     0,     0,
      23,    73,   240,    22,   166,     0,    25,     0,    26,     0,
       0,    23,    73,     0,     0,     0,     0,    25,     0,    26,
       1,     2,    71,    72,     5,     6,     7,     0,     0,     0,
       0,     1,     2,    71,    72,     5,     6,     7,     0,     0,
       0,     0,     8,     9,    10,    11,    12,     0,     0,     0,
       0,     0,     0,     8,     9,    10,    11,    12,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    22,    86,
       0,     0,     0,     0,     0,     0,    23,    73,    87,    22,
      86,     0,    25,     0,    26,     0,     0,    23,    73,   261,
       0,     0,     0,    25,     0,    26,     1,     2,    71,    72,
       5,     6,     7,     0,     0,     0,     0,     1,     2,    71,
      72,     5,     6,     7,     0,     0,     0,     0,     8,     9,
      10,    11,    12,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    22,    86,     0,     0,     0,     0,
       0,     0,    23,    73,   264,    22,    86,     0,    25,     0,
      26,     0,     0,    23,    73,     0,     0,     0,     0,    25,
     270,    26,     1,     2,    71,    72,     5,     6,     7,     0,
       0,     0,     1,     2,    71,    72,     5,     6,     7,     0,
       0,     0,     0,     0,     8,     9,    10,    11,    12,     0,
       0,     0,     0,     0,     8,     9,    10,    11,    12,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   306,
      22,     0,     0,     0,     0,     0,     0,     0,    23,    73,
      22,     0,     0,     0,    25,   307,    26,     0,    23,    73,
       0,     0,   192,     0,    25,     0,    26,     1,     2,    71,
      72,     5,     6,     7,     0,     0,     0,     1,     2,    71,
       4,     5,     6,     7,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   197,    22,     0,     0,     0,     0,
      19,     0,     0,    23,    73,    22,     0,     0,     0,    25,
       0,    26,     0,    23,    24,     0,     0,     0,     0,    25,
       0,    26,     1,     2,    71,    72,     5,     6,     7,     0,
       0,     0,     1,     2,    71,    72,     5,     6,     7,     0,
       0,     0,     0,     0,     8,     9,    10,    11,    12,     0,
       0,     0,     0,     0,     8,     9,    10,    11,    12,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   277,
      22,     0,     0,     0,     0,     0,     0,     0,    23,    73,
      22,     0,     0,     0,    25,     0,    26,     0,    23,    73,
       0,     0,     0,     0,    25,     0,    26
};

static const yytype_int16 yycheck[] =
{
       0,    13,    14,    48,   102,    15,    18,   158,    31,    35,
      12,     6,    24,    25,    26,     5,    59,     5,   172,    70,
      22,   110,    63,     3,   175,   114,    69,     7,    69,    42,
      23,    24,    25,    25,    56,    86,    58,    50,    64,    62,
       0,    35,    29,    50,    34,    18,    19,     0,    48,    29,
      35,    46,    59,    65,    66,    67,    68,    69,    70,   110,
      56,    73,    52,   114,    51,    60,   155,     5,    90,    91,
      64,     9,   226,   227,    86,    97,    49,    70,    68,    64,
      73,    46,    60,     3,     4,     5,     6,     7,     8,     9,
      28,   103,    62,    86,    50,    46,    60,    62,   110,   111,
      60,    65,   114,    50,   155,    25,    26,    27,    28,    29,
      66,    62,     5,    40,   116,   166,     9,   110,    59,    66,
     132,   114,   134,   135,    59,    60,    10,    11,   140,   283,
      65,    51,    52,    46,   146,    28,    50,    16,    17,    59,
      60,   292,   293,   155,   298,    65,    66,    67,    69,    62,
      59,    60,    66,   165,   166,    59,    65,    62,   118,    64,
      63,   173,   155,    68,   157,   158,    69,    62,   180,    64,
     182,    66,    69,   166,   184,   273,   188,   189,   190,     5,
     192,    50,   175,     9,    50,   197,   179,   180,   180,   182,
     182,    59,   152,   153,    38,   188,   188,    62,    42,    64,
      62,   252,    64,    68,     3,     4,     5,     6,     7,     8,
       9,    52,    53,     3,   146,    56,   148,     7,   230,    59,
      20,    21,    22,    23,    24,    61,    25,    26,    27,    28,
      29,     5,    52,    53,    42,   247,    56,    62,    63,    38,
     252,    61,    41,    42,    43,    44,    45,    46,    47,    48,
      61,    62,    51,    64,     5,   267,    39,    40,   268,   252,
      59,    60,    62,    63,     5,   277,    65,   279,    67,    69,
     282,    50,   317,    50,   286,    56,   288,   289,     5,     3,
       4,     5,     6,     7,     8,     9,    66,   299,    25,    26,
      27,    28,   304,    68,   306,    63,   296,   120,   121,   292,
     293,    25,    26,    27,    28,    29,    61,    62,   320,    64,
      34,    61,    62,   325,    64,   128,   129,   317,    12,    13,
      14,    15,    61,    62,    50,    64,    59,    51,    52,   122,
     123,   124,   125,   130,   131,    59,    60,   126,   127,    61,
      62,    65,    64,    67,    68,     3,     4,     5,     6,     7,
       8,     9,    52,    53,    54,   117,   118,   119,    50,    50,
      63,    59,    36,    50,    68,    66,   137,    25,    26,    27,
      28,    29,   296,    66,   252,   238,    34,    66,    66,   157,
      -1,    -1,   158,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    51,    52,    -1,    -1,    -1,    -1,    -1,
      -1,    59,    60,    -1,    -1,    -1,    -1,    65,    -1,    67,
      68,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    41,
      42,    43,    44,    45,    -1,    47,    48,    -1,    -1,    51,
       3,     4,     5,     6,     7,     8,     9,    59,    60,    -1,
      -1,    -1,    -1,    65,    -1,    67,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    27,    28,    29,     3,     4,     5,
       6,     7,     8,     9,    -1,    38,    -1,    -1,    41,    42,
      43,    44,    45,    -1,    47,    48,    -1,    -1,    51,    25,
      26,    27,    28,    29,    -1,    -1,    59,    60,    -1,    35,
      -1,    -1,    65,    -1,    67,    -1,    -1,    43,    44,    45,
      -1,    47,    48,    -1,    -1,    51,     3,     4,     5,     6,
       7,     8,     9,    59,    60,    -1,    -1,    -1,    -1,    65,
      -1,    67,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,
      27,    28,    29,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    43,    44,    45,    -1,
      47,    48,    -1,    -1,    51,    25,    26,    27,    28,    29,
      -1,    -1,    59,    60,    -1,    -1,    -1,    -1,    65,    -1,
      67,    -1,    -1,    -1,    -1,    -1,    46,    -1,    -1,    -1,
      -1,    51,    52,    -1,    -1,    -1,    -1,    -1,    -1,    59,
      60,    61,    -1,    -1,    -1,    65,    -1,    67,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      25,    26,    27,    28,    29,    -1,    -1,    -1,    -1,    34,
      -1,    25,    26,    27,    28,    29,    -1,    -1,    -1,    -1,
      34,    -1,    -1,    -1,    -1,    -1,    51,    52,    -1,    -1,
      -1,    -1,    -1,    -1,    59,    60,    61,    51,    52,    -1,
      65,    -1,    67,    -1,    -1,    59,    60,    61,    -1,    -1,
      -1,    65,    -1,    67,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    -1,    25,    26,    27,    28,
      29,    -1,    -1,    -1,    -1,    34,    -1,    25,    26,    27,
      28,    29,    -1,    -1,    -1,    -1,    34,    -1,    -1,    -1,
      -1,    -1,    51,    52,    -1,    -1,    -1,    -1,    -1,    -1,
      59,    60,    61,    51,    52,    -1,    65,    -1,    67,    -1,
      -1,    59,    60,    -1,    -1,    -1,    -1,    65,    -1,    67,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      -1,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    -1,    25,    26,    27,    28,    29,    -1,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    51,    52,
      -1,    -1,    -1,    -1,    -1,    -1,    59,    60,    61,    51,
      52,    -1,    65,    -1,    67,    -1,    -1,    59,    60,    61,
      -1,    -1,    -1,    65,    -1,    67,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    -1,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,    25,    26,
      27,    28,    29,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    29,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    51,    52,    -1,    -1,    -1,    -1,
      -1,    -1,    59,    60,    61,    51,    52,    -1,    65,    -1,
      67,    -1,    -1,    59,    60,    -1,    -1,    -1,    -1,    65,
      66,    67,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    50,
      51,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    59,    60,
      51,    -1,    -1,    -1,    65,    66,    67,    -1,    59,    60,
      -1,    -1,    63,    -1,    65,    -1,    67,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    29,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    29,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    50,    51,    -1,    -1,    -1,    -1,
      46,    -1,    -1,    59,    60,    51,    -1,    -1,    -1,    65,
      -1,    67,    -1,    59,    60,    -1,    -1,    -1,    -1,    65,
      -1,    67,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    50,
      51,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    59,    60,
      51,    -1,    -1,    -1,    65,    -1,    67,    -1,    59,    60,
      -1,    -1,    -1,    -1,    65,    -1,    67
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    25,    26,
      27,    28,    29,    38,    41,    42,    43,    44,    45,    46,
      47,    48,    51,    59,    60,    65,    67,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      95,    96,    97,    98,   102,   104,   108,   109,   110,   111,
     114,   116,   117,   121,   122,    20,    21,    22,    23,    24,
      60,     5,     6,    60,    76,    84,    84,     5,    85,    84,
      76,     5,    28,    98,   119,   123,    52,    61,    84,    98,
     100,   101,   115,   116,    66,    84,   100,   103,     5,    34,
      52,    68,    84,   107,     0,    25,    26,    27,    28,    59,
      60,    65,   120,    59,    60,   120,    56,    52,    53,    54,
      29,    51,    12,    13,    14,    15,    10,    11,    16,    17,
      18,    19,    49,    62,    63,    62,    35,    64,    90,    91,
      39,    40,    40,    62,    64,    68,    99,    68,    99,     6,
      60,   116,    52,    53,    56,    60,   120,    69,    59,    84,
      84,    84,    84,    84,     5,    34,    52,    84,   112,   113,
     115,    50,    50,    46,    59,    59,    84,   115,    61,    50,
      99,    61,    99,    61,    42,   105,   106,    66,    99,    63,
      50,   106,    63,    84,     5,    61,   113,    50,    84,     5,
      61,   113,    76,    77,    77,   116,    77,    78,    78,    79,
      79,    79,    79,    80,    80,    81,    81,    82,    82,    84,
       5,    84,    84,    89,    35,    84,    50,    50,    68,    84,
     107,     5,    68,   107,   116,   116,     3,     7,    29,   118,
      61,   113,    28,    98,   122,   119,   123,    63,    84,    84,
     115,    61,    99,    35,    90,    94,    94,    84,     5,   119,
      98,    61,    84,   100,    61,   100,    85,    38,    42,    66,
      66,   100,    84,    84,    68,    84,    61,    50,    84,    50,
      66,    61,    50,    50,    94,    94,    50,    84,    63,    63,
     118,    61,    59,    59,    84,   112,    36,    35,    50,    46,
      84,    85,   106,    84,    50,    66,    50,    66,    84,    84,
      94,    84,    84,    84,   119,   119,    90,    92,    94,    84,
      46,    68,    66,    84,    84,    50,    66,    37,    84,    66,
      66,    84,    66
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    70,    71,    71,    72,    72,    72,    72,    72,    72,
      72,    72,    73,    73,    73,    73,    73,    73,    73,    73,
      74,    74,    74,    74,    74,    74,    74,    74,    75,    75,
      75,    75,    75,    75,    75,    76,    76,    76,    77,    77,
      78,    78,    78,    78,    78,    79,    79,    79,    80,    80,
      80,    80,    80,    81,    81,    81,    82,    82,    82,    83,
      83,    83,    84,    84,    85,    85,    86,    86,    87,    87,
      87,    87,    87,    88,    88,    89,    89,    89,    89,    89,
      89,    89,    90,    90,    90,    91,    91,    91,    91,    91,
      92,    92,    92,    92,    93,    93,    94,    94,    95,    95,
      96,    97,    98,    98,    99,    99,   100,   100,   100,   100,
     101,   101,   102,   102,   102,   102,   103,   103,   104,   104,
     104,   104,   105,   105,   106,   106,   107,   107,   108,   108,
     108,   108,   109,   109,   109,   109,   109,   110,   110,   110,
     110,   111,   111,   112,   112,   112,   112,   112,   112,   113,
     113,   114,   114,   114,   114,   115,   116,   116,   116,   116,
     116,   116,   116,   117,   117,   117,   117,   117,   117,   118,
     118,   119,   119,   120,   120,   120,   120,   120,   120,   120,
     120,   121,   121,   122,   122,   123
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     2,     1,     1,     1,     1,
       2,     3,     1,     1,     1,     1,     2,     2,     2,     2,
       3,     1,     1,     1,     1,     2,     2,     3,     3,     4,
       4,     3,     4,     3,     4,     1,     2,     2,     1,     3,
       1,     3,     3,     3,     3,     1,     3,     3,     1,     3,
       3,     3,     3,     1,     3,     3,     1,     3,     3,     1,
       3,     3,     1,     5,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     1,     1,     1,     1,     2,     1,     1,
       1,     1,     1,     3,     2,     1,     4,     1,     1,     4,
       2,     1,     3,     2,     1,     1,     2,     4,     4,     5,
       4,     6,     1,     1,     1,     1,     1,     1,     2,     2,
       3,     3,     2,     4,     3,     4,     1,     3,     2,     3,
       4,     4,     4,     5,     1,     3,     1,     1,     4,     3,
       5,     4,     2,     2,     3,     4,     6,     4,     4,     5,
       5,     2,     3,     1,     3,     2,     1,     2,     2,     1,
       3,     1,     1,     1,     1,     3,     1,     1,     3,     3,
       3,     4,     3,     1,     4,     2,     5,     3,     5,     1,
       1,     1,     3,     3,     4,     4,     5,     5,     6,     6,
       7,     1,     1,     1,     1,     1
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
          case 71: /* input  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1407 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 72: /* primary_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1413 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 73: /* string  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1419 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 74: /* indexable  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1425 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 75: /* callable  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1431 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 76: /* unary_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1437 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 77: /* power_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1443 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 78: /* multiply_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1449 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 79: /* additive_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1455 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 80: /* relational_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1461 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 81: /* equality_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1467 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 82: /* logical_and_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1473 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 83: /* logical_or_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1479 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 84: /* exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1485 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 86: /* assignment_exp1  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1491 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 87: /* assignment_exp2  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1497 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 88: /* assignment_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1503 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 89: /* small_stmt  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1509 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 90: /* simple_stmt  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1515 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 91: /* compound_stmt  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1521 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 92: /* stmt  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1527 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 93: /* statement  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1533 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 94: /* suite  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1539 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 95: /* if_stmt  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1545 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 96: /* while_stmt  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1551 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 97: /* for_stmt  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1557 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 100: /* item  */
#line 68 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).named_argument).second; }
#line 1563 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 101: /* items2  */
#line 67 "ExpressionParser.y" /* yacc.c:1257  */
      { for(auto &v : ((*yyvaluep).named_arguments)) {delete v.second;} }
#line 1569 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 102: /* tuple  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1575 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 103: /* items  */
#line 67 "ExpressionParser.y" /* yacc.c:1257  */
      { for(auto &v : ((*yyvaluep).named_arguments)) {delete v.second;} }
#line 1581 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 104: /* list  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1587 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 105: /* comprehension0  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1593 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 106: /* comprehension  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1599 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 108: /* dict1  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1605 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 109: /* dict  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1611 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 110: /* idict1  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1617 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 111: /* idict  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1623 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 112: /* arg  */
#line 68 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).named_argument).second; }
#line 1629 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 113: /* args  */
#line 67 "ExpressionParser.y" /* yacc.c:1257  */
      { for(auto &v : ((*yyvaluep).named_arguments)) {delete v.second;} }
#line 1635 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 114: /* num  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1641 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 115: /* range  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1647 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 116: /* unit_exp  */
#line 65 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1653 "ExpressionParser.tab.c" /* yacc.c:1257  */
        break;

    case 120: /* indexer  */
#line 69 "ExpressionParser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).component).e1; delete ((*yyvaluep).component).e2; }
#line 1659 "ExpressionParser.tab.c" /* yacc.c:1257  */
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
#line 1923 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 76 "ExpressionParser.y" /* yacc.c:1646  */
    { ScanResult = (yyvsp[0].expr); unitExpression = true; (yyval.expr)=0;                                        }
#line 1929 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 80 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr);                                                                        }
#line 1935 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 81 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-1].expr), OperatorExpression::UNIT, (yyvsp[0].expr));  }
#line 1941 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 82 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1947 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 83 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new VariableExpression(DocumentObject, (yyvsp[0].path)); }
#line 1953 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 84 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1959 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 85 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1965 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 86 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-1].expr)->addComponent((yyvsp[0].component)); (yyval.expr) = (yyvsp[-1].expr); }
#line 1971 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 87 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].expr)->addComponent((yyvsp[0].string)); (yyval.expr) = (yyvsp[-2].expr); }
#line 1977 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 91 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new StringExpression(DocumentObject, (yyvsp[0].string)); }
#line 1983 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 92 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new StringExpression(DocumentObject, (yyvsp[0].string), StringExpression::StringRaw); }
#line 1989 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 93 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new StringExpression(DocumentObject, (yyvsp[0].string), StringExpression::StringPython); }
#line 1995 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 94 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new StringExpression(DocumentObject, (yyvsp[0].string), StringExpression::StringPythonRaw); }
#line 2001 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 95 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); if(!static_cast<StringExpression*>((yyvsp[-1].expr))->append((yyvsp[0].string))) YYABORT; }
#line 2007 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 96 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); if(!static_cast<StringExpression*>((yyvsp[-1].expr))->append((yyvsp[0].string),StringExpression::StringRaw)) YYABORT; }
#line 2013 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 97 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); if(!static_cast<StringExpression*>((yyvsp[-1].expr))->append((yyvsp[0].string),StringExpression::StringPython)) YYABORT; }
#line 2019 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 98 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); if(!static_cast<StringExpression*>((yyvsp[-1].expr))->append((yyvsp[0].string),StringExpression::StringPythonRaw)) YYABORT; }
#line 2025 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 101 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2031 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 102 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2037 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 103 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2043 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 104 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2049 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 105 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2055 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 106 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new VariableExpression(DocumentObject,(yyvsp[-1].path)); (yyval.expr)->addComponent((yyvsp[0].component)); }
#line 2061 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 107 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-1].expr)->addComponent((yyvsp[0].component)); (yyval.expr) = (yyvsp[-1].expr); }
#line 2067 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 108 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].expr)->addComponent((yyvsp[0].string)); (yyval.expr) = (yyvsp[-2].expr); }
#line 2073 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 111 "ExpressionParser.y" /* yacc.c:1646  */
    { 
                                                    (yyval.expr) = CallableExpression::create(DocumentObject,(yyvsp[-2].path));
                                                    if(!(yyval.expr)) { 
                                                        YYABORT;
                                                    } 
                                                }
#line 2084 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 117 "ExpressionParser.y" /* yacc.c:1646  */
    { 
                                                    (yyval.expr) = CallableExpression::create(DocumentObject, (yyvsp[-3].path), (yyvsp[-1].named_arguments)); 
                                                    if(!(yyval.expr)) { 
                                                        YYABORT;
                                                    } 
                                                }
#line 2095 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 123 "ExpressionParser.y" /* yacc.c:1646  */
    {   // This rule exists because of possible name clash of 
                                                    // function and unit, e.g. min
                                                    ObjectIdentifier name(DocumentObject);
                                                    name << ObjectIdentifier::SimpleComponent((yyvsp[-3].quantity).unitStr);
                                                    (yyval.expr) = CallableExpression::create(DocumentObject, name, (yyvsp[-1].named_arguments));
                                                    if(!(yyval.expr)) { 
                                                        YYABORT;
                                                    } 
                                                }
#line 2109 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 132 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-2].expr)); }
#line 2115 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 133 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-3].expr), (yyvsp[-1].named_arguments)); }
#line 2121 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 134 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-2].expr)); }
#line 2127 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 135 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new CallableExpression(DocumentObject, (yyvsp[-3].expr), (yyvsp[-1].named_arguments)); }
#line 2133 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 139 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2139 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 140 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))); }
#line 2145 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 141 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[0].expr), OperatorExpression::POS, new NumberExpression(DocumentObject, Quantity(1))); }
#line 2151 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 145 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2157 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 146 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, (yyvsp[0].expr));   }
#line 2163 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 149 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2169 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 150 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 2175 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 151 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 2181 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 152 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 2187 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 153 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MOD, (yyvsp[0].expr));   }
#line 2193 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 157 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2199 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 158 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::ADD, (yyvsp[0].expr));   }
#line 2205 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 159 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::SUB, (yyvsp[0].expr));   }
#line 2211 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 163 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2217 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 164 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LT, (yyvsp[0].expr));    }
#line 2223 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 165 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GT, (yyvsp[0].expr));    }
#line 2229 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 166 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::GTE, (yyvsp[0].expr));   }
#line 2235 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 167 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::LTE, (yyvsp[0].expr));   }
#line 2241 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 171 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2247 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 172 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::EQ, (yyvsp[0].expr));    }
#line 2253 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 173 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::NEQ, (yyvsp[0].expr));   }
#line 2259 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 177 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2265 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 178 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::AND_OP, (yyvsp[0].expr));   }
#line 2271 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 179 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::AND_OP2, (yyvsp[0].expr));   }
#line 2277 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 184 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2283 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 185 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::OR_OP, (yyvsp[0].expr));   }
#line 2289 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 186 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::OR_OP2, (yyvsp[0].expr));   }
#line 2295 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 190 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2301 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 191 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ConditionalExpression(DocumentObject, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr));                     }
#line 2307 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 195 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_list).push_back((yyvsp[0].string)); }
#line 2313 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 65:
#line 196 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].string_list).push_back((yyvsp[0].string)); (yyval.string_list).swap((yyvsp[-2].string_list)); }
#line 2319 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 200 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new AssignmentExpression(DocumentObject,(yyvsp[-2].string_list),(yyvsp[0].expr)); }
#line 2325 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 201 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<AssignmentExpression*>((yyvsp[-2].expr))->add((yyvsp[0].expr)); (yyval.expr) = (yyvsp[-2].expr); }
#line 2331 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 68:
#line 205 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new AssignmentExpression(DocumentObject,(yyvsp[-2].string),OperatorExpression::MUL,(yyvsp[0].expr)); }
#line 2337 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 206 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new AssignmentExpression(DocumentObject,(yyvsp[-2].string),OperatorExpression::DIV,(yyvsp[0].expr)); }
#line 2343 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 207 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new AssignmentExpression(DocumentObject,(yyvsp[-2].string),OperatorExpression::MOD,(yyvsp[0].expr)); }
#line 2349 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 208 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new AssignmentExpression(DocumentObject,(yyvsp[-2].string),OperatorExpression::ADD,(yyvsp[0].expr)); }
#line 2355 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 209 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new AssignmentExpression(DocumentObject,(yyvsp[-2].string),OperatorExpression::SUB,(yyvsp[0].expr)); }
#line 2361 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 213 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2367 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 214 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2373 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 218 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2379 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 76:
#line 219 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2385 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 77:
#line 220 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new JumpStatement(DocumentObject,JumpStatement::JUMP_RETURN,(yyvsp[0].expr)); }
#line 2391 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 221 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new JumpStatement(DocumentObject,JumpStatement::JUMP_BREAK); }
#line 2397 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 222 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new JumpStatement(DocumentObject,JumpStatement::JUMP_CONTINUE); }
#line 2403 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 80:
#line 223 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new PseudoStatement(DocumentObject,PseudoStatement::PY_BEGIN); }
#line 2409 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 224 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new PseudoStatement(DocumentObject,PseudoStatement::PY_END); }
#line 2415 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 228 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2421 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 229 "ExpressionParser.y" /* yacc.c:1646  */
    { 
                                                    SimpleStatement *stmt = dynamic_cast<SimpleStatement*>((yyvsp[-2].expr)); 
                                                    if(!stmt)
                                                        stmt = new SimpleStatement(DocumentObject, (yyvsp[-2].expr));
                                                    stmt->add((yyvsp[0].expr)); 
                                                    (yyval.expr) = stmt;
                                                 }
#line 2433 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 236 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2439 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 240 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2445 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 241 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<IfStatement*>((yyvsp[-3].expr))->addElse((yyvsp[0].expr)); (yyval.expr)=(yyvsp[-3].expr); }
#line 2451 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 87:
#line 242 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2457 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 88:
#line 243 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2463 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 244 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<ForStatement*>((yyvsp[-3].expr))->addElse((yyvsp[0].expr)); (yyval.expr)=(yyvsp[-3].expr); }
#line 2469 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 90:
#line 248 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new Statement(DocumentObject, (yyvsp[-1].expr)); }
#line 2475 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 91:
#line 249 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new Statement(DocumentObject, (yyvsp[0].expr)); }
#line 2481 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 92:
#line 250 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<Statement*>((yyvsp[-2].expr))->add((yyvsp[-1].expr)); (yyval.expr) = (yyvsp[-2].expr); }
#line 2487 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 93:
#line 251 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<Statement*>((yyvsp[-1].expr))->add((yyvsp[0].expr)); (yyval.expr) = (yyvsp[-1].expr); }
#line 2493 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 94:
#line 255 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2499 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 95:
#line 256 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2505 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 96:
#line 259 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2511 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 97:
#line 260 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2517 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 98:
#line 264 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new IfStatement(DocumentObject,(yyvsp[-2].expr),(yyvsp[0].expr)); }
#line 2523 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 99:
#line 265 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<IfStatement*>((yyval.expr))->addElseIf((yyvsp[-2].expr),(yyvsp[0].expr)); (yyval.expr)=(yyvsp[-4].expr); }
#line 2529 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 269 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new WhileStatement(DocumentObject,(yyvsp[-2].expr),(yyvsp[0].expr)); }
#line 2535 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 272 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ForStatement(DocumentObject,(yyvsp[-4].string_list),(yyvsp[-2].expr),(yyvsp[0].expr)); }
#line 2541 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 275 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string) = (yyvsp[0].string); }
#line 2547 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 103:
#line 276 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string) = (yyvsp[0].string); }
#line 2553 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 106:
#line 281 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first.clear(); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2559 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 107:
#line 282 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first.clear(); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2565 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 108:
#line 283 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = "*"; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2571 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 109:
#line 284 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = "*"; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2577 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 110:
#line 287 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_arguments).push_back((yyvsp[-2].named_argument)); (yyval.named_arguments).push_back((yyvsp[0].named_argument)); }
#line 2583 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 111:
#line 288 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].named_arguments).push_back((yyvsp[0].named_argument)); (yyval.named_arguments).swap((yyvsp[-2].named_arguments)); }
#line 2589 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 112:
#line 291 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject); }
#line 2595 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 113:
#line 292 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject, (yyvsp[-2].named_argument)); }
#line 2601 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 114:
#line 293 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject, (yyvsp[-1].named_arguments)); }
#line 2607 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 115:
#line 294 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new TupleExpression(DocumentObject, (yyvsp[-2].named_arguments)); }
#line 2613 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 116:
#line 297 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_arguments).push_back((yyvsp[0].named_argument)); }
#line 2619 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 117:
#line 298 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].named_arguments).push_back((yyvsp[0].named_argument)); (yyval.named_arguments).swap((yyvsp[-2].named_arguments)); }
#line 2625 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 118:
#line 301 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ListExpression(DocumentObject); }
#line 2631 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 119:
#line 302 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ListExpression(DocumentObject, (yyvsp[-1].named_arguments)); }
#line 2637 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 120:
#line 303 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ListExpression(DocumentObject, (yyvsp[-2].named_arguments)); }
#line 2643 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 121:
#line 304 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<ComprehensionExpression*>((yyvsp[-1].expr))->setExpr((yyvsp[-2].expr)); (yyval.expr) = (yyvsp[-1].expr); }
#line 2649 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 122:
#line 308 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ComprehensionExpression(DocumentObject,(yyvsp[-2].string_list),(yyvsp[0].expr)); }
#line 2655 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 123:
#line 309 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<ComprehensionExpression*>((yyvsp[-4].expr))->add((yyvsp[-2].string_list),(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-4].expr); }
#line 2661 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 124:
#line 313 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2667 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 125:
#line 314 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<ComprehensionExpression*>((yyvsp[-2].expr))->setCondition((yyvsp[0].expr)); (yyval.expr) = (yyvsp[-2].expr); }
#line 2673 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 128:
#line 319 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new DictExpression(DocumentObject, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2679 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 129:
#line 320 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new DictExpression(DocumentObject, 0, (yyvsp[0].expr)); }
#line 2685 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 130:
#line 321 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<DictExpression*>((yyvsp[-4].expr))->addItem((yyvsp[-2].expr),(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-4].expr); }
#line 2691 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 131:
#line 322 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<DictExpression*>((yyvsp[-3].expr))->addItem(0,(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-3].expr); }
#line 2697 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 132:
#line 325 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new DictExpression(DocumentObject); }
#line 2703 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 133:
#line 326 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2709 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 134:
#line 327 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-2].expr); }
#line 2715 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 135:
#line 328 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<ComprehensionExpression*>((yyvsp[-1].expr))->setExpr((yyvsp[-2].expr),0,false); (yyval.expr)=(yyvsp[-1].expr); }
#line 2721 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 136:
#line 329 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<ComprehensionExpression*>((yyvsp[-1].expr))->setExpr((yyvsp[-4].expr),(yyvsp[-2].expr),false); (yyval.expr)=(yyvsp[-1].expr); }
#line 2727 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 137:
#line 332 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new IDictExpression(DocumentObject, (yyvsp[-2].string), (yyvsp[0].expr)); }
#line 2733 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 138:
#line 333 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new IDictExpression(DocumentObject, "**", (yyvsp[0].expr)); }
#line 2739 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 139:
#line 334 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<IDictExpression*>((yyvsp[-4].expr))->addItem((yyvsp[-2].string),(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-4].expr); }
#line 2745 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 140:
#line 335 "ExpressionParser.y" /* yacc.c:1646  */
    { static_cast<IDictExpression*>((yyvsp[-4].expr))->addItem("**",(yyvsp[0].expr)); (yyval.expr) = (yyvsp[-4].expr); }
#line 2751 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 141:
#line 338 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2757 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 142:
#line 339 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-2].expr); }
#line 2763 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 143:
#line 342 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first.clear(); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2769 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 144:
#line 343 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = (yyvsp[-2].string); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2775 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 145:
#line 344 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = "*"; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2781 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 146:
#line 345 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first.clear(); (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2787 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 147:
#line 346 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = '*'; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2793 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 148:
#line 347 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_argument).first = "**"; (yyval.named_argument).second = (yyvsp[0].expr); }
#line 2799 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 149:
#line 350 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.named_arguments).push_back((yyvsp[0].named_argument)); }
#line 2805 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 150:
#line 351 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].named_arguments).push_back((yyvsp[0].named_argument)); (yyval.named_arguments).swap((yyvsp[-2].named_arguments)); }
#line 2811 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 151:
#line 354 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 2817 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 152:
#line 355 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((yyvsp[0].fvalue)));                        }
#line 2823 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 153:
#line 356 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue)));                }
#line 2829 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 154:
#line 357 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new ConstantExpression(DocumentObject, (yyvsp[0].constant).name, Quantity((yyvsp[0].constant).fvalue));      }
#line 2835 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 155:
#line 360 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new RangeExpression(DocumentObject, (yyvsp[-2].string), (yyvsp[0].string));                               }
#line 2841 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 156:
#line 363 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new UnitExpression(DocumentObject, (yyvsp[0].quantity).scaler, (yyvsp[0].quantity).unitStr );                }
#line 2847 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 157:
#line 364 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new UnitExpression(DocumentObject, Quantity::Inch, (yyvsp[0].string));                }
#line 2853 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 158:
#line 365 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::DIV, (yyvsp[0].expr));   }
#line 2859 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 159:
#line 366 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::MUL, (yyvsp[0].expr));   }
#line 2865 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 160:
#line 367 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-2].expr), OperatorExpression::POW, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))));   }
#line 2871 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 161:
#line 368 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = new OperatorExpression(DocumentObject, (yyvsp[-3].expr), OperatorExpression::POW, new OperatorExpression(DocumentObject, new NumberExpression(DocumentObject, Quantity((double)(yyvsp[0].ivalue))), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))));   }
#line 2877 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 162:
#line 369 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr);                                                                        }
#line 2883 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 163:
#line 372 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of the current object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject) << ObjectIdentifier::SimpleComponent((yyvsp[0].string));
                                                }
#line 2891 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 164:
#line 375 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of a sub-object of the current object*/
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  if(_Reader) {
                                                    (yyvsp[-2].string_or_identifier) = ObjectIdentifier::String(
                                                        PropertyLinkBase::importSubName(*_Reader,(yyvsp[-2].string_or_identifier).getString().c_str()),true);
                                                  }
                                                  (yyval.path).setDocumentObjectName(DocumentObject,false,(yyvsp[-2].string_or_identifier));
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 2905 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 165:
#line 384 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of the current document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentObjectName(DocumentObject);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                }
#line 2915 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 166:
#line 389 "ExpressionParser.y" /* yacc.c:1646  */
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
#line 2932 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 167:
#line 401 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property of a given document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  if(_Reader && !(yyvsp[-2].string_or_identifier).isRealString())
                                                      (yyvsp[-2].string_or_identifier) = ObjectIdentifier::String(_Reader->getName((yyvsp[-2].string_or_identifier).getString().c_str()));
                                                  (yyval.path) << ObjectIdentifier::SimpleComponent((yyvsp[-2].string_or_identifier));
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                  (yyval.path).resolveAmbiguity();
                                                }
#line 2945 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 168:
#line 409 "ExpressionParser.y" /* yacc.c:1646  */
    { /* Path to property from an external document, within a named document object */
                                                  (yyval.path) = ObjectIdentifier(DocumentObject);
                                                  (yyval.path).setDocumentName((yyvsp[-4].string_or_identifier), true);
                                                  (yyval.path).setDocumentObjectName((yyvsp[-2].string_or_identifier), true);
                                                  (yyval.path).addComponents((yyvsp[0].components));
                                                  (yyval.path).resolveAmbiguity();
                                                }
#line 2957 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 169:
#line 418 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.ivalue) = (yyvsp[0].ivalue); }
#line 2963 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 170:
#line 419 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.ivalue) = (yyvsp[0].fvalue); }
#line 2969 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 171:
#line 422 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.components).push_back(ObjectIdentifier::SimpleComponent((yyvsp[0].string)));}
#line 2975 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 172:
#line 423 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyvsp[-2].components).push_back(ObjectIdentifier::SimpleComponent((yyvsp[0].string))); (yyval.components).swap((yyvsp[-2].components)); }
#line 2981 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 173:
#line 426 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = Expression::Component((yyvsp[-1].expr));   }
#line 2987 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 174:
#line 427 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = Expression::Component((yyvsp[-2].expr),0,0,true); }
#line 2993 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 175:
#line 428 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = Expression::Component(0,(yyvsp[-1].expr)); }
#line 2999 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 176:
#line 429 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = Expression::Component(0,0,(yyvsp[-1].expr)); }
#line 3005 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 177:
#line 430 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = Expression::Component((yyvsp[-3].expr),(yyvsp[-1].expr));}
#line 3011 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 178:
#line 431 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = Expression::Component((yyvsp[-4].expr),0,(yyvsp[-1].expr)); }
#line 3017 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 179:
#line 432 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = Expression::Component(0,(yyvsp[-3].expr),(yyvsp[-1].expr)); }
#line 3023 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 180:
#line 433 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.component) = Expression::Component((yyvsp[-5].expr),(yyvsp[-3].expr),(yyvsp[-1].expr));}
#line 3029 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 181:
#line 436 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 3035 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 182:
#line 437 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string));       }
#line 3041 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 183:
#line 440 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 3047 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 184:
#line 441 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string));       }
#line 3053 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;

  case 185:
#line 444 "ExpressionParser.y" /* yacc.c:1646  */
    { (yyval.string_or_identifier) = ObjectIdentifier::String((yyvsp[0].string), true); }
#line 3059 "ExpressionParser.tab.c" /* yacc.c:1646  */
    break;


#line 3063 "ExpressionParser.tab.c" /* yacc.c:1646  */
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
#line 447 "ExpressionParser.y" /* yacc.c:1906  */

