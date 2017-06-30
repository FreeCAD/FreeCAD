/***************************************************************************
 *   Copyright (c) YEAR YOUR NAME         <Your e-mail address>            *
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


#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/PyObjectBase.h>

///BEGIN CAD-GDML
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/dom/DOM.hpp>

#include "./GDML/persistency/G4GDMLParser.hh"
#include "./GDML/G4FreeCAD.h"

//std::string resultString="";
//using namespace std;
///END CAD-GDML

/* module functions */
///BEGIN CAD-GDML
static PyObject * open(PyObject *self, PyObject *args)
{
    const char* Name;
    if (!PyArg_ParseTuple(args, "s",&Name))
        return NULL;                         

	PY_TRY {
//	Base::Console().Message("parser.Read %s \n",Name);
	Base::FileInfo file(Name);
	G4GDMLParser parser;
	parser.Read(Name,true);
	Base::Console().Message("GDML2FreeCAD\n");
	G4FreeCAD Conversion;
	//Begin Get filename without extension
/**/	char *filename = (char *)Name;
	char *moduleId = std::max( strrchr( filename, '/'), strrchr( filename,'\\'));
	moduleId = moduleId ? moduleId : filename;
	std::string resultString=moduleId;
	size_t pos = resultString.rfind('.');
	resultString=resultString.substr(0, pos);
/**/ 
	//End Get filename without extension
//	Conversion.GDML2FreeCAD(&parser,moduleId);
	Conversion.GDML2FreeCAD(&parser,resultString);
   } PY_CATCH;

    Py_Return;
}

static PyObject * exporter(PyObject *self, PyObject *args)
{
    PyObject* object;
    const char* filename;
    if (!PyArg_ParseTuple(args, "Os",&object,&filename))
        return NULL;

    PY_TRY {
		Base::Console().Message("FreeCAD2GDML");
		Base::FileInfo file(filename);
		G4FreeCAD Conversion;
		G4GDMLParser parser;
		//Base::Console().Message("FileName %s \n",resultString.c_str());
		//Conversion.FreeCAD2GDML(&parser,filename,resultString);
		Conversion.FreeCAD2GDML(&parser,filename);
    }
    PY_CATCH

    Py_Return;
}

/* registration table  */
struct PyMethodDef GDMLGui_methods[] = {
	///BEGIN GDML
    {"open"     ,open  ,METH_VARARGS,
     "open(string) -- Create a new document and load the GDML file into the document."},
	//END GDML
    {"export"     ,exporter  ,METH_VARARGS,
     "export(list,string) -- Export a list of objects into a single file."},
    {NULL, NULL}                   /* end of table marker */
};
