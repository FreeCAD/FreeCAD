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

#include "PreCompiled.h"

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/Writer.h>
#include <CXX/Objects.hxx>

#include "PropertyExpressionEngine.h"
#include "ExpressionVisitors.h"


FC_LOG_LEVEL_INIT("App", true);

using namespace App;
using namespace Base;
using namespace boost;
namespace sp = std::placeholders;

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyExpressionContainer , App::PropertyXLinkContainer)

static std::set<PropertyExpressionContainer*> _ExprContainers;

PropertyExpressionContainer::PropertyExpressionContainer() {
    static bool inited;
    if(!inited) {
        inited = true;
        GetApplication().signalRelabelDocument.connect(PropertyExpressionContainer::slotRelabelDocument);
    }
    _ExprContainers.insert(this);
}

PropertyExpressionContainer::~PropertyExpressionContainer() {
    _ExprContainers.erase(this);
}

void PropertyExpressionContainer::slotRelabelDocument(const App::Document &doc) {
    // For use a private _ExprContainers to track all living
    // PropertyExpressionContainer including those inside undo/redo stack,
    // because document relabel is not undoable/redoable.

    if(doc.getOldLabel() != doc.Label.getValue()) {
        for(auto prop : _ExprContainers)
            prop->onRelabeledDocument(doc);
    }
}

///////////////////////////////////////////////////////////////////////////////////////

struct PropertyExpressionEngine::Private {
    // For some reason, MSVC has trouble with vector of scoped_connection if
    // defined in header, hence the private structure here.
    std::vector<boost::signals2::scoped_connection> conns;
    std::unordered_map<std::string, std::vector<ObjectIdentifier> > propMap;
};

///////////////////////////////////////////////////////////////////////////////////////

TYPESYSTEM_SOURCE(App::PropertyExpressionEngine , App::PropertyExpressionContainer)

/**
 * @brief Construct a new PropertyExpressionEngine object.
 */

PropertyExpressionEngine::PropertyExpressionEngine()
    : validator(0)
{
}

/**
 * @brief Destroy the PropertyExpressionEngine object.
 */

PropertyExpressionEngine::~PropertyExpressionEngine() = default;

/**
 * @brief Estimate memory size of this property.
 *
 * \fixme Should probably return something else than 0.
 *
 * @return Size of object.
 */

unsigned int PropertyExpressionEngine::getMemSize() const
{
    return 0;
}

Property *PropertyExpressionEngine::Copy() const
{
    PropertyExpressionEngine * engine = new PropertyExpressionEngine();

    for (const auto & it : expressions) {
        ExpressionInfo info;
        if (it.second.expression)
            info.expression = std::shared_ptr<Expression>(it.second.expression->copy());
        engine->expressions[it.first] = info;
    }

    engine->validator = validator;

    return engine;
}

void PropertyExpressionEngine::hasSetValue()
{
    App::DocumentObject *owner = dynamic_cast<App::DocumentObject*>(getContainer());
    if(!owner || !owner->getNameInDocument() || owner->isRestoring() || testFlag(LinkDetached)) {
        PropertyExpressionContainer::hasSetValue();
        return;
    }

    std::map<App::DocumentObject*,bool> deps;
    std::vector<std::string> labels;
    unregisterElementReference();
    UpdateElementReferenceExpressionVisitor<PropertyExpressionEngine> v(*this);
    for(auto &e : expressions) {
        auto expr = e.second.expression;
        if(expr) {
            expr->getDepObjects(deps,&labels);
            if(!restoring)
                expr->visit(v);
        }
    }
    registerLabelReferences(std::move(labels));

    updateDeps(std::move(deps));

    if(pimpl) {
        pimpl->conns.clear();
        pimpl->propMap.clear();
    }
    // check if there is any hidden references
    bool hasHidden = false;
    for(auto &v : _Deps) {
        if(v.second) {
            hasHidden = true;
            break;
        }
    }
    if(hasHidden) {
        if(!pimpl) {
            pimpl = std::make_unique<Private>();
        }
        for(auto &e : expressions) {
            auto expr = e.second.expression;
            if(!expr) continue;
            for(auto &dep : expr->getIdentifiers()) {
                if(!dep.second)
                    continue;
                const ObjectIdentifier &var = dep.first;
                for(auto &vdep : var.getDep(true)) {
                    auto obj = vdep.first;
                    auto objName = obj->getFullName() + ".";
                    for(auto &propName : vdep.second) {
                        std::string key = objName + propName;
                        auto &propDeps = pimpl->propMap[key];
                        if(propDeps.empty()) {
                            //NOLINTBEGIN
                            if(!propName.empty()) {
                                pimpl->conns.emplace_back(obj->signalChanged.connect(std::bind(
                                            &PropertyExpressionEngine::slotChangedProperty,this,sp::_1,sp::_2)));
                            }
                            else {
                                pimpl->conns.emplace_back(obj->signalChanged.connect(std::bind(
                                            &PropertyExpressionEngine::slotChangedObject,this,sp::_1,sp::_2)));
                            }
                            //NOLINTEND
                        }
                        propDeps.push_back(e.first);
                    }
                }
            }
        }
    }

    PropertyExpressionContainer::hasSetValue();
}

void PropertyExpressionEngine::updateHiddenReference(const std::string &key) {
    if(!pimpl)
        return;
    auto it = pimpl->propMap.find(key);
    if(it == pimpl->propMap.end())
        return;
    for(auto &var : it->second) {
        auto it = expressions.find(var);
        if(it == expressions.end() || it->second.busy)
            continue;
        Property *myProp = var.getProperty();
        if(!myProp)
            continue;
        Base::StateLocker guard(it->second.busy);
        App::any value;
        try {
            value = it->second.expression->getValueAsAny();
            if(!isAnyEqual(value, myProp->getPathValue(var)))
                myProp->setPathValue(var, value);
        }catch(Base::Exception &e) {
            e.ReportException();
            FC_ERR("Failed to evaluate property binding "
                    << myProp->getFullName() << " on change of " << key);
        }catch(std::bad_cast &) {
            FC_ERR("Invalid type '" << value.type().name()
                    << "' in property binding " << myProp->getFullName()
                    << " on change of " << key);
        }catch(std::exception &e) {
            FC_ERR(e.what());
            FC_ERR("Failed to evaluate property binding "
                    << myProp->getFullName() << " on change of " << key);
        }
    }
}

void PropertyExpressionEngine::slotChangedObject(const App::DocumentObject &obj, const App::Property &) {
    updateHiddenReference(obj.getFullName());
}

void PropertyExpressionEngine::slotChangedProperty(const App::DocumentObject &, const App::Property &prop) {
    updateHiddenReference(prop.getFullName());
}

void PropertyExpressionEngine::Paste(const Property &from)
{
    const PropertyExpressionEngine &fromee = dynamic_cast<const PropertyExpressionEngine&>(from);

    AtomicPropertyChange signaller(*this);

    expressions.clear();
    for(auto &e : fromee.expressions) {
        ExpressionInfo info;
        if (e.second.expression)
            info.expression = std::shared_ptr<Expression>(e.second.expression->copy());
        expressions[e.first] = info;
        expressionChanged(e.first);
    }
    validator = fromee.validator;
    signaller.tryInvoke();
}

void PropertyExpressionEngine::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<ExpressionEngine count=\"" <<  expressions.size();
    if(PropertyExpressionContainer::_XLinks.empty()) {
        writer.Stream() << "\">" << std::endl;
        writer.incInd();
    } else {
        writer.Stream() << R"(" xlink="1">)" << std::endl;
        writer.incInd();
        PropertyExpressionContainer::Save(writer);
    }
    for (const auto & it : expressions) {
        std::string expression, comment;
        if (it.second.expression) {
            expression = it.second.expression->toString(true);
            comment = it.second.expression->comment;
        }

        writer.Stream() << writer.ind() << "<Expression path=\""
            << Property::encodeAttribute(it.first.toString()) <<"\" expression=\""
            << Property::encodeAttribute(expression) << "\"";
        if (!comment.empty())
            writer.Stream() << " comment=\""
                << Property::encodeAttribute(comment) << "\"";
        writer.Stream() << "/>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</ExpressionEngine>" << std::endl;
}

void PropertyExpressionEngine::Restore(Base::XMLReader &reader)
{
    reader.readElement("ExpressionEngine");
    int count = reader.getAttributeAsFloat("count");

    if(reader.hasAttribute("xlink") && reader.getAttributeAsInteger("xlink"))
        PropertyExpressionContainer::Restore(reader);

    restoredExpressions = std::make_unique<std::vector<RestoredExpression>>();
    restoredExpressions->reserve(count);
    for (int i = 0; i < count; ++i) {

        reader.readElement("Expression");
        restoredExpressions->emplace_back();
        auto &info = restoredExpressions->back();
        info.path = reader.getAttribute("path");
        info.expr = reader.getAttribute("expression");
        if(reader.hasAttribute("comment"))
            info.comment = reader.getAttribute("comment");
    }

    reader.readEndElement("ExpressionEngine");
}

/**
 * @brief Update graph structure with given path and expression.
 * @param path Path
 * @param expression Expression to query for dependencies
 * @param nodes Map with nodes of graph, including dependencies of 'expression'
 * @param revNodes Reverse map of the nodes, containing only the given paths, without dependencies.
 * @param edges Edges in graph
 */

void PropertyExpressionEngine::buildGraphStructures(const ObjectIdentifier & path,
                                                    const std::shared_ptr<Expression> expression,
                                                    boost::unordered_map<ObjectIdentifier, int> & nodes,
                                                    boost::unordered_map<int, ObjectIdentifier> & revNodes,
                                                    std::vector<Edge> & edges) const
{
    /* Insert target property into nodes structure */
    if (nodes.find(path) == nodes.end()) {
        int s = nodes.size();

        revNodes[s] = path;
        nodes[path] = s;
    }
    else {
        revNodes[nodes[path]] = path;
    }

    /* Insert dependencies into nodes structure */
    ExpressionDeps deps;
    if (expression)
        deps = expression->getDeps();

    for(auto &dep : deps) {
        for(auto &info : dep.second) {
            if(info.first.empty())
                continue;
            for(auto &oid : info.second) {
                ObjectIdentifier cPath(oid.canonicalPath());
                if (nodes.find(cPath) == nodes.end()) {
                    int s = nodes.size();
                    nodes[cPath] = s;
                }
                edges.emplace_back(nodes[path], nodes[cPath]);
            }
        }
    }
}

/**
 * @brief Create a canonical object identifier of the given object \a p.
 * @param p ObjectIndentifier
 * @return New ObjectIdentifier
 */

ObjectIdentifier PropertyExpressionEngine::canonicalPath(const ObjectIdentifier &p) const
{
    DocumentObject * docObj = freecad_dynamic_cast<DocumentObject>(getContainer());

    // Am I owned by a DocumentObject?
    if (!docObj)
        throw Base::RuntimeError("PropertyExpressionEngine must be owned by a DocumentObject.");

    int ptype;
    Property * prop = p.getProperty(&ptype);

    // p pointing to a property...?
    if (!prop)
        throw Base::RuntimeError(p.resolveErrorString().c_str());

    if(ptype || prop->getContainer()!=getContainer())
        return p;

    // In case someone calls this with p pointing to a PropertyExpressionEngine for some reason
    if (prop->isDerivedFrom(PropertyExpressionEngine::classTypeId))
        return p;

    // Dispatch call to actual canonicalPath implementation
    return p.canonicalPath();
}

/**
 * @brief Number of expressions managed by this object.
 * @return Number of expressions.
 */

size_t PropertyExpressionEngine::numExpressions() const
{
    return expressions.size();
}

void PropertyExpressionEngine::afterRestore()
{
    DocumentObject * docObj = freecad_dynamic_cast<DocumentObject>(getContainer());
    if(restoredExpressions && docObj) {
        Base::FlagToggler<bool> flag(restoring);
        AtomicPropertyChange signaller(*this);

        PropertyExpressionContainer::afterRestore();
        ObjectIdentifier::DocumentMapper mapper(this->_DocMap);

        for(auto &info : *restoredExpressions) {
            ObjectIdentifier path = ObjectIdentifier::parse(docObj, info.path);
            if (!info.expr.empty()) {
                std::shared_ptr<Expression> expression(Expression::parse(docObj, info.expr.c_str()));
                if(expression)
                    expression->comment = std::move(info.comment);
                setValue(path, expression);
            }
        }
        signaller.tryInvoke();
    }
    restoredExpressions.reset();
}

void PropertyExpressionEngine::onContainerRestored() {
    Base::FlagToggler<bool> flag(restoring);
    unregisterElementReference();
    UpdateElementReferenceExpressionVisitor<PropertyExpressionEngine> v(*this);
    for(auto &e : expressions) {
        auto expr = e.second.expression;
        if(expr)
            expr->visit(v);
    }
}

/**
 * @brief Get expression for \a path.
 * @param path ObjectIndentifier to query for.
 * @return Expression for \a path, or empty boost::any if not found.
 */

const boost::any PropertyExpressionEngine::getPathValue(const App::ObjectIdentifier & path) const
{
    // Get a canonical path
    ObjectIdentifier usePath(canonicalPath(path));

    ExpressionMap::const_iterator i = expressions.find(usePath);

    if (i != expressions.end())
        return i->second;
    else
        return boost::any();
}

/**
 * @brief Set expression with optional comment for \a path.
 * @param path Path to update
 * @param expr New expression
 * @param comment Optional comment.
 */

void PropertyExpressionEngine::setValue(const ObjectIdentifier & path, std::shared_ptr<Expression> expr)
{
    ObjectIdentifier usePath(canonicalPath(path));
    const Property * prop = usePath.getProperty();

    // Try to access value; it should trigger an exception if it is not supported, or if the path is invalid
    prop->getPathValue(usePath);

    // Check if the current expression equals the new one and do nothing if so to reduce unneeded computations
    ExpressionMap::iterator it = expressions.find(usePath);
    if(it != expressions.end()
            && (expr == it->second.expression ||
                (expr && it->second.expression
                 && expr->isSame(*it->second.expression))))
    {
        return;
    }

    if (expr) {
        std::string error = validateExpression(usePath, expr);
        if (!error.empty())
            throw Base::RuntimeError(error.c_str());
        AtomicPropertyChange signaller(*this);
        expressions[usePath] = ExpressionInfo(expr);
        expressionChanged(usePath);
        signaller.tryInvoke();
    } else if (it != expressions.end()) {
        AtomicPropertyChange signaller(*this);
        expressions.erase(it);
        expressionChanged(usePath);
        signaller.tryInvoke();
    }
}

/**
 * @brief The cycle_detector struct is used by the boost graph routines to detect cycles in the graph.
 */

struct cycle_detector : public boost::dfs_visitor<> {
    cycle_detector( bool& has_cycle, int & src)
      : _has_cycle(has_cycle), _src(src) { }

    template <class Edge, class Graph>
    void back_edge(Edge e, Graph&g) {
      _has_cycle = true;
      _src = source(e, g);
    }

  protected:
    bool& _has_cycle;
    int & _src;
};

/**
 * @brief Build a graph of all expressions in \a exprs.
 * @param exprs Expressions to use in graph
 * @param revNodes Map from int[nodeid] to ObjectIndentifer.
 * @param g Graph to update. May contain additional nodes than in revNodes, because of outside dependencies.
 */

void PropertyExpressionEngine::buildGraph(const ExpressionMap & exprs,
                    boost::unordered_map<int, ObjectIdentifier> & revNodes,
                    DiGraph & g, ExecuteOption option) const
{
    boost::unordered_map<ObjectIdentifier, int> nodes;
    std::vector<Edge> edges;

    // Build data structure for graph
    for (const auto & expr : exprs) {
        if(option!=ExecuteAll) {
            auto prop = expr.first.getProperty();
            if(!prop)
                throw Base::RuntimeError("Path does not resolve to a property.");
            bool is_output = prop->testStatus(App::Property::Output)||(prop->getType()&App::Prop_Output);
            if((is_output && option==ExecuteNonOutput) || (!is_output && option==ExecuteOutput))
                continue;
            if(option == ExecuteOnRestore
                    && !prop->testStatus(Property::Transient)
                    && !(prop->getType() & Prop_Transient)
                    && !prop->testStatus(Property::EvalOnRestore))
                continue;
        }
        buildGraphStructures(expr.first, expr.second.expression, nodes, revNodes, edges);
    }

    // Create graph
    g = DiGraph(nodes.size());

    // Add edges to graph
    for (const auto & edge : edges)
        add_edge(edge.first, edge.second, g);

    // Check for cycles
    bool has_cycle = false;
    int src = -1;
    cycle_detector vis(has_cycle, src);
    depth_first_search(g, visitor(vis));

    if (has_cycle) {
        std::string s =  revNodes[src].toString() + " reference creates a cyclic dependency.";

        throw Base::RuntimeError(s.c_str());
    }
}

/**
 * The code below builds a graph for all expressions in the engine, and
 * finds any circular dependencies. It also computes the internal evaluation
 * order, in case properties depends on each other.
 */

std::vector<App::ObjectIdentifier> PropertyExpressionEngine::computeEvaluationOrder(ExecuteOption option)
{
    std::vector<App::ObjectIdentifier> evaluationOrder;
    boost::unordered_map<int, ObjectIdentifier> revNodes;
    DiGraph g;

    buildGraph(expressions, revNodes, g, option);

    /* Compute evaluation order for expressions */
    std::vector<int> c;
    topological_sort(g, std::back_inserter(c));

    for (int i : c) {
        // we return the evaluation order for our properties, not the dependencies
        // the topo sort will contain node ids for both our props and their deps
        if (revNodes.find(i) != revNodes.end())
            evaluationOrder.push_back(revNodes[i]);
    }

    return evaluationOrder;
}

/**
 * @brief Compute and update values of all registered expressions.
 * @return StdReturn on success.
 */

DocumentObjectExecReturn *App::PropertyExpressionEngine::execute(ExecuteOption option, bool *touched)
{
    DocumentObject * docObj = freecad_dynamic_cast<DocumentObject>(getContainer());

    if (!docObj)
        throw Base::RuntimeError("PropertyExpressionEngine must be owned by a DocumentObject.");

    if (running)
        return DocumentObject::StdReturn;

    if(option == ExecuteOnRestore) {
        bool found = false;
        for(auto &e : expressions) {
            auto prop = e.first.getProperty();
            if(!prop)
                continue;
            if(prop->testStatus(App::Property::Transient)
                    || (prop->getType()&App::Prop_Transient)
                    || prop->testStatus(App::Property::EvalOnRestore))
            {
                found = true;
                break;
            }
        }
        if(!found)
            return DocumentObject::StdReturn;
    }

    /* Resetter class, to ensure that the "running" variable gets set to false, even if
     * an exception is thrown.
     */

    class resetter {
    public:
        explicit resetter(bool & b) : _b(b) { _b = true; }
        ~resetter() { _b = false; }

    private:
        bool & _b;
    };

    resetter r(running);

    // Compute evaluation order
    std::vector<App::ObjectIdentifier> evaluationOrder = computeEvaluationOrder(option);
    std::vector<ObjectIdentifier>::const_iterator it = evaluationOrder.begin();

#ifdef FC_PROPERTYEXPRESSIONENGINE_LOG
    std::clog << "Computing expressions for " << getName() << std::endl;
#endif

    /* Evaluate the expressions, and update properties */
    for (;it != evaluationOrder.end();++it) {

        // Get property to update
        Property * prop = it->getProperty();

        if (!prop)
            throw Base::RuntimeError("Path does not resolve to a property.");

        DocumentObject* parent = freecad_dynamic_cast<DocumentObject>(prop->getContainer());

        /* Make sure property belongs to the same container as this PropertyExpressionEngine */
        if (parent != docObj)
            throw Base::RuntimeError("Invalid property owner.");

        /* Set value of property */
        App::any value;
        try {
            // Evaluate expression
            std::shared_ptr<App::Expression> expression = expressions[*it].expression;
            if (expression) {
                value = expression->getValueAsAny();

                // Enable value comparison for all expression bindings to reduce
                // unnecessary touch and recompute.
                //
                // This optimization is necessary for some hidden reference to
                // work because it introduce dependency loop. The repeativtive
                // recompute can be stopped if the expression evaluates the same
                // value.
                //
                // In the future, we can generalize the optimization to all
                // property modification, i.e. do not touch unless value change
                //
                // if (option == ExecuteOnRestore && prop->testStatus(Property::EvalOnRestore))
                {
                    if (isAnyEqual(value, prop->getPathValue(*it)))
                        continue;
                    if (touched)
                        *touched = true;
                }
                prop->setPathValue(*it, value);
            }
        }catch(Base::Exception &e) {
            std::ostringstream ss;
            ss << e.what() << std::endl << "in property binding '" << prop->getFullName() << "'";
            e.setMessage(ss.str());
            throw;
        }catch(std::bad_cast &) {
            std::ostringstream ss;
            ss << "Invalid type '" << value.type().name() << "'";
            ss << "\nin property binding '" << prop->getFullName() << "'";
            throw Base::TypeError(ss.str().c_str());
        }catch(std::exception &e) {
            std::ostringstream ss;
            ss << e.what() << "\nin property binding '" << prop->getFullName() << "'";
            throw Base::RuntimeError(ss.str().c_str());
        }
    }
    return DocumentObject::StdReturn;
}

/**
 * @brief Find paths to document object.
 * @param obj Document object
 * @param paths Object identifier
 */

void PropertyExpressionEngine::getPathsToDocumentObject(DocumentObject* obj,
                                 std::vector<App::ObjectIdentifier> & paths) const
{
    DocumentObject * owner = freecad_dynamic_cast<DocumentObject>(getContainer());

    if (!owner || owner==obj)
        return;

    for(auto &v : expressions) {
        if (!v.second.expression)
            continue;
        const auto &deps = v.second.expression->getDeps();
        auto it = deps.find(obj);
        if(it==deps.end())
            continue;
        for(auto &dep : it->second)
            paths.insert(paths.end(),dep.second.begin(),dep.second.end());
    }
}

/**
 * @brief Determine whether any dependencies of any of the registered expressions have been touched.
 * @return True if at least on dependency has been touched.
 */

bool PropertyExpressionEngine::depsAreTouched() const
{
    for(auto &v : _Deps) {
        // v.second indicates if it is a hidden reference
        if(!v.second && v.first->isTouched())
            return true;
    }
    return false;
}

/**
 * @brief Validate the given path and expression.
 * @param path Object Identifier for expression.
 * @param expr Expression tree.
 * @return Empty string on success, error message on failure.
 */

std::string PropertyExpressionEngine::validateExpression(const ObjectIdentifier &path, std::shared_ptr<const Expression> expr) const
{
    std::string error;
    ObjectIdentifier usePath(canonicalPath(path));

    if (validator) {
        error = validator(usePath, expr);
        if (!error.empty())
            return error;
    }

    // Get document object
    DocumentObject * pathDocObj = usePath.getDocumentObject();
    assert(pathDocObj);

    auto inList = pathDocObj->getInListEx(true);
    for(auto &v : expr->getDepObjects()) {
        auto docObj = v.first;
        if(!v.second && inList.count(docObj)) {
            std::stringstream ss;
            ss << "cyclic reference to " << docObj->getFullName();
            return ss.str();
        }
    }

    // Check for internal document object dependencies

    // Copy current expressions
    ExpressionMap newExpressions = expressions;

    // Add expression in question
    std::shared_ptr<Expression> exprClone(expr->copy());
    newExpressions[usePath].expression = exprClone;

    // Build graph; an exception will be thrown if it is not a DAG
    try {
        boost::unordered_map<int, ObjectIdentifier> revNodes;
        DiGraph g;

        buildGraph(newExpressions, revNodes, g);
    }
    catch (const Base::Exception & e) {
        return e.what();
    }

    return {};
}

/**
 * @brief Rename paths based on \a paths.
 * @param paths Map with current and new object identifier.
 */

void PropertyExpressionEngine::renameExpressions(const std::map<ObjectIdentifier, ObjectIdentifier> & paths)
{
    ExpressionMap newExpressions;
    std::map<ObjectIdentifier, ObjectIdentifier> canonicalPaths;

    /* ensure input map uses canonical paths */
    for (const auto & path : paths)
        canonicalPaths[canonicalPath(path.first)] = path.second;

    for (ExpressionMap::const_iterator i = expressions.begin(); i != expressions.end(); ++i) {
        std::map<ObjectIdentifier, ObjectIdentifier>::const_iterator j = canonicalPaths.find(i->first);

        // Renamed now?
        if (j != canonicalPaths.end())
            newExpressions[j->second] = i->second;
        else
            newExpressions[i->first] = i->second;
    }

    aboutToSetValue();
    expressions = newExpressions;
    for (ExpressionMap::const_iterator i = expressions.begin(); i != expressions.end(); ++i)
        expressionChanged(i->first);

    hasSetValue();
}

/**
 * @brief Rename object identifiers in the registered expressions.
 * @param paths Map with current and new object identifiers.
 */

void PropertyExpressionEngine::renameObjectIdentifiers(const std::map<ObjectIdentifier, ObjectIdentifier> &paths)
{
    for (const auto & it : expressions) {
        RenameObjectIdentifierExpressionVisitor<PropertyExpressionEngine> v(*this, paths, it.first);
        it.second.expression->visit(v);
    }
}

PyObject *PropertyExpressionEngine::getPyObject()
{
    Py::List list;
    for (const auto & it : expressions) {
        Py::Tuple tuple(2);
        tuple.setItem(0, Py::String(it.first.toString()));
        auto expr = it.second.expression;
        tuple.setItem(1, expr ? Py::String(expr->toString()) : Py::None());
        list.append(tuple);
    }
    return Py::new_reference_to(list);
}

void PropertyExpressionEngine::setPyObject(PyObject *)
{
    throw Base::RuntimeError("Property is read-only");
}

/* The policy implemented in the following function is to auto erase binding in
 * case linked object is gone. I think it is better to cause error and get
 * user's attention
 *
void PropertyExpressionEngine::breakLink(App::DocumentObject *obj, bool clear) {
    auto owner = dynamic_cast<App::DocumentObject*>(getContainer());
    if(!owner)
        return;
    if(_Deps.count(obj)==0 && (!clear || obj!=owner || _Deps.empty()))
        return;
    AtomicPropertyChange signaler(*this);
    for(auto it=expressions.begin(),itNext=it;it!=expressions.end();it=itNext) {
        ++itNext;
        const auto &deps = it->second.expression->getDepObjects();
        if(clear) {
            // here means we are breaking all expression, except those that has
            // no depdenecy or self dependency
            if(deps.empty() || (deps.size()==1 && *deps.begin()==owner))
                continue;
        }else if(!deps.count(obj))
            continue;
        auto path = it->first;
        expressions.erase(it);
        expressionChanged(path);
    }
}
*/

bool PropertyExpressionEngine::adjustLink(const std::set<DocumentObject*> &inList) {
    auto owner = dynamic_cast<App::DocumentObject*>(getContainer());
    if(!owner)
        return false;
    bool found = false;
    for(auto &v : _Deps) {
        if(inList.count(v.first)) {
            found = true;
            break;
        }
    }
    if(!found)
        return false;

    AtomicPropertyChange signaler(*this);
    for(auto &v : expressions) {
        try {
            if(v.second.expression && v.second.expression->adjustLinks(inList))
                expressionChanged(v.first);
        }catch(Base::Exception &e) {
            std::ostringstream ss;
            ss << "Failed to adjust link for " << owner->getFullName() << " in expression "
                << v.second.expression->toString() << ": " << e.what();
            throw Base::RuntimeError(ss.str());
        }
    }
    return true;
}

void PropertyExpressionEngine::updateElementReference(DocumentObject *feature, bool reverse, bool notify)
{
    (void)notify;
    if(!feature)
        unregisterElementReference();
    UpdateElementReferenceExpressionVisitor<PropertyExpressionEngine> v(*this,feature,reverse);
    for(auto &e : expressions) {
        if (e.second.expression) {
            e.second.expression->visit(v);
            if (v.changed()) {
                expressionChanged(e.first);
                v.reset();
            }
        }
    }
    if(feature && v.changed()) {
        auto owner = dynamic_cast<App::DocumentObject*>(getContainer());
        if(owner)
            owner->onUpdateElementReference(this);
    }
}

bool PropertyExpressionEngine::referenceChanged() const {
    return false;
}

Property *PropertyExpressionEngine::CopyOnImportExternal(
        const std::map<std::string,std::string> &nameMap) const
{
    std::unique_ptr<PropertyExpressionEngine>  engine;
    for(auto it=expressions.begin();it!=expressions.end();++it) {
#ifdef BOOST_NO_CXX11_SMART_PTR
        std::shared_ptr<Expression> expr(it->second.expression->importSubNames(nameMap).release());
#else
        std::shared_ptr<Expression> expr(it->second.expression->importSubNames(nameMap));
#endif
        if(!expr && !engine)
            continue;
        if(!engine) {
            engine = std::make_unique<PropertyExpressionEngine>();
            for(auto it2=expressions.begin();it2!=it;++it2) {
                engine->expressions[it2->first] = ExpressionInfo(
                        std::shared_ptr<Expression>(it2->second.expression->copy()));
            }
        }else if(!expr)
            expr = it->second.expression;
        engine->expressions[it->first] = ExpressionInfo(expr);
    }
    if(!engine)
        return nullptr;
    engine->validator = validator;
    return engine.release();
}

Property *PropertyExpressionEngine::CopyOnLabelChange(App::DocumentObject *obj,
        const std::string &ref, const char *newLabel) const
{
    std::unique_ptr<PropertyExpressionEngine>  engine;
    for(auto it=expressions.begin();it!=expressions.end();++it) {
#ifdef BOOST_NO_CXX11_SMART_PTR
        std::shared_ptr<Expression> expr(it->second.expression->updateLabelReference(obj,ref,newLabel).release());
#else
        std::shared_ptr<Expression> expr(it->second.expression->updateLabelReference(obj,ref,newLabel));
#endif
        if(!expr && !engine)
            continue;
        if(!engine) {
            engine = std::make_unique<PropertyExpressionEngine>();
            for(auto it2=expressions.begin();it2!=it;++it2) {
                ExpressionInfo info;
                if (it2->second.expression)
                    info.expression = std::shared_ptr<Expression>(it2->second.expression->copy());
                engine->expressions[it2->first] = info;
            }
        }else if(!expr)
            expr = it->second.expression;
        engine->expressions[it->first] = ExpressionInfo(expr);
    }
    if(!engine)
        return nullptr;
    engine->validator = validator;
    return engine.release();
}

Property *PropertyExpressionEngine::CopyOnLinkReplace(const App::DocumentObject *parent,
        App::DocumentObject *oldObj, App::DocumentObject *newObj) const
{
    std::unique_ptr<PropertyExpressionEngine>  engine;
    for(auto it=expressions.begin();it!=expressions.end();++it) {
#ifdef BOOST_NO_CXX11_SMART_PTR
        std::shared_ptr<Expression> expr(
                it->second.expression->replaceObject(parent,oldObj,newObj).release());
#else
        std::shared_ptr<Expression> expr(
                it->second.expression->replaceObject(parent,oldObj,newObj));
#endif
        if(!expr && !engine)
            continue;
        if(!engine) {
            engine = std::make_unique<PropertyExpressionEngine>();
            for(auto it2=expressions.begin();it2!=it;++it2) {
                ExpressionInfo info;
                if (it2->second.expression)
                    info.expression = std::shared_ptr<Expression>(it2->second.expression->copy());
                engine->expressions[it2->first] = info;
            }
        }else if(!expr)
            expr = it->second.expression;
        engine->expressions[it->first] = ExpressionInfo(expr);
    }
    if(!engine)
        return nullptr;
    engine->validator = validator;
    return engine.release();
}

std::map<App::ObjectIdentifier, const App::Expression*>
PropertyExpressionEngine::getExpressions() const
{
    std::map<App::ObjectIdentifier, const Expression*> res;
    for(auto &v : expressions)
        res[v.first] = v.second.expression.get();
    return res;
}

void PropertyExpressionEngine::setExpressions(
        std::map<App::ObjectIdentifier, App::ExpressionPtr> &&exprs)
{
    AtomicPropertyChange signaller(*this);
#ifdef BOOST_NO_CXX11_SMART_PTR
    for(auto &v : exprs)
        setValue(v.first,std::shared_ptr<Expression>(v.second.release()));
#else
    for(auto &v : exprs)
        setValue(v.first,std::move(v.second));
#endif
}

void PropertyExpressionEngine::onRelabeledDocument(const App::Document &doc)
{
    RelabelDocumentExpressionVisitor v(doc);
    for(auto &e : expressions) {
        if (e.second.expression)
            e.second.expression->visit(v);
    }
}
