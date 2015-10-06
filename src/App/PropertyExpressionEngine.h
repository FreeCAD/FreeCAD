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

#ifndef EXPRESSIONENGINE_H
#define EXPRESSIONENGINE_H

#include <boost/unordered/unordered_map.hpp>
#include <boost/function.hpp>
#include <boost/signals.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <App/Property.h>
#include <App/Expression.h>
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

class AppExport PropertyExpressionEngine : public App::Property
{
    TYPESYSTEM_HEADER();
public:

    typedef boost::function<std::string (const App::ObjectIdentifier & path, boost::shared_ptr<const App::Expression> expr)> ValidatorFunc;
    
    /**
     * @brief The ExpressionInfo struct encapsulates an expression and a comment.
     */

    struct ExpressionInfo {
        boost::shared_ptr<App::Expression> expression; /**< The actual expression tree */
        std::string comment; /**< Optional comment for this expression */

        ExpressionInfo(boost::shared_ptr<App::Expression> expression = boost::shared_ptr<App::Expression>(), const char * comment = 0) {
            this->expression = expression;
            if (comment)
                this->comment = comment;
        }

        ExpressionInfo(const ExpressionInfo & other) {
            expression = other.expression;
            comment = other.comment;
        }

        ExpressionInfo & operator=(const ExpressionInfo & other) {
            expression = other.expression;
            comment = other.comment;
            return *this;
        }
    };

    PropertyExpressionEngine();
    ~PropertyExpressionEngine();

    unsigned int getMemSize (void) const;

    void setValue() { } // Dummy

    Property *Copy(void) const;

    void Paste(const Property &from);

    void Save (Base::Writer & writer) const;

    void Restore(Base::XMLReader &reader);

    void setValue(const App::ObjectIdentifier &path, boost::shared_ptr<App::Expression> expr, const char * comment = 0);

    const boost::any getPathValue(const App::ObjectIdentifier & path) const;

    DocumentObjectExecReturn * execute();

    void getDocumentObjectDeps(std::vector<DocumentObject*> & docObjs) const;

    bool depsAreTouched() const;

    boost::unordered_map<const App::ObjectIdentifier, const ExpressionInfo> getExpressions() const;

    /* Expression validator */
    void setValidator(ValidatorFunc f) { validator = f; }

    std::string validateExpression(const App::ObjectIdentifier & path, boost::shared_ptr<const App::Expression> expr) const;

    void renameExpressions(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> &paths);

    void renameObjectIdentifiers(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> & paths);

    const App::ObjectIdentifier canonicalPath(const App::ObjectIdentifier &p) const;

    size_t numExpressions() const;

    void slotObjectRenamed(const App::DocumentObject & obj);
    
    ///signal called when a expression was changed 
    boost::signal<void (const App::ObjectIdentifier &)> expressionChanged; 

    /* Python interface */
    PyObject *getPyObject(void);
    void setPyObject(PyObject *);

private:

    typedef boost::adjacency_list< boost::listS, boost::vecS, boost::directedS > DiGraph;
    typedef std::pair<int, int> Edge;
    typedef boost::unordered_map<const App::ObjectIdentifier, ExpressionInfo> ExpressionMap;

    std::vector<App::ObjectIdentifier> computeEvaluationOrder();

    void buildGraphStructures(const App::ObjectIdentifier &path,
                              const boost::shared_ptr<Expression> expression, boost::unordered_map<App::ObjectIdentifier, int> &nodes,
                              boost::unordered_map<int, App::ObjectIdentifier> &revNodes, std::vector<Edge> &edges) const;

    void buildGraph(const ExpressionMap &exprs,
                    boost::unordered_map<int, App::ObjectIdentifier> &revNodes, DiGraph &g) const;

    bool running; /**< Boolean used to avoid loops */

    ExpressionMap expressions; /**< Stored expressions */

    ValidatorFunc validator; /**< Valdiator functor */

};

}

#endif // EXPRESSIONENGINE_H
