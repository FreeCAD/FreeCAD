/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/Document.h>
#include "PartFeature.h"

// inclusion of the generated files (generated out of PartFeaturePy.xml)
#include "PartFeaturePy.h"
#include "PartFeaturePy.cpp"

using namespace Part;

// returns a string which represent the object e.g. when printed in python
std::string PartFeaturePy::representation(void) const
{
    return std::string("<Part::PartFeature>");
}

PyObject *PartFeaturePy::getElementHistory(PyObject *args) {
    const char *name;
    PyObject *recursive = Py_True;
    if (!PyArg_ParseTuple(args, "s|O", &name,&recursive))
        return 0;

    PY_TRY {
        auto feature = getFeaturePtr();
        std::string mapped = feature->Shape.getShape().getElementName(name,true);
        Py::List list;
        std::string original;
        bool recursve = PyObject_IsTrue(recursive);
        do {
            std::vector<std::string> history;
            long tag = feature->Shape.getShape().getElementHistory(mapped.c_str(),&original,&history);
            App::DocumentObject *obj;
            obj = tag?feature->getDocument()->getObjectByID(tag):0;
            if(!recursve) {
                Py::Tuple ret(3);
                if(obj && obj->getNameInDocument()) 
                    ret.setItem(0,Py::String(obj->getNameInDocument()));
                else 
                    ret.setItem(0,Py::Int(tag));
                ret.setItem(1,Py::String(original));
                ret.setItem(2,Py::List());
                return Py::new_reference_to(ret);
            }
            Py::List pyHistory;
            for(auto &h : history)
                pyHistory.append(Py::String(h));
            list.append(Py::TupleN(Py::String(feature->getNameInDocument()),
                        Py::String(mapped),pyHistory));
            if(!obj)
                break;
            feature = dynamic_cast<Feature*>(obj->getLinkedObject(true));
            mapped = original;
        }while(feature);

        if(!list.size())
            Py_Return;
        return Py::new_reference_to(list);
    }PY_CATCH
}

PyObject *PartFeaturePy::getCustomAttributes(const char* ) const
{
    return 0;
}

int PartFeaturePy::setCustomAttributes(const char* , PyObject *)
{
    return 0; 
}
