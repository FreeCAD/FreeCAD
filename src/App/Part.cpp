/***************************************************************************
 *   Copyright (c) Juergen Riegel          (juergen.riegel@web.de) 2014    *
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

#include <App/Document.h>
#include "Part.h"
#include "PartPy.h"
//#define new DEBUG_CLIENTBLOCK
using namespace App;


PROPERTY_SOURCE(App::Part, App::GeoFeature)


//===========================================================================
// Feature
//===========================================================================

Part::Part(void)
{
    ADD_PROPERTY(Member,(0));
}

Part::~Part(void)
{
}

DocumentObject* Part::addObject(const char* sType, const char* pObjectName)
{
    DocumentObject* obj = getDocument()->addObject(sType, pObjectName);
    if (obj) addObject(obj);
    return obj;
}

void Part::addObject(DocumentObject* obj)
{
    if (!hasObject(obj)) {
        std::vector<DocumentObject*> grp = Member.getValues();
        grp.push_back(obj);
        Member.setValues(grp);
    }
}

void Part::removeObject(DocumentObject* obj)
{
    std::vector<DocumentObject*> grp = Member.getValues();
    for (std::vector<DocumentObject*>::iterator it = grp.begin(); it != grp.end(); ++it) {
        if (*it == obj) {
            grp.erase(it);
            Member.setValues(grp);
            break;
        }
    }
}

void Part::removeObjectsFromDocument()
{
    std::vector<DocumentObject*> grp = Member.getValues();
    for (std::vector<DocumentObject*>::iterator it = grp.begin(); it != grp.end(); ++it) {
        removeObjectFromDocument(*it);
    }
}

void Part::removeObjectFromDocument(DocumentObject* obj)
{
    // remove all children
    if (obj->getTypeId().isDerivedFrom(Part::getClassTypeId())) {
        std::vector<DocumentObject*> grp = static_cast<Part*>(obj)->Member.getValues();
        for (std::vector<DocumentObject*>::iterator it = grp.begin(); it != grp.end(); ++it) {
            // recursive call to remove all subgroups
            removeObjectFromDocument(*it);
        }
    }

    this->getDocument()->remObject(obj->getNameInDocument());
}

DocumentObject *Part::getObject(const char *Name) const
{
    DocumentObject* obj = getDocument()->getObject(Name);
    if (obj && hasObject(obj))
        return obj;
    return 0;
}

bool Part::hasObject(const DocumentObject* obj) const
{
    const std::vector<DocumentObject*>& grp = Member.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if (*it == obj)
            return true;
    }

    return false;
}

bool Part::isChildOf(const Part* group) const
{
    const std::vector<DocumentObject*>& grp = group->Member.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if (*it == this)
            return true;
        if ((*it)->getTypeId().isDerivedFrom(Part::getClassTypeId())) {
            if (this->isChildOf(static_cast<Part*>(*it)))
                return true;
        }
    }

    return false;
}

std::vector<DocumentObject*> Part::getObjects() const
{
    return Member.getValues();
}

std::vector<DocumentObject*> Part::getObjectsOfType(const Base::Type& typeId) const
{
    std::vector<DocumentObject*> type;
    const std::vector<DocumentObject*>& grp = Member.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if ( (*it)->getTypeId().isDerivedFrom(typeId))
            type.push_back(*it);
    }

    return type;
}

int Part::countObjectsOfType(const Base::Type& typeId) const
{
    int type=0;
    const std::vector<DocumentObject*>& grp = Member.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if ( (*it)->getTypeId().isDerivedFrom(typeId))
            type++;
    }

    return type;
}


PyObject *Part::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new PartPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

// Python feature ---------------------------------------------------------

// Not quit sure yet makeing Part derivable in Python is good Idea!
// JR 2014

//namespace App {
///// @cond DOXERR
//PROPERTY_SOURCE_TEMPLATE(App::PartPython, App::Part)
//template<> const char* App::PartPython::getViewProviderName(void) const {
//    return "Gui::ViewProviderPartPython";
//}
//template<> PyObject* App::PartPython::getPyObject(void) {
//    if (PythonObject.is(Py::_None())) {
//        // ref counter is set to 1
//        PythonObject = Py::Object(new FeaturePythonPyT<App::PartPy>(this),true);
//    }
//    return Py::new_reference_to(PythonObject);
//}
///// @endcond
//
//// explicit template instantiation
//template class AppExport FeaturePythonT<App::Part>;
//}
