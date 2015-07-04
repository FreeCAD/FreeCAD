/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2014              *
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

#include <Base/Console.h>
#include <Base/Interpreter.h>

#include "Command.h"
#include "CommandPy.h"
#include "Path.h"
#include "PathPy.h"
#include "Tooltable.h"
#include "ToolPy.h"
#include "TooltablePy.h"
#include "PropertyPath.h"
#include "FeaturePath.h"
#include "PropertyTooltable.h"
#include "FeaturePathCompound.h"
#include "FeaturePathShape.h"

extern struct PyMethodDef Path_methods[];

PyDoc_STRVAR(module_Path_doc,
"This module is the Path module.");


/* Python entry */
extern "C" {
void PathExport initPath()
{
    PyObject* pathModule = Py_InitModule3("Path", Path_methods, module_Path_doc);   /* mod name, table ptr */
    Base::Console().Log("Loading Path module... done\n");


    // Add Types to module
    Base::Interpreter().addType(&Path::CommandPy            ::Type,pathModule,"Command");
    Base::Interpreter().addType(&Path::PathPy               ::Type,pathModule,"Path");
    Base::Interpreter().addType(&Path::ToolPy               ::Type,pathModule,"Tool");
    Base::Interpreter().addType(&Path::TooltablePy          ::Type,pathModule,"Tooltable");

    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.
    Path::Command                ::init();
    Path::Toolpath               ::init();
    Path::Tool                   ::init();
    Path::Tooltable              ::init();
    Path::PropertyPath           ::init();
    Path::Feature                ::init();
    Path::FeaturePython          ::init();
    Path::PropertyTooltable      ::init();
    Path::FeatureCompound        ::init();
    Path::FeatureCompoundPython  ::init();
    Path::FeatureShape           ::init();
    Path::FeatureShapePython     ::init();
}

} // extern "C"
