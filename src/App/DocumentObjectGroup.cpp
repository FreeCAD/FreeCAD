/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "DocumentObjectGroup.h"
#include "DocumentObjectGroupPy.h"
#include "Document.h"
#include "FeaturePythonPyImp.h"

using namespace App;

PROPERTY_SOURCE(App::DocumentObjectGroup, App::DocumentObject)


DocumentObjectGroup::DocumentObjectGroup() 
{
    ADD_PROPERTY_TYPE(Group,(0),"Base",(App::PropertyType)(Prop_Output),"List of referenced objects");
}

DocumentObjectGroup::~DocumentObjectGroup()
{
}

DocumentObject* DocumentObjectGroup::addObject(const char* sType, const char* pObjectName)
{
    DocumentObject* obj = getDocument()->addObject(sType, pObjectName);
    if (obj) addObject(obj);
    return obj;
}

void DocumentObjectGroup::addObject(DocumentObject* obj)
{
    if (!hasObject(obj)) {
        std::vector<DocumentObject*> grp = Group.getValues();
        grp.push_back(obj);
        Group.setValues(grp);
    }
}

void DocumentObjectGroup::removeObject(DocumentObject* obj)
{
    std::vector<DocumentObject*> grp = Group.getValues();
    for (std::vector<DocumentObject*>::iterator it = grp.begin(); it != grp.end(); ++it) {
        if (*it == obj) {
            grp.erase(it);
            Group.setValues(grp);
            break;
        }
    }
}

void DocumentObjectGroup::removeObjectsFromDocument()
{
    std::vector<DocumentObject*> grp = Group.getValues();
    for (std::vector<DocumentObject*>::iterator it = grp.begin(); it != grp.end(); ++it) {
        removeObjectFromDocument(*it);
    }
}

void DocumentObjectGroup::removeObjectFromDocument(DocumentObject* obj)
{
    // remove all children
    if (obj->getTypeId().isDerivedFrom(DocumentObjectGroup::getClassTypeId())) {
        std::vector<DocumentObject*> grp = static_cast<DocumentObjectGroup*>(obj)->Group.getValues();
        for (std::vector<DocumentObject*>::iterator it = grp.begin(); it != grp.end(); ++it) {
            // recursive call to remove all subgroups
            removeObjectFromDocument(*it);
        }
    }

    this->getDocument()->remObject(obj->getNameInDocument());
}

DocumentObject *DocumentObjectGroup::getObject(const char *Name) const
{
    DocumentObject* obj = getDocument()->getObject(Name);
    if (obj && hasObject(obj))
        return obj;
    return 0;
}

bool DocumentObjectGroup::hasObject(DocumentObject* obj) const
{
    const std::vector<DocumentObject*>& grp = Group.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if (*it == obj)
            return true;
    }

    return false;
}

bool DocumentObjectGroup::isChildOf(DocumentObjectGroup* group) const
{
    const std::vector<DocumentObject*>& grp = group->Group.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if (*it == this)
            return true;
        if ((*it)->getTypeId().isDerivedFrom(DocumentObjectGroup::getClassTypeId())) {
            if (this->isChildOf(static_cast<DocumentObjectGroup*>(*it)))
                return true;
        }
    }

    return false;
}

std::vector<DocumentObject*> DocumentObjectGroup::getObjects() const
{
    return Group.getValues();
}

std::vector<DocumentObject*> DocumentObjectGroup::getObjectsOfType(const Base::Type& typeId) const
{
    std::vector<DocumentObject*> type;
    const std::vector<DocumentObject*>& grp = Group.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if ( (*it)->getTypeId().isDerivedFrom(typeId))
            type.push_back(*it);
    }

    return type;
}

int DocumentObjectGroup::countObjectsOfType(const Base::Type& typeId) const
{
    int type=0;
    const std::vector<DocumentObject*>& grp = Group.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if ( (*it)->getTypeId().isDerivedFrom(typeId))
            type++;
    }

    return type;
}

DocumentObjectGroup* DocumentObjectGroup::getGroupOfObject(DocumentObject* obj)
{
    const Document* doc = obj->getDocument();
    std::vector<DocumentObject*> grps = doc->getObjectsOfType(DocumentObjectGroup::getClassTypeId());
    for (std::vector<DocumentObject*>::iterator it = grps.begin(); it != grps.end(); ++it) {
        DocumentObjectGroup* grp = (DocumentObjectGroup*)(*it);
        if (grp->hasObject(obj))
            return grp;
    }

    return 0;
}

PyObject *DocumentObjectGroup::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectGroupPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

// Python feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(App::DocumentObjectGroupPython, App::DocumentObjectGroup)
template<> const char* App::DocumentObjectGroupPython::getViewProviderName(void) const {
    return "Gui::ViewProviderDocumentObjectGroupPython";
}
template<> PyObject* App::DocumentObjectGroupPython::getPyObject(void) {
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<App::DocumentObjectGroupPy>(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class AppExport FeaturePythonT<App::DocumentObjectGroup>;
}
