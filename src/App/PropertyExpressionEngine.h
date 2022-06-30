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
    TYPESYSTEM_HEADER();
public:
    PropertyExpressionContainer();
    virtual ~PropertyExpressionContainer();

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

    virtual void updateElementReference(
            App::DocumentObject *feature, bool reverse=false, bool notify=false) override;
    virtual bool referenceChanged() const override;
    virtual bool adjustLink(const std::set<App::DocumentObject *> &inList) override;
    virtual Property *CopyOnImportExternal(const std::map<std::string,std::string> &nameMap) const override;
    virtual Property *CopyOnLabelChange(App::DocumentObject *obj, 
                        const std::string &ref, const char *newLabel) const override;
    virtual Property *CopyOnLinkReplace(const App::DocumentObject *parent,
                        App::DocumentObject *oldObj, App::DocumentObject *newObj) const override;

    typedef std::function<std::string (const App::ObjectIdentifier & path, std::shared_ptr<const App::Expression> expr)> ValidatorFunc;

    /**
     * @brief The ExpressionInfo struct encapsulates an expression.
     */

    struct ExpressionInfo {
        std::shared_ptr<App::Expression> expression; /**< The actual expression tree */
        bool busy;

        ExpressionInfo(std::shared_ptr<App::Expression> expression = std::shared_ptr<App::Expression>()) {
            this->expression = expression;
            this->busy = false;
        }

        ExpressionInfo(const ExpressionInfo & other) {
            expression = other.expression;
            busy = other.busy;
        }

        ExpressionInfo & operator=(const ExpressionInfo & other) {
            expression = other.expression;
            busy = other.busy;
            return *this;
        }
    };

    PropertyExpressionEngine();
    ~PropertyExpressionEngine();

    unsigned int getMemSize (void) const override;

    virtual std::map<App::ObjectIdentifier, const App::Expression*> getExpressions() const override;
    virtual void setExpressions(std::map<App::ObjectIdentifier, App::ExpressionPtr> &&exprs) override;
    virtual void onRelabeledDocument(const App::Document &doc) override;

    void setValue() { } // Dummy

    Property *Copy(void) const override;

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

    virtual void afterRestore() override;
    virtual void onContainerRestored() override;

    /* Python interface */
    PyObject *getPyObject(void) override;
    void setPyObject(PyObject *) override;

protected:
    virtual void hasSetValue() override;

private:

    typedef boost::adjacency_list< boost::listS, boost::vecS, boost::directedS > DiGraph;
    typedef std::pair<int, int> Edge;
    // Note: use std::map instead of unordered_map to keep the binding order stable
    #if defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    typedef std::map<App::ObjectIdentifier, ExpressionInfo> ExpressionMap;
    #else
    typedef std::map<const App::ObjectIdentifier, ExpressionInfo> ExpressionMap;
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

    bool running; /**< Boolean used to avoid loops */
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
