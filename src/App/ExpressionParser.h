/****************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>              *
 *   Copyright (c) 2019 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/


#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include <Base/Interpreter.h>
#include "Expression.h"

namespace App {

////////////////////////////////////////////////////////////////////////////////////
// Expecting the extended expression is going to be constantly amended (to
// conform to Python), we move most of the class declarations here to avoid
// constant recompiling of the whole FC code base, as the expression header is
// included by everyone
///////////////////////////////////////////////////////////////////////////////////

struct AppExport Expression::Component {
    ObjectIdentifier::Component comp;
    Expression* e1;
    Expression* e2;
    Expression* e3;

    Component(const std::string &n);
    Component(Expression *e1, Expression *e2, Expression *e3, bool isRange=false);
    Component(const ObjectIdentifier::Component &comp);
    Component(const Component &other);
    ~Component();
    Component &operator=(const Component &)=delete;

    void visit(ExpressionVisitor &v);
    bool isTouched() const;
    void toString(std::ostream &ss, bool persistent) const;
    Component *copy() const;
    Component *eval() const;

    Py::Object get(const Expression *owner, const Py::Object &pyobj) const;
    void set(const Expression *owner, Py::Object &pyobj, const Py::Object &value) const;
    void del(const Expression *owner, Py::Object &pyobj) const;
};

////////////////////////////////////////////////////////////////////////////////////

/**
  * Part of an expressions that contains a unit.
  *
  */

class  AppExport UnitExpression : public Expression {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    UnitExpression(const App::DocumentObject *_owner = 0, const Base::Quantity & _quantity = Base::Quantity(), const std::string & _unitStr = std::string());

    ~UnitExpression();

    virtual Expression * simplify() const override;

    void setUnit(const Base::Quantity &_quantity);

    void setQuantity(const Base::Quantity &_quantity);

    double getValue() const { return quantity.getValue(); }

    const Base::Unit & getUnit() const { return quantity.getUnit(); }

    const Base::Quantity & getQuantity() const { return quantity; }

    const std::string getUnitString() const { return unitStr; }

    double getScaler() const { return quantity.getValue(); }

protected:
    virtual Expression * _copy() const override;
    virtual void _toString(std::ostream &ss, bool persistent, int indent) const override;
    virtual Py::Object _getPyValue() const override;

protected:
    mutable PyObject *cache = 0;

private:
    Base::Quantity quantity;
    std::string unitStr; /**< The unit string from the original parsed string */
};

/**
  * Class implementing a number with an optional unit
  */

class AppExport NumberExpression : public UnitExpression {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    NumberExpression(const App::DocumentObject *_owner = 0, const Base::Quantity & quantity = Base::Quantity());

    virtual Expression * simplify() const override;

    void negate();

    bool isInteger(long *v=0) const;

protected:
    virtual Expression * _copy() const override;
    virtual void _toString(std::ostream &ss, bool persistent, int indent) const override;
};

class AppExport ConstantExpression : public NumberExpression {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    ConstantExpression(const App::DocumentObject *_owner = 0,
            const char *_name = "",
            const Base::Quantity &_quantity = Base::Quantity());

    std::string getName() const { return name; }

    bool isNumber() const;

protected:
    virtual Py::Object _getPyValue() const override;
    virtual void _toString(std::ostream &ss, bool persistent, int indent) const override;
    virtual Expression* _copy() const override;

protected:
    const char *name;
};

/**
  * Class implementing an infix expression.
  *
  */

class AppExport OperatorExpression : public UnitExpression {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
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
        POS
    };
    OperatorExpression(const App::DocumentObject *_owner = 0, Expression * _left = 0, Operator _op = NONE, Expression * _right = 0);

    virtual ~OperatorExpression();

    virtual bool isTouched() const override;

    virtual Expression * simplify() const override;

    virtual int priority() const override;

    Operator getOperator() const { return op; }

    Expression * getLeft() const { return left; }

    Expression * getRight() const { return right; }

protected:
    virtual Expression * _copy() const override;

    virtual Py::Object _getPyValue() const override;

    virtual void _toString(std::ostream &ss, bool persistent, int indent) const override;

    virtual void _visit(ExpressionVisitor & v) override;

    virtual bool isCommutative() const;

    virtual bool isLeftAssociative() const;

    virtual bool isRightAssociative() const;

    Operator op;        /**< Operator working on left and right */
    Expression * left;  /**< Left operand */
    Expression * right; /**< Right operand */
};

class AppExport ConditionalExpression : public Expression {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    ConditionalExpression(const App::DocumentObject *_owner = 0, Expression * _condition = 0,Expression * _trueExpr = 0,  Expression * _falseExpr = 0);

    virtual ~ConditionalExpression();

    virtual bool isTouched() const override;

    virtual Expression * simplify() const override;

    virtual int priority() const override;

protected:
    virtual Expression * _copy() const override;
    virtual void _visit(ExpressionVisitor & v) override;
    virtual void _toString(std::ostream &ss, bool persistent, int indent) const override;
    virtual Py::Object _getPyValue() const override;

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
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
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
        LIST,
        TUPLE,
        MSCALE, // matrix scale by vector
        MINVERT, // invert matrix/placement/rotation
        CREATE, // create new object of a given type

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

    FunctionExpression(const App::DocumentObject *_owner = 0, Function _f = NONE,
            std::string &&name = std::string(), std::vector<Expression *> _args = std::vector<Expression*>());

    virtual ~FunctionExpression();

    virtual bool isTouched() const override;

    virtual Expression * simplify() const override;

    static Py::Object evaluate(const Expression *owner, int type, const std::vector<Expression*> &args);

protected:
    static Py::Object evalAggregate(const Expression *owner, int type, const std::vector<Expression*> &args);
    virtual Py::Object _getPyValue() const override;
    virtual Expression * _copy() const override;
    virtual void _visit(ExpressionVisitor & v) override;
    virtual void _toString(std::ostream &ss, bool persistent, int indent) const override;

    Function f;        /**< Function to execute */
    std::string fname;
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
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    VariableExpression(const App::DocumentObject *_owner = 0, const ObjectIdentifier& _var = ObjectIdentifier());

    ~VariableExpression();

    virtual bool isTouched() const override;

    virtual Expression * simplify() const override;

    std::string name() const { return var.getPropertyName(); }

    ObjectIdentifier getPath() const { return var; }

    void setPath(const ObjectIdentifier & path);

    const App::Property *getProperty() const;

    virtual void addComponent(Component* component) override;

protected:
    virtual Expression * _copy() const override;
    virtual Py::Object _getPyValue() const override;
    virtual void _toString(std::ostream &ss, bool persistent, int indent) const override;
    virtual bool _isIndexable() const override;
    virtual void _getDeps(ExpressionDeps &) const override;
    virtual void _getDepObjects(std::set<App::DocumentObject*> &, std::vector<std::string> *) const override;
    virtual void _getIdentifiers(std::set<App::ObjectIdentifier> &) const override;
    virtual bool _adjustLinks(const std::set<App::DocumentObject*> &, ExpressionVisitor &) override;
    virtual void _importSubNames(const ObjectIdentifier::SubNameMap &) override;
    virtual void _updateLabelReference(App::DocumentObject *, const std::string &, const char *) override;
    virtual bool _updateElementReference(App::DocumentObject *,bool,ExpressionVisitor &) override;
    virtual bool _relabeledDocument(const std::string &, const std::string &, ExpressionVisitor &) override;
    virtual bool _renameObjectIdentifier(const std::map<ObjectIdentifier,ObjectIdentifier> &,
                                         const ObjectIdentifier &, ExpressionVisitor &) override;
    virtual void _collectReplacement(std::map<ObjectIdentifier,ObjectIdentifier> &,
                    const App::DocumentObject *parent, App::DocumentObject *oldObj,
                    App::DocumentObject *newObj) const override;
    virtual void _moveCells(const CellAddress &, int, int, ExpressionVisitor &) override;
    virtual void _offsetCells(int, int, ExpressionVisitor &) override;

protected:

    ObjectIdentifier var; /**< Variable name  */
};

//////////////////////////////////////////////////////////////////////

class AppExport PyObjectExpression : public Expression {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PyObjectExpression(const App::DocumentObject *_owner=0, PyObject *pyobj=0, bool owned=false)
        :Expression(_owner)
    {
        setPyValue(pyobj,owned);
    }

    virtual ~PyObjectExpression();

    void setPyValue(Py::Object pyobj);
    void setPyValue(PyObject *pyobj, bool owned=false);
    virtual Expression * simplify() const override { return copy(); }

protected:
    virtual Expression* _copy() const override;
    virtual void _toString(std::ostream &ss, bool persistent, int indent) const override;
    virtual Py::Object _getPyValue() const override;

protected:
    PyObject *pyObj = 0;
};

/**
  * Class implementing a string. Used to signal either a genuine string or
  * a failed evaluation of an expression.
  */

class AppExport StringExpression : public Expression {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    StringExpression(const App::DocumentObject *_owner = 0, const std::string & _text = std::string());
    ~StringExpression();

    virtual Expression * simplify() const override;

    virtual std::string getText() const { return text; }
protected:
    virtual Expression * _copy() const override;
    virtual void _toString(std::ostream &ss, bool persistent, int indent) const override;
    virtual Py::Object _getPyValue() const override;
    virtual bool _isIndexable() const override { return true; }

private:
    std::string text; /**< Text string */
    mutable PyObject *cache = 0;
};

class AppExport RangeExpression : public App::Expression {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    RangeExpression(const App::DocumentObject * _owner = 0, const std::string & begin = std::string(), const std::string & end = std::string());

    virtual ~RangeExpression() { }

    virtual bool isTouched() const override;

    virtual App::Expression * simplify() const override;

    Range getRange() const;

protected:
    virtual Expression * _copy() const override;
    virtual void _toString(std::ostream &ss, bool persistent, int indent) const override;
    virtual Py::Object _getPyValue() const override;
    virtual void _getDeps(ExpressionDeps &) const override;
    virtual bool _renameObjectIdentifier(const std::map<ObjectIdentifier,ObjectIdentifier> &,
                                         const ObjectIdentifier &, ExpressionVisitor &) override;
    virtual void _moveCells(const CellAddress &, int, int, ExpressionVisitor &) override;
    virtual void _offsetCells(int, int, ExpressionVisitor &) override;

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
  Expression::Component *component;
  Expression * expr;
  ObjectIdentifier path;
  std::deque<ObjectIdentifier::Component> components;
  long long int ivalue;
  double fvalue;
  struct {
    const char *name = "";
    double fvalue = 0;
  } constant;
  std::vector<Expression*> arguments;
  std::vector<Expression*> list;
  std::string string;
  std::pair<FunctionExpression::Function,std::string> func;
  ObjectIdentifier::String string_or_identifier;
  semantic_type() : component(0), expr(0), ivalue(0), fvalue(0)
                  , func({FunctionExpression::NONE, std::string()}) {}
};

#define YYSTYPE semantic_type
#include "ExpressionParser.tab.h"
#undef YYTOKENTYPE
#undef YYSTYPE
#undef YYSTYPE_ISDECLARED
}

}

#endif //EXPRESSION_PARSER_H
