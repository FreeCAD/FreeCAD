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
#include <App/PropertyLinks.h>
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

typedef std::unique_ptr<Expression> ExpressionPtr;

AppExport bool isAnyEqual(const App::any &v1, const App::any &v2);
AppExport Base::Quantity anyToQuantity(const App::any &value, const char *errmsg = 0);

typedef std::map<App::DocumentObject*, std::map<std::string, std::vector<ObjectIdentifier> > > ExpressionDeps;
class AppExport ExpressionVisitor {
public:
    virtual ~ExpressionVisitor() {}
    virtual void visit(Expression &e) = 0;
    virtual void aboutToChange() {}
    virtual int changed() const { return 0;}
    virtual void reset() {}
    virtual App::PropertyLinkBase* getPropertyLink() {return 0;}

protected:
    void getIdentifiers(Expression &e, std::set<App::ObjectIdentifier> &); 
    void getDeps(Expression &e, ExpressionDeps &); 
    void getDepObjects(Expression &e, std::set<App::DocumentObject*> &, std::vector<std::string> *); 
    bool adjustLinks(Expression &e, const std::set<App::DocumentObject*> &inList);
    bool relabeledDocument(Expression &e, const std::string &oldName, const std::string &newName);
    bool renameObjectIdentifier(Expression &e,
            const std::map<ObjectIdentifier,ObjectIdentifier> &, const ObjectIdentifier &);
    void collectReplacement(Expression &e, std::map<ObjectIdentifier,ObjectIdentifier> &, 
            const App::DocumentObject *parent, App::DocumentObject *oldObj, App::DocumentObject *newObj) const;
    bool updateElementReference(Expression &e, App::DocumentObject *feature,bool reverse);
    void importSubNames(Expression &e, const ObjectIdentifier::SubNameMap &subNameMap);
    void updateLabelReference(Expression &e, App::DocumentObject *obj, 
            const std::string &ref, const char *newLabel);
    void moveCells(Expression &e, const CellAddress &address, int rowCount, int colCount);
    void offsetCells(Expression &e, int rowOffset, int colOffset);
};

template<class P> class ExpressionModifier : public ExpressionVisitor {
public:
    ExpressionModifier(P & _prop)
        : prop(_prop)
        , propLink(Base::freecad_dynamic_cast<App::PropertyLinkBase>(&prop))
        , signaller(_prop,false)
        , _changed(0) 
    {}

    virtual ~ExpressionModifier() { }

    virtual void aboutToChange() override{
        ++_changed;
        signaller.aboutToChange();
    }

    virtual int changed() const override { return _changed; }

    virtual void reset() override {_changed = 0;}

    virtual App::PropertyLinkBase* getPropertyLink() override {return propLink;}

protected:
    P & prop;
    App::PropertyLinkBase *propLink;
    typename AtomicPropertyChangeInterface<P>::AtomicPropertyChange signaller;
    int _changed;
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

    virtual std::string toString(bool persistent=false) const = 0;

    static Expression * parse(const App::DocumentObject * owner, const std::string& buffer);

    Expression * copy() const;

    virtual int priority() const { return 0; }

    void getIdentifiers(std::set<App::ObjectIdentifier> &) const;
    std::set<App::ObjectIdentifier> getIdentifiers() const;

    void getDeps(ExpressionDeps &deps) const;
    ExpressionDeps getDeps() const;

    std::set<App::DocumentObject*> getDepObjects(std::vector<std::string> *labels=0) const;
    void getDepObjects(std::set<App::DocumentObject*> &, std::vector<std::string> *labels=0) const;

    ExpressionPtr importSubNames(const std::map<std::string,std::string> &nameMap) const;

    ExpressionPtr updateLabelReference(App::DocumentObject *obj, 
            const std::string &ref, const char *newLabel) const;

    ExpressionPtr replaceObject(const App::DocumentObject *parent,
            App::DocumentObject *oldObj, App::DocumentObject *newObj) const;

    bool adjustLinks(const std::set<App::DocumentObject*> &inList);

    virtual Expression * simplify() const = 0;

    void visit(ExpressionVisitor & v);

    class Exception : public Base::Exception {
    public:
        Exception(const char *sMessage) : Base::Exception(sMessage) { }
    };

    App::DocumentObject *  getOwner() const { return owner; }

    virtual boost::any getValueAsAny() const { static boost::any empty; return empty; }

    bool isSame(const Expression &other) const;

    friend ExpressionVisitor;

protected:
    virtual Expression *_copy() const = 0;
    virtual void _getDeps(ExpressionDeps &) const  {}
    virtual void _getDepObjects(std::set<App::DocumentObject*> &, std::vector<std::string> *) const  {}
    virtual void _getIdentifiers(std::set<App::ObjectIdentifier> &) const  {}
    virtual bool _adjustLinks(const std::set<App::DocumentObject*> &, ExpressionVisitor &) {return false;}
    virtual bool _updateElementReference(App::DocumentObject *,bool,ExpressionVisitor &) {return false;}
    virtual bool _relabeledDocument(const std::string &, const std::string &, ExpressionVisitor &) {return false;}
    virtual void _importSubNames(const ObjectIdentifier::SubNameMap &) {}
    virtual void _updateLabelReference(App::DocumentObject *, const std::string &, const char *) {}
    virtual bool _renameObjectIdentifier(const std::map<ObjectIdentifier,ObjectIdentifier> &, 
                                         const ObjectIdentifier &, ExpressionVisitor &) {return false;}
    virtual void _collectReplacement(std::map<ObjectIdentifier,ObjectIdentifier> &,
        const App::DocumentObject *parent, App::DocumentObject *oldObj, App::DocumentObject *newObj) const 
    {
        (void)parent;
        (void)oldObj;
        (void)newObj;
    }
    virtual void _moveCells(const CellAddress &, int, int, ExpressionVisitor &) {}
    virtual void _offsetCells(int, int, ExpressionVisitor &) {}
    virtual void _visit(ExpressionVisitor &) {}

protected:
    App::DocumentObject * owner; /**< The document object used to access unqualified variables (i.e local scope) */

public:
    std::string comment;
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

    virtual std::string toString(bool persistent=false) const;

    virtual Expression * _copy() const;

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

    virtual Expression * _copy() const;

    virtual int priority() const;

    void negate();

    virtual std::string toString(bool persistent=false) const;

    bool isInteger(long *v=0) const;

protected:
};

class AppExport ConstantExpression : public NumberExpression {
    TYPESYSTEM_HEADER();
public:
    ConstantExpression(const App::DocumentObject *_owner = 0, std::string _name = "", const Base::Quantity &_quantity = Base::Quantity());

    virtual std::string toString(bool persistent=false) const;

    virtual Expression * _copy() const;

    virtual int priority() const;

    std::string getName() const { return name; }

protected:
    std::string name; /**< Constant's name */
};

class AppExport BooleanExpression : public NumberExpression {
    TYPESYSTEM_HEADER();
public:
    BooleanExpression(const App::DocumentObject *_owner = 0, bool _value = false);

    virtual Expression * _copy() const;

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

    virtual std::string toString(bool persistent=false) const;

    virtual Expression * _copy() const;

    virtual int priority() const;

    virtual void _visit(ExpressionVisitor & v);

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

    virtual std::string toString(bool persistent=false) const;

    virtual Expression * _copy() const;

    virtual int priority() const;

    virtual void _visit(ExpressionVisitor & v);

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

    virtual std::string toString(bool persistent=false) const;

    virtual Expression * _copy() const;

    virtual int priority() const;

    virtual void _visit(ExpressionVisitor & v);

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

    virtual std::string toString(bool persistent=false) const;

    virtual Expression * _copy() const;

    virtual int priority() const;

    std::string name() const { return var.getPropertyName(); }

    ObjectIdentifier getPath() const { return var; }

    void setPath(const ObjectIdentifier & path);

    const App::Property *getProperty() const;

protected:
    virtual void _getDeps(ExpressionDeps &) const;
    virtual void _getDepObjects(std::set<App::DocumentObject*> &, std::vector<std::string> *) const;
    virtual void _getIdentifiers(std::set<App::ObjectIdentifier> &) const;
    virtual bool _adjustLinks(const std::set<App::DocumentObject*> &, ExpressionVisitor &);
    virtual void _importSubNames(const ObjectIdentifier::SubNameMap &);
    virtual void _updateLabelReference(App::DocumentObject *, const std::string &, const char *);
    virtual bool _updateElementReference(App::DocumentObject *,bool,ExpressionVisitor &);
    virtual bool _relabeledDocument(const std::string &, const std::string &, ExpressionVisitor &);
    virtual bool _renameObjectIdentifier(const std::map<ObjectIdentifier,ObjectIdentifier> &, 
                                         const ObjectIdentifier &, ExpressionVisitor &);
    virtual void _collectReplacement(std::map<ObjectIdentifier,ObjectIdentifier> &, 
                    const App::DocumentObject *parent, App::DocumentObject *oldObj, 
                    App::DocumentObject *newObj) const;
    virtual void _moveCells(const CellAddress &, int, int, ExpressionVisitor &);
    virtual void _offsetCells(int, int, ExpressionVisitor &);

protected:

    ObjectIdentifier var; /**< Variable name  */
};

//////////////////////////////////////////////////////////////////////

class AppExport PyObjectExpression : public Expression {
    TYPESYSTEM_HEADER();

public:
    PyObjectExpression(const App::DocumentObject *_owner=0, PyObject *pyobj=0, bool owned=false)
        :Expression(_owner)
    {
        setPyObject(pyobj,owned);
    }

    virtual ~PyObjectExpression();

    Py::Object getPyObject() const;

    void setPyObject(Py::Object pyobj);
    void setPyObject(PyObject *pyobj, bool owned=false);

    virtual std::string toString(bool) const;
    virtual boost::any getValueAsAny() const;

    virtual Expression * eval() const { return copy(); }
    virtual Expression * simplify() const { return copy(); }

protected:
    virtual Expression* _copy() const;

protected:
    PyObject *pyObj = 0;
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

    virtual std::string toString(bool persistent=false) const;

    virtual std::string getText() const { return text; }

    virtual int priority() const;

    virtual Expression * _copy() const;

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

    virtual std::string toString(bool persistent=false) const;

    virtual Expression * _copy() const;

    virtual int priority() const;

    virtual App::Expression * simplify() const;

    Range getRange() const;

protected:
    virtual void _getDeps(ExpressionDeps &) const;
    virtual bool _renameObjectIdentifier(const std::map<ObjectIdentifier,ObjectIdentifier> &, 
                                         const ObjectIdentifier &, ExpressionVisitor &);
    virtual void _moveCells(const CellAddress &, int, int, ExpressionVisitor &);
    virtual void _offsetCells(int, int, ExpressionVisitor &);

protected:
    std::string begin;
    std::string end;
};

namespace ExpressionParser {
AppExport Expression * parse(const App::DocumentObject *owner, const char *buffer);
AppExport UnitExpression * parseUnit(const App::DocumentObject *owner, const char *buffer);
AppExport ObjectIdentifier parsePath(const App::DocumentObject *owner, const char* buffer);
AppExport bool isTokenAnIndentifier(const std::string & str);
AppExport bool isTokenAUnit(const std::string & str);
AppExport std::vector<boost::tuple<int, int, std::string> > tokenize(const std::string & str);

/// Convenient class to mark begin of importing
class AppExport ExpressionImporter {
public:
    ExpressionImporter(Base::XMLReader &reader);
    ~ExpressionImporter();
    static Base::XMLReader *reader();
};

AppExport bool isModuleImported(PyObject *);

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
