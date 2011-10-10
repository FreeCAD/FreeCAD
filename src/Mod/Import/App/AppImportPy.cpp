/***************************************************************************
 *   (c) Jürgen Riegel (juergen.riegel@web.de) 2002                        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 *   Juergen Riegel 2002                                                   *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
#	include <stdio.h>
# if defined (_POSIX_C_SOURCE)
#   undef  _POSIX_C_SOURCE
# endif // (re-)defined in pyconfig.h
#	include <Python.h>
# include <BRep_Builder.hxx>
# include <BRepTools.hxx>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Feature.h>
#include <App/Property.h>
#include <Mod/Part/App/TopologyPy.h>



/* module functions */
static PyObject *
open(PyObject *self, PyObject *args)
{
  const char* Name;
  if (! PyArg_ParseTuple(args, "s",&Name))
    return NULL;

  Base::Console().Log("Open in Import with %s",Name);

  // extract ending
  std::string cEnding(Name);
  unsigned int pos = cEnding.find_last_of('.');
  if(pos == cEnding.size())
    Py_Error(PyExc_Exception,"no file ending");
  cEnding.erase(0,pos+1);

  if(cEnding == "stp" || cEnding == "step")
  {
    // create new document and add Import feature
    App::Document *pcDoc = App::GetApplication().newDocument();
    App::AbstractFeature *pcFeature = pcDoc->addFeature("ImportStep","Step Import");
    pcFeature->setPropertyString (Name,"FileName");
    pcFeature->TouchProperty("FileName");
    pcDoc->recompute();

  }else if(cEnding == "igs" || cEnding == "iges")
  {
    // create new document and add Import feature
    App::Document *pcDoc = App::GetApplication().newDocument();
    App::AbstractFeature *pcFeature = pcDoc->addFeature("ImportIges","Iges Import");
assert(0);
//    pcFeature->GetProperty("FileName").Set(Name);
    pcFeature->TouchProperty("FileName");
    pcDoc->recompute();

  }else

    Py_Error(PyExc_Exception,"unknown file ending");


	Py_Return;
}

/* module functions */
static PyObject *
save(PyObject *self, PyObject *args)
{
	char* str;

    if (! PyArg_ParseTuple(args, "s",&str))
        return NULL;

	TopoDS_Shape ResultShape;
	BRep_Builder aBuilder;

	BRepTools::Read(ResultShape,(const Standard_CString)str,aBuilder);

  return new Part::TopoShapePy(ResultShape);		  /* convert C -> Python */

}



/* registration table  */
struct PyMethodDef Import_methods[] = {
    {"open", open, 1},				/* method name, C func ptr, always-tuple */
    {"save", save, 1},

    {NULL, NULL}                   /* end of table marker */
};
