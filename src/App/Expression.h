/***************************************************************************
 *   Copyright (c) Eivind Kvedalen (eivind@kvedalen.name) 2015             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <string>
#include <boost/tuple/tuple.hpp>
#include <Base/Exception.h>
#include <Base/Unit.h>
#include <App/Property.h>
#include <App/ObjectIdentifier.h>
#include <Base/BaseClass.h>
#include <Base/Quantity.h>
#include <set>
#include <deque>
#include <App/Range.h>

namespace App  {

class DocumentObject;
class Expression;
class Document;

class AppExport ExpressionVisitor {
public:
    virtual ~ExpressionVisitor() {}
    virtual void visit(Expression * e) = 0;
};

template<class P> class ExpressionModifier : public ExpressionVisitor {
public:
    ExpressionModifier(P & _prop)
        : prop(_prop) { }

    virtual ~ExpressionModifier() { }

    void setExpressionChanged() {
        if (!signaller)
            signaller = boost::shared_ptr<typename AtomicPropertyChangeInterface<P>::AtomicPropertyChange>(AtomicPropertyChangeInterface<P>::getAtomicPropertyChange(prop));
    }

    bool getChanged() const { return signaller != 0; }

protected:
    P & prop;
    boost::shared_ptr<typename AtomicPropertyChangeInterface<P>::AtomicPropertyChange> signaller;
};

/**
  * Base class for expressions.
  *
  */

class AppExport Expression : public Base::BaseClass {
    TYPESYSTEM_HEADER();

public:

    Expression(const App::DocumentObject * _owner);

    virtual ~Expression();

    virtual bool isTouched() const { return false; }

    virtual Expression * eval() const = 0;

    virtual std::string toString() const = 0;

    static Expression * parse(const App::DocumentObject * owner, const std::string& buffer);

    virtual Expression * copy() const = 0;

    virtual int priority() const { return 0; }

    virtual void getDeps(std::set<ObjectIdentifier> &/*props*/) const { }

    virtual Expression * simplify() const = 0;

    virtual void visit(ExpressionVisitor & v) { v.visit(this); }

    class Exception : public Base::Exception {
    public:
        Exception(const char *sMessage) : Base::Exception(sMessage) { }
    };

    const App::DocumentObject *  getOwner() const { return owner; }

    virtual boost::any getValueAsAny() const { static boost::any empty; return empty; }

protected:
    const App::DocumentObject * owner; /**< The document object used to access unqualified variables (i.e local scope) */
};

/**
  * Part of an expressions that contains a unit.
  *
  */

class  AppExport UnitExpression : public Expression {
    TYPESYSTEM_HEADER();
public:
    UnitExpression(const App::DocumentObject *_owner = 0, const Base::Quantity & _quantity = Base::Quantity(), const std::string & _unitStr = std::string());

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const;

    void setUnit(const Base::Quantity &_quantity);

    double getValue() const { return quantity.getValue(); }

    const Base::Unit & getUnit() const { return quantity.getUnit(); }

    const Base::Quantity & getQuantity() const { return quantity; }

    const std::string getUnitString() const { return unitStr; }

    double getScaler() const { return quantity.getValue(); }

    boost::any getValueAsAny() const { return quantity.getUnit().isEmpty() ? boost::any(quantity.getValue()) : boost::any(quantity); }

protected:
    Base::Quantity quantity;
    std::string unitStr; /**< The unit string from the original parsed string */
};

/**
  * Class implementing a number with an optional unit
  */

class AppExport NumberExpression : public UnitExpression {
    TYPESYSTEM_HEADER();
public:
    NumberExpression(const App::DocumentObject *_owner = 0, const Base::Quantity & quantity = Base::Quantity());

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual Expression * copy() const;

    virtual int priority() const;

    void negate();

    virtual std::string toString() const;

protected:
};

class AppExport ConstantExpression : public NumberExpression {
    TYPESYSTEM_HEADER();
public:
    ConstantExpression(const App::DocumentObject *_owner = 0, std::string _name = "", const Base::Quantity &_quantity = Base::Quantity());

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const;

    std::string getName() const { return name; }

protected:
    std::string name; /**< Constant's name */
};

class AppExport BooleanExpression : public NumberExpression {
    TYPESYSTEM_HEADER();
public:
    BooleanExpression(const App::DocumentObject *_owner = 0, bool _value = false);

    virtual Expression * copy() const;

};


/**
  * Class implementing an infix expression.
  *
  */

class AppExport OperatorExpression : public UnitExpression {
    TYPESYSTEM_HEADER();
public:
    enum Operator {
        NONE,
        ADD,
        SUB,
        MUL,
        DIV,
        POW,
        EQ,
        NEQ,
        LT,
        GT,
        LTE,
        GTE,
        UNIT,
        NEG,
        POS
    };
    OperatorExpression(const App::DocumentObject *_owner = 0, Expression * _left = 0, Operator _op = NONE, Expression * _right = 0);

    virtual ~OperatorExpression();

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const;

    virtual void getDeps(std::set<ObjectIdentifier> &props) const;

    virtual void visit(ExpressionVisitor & v);

    Operator getOperator() const { return op; }

    Expression * getLeft() const { return left; }

    Expression * getRight() const { return right; }

protected:

    virtual bool isCommutative() const;

    virtual bool isLeftAssociative() const;

    virtual bool isRightAssociative() const;

    Operator op;        /**< Operator working on left and right */
    Expression * left;  /**< Left operand */
    Expression * right; /**< Right operand */
};

class AppExport ConditionalExpression : public Expression {
    TYPESYSTEM_HEADER();
public:
    ConditionalExpression(const App::DocumentObject *_owner = 0, Expression * _condition = 0,Expression * _trueExpr = 0,  Expression * _falseExpr = 0);

    virtual ~ConditionalExpression();

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const;

    virtual void getDeps(std::set<ObjectIdentifier> &props) const;

    virtual void visit(ExpressionVisitor & v);

protected:

    Expression * condition;  /**< Condition */
    Expression * trueExpr;  /**< Expression if abs(condition) is > 0.5 */
    Expression * falseExpr; /**< Expression if abs(condition) is < 0.5 */
};

/**
  * Class implementing various functions, e.g sin, cos, etc.
  *
  */

class AppExport FunctionExpression : public UnitExpression {
    TYPESYSTEM_HEADER();
public:
    enum Function {
        NONE,

        // Normal functions taking one or two arguments
        ACOS,
        ASIN,
        ATAN,
        ABS,
        EXP,
        LOG,
        LOG10,
        SIN,
        SINH,
        TAN,
        TANH,
        SQRT,
        COS,
        COSH,
        ATAN2,
        MOD,
        POW,
        ROUND,
        TRUNC,
        CEIL,
        FLOOR,
        HYPOT,
        CATH,

        // Aggregates
        AGGREGATES,

        SUM,
        AVERAGE,
        STDDEV,
        COUNT,
        MIN,
        MAX,

        // Last one
        LAST,
    };

    FunctionExpression(const App::DocumentObject *_owner = 0, Function _f = NONE, std::vector<Expression *> _args = std::vector<Expression*>());

    virtual ~FunctionExpression();

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const;

    virtual void getDeps(std::set<ObjectIdentifier> &props) const;

    virtual void visit(ExpressionVisitor & v);

protected:
    Expression *evalAggregate() const;

    Function f;        /**< Function to execute */
    std::vector<Expression *> args; /** Arguments to function*/
};

/**
  * Class implementing a reference to a property. If the name is unqualified,
  * the owner of the expression is searched. If it is qualified, the document
  * that contains the owning document object is searched for other document
  * objects to search. Both labels and internal document names are searched.
  *
  */

class AppExport VariableExpression : public UnitExpression {
    TYPESYSTEM_HEADER();
public:
    VariableExpression(const App::DocumentObject *_owner = 0, ObjectIdentifier _var = ObjectIdentifier());

    ~VariableExpression();

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const { return var.toString(); }

    virtual Expression * copy() const;

    virtual int priority() const;

    virtual void getDeps(std::set<ObjectIdentifier> &props) const;

    std::string name() const { return var.getPropertyName(); }

    ObjectIdentifier getPath() const { return var; }

    void setPath(const ObjectIdentifier & path);

    bool validDocumentObjectRename(const std::string & oldName, const std::string & newName);

    bool renameDocumentObject(const std::string & oldName, const std::string & newName);

    bool validDocumentRename(const std::string &oldName, const std::string &newName);

    bool renameDocument(const std::string &oldName, const std::string &newName);

    const App::Property *getProperty() const;

protected:

    ObjectIdentifier var; /**< Variable name  */
};

/**
  * Class implementing a string. Used to signal either a genuine string or
  * a failed evaluation of an expression.
  */

class AppExport StringExpression : public Expression {
    TYPESYSTEM_HEADER();
public:
    StringExpression(const App::DocumentObject *_owner = 0, const std::string & _text = std::string());

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual std::string getText() const { return text; }

    virtual int priority() const;

    virtual Expression * copy() const;

protected:

    std::string text; /**< Text string */
};

class AppExport RangeExpression : public App::Expression {
    TYPESYSTEM_HEADER();
public:
    RangeExpression(const App::DocumentObject * _owner = 0, const std::string & begin = std::string(), const std::string & end = std::string());

    virtual ~RangeExpression() { }

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const;

    virtual void getDeps(std::set<App::ObjectIdentifier> &props) const;

    virtual App::Expression * simplify() const;

    Range getRange() const { return range; }

    void setRange(const Range & r);

protected:
    Range range;
};

namespace ExpressionParser {
AppExport Expression * parse(const App::DocumentObject *owner, const char *buffer);
AppExport UnitExpression * parseUnit(const App::DocumentObject *owner, const char *buffer);
AppExport ObjectIdentifier parsePath(const App::DocumentObject *owner, const char* buffer);
AppExport bool isTokenAnIndentifier(const std::string & str);
AppExport bool isTokenAUnit(const std::string & str);
AppExport std::vector<boost::tuple<int, int, std::string> > tokenize(const std::string & str);

/**
 * @brief The semantic_type class encapsulates the value in the parse tree during parsing.
 */

class semantic_type {
public:
  struct  {
    Base::Quantity scaler;
    std::string unitStr;
  } quantity;
  Expression * expr;
  ObjectIdentifier path;
  std::deque<ObjectIdentifier::Component> components;
  long long int ivalue;
  double fvalue;
  struct {
    std::string name;
    double fvalue = 0;
  } constant;
  std::vector<Expression*> arguments;
  std::vector<Expression*> list;
  std::string string;
  FunctionExpression::Function func;
  ObjectIdentifier::String string_or_identifier;
  semantic_type() : expr(0), ivalue(0), fvalue(0), func(FunctionExpression::NONE) {}
};

#define YYSTYPE semantic_type
#include "ExpressionParser.tab.h"
#undef YYTOKENTYPE
#undef YYSTYPE
#undef YYSTYPE_ISDECLARED
}

}
#endif // EXPRESSION_H
