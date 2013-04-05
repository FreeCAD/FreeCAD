/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <Python.h>
#endif

#include <Base/PyObjectBase.h>
#include <Base/Console.h>

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Tree.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Mod/PartDesign/App/BodyPy.h>

namespace PartDesignGui {

// pointer to the active assembly object
PartDesign::Body                *ActivePartObject =0;
Gui::Document                   *ActiveGuiDoc     =0;
App::Document                   *ActiveAppDoc     =0;
Gui::ViewProviderDocumentObject *ActiveVp         =0;

}

static PyObject * setActivePart(PyObject *self, PyObject *args)
{
    if(PartDesignGui::ActivePartObject){
        // check if the document not already closed
        std::vector<App::Document*> docs = App::GetApplication().getDocuments();
        for(std::vector<App::Document*>::const_iterator it=docs.begin();it!=docs.end();++it)
            if(*it == PartDesignGui::ActiveAppDoc){
                PartDesignGui::ActiveGuiDoc->signalHighlightObject(*PartDesignGui::ActiveVp,Gui::Underlined,false);
                break;
            }
                
        PartDesignGui::ActivePartObject->IsActive.setValue(false);
        PartDesignGui::ActivePartObject = 0;
        PartDesignGui::ActiveGuiDoc    =0;
        PartDesignGui::ActiveAppDoc    =0;
        PartDesignGui::ActiveVp        =0;
    }

    PyObject *object=0;
    if (PyArg_ParseTuple(args,"|O!",&(PartDesign::BodyPy::Type), &object)&& object) {
        PartDesign::Body* Item = static_cast<PartDesign::BodyPy*>(object)->getBodyPtr();
        // Should be set!
        assert(Item);    

        Item->IsActive.setValue(true);
        PartDesignGui::ActivePartObject = Item;
        PartDesignGui::ActiveAppDoc = Item->getDocument();
        PartDesignGui::ActiveGuiDoc = Gui::Application::Instance->getDocument(PartDesignGui::ActiveAppDoc);
        PartDesignGui::ActiveVp = dynamic_cast<Gui::ViewProviderDocumentObject*> (PartDesignGui::ActiveGuiDoc->getViewProvider(Item)) ;
        PartDesignGui::ActiveGuiDoc->signalHighlightObject(*PartDesignGui::ActiveVp,Gui::Underlined,true);
       
    }

    Py_Return;
}
/* registration table  */
struct PyMethodDef Assembly_methods[] = {
    {"setActivePart"       ,setActivePart      ,METH_VARARGS,
     "setActivePart(BodyObject) -- Set the PartBody object in work."},

    {NULL, NULL}        /* end of table marker */
};
