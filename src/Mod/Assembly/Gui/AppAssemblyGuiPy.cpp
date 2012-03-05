/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Mod/Assembly/App/ItemPy.h>

// pointer to the active assembly object
Assembly::Item                  *ActiveAsmObject =0;
Gui::Document                   *ActiveGuiDoc    =0;
Gui::ViewProviderDocumentObject *ActiveVp        =0;



/* module functions */
static PyObject * setActiveAssembly(PyObject *self, PyObject *args)
{
    PyObject *object=0;
    if (PyArg_ParseTuple(args,"|O!",&(Assembly::ItemPy::Type), &object)&& object) {
        Assembly::Item* Item = static_cast<Assembly::ItemPy*>(object)->getItemPtr();
        // Should be set!
        assert(Item);    

        // get the gui document of the Assembly Item 
        if(ActiveAsmObject){

            ActiveGuiDoc->signalHighlightObject(*ActiveVp,Gui::None);
            ActiveAsmObject = 0;

        }
        ActiveAsmObject = Item;
        ActiveGuiDoc = Gui::Application::Instance->getDocument(Item->getDocument());
        ActiveVp = dynamic_cast<Gui::ViewProviderDocumentObject*> (ActiveGuiDoc->getViewProvider(Item)) ;
        ActiveGuiDoc->signalHighlightObject(*ActiveVp,Gui::Blue);
       
    }else{
        ActiveGuiDoc->signalHighlightObject(*ActiveVp,Gui::None);
        ActiveAsmObject = 0;
    }

    Py_Return;
}


/* registration table  */
struct PyMethodDef AssemblyGui_Import_methods[] = {
    {"setActiveAssembly"       ,setActiveAssembly      ,METH_VARARGS,
     "setActiveAssembly(AssemblyObject) -- Set the Assembly object in work."},

    {NULL, NULL}                   /* end of table marker */
};
