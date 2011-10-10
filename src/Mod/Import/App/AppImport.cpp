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
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>

#include <App/Application.h>
#include <Mod/Part/App/TopologyPy.h>

#include "FeatureImportStep.h"
#include "FeatureImportIges.h"


/* registration table  */
extern struct PyMethodDef Import_methods[];


// python entry
#ifdef FC_OS_WIN32
#	define ModuleExport __declspec(dllexport)
#else
#	define ModuleExport
#endif
extern "C" {
void ModuleExport initImport() {

	(void) Py_InitModule("Import", Import_methods);   /* mod name, table ptr */

  // load dependend module
  Base::Interpreter().loadModule("Part");

	App::AbstractFeatureFactory().AddProducer("ImportStep",new App::AbstractFeatureProducer<Import::FeatureImportStep>);
	App::AbstractFeatureFactory().AddProducer("ImportIges",new App::AbstractFeatureProducer<Import::FeatureImportIges>);

	Base::Console().Log("Import loaded\n");

	return;
}


} // extern "C" {
