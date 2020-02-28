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

#include "ExpressionParser.tab.hh"

// User implementation prologue.

#line 54 "ExpressionParser.tab.cc" // lalr1.cc:422
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


#line 70 "ExpressionParser.tab.cc" // lalr1.cc:423


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
#line 137 "ExpressionParser.tab.cc" // lalr1.cc:489

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
      case 152: // indexer
        value.move< ComponentPtr > (other.value);
        break;

      case 8: // CONSTANT
        value.move< ConstantInfo > (other.value);
        break;

      case 111: // exp_list
        value.move< ExpressionList > (other.value);
        break;

      case 84: // input
      case 85: // uexp
      case 86: // primary_exp
      case 87: // string
      case 88: // pstring
      case 89: // indexable
      case 90: // callable
      case 91: // indexable2
      case 92: // unary_exp
      case 93: // power_exp
      case 94: // multiply_exp
      case 95: // additive_exp
      case 96: // relational_exp
      case 97: // equality_exp
      case 98: // and_exp
      case 99: // or_exp
      case 100: // nocond_exp
      case 101: // cond_exp
      case 102: // lambda_nocond_exp
      case 103: // lambda_exp
      case 104: // exp
      case 106: // target
      case 108: // target2
      case 110: // assignment_exp1
      case 112: // assignment_exp2
      case 113: // assignment_exp
      case 114: // small_stmt
      case 115: // simple_stmt
      case 116: // compound_stmt
      case 117: // stmt
      case 118: // statement
      case 119: // suite
      case 120: // if_stmt
      case 121: // while_stmt
      case 122: // for_stmt
      case 123: // try_stmt
      case 126: // function_stmt
      case 128: // import_stmt1
      case 129: // import_stmt2
      case 130: // import_stmt3
      case 135: // tuple
      case 137: // list
      case 138: // comp_for
      case 140: // dict1
      case 141: // dict
      case 142: // idict1
      case 143: // idict
      case 146: // num
      case 147: // range
      case 148: // unit_exp
        value.move< ExpressionPtr > (other.value);
        break;

      case 39: // PSTRING
        value.move< ExpressionString > (other.value);
        break;

      case 133: // item
        value.move< FlagExpression > (other.value);
        break;

      case 134: // items2
      case 136: // items
        value.move< FlagExpressionList > (other.value);
        break;

      case 7: // INTEGER
      case 151: // integer
        value.move< Integer > (other.value);
        break;

      case 124: // arg_def
      case 144: // arg
        value.move< NamedArgument > (other.value);
        break;

      case 125: // arg_defs
      case 145: // args
        value.move< NamedArgumentList > (other.value);
        break;

      case 149: // identifier
      case 150: // iden
        value.move< ObjectIdentifier > (other.value);
        break;

      case 153: // document
      case 154: // object
        value.move< ObjectIdentifier::String > (other.value);
        break;

      case 105: // id_list
        value.move< StringList > (other.value);
        break;

      case 107: // target_list
      case 109: // target_list2
        value.move< VarList > (other.value);
        break;

      case 3: // ONE
      case 4: // NUM
        value.move< double > (other.value);
        break;

      case 5: // IDENTIFIER
      case 6: // UNIT
      case 9: // CELLADDRESS
      case 40: // STRING
      case 127: // module
      case 131: // id_or_cell
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
      case 152: // indexer
        value.move< ComponentPtr > (v);
        break;

      case 8: // CONSTANT
        value.move< ConstantInfo > (v);
        break;

      case 111: // exp_list
        value.move< ExpressionList > (v);
        break;

      case 84: // input
      case 85: // uexp
      case 86: // primary_exp
      case 87: // string
      case 88: // pstring
      case 89: // indexable
      case 90: // callable
      case 91: // indexable2
      case 92: // unary_exp
      case 93: // power_exp
      case 94: // multiply_exp
      case 95: // additive_exp
      case 96: // relational_exp
      case 97: // equality_exp
      case 98: // and_exp
      case 99: // or_exp
      case 100: // nocond_exp
      case 101: // cond_exp
      case 102: // lambda_nocond_exp
      case 103: // lambda_exp
      case 104: // exp
      case 106: // target
      case 108: // target2
      case 110: // assignment_exp1
      case 112: // assignment_exp2
      case 113: // assignment_exp
      case 114: // small_stmt
      case 115: // simple_stmt
      case 116: // compound_stmt
      case 117: // stmt
      case 118: // statement
      case 119: // suite
      case 120: // if_stmt
      case 121: // while_stmt
      case 122: // for_stmt
      case 123: // try_stmt
      case 126: // function_stmt
      case 128: // import_stmt1
      case 129: // import_stmt2
      case 130: // import_stmt3
      case 135: // tuple
      case 137: // list
      case 138: // comp_for
      case 140: // dict1
      case 141: // dict
      case 142: // idict1
      case 143: // idict
      case 146: // num
      case 147: // range
      case 148: // unit_exp
        value.move< ExpressionPtr > (v);
        break;

      case 39: // PSTRING
        value.move< ExpressionString > (v);
        break;

      case 133: // item
        value.move< FlagExpression > (v);
        break;

      case 134: // items2
      case 136: // items
        value.move< FlagExpressionList > (v);
        break;

      case 7: // INTEGER
      case 151: // integer
        value.move< Integer > (v);
        break;

      case 124: // arg_def
      case 144: // arg
        value.move< NamedArgument > (v);
        break;

      case 125: // arg_defs
      case 145: // args
        value.move< NamedArgumentList > (v);
        break;

      case 149: // identifier
      case 150: // iden
        value.move< ObjectIdentifier > (v);
        break;

      case 153: // document
      case 154: // object
        value.move< ObjectIdentifier::String > (v);
        break;

      case 105: // id_list
        value.move< StringList > (v);
        break;

      case 107: // target_list
      case 109: // target_list2
        value.move< VarList > (v);
        break;

      case 3: // ONE
      case 4: // NUM
        value.move< double > (v);
        break;

      case 5: // IDENTIFIER
      case 6: // UNIT
      case 9: // CELLADDRESS
      case 40: // STRING
      case 127: // module
      case 131: // id_or_cell
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
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, ConstantInfo&& v)
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
      case 152: // indexer
        value.template destroy< ComponentPtr > ();
        break;

      case 8: // CONSTANT
        value.template destroy< ConstantInfo > ();
        break;

      case 111: // exp_list
        value.template destroy< ExpressionList > ();
        break;

      case 84: // input
      case 85: // uexp
      case 86: // primary_exp
      case 87: // string
      case 88: // pstring
      case 89: // indexable
      case 90: // callable
      case 91: // indexable2
      case 92: // unary_exp
      case 93: // power_exp
      case 94: // multiply_exp
      case 95: // additive_exp
      case 96: // relational_exp
      case 97: // equality_exp
      case 98: // and_exp
      case 99: // or_exp
      case 100: // nocond_exp
      case 101: // cond_exp
      case 102: // lambda_nocond_exp
      case 103: // lambda_exp
      case 104: // exp
      case 106: // target
      case 108: // target2
      case 110: // assignment_exp1
      case 112: // assignment_exp2
      case 113: // assignment_exp
      case 114: // small_stmt
      case 115: // simple_stmt
      case 116: // compound_stmt
      case 117: // stmt
      case 118: // statement
      case 119: // suite
      case 120: // if_stmt
      case 121: // while_stmt
      case 122: // for_stmt
      case 123: // try_stmt
      case 126: // function_stmt
      case 128: // import_stmt1
      case 129: // import_stmt2
      case 130: // import_stmt3
      case 135: // tuple
      case 137: // list
      case 138: // comp_for
      case 140: // dict1
      case 141: // dict
      case 142: // idict1
      case 143: // idict
      case 146: // num
      case 147: // range
      case 148: // unit_exp
        value.template destroy< ExpressionPtr > ();
        break;

      case 39: // PSTRING
        value.template destroy< ExpressionString > ();
        break;

      case 133: // item
        value.template destroy< FlagExpression > ();
        break;

      case 134: // items2
      case 136: // items
        value.template destroy< FlagExpressionList > ();
        break;

      case 7: // INTEGER
      case 151: // integer
        value.template destroy< Integer > ();
        break;

      case 124: // arg_def
      case 144: // arg
        value.template destroy< NamedArgument > ();
        break;

      case 125: // arg_defs
      case 145: // args
        value.template destroy< NamedArgumentList > ();
        break;

      case 149: // identifier
      case 150: // iden
        value.template destroy< ObjectIdentifier > ();
        break;

      case 153: // document
      case 154: // object
        value.template destroy< ObjectIdentifier::String > ();
        break;

      case 105: // id_list
        value.template destroy< StringList > ();
        break;

      case 107: // target_list
      case 109: // target_list2
        value.template destroy< VarList > ();
        break;

      case 3: // ONE
      case 4: // NUM
        value.template destroy< double > ();
        break;

      case 5: // IDENTIFIER
      case 6: // UNIT
      case 9: // CELLADDRESS
      case 40: // STRING
      case 127: // module
      case 131: // id_or_cell
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
      case 152: // indexer
        value.move< ComponentPtr > (s.value);
        break;

      case 8: // CONSTANT
        value.move< ConstantInfo > (s.value);
        break;

      case 111: // exp_list
        value.move< ExpressionList > (s.value);
        break;

      case 84: // input
      case 85: // uexp
      case 86: // primary_exp
      case 87: // string
      case 88: // pstring
      case 89: // indexable
      case 90: // callable
      case 91: // indexable2
      case 92: // unary_exp
      case 93: // power_exp
      case 94: // multiply_exp
      case 95: // additive_exp
      case 96: // relational_exp
      case 97: // equality_exp
      case 98: // and_exp
      case 99: // or_exp
      case 100: // nocond_exp
      case 101: // cond_exp
      case 102: // lambda_nocond_exp
      case 103: // lambda_exp
      case 104: // exp
      case 106: // target
      case 108: // target2
      case 110: // assignment_exp1
      case 112: // assignment_exp2
      case 113: // assignment_exp
      case 114: // small_stmt
      case 115: // simple_stmt
      case 116: // compound_stmt
      case 117: // stmt
      case 118: // statement
      case 119: // suite
      case 120: // if_stmt
      case 121: // while_stmt
      case 122: // for_stmt
      case 123: // try_stmt
      case 126: // function_stmt
      case 128: // import_stmt1
      case 129: // import_stmt2
      case 130: // import_stmt3
      case 135: // tuple
      case 137: // list
      case 138: // comp_for
      case 140: // dict1
      case 141: // dict
      case 142: // idict1
      case 143: // idict
      case 146: // num
      case 147: // range
      case 148: // unit_exp
        value.move< ExpressionPtr > (s.value);
        break;

      case 39: // PSTRING
        value.move< ExpressionString > (s.value);
        break;

      case 133: // item
        value.move< FlagExpression > (s.value);
        break;

      case 134: // items2
      case 136: // items
        value.move< FlagExpressionList > (s.value);
        break;

      case 7: // INTEGER
      case 151: // integer
        value.move< Integer > (s.value);
        break;

      case 124: // arg_def
      case 144: // arg
        value.move< NamedArgument > (s.value);
        break;

      case 125: // arg_defs
      case 145: // args
        value.move< NamedArgumentList > (s.value);
        break;

      case 149: // identifier
      case 150: // iden
        value.move< ObjectIdentifier > (s.value);
        break;

      case 153: // document
      case 154: // object
        value.move< ObjectIdentifier::String > (s.value);
        break;

      case 105: // id_list
        value.move< StringList > (s.value);
        break;

      case 107: // target_list
      case 109: // target_list2
        value.move< VarList > (s.value);
        break;

      case 3: // ONE
      case 4: // NUM
        value.move< double > (s.value);
        break;

      case 5: // IDENTIFIER
      case 6: // UNIT
      case 9: // CELLADDRESS
      case 40: // STRING
      case 127: // module
      case 131: // id_or_cell
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
  parser::make_UNIT (std::string&& v)
  {
    return symbol_type (token::TOK_UNIT, std::move (v));
  }

  parser::symbol_type
  parser::make_INTEGER (Integer&& v)
  {
    return symbol_type (token::TOK_INTEGER, std::move (v));
  }

  parser::symbol_type
  parser::make_CONSTANT (ConstantInfo&& v)
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
  parser::make_NEG ()
  {
    return symbol_type (token::TOK_NEG);
  }

  parser::symbol_type
  parser::make_POS ()
  {
    return symbol_type (token::TOK_POS);
  }

  parser::symbol_type
  parser::make_NUM_AND_UNIT ()
  {
    return symbol_type (token::TOK_NUM_AND_UNIT);
  }

  parser::symbol_type
  parser::make_NUM_DIV_UNIT ()
  {
    return symbol_type (token::TOK_NUM_DIV_UNIT);
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
      case 152: // indexer
        value.move< ComponentPtr > (that.value);
        break;

      case 8: // CONSTANT
        value.move< ConstantInfo > (that.value);
        break;

      case 111: // exp_list
        value.move< ExpressionList > (that.value);
        break;

      case 84: // input
      case 85: // uexp
      case 86: // primary_exp
      case 87: // string
      case 88: // pstring
      case 89: // indexable
      case 90: // callable
      case 91: // indexable2
      case 92: // unary_exp
      case 93: // power_exp
      case 94: // multiply_exp
      case 95: // additive_exp
      case 96: // relational_exp
      case 97: // equality_exp
      case 98: // and_exp
      case 99: // or_exp
      case 100: // nocond_exp
      case 101: // cond_exp
      case 102: // lambda_nocond_exp
      case 103: // lambda_exp
      case 104: // exp
      case 106: // target
      case 108: // target2
      case 110: // assignment_exp1
      case 112: // assignment_exp2
      case 113: // assignment_exp
      case 114: // small_stmt
      case 115: // simple_stmt
      case 116: // compound_stmt
      case 117: // stmt
      case 118: // statement
      case 119: // suite
      case 120: // if_stmt
      case 121: // while_stmt
      case 122: // for_stmt
      case 123: // try_stmt
      case 126: // function_stmt
      case 128: // import_stmt1
      case 129: // import_stmt2
      case 130: // import_stmt3
      case 135: // tuple
      case 137: // list
      case 138: // comp_for
      case 140: // dict1
      case 141: // dict
      case 142: // idict1
      case 143: // idict
      case 146: // num
      case 147: // range
      case 148: // unit_exp
        value.move< ExpressionPtr > (that.value);
        break;

      case 39: // PSTRING
        value.move< ExpressionString > (that.value);
        break;

      case 133: // item
        value.move< FlagExpression > (that.value);
        break;

      case 134: // items2
      case 136: // items
        value.move< FlagExpressionList > (that.value);
        break;

      case 7: // INTEGER
      case 151: // integer
        value.move< Integer > (that.value);
        break;

      case 124: // arg_def
      case 144: // arg
        value.move< NamedArgument > (that.value);
        break;

      case 125: // arg_defs
      case 145: // args
        value.move< NamedArgumentList > (that.value);
        break;

      case 149: // identifier
      case 150: // iden
        value.move< ObjectIdentifier > (that.value);
        break;

      case 153: // document
      case 154: // object
        value.move< ObjectIdentifier::String > (that.value);
        break;

      case 105: // id_list
        value.move< StringList > (that.value);
        break;

      case 107: // target_list
      case 109: // target_list2
        value.move< VarList > (that.value);
        break;

      case 3: // ONE
      case 4: // NUM
        value.move< double > (that.value);
        break;

      case 5: // IDENTIFIER
      case 6: // UNIT
      case 9: // CELLADDRESS
      case 40: // STRING
      case 127: // module
      case 131: // id_or_cell
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
      case 152: // indexer
        value.copy< ComponentPtr > (that.value);
        break;

      case 8: // CONSTANT
        value.copy< ConstantInfo > (that.value);
        break;

      case 111: // exp_list
        value.copy< ExpressionList > (that.value);
        break;

      case 84: // input
      case 85: // uexp
      case 86: // primary_exp
      case 87: // string
      case 88: // pstring
      case 89: // indexable
      case 90: // callable
      case 91: // indexable2
      case 92: // unary_exp
      case 93: // power_exp
      case 94: // multiply_exp
      case 95: // additive_exp
      case 96: // relational_exp
      case 97: // equality_exp
      case 98: // and_exp
      case 99: // or_exp
      case 100: // nocond_exp
      case 101: // cond_exp
      case 102: // lambda_nocond_exp
      case 103: // lambda_exp
      case 104: // exp
      case 106: // target
      case 108: // target2
      case 110: // assignment_exp1
      case 112: // assignment_exp2
      case 113: // assignment_exp
      case 114: // small_stmt
      case 115: // simple_stmt
      case 116: // compound_stmt
      case 117: // stmt
      case 118: // statement
      case 119: // suite
      case 120: // if_stmt
      case 121: // while_stmt
      case 122: // for_stmt
      case 123: // try_stmt
      case 126: // function_stmt
      case 128: // import_stmt1
      case 129: // import_stmt2
      case 130: // import_stmt3
      case 135: // tuple
      case 137: // list
      case 138: // comp_for
      case 140: // dict1
      case 141: // dict
      case 142: // idict1
      case 143: // idict
      case 146: // num
      case 147: // range
      case 148: // unit_exp
        value.copy< ExpressionPtr > (that.value);
        break;

      case 39: // PSTRING
        value.copy< ExpressionString > (that.value);
        break;

      case 133: // item
        value.copy< FlagExpression > (that.value);
        break;

      case 134: // items2
      case 136: // items
        value.copy< FlagExpressionList > (that.value);
        break;

      case 7: // INTEGER
      case 151: // integer
        value.copy< Integer > (that.value);
        break;

      case 124: // arg_def
      case 144: // arg
        value.copy< NamedArgument > (that.value);
        break;

      case 125: // arg_defs
      case 145: // args
        value.copy< NamedArgumentList > (that.value);
        break;

      case 149: // identifier
      case 150: // iden
        value.copy< ObjectIdentifier > (that.value);
        break;

      case 153: // document
      case 154: // object
        value.copy< ObjectIdentifier::String > (that.value);
        break;

      case 105: // id_list
        value.copy< StringList > (that.value);
        break;

      case 107: // target_list
      case 109: // target_list2
        value.copy< VarList > (that.value);
        break;

      case 3: // ONE
      case 4: // NUM
        value.copy< double > (that.value);
        break;

      case 5: // IDENTIFIER
      case 6: // UNIT
      case 9: // CELLADDRESS
      case 40: // STRING
      case 127: // module
      case 131: // id_or_cell
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
      case 152: // indexer
        yylhs.value.build< ComponentPtr > ();
        break;

      case 8: // CONSTANT
        yylhs.value.build< ConstantInfo > ();
        break;

      case 111: // exp_list
        yylhs.value.build< ExpressionList > ();
        break;

      case 84: // input
      case 85: // uexp
      case 86: // primary_exp
      case 87: // string
      case 88: // pstring
      case 89: // indexable
      case 90: // callable
      case 91: // indexable2
      case 92: // unary_exp
      case 93: // power_exp
      case 94: // multiply_exp
      case 95: // additive_exp
      case 96: // relational_exp
      case 97: // equality_exp
      case 98: // and_exp
      case 99: // or_exp
      case 100: // nocond_exp
      case 101: // cond_exp
      case 102: // lambda_nocond_exp
      case 103: // lambda_exp
      case 104: // exp
      case 106: // target
      case 108: // target2
      case 110: // assignment_exp1
      case 112: // assignment_exp2
      case 113: // assignment_exp
      case 114: // small_stmt
      case 115: // simple_stmt
      case 116: // compound_stmt
      case 117: // stmt
      case 118: // statement
      case 119: // suite
      case 120: // if_stmt
      case 121: // while_stmt
      case 122: // for_stmt
      case 123: // try_stmt
      case 126: // function_stmt
      case 128: // import_stmt1
      case 129: // import_stmt2
      case 130: // import_stmt3
      case 135: // tuple
      case 137: // list
      case 138: // comp_for
      case 140: // dict1
      case 141: // dict
      case 142: // idict1
      case 143: // idict
      case 146: // num
      case 147: // range
      case 148: // unit_exp
        yylhs.value.build< ExpressionPtr > ();
        break;

      case 39: // PSTRING
        yylhs.value.build< ExpressionString > ();
        break;

      case 133: // item
        yylhs.value.build< FlagExpression > ();
        break;

      case 134: // items2
      case 136: // items
        yylhs.value.build< FlagExpressionList > ();
        break;

      case 7: // INTEGER
      case 151: // integer
        yylhs.value.build< Integer > ();
        break;

      case 124: // arg_def
      case 144: // arg
        yylhs.value.build< NamedArgument > ();
        break;

      case 125: // arg_defs
      case 145: // args
        yylhs.value.build< NamedArgumentList > ();
        break;

      case 149: // identifier
      case 150: // iden
        yylhs.value.build< ObjectIdentifier > ();
        break;

      case 153: // document
      case 154: // object
        yylhs.value.build< ObjectIdentifier::String > ();
        break;

      case 105: // id_list
        yylhs.value.build< StringList > ();
        break;

      case 107: // target_list
      case 109: // target_list2
        yylhs.value.build< VarList > ();
        break;

      case 3: // ONE
      case 4: // NUM
        yylhs.value.build< double > ();
        break;

      case 5: // IDENTIFIER
      case 6: // UNIT
      case 9: // CELLADDRESS
      case 40: // STRING
      case 127: // module
      case 131: // id_or_cell
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
    { ctx.ScanResult = std::move(yystack_[0].value.as< ExpressionPtr > ()); ctx.valueExpression = true; }
#line 2008 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 3:
#line 109 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[1].value.as< ExpressionPtr > ()), OP_UNIT, std::move(yystack_[0].value.as< ExpressionPtr > ()));  }
#line 2014 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 4:
#line 110 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_UNIT_ADD, OperatorExpression::create(ctx.obj, std::move(yystack_[1].value.as< ExpressionPtr > ()), OP_UNIT, std::move(yystack_[0].value.as< ExpressionPtr > ()))); }
#line 2020 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 5:
#line 112 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2026 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 6:
#line 113 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2032 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 7:
#line 114 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2038 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 8:
#line 115 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = VariableExpression::create(ctx.obj, std::move(yystack_[0].value.as< ObjectIdentifier > ())); }
#line 2044 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 9:
#line 116 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2050 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 10:
#line 117 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2056 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 11:
#line 121 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = StringExpression::create(ctx.obj,std::move(yystack_[0].value.as< std::string > ())); }
#line 2062 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 12:
#line 122 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<StringExpression&>(*yystack_[1].value.as< ExpressionPtr > ()).append(std::move(yystack_[0].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 2068 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 13:
#line 126 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = StringExpression::create(ctx.obj,std::move(yystack_[0].value.as< ExpressionString > ())); }
#line 2074 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 14:
#line 127 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<StringExpression&>(*yystack_[1].value.as< ExpressionPtr > ()).append(std::move(yystack_[0].value.as< ExpressionString > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 2080 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 15:
#line 131 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 2086 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 16:
#line 132 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2092 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 17:
#line 133 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2098 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 18:
#line 134 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2104 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 19:
#line 135 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2110 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 20:
#line 136 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2116 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 21:
#line 137 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = VariableExpression::create(ctx.obj,std::move(yystack_[1].value.as< ObjectIdentifier > ())); yylhs.value.as< ExpressionPtr > ()->addComponent(std::move(yystack_[0].value.as< ComponentPtr > ())); }
#line 2122 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 22:
#line 138 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[1].value.as< ExpressionPtr > ()->addComponent(std::move(yystack_[0].value.as< ComponentPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 2128 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 23:
#line 139 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< ExpressionPtr > ()->addComponent(Expression::createComponent(yystack_[0].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 2134 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 24:
#line 143 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = CallableExpression::create(ctx.obj,std::move(yystack_[2].value.as< ObjectIdentifier > ())); }
#line 2140 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 25:
#line 144 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = CallableExpression::create(ctx.obj, std::move(yystack_[3].value.as< ObjectIdentifier > ()), std::move(yystack_[1].value.as< NamedArgumentList > ().first), std::move(yystack_[1].value.as< NamedArgumentList > ().second)); }
#line 2146 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 26:
#line 145 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = CallableExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ())); }
#line 2152 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 27:
#line 146 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = CallableExpression::create(ctx.obj, std::move(yystack_[3].value.as< ExpressionPtr > ()), std::move(yystack_[1].value.as< NamedArgumentList > ().first), std::move(yystack_[1].value.as< NamedArgumentList > ().second)); }
#line 2158 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 28:
#line 147 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = CallableExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ())); }
#line 2164 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 29:
#line 148 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = CallableExpression::create(ctx.obj, std::move(yystack_[3].value.as< ExpressionPtr > ()), std::move(yystack_[1].value.as< NamedArgumentList > ().first), std::move(yystack_[1].value.as< NamedArgumentList > ().second)); }
#line 2170 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 30:
#line 149 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2176 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 31:
#line 153 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[1].value.as< ExpressionPtr > ()->addComponent(std::move(yystack_[0].value.as< ComponentPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 2182 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 32:
#line 154 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< ExpressionPtr > ()->addComponent(Expression::createComponent(yystack_[0].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 2188 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 33:
#line 158 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2194 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 34:
#line 159 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ()), OP_NEG, NumberExpression::create(ctx.obj, Quantity(-1))); }
#line 2200 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 35:
#line 160 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ()), OP_NOT, NumberExpression::create(ctx.obj, Quantity(-1))); }
#line 2206 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 36:
#line 161 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ()), OP_POS, NumberExpression::create(ctx.obj, Quantity(1))); }
#line 2212 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 37:
#line 165 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2218 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 38:
#line 166 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_POW, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 2224 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 39:
#line 167 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_POW2, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 2230 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 40:
#line 170 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2236 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 41:
#line 171 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_MUL, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 2242 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 42:
#line 172 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_DIV, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 2248 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 43:
#line 173 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_MOD, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 2254 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 44:
#line 177 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2260 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 45:
#line 178 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_ADD, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 2266 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 46:
#line 179 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_SUB, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 2272 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 47:
#line 183 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2278 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 48:
#line 184 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_LT, std::move(yystack_[0].value.as< ExpressionPtr > ()));    }
#line 2284 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 49:
#line 185 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_GT, std::move(yystack_[0].value.as< ExpressionPtr > ()));    }
#line 2290 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 50:
#line 186 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_GTE, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 2296 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 51:
#line 187 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_LTE, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 2302 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 52:
#line 191 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2308 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 53:
#line 192 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_EQ, std::move(yystack_[0].value.as< ExpressionPtr > ()));    }
#line 2314 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 54:
#line 193 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_NEQ, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 2320 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 55:
#line 194 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_IS, std::move(yystack_[0].value.as< ExpressionPtr > ()));    }
#line 2326 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 56:
#line 195 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_IS_NOT, std::move(yystack_[0].value.as< ExpressionPtr > ()));    }
#line 2332 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 57:
#line 196 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_IN, std::move(yystack_[0].value.as< ExpressionPtr > ()));    }
#line 2338 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 58:
#line 197 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_NOT_IN, std::move(yystack_[0].value.as< ExpressionPtr > ()));    }
#line 2344 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 59:
#line 201 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2350 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 60:
#line 202 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_AND, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 2356 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 61:
#line 206 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2362 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 62:
#line 207 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_OR, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 2368 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 63:
#line 211 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2374 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 64:
#line 212 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2380 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 65:
#line 216 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2386 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 66:
#line 217 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ConditionalExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), std::move(yystack_[4].value.as< ExpressionPtr > ()), std::move(yystack_[0].value.as< ExpressionPtr > ()), true); }
#line 2392 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 67:
#line 218 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ConditionalExpression::create(ctx.obj, std::move(yystack_[4].value.as< ExpressionPtr > ()), std::move(yystack_[2].value.as< ExpressionPtr > ()), std::move(yystack_[0].value.as< ExpressionPtr > ()));                     }
#line 2398 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 68:
#line 222 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = LambdaExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2404 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 69:
#line 223 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = LambdaExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ()), std::move(yystack_[2].value.as< NamedArgumentList > ().first), std::move(yystack_[2].value.as< NamedArgumentList > ().second)); }
#line 2410 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 70:
#line 227 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = LambdaExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2416 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 71:
#line 228 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = LambdaExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ()), std::move(yystack_[2].value.as< NamedArgumentList > ().first), std::move(yystack_[2].value.as< NamedArgumentList > ().second)); }
#line 2422 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 72:
#line 232 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2428 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 73:
#line 233 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2434 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 74:
#line 236 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< StringList > ().push_back(std::move(yystack_[0].value.as< std::string > ())); }
#line 2440 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 75:
#line 237 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< StringList > ().push_back(std::move(yystack_[0].value.as< std::string > ())); yylhs.value.as< StringList > () = std::move(yystack_[2].value.as< StringList > ()); }
#line 2446 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 76:
#line 241 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2452 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 77:
#line 242 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = VariableExpression::create(ctx.obj,std::move(yystack_[0].value.as< ObjectIdentifier > ())); }
#line 2458 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 78:
#line 246 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< VarList > ().second=-1; yylhs.value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2464 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 79:
#line 247 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< VarList > ().second=0; yylhs.value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2470 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 80:
#line 248 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< VarList > () = std::move(yystack_[2].value.as< VarList > ()); }
#line 2476 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 81:
#line 249 "ExpressionParser.y" // lalr1.cc:871
    { 
                                               if(yystack_[3].value.as< VarList > ().second>=0) 
                                                   PARSER_THROW("Multiple catch all target"); 
                                               yystack_[3].value.as< VarList > ().second = (int)yystack_[3].value.as< VarList > ().first.size(); 
                                               yystack_[3].value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); 
                                               yylhs.value.as< VarList > () = std::move(yystack_[3].value.as< VarList > ()); 
                                            }
#line 2488 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 82:
#line 259 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2494 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 83:
#line 260 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2500 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 84:
#line 264 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< VarList > ().second=-1; yylhs.value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2506 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 85:
#line 265 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< VarList > ().second=0; yylhs.value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2512 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 86:
#line 266 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< VarList > () = std::move(yystack_[2].value.as< VarList > ()); }
#line 2518 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 87:
#line 267 "ExpressionParser.y" // lalr1.cc:871
    { 
                                               if(yystack_[3].value.as< VarList > ().second>=0) 
                                                   PARSER_THROW("Multiple catch all target"); 
                                               yystack_[3].value.as< VarList > ().second = (int)yystack_[3].value.as< VarList > ().first.size(); 
                                               yystack_[3].value.as< VarList > ().first.push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); 
                                               yylhs.value.as< VarList > () = std::move(yystack_[3].value.as< VarList > ()); 
                                            }
#line 2530 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 88:
#line 277 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,yystack_[2].value.as< VarList > ().second,std::move(yystack_[2].value.as< VarList > ().first),std::move(yystack_[0].value.as< ExpressionList > ())); }
#line 2536 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 89:
#line 281 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionList > ().push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2542 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 90:
#line 282 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< ExpressionList > ().push_back(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionList > () = std::move(yystack_[2].value.as< ExpressionList > ()); }
#line 2548 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 91:
#line 286 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ()),OP_MUL); }
#line 2554 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 92:
#line 287 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ()),OP_POW); }
#line 2560 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 93:
#line 288 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ()),OP_DIV); }
#line 2566 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 94:
#line 289 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ()),OP_FDIV); }
#line 2572 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 95:
#line 290 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ()),OP_MOD); }
#line 2578 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 96:
#line 291 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ()),OP_ADD); }
#line 2584 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 97:
#line 292 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = AssignmentExpression::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ()),OP_SUB); }
#line 2590 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 98:
#line 296 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2596 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 99:
#line 297 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2602 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 100:
#line 301 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2608 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 101:
#line 302 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2614 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 102:
#line 303 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = JumpStatement::create(ctx.obj,JUMP_RETURN,std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2620 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 103:
#line 304 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = JumpStatement::create(ctx.obj,JUMP_RETURN); }
#line 2626 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 104:
#line 305 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = JumpStatement::create(ctx.obj,JUMP_BREAK); }
#line 2632 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 105:
#line 306 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = JumpStatement::create(ctx.obj,JUMP_CONTINUE); }
#line 2638 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 106:
#line 307 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = JumpStatement::create(ctx.obj,JUMP_RAISE); }
#line 2644 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 107:
#line 308 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = JumpStatement::create(ctx.obj,JUMP_RAISE,std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2650 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 108:
#line 309 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = PseudoStatement::create(ctx.obj,PY_BEGIN); }
#line 2656 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 109:
#line 310 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = PseudoStatement::create(ctx.obj,PY_END); }
#line 2662 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 110:
#line 311 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = PseudoStatement::create(ctx.obj,PY_PASS); }
#line 2668 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 111:
#line 312 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = DelStatement::create(ctx.obj,std::move(yystack_[0].value.as< VarList > ().first)); }
#line 2674 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 112:
#line 313 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ScopeStatement::create(ctx.obj,std::move(yystack_[0].value.as< StringList > ())); }
#line 2680 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 113:
#line 314 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ScopeStatement::create(ctx.obj,std::move(yystack_[0].value.as< StringList > ()),false); }
#line 2686 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 114:
#line 315 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2692 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 115:
#line 316 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2698 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 116:
#line 317 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2704 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 117:
#line 321 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2710 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 118:
#line 322 "ExpressionParser.y" // lalr1.cc:871
    { 
                                               SimpleStatement *stmt = dynamic_cast<SimpleStatement*>(yystack_[2].value.as< ExpressionPtr > ().get()); 
                                               if(!stmt) {
                                                   yystack_[2].value.as< ExpressionPtr > () = SimpleStatement::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()));
                                                   stmt = static_cast<SimpleStatement*>(yystack_[2].value.as< ExpressionPtr > ().get());
                                               }
                                               stmt->add(std::move(yystack_[0].value.as< ExpressionPtr > ())); 
                                               yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ());
                                            }
#line 2724 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 119:
#line 331 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 2730 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 120:
#line 335 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2736 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 121:
#line 336 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<IfStatement&>(*yystack_[3].value.as< ExpressionPtr > ()).addElse(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > ()= std::move(yystack_[3].value.as< ExpressionPtr > ()); }
#line 2742 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 122:
#line 337 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2748 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 123:
#line 338 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<WhileStatement&>(*yystack_[3].value.as< ExpressionPtr > ()).addElse(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > ()= std::move(yystack_[3].value.as< ExpressionPtr > ()); }
#line 2754 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 124:
#line 339 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2760 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 125:
#line 340 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ForStatement&>(*yystack_[3].value.as< ExpressionPtr > ()).addElse(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > ()= std::move(yystack_[3].value.as< ExpressionPtr > ()); }
#line 2766 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 126:
#line 341 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2772 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 127:
#line 342 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<TryStatement&>(*yystack_[0].value.as< ExpressionPtr > ()).check(); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2778 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 128:
#line 346 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = Statement::create(ctx.obj, std::move(yystack_[1].value.as< ExpressionPtr > ())); }
#line 2784 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 129:
#line 347 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = Statement::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2790 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 130:
#line 348 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<Statement&>(*yystack_[2].value.as< ExpressionPtr > ()).add(std::move(yystack_[1].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 2796 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 131:
#line 349 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<Statement&>(*yystack_[1].value.as< ExpressionPtr > ()).add(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 2802 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 132:
#line 353 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2808 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 133:
#line 354 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 2814 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 134:
#line 355 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2820 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 135:
#line 359 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 2826 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 136:
#line 360 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 2832 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 137:
#line 364 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = IfStatement::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2838 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 138:
#line 365 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<IfStatement&>(*yystack_[4].value.as< ExpressionPtr > ()).add(std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 2844 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 139:
#line 369 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = WhileStatement::create(ctx.obj,std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2850 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 140:
#line 373 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ForStatement::create(ctx.obj,yystack_[4].value.as< VarList > ().second,std::move(yystack_[4].value.as< VarList > ().first),std::move(yystack_[2].value.as< ExpressionList > ()),std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2856 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 141:
#line 377 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = TryStatement::create(ctx.obj,std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2862 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 142:
#line 378 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<TryStatement&>(*yystack_[3].value.as< ExpressionPtr > ()).add(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[3].value.as< ExpressionPtr > ()); }
#line 2868 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 143:
#line 379 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<TryStatement&>(*yystack_[4].value.as< ExpressionPtr > ()).add(std::move(yystack_[0].value.as< ExpressionPtr > ()),std::move(yystack_[2].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 2874 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 144:
#line 380 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<TryStatement&>(*yystack_[6].value.as< ExpressionPtr > ()).add(std::move(yystack_[0].value.as< ExpressionPtr > ()),std::move(yystack_[4].value.as< ExpressionPtr > ()),std::move(yystack_[2].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[6].value.as< ExpressionPtr > ()); }
#line 2880 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 145:
#line 381 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<TryStatement&>(*yystack_[3].value.as< ExpressionPtr > ()).addElse(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[3].value.as< ExpressionPtr > ()); }
#line 2886 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 146:
#line 382 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<TryStatement&>(*yystack_[3].value.as< ExpressionPtr > ()).addFinal(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[3].value.as< ExpressionPtr > ()); }
#line 2892 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 147:
#line 386 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = std::move(yystack_[0].value.as< std::string > ()); }
#line 2898 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 148:
#line 387 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = std::move(yystack_[2].value.as< std::string > ()); yylhs.value.as< NamedArgument > ().second = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 2904 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 149:
#line 388 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = "*"; yylhs.value.as< NamedArgument > ().first+=yystack_[0].value.as< std::string > (); }
#line 2910 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 150:
#line 389 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = "**"; yylhs.value.as< NamedArgument > ().first+=yystack_[0].value.as< std::string > (); }
#line 2916 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 151:
#line 393 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgumentList > ().first.push_back(std::move(yystack_[0].value.as< NamedArgument > ().first)); yylhs.value.as< NamedArgumentList > ().second.push_back(std::move(yystack_[0].value.as< NamedArgument > ().second)); }
#line 2922 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 152:
#line 394 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< NamedArgumentList > ().first.push_back(std::move(yystack_[0].value.as< NamedArgument > ().first)); yystack_[2].value.as< NamedArgumentList > ().second.push_back(std::move(yystack_[0].value.as< NamedArgument > ().second)); yylhs.value.as< NamedArgumentList > () = std::move(yystack_[2].value.as< NamedArgumentList > ()); }
#line 2928 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 153:
#line 398 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = FunctionStatement::create(ctx.obj, std::move(yystack_[4].value.as< std::string > ()), std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 2934 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 154:
#line 399 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = FunctionStatement::create(ctx.obj, std::move(yystack_[5].value.as< std::string > ()), std::move(yystack_[0].value.as< ExpressionPtr > ()), std::move(yystack_[3].value.as< NamedArgumentList > ().first), std::move(yystack_[3].value.as< NamedArgumentList > ().second)); }
#line 2940 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 155:
#line 403 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< std::string > () = std::move(yystack_[0].value.as< std::string > ()); }
#line 2946 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 156:
#line 404 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< std::string > () += "."; yylhs.value.as< std::string > () += yystack_[0].value.as< std::string > (); }
#line 2952 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 157:
#line 408 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ImportStatement::create(ctx.obj, std::move(yystack_[0].value.as< std::string > ())); }
#line 2958 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 158:
#line 409 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ImportStatement::create(ctx.obj, std::move(yystack_[2].value.as< std::string > ()), std::move(yystack_[0].value.as< std::string > ())); }
#line 2964 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 159:
#line 410 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ImportStatement&>(*yystack_[2].value.as< ExpressionPtr > ()).add(std::move(yystack_[0].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 2970 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 160:
#line 411 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ImportStatement&>(*yystack_[4].value.as< ExpressionPtr > ()).add(std::move(yystack_[2].value.as< std::string > ()), std::move(yystack_[0].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 2976 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 161:
#line 415 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = FromStatement::create(ctx.obj, std::move(yystack_[2].value.as< std::string > ()), std::move(yystack_[0].value.as< std::string > ())); }
#line 2982 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 162:
#line 416 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = FromStatement::create(ctx.obj, std::move(yystack_[4].value.as< std::string > ()), std::move(yystack_[2].value.as< std::string > ()), std::move(yystack_[2].value.as< std::string > ())); }
#line 2988 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 163:
#line 417 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<FromStatement&>(*yystack_[2].value.as< ExpressionPtr > ()).add(std::move(yystack_[0].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 2994 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 164:
#line 418 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<FromStatement&>(*yystack_[4].value.as< ExpressionPtr > ()).add(std::move(yystack_[2].value.as< std::string > ()), std::move(yystack_[0].value.as< std::string > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 3000 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 165:
#line 422 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = FromStatement::create(ctx.obj, std::move(yystack_[2].value.as< std::string > ()), std::string("*")); }
#line 3006 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 166:
#line 426 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< std::string > () = std::move(yystack_[0].value.as< std::string > ()); }
#line 3012 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 167:
#line 427 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< std::string > () = std::move(yystack_[0].value.as< std::string > ()); }
#line 3018 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 170:
#line 433 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< FlagExpression > ().first = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3024 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 171:
#line 434 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< FlagExpression > ().first = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3030 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 172:
#line 435 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< FlagExpression > ().first = std::move(yystack_[0].value.as< ExpressionPtr > ()); yylhs.value.as< FlagExpression > ().second = true; }
#line 3036 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 173:
#line 436 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< FlagExpression > ().first = std::move(yystack_[0].value.as< ExpressionPtr > ()); yylhs.value.as< FlagExpression > ().second = true; }
#line 3042 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 174:
#line 440 "ExpressionParser.y" // lalr1.cc:871
    { 
                                                yylhs.value.as< FlagExpressionList > ().first.push_back(std::move(yystack_[2].value.as< FlagExpression > ().first)); 
                                                yylhs.value.as< FlagExpressionList > ().second.push_back(yystack_[2].value.as< FlagExpression > ().second);
                                                yylhs.value.as< FlagExpressionList > ().first.push_back(std::move(yystack_[0].value.as< FlagExpression > ().first)); 
                                                yylhs.value.as< FlagExpressionList > ().second.push_back(yystack_[0].value.as< FlagExpression > ().second);
                                            }
#line 3053 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 175:
#line 446 "ExpressionParser.y" // lalr1.cc:871
    { 
                                                yystack_[2].value.as< FlagExpressionList > ().first.push_back(std::move(yystack_[0].value.as< FlagExpression > ().first)); 
                                                yystack_[2].value.as< FlagExpressionList > ().second.push_back(std::move(yystack_[0].value.as< FlagExpression > ().second)); 
                                                yylhs.value.as< FlagExpressionList > () = std::move(yystack_[2].value.as< FlagExpressionList > ()); 
                                            }
#line 3063 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 176:
#line 454 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = TupleExpression::create(ctx.obj); }
#line 3069 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 177:
#line 455 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = TupleExpression::create(ctx.obj, std::move(yystack_[2].value.as< FlagExpression > ().first), yystack_[2].value.as< FlagExpression > ().second); }
#line 3075 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 178:
#line 456 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = TupleExpression::create(ctx.obj, std::move(yystack_[1].value.as< FlagExpressionList > ().first), std::move(yystack_[1].value.as< FlagExpressionList > ().second)); }
#line 3081 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 179:
#line 457 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = TupleExpression::create(ctx.obj, std::move(yystack_[2].value.as< FlagExpressionList > ().first), std::move(yystack_[2].value.as< FlagExpressionList > ().second)); }
#line 3087 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 180:
#line 461 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< FlagExpressionList > ().first.push_back(std::move(yystack_[0].value.as< FlagExpression > ().first)); yylhs.value.as< FlagExpressionList > ().second.push_back(yystack_[0].value.as< FlagExpression > ().second); }
#line 3093 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 181:
#line 462 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< FlagExpressionList > ().first.push_back(std::move(yystack_[0].value.as< FlagExpression > ().first)); yystack_[2].value.as< FlagExpressionList > ().second.push_back(yystack_[0].value.as< FlagExpression > ().second); yylhs.value.as< FlagExpressionList > () = std::move(yystack_[2].value.as< FlagExpressionList > ()); }
#line 3099 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 182:
#line 466 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ListExpression::create(ctx.obj); }
#line 3105 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 183:
#line 467 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ListExpression::create(ctx.obj, std::move(yystack_[1].value.as< FlagExpressionList > ().first), std::move(yystack_[1].value.as< FlagExpressionList > ().second)); }
#line 3111 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 184:
#line 468 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ListExpression::create(ctx.obj, std::move(yystack_[2].value.as< FlagExpressionList > ().first), std::move(yystack_[2].value.as< FlagExpressionList > ().second)); }
#line 3117 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 185:
#line 469 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ComprehensionExpression&>(*yystack_[1].value.as< ExpressionPtr > ()).setExpr(std::move(yystack_[2].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3123 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 186:
#line 473 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ComprehensionExpression::create(ctx.obj,yystack_[2].value.as< VarList > ().second,std::move(yystack_[2].value.as< VarList > ().first),std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3129 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 187:
#line 474 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ComprehensionExpression&>(*yystack_[4].value.as< ExpressionPtr > ()).add(yystack_[2].value.as< VarList > ().second,std::move(yystack_[2].value.as< VarList > ().first),std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 3135 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 188:
#line 475 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ComprehensionExpression&>(*yystack_[2].value.as< ExpressionPtr > ()).addCond(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 3141 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 191:
#line 482 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = DictExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3147 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 192:
#line 483 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = DictExpression::create(ctx.obj, std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3153 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 193:
#line 484 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<DictExpression&>(*yystack_[4].value.as< ExpressionPtr > ()).addItem(std::move(yystack_[2].value.as< ExpressionPtr > ()),std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 3159 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 194:
#line 485 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<DictExpression&>(*yystack_[3].value.as< ExpressionPtr > ()).addItem(std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[3].value.as< ExpressionPtr > ()); }
#line 3165 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 195:
#line 489 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = DictExpression::create(ctx.obj); }
#line 3171 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 196:
#line 490 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3177 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 197:
#line 491 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 3183 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 198:
#line 492 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ComprehensionExpression&>(*yystack_[1].value.as< ExpressionPtr > ()).setExpr(std::move(yystack_[2].value.as< ExpressionPtr > ()),false); yylhs.value.as< ExpressionPtr > ()= std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3189 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 199:
#line 493 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<ComprehensionExpression&>(*yystack_[1].value.as< ExpressionPtr > ()).setExpr(std::move(yystack_[4].value.as< ExpressionPtr > ()),std::move(yystack_[2].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > ()= std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3195 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 200:
#line 497 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = IDictExpression::create(ctx.obj, std::move(yystack_[2].value.as< std::string > ()), std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3201 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 201:
#line 498 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = IDictExpression::create(ctx.obj, "**", std::move(yystack_[0].value.as< ExpressionPtr > ())); }
#line 3207 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 202:
#line 499 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<IDictExpression&>(*yystack_[4].value.as< ExpressionPtr > ()).addItem(std::move(yystack_[2].value.as< std::string > ()),std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 3213 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 203:
#line 500 "ExpressionParser.y" // lalr1.cc:871
    { static_cast<IDictExpression&>(*yystack_[4].value.as< ExpressionPtr > ()).addItem("**",std::move(yystack_[0].value.as< ExpressionPtr > ())); yylhs.value.as< ExpressionPtr > () = std::move(yystack_[4].value.as< ExpressionPtr > ()); }
#line 3219 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 204:
#line 504 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ()); }
#line 3225 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 205:
#line 505 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[2].value.as< ExpressionPtr > ()); }
#line 3231 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 206:
#line 509 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().second = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3237 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 207:
#line 510 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = std::move(yystack_[2].value.as< std::string > ()); yylhs.value.as< NamedArgument > ().second = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3243 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 208:
#line 511 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = "*"; yylhs.value.as< NamedArgument > ().second = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3249 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 209:
#line 512 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().second = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3255 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 210:
#line 513 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = "*"; yylhs.value.as< NamedArgument > ().second = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3261 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 211:
#line 514 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgument > ().first = "**"; yylhs.value.as< NamedArgument > ().second = std::move(yystack_[0].value.as< ExpressionPtr > ()); }
#line 3267 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 212:
#line 518 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< NamedArgumentList > ().first.push_back(std::move(yystack_[0].value.as< NamedArgument > ().first)); yylhs.value.as< NamedArgumentList > ().second.push_back(std::move(yystack_[0].value.as< NamedArgument > ().second)); }
#line 3273 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 213:
#line 519 "ExpressionParser.y" // lalr1.cc:871
    { yystack_[2].value.as< NamedArgumentList > ().first.push_back(std::move(yystack_[0].value.as< NamedArgument > ().first)); yystack_[2].value.as< NamedArgumentList > ().second.push_back(std::move(yystack_[0].value.as< NamedArgument > ().second)); yylhs.value.as< NamedArgumentList > () = std::move(yystack_[2].value.as< NamedArgumentList > ()); }
#line 3279 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 214:
#line 523 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = NumberExpression::create(ctx.obj, std::move(yystack_[0].value.as< double > ()));                        }
#line 3285 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 215:
#line 524 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = NumberExpression::create(ctx.obj, std::move(yystack_[0].value.as< double > ()));                        }
#line 3291 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 216:
#line 525 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = NumberExpression::create(ctx.obj, (double)yystack_[0].value.as< Integer > ());                }
#line 3297 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 217:
#line 526 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = ConstantExpression::create(ctx.obj, yystack_[0].value.as< ConstantInfo > ().first, yystack_[0].value.as< ConstantInfo > ().second);      }
#line 3303 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 218:
#line 530 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = RangeExpression::create(ctx.obj, std::move(yystack_[2].value.as< std::string > ()), std::move(yystack_[0].value.as< std::string > ()));                               }
#line 3309 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 219:
#line 534 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = UnitExpression::create(ctx.obj, yystack_[0].value.as< std::string > ().c_str());                                 }
#line 3315 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 220:
#line 535 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = UnitExpression::create(ctx.obj, yystack_[0].value.as< std::string > ().c_str());                                 }
#line 3321 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 221:
#line 537 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_DIV, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3327 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 222:
#line 538 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_MUL, std::move(yystack_[0].value.as< ExpressionPtr > ()));   }
#line 3333 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 223:
#line 539 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[2].value.as< ExpressionPtr > ()), OP_POW, NumberExpression::create(ctx.obj, Quantity((double)yystack_[0].value.as< Integer > ())));   }
#line 3339 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 224:
#line 540 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[3].value.as< ExpressionPtr > ()), OP_POW, NumberExpression::create(ctx.obj, Quantity(-(double)yystack_[0].value.as< Integer > ())));   }
#line 3345 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 225:
#line 541 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = std::move(yystack_[1].value.as< ExpressionPtr > ());                                                               }
#line 3351 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 226:
#line 542 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ExpressionPtr > () = OperatorExpression::create(ctx.obj, std::move(yystack_[3].value.as< ExpressionPtr > ()), OP_DIV, std::move(yystack_[1].value.as< ExpressionPtr > ()));   }
#line 3357 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 227:
#line 546 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ObjectIdentifier > () = ObjectIdentifier(ctx.obj); yylhs.value.as< ObjectIdentifier > () << ObjectIdentifier::SimpleComponent(yystack_[0].value.as< std::string > ()); }
#line 3363 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 228:
#line 547 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ObjectIdentifier > () = std::move(yystack_[0].value.as< ObjectIdentifier > ()); }
#line 3369 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 229:
#line 551 "ExpressionParser.y" // lalr1.cc:871
    {
                                                yylhs.value.as< ObjectIdentifier > () = ObjectIdentifier(ctx.obj, true);
                                                yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::LabelComponent(std::move(yystack_[0].value.as< std::string > ())));
                                            }
#line 3378 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 230:
#line 555 "ExpressionParser.y" // lalr1.cc:871
    {
                                                yylhs.value.as< ObjectIdentifier > () = ObjectIdentifier(ctx.obj, true);
                                                yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::SimpleComponent(std::move(yystack_[0].value.as< std::string > ())));
                                            }
#line 3387 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 231:
#line 559 "ExpressionParser.y" // lalr1.cc:871
    {
                                                yylhs.value.as< ObjectIdentifier > () = ObjectIdentifier(ctx.obj);
                                                yylhs.value.as< ObjectIdentifier > ().setDocumentObjectName(std::move(yystack_[2].value.as< ObjectIdentifier::String > ()), true, ObjectIdentifier::String(std::move(yystack_[0].value.as< std::string > ()),true),true);
                                            }
#line 3396 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 232:
#line 563 "ExpressionParser.y" // lalr1.cc:871
    {
                                                yylhs.value.as< ObjectIdentifier > () = ObjectIdentifier(ctx.obj);
                                                yystack_[2].value.as< ObjectIdentifier::String > ().checkImport(ctx.obj);
                                                if(yystack_[2].value.as< ObjectIdentifier::String > ().isRealString())
                                                    yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::LabelComponent(std::move(yystack_[2].value.as< ObjectIdentifier::String > ().getString())));
                                                else
                                                    yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::SimpleComponent(std::move(yystack_[2].value.as< ObjectIdentifier::String > ().getString())));
                                                yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::SimpleComponent(std::move(yystack_[0].value.as< std::string > ())));
                                            }
#line 3410 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 233:
#line 572 "ExpressionParser.y" // lalr1.cc:871
    {
                                                yylhs.value.as< ObjectIdentifier > () = ObjectIdentifier(ctx.obj);
                                                yylhs.value.as< ObjectIdentifier > ().setDocumentName(std::move(yystack_[2].value.as< ObjectIdentifier::String > ()), true);
                                                yylhs.value.as< ObjectIdentifier > ().setDocumentObjectName(std::move(yystack_[0].value.as< ObjectIdentifier::String > ()), true);
                                            }
#line 3420 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 234:
#line 577 "ExpressionParser.y" // lalr1.cc:871
    {
                                                yylhs.value.as< ObjectIdentifier > () = std::move(yystack_[2].value.as< ObjectIdentifier > ());
                                                yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::LabelComponent(std::move(yystack_[0].value.as< std::string > ())));
                                            }
#line 3429 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 235:
#line 581 "ExpressionParser.y" // lalr1.cc:871
    {
                                                yylhs.value.as< ObjectIdentifier > () = std::move(yystack_[2].value.as< ObjectIdentifier > ());
                                                yylhs.value.as< ObjectIdentifier > ().addComponent(ObjectIdentifier::SimpleComponent(std::move(yystack_[0].value.as< std::string > ())));
                                            }
#line 3438 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 236:
#line 588 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< Integer > () = std::move(yystack_[0].value.as< Integer > ()); }
#line 3444 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 237:
#line 589 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< Integer > () = std::move(yystack_[0].value.as< double > ()); }
#line 3450 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 238:
#line 593 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(std::move(yystack_[1].value.as< ExpressionPtr > ()));   }
#line 3456 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 239:
#line 594 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(std::move(yystack_[2].value.as< ExpressionPtr > ()),ExpressionPtr(),ExpressionPtr(),true); }
#line 3462 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 240:
#line 595 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(ExpressionPtr(),std::move(yystack_[1].value.as< ExpressionPtr > ())); }
#line 3468 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 241:
#line 596 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(ExpressionPtr(),ExpressionPtr(),std::move(yystack_[1].value.as< ExpressionPtr > ())); }
#line 3474 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 242:
#line 597 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(std::move(yystack_[3].value.as< ExpressionPtr > ()),std::move(yystack_[1].value.as< ExpressionPtr > ()));}
#line 3480 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 243:
#line 598 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(std::move(yystack_[4].value.as< ExpressionPtr > ()),ExpressionPtr(),std::move(yystack_[1].value.as< ExpressionPtr > ())); }
#line 3486 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 244:
#line 599 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(ExpressionPtr(),std::move(yystack_[3].value.as< ExpressionPtr > ()),std::move(yystack_[1].value.as< ExpressionPtr > ())); }
#line 3492 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 245:
#line 600 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ComponentPtr > () = Expression::createComponent(std::move(yystack_[5].value.as< ExpressionPtr > ()),std::move(yystack_[3].value.as< ExpressionPtr > ()),std::move(yystack_[1].value.as< ExpressionPtr > ()));}
#line 3498 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 246:
#line 604 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ObjectIdentifier::String > () = ObjectIdentifier::String(std::move(yystack_[0].value.as< std::string > ()), true); }
#line 3504 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 247:
#line 605 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ObjectIdentifier::String > () = ObjectIdentifier::String(std::move(yystack_[0].value.as< std::string > ()), false, true);}
#line 3510 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 248:
#line 609 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ObjectIdentifier::String > () = ObjectIdentifier::String(std::move(yystack_[0].value.as< std::string > ()), true); }
#line 3516 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;

  case 249:
#line 610 "ExpressionParser.y" // lalr1.cc:871
    { yylhs.value.as< ObjectIdentifier::String > () = ObjectIdentifier::String(std::move(yystack_[0].value.as< std::string > ()), false);}
#line 3522 "ExpressionParser.tab.cc" // lalr1.cc:871
    break;


#line 3526 "ExpressionParser.tab.cc" // lalr1.cc:871
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


  const short int parser::yypact_ninf_ = -221;

  const short int parser::yytable_ninf_ = -250;

  const short int
  parser::yypact_[] =
  {
     621,  -221,  -221,   -47,  -221,  -221,  -221,  1484,  1435,   -13,
      55,    97,    55,  1484,  -221,   141,   936,  1435,  1435,   405,
    -221,  -221,  1435,  -221,  -221,    78,  -221,   405,   135,   135,
    1484,   271,  1124,   137,    47,   424,   151,   383,  -221,   132,
     158,   947,   235,    28,  -221,   162,   370,   111,   522,   145,
     202,   107,  -221,  -221,  -221,   477,  -221,   313,  -221,  -221,
    -221,  -221,    -1,  -221,   812,  -221,   300,   191,   207,   128,
    -221,   203,   282,  -221,   192,  -221,  -221,   278,  -221,   285,
    -221,    18,  1020,   216,   199,   287,   299,  -221,  -221,   -32,
    -221,   874,  -221,     3,   254,   360,  1435,   369,  -221,   197,
     117,  -221,   123,   321,   332,   322,   271,   215,  -221,    14,
     328,  -221,   336,   338,  -221,   348,   348,  -221,   299,   235,
    -221,  -221,   -32,  1435,  -221,   352,    73,   170,   320,  -221,
    -221,  -221,  -221,  -221,   377,  -221,   324,   272,  -221,  -221,
    -221,   224,  1302,  -221,    18,  -221,  -221,   998,   425,  1344,
    -221,  1040,   435,  -221,  1484,  1484,  1484,  1484,  1484,  1484,
    1484,  1484,  1484,  1484,  1484,  1484,  1484,  1484,  1484,  1484,
    1484,  1484,  1484,  1484,  1435,  1435,  1435,  1435,  1435,  1435,
    1435,  1435,   442,  1435,  -221,   936,    80,  -221,  1435,   380,
     388,   391,  1351,   397,   398,    55,   443,  -221,  -221,  -221,
     540,  -221,    24,  -221,  -221,    90,   396,  1082,  -221,   204,
     212,   255,   414,    87,  -221,   457,   466,  1435,  -221,  -221,
    -221,  1435,    77,    21,  -221,   874,   874,  -221,  1435,   708,
      66,   469,  -221,  -221,  -221,   314,  1166,  -221,  1208,   405,
     182,  -221,   628,  1435,  1435,   100,  1435,  -221,   396,   294,
    1435,  1435,  -221,  -221,  -221,   334,  -221,  -221,  1393,   186,
    -221,   347,  -221,  -221,  -221,   162,   162,   162,   370,   370,
     111,   111,   111,   111,   522,   522,   522,   522,   522,   522,
     145,   202,   189,   416,  -221,  -221,  -221,  -221,  -221,  -221,
    -221,   271,  -221,  -221,   401,  -221,  -221,   418,   874,   874,
     874,   874,    12,   874,   874,    41,   462,  -221,   427,  1435,
     415,  -221,   417,   429,   241,    18,    18,   187,  -221,   381,
    -221,  -221,  -221,  -221,  -221,  -221,  -221,   812,  -221,  -221,
    -221,  -221,  -221,  -221,   468,  -221,  -221,  -221,   221,   271,
    -221,   433,   395,  -221,  -221,  -221,  -221,  -221,  -221,  -221,
     157,  1442,   405,  -221,  -221,  -221,  -221,   377,  -221,  -221,
    1435,  -221,  -221,  -221,  -221,  1215,  1435,   206,  1257,  -221,
    -221,  1435,  1435,  -221,  1435,   874,  -221,  -221,  -221,  -221,
     495,   874,  -221,  -221,   498,   510,  1435,  -221,  1435,  1435,
      18,  -221,   396,   396,  -221,  -221,   319,  -221,  -221,    -1,
     750,   512,   874,  -221,   874,   456,  1484,   109,   502,  -221,
    -221,   159,   147,  -221,  -221,   444,  1435,  -221,  1435,  -221,
     208,  -221,  -221,  -221,  -221,   459,  -221,  -221,  -221,  -221,
    -221,  -221,   260,  -221,  -221,  -221,  -221,  -221,   874,   502,
    1442,   217,  1484,  -221,  -221,   453,   460,  1435,  -221,   874,
    -221,  -221,  -221,  1442,   502,  -221,  -221,   461,  -221,  -221,
    -221
  };

  const unsigned char
  parser::yydefact_[] =
  {
       0,   214,   215,   166,   216,   217,   167,     0,   106,     0,
       0,     0,     0,     0,    13,    11,     0,     0,     0,     0,
     104,   105,   103,   108,   109,     0,   110,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     6,    33,     7,
      20,     9,    10,    30,    37,    40,    44,    47,    52,    59,
      61,    65,    72,    73,   101,    82,    84,     0,    98,    99,
     100,   117,   132,   129,   134,     2,   120,   122,   124,   127,
     126,   114,   115,   116,   227,    16,    17,     0,    18,     0,
      19,     5,     8,   228,     0,     0,     9,    30,    35,     8,
     107,     0,   155,   157,   147,     0,     0,     0,   151,     0,
       0,    34,     0,     0,     0,   246,     0,    76,    78,     0,
      77,   102,     0,   111,    74,   112,   113,    36,    76,     0,
      82,    85,    77,     0,   176,   170,   227,     0,     0,   171,
     166,   229,   230,   182,   170,   180,     0,   166,   190,   189,
     195,     0,     0,     1,     0,    12,    14,     0,     0,     0,
      22,     0,     0,    31,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   128,   119,     0,   131,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   168,   169,   196,
       0,   204,     0,   220,   219,     0,     3,     0,    21,     0,
       0,     0,     0,     0,   141,     0,     0,     0,   150,    70,
     149,     0,     0,     0,   133,     0,     0,    79,     0,     0,
       0,     0,   172,   173,    15,     0,     0,   178,     0,     0,
       0,   183,     0,     0,     0,     0,     0,   192,     4,   166,
       0,     0,    26,   206,   212,     0,   209,    23,     0,     0,
      28,     0,    32,    39,    38,    41,    42,    43,    46,    45,
      48,    49,    50,    51,    53,    54,    55,    56,    58,    57,
      60,    62,     0,     0,    92,    91,    94,    93,    95,    96,
      97,     0,    86,    89,    88,   118,   130,     0,     0,     0,
       0,     0,     0,     0,     0,   159,   163,   197,     0,     0,
       0,   205,     0,     0,     0,     0,     0,     0,    24,     0,
     234,   235,   248,   249,   233,   231,   232,     0,   135,   158,
     156,   148,    71,   152,   161,   165,   137,   139,     0,     0,
      80,     0,     0,    75,   218,   177,   170,   174,   179,   175,
       0,     0,     0,   185,   184,   181,   200,   191,   198,   201,
       0,   211,   208,   210,    27,     0,     0,     0,     0,   238,
      29,     0,     0,    87,     0,     0,   121,   123,   125,   142,
       0,     0,   146,   145,     0,     0,     0,   194,     0,     0,
       0,   225,   222,   221,   237,   236,     0,   223,    25,     0,
       0,     0,     0,    81,     0,     0,     0,     0,    63,   188,
      64,     0,     0,   207,   213,     0,     0,   240,     0,   239,
       0,    66,    67,    90,   138,     0,   143,   160,   164,   193,
     202,   203,     0,   224,   136,   162,   140,   153,     0,   186,
       0,     0,     0,   199,   241,     0,     0,     0,   242,     0,
     226,   154,    68,     0,   187,   244,   243,     0,   144,    69,
     245
  };

  const short int
  parser::yypgoto_[] =
  {
    -221,  -221,  -221,  -221,  -221,  -221,     0,   -24,    37,    15,
     222,   256,   399,   386,   375,   392,  -165,  -156,  -221,  -221,
    -221,   350,   529,    11,   -23,   -22,  -221,  -221,   337,  -221,
    -221,   356,     5,   -58,   239,  -221,  -167,  -221,  -221,  -221,
    -221,   351,  -220,  -221,     6,  -221,  -221,  -221,   -31,   -66,
      13,  -221,  -221,  -221,  -221,  -127,  -122,  -221,  -221,  -221,
    -221,   205,   -94,   -17,  -108,  -132,    17,  -221,   173,   -10,
    -221,   362
  };

  const short int
  parser::yydefgoto_[] =
  {
      -1,    36,    37,    38,    39,    40,    86,    42,    87,    44,
      45,    46,    47,    48,    49,    50,    51,   409,    52,   410,
      53,    54,   115,    55,   109,    56,    57,    58,   294,    59,
      60,    61,   213,    63,    64,    65,   214,    66,    67,    68,
      69,    98,    99,    70,    93,    71,    72,    73,    74,   222,
     127,   128,    75,   136,    76,   240,   142,    77,    78,    79,
      80,   254,   255,    81,   129,   206,    89,    83,   397,   150,
      84,    85
  };

  const short int
  parser::yytable_[] =
  {
      41,   126,   132,   126,   113,    62,   187,   119,   282,   121,
     342,   200,   248,   202,   245,   233,    41,    82,   100,   107,
     144,   102,    88,   203,   204,   215,   334,   107,   101,   310,
     108,   118,   153,    82,   380,  -247,   110,    43,   108,   256,
     207,   184,   120,   256,   110,   117,   149,   135,   122,    91,
       1,     2,     3,    43,     4,     5,     6,   261,   336,   337,
      92,   236,   238,   384,    41,   138,     7,   228,    43,   186,
     242,    94,   208,   314,   381,    11,   185,   216,   309,   208,
     312,    82,    94,   112,    13,   335,    14,    15,   139,   229,
     205,    41,   126,     1,     2,   203,   204,     4,     5,   256,
     208,    43,    94,   -83,   -83,   311,   107,    95,    82,   153,
      30,   123,   208,   319,    94,   216,   126,   227,    95,    32,
     126,    33,   296,   110,   172,    34,   133,    35,    43,   328,
      97,   376,   377,   378,   379,   235,   382,   383,    95,   341,
     114,    97,   130,   363,   223,   351,     6,  -249,   159,   352,
      95,   143,   173,   192,   193,   165,   166,   185,   119,    96,
     292,    97,   205,   167,   185,   168,   169,   224,   174,   263,
     264,   440,   145,    97,   160,   194,   126,   131,   321,   323,
     326,   358,   118,   392,   393,    41,   408,   441,   313,   365,
     394,   216,   351,   120,   395,   365,   352,   146,   170,   122,
     185,   305,    82,   154,   344,   126,   172,   126,   424,   130,
     406,   126,   442,     6,   426,  -248,   350,   130,   171,    43,
     126,     6,    43,  -246,   396,    41,    41,   351,   443,   107,
     412,   352,   229,   155,   229,   436,   371,   437,   190,   107,
     340,   439,    82,    82,   320,   197,   110,   198,   368,   347,
     108,   349,   322,   365,   191,   355,   110,   256,   432,   221,
     130,   353,    43,    43,     6,   369,  -249,   119,   416,   373,
     447,   451,   197,   239,   198,   408,     3,   454,   195,   453,
       6,   210,   458,   402,   452,   417,   244,   448,   408,   148,
     209,   118,   197,   149,   198,   325,   374,   459,    41,    41,
      41,    41,   120,    41,    41,   315,   316,   151,   122,   152,
      14,   105,   317,   149,   391,    82,    82,    82,    82,   130,
      82,    82,   394,     6,   315,   316,   395,    41,    43,   411,
     217,   317,   399,   450,   126,    43,    43,    43,    43,   107,
      43,    43,   187,    32,    82,    33,   188,   189,   243,    34,
     403,    35,   107,   197,  -247,   198,   110,   196,    90,   199,
     197,   211,   198,   108,    43,   218,   201,   103,   104,   110,
     360,   147,   111,   148,   220,    41,  -247,   149,   265,   266,
     267,    41,   125,   225,   134,   141,     1,     2,   182,   183,
       4,     5,    82,   237,   226,   197,  -248,   198,    82,   197,
      41,   198,    41,   241,    41,   186,   149,   364,   230,   197,
       3,   198,    43,   229,     6,   268,   269,    82,    43,    82,
     370,    82,   197,   231,   198,   234,   239,     1,     2,   137,
     257,     4,     5,     6,   156,   157,   158,    43,    41,    43,
     262,    43,   298,     7,    14,   105,   219,     3,   306,    41,
     299,     6,    11,   300,   398,    82,   197,   327,   198,   303,
     304,    13,   329,    14,    15,   138,    82,   317,   405,   106,
     197,   330,   198,   232,   343,    43,   374,    32,   372,    33,
     375,    14,   105,    34,   385,    35,    43,    30,   139,   386,
     401,   388,   247,   389,   390,   404,    32,   253,    33,   259,
     425,   253,    34,   427,    35,   140,   291,   175,   176,   177,
     178,   179,   180,   181,    32,   428,    33,   435,   438,   172,
      34,   449,    35,   444,   283,   284,   285,   286,   287,   288,
     289,   290,   455,   293,   161,   162,   163,   164,   297,   456,
     460,   295,   302,     1,     2,     3,   280,     4,     5,     6,
     308,   274,   275,   276,   277,   278,   279,   253,   116,     7,
     270,   271,   272,   273,   281,   338,   400,   331,    11,   433,
     414,   332,   324,   333,     0,     0,     0,    13,   293,    14,
      15,   138,     0,     0,     0,     0,   346,     0,   346,     0,
       0,     0,   346,   356,   357,     0,   359,     0,     0,     0,
     361,   362,     0,    30,   139,     0,     0,     0,   367,     0,
       0,     0,    32,     0,    33,     0,     0,     0,    34,     0,
      35,   307,     0,     0,     1,     2,     3,     0,     4,     5,
       6,     1,     2,     3,     0,     4,     5,     6,     0,     0,
       7,     0,     0,     0,     8,     9,     0,     7,    10,    11,
      12,     0,     0,     0,     0,     0,    11,     0,    13,   387,
      14,    15,     0,     0,    16,    13,    17,    14,    15,    18,
      19,    20,    21,    22,     0,    23,    24,    25,    26,    27,
      28,    29,     0,     0,    30,    31,     0,     0,     0,     0,
       0,    30,   123,    32,     0,    33,     0,     0,     0,    34,
      32,    35,    33,     0,     0,     0,    34,   354,    35,     0,
     413,     0,     0,     3,     0,   253,   415,     6,   420,     0,
       0,   421,   422,     0,   423,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   429,     0,   430,   431,
       0,     0,     0,     0,     0,     0,     0,    14,   105,     0,
       0,     0,     0,     1,     2,     3,     0,     4,     5,     6,
       0,     0,     0,     0,     0,     0,   445,     0,   446,     7,
       0,     0,   339,     8,     9,     0,     0,    10,    11,    12,
      32,     0,    33,     0,     0,     0,    34,    13,    35,    14,
      15,     0,     0,     0,   434,    17,     0,   457,    18,    19,
      20,    21,    22,     0,    23,    24,    25,    26,    27,    28,
      29,     0,     0,    30,    31,     1,     2,     3,     0,     4,
       5,     6,    32,     0,    33,     0,     0,     0,    34,     0,
      35,     7,     0,     0,     0,     8,     9,     0,     0,    10,
      11,    12,     0,     0,     0,     0,     0,     0,     0,    13,
       0,    14,    15,     0,     0,     0,     0,    17,     0,     0,
      18,    19,    20,    21,    22,     0,    23,    24,    25,    26,
      27,    28,    29,     0,     0,    30,    31,     1,     2,     3,
       0,     4,     5,     6,    32,     0,    33,     0,     0,     0,
      34,     0,    35,     7,     0,     0,     0,     8,     0,     0,
       0,    10,    11,    12,     0,     0,     0,     0,     0,     0,
       0,    13,     0,    14,    15,     0,   212,     0,     0,     0,
       0,     0,     0,     0,    20,    21,    22,     0,    23,    24,
       0,    26,    27,    28,    29,     0,     0,    30,    31,     1,
       2,     3,     0,     4,     5,     6,    32,     0,    33,     0,
       0,     0,    34,     0,    35,     7,     0,     0,     0,     8,
       0,     0,     0,    10,    11,    12,     0,     0,     0,     0,
       0,     0,     0,    13,     0,    14,    15,   -76,   -76,   -76,
     -76,   -76,   -76,   -76,     0,     0,    20,    21,    22,     0,
      23,    24,     0,    26,    27,    28,    29,     0,     0,    30,
      31,     1,     2,   249,     0,     4,     5,     6,    32,     0,
      33,     0,     0,     0,    34,     0,    35,     7,     0,   147,
       0,   148,   -76,   -76,     0,   149,    11,     0,     0,     0,
       0,     0,     0,     0,     0,    13,     0,    14,    15,   250,
       0,     0,     0,     1,     2,   249,     0,     4,     5,     6,
     -77,   -77,   -77,   -77,   -77,   -77,   -77,     0,     0,     7,
       0,    30,   251,     0,     0,     0,     0,     0,    11,     0,
      32,   252,    33,     0,     0,     0,    34,    13,    35,    14,
      15,   250,     0,     0,     0,     1,     2,   249,     0,     4,
       5,     6,   207,     0,     0,   -77,   -77,     0,   149,     0,
       0,     7,     0,    30,   251,     0,     0,     0,     0,     0,
      11,     0,    32,   260,    33,     0,     0,     0,    34,    13,
      35,    14,    15,   250,     0,     0,     0,     1,     2,     3,
       0,     4,     5,     6,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     7,     0,    30,   251,     0,     0,     0,
       0,     0,    11,     0,    32,   318,    33,     0,     0,     0,
      34,    13,    35,    14,    15,     0,     0,     0,     0,     1,
       2,     3,     0,     4,     5,     6,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     7,     0,    30,   123,     0,
       0,     0,     0,     0,    11,     0,    32,   124,    33,     0,
       0,     0,    34,    13,    35,    14,    15,     0,     0,     0,
       0,     1,     2,     3,     0,     4,     5,     6,     1,     2,
     249,     0,     4,     5,     6,     0,     0,     7,     0,    30,
     123,     0,     0,     0,     7,     0,    11,     0,    32,   345,
      33,     0,     0,    11,    34,    13,    35,    14,    15,     0,
       0,     0,    13,     0,    14,    15,   250,     0,     0,     0,
       1,     2,     3,     0,     4,     5,     6,     0,     0,     0,
       0,    30,   123,     0,     0,     0,     7,     0,    30,   251,
      32,   348,    33,     0,     0,    11,    34,    32,    35,    33,
       0,     0,     0,    34,    13,    35,    14,    15,     0,     0,
       0,     0,     0,     0,     0,     1,     2,     3,     0,     4,
       5,     6,     0,     0,     0,     0,     0,     0,     0,   418,
      30,     7,     0,     0,     0,     0,     0,     0,     0,    32,
      11,    33,     0,     0,     0,    34,   419,    35,     0,    13,
       0,    14,    15,     0,     0,     0,     0,     1,     2,     3,
       0,     4,     5,     6,     1,     2,     3,     0,     4,     5,
       6,     0,     0,     7,     0,    30,     0,     0,     0,     0,
       7,     0,    11,     0,    32,     0,    33,     0,   246,    11,
      34,    13,    35,    14,    15,     0,     0,     0,    13,     0,
      14,    15,     0,     0,     0,     0,     1,     2,     3,     0,
       4,     5,     6,     0,     0,     0,   258,    30,     0,     0,
       0,     0,     7,   301,    30,     0,    32,     0,    33,     0,
       0,    11,    34,    32,    35,    33,     0,     0,     0,    34,
      13,    35,    14,    15,     0,     0,     0,     0,     1,     2,
       3,     0,     4,     5,     6,     1,     2,     3,     0,     4,
       5,     6,     0,     0,     7,   366,    30,     0,     0,     0,
       0,     7,     0,    11,     0,    32,     0,    33,     0,     0,
     407,    34,    13,    35,    14,    15,     0,     0,     0,    13,
       0,    14,    15,     0,     0,     0,     0,     1,     2,     3,
       0,     4,     5,     6,     0,     0,     0,     0,    30,     0,
       0,     0,     0,     7,     0,    30,     0,    32,     0,    33,
       0,     0,     0,    34,    32,    35,    33,     0,     0,     0,
      34,    13,    35,    14,    15,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    30,     0,     0,
       0,     0,     0,     0,     0,     0,    32,     0,    33,     0,
       0,     0,    34,     0,    35
  };

  const short int
  parser::yycheck_[] =
  {
       0,    32,    33,    34,    27,     0,    64,    31,   173,    31,
     230,    77,   144,    79,   141,   123,    16,     0,    12,    19,
      37,    16,     7,     5,     6,    22,     5,    27,    13,     5,
      19,    31,    42,    16,    22,    82,    19,     0,    27,   147,
      72,    42,    31,   151,    27,    30,    78,    34,    31,    62,
       3,     4,     5,    16,     7,     8,     9,   151,   225,   226,
       5,   127,   128,    22,    64,    41,    19,    53,    31,    64,
     136,     5,    82,   205,    62,    28,    77,    74,   200,    89,
     202,    64,     5,     5,    37,    64,    39,    40,    64,    75,
      72,    91,   123,     3,     4,     5,     6,     7,     8,   207,
     110,    64,     5,    75,    76,    81,   106,    41,    91,   119,
      63,    64,   122,   207,     5,    74,   147,   106,    41,    72,
     151,    74,    42,   106,    17,    78,    79,    80,    91,    42,
      64,   298,   299,   300,   301,    62,   303,   304,    41,    73,
       5,    64,     5,   251,    27,    45,     9,    74,    37,    49,
      41,     0,    45,    25,    26,    10,    11,    77,   182,    62,
     182,    64,    72,    18,    77,    20,    21,    44,    61,   154,
     155,    62,    40,    64,    63,    47,   207,    40,   209,   210,
     211,    81,   182,   315,   316,   185,   351,   407,   205,   255,
       3,    74,    45,   182,     7,   261,    49,    39,    53,   182,
      77,   195,   185,    41,   235,   236,    17,   238,   375,     5,
      53,   242,    53,     9,   381,    74,   239,     5,    16,   182,
     251,     9,   185,    82,    37,   225,   226,    45,    81,   229,
     357,    49,    75,    71,    75,   402,    47,   404,    47,   239,
     229,   406,   225,   226,    40,    75,   229,    77,    62,   236,
     239,   238,    40,   319,    47,   242,   239,   365,   390,    62,
       5,    79,   225,   226,     9,    79,    74,   291,    62,   291,
      62,   438,    75,    49,    77,   440,     5,   442,    75,    62,
       9,    82,   449,    62,   440,    79,    62,    79,   453,    74,
      74,   291,    75,    78,    77,    40,    75,   453,   298,   299,
     300,   301,   291,   303,   304,    64,    65,    72,   291,    74,
      39,    40,    71,    78,    73,   298,   299,   300,   301,     5,
     303,   304,     3,     9,    64,    65,     7,   327,   291,   352,
      76,    71,   327,    73,   365,   298,   299,   300,   301,   339,
     303,   304,   400,    72,   327,    74,    46,    47,    76,    78,
     339,    80,   352,    75,    82,    77,   339,    75,     8,    81,
      75,    74,    77,   352,   327,     5,    81,    17,    18,   352,
      76,    72,    22,    74,     5,   375,    82,    78,   156,   157,
     158,   381,    32,    62,    34,    35,     3,     4,    75,    76,
       7,     8,   375,    73,    62,    75,    74,    77,   381,    75,
     400,    77,   402,    79,   404,   400,    78,    73,    72,    75,
       5,    77,   375,    75,     9,   159,   160,   400,   381,   402,
      73,   404,    75,    75,    77,    73,    49,     3,     4,     5,
       5,     7,     8,     9,    64,    65,    66,   400,   438,   402,
       5,   404,    62,    19,    39,    40,    96,     5,     5,   449,
      62,     9,    28,    62,    73,   438,    75,    43,    77,    62,
      62,    37,     5,    39,    40,    41,   449,    71,    73,    64,
      75,     5,    77,   123,     5,   438,    75,    72,    62,    74,
      62,    39,    40,    78,    22,    80,   449,    63,    64,    62,
      22,    76,   142,    76,    65,    62,    72,   147,    74,   149,
       5,   151,    78,     5,    80,    81,    64,    30,    31,    32,
      33,    34,    35,    36,    72,     5,    74,     5,    62,    17,
      78,    62,    80,    79,   174,   175,   176,   177,   178,   179,
     180,   181,    79,   183,    12,    13,    14,    15,   188,    79,
      79,   185,   192,     3,     4,     5,   171,     7,     8,     9,
     200,   165,   166,   167,   168,   169,   170,   207,    29,    19,
     161,   162,   163,   164,   172,   228,   327,   217,    28,   396,
     365,   221,   210,   222,    -1,    -1,    -1,    37,   228,    39,
      40,    41,    -1,    -1,    -1,    -1,   236,    -1,   238,    -1,
      -1,    -1,   242,   243,   244,    -1,   246,    -1,    -1,    -1,
     250,   251,    -1,    63,    64,    -1,    -1,    -1,   258,    -1,
      -1,    -1,    72,    -1,    74,    -1,    -1,    -1,    78,    -1,
      80,    81,    -1,    -1,     3,     4,     5,    -1,     7,     8,
       9,     3,     4,     5,    -1,     7,     8,     9,    -1,    -1,
      19,    -1,    -1,    -1,    23,    24,    -1,    19,    27,    28,
      29,    -1,    -1,    -1,    -1,    -1,    28,    -1,    37,   309,
      39,    40,    -1,    -1,    43,    37,    45,    39,    40,    48,
      49,    50,    51,    52,    -1,    54,    55,    56,    57,    58,
      59,    60,    -1,    -1,    63,    64,    -1,    -1,    -1,    -1,
      -1,    63,    64,    72,    -1,    74,    -1,    -1,    -1,    78,
      72,    80,    74,    -1,    -1,    -1,    78,    79,    80,    -1,
     360,    -1,    -1,     5,    -1,   365,   366,     9,   368,    -1,
      -1,   371,   372,    -1,   374,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   386,    -1,   388,   389,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    40,    -1,
      -1,    -1,    -1,     3,     4,     5,    -1,     7,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,   416,    -1,   418,    19,
      -1,    -1,    64,    23,    24,    -1,    -1,    27,    28,    29,
      72,    -1,    74,    -1,    -1,    -1,    78,    37,    80,    39,
      40,    -1,    -1,    -1,    44,    45,    -1,   447,    48,    49,
      50,    51,    52,    -1,    54,    55,    56,    57,    58,    59,
      60,    -1,    -1,    63,    64,     3,     4,     5,    -1,     7,
       8,     9,    72,    -1,    74,    -1,    -1,    -1,    78,    -1,
      80,    19,    -1,    -1,    -1,    23,    24,    -1,    -1,    27,
      28,    29,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,
      -1,    39,    40,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    49,    50,    51,    52,    -1,    54,    55,    56,    57,
      58,    59,    60,    -1,    -1,    63,    64,     3,     4,     5,
      -1,     7,     8,     9,    72,    -1,    74,    -1,    -1,    -1,
      78,    -1,    80,    19,    -1,    -1,    -1,    23,    -1,    -1,
      -1,    27,    28,    29,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    37,    -1,    39,    40,    -1,    42,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    50,    51,    52,    -1,    54,    55,
      -1,    57,    58,    59,    60,    -1,    -1,    63,    64,     3,
       4,     5,    -1,     7,     8,     9,    72,    -1,    74,    -1,
      -1,    -1,    78,    -1,    80,    19,    -1,    -1,    -1,    23,
      -1,    -1,    -1,    27,    28,    29,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    37,    -1,    39,    40,    30,    31,    32,
      33,    34,    35,    36,    -1,    -1,    50,    51,    52,    -1,
      54,    55,    -1,    57,    58,    59,    60,    -1,    -1,    63,
      64,     3,     4,     5,    -1,     7,     8,     9,    72,    -1,
      74,    -1,    -1,    -1,    78,    -1,    80,    19,    -1,    72,
      -1,    74,    75,    76,    -1,    78,    28,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    -1,    39,    40,    41,
      -1,    -1,    -1,     3,     4,     5,    -1,     7,     8,     9,
      30,    31,    32,    33,    34,    35,    36,    -1,    -1,    19,
      -1,    63,    64,    -1,    -1,    -1,    -1,    -1,    28,    -1,
      72,    73,    74,    -1,    -1,    -1,    78,    37,    80,    39,
      40,    41,    -1,    -1,    -1,     3,     4,     5,    -1,     7,
       8,     9,    72,    -1,    -1,    75,    76,    -1,    78,    -1,
      -1,    19,    -1,    63,    64,    -1,    -1,    -1,    -1,    -1,
      28,    -1,    72,    73,    74,    -1,    -1,    -1,    78,    37,
      80,    39,    40,    41,    -1,    -1,    -1,     3,     4,     5,
      -1,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    19,    -1,    63,    64,    -1,    -1,    -1,
      -1,    -1,    28,    -1,    72,    73,    74,    -1,    -1,    -1,
      78,    37,    80,    39,    40,    -1,    -1,    -1,    -1,     3,
       4,     5,    -1,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    -1,    63,    64,    -1,
      -1,    -1,    -1,    -1,    28,    -1,    72,    73,    74,    -1,
      -1,    -1,    78,    37,    80,    39,    40,    -1,    -1,    -1,
      -1,     3,     4,     5,    -1,     7,     8,     9,     3,     4,
       5,    -1,     7,     8,     9,    -1,    -1,    19,    -1,    63,
      64,    -1,    -1,    -1,    19,    -1,    28,    -1,    72,    73,
      74,    -1,    -1,    28,    78,    37,    80,    39,    40,    -1,
      -1,    -1,    37,    -1,    39,    40,    41,    -1,    -1,    -1,
       3,     4,     5,    -1,     7,     8,     9,    -1,    -1,    -1,
      -1,    63,    64,    -1,    -1,    -1,    19,    -1,    63,    64,
      72,    73,    74,    -1,    -1,    28,    78,    72,    80,    74,
      -1,    -1,    -1,    78,    37,    80,    39,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,    -1,     7,
       8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    62,
      63,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    72,
      28,    74,    -1,    -1,    -1,    78,    79,    80,    -1,    37,
      -1,    39,    40,    -1,    -1,    -1,    -1,     3,     4,     5,
      -1,     7,     8,     9,     3,     4,     5,    -1,     7,     8,
       9,    -1,    -1,    19,    -1,    63,    -1,    -1,    -1,    -1,
      19,    -1,    28,    -1,    72,    -1,    74,    -1,    76,    28,
      78,    37,    80,    39,    40,    -1,    -1,    -1,    37,    -1,
      39,    40,    -1,    -1,    -1,    -1,     3,     4,     5,    -1,
       7,     8,     9,    -1,    -1,    -1,    62,    63,    -1,    -1,
      -1,    -1,    19,    62,    63,    -1,    72,    -1,    74,    -1,
      -1,    28,    78,    72,    80,    74,    -1,    -1,    -1,    78,
      37,    80,    39,    40,    -1,    -1,    -1,    -1,     3,     4,
       5,    -1,     7,     8,     9,     3,     4,     5,    -1,     7,
       8,     9,    -1,    -1,    19,    62,    63,    -1,    -1,    -1,
      -1,    19,    -1,    28,    -1,    72,    -1,    74,    -1,    -1,
      28,    78,    37,    80,    39,    40,    -1,    -1,    -1,    37,
      -1,    39,    40,    -1,    -1,    -1,    -1,     3,     4,     5,
      -1,     7,     8,     9,    -1,    -1,    -1,    -1,    63,    -1,
      -1,    -1,    -1,    19,    -1,    63,    -1,    72,    -1,    74,
      -1,    -1,    -1,    78,    72,    80,    74,    -1,    -1,    -1,
      78,    37,    80,    39,    40,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    63,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    72,    -1,    74,    -1,
      -1,    -1,    78,    -1,    80
  };

  const unsigned char
  parser::yystos_[] =
  {
       0,     3,     4,     5,     7,     8,     9,    19,    23,    24,
      27,    28,    29,    37,    39,    40,    43,    45,    48,    49,
      50,    51,    52,    54,    55,    56,    57,    58,    59,    60,
      63,    64,    72,    74,    78,    80,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   101,   103,   104,   106,   108,   109,   110,   112,
     113,   114,   115,   116,   117,   118,   120,   121,   122,   123,
     126,   128,   129,   130,   131,   135,   137,   140,   141,   142,
     143,   146,   149,   150,   153,   154,    89,    91,    92,   149,
     104,    62,     5,   127,     5,    41,    62,    64,   124,   125,
     127,    92,   115,   104,   104,    40,    64,    89,   106,   107,
     149,   104,     5,   107,     5,   105,   105,    92,    89,    90,
     106,   108,   149,    64,    73,   104,   131,   133,   134,   147,
       5,    40,   131,    79,   104,   133,   136,     5,    41,    64,
      81,   104,   139,     0,   146,    40,    39,    72,    74,    78,
     152,    72,    74,   152,    41,    71,    64,    65,    66,    37,
      63,    12,    13,    14,    15,    10,    11,    18,    20,    21,
      53,    16,    17,    45,    61,    30,    31,    32,    33,    34,
      35,    36,    75,    76,    42,    77,   115,   116,    46,    47,
      47,    47,    25,    26,    47,    75,    75,    75,    77,    81,
     132,    81,   132,     5,     6,    72,   148,    72,   152,    74,
      82,    74,    42,   115,   119,    22,    74,    76,     5,   104,
       5,    62,   132,    27,    44,    62,    62,   106,    53,    75,
      72,    75,   104,   147,    73,    62,   132,    73,   132,    49,
     138,    79,   132,    76,    62,   138,    76,   104,   148,     5,
      41,    64,    73,   104,   144,   145,   147,     5,    62,   104,
      73,   145,     5,    92,    92,    93,    93,    93,    94,    94,
      95,    95,    95,    95,    96,    96,    96,    96,    96,    96,
      97,    98,    99,   104,   104,   104,   104,   104,   104,   104,
     104,    64,   108,   104,   111,   114,    42,   104,    62,    62,
      62,    62,   104,    62,    62,   127,     5,    81,   104,   139,
       5,    81,   139,   146,   148,    64,    65,    71,    73,   145,
      40,   131,    40,   131,   154,    40,   131,    43,    42,     5,
       5,   104,   104,   124,     5,    64,   119,   119,   111,    64,
     106,    73,   125,     5,   131,    73,   104,   133,    73,   133,
     107,    45,    49,    79,    79,   133,   104,   104,    81,   104,
      76,   104,   104,   147,    73,   132,    62,   104,    62,    79,
      73,    47,    62,   108,    75,    62,   119,   119,   119,   119,
      22,    62,   119,   119,    22,    22,    62,   104,    76,    76,
      65,    73,   148,   148,     3,     7,    37,   151,    73,   115,
     117,    22,    62,   106,    62,    73,    53,    28,    99,   100,
     102,   107,   138,   104,   144,   104,    62,    79,    62,    79,
     104,   104,   104,   104,   119,     5,   119,     5,     5,   104,
     104,   104,   148,   151,    44,     5,   119,   119,    62,    99,
      62,   125,    53,    81,    79,   104,   104,    62,    79,    62,
      73,   119,   100,    62,    99,    79,    79,   104,   119,   100,
      79
  };

  const unsigned char
  parser::yyr1_[] =
  {
       0,    83,    84,    85,    85,    86,    86,    86,    86,    86,
      86,    87,    87,    88,    88,    89,    89,    89,    89,    89,
      89,    89,    89,    89,    90,    90,    90,    90,    90,    90,
      90,    91,    91,    92,    92,    92,    92,    93,    93,    93,
      94,    94,    94,    94,    95,    95,    95,    96,    96,    96,
      96,    96,    97,    97,    97,    97,    97,    97,    97,    98,
      98,    99,    99,   100,   100,   101,   101,   101,   102,   102,
     103,   103,   104,   104,   105,   105,   106,   106,   107,   107,
     107,   107,   108,   108,   109,   109,   109,   109,   110,   111,
     111,   112,   112,   112,   112,   112,   112,   112,   113,   113,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   115,   115,   115,
     116,   116,   116,   116,   116,   116,   116,   116,   117,   117,
     117,   117,   118,   118,   118,   119,   119,   120,   120,   121,
     122,   123,   123,   123,   123,   123,   123,   124,   124,   124,
     124,   125,   125,   126,   126,   127,   127,   128,   128,   128,
     128,   129,   129,   129,   129,   130,   131,   131,   132,   132,
     133,   133,   133,   133,   134,   134,   135,   135,   135,   135,
     136,   136,   137,   137,   137,   137,   138,   138,   138,   139,
     139,   140,   140,   140,   140,   141,   141,   141,   141,   141,
     142,   142,   142,   142,   143,   143,   144,   144,   144,   144,
     144,   144,   145,   145,   146,   146,   146,   146,   147,   148,
     148,   148,   148,   148,   148,   148,   148,   149,   149,   150,
     150,   150,   150,   150,   150,   150,   151,   151,   152,   152,
     152,   152,   152,   152,   152,   152,   153,   153,   154,   154
  };

  const unsigned char
  parser::yyr2_[] =
  {
       0,     2,     1,     2,     3,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     2,     3,     1,     1,     1,     1,
       1,     2,     2,     3,     3,     4,     3,     4,     3,     4,
       1,     2,     3,     1,     2,     2,     2,     1,     3,     3,
       1,     3,     3,     3,     1,     3,     3,     1,     3,     3,
       3,     3,     1,     3,     3,     3,     3,     3,     3,     1,
       3,     1,     3,     1,     1,     1,     5,     5,     3,     4,
       3,     4,     1,     1,     1,     3,     1,     1,     1,     2,
       3,     4,     1,     1,     1,     2,     3,     4,     3,     1,
       3,     3,     3,     3,     3,     3,     3,     3,     1,     1,
       1,     1,     2,     1,     1,     1,     1,     2,     1,     1,
       1,     2,     2,     2,     1,     1,     1,     1,     3,     2,
       1,     4,     1,     4,     1,     4,     1,     1,     2,     1,
       3,     2,     1,     3,     1,     2,     4,     4,     5,     4,
       6,     3,     4,     5,     7,     4,     4,     1,     3,     2,
       2,     1,     3,     6,     7,     1,     3,     2,     4,     3,
       5,     4,     6,     3,     5,     4,     1,     1,     1,     1,
       1,     1,     2,     2,     3,     3,     2,     4,     3,     4,
       1,     3,     2,     3,     4,     4,     4,     5,     3,     1,
       1,     4,     3,     5,     4,     2,     2,     3,     4,     6,
       4,     4,     5,     5,     2,     3,     1,     3,     2,     1,
       2,     2,     1,     3,     1,     1,     1,     1,     3,     1,
       1,     3,     3,     3,     4,     3,     5,     1,     1,     2,
       2,     3,     3,     3,     3,     3,     1,     1,     3,     4,
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
  "'%'", "NEG", "POS", "NUM_AND_UNIT", "NUM_DIV_UNIT", "'^'", "'('", "')'",
  "'.'", "','", "'='", "';'", "'['", "']'", "'{'", "'}'", "'#'", "$accept",
  "input", "uexp", "primary_exp", "string", "pstring", "indexable",
  "callable", "indexable2", "unary_exp", "power_exp", "multiply_exp",
  "additive_exp", "relational_exp", "equality_exp", "and_exp", "or_exp",
  "nocond_exp", "cond_exp", "lambda_nocond_exp", "lambda_exp", "exp",
  "id_list", "target", "target_list", "target2", "target_list2",
  "assignment_exp1", "exp_list", "assignment_exp2", "assignment_exp",
  "small_stmt", "simple_stmt", "compound_stmt", "stmt", "statement",
  "suite", "if_stmt", "while_stmt", "for_stmt", "try_stmt", "arg_def",
  "arg_defs", "function_stmt", "module", "import_stmt1", "import_stmt2",
  "import_stmt3", "id_or_cell", "sep", "item", "items2", "tuple", "items",
  "list", "comp_for", "dict_expand", "dict1", "dict", "idict1", "idict",
  "arg", "args", "num", "range", "unit_exp", "identifier", "iden",
  "integer", "indexer", "document", "object", YY_NULLPTR
  };

#if YYDEBUG
  const unsigned short int
  parser::yyrline_[] =
  {
       0,   105,   105,   109,   110,   112,   113,   114,   115,   116,
     117,   121,   122,   126,   127,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   143,   144,   145,   146,   147,   148,
     149,   153,   154,   158,   159,   160,   161,   165,   166,   167,
     170,   171,   172,   173,   177,   178,   179,   183,   184,   185,
     186,   187,   191,   192,   193,   194,   195,   196,   197,   201,
     202,   206,   207,   211,   212,   216,   217,   218,   222,   223,
     227,   228,   232,   233,   236,   237,   241,   242,   246,   247,
     248,   249,   259,   260,   264,   265,   266,   267,   277,   281,
     282,   286,   287,   288,   289,   290,   291,   292,   296,   297,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,   314,   315,   316,   317,   321,   322,   331,
     335,   336,   337,   338,   339,   340,   341,   342,   346,   347,
     348,   349,   353,   354,   355,   359,   360,   364,   365,   369,
     373,   377,   378,   379,   380,   381,   382,   386,   387,   388,
     389,   393,   394,   398,   399,   403,   404,   408,   409,   410,
     411,   415,   416,   417,   418,   422,   426,   427,   430,   430,
     433,   434,   435,   436,   440,   446,   454,   455,   456,   457,
     461,   462,   466,   467,   468,   469,   473,   474,   475,   479,
     479,   482,   483,   484,   485,   489,   490,   491,   492,   493,
     497,   498,   499,   500,   504,   505,   509,   510,   511,   512,
     513,   514,   518,   519,   523,   524,   525,   526,   530,   534,
     535,   537,   538,   539,   540,   541,   542,   546,   547,   551,
     555,   559,   563,   572,   577,   581,   588,   589,   593,   594,
     595,   596,   597,   598,   599,   600,   604,   605,   609,   610
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
       2,     2,     2,     2,     2,    82,     2,    66,     2,     2,
      72,    73,    64,    63,    75,     2,    74,    65,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    62,    77,
       2,    76,     2,    61,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    78,     2,    79,    71,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    80,     2,    81,     2,     2,     2,     2,
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
      55,    56,    57,    58,    59,    60,    67,    68,    69,    70
    };
    const unsigned int user_token_number_max_ = 319;
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
#line 4494 "ExpressionParser.tab.cc" // lalr1.cc:1179
#line 613 "ExpressionParser.y" // lalr1.cc:1180

