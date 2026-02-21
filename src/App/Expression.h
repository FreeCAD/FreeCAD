// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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

#pragma once

#include <deque>
#include <set>
#include <string>

#include <App/PropertyLinks.h>
#include <App/ObjectIdentifier.h>
#include <App/Range.h>
#include <Base/Exception.h>
#include <Base/BaseClass.h>


namespace Base
{
class Quantity;
}

namespace App  {

class DocumentObject;
class Expression;
class Document;

using ExpressionPtr = std::unique_ptr<Expression>;


/**
 * @brief Check whether two values are equal.
 *
 * @param[in] v1 The first value.
 * @param[in] v2 The second value.
 * @return true if the values are equal, false otherwise.
 */
AppExport bool isAnyEqual(const App::any& v1, const App::any& v2);

/**
 * @brief Convert an App::any value to a Base::Quantity.
 *
 * @param[in] value The value to convert.
 * @param[in] errmsg Optional error message to use in case of failure.
 *
 * @return The converted Base::Quantity.
 * @throw Base::TypeError if the value cannot be converted to a Base::Quantity.
 */
AppExport Base::Quantity anyToQuantity(const App::any &value, const char *errmsg = nullptr);

// clang-format off
/// Map of depending objects to a map of depending property name to the full referencing object identifier
using ExpressionDeps = std::map<App::DocumentObject*, std::map<std::string, std::vector<ObjectIdentifier>>>;
// clang-format on

/**
 * @brief %Base class for expression visitors.
 * @ingroup ExpressionFramework
 *
 * This is a class that provides functionality to define visitors for
 * expressions.  For a high-level overview of the %Expression framework see
 * topic @ref ExpressionFramework "Expression Framework".
 */
class AppExport ExpressionVisitor {
public:
    virtual ~ExpressionVisitor() = default;

    /**
     * @brief Visit an expression.
     *
     * This is the method that is called when visiting an expression.
     *
     * @param[in,out] e The expression to visit.
     */
    virtual void visit(Expression& e) = 0;

    /// Called before starting modifications
    virtual void aboutToChange() {}

    /**
     * @brief Called after a modification has been made.
     *
     * @return The number of changes made.
     */
    virtual int changed() const { return 0;}

    /// Reset the change counter.
    virtual void reset() {}

    /// Get the property link if applicable.
    virtual App::PropertyLinkBase* getPropertyLink() {return nullptr;}

protected:
    /**
     * @brief Get the identifiers used in an expression.
     *
     * @param[in] e The expression.
     * @param[in, out] ids The map to fill with identifiers.
     */
    void getIdentifiers(Expression& e, std::map<App::ObjectIdentifier, bool>& ids);

    /**
     * @brief Adjust the links in an expression.
     *
     * @param[in,out] e The expression.
     * @param[in] inList The set of document objects to adjust links for.
     * @return true if any links were adjusted, false otherwise.
     */
    bool adjustLinks(Expression& e, const std::set<App::DocumentObject*>& inList);

    /**
     * @brief Update label references in an expression.
     *
     * @param[in,out] e The expression.
     * @param[in] oldName The old document name.
     * @param[in] newName The new document name.
     * @return true if any label references were updated, false otherwise.
     */
    bool relabeledDocument(Expression& e, const std::string& oldName, const std::string& newName);

    /**
     * @brief Rename object identifiers in an expression.
     *
     * Object identifiers are renamed according to the provided map of old to
     * new object identifiers.  The new identifiers are made relative to @p
     * path.
     *
     * @param[in,out] e The expression.
     * @param[in] paths The map of old to new object identifiers.
     * @param[in] path The object identifier path context.
     *
     * @return True if any object identifiers were renamed, false otherwise.
     */
    bool renameObjectIdentifier(Expression& e,
                                const std::map<ObjectIdentifier, ObjectIdentifier>& paths,
                                const ObjectIdentifier& path);

    /**
     * @brief Collect replacements in an expression.
     *
     * This function runs before the @ref renameObjectIdentifier function and
     * collects all occurrences of a specific old document object
     * and maps them to a new document object in the provided map.
     *
     * @param[in,out] e The expression.
     * @param[in,out] paths The map to fill with old to new object identifier replacements.
     * @param[in] parent The parent document object.
     * @param[in] oldObj The old document object to be replaced.
     * @param[in] newObj The new document object to replace with.
     */
    void collectReplacement(Expression& e,
                            std::map<ObjectIdentifier, ObjectIdentifier>& paths,
                            const App::DocumentObject* parent,
                            App::DocumentObject* oldObj,
                            App::DocumentObject* newObj) const;

    /**
     * @brief Update element references in an expression.
     *
     * @param[in,out] e The expression.
     * @param[in] feature The document object feature for which the update
     * element references should be updated.
     * @param[in] reverse If true, use the old style, i.e. non-mapped element
     * reference to query for the new style, i.e. mapped element reference when
     * update. If false, then the other way around.
     *
     * @return true if any element references were updated, false otherwise.
     */
    bool updateElementReference(Expression& e, App::DocumentObject* feature, bool reverse);

    /**
     * @brief Rewrite sub-names in an expression after importing external
     * objects.
     *
     * @param[in,out] e The expression.
     * @param[in] subNameMap The map of sub-name replacements.
     */
    void importSubNames(Expression& e, const ObjectIdentifier::SubNameMap& subNameMap);

    /**
     * @brief Update label references in an expression.
     *
     * @param[in,out] e The expression.
     * @param[in] obj The document object whose label has changed.
     * @param[in] ref The old label reference.
     * @param[in] newLabel The new label.
     */
    void updateLabelReference(Expression& e,
                              App::DocumentObject* obj,
                              const std::string& ref,
                              const char* newLabel);

    /**
     * @brief Move cell references in an expression.
     *
     * This function visits the expression and updates all cell references to
     * @p address.  These cell references are then updated by moving them by @p
     * rowCount rows and @p colCount columns.
     *
     * @param[in,out] e The expression.
     * @param[in] address The cell address that needs to be updated.
     * @param[in] rowCount The number of rows to move.
     * @param[in] colCount The number of columns to move.
     */
    void moveCells(Expression& e, const CellAddress& address, int rowCount, int colCount);

    /**
     * @brief Offset cell references in an expression.
     *
     * This function visits the expression and in a range expression, offsets
     * all cell relative references by @p rowOffset rows and @p colOffset
     * columns.
     *
     * @param[in,out] e The expression.
     * @param[in] rowOffset The number of rows to offset.
     * @param[in] colOffset The number of columns to offset.
     */
    void offsetCells(Expression &e, int rowOffset, int colOffset);
};

/**
 * @brief Class for expression visitors that can modify expressions.
 * @ingroup ExpressionFramework
 *
 * With respect to the base class @ref ExpressionVisitor, this class adds
 * functionality to modify the visited @ref Expression objects and signal on
 * changes.  It also keeps track of the number of modifications made to the
 * visited expressions.  For a high-level overview of the %Expression framework
 * see topic @ref ExpressionFramework "Expression Framework".
 *
 * @tparam P The property type being modified.
 */
template<class P> class ExpressionModifier : public ExpressionVisitor {
public:
    /**
     * @brief Construct a new ExpressionModifier object.
     *
     * @param[in,out] _prop The property being modified.
     */
    explicit ExpressionModifier(P & _prop)
        : prop(_prop)
        , propLink(freecad_cast<App::PropertyLinkBase*>(&prop))
        , signaller(_prop,false)
    {}

    ~ExpressionModifier() override = default;

    void aboutToChange() override{
        ++_changed;
        signaller.aboutToChange();
    }

    int changed() const override { return _changed; }

    void reset() override {_changed = 0;}

    App::PropertyLinkBase* getPropertyLink() override {return propLink;}

protected:
    /// The property being modified.
    P& prop;

    /// The property link if applicable.
    App::PropertyLinkBase* propLink;

    /// The atomic property change signaller.
    typename AtomicPropertyChangeInterface<P>::AtomicPropertyChange signaller;

    /// The number of changes made.
    int _changed{0};
};


/**
  * @brief %Base class for expressions.
  * @ingroup ExpressionFramework
  *
  * @details For a high-level overview of the %Expression framework see topic
  * @ref ExpressionFramework "Expression Framework".
  */
class AppExport Expression : public Base::BaseClass {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * @brief Construct a new Expression object.
     *
     * @param[in] _owner The document object that owns this expression.
     */
    explicit Expression(const App::DocumentObject * _owner);

    ~Expression() override;

    /**
     * @brief Check if the expression is touched.
     *
     * An expression is touched if one of the properties it references is
     * touched.
     *
     * @return true if the expression is touched, false otherwise.
     */
    virtual bool isTouched() const { return false; }

    /**
     * @brief Evaluate the expression.
     *
     * Evaluating an expression returns another expression that represents a
     * value.  Contrast this with the @ref simplify function that returns a
     * possibly simpler version of the same expression.
     *
     * @return The evaluated expression.
     */
    Expression* eval() const;

    /**
     * @brief Convert the expression to a string.
     *
     * @param[in] persistent If true, the string representation is persistent
     * and can be saved to a file.
     * @param[in] checkPriority If true, check whether the expression requires
     * parentheses based on operator priority.
     * @param[in] indent The indentation level for pretty-printing.
     *
     * @return The string representation of the expression.
     */
    std::string toString(bool persistent = false, bool checkPriority = false, int indent = 0) const;

    /**
     * @brief Write a string representation of the expression to a stream.
     *
     * @param[in,out] os The output stream to write to.
     * @copydoc Expression::toString(bool, bool, int) const
     */
    void toString(std::ostream &os, bool persistent=false, bool checkPriority=false, int indent=0) const;

    /**
     * @brief Parse an expression from a string.
     *
     * @param[in] owner The document object that will own the parsed expression.
     * @param[in] buffer The string to parse.
     *
     * @return The parsed expression.
     */
    static Expression* parse(const App::DocumentObject * owner, const std::string& buffer);

    /// Copy an expression.
    Expression * copy() const;

    /**
     * @brief Get the operator priority.
     *
     * This is used to determine whether parentheses are needed when
     * converting the expression to a string.
     *
     * @return The operator priority.
     */
    virtual int priority() const;

    /**
     * @brief Get the identifiers in the expression.
     *
     * @param[in,out] deps The mapping from object identifiers to a boolean
     * indicating whether the dependency is hidden (`href` references).
     */
    void getIdentifiers(std::map<App::ObjectIdentifier, bool>& deps) const;

    /**
     * @brief Get the identifiers in the expression.
     *
     * @return The mapping from object identifiers to a boolean indicating
     * whether the dependency is hidden (`href` references).
     */
    std::map<App::ObjectIdentifier,bool> getIdentifiers() const;

    /// Options for obtaining the dependencies of an expression.
    enum DepOption
    {
        DepNormal,  ///< Get normal dependencies.
        DepHidden,  ///< Get the hidden dependencies (`href` references).
        DepAll,     ///< Get both normal and hidden dependencies.
    };

    /**
     * @brief Get the dependencies of the expression.
     *
     * @param[in,out] deps The dependencies to fill.
     * @param[in] option The dependency option.
     */
    void getDeps(ExpressionDeps& deps, int option = DepNormal) const;

    /**
     * @brief Get the dependencies of the expression.
     *
     * @param[in] option The dependency option.
     * @return The dependencies.
     */
    ExpressionDeps getDeps(int option=DepNormal) const;

    /**
     * @brief Get the dependent document objects of the expression.
     *
     * @param[in,out] labels Optional vector to fill with labels of dependent objects.
     *
     * @return Map of dependent document objects to a boolean indicating whether
     * they are hidden (`href` references).
     */
    std::map<App::DocumentObject*, bool> getDepObjects(std::vector<std::string>* labels = nullptr) const;

    /**
     * @brief Get the dependent document objects of the expression.
     *
     * @param[in,out] deps The map to fill with dependent document objects to a
     * boolean indicating whether they are hidden (`href` references).
     * @param[in,out] labels Optional vector to fill with labels of dependent objects.
     */
    void getDepObjects(std::map<App::DocumentObject*,bool>& deps, std::vector<std::string> *labels=nullptr) const;

    /**
     * @brief Import sub-names in the expression after importing external objects.
     *
     * @param[in] nameMap The map of sub-name replacements.
     *
     * @return The updated expression.
     */
    ExpressionPtr importSubNames(const std::map<std::string,std::string> &nameMap) const;

    /**
     * @brief Update label references in the expression.
     *
     * @param[in] obj The document object whose label has changed.
     * @param[in] ref The old label reference.
     * @param[in] newLabel The new label.
     *
     * @return The updated expression.
     */
    ExpressionPtr updateLabelReference(App::DocumentObject *obj,
            const std::string &ref, const char *newLabel) const;

    /**
     * @brief Replace a document object in the expression.
     *
     * @param[in] parent The parent document object.
     * @param[in] oldObj The old document object to be replaced.
     * @param[in] newObj The new document object to replace with.
     *
     * @return The updated expression.
     */
    ExpressionPtr replaceObject(const App::DocumentObject *parent,
            App::DocumentObject *oldObj, App::DocumentObject *newObj) const;

    /**
     * @brief Adjust links according to a new inList.
     *
     * @param[in] inList The new inList to adjust links for.
     * @return true if any links were adjusted, false otherwise.
     */
    bool adjustLinks(const std::set<App::DocumentObject*> &inList);

    /**
     * @brief Simplify the expression.
     *
     * In contrast to @ref eval, which evaluates the expression to a value,
     * this function simplifies the expression by computing all constant
     * expressions.
     *
     * @return The simplified expression.
     */
    virtual Expression * simplify() const = 0;

    /// Visit the expression with a visitor.
    void visit(ExpressionVisitor & v);

    /// Exception class for expression errors.
    class Exception : public Base::Exception {
    public:
        explicit Exception(const char *sMessage) : Base::Exception(sMessage) { }
    };

    /// Get the owner of the expression.
    App::DocumentObject *  getOwner() const { return owner; }

    struct Component;

    /// Add a component to the expression.
    virtual void addComponent(Component* component);

    using ComponentList = std::vector<Component*>;

    /// Create a component from a name.
    static Component* createComponent(const std::string& n);

    /**
     * @brief Create an index or range component.
     *
     * @param[in] e1 Either the index in an array index or the start of a
     * range.
     *
     * @param[in] e2 The end of a range, may be `nullptr`.
     * @param[in] e3 The step of a range, may be `nullptr`.
     */
    static Component *createComponent(Expression *e1, Expression *e2=nullptr,
            Expression *e3=nullptr, bool isRange=false);

    /// Check if the expression has a component.
    bool hasComponent() const {return !components.empty();}

    /// Get the value as boost::any.
    boost::any getValueAsAny() const;

    /// Get the value as a Python object.
    Py::Object getPyValue() const;

    /**
     * @brief Check if this expression is the same as another.
     *
     * @param[in] other The other expression to compare with.
     * @param[in] checkComment If true, also check the comment.
     */
    bool isSame(const Expression &other, bool checkComment=true) const;

    friend class ExpressionVisitor;

protected:
    virtual bool _isIndexable() const {return false;}
    virtual Expression *_copy() const = 0;
    virtual void _toString(std::ostream &ss, bool persistent, int indent=0) const = 0;
    virtual void _getIdentifiers(std::map<App::ObjectIdentifier,bool> &) const  {}
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
    virtual Py::Object _getPyValue() const = 0;
    virtual void _visit(ExpressionVisitor &) {}

protected:
    // clang-format off

    /// The document object used to access unqualified variables (i.e local scope).
    App::DocumentObject * owner;

    /// The list of components.
    ComponentList components;

public:
    std::string comment;
    // clang-format on
};

}