#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <string>
#include <App/DocumentObject.h>
#include <Base/Exception.h>
#include <Base/Unit.h>
#include <set>

namespace App  {

class Document;

class Expression {
public:
    Expression(const App::DocumentObject * _owner);

    virtual ~Expression();

    virtual bool isTouched() const { return false; }

    virtual Expression * eval() const = 0;

    virtual std::string toString() const = 0;

    static Expression * parse(const App::DocumentObject * owner, const std::string& buffer);

    virtual Expression * copy() const = 0;

    virtual int priority() const { return 0; }

    virtual void getDeps(std::set<const App::Property*> &props) const { }

    virtual void getDeps(std::set<std::string> &props) const { }

    virtual Expression * simplify() const = 0;

    class Exception : public Base::Exception {
    public:
        Exception(const char *sMessage) : Base::Exception(sMessage) { }
    };

    class CircularException : public Exception {
    public:
        CircularException() : Exception("Circular expression.") { }
    };

    class UnitException : public Exception {
    public:
        UnitException(const char *sMessage) : Exception(sMessage) { }
    };

protected:
    const App::DocumentObject * owner;
};

class UnitExpression : public Expression {
public:
    UnitExpression(const App::DocumentObject *_owner, const Base::Unit & _unit = Base::Unit(), const char * _unitstr = 0, double _scaler = 1.0);

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const { return 10; }

    void setUnit(const Base::Unit & _unit, const char * _unitstr, double _scaler);

    const Base::Unit & getUnit() const { return unit; }

    const std::string getUnitString() const { return unitstr; }

    double getScaler() const { return scaler; }

protected:
    Base::Unit unit;
    std::string unitstr;
    double scaler;
};

class NumberExpression : public UnitExpression {
public:
    NumberExpression(const App::DocumentObject *_owner, double _value, const Base::Unit & _units = Base::Unit(), const char * _unitstr = 0, double _scaler = 1.0);

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const { return 10; }

    double getValue() const { return value * getScaler(); }

    void negate();

protected:
    double value;
};

class OperatorExpression : public UnitExpression {
public:
    enum Operator {
        ADD,
        SUB,
        MUL,
        DIV,
        POW
    };
    OperatorExpression(const App::DocumentObject *_owner, Expression * _left, Operator _op, Expression * _right);

    virtual ~OperatorExpression();

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const;

    virtual void getDeps(std::set<const App::Property*> &props) const;

    virtual void getDeps(std::set<std::string> &props) const;

protected:
    Operator op;
    Expression * left;
    Expression * right;
};

class FunctionExpression : public UnitExpression {
public:
    enum Function {
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
        ATAN2,
        MOD,
        POW,
    };

    FunctionExpression(const App::DocumentObject *_owner, Function _f, Expression * _arg1 , Expression * _arg2 = 0);

    virtual ~FunctionExpression();

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const { return 10; }

    virtual void getDeps(std::set<const App::Property*> &props) const;

    virtual void getDeps(std::set<std::string> &props) const;

protected:
    Function f;
    Expression * arg1;
    Expression * arg2;
};

class VariableExpression : public UnitExpression {
public:
    VariableExpression(const App::DocumentObject *_owner, const std::string & _var);

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const { return var; }

    virtual Expression * copy() const;

    virtual int priority() const { return 10; }

    virtual void getDeps(std::set<const App::Property*> &props) const;

    virtual void getDeps(std::set<std::string> &props) const;

protected:

    Property *getProperty() const;

    std::string var;
};

class StringExpression : public Expression {
    public:
    StringExpression(const App::DocumentObject *_owner, const std::string & _text);

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const { return text; }

    virtual Expression * copy() const;

protected:
    std::string text;
};

}

namespace ExpressionParser {
App::Expression * parse(const App::DocumentObject *owner, const char *buffer);
App::UnitExpression * parseUnit(const App::DocumentObject *owner, const char *buffer);
}

#endif // EXPRESSION_H
