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

// pointer to the active assembly object
PartDesign::Body                *ActivePartObject =0;
Gui::Document                   *ActiveGuiDoc    =0;
Gui::ViewProviderDocumentObject *ActiveVp        =0;



static PyObject * setActivePart(PyObject *self, PyObject *args)
{
    PyObject *object=0;
    if (PyArg_ParseTuple(args,"|O!",&(PartDesign::BodyPy::Type), &object)&& object) {
        PartDesign::Body* Item = static_cast<PartDesign::BodyPy*>(object)->getBodyPtr();
        // Should be set!
        assert(Item);    

        // get the gui document of the Assembly Item 
        if(ActivePartObject){

            ActiveGuiDoc->signalHighlightObject(*ActiveVp,Gui::Blue,false);
            ActivePartObject = 0;

        }
        ActivePartObject = Item;
        ActiveGuiDoc = Gui::Application::Instance->getDocument(Item->getDocument());
        ActiveVp = dynamic_cast<Gui::ViewProviderDocumentObject*> (ActiveGuiDoc->getViewProvider(Item)) ;
        ActiveGuiDoc->signalHighlightObject(*ActiveVp,Gui::Underlined,true);
       
    }else{
        ActiveGuiDoc->signalHighlightObject(*ActiveVp,Gui::Underlined,false);
        ActivePartObject = 0;
    }

    Py_Return;
}
/* registration table  */
struct PyMethodDef Assembly_methods[] = {
    {"setActivePart"       ,setActivePart      ,METH_VARARGS,
     "setActivePart(BodyObject) -- Set the PartBody object in work."},

    {NULL, NULL}        /* end of table marker */
};
