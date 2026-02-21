// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include "Expression.h"
#include <Base/Matrix.h>
#include <Base/Quantity.h>
#include <Base/Vector3D.h>

namespace App
{

////////////////////////////////////////////////////////////////////////////////////
// Expecting the extended expression is going to be constantly amended (to
// conform to Python), we move most of the class declarations here to avoid
// constant recompiling of the whole FC code base, as the expression header is
// included by everyone
///////////////////////////////////////////////////////////////////////////////////

/**
 * @brief %Part of an expression that represents an index or range.
 */
struct AppExport Expression::Component
{
    ObjectIdentifier::Component comp;
    Expression* e1;
    Expression* e2;
    Expression* e3;

    explicit Component(const std::string& n);
    Component(Expression* e1, Expression* e2, Expression* e3, bool isRange = false);
    explicit Component(const ObjectIdentifier::Component& comp);
    Component(const Component& other);
    ~Component();
    Component& operator=(const Component&) = delete;

    void visit(ExpressionVisitor& v);
    bool isTouched() const;
    void toString(std::ostream& ss, bool persistent) const;
    Component* copy() const;
    Component* eval() const;

    Py::Object get(const Expression* owner, const Py::Object& pyobj) const;
    void set(const Expression* owner, Py::Object& pyobj, const Py::Object& value) const;
    void del(const Expression* owner, Py::Object& pyobj) const;
};

////////////////////////////////////////////////////////////////////////////////////

/**
 * Part of an expressions that contains a unit.
 *
 */

class AppExport UnitExpression: public Expression
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit UnitExpression(const App::DocumentObject* _owner = nullptr,
                            const Base::Quantity& _quantity = Base::Quantity(),
                            const std::string& _unitStr = std::string());

    ~UnitExpression() override;

    Expression* simplify() const override;

    void setUnit(const Base::Quantity& _quantity);

    void setQuantity(const Base::Quantity& _quantity);

    double getValue() const
    {
        return quantity.getValue();
    }

    const Base::Unit& getUnit() const
    {
        return quantity.getUnit();
    }

    const Base::Quantity& getQuantity() const
    {
        return quantity;
    }

    const std::string getUnitString() const
    {
        return unitStr;
    }

    double getScaler() const
    {
        return quantity.getValue();
    }

protected:
    Expression* _copy() const override;
    void _toString(std::ostream& ss, bool persistent, int indent) const override;
    Py::Object _getPyValue() const override;

protected:
    mutable PyObject* cache = nullptr;

private:
    Base::Quantity quantity;
    std::string unitStr; /**< The unit string from the original parsed string */
};

/**
 * Class implementing a number with an optional unit
 */

class AppExport NumberExpression: public UnitExpression
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit NumberExpression(const App::DocumentObject* _owner = nullptr,
                              const Base::Quantity& quantity = Base::Quantity());

    Expression* simplify() const override;

    /**
     * @brief Negate the stored value.
     */
    void negate();

    bool isInteger(long* v = nullptr) const;

protected:
    Expression* _copy() const override;
    void _toString(std::ostream& ss, bool persistent, int indent) const override;
};

class AppExport ConstantExpression: public NumberExpression
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit ConstantExpression(const App::DocumentObject* _owner = nullptr,
                                const char* _name = "",
                                const Base::Quantity& _quantity = Base::Quantity());

    std::string getName() const
    {
        return name;
    }

    bool isNumber() const;

protected:
    Py::Object _getPyValue() const override;
    void _toString(std::ostream& ss, bool persistent, int indent) const override;
    Expression* _copy() const override;

protected:
    const char* name;
};

/**
 * Class implementing an infix expression.
 *
 */

class AppExport OperatorExpression: public UnitExpression
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum Operator
    {
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
    explicit OperatorExpression(const App::DocumentObject* _owner = nullptr,
                                Expression* _left = nullptr,
                                Operator _op = NONE,
                                Expression* _right = nullptr);

    ~OperatorExpression() override;

    bool isTouched() const override;

    Expression* simplify() const override;

    int priority() const override;

    Operator getOperator() const
    {
        return op;
    }

    Expression* getLeft() const
    {
        return left;
    }

    Expression* getRight() const
    {
        return right;
    }

protected:
    Expression* _copy() const override;

    Py::Object _getPyValue() const override;

    void _toString(std::ostream& ss, bool persistent, int indent) const override;

    void _visit(ExpressionVisitor& v) override;

    virtual bool isCommutative() const;

    virtual bool isLeftAssociative() const;

    virtual bool isRightAssociative() const;

    Operator op;       /**< Operator working on left and right */
    Expression* left;  /**< Left operand */
    Expression* right; /**< Right operand */
};

class AppExport ConditionalExpression: public Expression
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit ConditionalExpression(const App::DocumentObject* _owner = nullptr,
                                   Expression* _condition = nullptr,
                                   Expression* _trueExpr = nullptr,
                                   Expression* _falseExpr = nullptr);

    ~ConditionalExpression() override;

    bool isTouched() const override;

    Expression* simplify() const override;

    int priority() const override;

protected:
    Expression* _copy() const override;
    void _visit(ExpressionVisitor& v) override;
    void _toString(std::ostream& ss, bool persistent, int indent) const override;
    Py::Object _getPyValue() const override;

protected:
    Expression* condition; /**< Condition */
    Expression* trueExpr;  /**< Expression if abs(condition) is > 0.5 */
    Expression* falseExpr; /**< Expression if abs(condition) is < 0.5 */
};

/**
 * Class implementing various functions, e.g sin, cos, etc.
 *
 */

class AppExport FunctionExpression: public UnitExpression
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum Function
    {
        NONE,

        // Normal functions taking one or two arguments
        ABS,
        ACOS,
        ASIN,
        ATAN,
        ATAN2,
        CATH,
        CBRT,
        CEIL,
        COS,
        COSH,
        EXP,
        FLOOR,
        HYPOT,
        LOG,
        LOG10,
        MOD,
        POW,
        ROUND,
        SIN,
        SINH,
        SQRT,
        TAN,
        TANH,
        TRUNC,

        // Vector
        VANGLE,
        VCROSS,
        VDOT,
        VLINEDIST,
        VLINESEGDIST,
        VLINEPROJ,
        VNORMALIZE,
        VPLANEDIST,
        VPLANEPROJ,
        VSCALE,
        VSCALEX,
        VSCALEY,
        VSCALEZ,

        // Matrix
        MINVERT,   // invert matrix/placement/rotation
        MROTATE,   // Rotate matrix/placement/rotation around axis, by rotation object, or by euler
                   // angles.
        MROTATEX,  // Rotate matrix/placement/rotation around x-axis.
        MROTATEY,  // Rotate matrix/placement/rotation around y-axis.
        MROTATEZ,  // Rotate matrix/placement/rotation around z-axis.
        MSCALE,    // matrix scale by vector
        MTRANSLATE,  // Translate matrix/placement.

        // Object creation
        CREATE,        // Create new object of a given type.
        LIST,          // Create Python list.
        MATRIX,        // Create matrix object.
        PLACEMENT,     // Create placement object.
        ROTATION,      // Create rotation object.
        ROTATIONX,     // Create x-axis rotation object.
        ROTATIONY,     // Create y-axis rotation object.
        ROTATIONZ,     // Create z-axis rotation object.
        STR,           // stringify
        PARSEQUANT,    // parse string quantity
        TRANSLATIONM,  // Create translation matrix object.
        TUPLE,         // Create Python tuple.
        VECTOR,        // Create vector object.

        HIDDENREF,  // hidden reference that has no dependency check
        HREF,       // deprecated alias of HIDDENREF

        // Non aggregated logical
        NOT,   // logical NOT

        // Aggregates
        AGGREGATES,

        AVERAGE,
        COUNT,
        MAX,
        MIN,
        STDDEV,
        SUM,

        // Logical aggregates, evaluates to {0,1}
        AND,  // logical AND
        OR,  // logical OR

        // Last one
        LAST,
    };

    explicit FunctionExpression(const App::DocumentObject* _owner = nullptr,
                                Function _f = NONE,
                                std::string&& name = std::string(),
                                std::vector<Expression*> _args = std::vector<Expression*>());

    ~FunctionExpression() override;

    bool isTouched() const override;

    Expression* simplify() const override;

    static Py::Object
    evaluate(const Expression* owner, int type, const std::vector<Expression*>& args);

    Function getFunction() const
    {
        return f;
    }
    const std::vector<Expression*>& getArgs() const
    {
        return args;
    }

protected:
    static Py::Object
    evalAggregate(const Expression* owner, int type, const std::vector<Expression*>& args);
    static Base::Vector3d evaluateSecondVectorArgument(const Expression* expression,
                                                       const std::vector<Expression*>& arguments);
    static double extractLengthValueArgument(const Expression* expression,
                                             const std::vector<Expression*>& arguments,
                                             int argumentIndex);
    static Base::Vector3d extractVectorArgument(const Expression* expression,
                                                const std::vector<Expression*>& arguments,
                                                int argumentIndex);
    static void initialiseObject(const Py::Object* object,
                                 const std::vector<Expression*>& arguments,
                                 const unsigned long offset = 0);
    static Py::Object transformFirstArgument(const Expression* expression,
                                             const std::vector<Expression*>& arguments,
                                             const Base::Matrix4D* transformationMatrix);
    static Py::Object translationMatrix(double x, double y, double z);
    Py::Object _getPyValue() const override;
    Expression* _copy() const override;
    void _visit(ExpressionVisitor& v) override;
    void _toString(std::ostream& ss, bool persistent, int indent) const override;

    Function f; /**< Function to execute */
    std::string fname;
    std::vector<Expression*> args; /** Arguments to function*/
};

/**
 * Class implementing a reference to a property. If the name is unqualified,
 * the owner of the expression is searched. If it is qualified, the document
 * that contains the owning document object is searched for other document
 * objects to search. Both labels and internal document names are searched.
 *
 */

class AppExport VariableExpression: public UnitExpression
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit VariableExpression(const App::DocumentObject* _owner = nullptr,
                                const ObjectIdentifier& _var = ObjectIdentifier());

    ~VariableExpression() override;

    bool isTouched() const override;

    Expression* simplify() const override;

    std::string name() const
    {
        return var.getPropertyName();
    }

    ObjectIdentifier getPath() const
    {
        return var;
    }

    void setPath(const ObjectIdentifier& path);

    /**
     * @brief Find the property this expression referse to.
     *
     * Unqualified names (i.e the name only without any dots) are resolved in
     * the owning DocumentObjects.  Qualified names are looked up in the owning
     * Document, first, by its internal name, then if not found, by the
     * DocumentObjects' labels.
     *
     * @return The Property object if it is derived from either
     * PropertyInteger, PropertyFloat, or PropertyString.
     *
     * @trhows Expression::Exception If the property cannot be resolved.
     */
    const App::Property* getProperty() const;

    void addComponent(Component* component) override;

protected:
    Expression* _copy() const override;
    Py::Object _getPyValue() const override;
    void _toString(std::ostream& ss, bool persistent, int indent) const override;
    bool _isIndexable() const override;
    void _getIdentifiers(std::map<App::ObjectIdentifier, bool>&) const override;
    bool _adjustLinks(const std::set<App::DocumentObject*>&, ExpressionVisitor&) override;
    void _importSubNames(const ObjectIdentifier::SubNameMap&) override;
    void _updateLabelReference(App::DocumentObject*, const std::string&, const char*) override;
    bool _updateElementReference(App::DocumentObject*, bool, ExpressionVisitor&) override;
    bool _relabeledDocument(const std::string&, const std::string&, ExpressionVisitor&) override;
    bool _renameObjectIdentifier(const std::map<ObjectIdentifier, ObjectIdentifier>&,
                                 const ObjectIdentifier&,
                                 ExpressionVisitor&) override;
    void _collectReplacement(std::map<ObjectIdentifier, ObjectIdentifier>&,
                             const App::DocumentObject* parent,
                             App::DocumentObject* oldObj,
                             App::DocumentObject* newObj) const override;
    void _moveCells(const CellAddress&, int, int, ExpressionVisitor&) override;
    void _offsetCells(int, int, ExpressionVisitor&) override;

protected:
    ObjectIdentifier var; /**< Variable name  */
};

//////////////////////////////////////////////////////////////////////

class AppExport PyObjectExpression: public Expression
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit PyObjectExpression(const App::DocumentObject* _owner = nullptr,
                                PyObject* pyobj = nullptr,
                                bool owned = false)
        : Expression(_owner)
    {
        setPyValue(pyobj, owned);
    }

    ~PyObjectExpression() override;

    void setPyValue(Py::Object pyobj);
    void setPyValue(PyObject* pyobj, bool owned = false);
    Expression* simplify() const override
    {
        return copy();
    }

protected:
    Expression* _copy() const override;
    void _toString(std::ostream& ss, bool persistent, int indent) const override;
    Py::Object _getPyValue() const override;

protected:
    PyObject* pyObj = nullptr;
};

/**
 * Class implementing a string. Used to signal either a genuine string or
 * a failed evaluation of an expression.
 */

class AppExport StringExpression: public Expression
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit StringExpression(const App::DocumentObject* _owner = nullptr,
                              const std::string& _text = std::string());
    ~StringExpression() override;

    Expression* simplify() const override;

    virtual std::string getText() const
    {
        return text;
    }

protected:
    Expression* _copy() const override;
    void _toString(std::ostream& ss, bool persistent, int indent) const override;
    Py::Object _getPyValue() const override;
    bool _isIndexable() const override
    {
        return true;
    }

private:
    std::string text; /**< Text string */
    mutable PyObject* cache = nullptr;
};

class AppExport RangeExpression: public App::Expression
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit RangeExpression(const App::DocumentObject* _owner = nullptr,
                             const std::string& begin = std::string(),
                             const std::string& end = std::string());

    ~RangeExpression() override = default;

    bool isTouched() const override;

    App::Expression* simplify() const override;

    Range getRange() const;

protected:
    Expression* _copy() const override;
    void _toString(std::ostream& ss, bool persistent, int indent) const override;
    Py::Object _getPyValue() const override;
    void _getIdentifiers(std::map<App::ObjectIdentifier, bool>&) const override;
    bool _renameObjectIdentifier(const std::map<ObjectIdentifier, ObjectIdentifier>&,
                                 const ObjectIdentifier&,
                                 ExpressionVisitor&) override;
    void _moveCells(const CellAddress&, int, int, ExpressionVisitor&) override;
    void _offsetCells(int, int, ExpressionVisitor&) override;

protected:
    std::string begin;
    std::string end;
};

/**
 * @brief Namespace for parsing expressions.
 *
 * Contains functionality for parsing expressions, the units of
 * expressions, whether a token is an identifier, unit, or constant.
 */
namespace ExpressionParser
{
AppExport Expression* parse(const App::DocumentObject* owner, const char* buffer);
AppExport UnitExpression* parseUnit(const App::DocumentObject* owner, const char* buffer);
AppExport ObjectIdentifier parsePath(const App::DocumentObject* owner, const char* buffer);
AppExport bool isTokenAnIndentifier(const std::string& str);
AppExport bool isTokenAConstant(const std::string& str);
AppExport bool isTokenAUnit(const std::string& str);
AppExport std::vector<std::tuple<int, int, std::string>> tokenize(const std::string& str);

/// Convenient class to mark begin of importing
class AppExport ExpressionImporter
{
public:
    explicit ExpressionImporter(Base::XMLReader& reader);
    ~ExpressionImporter();
    static Base::XMLReader* reader();
};

AppExport bool isModuleImported(PyObject*);

/**
 * @brief The semantic_type class encapsulates the value in the parse tree during parsing.
 */

class semantic_type
{
public:
    struct
    {
        Base::Quantity scaler;
        std::string unitStr;
    } quantity;
    Expression::Component* component {nullptr};
    Expression* expr {nullptr};
    ObjectIdentifier path;
    std::deque<ObjectIdentifier::Component> components;
    long long int ivalue {0};
    double fvalue {0};
    struct
    {
        const char* name = "";
        double fvalue = 0;
    } constant;
    std::vector<Expression*> arguments;
    std::vector<Expression*> list;
    std::string string;
    std::pair<FunctionExpression::Function, std::string> func;
    ObjectIdentifier::String string_or_identifier;
    semantic_type()
        : func({FunctionExpression::NONE, std::string()})
    {}
};

#define YYSTYPE semantic_type
#include "Expression.tab.h"
#undef YYTOKENTYPE
#undef YYSTYPE
#undef YYSTYPE_ISDECLARED
}  // namespace ExpressionParser

}  // namespace App