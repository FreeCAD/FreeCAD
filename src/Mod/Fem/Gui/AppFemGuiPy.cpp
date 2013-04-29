/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <werner.wm.mayer@gmx.de>              *
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

#include <App/DocumentObjectPy.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Tree.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Mod/Fem/App/FemAnalysis.h>


// pointer to the active Analysis object
Fem::FemAnalysis                 *ActiveAnalysis  =0;
Gui::Document                    *ActiveGuiDoc    =0;
App::Document                    *ActiveAppDoc    =0;
Gui::ViewProviderDocumentObject  *ActiveVp        =0;


/* module functions */
static PyObject * setActiveAnalysis(PyObject *self, PyObject *args)
{   
    if(ActiveAnalysis){
        // check if the document not already closed
        std::vector<App::Document*> docs = App::GetApplication().getDocuments();
        for(std::vector<App::Document*>::const_iterator it=docs.begin();it!=docs.end();++it)
            if(*it == ActiveAppDoc){
                ActiveGuiDoc->signalHighlightObject(*ActiveVp,Gui::Blue,false);
                break;
            }
                
        ActiveAnalysis = 0;
        ActiveGuiDoc    =0;
        ActiveAppDoc    =0;
        ActiveVp        =0;
    }

    PyObject *object=0;
    if (PyArg_ParseTuple(args,"|O!",&(App::DocumentObjectPy::Type), &object)&& object) {
        App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
        if(!obj || !obj->getTypeId().isDerivedFrom(Fem::FemAnalysis::getClassTypeId()) ){
            PyErr_SetString(PyExc_Exception, "Active Analysis object have to be of type Fem::FemAnalysis!");
            return 0;
        }

        // get the gui document of the Analysis Item 
        ActiveAnalysis = static_cast<Fem::FemAnalysis*>(obj);
        ActiveAppDoc = ActiveAnalysis->getDocument();
        ActiveGuiDoc = Gui::Application::Instance->getDocument(ActiveAppDoc);
        ActiveVp = dynamic_cast<Gui::ViewProviderDocumentObject*> (ActiveGuiDoc->getViewProvider(ActiveAnalysis)) ;
        ActiveGuiDoc->signalHighlightObject(*ActiveVp,Gui::Blue,true);
    }

    Py_Return;
}

/* module functions */
static PyObject * getActiveAnalysis(PyObject *self, PyObject *args)
{   
    if(ActiveAnalysis){
 
        return ActiveAnalysis->getPyObject();
    }


    Py_Return;
}



/* registration table  */
struct PyMethodDef FemGui_Import_methods[] = {
    {"setActiveAnalysis"       ,setActiveAnalysis      ,METH_VARARGS,
     "setActiveAnalysis(AnalysisObject) -- Set the Analysis object in work."},
    {"getActiveAnalysis"       ,getActiveAnalysis      ,METH_VARARGS,
     "getActiveAnalysis() -- Returns the Analysis object in work."},
    {NULL, NULL}                   /* end of table marker */
};
