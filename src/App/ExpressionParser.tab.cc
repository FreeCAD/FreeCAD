// A Bison parser, made by GNU Bison 3.0.4.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.


// First part of user declarations.
#line 15 "ExpressionParser.y" // lalr1.cc:414



#line 40 "ExpressionParser.tab.cc" // lalr1.cc:414

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif


# include <cassert>
# include <cstdlib> // std::abort
# include <iostream>
# include <stdexcept>
# include <string>
# include <vector>
#include <typeinfo>
#ifndef YYASSERT
# include <cassert>
# define YYASSERT assert
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

/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

#line 23 "ExpressionParser.y" // lalr1.cc:418
namespace App { namespace ExpressionParser {
#line 124 "ExpressionParser.tab.cc" // lalr1.cc:418

  // reserve, can be overriden for other containers
  template <class S> void stack_prepare (S &) { }
  template <class T> void stack_prepare (std::vector <T> &s) { s.reserve (200); }

  template <class T, class S = ExpressionParserStack >
  class stack
  {
  public:
    // Hide our reversed order.
    typedef typename S::reverse_iterator iterator;
    typedef typename S::const_reverse_iterator const_iterator;

    stack ()
      : seq_ ()
    {
      stack_prepare(seq_);
    }

    stack (unsigned int n)
      : seq_ (n)
    {}

    inline
    T&
    operator[] (unsigned int i)
    {
      return seq_[seq_.size () - 1 - i];
    }

    inline
    const T&
    operator[] (unsigned int i) const
    {
      return seq_[seq_.size () - 1 - i];
    }

    /// Steal the contents of \a t.
    ///
    /// Close to move-semantics.
    inline
    void
    push (T& t)
    {
      seq_.emplace_back ();
      operator[](0).move (t);
    }

    inline
    void
    pop (unsigned int n = 1)
    {
      for (; n; --n)
        seq_.pop_back ();
    }

    void
    clear ()
    {
      seq_.clear ();
    }

    inline
    typename S::size_type
    size () const
    {
      return seq_.size ();
    }

    inline
    const_iterator
    begin () const
    {
      return seq_.rbegin ();
    }

    inline
    const_iterator
    end () const
    {
      return seq_.rend ();
    }

  private:
    stack (const stack&);
    stack& operator= (const stack&);
    /// The wrapped container.
    S seq_;
  };

  /// Present a slice of the top of a stack.
  template <class T, class S = stack<T> >
  class slice
  {
  public:
    slice (const S& stack, unsigned int range)
      : stack_ (stack)
      , range_ (range)
    {}

    inline
    const T&
    operator [] (unsigned int i) const
    {
      return stack_[range_ - i];
    }

  private:
    const S& stack_;
    unsigned int range_;
  };



  /// A char[S] buffer to store and retrieve objects.
  ///
  /// Sort of a variant, but does not keep track of the nature
  /// of the stored data, since that knowledge is available
  /// via the current state.
  template <size_t S>
  struct variant
  {
    /// Type of *this.
    typedef variant<S> self_type;

    /// Empty construction.
    variant ()
      : yytypeid_ (YY_NULLPTR)
    {}

    /// Construct and fill.
    template <typename T>
    variant (T&& t)
      : yytypeid_ (&typeid (T))
    {
      static_assert (sizeof (T) <= S, "variant size too small");
      new (yyas_<T> ()) T (std::move (t));
    }

    /// Destruction, allowed only if empty.
    ~variant ()
    {
      YYASSERT (!yytypeid_);
    }

    /// Instantiate an empty \a T in here.
    template <typename T>
    T&
    build ()
    {
      YYASSERT (!yytypeid_);
      static_assert (sizeof (T) <= S, "variant size too small");
      yytypeid_ = & typeid (T);
      return *new (yyas_<T> ()) T;
    }

    /// Instantiate a \a T in here from \a t.
    template <typename T>
    T&
    build (T &&t)
    {
      YYASSERT (!yytypeid_);
      static_assert (sizeof (T) <= S, "variant size too small");
      yytypeid_ = & typeid (T);
      return *new (yyas_<T> ()) T (std::move (t));
    }

    /// Accessor to a built \a T.
    template <typename T>
    T&
    as ()
    {
      YYASSERT (*yytypeid_ == typeid (T));
      static_assert (sizeof (T) <= S, "variant size too small");
      return *yyas_<T> ();
    }

    /// Const accessor to a built \a T (for %printer).
    template <typename T>
    const T&
    as () const
    {
      YYASSERT (*yytypeid_ == typeid (T));
      static_assert (sizeof (T) <= S, "variant size too small");
      return *yyas_<T> ();
    }

    /// Swap the content with \a other, of same type.
    ///
    /// Both variants must be built beforehand, because swapping the actual
    /// data requires reading it (with as()), and this is not possible on
    /// unconstructed variants: it would require some dynamic testing, which
    /// should not be the variant's responsability.
    /// Swapping between built and (possibly) non-built is done with
    /// variant::move ().
    template <typename T>
    void
    swap (self_type& other)
    {
      YYASSERT (yytypeid_);
      YYASSERT (*yytypeid_ == *other.yytypeid_);
      std::swap (as<T> (), other.as<T> ());
    }

    /// Move the content of \a other to this.
    ///
    /// Destroys \a other.
    template <typename T>
    void
    move (self_type& other)
    {
      build<T> ();
      YYASSERT (yytypeid_);
      YYASSERT (*yytypeid_ == *other.yytypeid_);
      as<T> () = std::move (other.as<T> ());
      other.destroy<T> ();
    }

#if 0
    /// Copy the content of \a other to this.
    template <typename T>
    void
    copy (const self_type& other)
    {
      build<T> (other.as<T> ());
    }
#endif

    /// Destroy the stored \a T.
    template <typename T>
    void
    destroy ()
    {
      as<T> ().~T ();
      yytypeid_ = YY_NULLPTR;
    }

  private:
    /// Prohibit blind copies
    /// Don't use the templated constructor (which would only fail at runtime with an assertion)!
    self_type& operator=(const self_type&);
    self_type& operator=(self_type&&);
    variant (const self_type&);
    variant (self_type&&);

    /// Accessor to raw memory as \a T.
    template <typename T>
    T*
    yyas_ ()
    {
      void *yyp = yybuffer_.yyraw;
      return static_cast<T*> (yyp);
     }

    /// Const accessor to raw memory as \a T.
    template <typename T>
    const T*
    yyas_ () const
    {
      const void *yyp = yybuffer_.yyraw;
      return static_cast<const T*> (yyp);
     }

    union
    {
      /// Strongest alignment constraints.
      long double yyalign_me;
      /// A buffer large enough to store any of the semantic values.
      char yyraw[S];
    } yybuffer_;

    /// Whether the content is built: if defined, the name of the stored type.
    const std::type_info *yytypeid_;
  };


  /// A Bison parser.
  class parser
  {
  public:
#ifndef YYSTYPE
    /// An auxiliary type to compute the largest semantic type.
    union union_type
    {
      // indexer
      char dummy1[sizeof(ComponentPtr)];

      // exp_list
      char dummy2[sizeof(ExpressionList)];

      // input
      // primary_exp
      // string
      // pstring
      // indexable
      // callable
      // indexable2
      // unary_exp
      // power_exp
      // multiply_exp
      // additive_exp
      // relational_exp
      // equality_exp
      // and_exp
      // or_exp
      // nocond_exp
      // cond_exp
      // lambda_nocond_exp
      // lambda_exp
      // exp
      // target
      // target2
      // assignment_exp1
      // assignment_exp2
      // assignment_exp
      // small_stmt
      // simple_stmt
      // compound_stmt
      // stmt
      // statement
      // suite
      // if_stmt
      // while_stmt
      // for_stmt
      // try_stmt
      // function_stmt
      // import_stmt1
      // import_stmt2
      // import_stmt3
      // tuple
      // list
      // comp_for
      // dict1
      // dict
      // idict1
      // idict
      // num
      // range
      // unit_exp
      char dummy3[sizeof(ExpressionPtr)];

      // PSTRING
      char dummy4[sizeof(ExpressionString)];

      // item
      char dummy5[sizeof(FlagExpression)];

      // items2
      // items
      char dummy6[sizeof(FlagExpressionList)];

      // INTEGER
      // integer
      char dummy7[sizeof(Integer)];

      // arg_def
      // arg
      char dummy8[sizeof(NamedArgument)];

      // arg_defs
      // args
      char dummy9[sizeof(NamedArgumentList)];

      // identifier
      // iden
      char dummy10[sizeof(ObjectIdentifier)];

      // document
      // object
      char dummy11[sizeof(ObjectIdentifier::String)];

      // id_list
      char dummy12[sizeof(StringList)];

      // UNIT
      // CONSTANT
      char dummy13[sizeof(UnitInfo)];

      // target_list
      // target_list2
      char dummy14[sizeof(VarList)];

      // ONE
      // NUM
      char dummy15[sizeof(double)];

      // IDENTIFIER
      // CELLADDRESS
      // STRING
      // module
      // id_or_cell
      char dummy16[sizeof(std::string)];
};

    /// Symbol semantic values.
    typedef variant<sizeof(union_type)> semantic_type;
#else
    typedef YYSTYPE semantic_type;
#endif

    /// Syntax errors thrown from user actions.
    struct syntax_error : std::runtime_error
    {
      syntax_error (const std::string& m);
    };

    /// Tokens.
    struct token
    {
      enum yytokentype
      {
        TOK_END = 0,
        TOK_ONE = 258,
        TOK_NUM = 259,
        TOK_IDENTIFIER = 260,
        TOK_UNIT = 261,
        TOK_INTEGER = 262,
        TOK_CONSTANT = 263,
        TOK_CELLADDRESS = 264,
        TOK_EQ = 265,
        TOK_NEQ = 266,
        TOK_LT = 267,
        TOK_GT = 268,
        TOK_GTE = 269,
        TOK_LTE = 270,
        TOK_AND_OP = 271,
        TOK_OR_OP = 272,
        TOK_IS = 273,
        TOK_NOT = 274,
        TOK_IS_NOT = 275,
        TOK_NOT_IN = 276,
        TOK_AS = 277,
        TOK_RAISE = 278,
        TOK_TRY = 279,
        TOK_EXCEPT = 280,
        TOK_FINALLY = 281,
        TOK_IMPORT = 282,
        TOK_LAMBDA = 283,
        TOK_FROM = 284,
        TOK_POW_ASSIGN = 285,
        TOK_MUL_ASSIGN = 286,
        TOK_FDIV_ASSIGN = 287,
        TOK_DIV_ASSIGN = 288,
        TOK_MOD_ASSIGN = 289,
        TOK_ADD_ASSIGN = 290,
        TOK_SUB_ASSIGN = 291,
        TOK_MINUSSIGN = 292,
        TOK_FDIV = 293,
        TOK_PSTRING = 294,
        TOK_STRING = 295,
        TOK_EXPAND = 296,
        TOK_NEWLINE = 297,
        TOK_INDENT = 298,
        TOK_DEDENT = 299,
        TOK_IF = 300,
        TOK_ELIF = 301,
        TOK_ELSE = 302,
        TOK_WHILE = 303,
        TOK_FOR = 304,
        TOK_BREAK = 305,
        TOK_CONTINUE = 306,
        TOK_RETURN = 307,
        TOK_IN = 308,
        TOK_PY_BEGIN = 309,
        TOK_PY_END = 310,
        TOK_DEF = 311,
        TOK_PASS = 312,
        TOK_DEL = 313,
        TOK_GLOBAL = 314,
        TOK_NONLOCAL = 315,
        TOK_NUM_AND_UNIT = 316,
        TOK_NEG = 317,
        TOK_POS = 318
      };
    };

    /// (External) token type, as returned by yylex.
    typedef token::yytokentype token_type;

    /// Symbol type: an internal symbol number.
    typedef int symbol_number_type;

    /// The symbol type number to denote an empty symbol.
    enum { empty_symbol = -2 };

    /// Internal symbol number for tokens (subsumed by symbol_number_type).
    typedef unsigned char token_number_type;

    /// A complete symbol.
    ///
    /// Expects its Base type to provide access to the symbol type
    /// via type_get().
    ///
    /// Provide access to semantic value.
    template <typename Base>
    struct basic_symbol : Base
    {
      /// Alias to Base.
      typedef Base super_type;

      /// Default constructor.
      basic_symbol ();

      /// Move constructor and assignment.
      basic_symbol (basic_symbol&& other);
      basic_symbol& operator= (basic_symbol&& other);

      /// Constructor for valueless symbols, and symbols from each type.

  basic_symbol (typename Base::kind_type t);

  basic_symbol (typename Base::kind_type t, ComponentPtr&& v);

  basic_symbol (typename Base::kind_type t, ExpressionList&& v);

  basic_symbol (typename Base::kind_type t, ExpressionPtr&& v);

  basic_symbol (typename Base::kind_type t, ExpressionString&& v);

  basic_symbol (typename Base::kind_type t, FlagExpression&& v);

  basic_symbol (typename Base::kind_type t, FlagExpressionList&& v);

  basic_symbol (typename Base::kind_type t, Integer&& v);

  basic_symbol (typename Base::kind_type t, NamedArgument&& v);

  basic_symbol (typename Base::kind_type t, NamedArgumentList&& v);

  basic_symbol (typename Base::kind_type t, ObjectIdentifier&& v);

  basic_symbol (typename Base::kind_type t, ObjectIdentifier::String&& v);

  basic_symbol (typename Base::kind_type t, StringList&& v);

  basic_symbol (typename Base::kind_type t, UnitInfo&& v);

  basic_symbol (typename Base::kind_type t, VarList&& v);

  basic_symbol (typename Base::kind_type t, double&& v);

  basic_symbol (typename Base::kind_type t, std::string&& v);


      /// Constructor for symbols with semantic value.
      basic_symbol (typename Base::kind_type t,
                    semantic_type&& v);

      /// Destroy the symbol.
      ~basic_symbol ();

      /// Destroy contents, and record that is empty.
      void clear ();

      /// Whether empty.
      bool empty () const;

      /// Destructive move, \a s is emptied into this.
      void move (basic_symbol& s);

      /// The semantic value.
      semantic_type value;

    private:
      /// This class is not copyable.
      basic_symbol (const basic_symbol& other);
      basic_symbol& operator= (const basic_symbol& other);
    };

    /// Type access provider for token (enum) based symbols.
    struct by_type
    {
      /// Default constructor.
      by_type ();

      /// Copy constructor.
      by_type (const by_type& other);

      /// The symbol type as needed by the constructor.
      typedef token_type kind_type;

      /// Constructor from (external) token numbers.
      by_type (kind_type t);

      /// Record that this symbol is empty.
      void clear ();

      /// Steal the symbol type from \a that.
      void move (by_type& that);

      /// The (internal) type number (corresponding to \a type).
      /// \a empty when empty.
      symbol_number_type type_get () const;

      /// The token.
      token_type token () const;

      /// The symbol type.
      /// \a empty_symbol when empty.
      /// An int, not token_number_type, to be able to store empty_symbol.
      int type;
    };

    /// "External" symbols: returned by the scanner.
    typedef basic_symbol<by_type> symbol_type;

    // Symbol constructors declarations.
    static inline
    symbol_type
    make_END ();

    static inline
    symbol_type
    make_ONE (double&& v);

    static inline
    symbol_type
    make_NUM (double&& v);

    static inline
    symbol_type
    make_IDENTIFIER (std::string&& v);

    static inline
    symbol_type
    make_UNIT (UnitInfo&& v);

    static inline
    symbol_type
    make_INTEGER (Integer&& v);

    static inline
    symbol_type
    make_CONSTANT (UnitInfo&& v);

    static inline
    symbol_type
    make_CELLADDRESS (std::string&& v);

    static inline
    symbol_type
    make_EQ ();

    static inline
    symbol_type
    make_NEQ ();

    static inline
    symbol_type
    make_LT ();

    static inline
    symbol_type
    make_GT ();

    static inline
    symbol_type
    make_GTE ();

    static inline
    symbol_type
    make_LTE ();

    static inline
    symbol_type
    make_AND_OP ();

    static inline
    symbol_type
    make_OR_OP ();

    static inline
    symbol_type
    make_IS ();

    static inline
    symbol_type
    make_NOT ();

    static inline
    symbol_type
    make_IS_NOT ();

    static inline
    symbol_type
    make_NOT_IN ();

    static inline
    symbol_type
    make_AS ();

    static inline
    symbol_type
    make_RAISE ();

    static inline
    symbol_type
    make_TRY ();

    static inline
    symbol_type
    make_EXCEPT ();

    static inline
    symbol_type
    make_FINALLY ();

    static inline
    symbol_type
    make_IMPORT ();

    static inline
    symbol_type
    make_LAMBDA ();

    static inline
    symbol_type
    make_FROM ();

    static inline
    symbol_type
    make_POW_ASSIGN ();

    static inline
    symbol_type
    make_MUL_ASSIGN ();

    static inline
    symbol_type
    make_FDIV_ASSIGN ();

    static inline
    symbol_type
    make_DIV_ASSIGN ();

    static inline
    symbol_type
    make_MOD_ASSIGN ();

    static inline
    symbol_type
    make_ADD_ASSIGN ();

    static inline
    symbol_type
    make_SUB_ASSIGN ();

    static inline
    symbol_type
    make_MINUSSIGN ();

    static inline
    symbol_type
    make_FDIV ();

    static inline
    symbol_type
    make_PSTRING (ExpressionString&& v);

    static inline
    symbol_type
    make_STRING (std::string&& v);

    static inline
    symbol_type
    make_EXPAND ();

    static inline
    symbol_type
    make_NEWLINE ();

    static inline
    symbol_type
    make_INDENT ();

    static inline
    symbol_type
    make_DEDENT ();

    static inline
    symbol_type
    make_IF ();

    static inline
    symbol_type
    make_ELIF ();

    static inline
    symbol_type
    make_ELSE ();

    static inline
    symbol_type
    make_WHILE ();

    static inline
    symbol_type
    make_FOR ();

    static inline
    symbol_type
    make_BREAK ();

    static inline
    symbol_type
    make_CONTINUE ();

    static inline
    symbol_type
    make_RETURN ();

    static inline
    symbol_type
    make_IN ();

    static inline
    symbol_type
    make_PY_BEGIN ();

    static inline
    symbol_type
    make_PY_END ();

    static inline
    symbol_type
    make_DEF ();

    static inline
    symbol_type
    make_PASS ();

    static inline
    symbol_type
    make_DEL ();

    static inline
    symbol_type
    make_GLOBAL ();

    static inline
    symbol_type
    make_NONLOCAL ();

    static inline
    symbol_type
    make_NUM_AND_UNIT ();

    static inline
    symbol_type
    make_NEG ();

    static inline
    symbol_type
    make_POS ();


    /// Build a parser object.
    parser (Context &ctx_yyarg);
    virtual ~parser ();

    /// Parse.
    /// \returns  0 iff parsing succeeded.
    virtual int parse ();

#if YYDEBUG
    /// The current debugging stream.
    std::ostream& debug_stream () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging stream.
    void set_debug_stream (std::ostream &);

    /// Type for debugging levels.
    typedef int debug_level_type;
    /// The current debugging level.
    debug_level_type debug_level () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging level.
    void set_debug_level (debug_level_type l);
#endif

    /// Report a syntax error.
    /// \param msg    a description of the syntax error.
    virtual void error (const std::string& msg);

    /// Report a syntax error.
    void error (const syntax_error& err);

  private:
    /// This class is not copyable.
    parser (const parser&);
    parser& operator= (const parser&);

    /// State numbers.
    typedef int state_type;

    /// Generate an error message.
    /// \param yystate   the state where the error occurred.
    /// \param yyla      the lookahead token.
    virtual std::string yysyntax_error_ (state_type yystate,
                                         const symbol_type& yyla) const;

    /// Compute post-reduction state.
    /// \param yystate   the current state
    /// \param yysym     the nonterminal to push on the stack
    state_type yy_lr_goto_state_ (state_type yystate, int yysym);

    /// Whether the given \c yypact_ value indicates a defaulted state.
    /// \param yyvalue   the value to check
    static bool yy_pact_value_is_default_ (int yyvalue);

    /// Whether the given \c yytable_ value indicates a syntax error.
    /// \param yyvalue   the value to check
    static bool yy_table_value_is_error_ (int yyvalue);

    static const short int yypact_ninf_;
    static const short int yytable_ninf_;

    /// Convert a scanner token number \a t to a symbol number.
    static token_number_type yytranslate_ (int t);

    // Tables.
  // YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
  // STATE-NUM.
  static const short int yypact_[];

  // YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
  // Performed when YYTABLE does not specify something else to do.  Zero
  // means the default is an error.
  static const unsigned char yydefact_[];

  // YYPGOTO[NTERM-NUM].
  static const short int yypgoto_[];

  // YYDEFGOTO[NTERM-NUM].
  static const short int yydefgoto_[];

  // YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
  // positive, shift that token.  If negative, reduce the rule whose
  // number is the opposite.  If YYTABLE_NINF, syntax error.
  static const short int yytable_[];

  static const short int yycheck_[];

  // YYSTOS[STATE-NUM] -- The (internal number of the) accessing
  // symbol of state STATE-NUM.
  static const unsigned char yystos_[];

  // YYR1[YYN] -- Symbol number of symbol that rule YYN derives.
  static const unsigned char yyr1_[];

  // YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.
  static const unsigned char yyr2_[];


    /// Convert the symbol name \a n to a form suitable for a diagnostic.
    static std::string yytnamerr_ (const char *n);


    /// For a symbol, its name in clear.
    static const char* const yytname_[];
#if YYDEBUG
  // YYRLINE[YYN] -- Source line where rule number YYN was defined.
  static const unsigned short int yyrline_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yy_reduce_print_ (int r);
    /// Print the state stack on the debug stream.
    virtual void yystack_print_ ();

    // Debugging.
    int yydebug_;
    std::ostream* yycdebug_;

    /// \brief Display a symbol type, value and location.
    /// \param yyo    The output stream.
    /// \param yysym  The symbol.
    template <typename Base>
    void yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const;
#endif

    /// \brief Reclaim the memory associated to a symbol.
    /// \param yymsg     Why this token is reclaimed.
    ///                  If null, print nothing.
    /// \param yysym     The symbol.
    template <typename Base>
    void yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const;

  private:

    /// Type access provider for state based symbols.
    struct by_state
    {
      /// Default constructor.
      by_state ();

      /// The symbol type as needed by the constructor.
      typedef state_type kind_type;

      /// Constructor.
      by_state (kind_type s);

      /// Copy constructor.
      by_state (const by_state& other);

      /// Record that this symbol is empty.
      void clear ();

      /// Steal the symbol type from \a that.
      void move (by_state& that);

      /// The (internal) type number (corresponding to \a state).
      /// \a empty_symbol when empty.
      symbol_number_type type_get () const;

      /// The state number used to denote an empty symbol.
      enum { empty_state = -1 };

      /// The state.
      /// \a empty when empty.
      state_type state;
    };

    /// "Internal" symbol: element of the stack.
    struct stack_symbol_type : basic_symbol<by_state>
    {
      /// Superclass.
      typedef basic_symbol<by_state> super_type;
      /// Construct an empty symbol.
      stack_symbol_type ();
      /// Steal the contents from \a sym to build this.
      stack_symbol_type (state_type s, symbol_type& sym);

      /// Move constructor. Manual defined as MSVC2013 is not able to auto generate
      stack_symbol_type(stack_symbol_type &&other)
          :super_type(std::move(other))
      {}

      /// Move assignment. Manual defined as MSVC2013 is not able to auto generate
      stack_symbol_type &operator=(stack_symbol_type &&other) {
          super_type::operator=(std::move(other));
          return *this;
      }
    };

    /// Stack type.
    typedef stack<stack_symbol_type> stack_type;

    /// The stack.
    stack_type yystack_;

    /// Push a new state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the symbol
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, stack_symbol_type& s);

    /// Push a new look ahead token on the state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the state
    /// \param sym  the symbol (for its value and location).
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, state_type s, symbol_type& sym);

    /// Pop \a n symbols the three stacks.
    void yypop_ (unsigned int n = 1);

    /// Constants.
    enum
    {
      yyeof_ = 0,
      yylast_ = 2036,     ///< Last index in yytable_.
      yynnts_ = 71,  ///< Number of nonterminal symbols.
      yyfinal_ = 149, ///< Termination state number.
      yyterror_ = 1,
      yyerrcode_ = 256,
      yyntokens_ = 82  ///< Number of tokens.
    };


    // User arguments.
    Context &ctx;
  };


#line 23 "ExpressionParser.y" // lalr1.cc:418
} } // App::ExpressionParser
#line 1210 "ExpressionParser.tab.cc" // lalr1.cc:418





// User implementation prologue.

#line 1218 "ExpressionParser.tab.cc" // lalr1.cc:422
// Unqualified %code blocks.
#line 30 "ExpressionParser.y" // lalr1.cc:423


#undef YY_DECL
#define YY_DECL int yylex(parser::semantic_type* yylval, Context &ctx)
namespace App { 
    namespace ExpressionParser {
        template <class T> void stack_prepare (ExpressionParserStack &s) { s.reserve (200); }

        YY_DECL;
    } 
}


#line 1234 "ExpressionParser.tab.cc" // lalr1.cc:423


#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif



// Suppress unused-variable warnings by "using" E.
#define YYUSE(E) ((void) (E))

// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << std::endl;                  \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yystack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YYUSE(Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void>(0)
# define YY_STACK_PRINT()                static_cast<void>(0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

#line 23 "ExpressionParser.y" // lalr1.cc:489
namespace App { namespace ExpressionParser {
#line 1301 "ExpressionParser.tab.cc" // lalr1.cc:489

  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  parser::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr = "";
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
              // Fall through.
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }


  /// Build a parser object.
  parser::parser (Context &ctx_yyarg)
    :
#if YYDEBUG
      yydebug_ (false),
      yycdebug_ (&std::cerr),
#endif
      ctx (ctx_yyarg)
  {}

  parser::~parser ()
  {}


  /*---------------.
  | Symbol types.  |
  `---------------*/

  inline
  parser::syntax_error::syntax_error (const std::string& m)
    : std::runtime_error (m)
  {}

  // basic_symbol.
  template <typename Base>
  inline
  parser::basic_symbol<Base>::basic_symbol ()
    : value ()
  {}

  template <typename Base>
  inline
  parser::basic_symbol<Base>::basic_symbol (basic_symbol&& other)
  {
    *this = std::move (other);
  }

  template <typename Base>
  inline
  parser::basic_symbol<Base>& parser::basic_symbol<Base>::operator= (basic_symbol&& other)
  {
    static_cast<Base &> (*this) = other;
      switch (other.type_get ())
    {
      case 150: // indexer
        value.move< ComponentPtr > (other.value);
        break;

      case 109: // exp_list
        value.move< ExpressionList > (other.value);
        break;

      case 83: // input
      case 84: // primary_exp
      case 85: // string
      case 86: // pstring
      case 87: // indexable
      case 88: // callable
      case 89: // indexable2
      case 90: // unary_exp
      case 91: // power_exp
      case 92: // multiply_exp
      case 93: // additive_exp
      case 94: // relational_exp
      case 95: // equality_exp
      case 96: // and_exp
      case 97: // or_exp
      case 98: // nocond_exp
      case 99: // cond_exp
      case 100: // lambda_nocond_exp
      case 101: // lambda_exp
      case 102: // exp
      case 104: // target
      case 106: // target2
      case 108: // assignment_exp1
      case 110: // assignment_exp2
      case 111: // assignment_exp
      case 112: // small_stmt
      case 113: // simple_stmt
      case 114: // compound_stmt
      case 115: // stmt
      case 116: // statement
      case 117: // suite
      case 118: // if_stmt
      case 119: // while_stmt
      case 120: // for_stmt
      case 121: // try_stmt
      case 124: // function_stmt
      case 126: // import_stmt1
      case 127: // import_stmt2
      case 128: // import_stmt3
      case 133: // tuple
      case 135: // list
      case 136: // comp_for
      case 138: // dict1
      case 139: // dict
      case 140: // idict1
      case 141: // idict
      case 144: // num
      case 145: // range
      case 146: // unit_exp
        value.move< ExpressionPtr > (other.value);
        break;

      case 39: // PSTRING
        value.move< ExpressionString > (other.value);
        break;

      case 131: // item
        value.move< FlagExpression > (other.value);
        break;

      case 132: // items2
      case 134: // items
        value.move< FlagExpressionList > (other.value);
        break;

      case 7: // INTEGER
      case 149: // integer
        value.move< Integer > (other.value);
        break;

      case 122: // arg_def
      case 142: // arg
        value.move< NamedArgument > (other.value);
        break;

      case 123: // arg_defs
      case 143: // args
        value.move< NamedArgumentList > (other.value);
        break;

      case 147: // identifier
      case 148: // iden
        value.move< ObjectIdentifier > (other.value);
        break;

      case 151: // document
      case 152: // object
        value.move< ObjectIdentifier::String > (other.value);
        break;

      case 103: // id_list
        value.move< StringList > (other.value);
        break;

      case 6: // UNIT
      case 8: // CONSTANT
        value.move< UnitInfo > (other.value);
        break;

      case 105: // target_list
      case 107: // target_list2
        value.move< VarList > (other.value);
        break;

      case 3: // ONE
      case 4: // NUM
        value.move< double > (other.value);
        break;

      case 5: // IDENTIFIER
      case 9: // CELLADDRESS
      case 40: // STRING
      case 125: // module
      case 129: // id_or_cell
        value.move< std::string > (other.value);
        break;

      default:
        break;
    }

    return *this;
  }

  template <typename Base>
  inline
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, semantic_type&& v)
    : Base (t)
    , value ()
  {
    (void) v;
      switch (this->type_get ())
    {
      case 150: // indexer
        value.move< ComponentPtr > (v);
        break;

      case 109: // exp_list
        value.move< ExpressionList > (v);
        break;

      case 83: // input
      case 84: // primary_exp
      case 85: // string
      case 86: // pstring
      case 87: // indexable
      case 88: // callable
      case 89: // indexable2
      case 90: // unary_exp
      case 91: // power_exp
      case 92: // multiply_exp
      case 93: // additive_exp
      case 94: // relational_exp
      case 95: // equality_exp
      case 96: // and_exp
      case 97: // or_exp
      case 98: // nocond_exp
      case 99: // cond_exp
      case 100: // lambda_nocond_exp
      case 101: // lambda_exp
      case 102: // exp
      case 104: // target
      case 106: // target2
      case 108: // assignment_exp1
      case 110: // assignment_exp2
      case 111: // assignment_exp
      case 112: // small_stmt
      case 113: // simple_stmt
      case 114: // compound_stmt
      case 115: // stmt
      case 116: // statement
      case 117: // suite
      case 118: // if_stmt
      case 119: // while_stmt
      case 120: // for_stmt
      case 121: // try_stmt
      case 124: // function_stmt
      case 126: // import_stmt1
      case 127: // import_stmt2
      case 128: // import_stmt3
      case 133: // tuple
      case 135: // list
      case 136: // comp_for
      case 138: // dict1
      case 139: // dict
      case 140: // idict1
      case 141: // idict
      case 144: // num
      case 145: // range
      case 146: // unit_exp
        value.move< ExpressionPtr > (v);
        break;

      case 39: // PSTRING
        value.move< ExpressionString > (v);
        break;

      case 131: // item
        value.move< FlagExpression > (v);
        break;

      case 132: // items2
      case 134: // items
        value.move< FlagExpressionList > (v);
        break;

      case 7: // INTEGER
      case 149: // integer
        value.move< Integer > (v);
        break;

      case 122: // arg_def
      case 142: // arg
        value.move< NamedArgument > (v);
        break;

      case 123: // arg_defs
      case 143: // args
        value.move< NamedArgumentList > (v);
        break;

      case 147: // identifier
      case 148: // iden
        value.move< ObjectIdentifier > (v);
        break;

      case 151: // document
      case 152: // object
        value.move< ObjectIdentifier::String > (v);
        break;

      case 103: // id_list
        value.move< StringList > (v);
        break;

      case 6: // UNIT
      case 8: // CONSTANT
        value.move< UnitInfo > (v);
        break;

      case 105: // target_list
      case 107: // target_list2
        value.move< VarList > (v);
        break;

      case 3: // ONE
      case 4: // NUM
        value.move< double > (v);
        break;

      case 5: // IDENTIFIER
      case 9: // CELLADDRESS
      case 40: // STRING
      case 125: // module
      case 129: // id_or_cell
        value.move< std::string > (v);
        break;

      default:
        break;
    }
}


  // Implementation of basic_symbol constructor for each type.

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t)
    : Base (t)
    , value ()
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, ComponentPtr&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, ExpressionList&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, ExpressionPtr&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, ExpressionString&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, FlagExpression&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, FlagExpressionList&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, Integer&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, NamedArgument&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, NamedArgumentList&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, ObjectIdentifier&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, ObjectIdentifier::String&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, StringList&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, UnitInfo&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, VarList&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, double&& v)
    : Base (t)
    , value (std::move (v))
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, std::string&& v)
    : Base (t)
    , value (std::move (v))
  {}


  template <typename Base>
  inline
  parser::basic_symbol<Base>::~basic_symbol ()
  {
    clear ();
  }

  template <typename Base>
  inline
  void
  parser::basic_symbol<Base>::clear ()
  {
    // User destructor.
    symbol_number_type yytype = this->type_get ();
    basic_symbol<Base>& yysym = *this;
    (void) yysym;
    switch (yytype)
    {
   default:
      break;
    }

    // Type destructor.
    switch (yytype)
    {
      case 150: // indexer
        value.template destroy< ComponentPtr > ();
        break;

      case 109: // exp_list
        value.template destroy< ExpressionList > ();
        break;

      case 83: // input
      case 84: // primary_exp
      case 85: // string
      case 86: // pstring
      case 87: // indexable
      case 88: // callable
      case 89: // indexable2
      case 90: // unary_exp
      case 91: // power_exp
      case 92: // multiply_exp
      case 93: // additive_exp
      case 94: // relational_exp
      case 95: // equality_exp
      case 96: // and_exp
      case 97: // or_exp
      case 98: // nocond_exp
      case 99: // cond_exp
      case 100: // lambda_nocond_exp
      case 101: // lambda_exp
      case 102: // exp
      case 104: // target
      case 106: // target2
      case 108: // assignment_exp1
      case 110: // assignment_exp2
      case 111: // assignment_exp
      case 112: // small_stmt
      case 113: // simple_stmt
      case 114: // compound_stmt
      case 115: // stmt
      case 116: // statement
      case 117: // suite
      case 118: // if_stmt
      case 119: // while_stmt
      case 120: // for_stmt
      case 121: // try_stmt
      case 124: // function_stmt
      case 126: // import_stmt1
      case 127: // import_stmt2
      case 128: // import_stmt3
      case 133: // tuple
      case 135: // list
      case 136: // comp_for
      case 138: // dict1
      case 139: // dict
      case 140: // idict1
      case 141: // idict
      case 144: // num
      case 145: // range
      case 146: // unit_exp
        value.template destroy< ExpressionPtr > ();
        break;

      case 39: // PSTRING
        value.template destroy< ExpressionString > ();
        break;

      case 131: // item
        value.template destroy< FlagExpression > ();
        break;

      case 132: // items2
      case 134: // items
        value.template destroy< FlagExpressionList > ();
        break;

      case 7: // INTEGER
      case 149: // integer
        value.template destroy< Integer > ();
        break;

      case 122: // arg_def
      case 142: // arg
        value.template destroy< NamedArgument > ();
        break;

      case 123: // arg_defs
      case 143: // args
        value.template destroy< NamedArgumentList > ();
        break;

      case 147: // identifier
      case 148: // iden
        value.template destroy< ObjectIdentifier > ();
        break;

      case 151: // document
      case 152: // object
        value.template destroy< ObjectIdentifier::String > ();
        break;

      case 103: // id_list
        value.template destroy< StringList > ();
        break;

      case 6: // UNIT
      case 8: // CONSTANT
        value.template destroy< UnitInfo > ();
        break;

      case 105: // target_list
      case 107: // target_list2
        value.template destroy< VarList > ();
        break;

      case 3: // ONE
      case 4: // NUM
        value.template destroy< double > ();
        break;

      case 5: // IDENTIFIER
      case 9: // CELLADDRESS
      case 40: // STRING
      case 125: // module
      case 129: // id_or_cell
        value.template destroy< std::string > ();
        break;

      default:
        break;
    }

    Base::clear ();
  }

  template <typename Base>
  inline
  bool
  parser::basic_symbol<Base>::empty () const
  {
    return Base::type_get () == empty_symbol;
  }

  template <typename Base>
  inline
  void
  parser::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move(s);
      switch (this->type_get ())
    {
      case 150: // indexer
        value.move< ComponentPtr > (s.value);
        break;

      case 109: // exp_list
        value.move< ExpressionList > (s.value);
        break;

      case 83: // input
      case 84: // primary_exp
      case 85: // string
      case 86: // pstring
      case 87: // indexable
      case 88: // callable
      case 89: // indexable2
      case 90: // unary_exp
      case 91: // power_exp
      case 92: // multiply_exp
      case 93: // additive_exp
      case 94: // relational_exp
      case 95: // equality_exp
      case 96: // and_exp
      case 97: // or_exp
      case 98: // nocond_exp
      case 99: // cond_exp
      case 100: // lambda_nocond_exp
      case 101: // lambda_exp
      case 102: // exp
      case 104: // target
      case 106: // target2
      case 108: // assignment_exp1
      case 110: // assignment_exp2
      case 111: // assignment_exp
      case 112: // small_stmt
      case 113: // simple_stmt
      case 114: // compound_stmt
      case 115: // stmt
      case 116: // statement
      case 117: // suite
      case 118: // if_stmt
      case 119: // while_stmt
      case 120: // for_stmt
      case 121: // try_stmt
      case 124: // function_stmt
      case 126: // import_stmt1
      case 127: // import_stmt2
      case 128: // import_stmt3
      case 133: // tuple
      case 135: // list
      case 136: // comp_for
      case 138: // dict1
      case 139: // dict
      case 140: // idict1
      case 141: // idict
      case 144: // num
      case 145: // range
      case 146: // unit_exp
        value.move< ExpressionPtr > (s.value);
        break;

      case 39: // PSTRING
        value.move< ExpressionString > (s.value);
        break;

      case 131: // item
        value.move< FlagExpression > (s.value);
        break;

      case 132: // items2
      case 134: // items
        value.move< FlagExpressionList > (s.value);
        break;

      case 7: // INTEGER
      case 149: // integer
        value.move< Integer > (s.value);
        break;

      case 122: // arg_def
      case 142: // arg
        value.move< NamedArgument > (s.value);
        break;

      case 123: // arg_defs
      case 143: // args
        value.move< NamedArgumentList > (s.value);
        break;

      case 147: // identifier
      case 148: // iden
        value.move< ObjectIdentifier > (s.value);
        break;

      case 151: // document
      case 152: // object
        value.move< ObjectIdentifier::String > (s.value);
        break;

      case 103: // id_list
        value.move< StringList > (s.value);
        break;

      case 6: // UNIT
      case 8: // CONSTANT
        value.move< UnitInfo > (s.value);
        break;

      case 105: // target_list
      case 107: // target_list2
        value.move< VarList > (s.value);
        break;

      case 3: // ONE
      case 4: // NUM
        value.move< double > (s.value);
        break;

      case 5: // IDENTIFIER
      case 9: // CELLADDRESS
      case 40: // STRING
      case 125: // module
      case 129: // id_or_cell
        value.move< std::string > (s.value);
        break;

      default:
        break;
    }

  }

  // by_type.
  inline
  parser::by_type::by_type ()
    : type (empty_symbol)
  {}

  inline
  parser::by_type::by_type (const by_type& other)
    : type (other.type)
  {}

  inline
  parser::by_type::by_type (token_type t)
    : type (yytranslate_ (t))
  {}

  inline
  void
  parser::by_type::clear ()
  {
    type = empty_symbol;
  }

  inline
  void
  parser::by_type::move (by_type& that)
  {
    type = that.type;
    that.clear ();
  }

  inline
  int
  parser::by_type::type_get () const
  {
    return type;
  }
  // Implementation of make_symbol for each symbol type.
  parser::symbol_type
  parser::make_END ()
  {
    return symbol_type (token::TOK_END);
  }

  parser::symbol_type
  parser::make_ONE (double&& v)
  {
    return symbol_type (token::TOK_ONE, std::move (v));
  }

  parser::symbol_type
  parser::make_NUM (double&& v)
  {
    return symbol_type (token::TOK_NUM, std::move (v));
  }

  parser::symbol_type
  parser::make_IDENTIFIER (std::string&& v)
  {
    return symbol_type (token::TOK_IDENTIFIER, std::move (v));
  }

  parser::symbol_type
  parser::make_UNIT (UnitInfo&& v)
  {
    return symbol_type (token::TOK_UNIT, std::move (v));
  }

  parser::symbol_type
  parser::make_INTEGER (Integer&& v)
  {
    return symbol_type (token::TOK_INTEGER, std::move (v));
  }

  parser::symbol_type
  parser::make_CONSTANT (UnitInfo&& v)
  {
    return symbol_type (token::TOK_CONSTANT, std::move (v));
  }

  parser::symbol_type
  parser::make_CELLADDRESS (std::string&& v)
  {
    return symbol_type (token::TOK_CELLADDRESS, std::move (v));
  }

  parser::symbol_type
  parser::make_EQ ()
  {
    return symbol_type (token::TOK_EQ);
  }

  parser::symbol_type
  parser::make_NEQ ()
  {
    return symbol_type (token::TOK_NEQ);
  }

  parser::symbol_type
  parser::make_LT ()
  {
    return symbol_type (token::TOK_LT);
  }

  parser::symbol_type
  parser::make_GT ()
  {
    return symbol_type (token::TOK_GT);
  }

  parser::symbol_type
  parser::make_GTE ()
  {
    return symbol_type (token::TOK_GTE);
  }

  parser::symbol_type
  parser::make_LTE ()
  {
    return symbol_type (token::TOK_LTE);
  }

  parser::symbol_type
  parser::make_AND_OP ()
  {
    return symbol_type (token::TOK_AND_OP);
  }

  parser::symbol_type
  parser::make_OR_OP ()
  {
    return symbol_type (token::TOK_OR_OP);
  }

  parser::symbol_type
  parser::make_IS ()
  {
    return symbol_type (token::TOK_IS);
  }

  parser::symbol_type
  parser::make_NOT ()
  {
    return symbol_type (token::TOK_NOT);
  }

  parser::symbol_type
  parser::make_IS_NOT ()
  {
    return symbol_type (token::TOK_IS_NOT);
  }

  parser::symbol_type
  parser::make_NOT_IN ()
  {
    return symbol_type (token::TOK_NOT_IN);
  }

  parser::symbol_type
  parser::make_AS ()
  {
    return symbol_type (token::TOK_AS);
  }

  parser::symbol_type
  parser::make_RAISE ()
  {
    return symbol_type (token::TOK_RAISE);
  }

  parser::symbol_type
  parser::make_TRY ()
  {
    return symbol_type (token::TOK_TRY);
  }

  parser::symbol_type
  parser::make_EXCEPT ()
  {
    return symbol_type (token::TOK_EXCEPT);
  }

  parser::symbol_type
  parser::make_FINALLY ()
  {
    return symbol_type (token::TOK_FINALLY);
  }

  parser::symbol_type
  parser::make_IMPORT ()
  {
    return symbol_type (token::TOK_IMPORT);
  }

  parser::symbol_type
  parser::make_LAMBDA ()
  {
    return symbol_type (token::TOK_LAMBDA);
  }

  parser::symbol_type
  parser::make_FROM ()
  {
    return symbol_type (token::TOK_FROM);
  }

  parser::symbol_type
  parser::make_POW_ASSIGN ()
  {
    return symbol_type (token::TOK_POW_ASSIGN);
  }

  parser::symbol_type
  parser::make_MUL_ASSIGN ()
  {
    return symbol_type (token::TOK_MUL_ASSIGN);
  }

  parser::symbol_type
  parser::make_FDIV_ASSIGN ()
  {
    return symbol_type (token::TOK_FDIV_ASSIGN);
  }

  parser::symbol_type
  parser::make_DIV_ASSIGN ()
  {
    return symbol_type (token::TOK_DIV_ASSIGN);
  }

  parser::symbol_type
  parser::make_MOD_ASSIGN ()
  {
    return symbol_type (token::TOK_MOD_ASSIGN);
  }

  parser::symbol_type
  parser::make_ADD_ASSIGN ()
  {
    return symbol_type (token::TOK_ADD_ASSIGN);
  }

  parser::symbol_type
  parser::make_SUB_ASSIGN ()
  {
    return symbol_type (token::TOK_SUB_ASSIGN);
  }

  parser::symbol_type
  parser::make_MINUSSIGN ()
  {
    return symbol_type (token::TOK_MINUSSIGN);
  }

  parser::symbol_type
  parser::make_FDIV ()
  {
    return symbol_type (token::TOK_FDIV);
  }

  parser::symbol_type
  parser::make_PSTRING (ExpressionString&& v)
  {
    return symbol_type (token::TOK_PSTRING, std::move (v));
  }

  parser::symbol_type
  parser::make_STRING (std::string&& v)
  {
    return symbol_type (token::TOK_STRING, std::move (v));
  }

  parser::symbol_type
  parser::make_EXPAND ()
  {
    return symbol_type (token::TOK_EXPAND);
  }

  parser::symbol_type
  parser::make_NEWLINE ()
  {
    return symbol_type (token::TOK_NEWLINE);
  }

  parser::symbol_type
  parser::make_INDENT ()
  {
    return symbol_type (token::TOK_INDENT);
  }

  parser::symbol_type
  parser::make_DEDENT ()
  {
    return symbol_type (token::TOK_DEDENT);
  }

  parser::symbol_type
  parser::make_IF ()
  {
    return symbol_type (token::TOK_IF);
  }

  parser::symbol_type
  parser::make_ELIF ()
  {
    return symbol_type (token::TOK_ELIF);
  }

  parser::symbol_type
  parser::make_ELSE ()
  {
    return symbol_type (token::TOK_ELSE);
  }

  parser::symbol_type
  parser::make_WHILE ()
  {
    return symbol_type (token::TOK_WHILE);
  }

  parser::symbol_type
  parser::make_FOR ()
  {
    return symbol_type (token::TOK_FOR);
  }

  parser::symbol_type
  parser::make_BREAK ()
  {
    return symbol_type (token::TOK_BREAK);
  }

  parser::symbol_type
  parser::make_CONTINUE ()
  {
    return symbol_type (token::TOK_CONTINUE);
  }

  parser::symbol_type
  parser::make_RETURN ()
  {
    return symbol_type (token::TOK_RETURN);
  }

  parser::symbol_type
  parser::make_IN ()
  {
    return symbol_type (token::TOK_IN);
  }

  parser::symbol_type
  parser::make_PY_BEGIN ()
  {
    return symbol_type (token::TOK_PY_BEGIN);
  }

  parser::symbol_type
  parser::make_PY_END ()
  {
    return symbol_type (token::TOK_PY_END);
  }

  parser::symbol_type
  parser::make_DEF ()
  {
    return symbol_type (token::TOK_DEF);
  }

  parser::symbol_type
  parser::make_PASS ()
  {
    return symbol_type (token::TOK_PASS);
  }

  parser::symbol_type
  parser::make_DEL ()
  {
    return symbol_type (token::TOK_DEL);
  }

  parser::symbol_type
  parser::make_GLOBAL ()
  {
    return symbol_type (token::TOK_GLOBAL);
  }

  parser::symbol_type
  parser::make_NONLOCAL ()
  {
    return symbol_type (token::TOK_NONLOCAL);
  }

  parser::symbol_type
  parser::make_NUM_AND_UNIT ()
  {
    return symbol_type (token::TOK_NUM_AND_UNIT);
  }

  parser::symbol_type
  parser::make_NEG ()
  {
    return symbol_type (token::TOK_NEG);
  }

  parser::symbol_type
  parser::make_POS ()
  {
    return symbol_type (token::TOK_POS);
  }



  // by_state.
  inline
  parser::by_state::by_state ()
    : state (empty_state)
  {}

  inline
  parser::by_state::by_state (const by_state& other)
    : state (other.state)
  {}

  inline
  void
  parser::by_state::clear ()
  {
    state = empty_state;
  }

  inline
  void
  parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  inline
  parser::by_state::by_state (state_type s)
    : state (s)
  {}

  inline
  parser::symbol_number_type
  parser::by_state::type_get () const
  {
    if (state == empty_state)
      return empty_symbol;
    else
      return yystos_[state];
  }

  inline
  parser::stack_symbol_type::stack_symbol_type ()
  {}


  inline
  parser::stack_symbol_type::stack_symbol_type (state_type s, symbol_type& that)
    : super_type (s)
  {
      switch (that.type_get ())
    {
      case 150: // indexer
        value.move< ComponentPtr > (that.value);
        break;

      case 109: // exp_list
        value.move< ExpressionList > (that.value);
        break;

      case 83: // input
      case 84: // primary_exp
      case 85: // string
      case 86: // pstring
      case 87: // indexable
      case 88: // callable
      case 89: // indexable2
      case 90: // unary_exp
      case 91: // power_exp
      case 92: // multiply_exp
      case 93: // additive_exp
      case 94: // relational_exp
      case 95: // equality_exp
      case 96: // and_exp
      case 97: // or_exp
      case 98: // nocond_exp
      case 99: // cond_exp
      case 100: // lambda_nocond_exp
      case 101: // lambda_exp
      case 102: // exp
      case 104: // target
      case 106: // target2
      case 108: // assignment_exp1
      case 110: // assignment_exp2
      case 111: // assignment_exp
      case 112: // small_stmt
      case 113: // simple_stmt
      case 114: // compound_stmt
      case 115: // stmt
      case 116: // statement
      case 117: // suite
      case 118: // if_stmt
      case 119: // while_stmt
      case 120: // for_stmt
      case 121: // try_stmt
      case 124: // function_stmt
      case 126: // import_stmt1
      case 127: // import_stmt2
      case 128: // import_stmt3
      case 133: // tuple
      case 135: // list
      case 136: // comp_for
      case 138: // dict1
      case 139: // dict
      case 140: // idict1
      case 141: // idict
      case 144: // num
      case 145: // range
      case 146: // unit_exp
        value.move< ExpressionPtr > (that.value);
        break;

      case 39: // PSTRING
        value.move< ExpressionString > (that.value);
        break;

      case 131: // item
        value.move< FlagExpression > (that.value);
        break;

      case 132: // items2
      case 134: // items
        value.move< FlagExpressionList > (that.value);
        break;

      case 7: // INTEGER
      case 149: // integer
        value.move< Integer > (that.value);
        break;

      case 122: // arg_def
      case 142: // arg
        value.move< NamedArgument > (that.value);
        break;

      case 123: // arg_defs
      case 143: // args
        value.move< NamedArgumentList > (that.value);
        break;

      case 147: // identifier
      case 148: // iden
        value.move< ObjectIdentifier > (that.value);
        break;

      case 151: // document
      case 152: // object
        value.move< ObjectIdentifier::String > (that.value);
        break;

      case 103: // id_list
        value.move< StringList > (that.value);
        break;

      case 6: // UNIT
      case 8: // CONSTANT
        value.move< UnitInfo > (that.value);
        break;

      case 105: // target_list
      case 107: // target_list2
        value.move< VarList > (that.value);
        break;

      case 3: // ONE
      case 4: // NUM
        value.move< double > (that.value);
        break;

      case 5: // IDENTIFIER
      case 9: // CELLADDRESS
      case 40: // STRING
      case 125: // module
      case 129: // id_or_cell
        value.move< std::string > (that.value);
        break;

      default:
        break;
    }

    // that is emptied.
    that.type = empty_symbol;
  }

#if 0
  inline
  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
      switch (that.type_get ())
    {
      case 150: // indexer
        value.copy< ComponentPtr > (that.value);
        break;

      case 109: // exp_list
        value.copy< ExpressionList > (that.value);
        break;

      case 83: // input
      case 84: // primary_exp
      case 85: // string
      case 86: // pstring
      case 87: // indexable
      case 88: // callable
      case 89: // indexable2
      case 90: // unary_exp
      case 91: // power_exp
      case 92: // multiply_exp
      case 93: // additive_exp
      case 94: // relational_exp
      case 95: // equality_exp
      case 96: // and_exp
      case 97: // or_exp
      case 98: // nocond_exp
      case 99: // cond_exp
      case 100: // lambda_nocond_exp
      case 101: // lambda_exp
      case 102: // exp
      case 104: // target
      case 106: // target2
      case 108: // assignment_exp1
      case 110: // assignment_exp2
      case 111: // assignment_exp
      case 112: // small_stmt
      case 113: // simple_stmt
      case 114: // compound_stmt
      case 115: // stmt
      case 116: // statement
      case 117: // suite
      case 118: // if_stmt
      case 119: // while_stmt
      case 120: // for_stmt
      case 121: // try_stmt
      case 124: // function_stmt
      case 126: // import_stmt1
      case 127: // import_stmt2
      case 128: // import_stmt3
      case 133: // tuple
      case 135: // list
      case 136: // comp_for
      case 138: // dict1
      case 139: // dict
      case 140: // idict1
      case 141: // idict
      case 144: // num
      case 145: // range
      case 146: // unit_exp
        value.copy< ExpressionPtr > (that.value);
        break;

      case 39: // PSTRING
        value.copy< ExpressionString > (that.value);
        break;

      case 131: // item
        value.copy< FlagExpression > (that.value);
        break;

      case 132: // items2
      case 134: // items
        value.copy< FlagExpressionList > (that.value);
        break;

      case 7: // INTEGER
      case 149: // integer
        value.copy< Integer > (that.value);
        break;

      case 122: // arg_def
      case 142: // arg
        value.copy< NamedArgument > (that.value);
        break;

      case 123: // arg_defs
      case 143: // args
        value.copy< NamedArgumentList > (that.value);
        break;

      case 147: // identifier
      case 148: // iden
        value.copy< ObjectIdentifier > (that.value);
        break;

      case 151: // document
      case 152: // object
        value.copy< ObjectIdentifier::String > (that.value);
        break;

      case 103: // id_list
        value.copy< StringList > (that.value);
        break;

      case 6: // UNIT
      case 8: // CONSTANT
        value.copy< UnitInfo > (that.value);
        break;

      case 105: // target_list
      case 107: // target_list2
        value.copy< VarList > (that.value);
        break;

      case 3: // ONE
      case 4: // NUM
        value.copy< double > (that.value);
        break;

      case 5: // IDENTIFIER
      case 9: // CELLADDRESS
      case 40: // STRING
      case 125: // module
      case 129: // id_or_cell
        value.copy< std::string > (that.value);
        break;

      default:
        break;
    }

    return *this;
  }
#endif


  template <typename Base>
  inline
  void
  parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YYDEBUG
  template <typename Base>
  void
  parser::yy_print_ (std::ostream& yyo,
                                     const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YYUSE (yyoutput);
    symbol_number_type yytype = yysym.type_get ();
    // Avoid a (spurious) G++ 4.8 warning about "array subscript is
    // below array bounds".
    if (yysym.empty ())
      std::abort ();
    yyo << (yytype < yyntokens_ ? "token" : "nterm")
        << ' ' << yytname_[yytype] << " (";
    YYUSE (yytype);
    yyo << ')';
  }
#endif

  inline
  void
  parser::yypush_ (const char* m, state_type s, symbol_type& sym)
  {
    stack_symbol_type t (s, sym);
    yypush_ (m, t);
  }

  inline
  void
  parser::yypush_ (const char* m, stack_symbol_type& s)
  {
    if (m)
      YY_SYMBOL_PRINT (m, s);
    yystack_.push (s);
  }

  inline
  void
  parser::yypop_ (unsigned int n)
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  parser::debug_level_type
  parser::debug_level () const
  {
    return yydebug_;
  }

  void
  parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  inline parser::state_type
  parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - yyntokens_] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - yyntokens_];
  }

  inline bool
  parser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  inline bool
  parser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  parser::parse ()
  {
    // State.
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The return value of parse ().
    int yyresult;

    // FIXME: This shoud be completely indented.  It is not yet to
    // avoid gratuitous conflicts when merging into the master branch.
    try
      {
    YYCDEBUG << "Starting parse" << std::endl;


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, yyla);

    // A new symbol was pushed on the stack.
  yynewstate:
    YYCDEBUG << "Entering state " << yystack_[0].state << std::endl;

    // Accept?
    if (yystack_[0].state == yyfinal_)
      goto yyacceptlab;

    goto yybackup;

    // Backup.
  yybackup:

    // Try to take a decision without lookahead.
    yyn = yypact_[yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token: ";
        try
          {
            yyla.type = yytranslate_ (yylex (&yyla.value, ctx));
          }
        catch (const syntax_error& yyexc)
          {
            error (yyexc);
            goto yyerrlab1;
          }
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.type_get ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.type_get ())
      goto yydefault;

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", yyn, yyla);
    goto yynewstate;

  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;

  /*-----------------------------.
  | yyreduce -- Do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_(yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
        switch (yyr1_[yyn])
    {
      case 150: // indexer
        yylhs.value.build< ComponentPtr > ();
        break;

      case 109: // exp_list
        yylhs.value.build< ExpressionList > ();
        break;

      case 83: // input
      case 84: // primary_exp
      case 85: // string
      case 86: // pstring
      case 87: // indexable
      case 88: // callable
      case 89: // indexable2
      case 90: // unary_exp
      case 91: // power_exp
      case 92: // multiply_exp
      case 93: // additive_exp
      case 94: // relational_exp
      case 95: // equality_exp
      case 96: // and_exp
      case 97: // or_exp
      case 98: // nocond_exp
      case 99: // cond_exp
      case 100: // lambda_nocond_exp
      case 101: // lambda_exp
      case 102: // exp
      case 104: // target
      case 106: // target2
      case 108: // assignment_exp1
      case 110: // assignment_exp2
      case 111: // assignment_exp
      case 112: // small_stmt
      case 113: // simple_stmt
      case 114: // compound_stmt
      case 115: // stmt
      case 116: // statement
      case 117: // suite
      case 118: // if_stmt
      case 119: // while_stmt
      case 120: // for_stmt
      case 121: // try_stmt
      case 124: // function_stmt
      case 126: // import_stmt1
      case 127: // import_stmt2
      case 128: // import_stmt3
      case 133: // tuple
      case 135: // list
      case 136: // comp_for
      case 138: // dict1
      case 139: // dict
      case 140: // idict1
      case 141: // idict
      case 144: // num
      case 145: // range
      case 146: // unit_exp
        yylhs.value.build< ExpressionPtr > ();
        break;

      case 39: // PSTRING
        yylhs.value.build< ExpressionString > ();
        break;

      case 131: // item
        yylhs.value.build< FlagExpression > ();
        break;

      case 132: // items2
      case 134: // items
        yylhs.value.build< FlagExpressionList > ();
        break;

      case 7: // INTEGER
      case 149: // integer
        yylhs.value.build< Integer > ();
        break;

      case 122: // arg_def
      case 142: // arg
        yylhs.value.build< NamedArgument > ();
        break;

      case 123: // arg_defs
      case 143: // args
        yylhs.value.build< NamedArgumentList > ();
        break;

      case 147: // identifier
      case 148: // iden
        yylhs.value.build< ObjectIdentifier > ();
        break;

      case 151: // document
      case 152: // object
        yylhs.value.build< ObjectIdentifier::String > ();
        break;

      case 103: // id_list
        yylhs.value.build< StringList > ();
        break;

      case 6: // UNIT
      case 8: // CONSTANT
        yylhs.value.build< UnitInfo > ();
        break;

      case 105: // target_list
      case 107: // target_list2
        yylhs.value.build< VarList > ();
        break;

      case 3: // ONE
      case 4: // NUM
        yylhs.value.build< double > ();
        break;

      case 5: // IDENTIFIER
      case 9: // CELLADDRESS
      case 40: // STRING
      case 125: // module
      case 129: // id_or_cell
        yylhs.value.build< std::string > ();
        break;

      default:
        break;
    }



      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
      try
        {
          switch (yyn)
            {
  case 2:
#line 105 "ExpressionParser.y" // lalr1.cc:871
    { ctx.ScanResult = std::move(yystack_[0].value.as< ExpressionPtr > ()); ctx.valueExpression = true;                                       }
#line 3159 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 3:
#line 106 "ExpressionParser.y" // lalr1.cc:871
    { ctx.ScanResult = std::move(yystack_[0].value.as< ExpressionPtr > ()); ctx.unitExpression = true;                                        }
#line 3165 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 4:
#line 107 "ExpressionParser.y" // lalr1.cc:871
    { ctx.ScanResult = std::move(yystack_[1].value.as< ExpressionPtr > ()); ctx.unitExpression = true;                                        }
#line 3171 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 5:
#line 111 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ());                                                                        }
#line 3177 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 6:
#line 112 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[1].value.as< ExpressionPtr > ()), OP_UNIT, std::move(yystack_[0].value.as< ExpressionPtr > ()));  }
#line 3183 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 7:
#line 113 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3189 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 8:
#line 114 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = VariableExpression::create(ctx.obj, std::move(yystack_[0].value.as< ObjectIdentifier > ())); }
#line 3195 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 9:
#line 115 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3201 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 10:
#line 116 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3207 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 11:
#line 120 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = StringExpression::create(ctx.obj,std::move(yystack_[0].value.as< std::string > ())); }
#line 3213 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 12:
#line 121 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<StringExpression&>(*yystack_[1].value.as< ExpressionPtr > ()).append(std::move(yystack_[0].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3219 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 13:
#line 125 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = StringExpression::create(ctx.obj,std::move(yystack_[0].value.as< ExpressionString > ())); }
#line 3225 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 14:
#line 126 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<StringExpression&>(*yystack_[1].value.as< ExpressionPtr > ()).append(std::move(yystack_[0].value.as< ExpressionString > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3231 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 15:
#line 130 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3237 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 16:
#line 131 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3243 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 17:
#line 132 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3249 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 18:
#line 133 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3255 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 19:
#line 134 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3261 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 20:
#line 135 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3267 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 21:
#line 136 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = VariableExpression::create(ctx.obj,std::move(yystack_[1].value.as< ObjectIdentifier > ())); yylhs.value.as< ExpressionPtr > ()->addComponent(std::move(yystack_[0].value.as< ComponentPtr > ())); }
#line 3273 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 22:
#line 137 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[1].value.as< ExpressionPtr > ()->addComponent(std::move(yystack_[0].value.as< ComponentPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3279 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 23:
#line 138 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< ExpressionPtr > ()->addComponent(Expression::createComponent(yystack_[0].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 3285 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 24:
#line 142 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = CallableExpression::create(ctx.obj,std::move(yystack_[2].value.as< ObjectIdentifier > ())); }
#line 3291 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 25:
#line 143 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = CallableExpression::create(ctx.obj, std::move(yystack_[3].value.as< ObjectIdentifier > ()), std::move(yystack_[1].value.as< NamedArgumentList > ().first), std::move(yystack_[1].value.as< NamedArgumentList > ().second)); }
#line 3297 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 26:
#line 144 "ExpressionParser.y" // lalr1.cc:871
    {   // This rule exists because of possible name clash of 
                                                // function and unit, e.g. min
                                                yylhs.value.as< ExpressionPtr > () = CallableExpression::create(ctx.obj, std::move(yystack_[3].value.as< UnitInfo > ().first), std::move(yystack_[1].value.as< NamedArgumentList > ().first), std::move(yystack_[1].value.as< NamedArgumentList > ().second));
                                            }
#line 3306 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 27:
#line 148 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = CallableExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ())); }
#line 3312 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 28:
#line 149 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = CallableExpression::create(ctx.obj, std::move(yystack_[3].value.as< ExpressionPtr > ()), std::move(yystack_[1].value.as< NamedArgumentList > ().first), std::move(yystack_[1].value.as< NamedArgumentList > ().second)); }
#line 3318 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 29:
#line 150 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = CallableExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ())); }
#line 3324 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 30:
#line 151 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = CallableExpression::create(ctx.obj, std::move(yystack_[3].value.as< ExpressionPtr > ()), std::move(yystack_[1].value.as< NamedArgumentList > ().first), std::move(yystack_[1].value.as< NamedArgumentList > ().second)); }
#line 3330 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 31:
#line 152 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3336 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 32:
#line 156 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[1].value.as< ExpressionPtr > ()->addComponent(std::move(yystack_[0].value.as< ComponentPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3342 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 33:
#line 157 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< ExpressionPtr > ()->addComponent(Expression::createComponent(yystack_[0].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 3348 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 34:
#line 161 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3354 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 35:
#line 162 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ()), OP_NEG, NumberExpression::create(ctx.obj, Quantity(-1))); }
#line 3360 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 36:
#line 163 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ()), OP_NOT, NumberExpression::create(ctx.obj, Quantity(-1))); }
#line 3366 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 37:
#line 164 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ()), OP_POS, NumberExpression::create(ctx.obj, Quantity(1))); }
#line 3372 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 38:
#line 168 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3378 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 39:
#line 169 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_POW, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3384 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 40:
#line 170 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_POW2, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3390 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 41:
#line 173 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3396 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 42:
#line 174 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_MUL, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3402 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 43:
#line 175 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_DIV, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3408 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 44:
#line 176 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_DIV, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3414 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 45:
#line 177 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_MOD, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3420 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 46:
#line 181 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3426 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 47:
#line 182 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_ADD, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3432 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 48:
#line 183 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_SUB, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3438 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 49:
#line 187 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3444 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 50:
#line 188 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_LT, std::move(yystack_[0].value.as< ExpressionPtr > ()));    }
#line 3450 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 51:
#line 189 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_GT, std::move(yystack_[0].value.as< ExpressionPtr > ()));    }
#line 3456 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 52:
#line 190 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_GTE, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3462 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 53:
#line 191 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_LTE, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3468 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 54:
#line 195 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3474 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 55:
#line 196 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_EQ, std::move(yystack_[0].value.as< ExpressionPtr > ()));    }
#line 3480 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 56:
#line 197 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_NEQ, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3486 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 57:
#line 198 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_IS, std::move(yystack_[0].value.as< ExpressionPtr > ()));    }
#line 3492 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 58:
#line 199 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_IS_NOT, std::move(yystack_[0].value.as< ExpressionPtr > ()));    }
#line 3498 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 59:
#line 200 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_IN, std::move(yystack_[0].value.as< ExpressionPtr > ()));    }
#line 3504 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 60:
#line 201 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_NOT_IN, std::move(yystack_[0].value.as< ExpressionPtr > ()));    }
#line 3510 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 61:
#line 205 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3516 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 62:
#line 206 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_AND, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3522 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 63:
#line 210 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3528 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 64:
#line 211 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_OR, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3534 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 65:
#line 215 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3540 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 66:
#line 216 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3546 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 67:
#line 220 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3552 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 68:
#line 221 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ConditionalExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), std::move(yystack_[4].value.as< ExpressionPtr > ()), std::move(yystack_[0].value.as< ExpressionPtr > ()), true); }
#line 3558 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 69:
#line 222 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ConditionalExpression::create(ctx.obj, std::move(yystack_[4].value.as< ExpressionPtr > ()), std::move(yystack_[2].value.as< ExpressionPtr > ()), std::move(yystack_[0].value.as< ExpressionPtr > ()));                     }
#line 3564 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 70:
#line 226 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = LambdaExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3570 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 71:
#line 227 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = LambdaExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ()), std::move(yystack_[2].value.as< NamedArgumentList > ().first), std::move(yystack_[2].value.as< NamedArgumentList > ().second)); }
#line 3576 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 72:
#line 231 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = LambdaExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3582 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 73:
#line 232 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = LambdaExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ()), std::move(yystack_[2].value.as< NamedArgumentList > ().first), std::move(yystack_[2].value.as< NamedArgumentList > ().second)); }
#line 3588 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 74:
#line 236 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3594 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 75:
#line 237 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3600 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 76:
#line 240 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< StringList > ().push_back(std::move(yystack_[0].value.as< std::string > ())); }
#line 3606 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 77:
#line 241 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< StringList > ().push_back(std::move(yystack_[0].value.as< std::string > ())); yylhs.value.as< StringList > () = std::move(yystack_[2].value.as< StringList > ()); }
#line 3612 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 78:
#line 245 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3618 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 79:
#line 246 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = VariableExpression::create(ctx.obj,std::move(yystack_[0].value.as< ObjectIdentifier > ())); }
#line 3624 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 80:
#line 250 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< VarList > ().second=-1; yylhs.value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3630 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 81:
#line 251 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< VarList > ().second=0; yylhs.value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3636 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 82:
#line 252 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< VarList > () = std::move(yystack_[2].value.as< VarList > ()); }
#line 3642 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 83:
#line 253 "ExpressionParser.y" // lalr1.cc:871
    { 
                                               if(yystack_[3].value.as< VarList > ().second>=0) 
                                                   PARSER_THROW("Multiple catch all target"); 
                                               yystack_[3].value.as< VarList > ().second = (int)yystack_[3].value.as< VarList > ().first.size(); 
                                               yystack_[3].value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); 
                                               yylhs.value.as< VarList > () = std::move(yystack_[3].value.as< VarList > ()); 
                                            }
#line 3654 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 84:
#line 263 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3660 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 85:
#line 264 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3666 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 86:
#line 268 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< VarList > ().second=-1; yylhs.value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3672 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 87:
#line 269 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< VarList > ().second=0; yylhs.value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3678 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 88:
#line 270 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< VarList > () = std::move(yystack_[2].value.as< VarList > ()); }
#line 3684 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 89:
#line 271 "ExpressionParser.y" // lalr1.cc:871
    { 
                                               if(yystack_[3].value.as< VarList > ().second>=0) 
                                                   PARSER_THROW("Multiple catch all target"); 
                                               yystack_[3].value.as< VarList > ().second = (int)yystack_[3].value.as< VarList > ().first.size(); 
                                               yystack_[3].value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); 
                                               yylhs.value.as< VarList > () = std::move(yystack_[3].value.as< VarList > ()); 
                                            }
#line 3696 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 90:
#line 281 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,yystack_[2].value.as< VarList > ().second,std::move(yystack_[2].value.as< VarList > ().first),std::move(yystack_[0].value.as< ExpressionList > ())); }
#line 3702 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 91:
#line 285 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionList > ().push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3708 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 92:
#line 286 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< ExpressionList > ().push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionList > () = std::move(yystack_[2].value.as< ExpressionList > ()); }
#line 3714 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 93:
#line 290 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ()),OP_MUL); }
#line 3720 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 94:
#line 291 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ()),OP_POW); }
#line 3726 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 95:
#line 292 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ()),OP_DIV); }
#line 3732 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 96:
#line 293 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ()),OP_FDIV); }
#line 3738 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 97:
#line 294 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ()),OP_MOD); }
#line 3744 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 98:
#line 295 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ()),OP_ADD); }
#line 3750 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 99:
#line 296 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ()),OP_SUB); }
#line 3756 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 100:
#line 300 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3762 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 101:
#line 301 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3768 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 102:
#line 305 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3774 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 103:
#line 306 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3780 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 104:
#line 307 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = JumpStatement::create(ctx.obj,JUMP_RETURN,std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3786 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 105:
#line 308 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = JumpStatement::create(ctx.obj,JUMP_RETURN); }
#line 3792 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 106:
#line 309 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = JumpStatement::create(ctx.obj,JUMP_BREAK); }
#line 3798 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 107:
#line 310 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = JumpStatement::create(ctx.obj,JUMP_CONTINUE); }
#line 3804 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 108:
#line 311 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = JumpStatement::create(ctx.obj,JUMP_RAISE); }
#line 3810 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 109:
#line 312 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = JumpStatement::create(ctx.obj,JUMP_RAISE,std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3816 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 110:
#line 313 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = PseudoStatement::create(ctx.obj,PY_BEGIN); }
#line 3822 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 111:
#line 314 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = PseudoStatement::create(ctx.obj,PY_END); }
#line 3828 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 112:
#line 315 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = PseudoStatement::create(ctx.obj,PY_PASS); }
#line 3834 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 113:
#line 316 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = DelStatement::create(ctx.obj,std::move(yystack_[0].value.as< VarList > ().first)); }
#line 3840 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 114:
#line 317 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ScopeStatement::create(ctx.obj,std::move(yystack_[0].value.as< StringList > ())); }
#line 3846 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 115:
#line 318 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ScopeStatement::create(ctx.obj,std::move(yystack_[0].value.as< StringList > ()),false); }
#line 3852 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 116:
#line 319 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3858 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 117:
#line 320 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3864 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 118:
#line 321 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3870 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 119:
#line 325 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3876 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 120:
#line 326 "ExpressionParser.y" // lalr1.cc:871
    { 
                                               SimpleStatement *stmt = dynamic_cast<SimpleStatement*>(yystack_[2].value.as< ExpressionPtr > ().get()); 
                                               if(!stmt) {
                                                   yystack_[2].value.as< ExpressionPtr > () = SimpleStatement::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()));
                                                   stmt = static_cast<SimpleStatement*>(yystack_[2].value.as< ExpressionPtr > ().get());
                                               }
                                               stmt->add(std::move(yystack_[0].value.as< ExpressionPtr > ())); 
                                               yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ());
                                            }
#line 3890 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 121:
#line 335 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3896 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 122:
#line 339 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3902 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 123:
#line 340 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<IfStatement&>(*yystack_[3].value.as< ExpressionPtr > ()).addElse(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > ()= std::move(yystack_[3].value.as< ExpressionPtr > ()); }
#line 3908 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 124:
#line 341 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3914 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 125:
#line 342 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<WhileStatement&>(*yystack_[3].value.as< ExpressionPtr > ()).addElse(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > ()= std::move(yystack_[3].value.as< ExpressionPtr > ()); }
#line 3920 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 126:
#line 343 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3926 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 127:
#line 344 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ForStatement&>(*yystack_[3].value.as< ExpressionPtr > ()).addElse(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > ()= std::move(yystack_[3].value.as< ExpressionPtr > ()); }
#line 3932 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 128:
#line 345 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3938 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 129:
#line 346 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<TryStatement&>(*yystack_[0].value.as< ExpressionPtr > ()).check(); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3944 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 130:
#line 350 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = Statement::create(ctx.obj, std::move(yystack_[1].value.as< ExpressionPtr > ())); }
#line 3950 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 131:
#line 351 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = Statement::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3956 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 132:
#line 352 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<Statement&>(*yystack_[2].value.as< ExpressionPtr > ()).add(std::move(yystack_[1].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 3962 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 133:
#line 353 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<Statement&>(*yystack_[1].value.as< ExpressionPtr > ()).add(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3968 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 134:
#line 357 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3974 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 135:
#line 358 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3980 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 136:
#line 359 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3986 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 137:
#line 363 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3992 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 138:
#line 364 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3998 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 139:
#line 368 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = IfStatement::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 4004 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 140:
#line 369 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<IfStatement&>(*yystack_[4].value.as< ExpressionPtr > ()).add(std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 4010 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 141:
#line 373 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = WhileStatement::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 4016 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 142:
#line 377 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ForStatement::create(ctx.obj,yystack_[4].value.as< VarList > ().second,std::move(yystack_[4].value.as< VarList > ().first),std::move(yystack_[2].value.as< ExpressionList > ()),std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 4022 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 143:
#line 381 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = TryStatement::create(ctx.obj,std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 4028 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 144:
#line 382 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<TryStatement&>(*yystack_[3].value.as< ExpressionPtr > ()).add(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[3].value.as< ExpressionPtr > ()); }
#line 4034 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 145:
#line 383 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<TryStatement&>(*yystack_[4].value.as< ExpressionPtr > ()).add(std::move(yystack_[0].value.as< ExpressionPtr > ()),std::move(yystack_[2].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 4040 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 146:
#line 384 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<TryStatement&>(*yystack_[6].value.as< ExpressionPtr > ()).add(std::move(yystack_[0].value.as< ExpressionPtr > ()),std::move(yystack_[4].value.as< ExpressionPtr > ()),std::move(yystack_[2].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[6].value.as< ExpressionPtr > ()); }
#line 4046 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 147:
#line 385 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<TryStatement&>(*yystack_[3].value.as< ExpressionPtr > ()).addElse(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[3].value.as< ExpressionPtr > ()); }
#line 4052 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 148:
#line 386 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<TryStatement&>(*yystack_[3].value.as< ExpressionPtr > ()).addFinal(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[3].value.as< ExpressionPtr > ()); }
#line 4058 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 149:
#line 390 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = std::move(yystack_[0].value.as< std::string > ()); }
#line 4064 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 150:
#line 391 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = std::move(yystack_[2].value.as< std::string > ()); yylhs.value.as< NamedArgument > ().second = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 4070 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 151:
#line 392 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = "*"; yylhs.value.as< NamedArgument > ().first+=yystack_[0].value.as< std::string > (); }
#line 4076 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 152:
#line 393 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = "**"; yylhs.value.as< NamedArgument > ().first+=yystack_[0].value.as< std::string > (); }
#line 4082 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 153:
#line 397 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgumentList > ().first.push_back(std::move(yystack_[0].value.as< NamedArgument > ().first)); yylhs.value.as< NamedArgumentList > ().second.push_back(std::move(yystack_[0].value.as< NamedArgument > ().second)); }
#line 4088 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 154:
#line 398 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< NamedArgumentList > ().first.push_back(std::move(yystack_[0].value.as< NamedArgument > ().first)); yystack_[2].value.as< NamedArgumentList > ().second.push_back(std::move(yystack_[0].value.as< NamedArgument > ().second)); yylhs.value.as< NamedArgumentList > () = std::move(yystack_[2].value.as< NamedArgumentList > ()); }
#line 4094 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 155:
#line 402 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = FunctionStatement::create(ctx.obj, std::move(yystack_[4].value.as< std::string > ()), std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 4100 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 156:
#line 403 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = FunctionStatement::create(ctx.obj, std::move(yystack_[5].value.as< std::string > ()), std::move(yystack_[0].value.as< ExpressionPtr > ()), std::move(yystack_[3].value.as< NamedArgumentList > ().first), std::move(yystack_[3].value.as< NamedArgumentList > ().second)); }
#line 4106 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 157:
#line 407 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< std::string > () = std::move(yystack_[0].value.as< std::string > ()); }
#line 4112 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 158:
#line 408 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< std::string > () += "."; yylhs.value.as< std::string > () += yystack_[0].value.as< std::string > (); }
#line 4118 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 159:
#line 412 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ImportStatement::create(ctx.obj, std::move(yystack_[0].value.as< std::string > ())); }
#line 4124 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 160:
#line 413 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ImportStatement::create(ctx.obj, std::move(yystack_[2].value.as< std::string > ()), std::move(yystack_[0].value.as< std::string > ())); }
#line 4130 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 161:
#line 414 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ImportStatement&>(*yystack_[2].value.as< ExpressionPtr > ()).add(std::move(yystack_[0].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 4136 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 162:
#line 415 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ImportStatement&>(*yystack_[4].value.as< ExpressionPtr > ()).add(std::move(yystack_[2].value.as< std::string > ()), std::move(yystack_[0].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 4142 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 163:
#line 419 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = FromStatement::create(ctx.obj, std::move(yystack_[2].value.as< std::string > ()), std::move(yystack_[0].value.as< std::string > ())); }
#line 4148 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 164:
#line 420 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = FromStatement::create(ctx.obj, std::move(yystack_[4].value.as< std::string > ()), std::move(yystack_[2].value.as< std::string > ()), std::move(yystack_[2].value.as< std::string > ())); }
#line 4154 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 165:
#line 421 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<FromStatement&>(*yystack_[2].value.as< ExpressionPtr > ()).add(std::move(yystack_[0].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 4160 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 166:
#line 422 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<FromStatement&>(*yystack_[4].value.as< ExpressionPtr > ()).add(std::move(yystack_[2].value.as< std::string > ()), std::move(yystack_[0].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 4166 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 167:
#line 426 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = FromStatement::create(ctx.obj, std::move(yystack_[2].value.as< std::string > ()), std::string("*")); }
#line 4172 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 168:
#line 430 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< std::string > () = std::move(yystack_[0].value.as< std::string > ()); }
#line 4178 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 169:
#line 431 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< std::string > () = std::move(yystack_[0].value.as< std::string > ()); }
#line 4184 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 172:
#line 437 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< FlagExpression > ().first = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 4190 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 173:
#line 438 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< FlagExpression > ().first = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 4196 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 174:
#line 439 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< FlagExpression > ().first = std::move(yystack_[0].value.as< ExpressionPtr > ()); yylhs.value.as< FlagExpression > ().second = true; }
#line 4202 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 175:
#line 440 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< FlagExpression > ().first = std::move(yystack_[0].value.as< ExpressionPtr > ()); yylhs.value.as< FlagExpression > ().second = true; }
#line 4208 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 176:
#line 444 "ExpressionParser.y" // lalr1.cc:871
    { 
                                                yylhs.value.as< FlagExpressionList > ().first.push_back(std::move(yystack_[2].value.as< FlagExpression > ().first)); 
                                                yylhs.value.as< FlagExpressionList > ().second.push_back(yystack_[2].value.as< FlagExpression > ().second);
                                                yylhs.value.as< FlagExpressionList > ().first.push_back(std::move(yystack_[0].value.as< FlagExpression > ().first)); 
                                                yylhs.value.as< FlagExpressionList > ().second.push_back(yystack_[0].value.as< FlagExpression > ().second);
                                            }
#line 4219 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 177:
#line 450 "ExpressionParser.y" // lalr1.cc:871
    { 
                                                yystack_[2].value.as< FlagExpressionList > ().first.push_back(std::move(yystack_[0].value.as< FlagExpression > ().first)); 
                                                yystack_[2].value.as< FlagExpressionList > ().second.push_back(std::move(yystack_[0].value.as< FlagExpression > ().second)); 
                                                yylhs.value.as< FlagExpressionList > () = std::move(yystack_[2].value.as< FlagExpressionList > ()); 
                                            }
#line 4229 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 178:
#line 458 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = TupleExpression::create(ctx.obj); }
#line 4235 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 179:
#line 459 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = TupleExpression::create(ctx.obj, std::move(yystack_[2].value.as< FlagExpression > ().first), yystack_[2].value.as< FlagExpression > ().second); }
#line 4241 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 180:
#line 460 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = TupleExpression::create(ctx.obj, std::move(yystack_[1].value.as< FlagExpressionList > ().first), std::move(yystack_[1].value.as< FlagExpressionList > ().second)); }
#line 4247 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 181:
#line 461 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = TupleExpression::create(ctx.obj, std::move(yystack_[2].value.as< FlagExpressionList > ().first), std::move(yystack_[2].value.as< FlagExpressionList > ().second)); }
#line 4253 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 182:
#line 465 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< FlagExpressionList > ().first.push_back(std::move(yystack_[0].value.as< FlagExpression > ().first)); yylhs.value.as< FlagExpressionList > ().second.push_back(yystack_[0].value.as< FlagExpression > ().second); }
#line 4259 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 183:
#line 466 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< FlagExpressionList > ().first.push_back(std::move(yystack_[0].value.as< FlagExpression > ().first)); yystack_[2].value.as< FlagExpressionList > ().second.push_back(yystack_[0].value.as< FlagExpression > ().second); yylhs.value.as< FlagExpressionList > () = std::move(yystack_[2].value.as< FlagExpressionList > ()); }
#line 4265 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 184:
#line 470 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ListExpression::create(ctx.obj); }
#line 4271 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 185:
#line 471 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ListExpression::create(ctx.obj, std::move(yystack_[1].value.as< FlagExpressionList > ().first), std::move(yystack_[1].value.as< FlagExpressionList > ().second)); }
#line 4277 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 186:
#line 472 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ListExpression::create(ctx.obj, std::move(yystack_[2].value.as< FlagExpressionList > ().first), std::move(yystack_[2].value.as< FlagExpressionList > ().second)); }
#line 4283 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 187:
#line 473 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ComprehensionExpression&>(*yystack_[1].value.as< ExpressionPtr > ()).setExpr(std::move(yystack_[2].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 4289 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 188:
#line 477 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ComprehensionExpression::create(ctx.obj,yystack_[2].value.as< VarList > ().second,std::move(yystack_[2].value.as< VarList > ().first),std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 4295 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 189:
#line 478 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ComprehensionExpression&>(*yystack_[4].value.as< ExpressionPtr > ()).add(yystack_[2].value.as< VarList > ().second,std::move(yystack_[2].value.as< VarList > ().first),std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 4301 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 190:
#line 479 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ComprehensionExpression&>(*yystack_[2].value.as< ExpressionPtr > ()).addCond(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 4307 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 193:
#line 486 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = DictExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 4313 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 194:
#line 487 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = DictExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 4319 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 195:
#line 488 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<DictExpression&>(*yystack_[4].value.as< ExpressionPtr > ()).addItem(std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 4325 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 196:
#line 489 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<DictExpression&>(*yystack_[3].value.as< ExpressionPtr > ()).addItem(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[3].value.as< ExpressionPtr > ()); }
#line 4331 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 197:
#line 493 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = DictExpression::create(ctx.obj); }
#line 4337 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 198:
#line 494 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 4343 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 199:
#line 495 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 4349 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 200:
#line 496 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ComprehensionExpression&>(*yystack_[1].value.as< ExpressionPtr > ()).setExpr(std::move(yystack_[2].value.as< ExpressionPtr > ()),false); yylhs.value.as< ExpressionPtr > ()= std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 4355 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 201:
#line 497 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ComprehensionExpression&>(*yystack_[1].value.as< ExpressionPtr > ()).setExpr(std::move(yystack_[4].value.as< ExpressionPtr > ()),std::move(yystack_[2].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > ()= std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 4361 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 202:
#line 501 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = IDictExpression::create(ctx.obj, std::move(yystack_[2].value.as< std::string > ()), std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 4367 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 203:
#line 502 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = IDictExpression::create(ctx.obj, "**", std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 4373 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 204:
#line 503 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<IDictExpression&>(*yystack_[4].value.as< ExpressionPtr > ()).addItem(std::move(yystack_[2].value.as< std::string > ()),std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 4379 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 205:
#line 504 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<IDictExpression&>(*yystack_[4].value.as< ExpressionPtr > ()).addItem("**",std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 4385 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 206:
#line 508 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 4391 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 207:
#line 509 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 4397 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 208:
#line 513 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().second = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 4403 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 209:
#line 514 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = std::move(yystack_[2].value.as< std::string > ()); yylhs.value.as< NamedArgument > ().second = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 4409 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 210:
#line 515 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = "*"; yylhs.value.as< NamedArgument > ().second = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 4415 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 211:
#line 516 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().second = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 4421 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 212:
#line 517 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = "*"; yylhs.value.as< NamedArgument > ().second = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 4427 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 213:
#line 518 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = "**"; yylhs.value.as< NamedArgument > ().second = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 4433 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 214:
#line 522 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgumentList > ().first.push_back(std::move(yystack_[0].value.as< NamedArgument > ().first)); yylhs.value.as< NamedArgumentList > ().second.push_back(std::move(yystack_[0].value.as< NamedArgument > ().second)); }
#line 4439 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 215:
#line 523 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< NamedArgumentList > ().first.push_back(std::move(yystack_[0].value.as< NamedArgument > ().first)); yystack_[2].value.as< NamedArgumentList > ().second.push_back(std::move(yystack_[0].value.as< NamedArgument > ().second)); yylhs.value.as< NamedArgumentList > () = std::move(yystack_[2].value.as< NamedArgumentList > ()); }
#line 4445 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 216:
#line 527 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = NumberExpression::create(ctx.obj, std::move(yystack_[0].value.as< double > ()));                        }
#line 4451 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 217:
#line 528 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = NumberExpression::create(ctx.obj, std::move(yystack_[0].value.as< double > ()));                        }
#line 4457 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 218:
#line 529 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = NumberExpression::create(ctx.obj, (double)yystack_[0].value.as< Integer > ());                }
#line 4463 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 219:
#line 530 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ConstantExpression::create(ctx.obj, std::move(yystack_[0].value.as< UnitInfo > ().first), yystack_[0].value.as< UnitInfo > ().second);      }
#line 4469 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 220:
#line 534 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = RangeExpression::create(ctx.obj, std::move(yystack_[2].value.as< std::string > ()), std::move(yystack_[0].value.as< std::string > ()));                               }
#line 4475 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 221:
#line 538 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = UnitExpression::create(ctx.obj, yystack_[0].value.as< UnitInfo > ().second, std::move(yystack_[0].value.as< UnitInfo > ().first));                }
#line 4481 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 222:
#line 540 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_DIV, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 4487 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 223:
#line 541 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_MUL, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 4493 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 224:
#line 542 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_POW, NumberExpression::create(ctx.obj, Quantity((double)yystack_[0].value.as< Integer > ())));   }
#line 4499 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 225:
#line 543 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[3].value.as< ExpressionPtr > ()), OP_POW, OperatorExpression::create(ctx.obj, NumberExpression::create(ctx.obj, Quantity((double)yystack_[0].value.as< Integer > ())), OP_NEG, NumberExpression::create(ctx.obj, Quantity(-1))));   }
#line 4505 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 226:
#line 544 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ());                                                                        }
#line 4511 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 227:
#line 548 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ObjectIdentifier > () = ObjectIdentifier(ctx.obj); yylhs.value.as< ObjectIdentifier > () << ObjectIdentifier::SimpleComponent(yystack_[0].value.as< std::string > ()); }
#line 4517 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 228:
#line 549 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ObjectIdentifier > () = std::move(yystack_[0].value.as< ObjectIdentifier > ()); }
#line 4523 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 229:
#line 553 "ExpressionParser.y" // lalr1.cc:871
    { /* Path to property of a sub-object of the current object*/
                                                yylhs.value.as< ObjectIdentifier > () = ObjectIdentifier(ctx.obj,true);
                                                yylhs.value.as< ObjectIdentifier > ().setDocumentObjectName(ctx.obj,false,ObjectIdentifier::String(std::move(yystack_[2].value.as< std::string > ()),true),true);
                                                yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::SimpleComponent(yystack_[0].value.as< std::string > ()));
                                            }
#line 4533 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 230:
#line 558 "ExpressionParser.y" // lalr1.cc:871
    { /* Path to property of the current document object */
                                                yylhs.value.as< ObjectIdentifier > () = ObjectIdentifier(ctx.obj,true);
                                                yylhs.value.as< ObjectIdentifier > ().setDocumentObjectName(ctx.obj);
                                                yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::SimpleComponent(yystack_[0].value.as< std::string > ()));
                                            }
#line 4543 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 231:
#line 563 "ExpressionParser.y" // lalr1.cc:871
    { /* Path to property of a sub-object */
                                                yylhs.value.as< ObjectIdentifier > () = ObjectIdentifier(ctx.obj);
                                                yylhs.value.as< ObjectIdentifier > ().setDocumentObjectName(std::move(yystack_[4].value.as< ObjectIdentifier::String > ()), true, ObjectIdentifier::String(std::move(yystack_[2].value.as< std::string > ()),true),true);
                                                yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::SimpleComponent(yystack_[0].value.as< std::string > ()));
                                                yylhs.value.as< ObjectIdentifier > ().resolveAmbiguity();
                                            }
#line 4554 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 232:
#line 569 "ExpressionParser.y" // lalr1.cc:871
    { /* Path to property of a given document object */
                                                yylhs.value.as< ObjectIdentifier > () = ObjectIdentifier(ctx.obj);
                                                yystack_[2].value.as< ObjectIdentifier::String > ().checkImport(ctx.obj);
                                                yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::SimpleComponent(yystack_[2].value.as< ObjectIdentifier::String > ()));
                                                yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::SimpleComponent(yystack_[0].value.as< std::string > ()));
                                                yylhs.value.as< ObjectIdentifier > ().resolveAmbiguity();
                                            }
#line 4566 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 233:
#line 576 "ExpressionParser.y" // lalr1.cc:871
    { /* Path to property from an external document, within a named document object */
                                                yylhs.value.as< ObjectIdentifier > () = ObjectIdentifier(ctx.obj);
                                                yylhs.value.as< ObjectIdentifier > ().setDocumentName(std::move(yystack_[4].value.as< ObjectIdentifier::String > ()), true);
                                                yylhs.value.as< ObjectIdentifier > ().setDocumentObjectName(std::move(yystack_[2].value.as< ObjectIdentifier::String > ()), true);
                                                yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::SimpleComponent(yystack_[0].value.as< std::string > ()));
                                                yylhs.value.as< ObjectIdentifier > ().resolveAmbiguity();
                                            }
#line 4578 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 234:
#line 584 "ExpressionParser.y" // lalr1.cc:871
    {   yylhs.value.as< ObjectIdentifier > () = ObjectIdentifier(ctx.obj);
                                                yylhs.value.as< ObjectIdentifier > ().setDocumentName(std::move(yystack_[6].value.as< ObjectIdentifier::String > ()), true);
                                                yylhs.value.as< ObjectIdentifier > ().setDocumentObjectName(std::move(yystack_[4].value.as< ObjectIdentifier::String > ()), true, ObjectIdentifier::String(std::move(yystack_[2].value.as< std::string > ()),true));
                                                yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::SimpleComponent(yystack_[0].value.as< std::string > ()));
                                                yylhs.value.as< ObjectIdentifier > ().resolveAmbiguity();
                                            }
#line 4589 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 235:
#line 590 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ObjectIdentifier > ()= std::move(yystack_[2].value.as< ObjectIdentifier > ()); yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::SimpleComponent(yystack_[0].value.as< std::string > ())); }
#line 4595 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 236:
#line 594 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< Integer > () = std::move(yystack_[0].value.as< Integer > ()); }
#line 4601 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 237:
#line 595 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< Integer > () = std::move(yystack_[0].value.as< double > ()); }
#line 4607 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 238:
#line 599 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(std::move(yystack_[1].value.as< ExpressionPtr > ()));   }
#line 4613 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 239:
#line 600 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(std::move(yystack_[2].value.as< ExpressionPtr > ()),ExpressionPtr(),ExpressionPtr(),true); }
#line 4619 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 240:
#line 601 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(ExpressionPtr(),std::move(yystack_[1].value.as< ExpressionPtr > ())); }
#line 4625 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 241:
#line 602 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(ExpressionPtr(),ExpressionPtr(),std::move(yystack_[1].value.as< ExpressionPtr > ())); }
#line 4631 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 242:
#line 603 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(std::move(yystack_[3].value.as< ExpressionPtr > ()),std::move(yystack_[1].value.as< ExpressionPtr > ()));}
#line 4637 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 243:
#line 604 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(std::move(yystack_[4].value.as< ExpressionPtr > ()),ExpressionPtr(),std::move(yystack_[1].value.as< ExpressionPtr > ())); }
#line 4643 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 244:
#line 605 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(ExpressionPtr(),std::move(yystack_[3].value.as< ExpressionPtr > ()),std::move(yystack_[1].value.as< ExpressionPtr > ())); }
#line 4649 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 245:
#line 606 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(std::move(yystack_[5].value.as< ExpressionPtr > ()),std::move(yystack_[3].value.as< ExpressionPtr > ()),std::move(yystack_[1].value.as< ExpressionPtr > ()));}
#line 4655 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 246:
#line 610 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ObjectIdentifier::String > () = ObjectIdentifier::String(std::move(yystack_[0].value.as< std::string > ()), true); }
#line 4661 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 247:
#line 611 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ObjectIdentifier::String > () = ObjectIdentifier::String(std::move(yystack_[0].value.as< std::string > ()), false, true);}
#line 4667 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 248:
#line 615 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ObjectIdentifier::String > () = ObjectIdentifier::String(std::move(yystack_[0].value.as< std::string > ()), true); }
#line 4673 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 249:
#line 616 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ObjectIdentifier::String > () = ObjectIdentifier::String(std::move(yystack_[0].value.as< std::string > ()), false);}
#line 4679 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;


#line 4683 "ExpressionParser.tab.cc" // lalr1.cc:871
            default:
              break;
            }
        }
      catch (const syntax_error& yyexc)
        {
          error (yyexc);
          YYERROR;
        }
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;
      YY_STACK_PRINT ();

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, yylhs);
    }
    goto yynewstate;

  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        error (yysyntax_error_ (yystack_[0].state, yyla));
      }


    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.type_get () == yyeof_)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:

    /* Pacify compilers like GCC when the user code never invokes
       YYERROR and the label yyerrorlab therefore never appears in user
       code.  */
    if (false)
      goto yyerrorlab;
    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    goto yyerrlab1;

  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    {
      stack_symbol_type error_token;
      for (;;)
        {
          yyn = yypact_[yystack_[0].state];
          if (!yy_pact_value_is_default_ (yyn))
            {
              yyn += yyterror_;
              if (0 <= yyn && yyn <= yylast_ && yycheck_[yyn] == yyterror_)
                {
                  yyn = yytable_[yyn];
                  if (0 < yyn)
                    break;
                }
            }

          // Pop the current state because it cannot handle the error token.
          if (yystack_.size () == 1)
            YYABORT;

          yy_destroy_ ("Error: popping", yystack_[0]);
          yypop_ ();
          YY_STACK_PRINT ();
        }


      // Shift the error token.
      error_token.state = yyn;
      yypush_ ("Shifting", error_token);
    }
    goto yynewstate;

    // Accept.
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;

    // Abort.
  yyabortlab:
    yyresult = 1;
    goto yyreturn;

  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack"
                 << std::endl;
        // Do not try to display the values of the reclaimed symbols,
        // as their printer might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
  }

  void
  parser::error (const syntax_error& yyexc)
  {
    error (yyexc.what());
  }

  // Generate an error message.
  std::string
  parser::yysyntax_error_ (state_type yystate, const symbol_type& yyla) const
  {
    // Number of reported tokens (one for the "unexpected", one per
    // "expected").
    size_t yycount = 0;
    // Its maximum.
    enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
    // Arguments of yyformat.
    char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];

    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yyla) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yyla.  (However, yyla is currently not documented for users.)
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state
         merging (from LALR or IELR) and default reductions corrupt the
         expected token list.  However, the list is correct for
         canonical LR with one exception: it will still contain any
         token that will not be accepted due to an error action in a
         later state.
    */
    if (!yyla.empty ())
      {
        int yytoken = yyla.type_get ();
        yyarg[yycount++] = yytname_[yytoken];
        int yyn = yypact_[yystate];
        if (!yy_pact_value_is_default_ (yyn))
          {
            /* Start YYX at -YYN if negative to avoid negative indexes in
               YYCHECK.  In other words, skip the first -YYN actions for
               this state because they are default actions.  */
            int yyxbegin = yyn < 0 ? -yyn : 0;
            // Stay within bounds of both yycheck and yytname.
            int yychecklim = yylast_ - yyn + 1;
            int yyxend = yychecklim < yyntokens_ ? yychecklim : yyntokens_;
            for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
              if (yycheck_[yyx + yyn] == yyx && yyx != yyterror_
                  && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
                {
                  if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                    {
                      yycount = 1;
                      break;
                    }
                  else
                    yyarg[yycount++] = yytname_[yyx];
                }
          }
      }

    char const* yyformat = YY_NULLPTR;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
        YYCASE_(0, YY_("syntax error"));
        YYCASE_(1, YY_("syntax error, unexpected %s"));
        YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    std::string yyres;
    // Argument number.
    size_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += yytnamerr_ (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  const short int parser::yypact_ninf_ = -279;

  const short int parser::yytable_ninf_ = -250;

  const short int
  parser::yypact_[] =
  {
     766,  -279,  -279,   -47,    -8,  -279,  -279,  -279,  1930,   142,
      47,    67,    48,    67,  1930,  -279,   130,  1074,   142,   142,
     228,  -279,  -279,   142,  -279,  -279,   135,  -279,   228,   152,
     152,  1930,    87,  1339,    68,  1416,   604,   207,  -279,   172,
     183,   627,   210,   202,  -279,    56,   429,    81,   441,   382,
     211,    59,  -279,  -279,  -279,   594,  -279,   322,  -279,  -279,
    -279,  -279,    23,  -279,   920,  -279,   375,   189,   201,   198,
    -279,   214,   239,  -279,   245,  -279,  -279,   224,  -279,   261,
    -279,     9,   297,   219,   273,   277,   294,  1433,    -8,  1510,
     265,  -279,  -279,   218,  -279,   997,  -279,    30,   305,   396,
     142,   400,  -279,   195,     6,  -279,    12,   134,   347,   349,
     342,   521,   266,  -279,    79,   353,  -279,   348,   350,  -279,
     360,   360,  -279,   265,   210,  -279,  -279,   218,   142,  -279,
     365,   118,   232,   190,  -279,   252,  -279,   366,  -279,  -279,
     393,  -279,   249,   222,  -279,  -279,  -279,    80,  1715,  -279,
    -279,  -279,  1228,   438,  1776,  -279,  1245,   440,  -279,  1930,
    1930,  1930,  1957,  1930,  1930,  1930,  1930,  1930,  1930,  1930,
    1930,  1930,  1930,  1930,  1930,  1930,  1930,  1930,  1930,   142,
     142,   142,   142,   142,   142,   142,   142,   435,   142,  -279,
    1151,    40,  -279,   142,   384,   387,   389,  1803,   395,   401,
      67,   460,  -279,  -279,  -279,   688,  -279,    20,  -279,     9,
     403,     9,     9,   149,  1322,  -279,   471,   114,   129,   251,
     142,   142,  -279,  -279,   292,  -279,   443,    45,  -279,   482,
     484,   142,  -279,  -279,  -279,   142,    21,    37,  -279,  -279,
     997,   997,  -279,   142,   504,    26,   486,  -279,  -279,  -279,
     381,  1527,  -279,  1604,  -279,   381,   228,   165,  -279,  1621,
     142,   142,    92,   142,  -279,  -279,   302,  -279,  1853,    77,
    -279,   307,  -279,  -279,  -279,    56,    56,   403,    56,   429,
     429,    81,    81,    81,    81,   441,   441,   441,   441,   441,
     441,   382,   211,    10,   436,  -279,  -279,  -279,  -279,  -279,
    -279,  -279,    87,  -279,  -279,   423,  -279,  -279,   442,   997,
     997,   997,   997,    13,   997,   997,    32,   481,  -279,   445,
     142,   430,  -279,   448,   403,   403,  -279,  -279,   388,  -279,
    -279,   332,  -279,  -279,  -279,   437,   452,  -279,   142,  -279,
    -279,  -279,  -279,  1433,   920,  -279,  -279,  -279,  -279,  -279,
    -279,   489,  -279,  -279,  -279,   173,   521,  -279,   465,   344,
    -279,  -279,  -279,  -279,  -279,  -279,  -279,  -279,   109,  1880,
     228,  -279,  -279,  -279,  -279,   393,  -279,  -279,  -279,   142,
     123,  1698,  -279,  -279,   142,   142,  -279,   142,   997,  -279,
    -279,  -279,  -279,   523,   997,  -279,  -279,   526,   527,   142,
    -279,   142,   142,  -279,  -279,   154,   381,  -279,  -279,    23,
     843,   529,   997,  -279,   997,   473,  1930,    53,   519,  -279,
    -279,   115,   128,   459,   142,  -279,   142,  -279,   180,  -279,
    -279,  -279,  -279,   476,  -279,  -279,  -279,  -279,  -279,  -279,
     466,  -279,  -279,  -279,  -279,  -279,  -279,   997,   519,  1880,
     208,  1930,  -279,  -279,   462,   464,   142,  -279,   997,   381,
    -279,  -279,  1880,   519,  -279,  -279,   467,  -279,  -279,  -279,
    -279
  };

  const unsigned char
  parser::yydefact_[] =
  {
       0,   216,   217,   168,   221,   218,   219,   169,     0,   108,
       0,     0,     0,     0,     0,    13,    11,     0,     0,     0,
       0,   106,   107,   105,   110,   111,     0,   112,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    34,     7,
      20,     9,    10,    31,    38,    41,    46,    49,    54,    61,
      63,    67,    74,    75,   103,    84,    86,     0,   100,   101,
     102,   119,   134,   131,   136,     2,   122,   124,   126,   129,
     128,   116,   117,   118,   227,    16,    17,     0,    18,     0,
      19,     5,     3,     8,   228,     0,     0,     0,     0,     0,
       9,    31,    36,     8,   109,     0,   157,   159,   149,     0,
       0,     0,   153,     0,     0,    35,     0,     0,     0,     0,
     246,     0,    78,    80,     0,    79,   104,     0,   113,    76,
     114,   115,    37,    78,     0,    84,    87,    79,     0,   178,
     172,   227,     0,     0,   173,     0,   168,     0,   230,   184,
     172,   182,     0,   168,   192,   191,   197,     0,     0,     1,
      12,    14,     0,     0,     0,    22,     0,     0,    32,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   130,
     121,     0,   133,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   170,   171,   198,     0,   206,     0,   221,     0,
       6,     0,     0,     0,     0,    21,     0,     0,     0,   168,
       0,     0,   208,   214,     0,   211,     0,     0,   143,     0,
       0,     0,   152,    72,   151,     0,     0,     0,   135,     4,
       0,     0,    81,     0,     0,     0,     0,   174,   175,    15,
       0,     0,   180,     0,   226,     0,     0,     0,   185,     0,
       0,     0,     0,     0,   194,    27,     0,    23,     0,     0,
      29,     0,    33,    40,    39,    42,    43,    44,    45,    48,
      47,    50,    51,    52,    53,    55,    56,    57,    58,    60,
      59,    62,    64,     0,     0,    94,    93,    96,    95,    97,
      98,    99,     0,    88,    91,    90,   120,   132,     0,     0,
       0,     0,     0,     0,     0,     0,   161,   165,   199,     0,
       0,     0,   207,     0,   223,   222,   237,   236,     0,   224,
      24,     0,   235,   248,   249,     0,     0,   232,     0,   213,
     210,   212,    26,     0,     0,   137,   160,   158,   150,    73,
     154,   163,   167,   139,   141,     0,     0,    82,     0,     0,
      77,   220,   179,   172,   176,   181,   177,   229,     0,     0,
       0,   187,   186,   183,   202,   193,   200,   203,    28,     0,
       0,     0,   238,    30,     0,     0,    89,     0,     0,   123,
     125,   127,   144,     0,     0,   148,   147,     0,     0,     0,
     196,     0,     0,   225,    25,     0,     0,   209,   215,     0,
       0,     0,     0,    83,     0,     0,     0,     0,    65,   190,
      66,     0,     0,     0,     0,   240,     0,   239,     0,    68,
      69,    92,   140,     0,   145,   162,   166,   195,   204,   205,
       0,   233,   231,   138,   164,   142,   155,     0,   188,     0,
       0,     0,   201,   241,     0,     0,     0,   242,     0,     0,
     156,    70,     0,   189,   244,   243,     0,   146,   234,    71,
     245
  };

  const short int
  parser::yypgoto_[] =
  {
    -279,  -279,  -279,  -279,  -279,     0,   -22,    38,    15,   339,
     263,   293,   308,   371,   372,  -177,  -278,  -279,  -279,  -279,
     336,   518,    17,   -10,   -11,  -279,  -279,   310,  -279,  -279,
     361,     7,   -53,   215,  -279,   158,  -279,  -279,  -279,  -279,
     318,  -229,  -279,    -5,  -279,  -279,  -279,   -21,   -73,   -33,
    -279,  -279,  -279,  -279,  -144,  -164,  -279,  -279,  -279,  -279,
     212,  -108,  -279,   -78,     5,    19,  -279,   230,    -2,  -279,
     345
  };

  const short int
  parser::yydefgoto_[] =
  {
      -1,    37,    38,    39,    40,    90,    42,    91,    44,    45,
      46,    47,    48,    49,    50,    51,   419,    52,   420,    53,
      54,   120,    55,   114,    56,    57,    58,   305,    59,    60,
      61,   227,    63,    64,    65,   228,    66,    67,    68,    69,
     102,   103,    70,    97,    71,    72,    73,    74,   343,   132,
     133,    75,   142,    76,   257,   148,    77,    78,    79,    80,
     223,   224,    81,   134,   135,    93,    84,   329,   155,    85,
      86
  };

  const short int
  parser::yytable_[] =
  {
      41,   293,   141,   262,   205,    82,   207,    62,   104,   225,
     124,   192,   131,   138,   131,   208,   359,    41,   118,    83,
     112,   126,   107,    92,   106,   321,    98,   177,   112,   105,
     236,    98,   123,   237,  -247,   393,    83,   113,    43,   115,
     158,   320,   351,   323,   266,   113,   122,   115,   271,   125,
     248,   127,   229,    98,   397,    43,   238,   384,    98,   251,
     253,   144,    99,    87,    41,   189,   131,    99,   131,   259,
      43,   191,    96,   136,   225,   394,   177,     7,   225,   230,
     209,   215,   307,    83,   145,   101,   210,   345,   190,    99,
     101,   215,     3,    88,    99,    41,     7,   159,   358,   190,
     322,   352,    43,   230,   178,   230,   331,   131,   137,    95,
     100,   112,   101,   215,    83,   449,   190,   101,   164,   136,
     179,   190,   158,     7,   160,   215,    15,   110,   242,   256,
     115,   131,   243,    43,   136,   131,   225,   369,     7,   381,
     117,   370,   261,   341,   165,     1,     2,     3,    88,     5,
       6,     7,   326,   244,   333,   382,   327,   119,    89,   136,
      34,     8,   416,     7,    35,   124,    36,   277,   451,   336,
      12,   461,   376,   369,   273,   274,   303,   370,   239,    14,
     250,    15,    16,   244,   469,   424,   328,   123,   450,   244,
      41,  -249,   418,   131,   440,   316,   334,   337,   211,   212,
     131,   425,   213,  -248,   125,    31,   127,   149,   452,    83,
     369,  -246,   150,    89,   370,    34,   324,   325,   364,    35,
     366,    36,   151,   197,   198,    43,   373,   176,    43,   361,
     131,   422,   131,     3,   367,   412,   195,     7,   131,   448,
      41,    41,   456,   371,   112,   199,   368,   387,   196,   -79,
     -79,   -79,   -79,   -79,   -79,   -79,   112,   235,   457,    83,
      83,   357,   252,   115,   202,   225,   203,    15,   110,   202,
     462,   203,   418,   113,   463,   115,   -85,   -85,    43,    43,
     124,   156,   202,   157,   203,   418,   236,   154,   200,   214,
     214,   386,   111,   -79,   -79,   154,   154,   260,   202,    89,
     203,    34,   123,  -247,   204,    35,   202,    36,   203,    41,
      41,    41,    41,   201,    41,    41,   211,   212,  -249,   125,
     213,   127,   131,   202,   254,   203,   338,   258,    83,    83,
      83,    83,  -247,    83,    83,   202,   152,   203,   153,   153,
      43,   206,   154,   154,    41,    94,   216,    43,    43,    43,
      43,   409,    43,    43,   108,   109,   112,   192,   217,   116,
     421,   211,   212,    83,   342,   213,   202,   218,   203,   130,
     112,   140,   147,   413,   378,   115,   202,   236,   203,   383,
     231,   202,    43,   203,   441,   442,   136,   113,    41,   115,
       7,   326,   170,   171,    41,   327,   187,   188,   353,   354,
     172,   232,   173,   174,   404,   234,   202,    83,   203,   240,
      41,   241,    41,    83,    41,  -248,   415,   191,   202,   245,
     203,   193,   194,   222,   244,   130,    43,   279,   280,    83,
     154,    83,    43,    83,   246,   175,   233,   249,   468,   255,
       3,    88,   256,   267,     7,   272,   309,    41,    43,   310,
      43,   311,    43,   166,   167,   168,   169,   314,    41,   281,
     282,   283,   284,   315,   247,   317,    83,   389,   390,   391,
     392,   213,   395,   396,    15,   110,   332,    83,   285,   286,
     287,   288,   289,   290,   264,    43,   344,   346,   222,   347,
     269,   360,   222,   161,   162,   163,    43,   387,   385,   302,
     275,   276,   278,   398,   388,   401,    89,   399,    34,     3,
     405,   411,    35,     7,    36,   294,   295,   296,   297,   298,
     299,   300,   301,   402,   304,   406,     3,   414,   433,   308,
       7,   435,   436,   313,   444,   447,   177,   453,   458,   459,
     464,   319,   465,    15,   110,   470,   432,   291,   121,   292,
     222,   306,   434,   355,   350,   408,   339,   340,   403,   410,
      15,   110,   335,     0,     0,     0,     0,   348,   356,     0,
     445,   349,   446,     0,     0,    89,     0,    34,     0,   304,
       0,    35,     0,    36,     0,     0,     0,   363,     0,   363,
       0,     0,    89,     0,    34,   363,   374,   375,    35,   377,
      36,     0,     0,     0,   380,   460,     0,     1,     2,   143,
      88,     5,     6,     7,     0,     0,   467,     0,     0,     0,
       0,     0,     0,     8,   180,   181,   182,   183,   184,   185,
     186,     0,    12,     0,     0,     0,     0,     0,     0,     0,
       0,    14,     0,    15,    16,   144,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   400,   -78,   -78,   -78,
     -78,   -78,   -78,   -78,     0,     0,     0,    31,   145,     0,
       0,     0,     0,     0,   407,    89,     0,    34,     0,   222,
       0,    35,     0,    36,   146,     0,     0,     0,     0,     0,
       0,     1,     2,     3,    88,     5,     6,     7,   152,     0,
     153,   -78,   -78,     0,   154,     0,     0,     8,     0,     0,
       0,     0,     0,     0,     0,   423,    12,   428,     0,     0,
     429,   430,     0,   431,     0,    14,     0,    15,    16,   144,
       0,     0,     0,     0,     0,   437,     0,   438,   439,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    31,   145,     0,     0,     0,     0,     0,     0,    89,
     454,    34,   455,     0,     0,    35,     0,    36,   318,     1,
       2,     3,     4,     5,     6,     7,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     0,     0,     0,     9,
      10,     0,   466,    11,    12,    13,     0,     0,     0,     0,
       0,     0,     0,    14,     0,    15,    16,     0,     0,    17,
       0,    18,     0,     0,    19,    20,    21,    22,    23,     0,
      24,    25,    26,    27,    28,    29,    30,     0,     0,    31,
      32,     0,     0,     0,     0,     0,     0,    33,     0,    34,
       0,     0,     0,    35,     0,    36,     1,     2,     3,    88,
       5,     6,     7,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     8,     0,     0,     0,     9,    10,     0,     0,
      11,    12,    13,     0,     0,     0,     0,     0,     0,     0,
      14,     0,    15,    16,     0,     0,     0,   443,    18,     0,
       0,    19,    20,    21,    22,    23,     0,    24,    25,    26,
      27,    28,    29,    30,     0,     0,    31,    32,     0,     0,
       0,     0,     0,     0,    89,     0,    34,     0,     0,     0,
      35,     0,    36,     1,     2,     3,    88,     5,     6,     7,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     8,
       0,     0,     0,     9,    10,     0,     0,    11,    12,    13,
       0,     0,     0,     0,     0,     0,     0,    14,     0,    15,
      16,     0,     0,     0,     0,    18,     0,     0,    19,    20,
      21,    22,    23,     0,    24,    25,    26,    27,    28,    29,
      30,     0,     0,    31,    32,     0,     0,     0,     0,     0,
       0,    89,     0,    34,     0,     0,     0,    35,     0,    36,
       1,     2,     3,    88,     5,     6,     7,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     8,     0,     0,     0,
       9,     0,     0,     0,    11,    12,    13,     0,     0,     0,
       0,     0,     0,     0,    14,     0,    15,    16,     0,   226,
       0,     0,     0,     0,     0,     0,     0,    21,    22,    23,
       0,    24,    25,     0,    27,    28,    29,    30,     0,     0,
      31,    32,     0,     0,     0,     0,     0,     0,    89,     0,
      34,     0,     0,     0,    35,     0,    36,     1,     2,     3,
       4,     5,     6,     7,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     8,     0,     0,     0,     9,     0,     0,
       0,    11,    12,    13,     0,     0,     0,     0,     0,     0,
       0,    14,     0,    15,    16,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    21,    22,    23,     0,    24,    25,
       0,    27,    28,    29,    30,     0,     0,    31,    32,     0,
       0,     0,     0,     0,     0,    33,     0,    34,     0,     0,
       0,    35,     0,    36,     1,     2,     3,    88,     5,     6,
       7,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       8,     0,     0,     0,     9,     0,     0,     0,    11,    12,
      13,     0,     0,     0,     0,     0,     0,     0,    14,     0,
      15,    16,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    21,    22,    23,     0,    24,    25,     0,    27,    28,
      29,    30,     0,     0,    31,    32,     0,     0,     0,     0,
       0,     0,    89,     0,    34,     0,     0,     0,    35,     0,
      36,     1,     2,   219,    88,     5,     6,     7,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     8,     1,     2,
     219,    88,     5,     6,     7,     0,    12,     0,     0,     0,
       0,     0,     0,     0,     8,    14,     0,    15,    16,   220,
       0,     0,     0,    12,     0,     0,     0,     0,     0,     0,
       0,     0,    14,     0,    15,    16,   220,     0,     0,     0,
       0,    31,   221,     0,     0,     0,     0,     0,     0,    89,
     265,    34,     0,     0,     0,    35,     0,    36,    31,   221,
       0,     0,     0,     0,     0,     0,    89,   270,    34,     0,
       0,     0,    35,     0,    36,     1,     2,   219,    88,     5,
       6,     7,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     8,     1,     2,     3,     4,     5,     6,     7,     0,
      12,     0,     0,     0,     0,     0,     0,     0,     8,    14,
       0,    15,    16,   220,     0,     0,     0,    12,     0,     0,
       0,     0,     0,     0,     0,     0,    14,     0,    15,    16,
       0,     0,     0,     0,     0,    31,   221,     0,     0,     0,
       0,     0,     0,    89,   330,    34,     0,     0,     0,    35,
       0,    36,    31,   128,     0,     0,     0,     0,     0,     0,
      33,   129,    34,     0,     0,     0,    35,     0,    36,     1,
       2,     3,    88,     5,     6,     7,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     1,     2,   219,    88,
       5,     6,     7,     0,    12,     0,     0,     0,     0,     0,
       0,     0,     8,    14,     0,    15,    16,     0,     0,     0,
       0,    12,     0,     0,     0,     0,     0,     0,     0,     0,
      14,     0,    15,    16,   220,     0,     0,     0,     0,    31,
     128,     0,     0,     0,     0,     0,     0,    89,     0,    34,
       0,     0,     0,    35,   139,    36,    31,   221,     0,     0,
       0,     0,     0,     0,    89,     0,    34,     0,     0,     0,
      35,     0,    36,     1,     2,     3,    88,     5,     6,     7,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     8,
       1,     2,     3,    88,     5,     6,     7,     0,    12,     0,
       0,     0,     0,     0,     0,     0,     8,    14,     0,    15,
      16,     0,     0,     0,     0,    12,     0,     0,     0,     0,
       0,     0,     0,     0,    14,     0,    15,    16,     0,     0,
       0,     0,     0,    31,   128,     0,     0,     0,     0,     0,
       0,    89,   129,    34,     0,     0,     0,    35,     0,    36,
      31,   128,     0,     0,     0,     0,     0,     0,    89,   362,
      34,     0,     0,     0,    35,     0,    36,     1,     2,     3,
      88,     5,     6,     7,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     8,     1,     2,     3,    88,     5,     6,
       7,     0,    12,     0,     0,     0,     0,     0,     0,     0,
       8,    14,     0,    15,    16,     0,     0,     0,     0,    12,
       0,     0,     0,     0,     0,     0,     0,     0,    14,     0,
      15,    16,     0,     0,     0,     0,     0,    31,   128,     0,
       0,     0,     0,     0,     0,    89,   365,    34,     0,     0,
       0,    35,     0,    36,    31,   128,     0,     0,     0,     0,
       0,     0,    89,     0,    34,     0,     0,     0,    35,   372,
      36,     1,     2,     3,    88,     5,     6,     7,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     8,     1,     2,
       3,    88,     5,     6,     7,     0,    12,     0,     0,     0,
       0,     0,     0,     0,     8,    14,     0,    15,    16,     0,
       0,     0,     0,    12,     0,     0,     0,     0,     0,     0,
       0,     0,    14,     0,    15,    16,     0,     0,     0,     0,
     426,    31,     0,     0,     0,     0,     0,     0,     0,    89,
       0,    34,     0,     0,     0,    35,   427,    36,    31,     1,
       2,     3,    88,     5,     6,     7,    89,     0,    34,     0,
     263,     0,    35,     0,    36,     8,     0,     0,     0,     0,
       0,     0,     0,     0,    12,     0,     1,     2,     3,    88,
       5,     6,     7,    14,     0,    15,    16,     0,     0,     0,
       0,     0,     8,     0,     0,     0,     0,     0,     0,     0,
       0,    12,     0,     0,     0,     0,     0,     0,   268,    31,
      14,     0,    15,    16,     0,     0,     0,    89,     0,    34,
       0,     0,     0,    35,     0,    36,     1,     2,     3,    88,
       5,     6,     7,     0,     0,   312,    31,     0,     0,     0,
       0,     0,     8,     0,    89,     0,    34,     0,     0,     0,
      35,    12,    36,     1,     2,     3,    88,     5,     6,     7,
      14,     0,    15,    16,     0,     0,     0,     0,     0,     8,
       0,     0,     0,     0,     0,     0,     0,     0,   417,     0,
       0,     0,     0,     0,     0,   379,    31,    14,     0,    15,
      16,     0,     0,     0,    89,     0,    34,     0,     0,     0,
      35,     0,    36,     1,     2,     3,    88,     5,     6,     7,
       0,     0,     0,    31,     0,     0,     0,     0,     0,     8,
       0,    89,     0,    34,     0,     0,     0,    35,     0,    36,
       1,     2,     3,     4,     5,     6,     7,    14,     0,    15,
      16,     0,     0,     0,     0,     0,     8,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    31,    14,     0,    15,    16,     0,     0,
       0,    89,     0,    34,     0,     0,     0,    35,     0,    36,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      31,     0,     0,     0,     0,     0,     0,     0,    33,     0,
      34,     0,     0,     0,    35,     0,    36
  };

  const short int
  parser::yycheck_[] =
  {
       0,   178,    35,   147,    77,     0,    79,     0,    13,    87,
      32,    64,    33,    34,    35,     6,   245,    17,    28,     0,
      20,    32,    17,     8,    17,     5,     5,    17,    28,    14,
     103,     5,    32,    27,    81,    22,    17,    20,     0,    20,
      42,   205,     5,   207,   152,    28,    31,    28,   156,    32,
     128,    32,    22,     5,    22,    17,    44,    47,     5,   132,
     133,    41,    41,    71,    64,    42,    87,    41,    89,   142,
      32,    64,     5,     5,   152,    62,    17,     9,   156,    73,
      71,    83,    42,    64,    64,    64,    81,    42,    76,    41,
      64,    93,     5,     6,    41,    95,     9,    41,    72,    76,
      80,    64,    64,    73,    45,    73,   214,   128,    40,    62,
      62,   111,    64,   115,    95,    62,    76,    64,    37,     5,
      61,    76,   124,     9,    68,   127,    39,    40,   111,    49,
     111,   152,    53,    95,     5,   156,   214,    45,     9,    62,
       5,    49,    62,   221,    63,     3,     4,     5,     6,     7,
       8,     9,     3,    74,    40,    78,     7,     5,    71,     5,
      73,    19,    53,     9,    77,   187,    79,   162,    53,    40,
      28,   449,    80,    45,   159,   160,   187,    49,    44,    37,
      62,    39,    40,    74,   462,    62,    37,   187,   417,    74,
     190,    73,   369,   214,    40,   200,   217,   218,    64,    65,
     221,    78,    68,    73,   187,    63,   187,     0,    80,   190,
      45,    81,    40,    71,    49,    73,   211,   212,   251,    77,
     253,    79,    39,    25,    26,   187,   259,    16,   190,   250,
     251,   375,   253,     5,   255,    62,    47,     9,   259,   416,
     240,   241,    62,    78,   244,    47,   256,    74,    47,    30,
      31,    32,    33,    34,    35,    36,   256,    62,    78,   240,
     241,   244,    72,   244,    74,   343,    76,    39,    40,    74,
      62,    76,   449,   256,   451,   256,    74,    75,   240,   241,
     302,    71,    74,    73,    76,   462,   359,    77,    74,    71,
      71,   302,    64,    74,    75,    77,    77,    75,    74,    71,
      76,    73,   302,    81,    80,    77,    74,    79,    76,   309,
     310,   311,   312,    74,   314,   315,    64,    65,    73,   302,
      68,   302,   343,    74,    72,    76,    75,    78,   309,   310,
     311,   312,    81,   314,   315,    74,    71,    76,    73,    73,
     302,    80,    77,    77,   344,     9,    73,   309,   310,   311,
     312,   344,   314,   315,    18,    19,   356,   410,    81,    23,
     370,    64,    65,   344,    72,    68,    74,    73,    76,    33,
     370,    35,    36,   356,    72,   356,    74,   450,    76,    72,
      75,    74,   344,    76,   405,   406,     5,   370,   388,   370,
       9,     3,    10,    11,   394,     7,    74,    75,   240,   241,
      18,     5,    20,    21,    72,     5,    74,   388,    76,    62,
     410,    62,   412,   394,   414,    73,    72,   410,    74,    71,
      76,    46,    47,    87,    74,    89,   388,   164,   165,   410,
      77,   412,   394,   414,    74,    53,   100,    72,   459,    73,
       5,     6,    49,     5,     9,     5,    62,   447,   410,    62,
     412,    62,   414,    12,    13,    14,    15,    62,   458,   166,
     167,   168,   169,    62,   128,     5,   447,   309,   310,   311,
     312,    68,   314,   315,    39,    40,     5,   458,   170,   171,
     172,   173,   174,   175,   148,   447,    43,     5,   152,     5,
     154,     5,   156,    64,    65,    66,   458,    74,    62,    64,
     161,   162,   163,    22,    62,    75,    71,    62,    73,     5,
      73,    22,    77,     9,    79,   179,   180,   181,   182,   183,
     184,   185,   186,    75,   188,    73,     5,    62,     5,   193,
       9,     5,     5,   197,     5,    62,    17,    78,    62,    73,
      78,   205,    78,    39,    40,    78,   388,   176,    30,   177,
     214,   190,   394,   243,   236,   343,   220,   221,   328,   344,
      39,    40,   217,    -1,    -1,    -1,    -1,   231,    64,    -1,
     412,   235,   414,    -1,    -1,    71,    -1,    73,    -1,   243,
      -1,    77,    -1,    79,    -1,    -1,    -1,   251,    -1,   253,
      -1,    -1,    71,    -1,    73,   259,   260,   261,    77,   263,
      79,    -1,    -1,    -1,   268,   447,    -1,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,   458,    -1,    -1,    -1,
      -1,    -1,    -1,    19,    30,    31,    32,    33,    34,    35,
      36,    -1,    28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    37,    -1,    39,    40,    41,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   320,    30,    31,    32,
      33,    34,    35,    36,    -1,    -1,    -1,    63,    64,    -1,
      -1,    -1,    -1,    -1,   338,    71,    -1,    73,    -1,   343,
      -1,    77,    -1,    79,    80,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,     5,     6,     7,     8,     9,    71,    -1,
      73,    74,    75,    -1,    77,    -1,    -1,    19,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   379,    28,   381,    -1,    -1,
     384,   385,    -1,   387,    -1,    37,    -1,    39,    40,    41,
      -1,    -1,    -1,    -1,    -1,   399,    -1,   401,   402,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    63,    64,    -1,    -1,    -1,    -1,    -1,    -1,    71,
     424,    73,   426,    -1,    -1,    77,    -1,    79,    80,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,    23,
      24,    -1,   456,    27,    28,    29,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    37,    -1,    39,    40,    -1,    -1,    43,
      -1,    45,    -1,    -1,    48,    49,    50,    51,    52,    -1,
      54,    55,    56,    57,    58,    59,    60,    -1,    -1,    63,
      64,    -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    73,
      -1,    -1,    -1,    77,    -1,    79,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    19,    -1,    -1,    -1,    23,    24,    -1,    -1,
      27,    28,    29,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      37,    -1,    39,    40,    -1,    -1,    -1,    44,    45,    -1,
      -1,    48,    49,    50,    51,    52,    -1,    54,    55,    56,
      57,    58,    59,    60,    -1,    -1,    63,    64,    -1,    -1,
      -1,    -1,    -1,    -1,    71,    -1,    73,    -1,    -1,    -1,
      77,    -1,    79,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,
      -1,    -1,    -1,    23,    24,    -1,    -1,    27,    28,    29,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,    -1,    39,
      40,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    49,
      50,    51,    52,    -1,    54,    55,    56,    57,    58,    59,
      60,    -1,    -1,    63,    64,    -1,    -1,    -1,    -1,    -1,
      -1,    71,    -1,    73,    -1,    -1,    -1,    77,    -1,    79,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,
      23,    -1,    -1,    -1,    27,    28,    29,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    37,    -1,    39,    40,    -1,    42,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    50,    51,    52,
      -1,    54,    55,    -1,    57,    58,    59,    60,    -1,    -1,
      63,    64,    -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,
      73,    -1,    -1,    -1,    77,    -1,    79,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    19,    -1,    -1,    -1,    23,    -1,    -1,
      -1,    27,    28,    29,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    37,    -1,    39,    40,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    50,    51,    52,    -1,    54,    55,
      -1,    57,    58,    59,    60,    -1,    -1,    63,    64,    -1,
      -1,    -1,    -1,    -1,    -1,    71,    -1,    73,    -1,    -1,
      -1,    77,    -1,    79,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    -1,    -1,    -1,    23,    -1,    -1,    -1,    27,    28,
      29,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,    -1,
      39,    40,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    50,    51,    52,    -1,    54,    55,    -1,    57,    58,
      59,    60,    -1,    -1,    63,    64,    -1,    -1,    -1,    -1,
      -1,    -1,    71,    -1,    73,    -1,    -1,    -1,    77,    -1,
      79,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,     3,     4,
       5,     6,     7,     8,     9,    -1,    28,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    37,    -1,    39,    40,    41,
      -1,    -1,    -1,    28,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    37,    -1,    39,    40,    41,    -1,    -1,    -1,
      -1,    63,    64,    -1,    -1,    -1,    -1,    -1,    -1,    71,
      72,    73,    -1,    -1,    -1,    77,    -1,    79,    63,    64,
      -1,    -1,    -1,    -1,    -1,    -1,    71,    72,    73,    -1,
      -1,    -1,    77,    -1,    79,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    19,     3,     4,     5,     6,     7,     8,     9,    -1,
      28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    37,
      -1,    39,    40,    41,    -1,    -1,    -1,    28,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    37,    -1,    39,    40,
      -1,    -1,    -1,    -1,    -1,    63,    64,    -1,    -1,    -1,
      -1,    -1,    -1,    71,    72,    73,    -1,    -1,    -1,    77,
      -1,    79,    63,    64,    -1,    -1,    -1,    -1,    -1,    -1,
      71,    72,    73,    -1,    -1,    -1,    77,    -1,    79,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,     3,     4,     5,     6,
       7,     8,     9,    -1,    28,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    19,    37,    -1,    39,    40,    -1,    -1,    -1,
      -1,    28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      37,    -1,    39,    40,    41,    -1,    -1,    -1,    -1,    63,
      64,    -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    73,
      -1,    -1,    -1,    77,    78,    79,    63,    64,    -1,    -1,
      -1,    -1,    -1,    -1,    71,    -1,    73,    -1,    -1,    -1,
      77,    -1,    79,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,
       3,     4,     5,     6,     7,     8,     9,    -1,    28,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    37,    -1,    39,
      40,    -1,    -1,    -1,    -1,    28,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    37,    -1,    39,    40,    -1,    -1,
      -1,    -1,    -1,    63,    64,    -1,    -1,    -1,    -1,    -1,
      -1,    71,    72,    73,    -1,    -1,    -1,    77,    -1,    79,
      63,    64,    -1,    -1,    -1,    -1,    -1,    -1,    71,    72,
      73,    -1,    -1,    -1,    77,    -1,    79,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    19,     3,     4,     5,     6,     7,     8,
       9,    -1,    28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    37,    -1,    39,    40,    -1,    -1,    -1,    -1,    28,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,    -1,
      39,    40,    -1,    -1,    -1,    -1,    -1,    63,    64,    -1,
      -1,    -1,    -1,    -1,    -1,    71,    72,    73,    -1,    -1,
      -1,    77,    -1,    79,    63,    64,    -1,    -1,    -1,    -1,
      -1,    -1,    71,    -1,    73,    -1,    -1,    -1,    77,    78,
      79,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,     3,     4,
       5,     6,     7,     8,     9,    -1,    28,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    37,    -1,    39,    40,    -1,
      -1,    -1,    -1,    28,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    37,    -1,    39,    40,    -1,    -1,    -1,    -1,
      62,    63,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,
      -1,    73,    -1,    -1,    -1,    77,    78,    79,    63,     3,
       4,     5,     6,     7,     8,     9,    71,    -1,    73,    -1,
      75,    -1,    77,    -1,    79,    19,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    28,    -1,     3,     4,     5,     6,
       7,     8,     9,    37,    -1,    39,    40,    -1,    -1,    -1,
      -1,    -1,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    28,    -1,    -1,    -1,    -1,    -1,    -1,    62,    63,
      37,    -1,    39,    40,    -1,    -1,    -1,    71,    -1,    73,
      -1,    -1,    -1,    77,    -1,    79,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    62,    63,    -1,    -1,    -1,
      -1,    -1,    19,    -1,    71,    -1,    73,    -1,    -1,    -1,
      77,    28,    79,     3,     4,     5,     6,     7,     8,     9,
      37,    -1,    39,    40,    -1,    -1,    -1,    -1,    -1,    19,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    28,    -1,
      -1,    -1,    -1,    -1,    -1,    62,    63,    37,    -1,    39,
      40,    -1,    -1,    -1,    71,    -1,    73,    -1,    -1,    -1,
      77,    -1,    79,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    63,    -1,    -1,    -1,    -1,    -1,    19,
      -1,    71,    -1,    73,    -1,    -1,    -1,    77,    -1,    79,
       3,     4,     5,     6,     7,     8,     9,    37,    -1,    39,
      40,    -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    63,    37,    -1,    39,    40,    -1,    -1,
      -1,    71,    -1,    73,    -1,    -1,    -1,    77,    -1,    79,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      63,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,
      73,    -1,    -1,    -1,    77,    -1,    79
  };

  const unsigned char
  parser::yystos_[] =
  {
       0,     3,     4,     5,     6,     7,     8,     9,    19,    23,
      24,    27,    28,    29,    37,    39,    40,    43,    45,    48,
      49,    50,    51,    52,    54,    55,    56,    57,    58,    59,
      60,    63,    64,    71,    73,    77,    79,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    99,   101,   102,   104,   106,   107,   108,   110,
     111,   112,   113,   114,   115,   116,   118,   119,   120,   121,
     124,   126,   127,   128,   129,   133,   135,   138,   139,   140,
     141,   144,   146,   147,   148,   151,   152,    71,     6,    71,
      87,    89,    90,   147,   102,    62,     5,   125,     5,    41,
      62,    64,   122,   123,   125,    90,   113,   146,   102,   102,
      40,    64,    87,   104,   105,   147,   102,     5,   105,     5,
     103,   103,    90,    87,    88,   104,   106,   147,    64,    72,
     102,   129,   131,   132,   145,   146,     5,    40,   129,    78,
     102,   131,   134,     5,    41,    64,    80,   102,   137,     0,
      40,    39,    71,    73,    77,   150,    71,    73,   150,    41,
      68,    64,    65,    66,    37,    63,    12,    13,    14,    15,
      10,    11,    18,    20,    21,    53,    16,    17,    45,    61,
      30,    31,    32,    33,    34,    35,    36,    74,    75,    42,
      76,   113,   114,    46,    47,    47,    47,    25,    26,    47,
      74,    74,    74,    76,    80,   130,    80,   130,     6,    71,
     146,    64,    65,    68,    71,   150,    73,    81,    73,     5,
      41,    64,   102,   142,   143,   145,    42,   113,   117,    22,
      73,    75,     5,   102,     5,    62,   130,    27,    44,    44,
      62,    62,   104,    53,    74,    71,    74,   102,   145,    72,
      62,   130,    72,   130,    72,    73,    49,   136,    78,   130,
      75,    62,   136,    75,   102,    72,   143,     5,    62,   102,
      72,   143,     5,    90,    90,    91,    91,   146,    91,    92,
      92,    93,    93,    93,    93,    94,    94,    94,    94,    94,
      94,    95,    96,    97,   102,   102,   102,   102,   102,   102,
     102,   102,    64,   106,   102,   109,   112,    42,   102,    62,
      62,    62,    62,   102,    62,    62,   125,     5,    80,   102,
     137,     5,    80,   137,   146,   146,     3,     7,    37,   149,
      72,   143,     5,    40,   129,   152,    40,   129,    75,   102,
     102,   145,    72,   130,    43,    42,     5,     5,   102,   102,
     122,     5,    64,   117,   117,   109,    64,   104,    72,   123,
       5,   129,    72,   102,   131,    72,   131,   129,   105,    45,
      49,    78,    78,   131,   102,   102,    80,   102,    72,    62,
     102,    62,    78,    72,    47,    62,   106,    74,    62,   117,
     117,   117,   117,    22,    62,   117,   117,    22,    22,    62,
     102,    75,    75,   149,    72,    73,    73,   102,   142,   113,
     115,    22,    62,   104,    62,    72,    53,    28,    97,    98,
     100,   105,   136,   102,    62,    78,    62,    78,   102,   102,
     102,   102,   117,     5,   117,     5,     5,   102,   102,   102,
      40,   129,   129,    44,     5,   117,   117,    62,    97,    62,
     123,    53,    80,    78,   102,   102,    62,    78,    62,    73,
     117,    98,    62,    97,    78,    78,   102,   117,   129,    98,
      78
  };

  const unsigned char
  parser::yyr1_[] =
  {
       0,    82,    83,    83,    83,    84,    84,    84,    84,    84,
      84,    85,    85,    86,    86,    87,    87,    87,    87,    87,
      87,    87,    87,    87,    88,    88,    88,    88,    88,    88,
      88,    88,    89,    89,    90,    90,    90,    90,    91,    91,
      91,    92,    92,    92,    92,    92,    93,    93,    93,    94,
      94,    94,    94,    94,    95,    95,    95,    95,    95,    95,
      95,    96,    96,    97,    97,    98,    98,    99,    99,    99,
     100,   100,   101,   101,   102,   102,   103,   103,   104,   104,
     105,   105,   105,   105,   106,   106,   107,   107,   107,   107,
     108,   109,   109,   110,   110,   110,   110,   110,   110,   110,
     111,   111,   112,   112,   112,   112,   112,   112,   112,   112,
     112,   112,   112,   112,   112,   112,   112,   112,   112,   113,
     113,   113,   114,   114,   114,   114,   114,   114,   114,   114,
     115,   115,   115,   115,   116,   116,   116,   117,   117,   118,
     118,   119,   120,   121,   121,   121,   121,   121,   121,   122,
     122,   122,   122,   123,   123,   124,   124,   125,   125,   126,
     126,   126,   126,   127,   127,   127,   127,   128,   129,   129,
     130,   130,   131,   131,   131,   131,   132,   132,   133,   133,
     133,   133,   134,   134,   135,   135,   135,   135,   136,   136,
     136,   137,   137,   138,   138,   138,   138,   139,   139,   139,
     139,   139,   140,   140,   140,   140,   141,   141,   142,   142,
     142,   142,   142,   142,   143,   143,   144,   144,   144,   144,
     145,   146,   146,   146,   146,   146,   146,   147,   147,   148,
     148,   148,   148,   148,   148,   148,   149,   149,   150,   150,
     150,   150,   150,   150,   150,   150,   151,   151,   152,   152
  };

  const unsigned char
  parser::yyr2_[] =
  {
       0,     2,     1,     1,     3,     1,     2,     1,     1,     1,
       1,     1,     2,     1,     2,     3,     1,     1,     1,     1,
       1,     2,     2,     3,     3,     4,     4,     3,     4,     3,
       4,     1,     2,     3,     1,     2,     2,     2,     1,     3,
       3,     1,     3,     3,     3,     3,     1,     3,     3,     1,
       3,     3,     3,     3,     1,     3,     3,     3,     3,     3,
       3,     1,     3,     1,     3,     1,     1,     1,     5,     5,
       3,     4,     3,     4,     1,     1,     1,     3,     1,     1,
       1,     2,     3,     4,     1,     1,     1,     2,     3,     4,
       3,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       1,     1,     1,     1,     2,     1,     1,     1,     1,     2,
       1,     1,     1,     2,     2,     2,     1,     1,     1,     1,
       3,     2,     1,     4,     1,     4,     1,     4,     1,     1,
       2,     1,     3,     2,     1,     3,     1,     2,     4,     4,
       5,     4,     6,     3,     4,     5,     7,     4,     4,     1,
       3,     2,     2,     1,     3,     6,     7,     1,     3,     2,
       4,     3,     5,     4,     6,     3,     5,     4,     1,     1,
       1,     1,     1,     1,     2,     2,     3,     3,     2,     4,
       3,     4,     1,     3,     2,     3,     4,     4,     4,     5,
       3,     1,     1,     4,     3,     5,     4,     2,     2,     3,
       4,     6,     4,     4,     5,     5,     2,     3,     1,     3,
       2,     1,     2,     2,     1,     3,     1,     1,     1,     1,
       3,     1,     3,     3,     3,     4,     3,     1,     1,     4,
       2,     5,     3,     5,     7,     3,     1,     1,     3,     4,
       4,     5,     5,     6,     6,     7,     1,     1,     1,     1
  };



  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a yyntokens_, nonterminals.
  const char*
  const parser::yytname_[] =
  {
  "\"end of input\"", "error", "$undefined", "ONE", "NUM", "IDENTIFIER",
  "UNIT", "INTEGER", "CONSTANT", "CELLADDRESS", "EQ", "NEQ", "LT", "GT",
  "GTE", "LTE", "AND_OP", "OR_OP", "IS", "NOT", "IS_NOT", "NOT_IN", "AS",
  "RAISE", "TRY", "EXCEPT", "FINALLY", "IMPORT", "LAMBDA", "FROM",
  "POW_ASSIGN", "MUL_ASSIGN", "FDIV_ASSIGN", "DIV_ASSIGN", "MOD_ASSIGN",
  "ADD_ASSIGN", "SUB_ASSIGN", "MINUSSIGN", "FDIV", "PSTRING", "STRING",
  "EXPAND", "NEWLINE", "INDENT", "DEDENT", "IF", "ELIF", "ELSE", "WHILE",
  "FOR", "BREAK", "CONTINUE", "RETURN", "IN", "PY_BEGIN", "PY_END", "DEF",
  "PASS", "DEL", "GLOBAL", "NONLOCAL", "'?'", "':'", "'+'", "'*'", "'/'",
  "'%'", "NUM_AND_UNIT", "'^'", "NEG", "POS", "'('", "')'", "'.'", "','",
  "'='", "';'", "'['", "']'", "'{'", "'}'", "'#'", "$accept", "input",
  "primary_exp", "string", "pstring", "indexable", "callable",
  "indexable2", "unary_exp", "power_exp", "multiply_exp", "additive_exp",
  "relational_exp", "equality_exp", "and_exp", "or_exp", "nocond_exp",
  "cond_exp", "lambda_nocond_exp", "lambda_exp", "exp", "id_list",
  "target", "target_list", "target2", "target_list2", "assignment_exp1",
  "exp_list", "assignment_exp2", "assignment_exp", "small_stmt",
  "simple_stmt", "compound_stmt", "stmt", "statement", "suite", "if_stmt",
  "while_stmt", "for_stmt", "try_stmt", "arg_def", "arg_defs",
  "function_stmt", "module", "import_stmt1", "import_stmt2",
  "import_stmt3", "id_or_cell", "sep", "item", "items2", "tuple", "items",
  "list", "comp_for", "dict_expand", "dict1", "dict", "idict1", "idict",
  "arg", "args", "num", "range", "unit_exp", "identifier", "iden",
  "integer", "indexer", "document", "object", YY_NULLPTR
  };

#if YYDEBUG
  const unsigned short int
  parser::yyrline_[] =
  {
       0,   105,   105,   106,   107,   111,   112,   113,   114,   115,
     116,   120,   121,   125,   126,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   142,   143,   144,   148,   149,   150,
     151,   152,   156,   157,   161,   162,   163,   164,   168,   169,
     170,   173,   174,   175,   176,   177,   181,   182,   183,   187,
     188,   189,   190,   191,   195,   196,   197,   198,   199,   200,
     201,   205,   206,   210,   211,   215,   216,   220,   221,   222,
     226,   227,   231,   232,   236,   237,   240,   241,   245,   246,
     250,   251,   252,   253,   263,   264,   268,   269,   270,   271,
     281,   285,   286,   290,   291,   292,   293,   294,   295,   296,
     300,   301,   305,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,   317,   318,   319,   320,   321,   325,
     326,   335,   339,   340,   341,   342,   343,   344,   345,   346,
     350,   351,   352,   353,   357,   358,   359,   363,   364,   368,
     369,   373,   377,   381,   382,   383,   384,   385,   386,   390,
     391,   392,   393,   397,   398,   402,   403,   407,   408,   412,
     413,   414,   415,   419,   420,   421,   422,   426,   430,   431,
     434,   434,   437,   438,   439,   440,   444,   450,   458,   459,
     460,   461,   465,   466,   470,   471,   472,   473,   477,   478,
     479,   483,   483,   486,   487,   488,   489,   493,   494,   495,
     496,   497,   501,   502,   503,   504,   508,   509,   513,   514,
     515,   516,   517,   518,   522,   523,   527,   528,   529,   530,
     534,   538,   540,   541,   542,   543,   544,   548,   549,   553,
     558,   563,   569,   576,   583,   590,   594,   595,   599,   600,
     601,   602,   603,   604,   605,   606,   610,   611,   615,   616
  };

  // Print the state stack on the debug stream.
  void
  parser::yystack_print_ ()
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << i->state;
    *yycdebug_ << std::endl;
  }

  // Report on the debug stream that the rule \a yyrule is going to be reduced.
  void
  parser::yy_reduce_print_ (int yyrule)
  {
    unsigned int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):" << std::endl;
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG

  // Symbol number corresponding to token number t.
  inline
  parser::token_number_type
  parser::yytranslate_ (int t)
  {
    static
    const token_number_type
    translate_table[] =
    {
     0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    81,     2,    66,     2,     2,
      71,    72,    64,    63,    74,     2,    73,    65,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    62,    76,
       2,    75,     2,    61,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    77,     2,    78,    68,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    79,     2,    80,     2,     2,     2,     2,
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
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    67,    69,    70
    };
    const unsigned int user_token_number_max_ = 318;
    const token_number_type undef_token_ = 2;

    if (static_cast<int>(t) <= yyeof_)
      return yyeof_;
    else if (static_cast<unsigned int> (t) <= user_token_number_max_)
      return translate_table[t];
    else
      return undef_token_;
  }

#line 23 "ExpressionParser.y" // lalr1.cc:1179
} } // App::ExpressionParser
#line 5748 "ExpressionParser.tab.cc" // lalr1.cc:1179
#line 619 "ExpressionParser.y" // lalr1.cc:1180

