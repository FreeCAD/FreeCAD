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
#include <App/DocumentObject.h>
#include <Base/Exception.h>
#include <Base/Unit.h>
#include <App/Property.h>
#include <Base/BaseClass.h>
#include <Base/Quantity.h>
#include <Mod/Spreadsheet/App/Range.h>
#include <set>

namespace Spreadsheet  {

class Expression;

class SpreadsheetExport ExpressionVisitor {
public:
    virtual ~ExpressionVisitor() {}
    virtual void visit(Expression * e) = 0;
};

class SpreadsheetExport Path {

public:

    class String {
    public:
        String(const std::string & s = "", bool _isRealString = false) : str(s), isString(_isRealString) { }

        std::string getString() const { return str; }

        operator std::string() const { return str; }

        operator const char *() const { return str.c_str(); }

        bool operator==(const String & other) const { return str == other.str; }

        bool operator!=(const String & other) const { return str != other.str; }

        bool operator>=(const String & other) const { return str >= other.str; }

        bool operator<(const String & other) const { return str < other.str; }

        bool operator>(const String & other) const { return str > other.str; }

        std::string toString() const;

        bool isRealString() const { return isString; }

        std::string str;
        bool isString;
    };

    struct SpreadsheetExport Component {

        enum typeEnum {
            SIMPLE,
            MAP,
            ARRAY
        } ;

        std::string component;
        typeEnum type;
        int index;
        String key;
        bool keyIsString;

        Component(const std::string & _component, typeEnum _type = SIMPLE, int _index = -1, String _key = String());

        static Component SimpleComponent(const std::string & _component);

        static Component ArrayComponent(const std::string & _component, int _index);

        static Component MapComponent(const std::string & _component, const String &_key);

        bool operator==(const Component & other) const;

        bool isSimple() const { return type == SIMPLE; }

        bool isMap() const { return type == MAP; }

        bool isArray() const { return type == ARRAY; }

        std::string toString() const;

    };

    Path(const App::DocumentObject * _owner = 0, const std::string & property = std::string());

    void addComponent(const Component &c) { components.push_back(c); resolve(); }

    template<typename C>
    void addComponents(const C &cs) { components.insert(components.end(), cs.begin(), cs.end()); resolve(); }

    void setDocumentName(const String & name, bool force = false) { documentName = name; documentNameSet = force; }

    const String getDocumentName() const { return documentName; }

    void setDocumentObjectName(const String & name, bool force = false) { documentObjectName = name; documentObjectNameSet = force; }

    const String getDocumentObjectName() const { return documentObjectName; }

    const std::string & getPropertyName() const { return components[propertyIndex].component; }

    const Component & getPropertyComponent(int i) const { assert(i >=0 && i < components.size()); return components[propertyIndex + i]; }

    const std::string & getSubComponent(int i) const { assert(i >= 1); return components[propertyIndex - 1].component; }

    std::string getSubPathStr() const;

    bool operator==(const Path & other) const;

    bool operator!=(const Path & other) const { return !(operator==)(other); }

    bool operator<(const Path &other) const;

    int numComponents() const;

    static Path parse(const App::DocumentObject * _owner, const char * expr);

    virtual std::string toString() const;

    void resolve();

    void resetResolve();

    const App::Property *getProperty() const;

    std::string getPythonAccessor() const;

    void renameDocumentObject(const std::string & oldName, const std::string & newName);

    void renameDocument(const std::string &oldName, const std::string &newName);

    App::Document *getDocument() const;

protected:

    const App::DocumentObject *getDocumentObject(const App::Document *doc, const std::string &name) const;

    const App::DocumentObject * owner;
    mutable int propertyIndex;
    String documentName;
    bool documentNameSet;
    String documentObjectName;
    bool documentObjectNameSet;
    std::string propertyName;

    std::vector<Component> components;
};

/**
  * Base class for expressions.
  *
  */

class SpreadsheetExport Expression : public Base::BaseClass {
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

    virtual void getDeps(std::set<Path> &props) const { }

    virtual Expression * simplify() const = 0;

    virtual void visit(ExpressionVisitor & v) { v.visit(this); }

    class Exception : public Base::Exception {
    public:
        Exception(const char *sMessage) : Base::Exception(sMessage) { }
    };

    const App::DocumentObject *  getOwner() const { return owner; }

protected:
    const App::DocumentObject * owner; /**< The document object used to access unqualified variables (i.e local scope) */
};

/**
  * Part of an expressions that contains a unit.
  *
  */

class  SpreadsheetExport UnitExpression : public Expression {
    TYPESYSTEM_HEADER();
public:
    UnitExpression(const App::DocumentObject *_owner = 0, const Base::Quantity & _quantity = Base::Quantity(), const std::string & _unitStr = std::string());

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const { return 10; }

    void setUnit(const Base::Quantity &_quantity);

    double getValue() const { return quantity.getValue(); }

    const Base::Unit & getUnit() const { return quantity.getUnit(); }

    const Base::Quantity & getQuantity() const { return quantity; }

    const std::string getUnitString() const { return unitStr; }

    double getScaler() const { return quantity.getValue(); }

protected:
    Base::Quantity quantity;
    std::string unitStr; /**< The unit string from the original parsed string */
};

/**
  * Class implementing a number with an optional unit
  */

class SpreadsheetExport NumberExpression : public UnitExpression {
    TYPESYSTEM_HEADER();
public:
    NumberExpression(const App::DocumentObject *_owner = 0, const Base::Quantity & quantity = Base::Quantity());

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual Expression * copy() const;

    virtual int priority() const { return 10; }

    void negate();

    virtual std::string toString() const;

protected:
};

class SpreadsheetExport ConstantExpression : public NumberExpression {
    TYPESYSTEM_HEADER();
public:
    ConstantExpression(const App::DocumentObject *_owner = 0, std::string _name = "", const Base::Quantity &_quantity = Base::Quantity());

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const { return 10; }

    std::string getName() const { return name; }

protected:
    std::string name; /**< Constant's name */
};


/**
  * Class implementing an infix expression.
  *
  */

class SpreadsheetExport OperatorExpression : public UnitExpression {
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
        UNIT
    };
    OperatorExpression(const App::DocumentObject *_owner = 0, Expression * _left = 0, Operator _op = NONE, Expression * _right = 0);

    virtual ~OperatorExpression();

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const;

    virtual void getDeps(std::set<Path> &props) const;

    virtual void visit(ExpressionVisitor & v);

protected:
    Operator op;        /**< Operator working on left and right */
    Expression * left;  /**< Left operand */
    Expression * right; /**< Right operand */
};

class SpreadsheetExport RangeExpression : public Expression {
    TYPESYSTEM_HEADER();
public:
    RangeExpression(const App::DocumentObject * _owner = 0, const std::string & begin = std::string(), const std::string & end = std::string());

    virtual ~RangeExpression() { }

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const { return 0; }

    virtual void getDeps(std::set<Path> &props) const;

    virtual Expression * simplify() const;

    Range getRange() const { return range; }

protected:
    Range range;
};

class SpreadsheetExport ConditionalExpression : public Expression {
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

    virtual void getDeps(std::set<Path> &props) const;

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

class SpreadsheetExport FunctionExpression : public UnitExpression {
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

        // Aggregates
        SUM,
        AVERAGE,
        STDDEV,
        COUNT,
        MIN,
        MAX
    };

    FunctionExpression(const App::DocumentObject *_owner = 0, Function _f = NONE, std::vector<Expression *> _args = std::vector<Expression*>());

    virtual ~FunctionExpression();

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const { return 10; }

    virtual void getDeps(std::set<Path> &props) const;

    virtual void visit(ExpressionVisitor & v);

protected:
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

class SpreadsheetExport VariableExpression : public UnitExpression {
    TYPESYSTEM_HEADER();
public:
    VariableExpression(const App::DocumentObject *_owner = 0, Path _var = Path());

    ~VariableExpression();

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const { return var.toString(); }

    virtual Expression * copy() const;

    virtual int priority() const { return 10; }

    virtual void getDeps(std::set<Path> &props) const;

    std::string name() const { return var.getPropertyName(); }

    Path getPath() const { return var; }

    void setName(const std::string & name) { assert(0); }

    void resolve();

    void renameDocumentObject(const std::string & oldName, const std::string & newName);

    void renameDocument(const std::string &oldName, const std::string &newName);

protected:

    const App::Property *getProperty() const;

    Path var; /**< Variable name  */
};

/**
  * Class implementing a string. Used to signal either a genuine string or
  * a failed evaluation of an expression.
  */

class SpreadsheetExport StringExpression : public Expression {
    TYPESYSTEM_HEADER();
public:
    StringExpression(const App::DocumentObject *_owner = 0, const std::string & _text = std::string());

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual std::string getText() const { return text; }

    virtual Expression * copy() const;

protected:

    std::string text; /**< Text string */
};

namespace ExpressionParser {
SpreadsheetExport Expression * parse(const App::DocumentObject *owner, const char *buffer);
SpreadsheetExport UnitExpression * parseUnit(const App::DocumentObject *owner, const char *buffer);
SpreadsheetExport Path parsePath(const App::DocumentObject *owner, const char* buffer);
}

}
#endif // EXPRESSION_H
