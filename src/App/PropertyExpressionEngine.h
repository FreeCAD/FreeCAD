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

#include <functional>
#include <set>

#include <fastsignals/signal.h>

#include <FCConfig.h>

#include <App/PropertyLinks.h>


namespace Base
{
class Writer;
class XMLReader;
}  // namespace Base

namespace App
{

class DocumentObject;
class DocumentObjectExecReturn;
class ObjectIdentifier;
class Expression;
using ExpressionPtr = std::unique_ptr<Expression>;

class AppExport PropertyExpressionContainer: public App::PropertyXLinkContainer
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyExpressionContainer();
    ~PropertyExpressionContainer() override;

    /**
     * @brief Get the expressions map.
     *
     * This function returns a mapping from object identifier to expression.
     * The object identifier specifies the property in the document object that
     * the expression is bound to.
     *
     * @return The map of ObjectIdentifier to Expression pointers.
     */
    virtual std::map<App::ObjectIdentifier, const App::Expression*> getExpressions() const = 0;

    /**
     * @brief Set the expressions map.
     *
     * This function sets the mapping from object identifier to expression.
     * The object identifier specifies the property in the document object that
     * the expression is bound to.
     *
     * @param[in] exprs The new map of ObjectIdentifier to Expression pointers.
     */
    virtual void setExpressions(std::map<App::ObjectIdentifier, App::ExpressionPtr>&& exprs) = 0;

protected:
    /**
     * @brief Handle document relabeling.
     *
     * Update the expressions in response to a document being relabeled.
     *
     * @param[in] doc The document that was relabeled.
     */
    virtual void onRelabeledDocument(const App::Document& doc) = 0;

    /**
     * @brief Handle dynamic property renaming.
     *
     * Update the expressions in response to a dynamic property being renamed.
     *
     * @param[in] prop The property that was renamed.
     * @param[in] oldName The old name of the property.
     */
    virtual void onRenameDynamicProperty(const App::Property& prop, const char* oldName) = 0;

private:
    static void slotRelabelDocument(const App::Document& doc);
    static void slotRenameDynamicProperty(const App::Property& prop, const char* oldName);
};


/**
 * @brief The class that manages expressions that target a property in a
 * document object.
 * @ingroup ExpressionFramework
 *
 * This class manages a set of expressions that are bound to properties in
 * document objects.  It provides functionality to evaluate the expressions,
 * handle dependencies between expressions, and update the properties they
 * are bound to.  For a high-level overview of the %Expression framework see
 * topic @ref ExpressionFramework "Expression Framework".
 */
class AppExport PropertyExpressionEngine
    : public App::PropertyExpressionContainer,
      private App::AtomicPropertyChangeInterface<PropertyExpressionEngine>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    void updateElementReference(App::DocumentObject* feature,
                                bool reverse = false,
                                bool notify = false) override;
    bool referenceChanged() const override;
    bool adjustLink(const std::set<App::DocumentObject*>& inList) override;
    Property*
    CopyOnImportExternal(const std::map<std::string, std::string>& nameMap) const override;
    Property* CopyOnLabelChange(App::DocumentObject* obj,
                                const std::string& ref,
                                const char* newLabel) const override;
    Property* CopyOnLinkReplace(const App::DocumentObject* parent,
                                App::DocumentObject* oldObj,
                                App::DocumentObject* newObj) const override;

    using ValidatorFunc = std::function<std::string(const App::ObjectIdentifier& path,
                                                    std::shared_ptr<const App::Expression> expr)>;

    /**
     * @brief This struct encapsulates an expression.
     */
    struct ExpressionInfo
    {
        std::shared_ptr<App::Expression> expression; /**< The actual expression tree */
        bool busy;

        explicit ExpressionInfo(
            std::shared_ptr<App::Expression> expression = std::shared_ptr<App::Expression>())
        {
            this->expression = expression;
            this->busy = false;
        }

        ExpressionInfo(const ExpressionInfo&) = default;

        ExpressionInfo& operator=(const ExpressionInfo&) = default;
    };

    PropertyExpressionEngine();
    ~PropertyExpressionEngine() override;

    unsigned int getMemSize() const override;

    std::map<App::ObjectIdentifier, const App::Expression*> getExpressions() const override;
    void setExpressions(std::map<App::ObjectIdentifier, App::ExpressionPtr>&& exprs) override;
    void onRelabeledDocument(const App::Document& doc) override;
    void onRenameDynamicProperty(const App::Property& prop, const char* oldName) override;

    /// Dummy setValue to satisfy a macro.
    void setValue() {}

    Property* Copy() const override;

    void Paste(const Property& from) override;

    void Save(Base::Writer& writer) const override;

    void Restore(Base::XMLReader& reader) override;

    /**
     * @brief Set a given expression to @a path.
     *
     * Note that the "value" in this context is an expression.  This means that
     * this function does not evaluate the expression and updates the property
     * that @a path points to.  It merely registers the expression to be used
     * when evaluating the property later.
     *
     * @param[in] path The path that the expression is targeting.
     * @param[in] expr The new expression.
     */
    void setValue(const App::ObjectIdentifier& path, std::shared_ptr<App::Expression> expr);

    /**
     * @brief Get the expression for @a path.
     *
     * Note that the "value" in this context is an expression.  This means that
     * this function does not return the evaluated value of the property that
     * @a path points to.  It merely returns the registered expression.
     *
     * @param[in] path ObjectIndentifier to query for.
     *
     * @return The expression for @a path, or empty boost::any if not found.
     */
    const boost::any getPathValue(const App::ObjectIdentifier& path) const override;

    /// Execute options
    enum ExecuteOption
    {
        /// Execute all expression
        ExecuteAll,
        /// Execute only output property bindings
        ExecuteOutput,
        /// Execute only non-output property bindings
        ExecuteNonOutput,
        /// Execute on document restore
        ExecuteOnRestore,
    };

    /**
     * @brief Evaluate the expressions.
     *
     * Evaluate the expressions and update the properties they are bound to.
     *
     * @param[in] option: execution option, see ExecuteOption.
     * @param[out] touched: if not null, set to true if any property was
     * changed.
     *
     * @return On success a pointer to DocumentObject::StdReturn is returned.  On failure, it
     * returns a pointer to a newly created App::DocumentObjectExecReturn that contains the error
     * message.
     */
    DocumentObjectExecReturn* execute(ExecuteOption option = ExecuteAll, bool* touched = nullptr);

    /**
     * @brief Find the paths to a given document object.
     *
     * @param[in] obj The document object to find paths to.
     * @param[out] paths Object identifiers that point to @a obj.
     */
    void getPathsToDocumentObject(DocumentObject* obj,
                                  std::vector<App::ObjectIdentifier>& paths) const;

    /**
     * @brief Check if any dependencies are touched.
     *
     * Determine whether any dependencies of any of the registered expressions
     * have been touched.
     *
     * @return True if at least on dependency has been touched.
     */
    bool depsAreTouched() const;

    /**
     * @brief Set an extra validator function.
     *
     * @param[in] f The validator function.
     */
    void setValidator(ValidatorFunc f)
    {
        validator = f;
    }

    /**
     * @brief Validate the expression expression for a given path.
     *
     * @param[in] path The object identifier that the expression is targeting.
     * @param expr The expression to validate.
     *
     * @return An empty string on success, an error message on failure.
     */
    std::string validateExpression(const App::ObjectIdentifier& path,
                                   std::shared_ptr<const App::Expression> expr) const;

    /**
     * @brief Rename object identifiers in the registered expressions.
     *
     * @param[in] paths A map with the current and new object identifiers.
     */
    void renameExpressions(const std::map<App::ObjectIdentifier, App::ObjectIdentifier>& paths);

    /**
     * @brief Rename object identifiers in the registered expressions.
     *
     * @param[in] paths A map with the current and new object identifiers.
     */
    void
    renameObjectIdentifiers(const std::map<App::ObjectIdentifier, App::ObjectIdentifier>& paths);

    /**
     * @brief Create a canonical object identifier of the given object \a p.
     *
     * @param oid The object identifier from which we want a canonical path.
     * @return The canonical object identifier.
     */
    App::ObjectIdentifier canonicalPath(const App::ObjectIdentifier& oid) const override;

    /// Get the number of expressions managed by this object.
    size_t numExpressions() const;

    /// signal called when an expression was changed
    fastsignals::signal<void(const App::ObjectIdentifier&)> expressionChanged;

    void afterRestore() override;
    void onContainerRestored() override;

    void getLinksTo(std::vector<App::ObjectIdentifier>& identifiers,
                    App::DocumentObject* obj,
                    const char* subname = nullptr,
                    bool all = false) const override;

    /* Python interface */
    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

protected:
    void hasSetValue() override;

private:
    using Edge = std::pair<int, int>;
// Note: use std::map instead of unordered_map to keep the binding order stable
#if defined(FC_OS_MACOSX) || defined(FC_OS_BSD) || defined(_LIBCPP_VERSION)
    using ExpressionMap = std::map<App::ObjectIdentifier, ExpressionInfo>;
#else
    using ExpressionMap = std::map<const App::ObjectIdentifier, ExpressionInfo>;
#endif

    /**
     * @brief Compute the evaluation order of the expressions.
     *
     * This method builds a graph for all expressions in the engine, and finds
     * any circular dependencies.  It also computes the internal evaluation
     * order, in case properties depend on each other.
     *
     * @param[in] option Execution option, see ExecuteOption.
     *
     * @return A vector with the evaluation order of the properties and their
     * dependencies in terms of object identifiers.
     *
     * @throws Base::RuntimeError if a circular dependency is detected.
     */
    std::vector<App::ObjectIdentifier> computeEvaluationOrder(ExecuteOption option);

    void slotChangedObject(const App::DocumentObject& obj, const App::Property& prop);
    void slotChangedProperty(const App::DocumentObject& obj, const App::Property& prop);
    void updateHiddenReference(const std::string& key);

    bool running = false; /**< Boolean used to avoid loops */
    bool restoring = false;

    ExpressionMap expressions; /**< Stored expressions */

    ValidatorFunc validator; /**< Valdiator functor */

    struct RestoredExpression
    {
        std::string path;
        std::string expr;
        std::string comment;
    };
    /**< Expressions are read from file to this map first before they are validated and inserted
     * into the actual map */
    std::unique_ptr<std::vector<RestoredExpression>> restoredExpressions;

    void tryRestoreExpression(DocumentObject* docObj, const RestoredExpression& info);

    struct Private;
    std::unique_ptr<Private> pimpl;

    friend class AtomicPropertyChange;
};

}  // namespace App
