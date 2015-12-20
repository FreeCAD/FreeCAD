/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de)          *
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

#ifndef _PreComp_
#endif

#include <Base/Writer.h>

#include "Document.h"
#include "DocumentObject.h"
#include "DocumentObjectPy.h"
#include "DocumentObjectGroup.h"
#include "PropertyLinks.h"
#include "PropertyExpressionEngine.h"
#include <boost/signals/connection.hpp>
#include <boost/bind.hpp>

using namespace App;


PROPERTY_SOURCE(App::DocumentObject, App::PropertyContainer)

DocumentObjectExecReturn *DocumentObject::StdReturn = 0;

//===========================================================================
// DocumentObject
//===========================================================================

DocumentObject::DocumentObject(void)
    : ExpressionEngine(),_pDoc(0),pcNameInDocument(0)
{
    // define Label of type 'Output' to avoid being marked as touched after relabeling
    ADD_PROPERTY_TYPE(Label,("Unnamed"),"Base",Prop_Output,"User name of the object (UTF8)");
    ADD_PROPERTY_TYPE(ExpressionEngine,(),"Base",Prop_Hidden,"Property expressions");
}

DocumentObject::~DocumentObject(void)
{
    if (!PythonObject.is(Py::_None())){
        // Remark: The API of Py::Object has been changed to set whether the wrapper owns the passed 
        // Python object or not. In the constructor we forced the wrapper to own the object so we need
        // not to dec'ref the Python object any more.
        // But we must still invalidate the Python object because it need not to be
        // destructed right now because the interpreter can own several references to it.
        Base::PyObjectBase* obj = (Base::PyObjectBase*)PythonObject.ptr();
        // Call before decrementing the reference counter, otherwise a heap error can occur
        obj->setInvalid();
    }
}

namespace App {
class ObjectExecution
{
public:
    ObjectExecution(DocumentObject* o) : obj(o)
    { obj->StatusBits.set(3); }
    ~ObjectExecution()
    { obj->StatusBits.reset(3); }
private:
    DocumentObject* obj;
};
}

App::DocumentObjectExecReturn *DocumentObject::recompute(void)
{
    // set/unset the execution bit
    ObjectExecution exe(this);
    return this->execute();
}

DocumentObjectExecReturn *DocumentObject::execute(void)
{
    return StdReturn;
}

short DocumentObject::mustExecute(void) const
{
    return (isTouched() ? 1 : 0);
}

const char* DocumentObject::getStatusString(void) const
{
    if (isError()) {
        const char* text = getDocument()->getErrorDescription(this);
        return text ? text : "Error";
    }
    else if (isTouched())
        return "Touched";
    else
        return "Valid";
}

const char *DocumentObject::getNameInDocument(void) const
{
    // Note: It can happen that we query the internal name of an object even if it is not
    // part of a document (anymore). This is the case e.g. if we have a reference in Python 
    // to an object that has been removed from the document. In this case we should rather
    // return 0.
    //assert(pcNameInDocument);
    if (!pcNameInDocument) return 0;
    return pcNameInDocument->c_str();
}

std::vector<DocumentObject*> DocumentObject::getOutList(void) const
{
    std::vector<Property*> List;
    std::vector<DocumentObject*> ret;
    getPropertyList(List);
    for (std::vector<Property*>::const_iterator It = List.begin();It != List.end(); ++It) {
        if ((*It)->isDerivedFrom(PropertyLinkList::getClassTypeId())) {
            const std::vector<DocumentObject*> &OutList = static_cast<PropertyLinkList*>(*It)->getValues();
            for (std::vector<DocumentObject*>::const_iterator It2 = OutList.begin();It2 != OutList.end(); ++It2) {
                if (*It2)
                    ret.push_back(*It2);
            }
        }
        else if ((*It)->isDerivedFrom(PropertyLinkSubList::getClassTypeId())) {
            const std::vector<DocumentObject*> &OutList = static_cast<PropertyLinkSubList*>(*It)->getValues();
            for (std::vector<DocumentObject*>::const_iterator It2 = OutList.begin();It2 != OutList.end(); ++It2) {
                if (*It2)
                    ret.push_back(*It2);
            }
        }
        else if ((*It)->isDerivedFrom(PropertyLink::getClassTypeId())) {
            if (static_cast<PropertyLink*>(*It)->getValue())
                ret.push_back(static_cast<PropertyLink*>(*It)->getValue());
        }
        else if ((*It)->isDerivedFrom(PropertyLinkSub::getClassTypeId())) {
            if (static_cast<PropertyLinkSub*>(*It)->getValue())
                ret.push_back(static_cast<PropertyLinkSub*>(*It)->getValue());
        }
    }

    // Get document objects that this document object relies on
    ExpressionEngine.getDocumentObjectDeps(ret);

    return ret;
}

std::vector<App::DocumentObject*> DocumentObject::getInList(void) const
{
    if (_pDoc)
        return _pDoc->getInList(this);
    else
        return std::vector<App::DocumentObject*>();
}

DocumentObjectGroup* DocumentObject::getGroup() const
{
    return DocumentObjectGroup::getGroupOfObject(this);
}

void DocumentObject::onLostLinkToObject(DocumentObject*)
{

}

App::Document *DocumentObject::getDocument(void) const
{
    return _pDoc;
}

void DocumentObject::setDocument(App::Document* doc)
{
    _pDoc=doc;
    onSettingDocument();
}

void DocumentObject::onBeforeChange(const Property* prop)
{

    // Store current name in oldLabel, to be able to easily retrieve old name of document object later
    // when renaming expressions.
    if (prop == &Label)
        oldLabel = Label.getStrValue();

    if (_pDoc)
        _pDoc->onBeforeChangeProperty(this,prop);
}

/// get called by the container when a Property was changed
void DocumentObject::onChanged(const Property* prop)
{
    if (_pDoc)
        _pDoc->onChangedProperty(this,prop);

    if (prop == &Label && _pDoc && oldLabel != Label.getStrValue())
        _pDoc->signalRelabelObject(*this);

    if (prop->getType() & Prop_Output)
        return;
    // set object touched
    StatusBits.set(0);
}

PyObject *DocumentObject::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

std::vector<PyObject *> DocumentObject::getPySubObjects(const std::vector<std::string>&) const
{
    // default implementation returns nothing
    return std::vector<PyObject *>();
}

void DocumentObject::touch(void)
{
    StatusBits.set(0);
}

/**
 * @brief Check whether the document object is touched or not.
 * @return true if document object is touched, false if not.
 */

bool DocumentObject::isTouched() const
{
    return ExpressionEngine.isTouched() || StatusBits.test(0);
}

void DocumentObject::Save (Base::Writer &writer) const
{
    writer.ObjectName = this->getNameInDocument();
    App::PropertyContainer::Save(writer);
}

/**
 * @brief Associate the expression \expr with the object identifier \a path in this document object.
 * @param path Target object identifier for the result of the expression
 * @param expr Expression tree
 * @param comment Optional comment describing the expression
 */

void DocumentObject::setExpression(const ObjectIdentifier &path, boost::shared_ptr<Expression> expr, const char * comment)
{
    ExpressionEngine.setValue(path, expr, comment);
    connectRelabelSignals();
}

/**
 * @brief Get expression information associated with \a path.
 * @param path Object identifier
 * @return Expression info, containing expression and optional comment.
 */

const PropertyExpressionEngine::ExpressionInfo DocumentObject::getExpression(const ObjectIdentifier &path) const
{
    boost::any value = ExpressionEngine.getPathValue(path);

    if (value.type() == typeid(PropertyExpressionEngine::ExpressionInfo))
        return boost::any_cast<PropertyExpressionEngine::ExpressionInfo>(value);
    else
        return PropertyExpressionEngine::ExpressionInfo();
}

/**
 * @brief Invoke ExpressionEngine's renameObjectIdentifier, to possibly rewrite expressions using
 * the \a paths map with current and new identifiers.
 *
 * @param paths
 */

void DocumentObject::renameObjectIdentifiers(const std::map<ObjectIdentifier, ObjectIdentifier> &paths)
{
    ExpressionEngine.renameObjectIdentifiers(paths);
}

/**
 * @brief Helper function that sets up a signal to track document object renames.
 */

void DocumentObject::connectRelabelSignals()
{
    // Only keep signal if the ExpressionEngine has at least one expression
    if (ExpressionEngine.numExpressions() > 0) {

        // Not already connected?
        if (!onRelabledObjectConnection.connected())
            onRelabledObjectConnection = getDocument()->signalRelabelObject.connect(boost::bind(&PropertyExpressionEngine::slotObjectRenamed, &ExpressionEngine, _1));

        // Connect to signalDeletedObject, to properly track deletion of other objects that might be referenced in an expression
        if (!onDeletedObjectConnection.connected())
            onDeletedObjectConnection = getDocument()->signalDeletedObject.connect(boost::bind(&PropertyExpressionEngine::slotObjectDeleted, &ExpressionEngine, _1));

        try {
            // Crude method to resolve all expression dependencies
            ExpressionEngine.execute();
        }
        catch (...) {
            // Ignore any error
        }
    }
    else {
        // Disconnect signals; nothing to track now
        onRelabledObjectConnection.disconnect();
        onRelabledDocumentConnection.disconnect();
        onDeletedObjectConnection.disconnect();
    }
}
