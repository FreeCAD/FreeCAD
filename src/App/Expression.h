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

typedef std::map<App::DocumentObject*, std::map<std::string, std::vector<ObjectIdentifier> > > ExpressionDeps;

class AppExport ExpressionVisitor {
public:
    virtual ~ExpressionVisitor() {}
    virtual void visit(Expression * e) = 0;
    virtual void setExpressionChanged() {}
    virtual bool getChanged() const { return false;}

protected:
    void getIdentifiers(Expression &e, std::set<App::ObjectIdentifier> &); 
    void getDeps(Expression &e, ExpressionDeps &); 
    void getDepObjects(Expression &e, std::set<App::DocumentObject*> &); 
    bool adjustLinks(Expression &e, const std::set<App::DocumentObject*> &inList);
    bool renameDocumentObject(Expression &e, const App::DocumentObject *);
    bool renameDocument(Expression &e, const std::string &oldName, const std::string &newName);
};

template<class P> class ExpressionModifier : public ExpressionVisitor {
public:
    ExpressionModifier(P & _prop)
        : prop(_prop) { }

    virtual ~ExpressionModifier() { }

    virtual void setExpressionChanged() override{
        if (!signaller)
            signaller = boost::shared_ptr<typename AtomicPropertyChangeInterface<P>::AtomicPropertyChange>(AtomicPropertyChangeInterface<P>::getAtomicPropertyChange(prop));
    }

    virtual bool getChanged() const override { return signaller != 0; }

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

    Expression(const Expression&) = delete;
    void operator=(const Expression &)=delete;

    virtual bool isTouched() const { return false; }

    Expression * eval() const;

    std::string toString() const;

    Expression *copy() const;

    static Expression * parse(const App::DocumentObject * owner, const std::string& buffer);

    virtual int priority() const { return 0; }

    void getIdentifiers(std::set<App::ObjectIdentifier> &) const;
    std::set<App::ObjectIdentifier> getIdentifiers() const;

    void getDeps(ExpressionDeps &deps) const;
    ExpressionDeps getDeps() const;

    std::set<App::DocumentObject*> getDepObjects() const;
    void getDepObjects(std::set<App::DocumentObject*> &) const;

    bool adjustLinks(const std::set<App::DocumentObject*> &inList);

    virtual Expression *simplify() const { return copy(); }

    virtual void visit(ExpressionVisitor & v) { v.visit(this); }

    class Exception : public Base::Exception {
    public:
        Exception(const char *sMessage) : Base::Exception(sMessage) { }
    };

    App::DocumentObject *  getOwner() const { return owner; }

    boost::any getValueAsAny() const;

    void addComponent(const ObjectIdentifier::Component &component) {
        components.push_back(component);
    }

protected:
    virtual bool _isIndexable() const {return false;}
    virtual Expression *_copy() const = 0;
    virtual void _toString(std::ostringstream &ss) const = 0;
    virtual void _getDeps(ExpressionDeps &) const  {}
    virtual void _getDepObjects(std::set<App::DocumentObject*> &) const  {}
    virtual void _getIdentifiers(std::set<App::ObjectIdentifier> &) const  {}
    virtual bool _adjustLinks(const std::set<App::DocumentObject*> &, ExpressionVisitor &) {return false;}
    virtual bool _renameDocumentObject(const App::DocumentObject *, ExpressionVisitor &) {return false;}
    virtual bool _renameDocument(const std::string &, const std::string &, ExpressionVisitor &) {return false;}
    virtual boost::any _getValueAsAny() const = 0;
    virtual Expression *_eval() const {return 0;}

    Expression *fromAny(boost::any) const;

    friend ExpressionVisitor;

protected:
    App::DocumentObject * owner; /**< The document object used to access unqualified variables (i.e local scope) */

    std::vector<ObjectIdentifier::Component> components;
};

/**
  * Part of an expressions that contains a unit.
  *
  */

class  AppExport UnitExpression : public Expression {
    TYPESYSTEM_HEADER();
public:
    UnitExpression(const App::DocumentObject *_owner = 0, const Base::Quantity & _quantity = Base::Quantity(), const std::string & _unitStr = std::string());

    virtual Expression * simplify() const;

    virtual int priority() const;

    void setUnit(const Base::Quantity &_quantity);

    double getValue() const { return quantity.getValue(); }

    const Base::Unit & getUnit() const { return quantity.getUnit(); }

    const Base::Quantity & getQuantity() const { return quantity; }

    const std::string getUnitString() const { return unitStr; }

    double getScaler() const { return quantity.getValue(); }

protected:
    virtual void _toString(std::ostringstream &ss) const;
    virtual Expression *_copy() const;
    virtual Expression *_eval() const;
    virtual boost::any _getValueAsAny() const { 
        return quantity.getUnit().isEmpty() ? boost::any(quantity.getValue()) : boost::any(quantity); 
    }

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

    virtual int priority() const;

    void negate();

protected:
    virtual void _toString(std::ostringstream &ss) const;
    virtual Expression *_copy() const;
};

class AppExport ConstantExpression : public NumberExpression {
    TYPESYSTEM_HEADER();
public:
    ConstantExpression(const App::DocumentObject *_owner = 0, std::string _name = "", const Base::Quantity &_quantity = Base::Quantity());

    virtual int priority() const;

    std::string getName() const { return name; }

    virtual Expression *simplify() const;

protected:
    virtual Expression *_eval() const;
    virtual boost::any _getValueAsAny() const;
    virtual void _toString(std::ostringstream &ss) const;
    virtual Expression *_copy() const;

    std::string name; /**< Constant's name */
};

class AppExport BooleanExpression : public NumberExpression {
    TYPESYSTEM_HEADER();
public:
    BooleanExpression(const App::DocumentObject *_owner = 0, bool _value = false);

protected:
    virtual Expression *_copy() const;
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
        MOD,
        POW,
        EQ,
        NEQ,
        LT,
        GT,
        LTE,
        GTE,
        UNIT,
        NEG,
        POS,
        AND_OP,
        AND_OP2,
        OR_OP,
        OR_OP2,
    };
    OperatorExpression(const App::DocumentObject *_owner = 0, Expression * _left = 0, Operator _op = NONE, Expression * _right = 0);

    virtual ~OperatorExpression();

    virtual bool isTouched() const;

    virtual Expression * simplify() const;

    virtual int priority() const;

    virtual void visit(ExpressionVisitor & v);

    Operator getOperator() const { return op; }

    Expression * getLeft() const { return left; }

    Expression * getRight() const { return right; }

protected:
    boost::any _calc(boost::any l, boost::any r) const;
    Expression *_calc(Expression *l, Expression *r) const;
    Expression *_calc(Expression *l) const;

    virtual void _toString(std::ostringstream &ss) const;
    virtual Expression *_copy() const;
    virtual Expression *_eval() const;
    virtual boost::any _getValueAsAny() const;

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

    virtual Expression *simplify() const;

    virtual int priority() const;

    virtual void visit(ExpressionVisitor & v);

protected:
    virtual void _toString(std::ostringstream &ss) const;
    virtual Expression *_copy() const;
    virtual Expression *_eval() const;
    virtual boost::any _getValueAsAny() const;

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
    FunctionExpression(const App::DocumentObject *_owner=0, int f=0,
            const std::vector<Expression *> &_args = {});

    virtual ~FunctionExpression();

    virtual bool isTouched() const;

    virtual Expression * simplify() const;

    virtual int priority() const;

    virtual void visit(ExpressionVisitor & v);

protected:
    virtual void _toString(std::ostringstream &ss) const;
    virtual Expression *_copy() const;
    virtual Expression *_eval() const;
    virtual boost::any _getValueAsAny() const;
    Expression *evalAggregate() const;

    int f;        /**< Function to execute */
    std::vector<Expression *> args; /** Arguments to function*/
};

/**
  * Class implementing a reference to a property. If the name is unqualified,
  * the owner of the expression is searched. If it is qualified, the document
  * that contains the owning document object is searched for other document
  * objects to search. Both labels and internal document names are searched.
  *
  */

class AppExport VariableExpression : public Expression {
    TYPESYSTEM_HEADER();
public:
    VariableExpression(const App::DocumentObject *_owner = 0, ObjectIdentifier _var = ObjectIdentifier());

    ~VariableExpression();

    virtual bool isTouched() const;

    virtual int priority() const;

    std::string name() const { return var.getPropertyName(); }

    const ObjectIdentifier &getPath() const { return var; }

    void setPath(const ObjectIdentifier & path);

protected:
    virtual bool _isIndexable() const;
    virtual void _toString(std::ostringstream &ss) const;
    virtual Expression *_copy() const;
    virtual void _getDeps(ExpressionDeps &) const;
    virtual void _getDepObjects(std::set<App::DocumentObject*> &) const;
    virtual void _getIdentifiers(std::set<App::ObjectIdentifier> &) const;
    virtual bool _adjustLinks(const std::set<App::DocumentObject*> &, ExpressionVisitor &);
    virtual bool _renameDocumentObject(const App::DocumentObject *, ExpressionVisitor &);
    virtual bool _renameDocument(const std::string &, const std::string &, ExpressionVisitor &);
    virtual boost::any _getValueAsAny() const;

protected:

    ObjectIdentifier var; /**< Variable name  */
};

/**
  * Class implementing a callable expression with named arguments and optional trailing accessor
  */

class AppExport CallableExpression : public Expression {
    TYPESYSTEM_HEADER();
public:
    CallableExpression(const App::DocumentObject *_owner = 0, Expression *expr=0, 
            const std::vector<std::pair<std::string,Expression *> > &_args = {});

    static Expression *create(const App::DocumentObject *owner, const ObjectIdentifier &path,
            const std::vector<std::pair<std::string,Expression *> > &args);

    static Py::Object evaluate(const App::DocumentObject *owner, 
            const std::string &expr, PyObject *tuple, PyObject *kwds);

    virtual ~CallableExpression();

    virtual bool isTouched() const;

    virtual void visit(ExpressionVisitor & v);

protected:
    virtual bool _isIndexable() const { return true; }
    virtual Expression *_eval() const;
    virtual boost::any _getValueAsAny() const;
    virtual void _toString(std::ostringstream &ss) const;
    virtual Expression *_copy() const;

protected:
    Expression *expr;
    std::string name;
    int ftype;
    std::vector<std::pair<std::string,Expression *> > args;
};

/**
  * Class implementing a string. Used to signal either a genuine string or
  * a failed evaluation of an expression.
  */
class AppExport StringExpression : public Expression {
    TYPESYSTEM_HEADER();
public:
    StringExpression(const App::DocumentObject *_owner = 0, 
            const std::string & _text = std::string(), bool r_literal=false);

    virtual const std::string &getText() const { return text; }

    virtual int priority() const;

    bool rLiteral() const {return r_literal;}

protected:
    virtual bool _isIndexable() const { return true; }
    virtual Expression * _copy() const;
    virtual void _toString(std::ostringstream &ss) const;
    virtual boost::any _getValueAsAny() const;

protected:

    std::string text; /**< Text string */
    bool r_literal;
};

//////////////////////////////////////////////////////////////////////

class AppExport RangeExpression : public App::Expression {
    TYPESYSTEM_HEADER();
public:
    RangeExpression(const App::DocumentObject * _owner = 0, const std::string & begin = std::string(), const std::string & end = std::string());

    virtual ~RangeExpression() { }

    virtual bool isTouched() const;

    virtual int priority() const;

    Range getRange() const { return range; }

    void setRange(const Range & r);

protected:
    virtual void _toString(std::ostringstream &) const;
    virtual Expression *_copy() const;
    virtual void _getDeps(ExpressionDeps &) const;
    virtual void _getDepObjects(std::set<App::DocumentObject*> &) const;
    virtual boost::any _getValueAsAny() const;

protected:
    Range range;
};

//////////////////////////////////////////////////////////////////////

class AppExport PyObjectExpression : public Expression {
    TYPESYSTEM_HEADER();

public:
    PyObjectExpression(const App::DocumentObject * _owner=0, Py::Object obj=Py::Object());

    virtual int priority() const;

    Py::Object getPyObject() const { return pyObj; }

    void setPyObject(Py::Object pyobj) {pyObj = pyobj;}

protected:
    virtual boost::any _getValueAsAny() const;
    virtual void _toString(std::ostringstream &) const;
    virtual Expression *_copy() const;

protected:
    Py::Object pyObj;
};

//////////////////////////////////////////////////////////////////////

class AppExport ListExpression : public Expression {
    TYPESYSTEM_HEADER();

public:
    ListExpression(const App::DocumentObject * _owner=0, const std::vector<Expression *> &items={});
    virtual ~ListExpression();

    virtual int priority() const;

    void addItem(Expression *item);

protected:
    virtual bool _isIndexable() const {return true;}
    virtual boost::any _getValueAsAny() const;
    virtual void _toString(std::ostringstream &) const;
    virtual Expression *_copy() const;

protected:
    std::vector<Expression*> items;
};

//////////////////////////////////////////////////////////////////////

class AppExport TupleExpression : public ListExpression {
    TYPESYSTEM_HEADER();

public:
    TupleExpression(const App::DocumentObject * _owner=0, Expression *item=0);
    TupleExpression(const App::DocumentObject * _owner, const std::vector<Expression *> &);

protected:
    virtual boost::any _getValueAsAny() const;
    virtual Expression *_copy() const;
    virtual void _toString(std::ostringstream &) const;
};

//////////////////////////////////////////////////////////////////////

class AppExport DictExpression : public Expression {
    TYPESYSTEM_HEADER();

public:
    DictExpression(const App::DocumentObject * _owner=0, Expression *key=0, Expression *value=0);
    virtual ~DictExpression();

    virtual int priority() const;

    void addItem(Expression *key, Expression *value);

protected:
    virtual bool _isIndexable() const {return true;}
    virtual boost::any _getValueAsAny() const;
    virtual void _toString(std::ostringstream &) const;
    virtual Expression *_copy() const;

protected:
    std::vector<std::pair<Expression*,Expression*> > items;
};

//////////////////////////////////////////////////////////////////////
//
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
  ObjectIdentifier::Component component;
  long long int ivalue;
  double fvalue;
  struct {
    std::string name;
    double fvalue;
  } constant;
  std::vector<Expression*> arguments;
  std::vector<std::pair<std::string,Expression*> > named_arguments;
  std::pair<std::string,Expression*> named_argument;
  std::string string;
  ObjectIdentifier::String string_or_identifier;
  semantic_type() : expr(0), ivalue(0), fvalue(0) {}
};

}

}
#endif // EXPRESSION_H
