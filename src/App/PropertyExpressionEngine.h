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

#ifndef EXPRESSIONENGINE_H
#define EXPRESSIONENGINE_H

#include <functional>
#include <boost/unordered/unordered_map.hpp>
#include <boost_signals2.hpp>
#include <boost_graph_adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <App/PropertyLinks.h>
#include <set>

namespace Base {
class Writer;
class XMLReader;
}

namespace App {

class DocumentObject;
class DocumentObjectExecReturn;
class ObjectIdentifier;
class Expression;
using ExpressionPtr = std::unique_ptr<Expression>;

class AppExport PropertyExpressionContainer : public App::PropertyXLinkContainer
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    PropertyExpressionContainer();
    ~PropertyExpressionContainer() override;

    virtual std::map<App::ObjectIdentifier, const App::Expression*> getExpressions() const = 0;
    virtual void setExpressions(std::map<App::ObjectIdentifier, App::ExpressionPtr> &&exprs) = 0;

protected:
    virtual void onRelabeledDocument(const App::Document &doc) = 0;

private:
    static void slotRelabelDocument(const App::Document &doc);
};

class AppExport PropertyExpressionEngine : public App::PropertyExpressionContainer,
                                           private App::AtomicPropertyChangeInterface<PropertyExpressionEngine>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:

    void updateElementReference(
            App::DocumentObject *feature, bool reverse=false, bool notify=false) override;
    bool referenceChanged() const override;
    bool adjustLink(const std::set<App::DocumentObject *> &inList) override;
    Property *CopyOnImportExternal(const std::map<std::string,std::string> &nameMap) const override;
    Property *CopyOnLabelChange(App::DocumentObject *obj,
                        const std::string &ref, const char *newLabel) const override;
    Property *CopyOnLinkReplace(const App::DocumentObject *parent,
                        App::DocumentObject *oldObj, App::DocumentObject *newObj) const override;

    using ValidatorFunc = std::function<std::string (const App::ObjectIdentifier & path, std::shared_ptr<const App::Expression> expr)>;

    /**
     * @brief The ExpressionInfo struct encapsulates an expression.
     */

    struct ExpressionInfo {
        std::shared_ptr<App::Expression> expression; /**< The actual expression tree */
        bool busy;

        explicit ExpressionInfo(std::shared_ptr<App::Expression> expression = std::shared_ptr<App::Expression>()) {
            this->expression = expression;
            this->busy = false;
        }

        ExpressionInfo(const ExpressionInfo &) = default;

        ExpressionInfo & operator=(const ExpressionInfo &) = default;
    };

    PropertyExpressionEngine();
    ~PropertyExpressionEngine() override;

    unsigned int getMemSize () const override;

    std::map<App::ObjectIdentifier, const App::Expression*> getExpressions() const override;
    void setExpressions(std::map<App::ObjectIdentifier, App::ExpressionPtr> &&exprs) override;
    void onRelabeledDocument(const App::Document &doc) override;

    void setValue() { } // Dummy

    Property *Copy() const override;

    void Paste(const Property &from) override;

    void Save (Base::Writer & writer) const override;

    void Restore(Base::XMLReader &reader) override;

    void setValue(const App::ObjectIdentifier &path, std::shared_ptr<App::Expression> expr);

    const boost::any getPathValue(const App::ObjectIdentifier & path) const override;

    /// Execute options
    enum ExecuteOption {
        /// Execute all expression
        ExecuteAll,
        /// Execute only output property bindings
        ExecuteOutput,
        /// Execute only non-output property bindings
        ExecuteNonOutput,
        /// Execute on document restore
        ExecuteOnRestore,
    };
    /** Evaluate the expressions
     *
     * @param option: execution option, see ExecuteOption.
     */
    DocumentObjectExecReturn * execute(ExecuteOption option=ExecuteAll, bool *touched=nullptr);

    void getPathsToDocumentObject(DocumentObject*, std::vector<App::ObjectIdentifier> & paths) const;

    bool depsAreTouched() const;

    /* Expression validator */
    void setValidator(ValidatorFunc f) { validator = f; }

    std::string validateExpression(const App::ObjectIdentifier & path, std::shared_ptr<const App::Expression> expr) const;

    void renameExpressions(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> &paths);

    void renameObjectIdentifiers(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> & paths);

    App::ObjectIdentifier canonicalPath(const App::ObjectIdentifier &p) const override;

    size_t numExpressions() const;

    ///signal called when an expression was changed
    boost::signals2::signal<void (const App::ObjectIdentifier &)> expressionChanged;

    void afterRestore() override;
    void onContainerRestored() override;

    /* Python interface */
    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;

protected:
    void hasSetValue() override;

private:

    using DiGraph = boost::adjacency_list< boost::listS, boost::vecS, boost::directedS >;
    using Edge = std::pair<int, int>;
    // Note: use std::map instead of unordered_map to keep the binding order stable
    #if defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    using ExpressionMap = std::map<App::ObjectIdentifier, ExpressionInfo>;
    #else
    using ExpressionMap = std::map<const App::ObjectIdentifier, ExpressionInfo>;
    #endif

    std::vector<App::ObjectIdentifier> computeEvaluationOrder(ExecuteOption option);

    void buildGraphStructures(const App::ObjectIdentifier &path,
                              const std::shared_ptr<Expression> expression, boost::unordered_map<App::ObjectIdentifier, int> &nodes,
                              boost::unordered_map<int, App::ObjectIdentifier> &revNodes, std::vector<Edge> &edges) const;

    void buildGraph(const ExpressionMap &exprs,
                boost::unordered_map<int, App::ObjectIdentifier> &revNodes,
                DiGraph &g, ExecuteOption option=ExecuteAll) const;

    void slotChangedObject(const App::DocumentObject &obj, const App::Property &prop);
    void slotChangedProperty(const App::DocumentObject &obj, const App::Property &prop);
    void updateHiddenReference(const std::string &key);

    bool running = false; /**< Boolean used to avoid loops */
    bool restoring = false;

    ExpressionMap expressions; /**< Stored expressions */

    ValidatorFunc validator; /**< Valdiator functor */

    struct RestoredExpression {
        std::string path;
        std::string expr;
        std::string comment;
    };
    /**< Expressions are read from file to this map first before they are validated and inserted into the actual map */
    std::unique_ptr<std::vector<RestoredExpression> > restoredExpressions;

    struct Private;
    std::unique_ptr<Private> pimpl;

    friend class AtomicPropertyChange;

};

}

#endif // EXPRESSIONENGINE_H
