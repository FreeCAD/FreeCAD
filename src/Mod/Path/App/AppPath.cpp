/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Base/Interpreter.h>

#include "Command.h"
#include "CommandPy.h"
#include "Path.h"
#include "PathPy.h"
#include "Tool.h"
#include "Tooltable.h"
#include "ToolPy.h"
#include "TooltablePy.h"
#include "PropertyPath.h"
#include "FeaturePath.h"
#include "PropertyTool.h"
#include "PropertyTooltable.h"
#include "FeaturePathCompound.h"
#include "FeaturePathShape.h"
#include "AreaPy.h"
#include "FeatureArea.h"
#include "Voronoi.h"
#include "VoronoiCell.h"
#include "VoronoiCellPy.h"
#include "VoronoiEdge.h"
#include "VoronoiEdgePy.h"
#include "VoronoiPy.h"
#include "VoronoiVertex.h"
#include "VoronoiVertexPy.h"


namespace Path {
extern PyObject* initModule();
}

/* Python entry */
PyMOD_INIT_FUNC(Path)
{
    // load dependent module
    try {
        Base::Interpreter().runString("import Part");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* pathModule = Path::initModule();
    Base::Console().Log("Loading Path module... done\n");

    // Add Types to module
    Base::Interpreter().addType(&Path::CommandPy        ::Type, pathModule, "Command");
    Base::Interpreter().addType(&Path::PathPy           ::Type, pathModule, "Path");
    Base::Interpreter().addType(&Path::ToolPy           ::Type, pathModule, "Tool");
    Base::Interpreter().addType(&Path::TooltablePy      ::Type, pathModule, "Tooltable");
    Base::Interpreter().addType(&Path::AreaPy           ::Type, pathModule, "Area");
    Base::Interpreter().addType(&Path::VoronoiPy        ::Type, pathModule, "Voronoi");
    Base::Interpreter().addType(&Path::VoronoiCellPy    ::Type, pathModule, "VoronoiCell");
    Base::Interpreter().addType(&Path::VoronoiEdgePy    ::Type, pathModule, "VoronoiEdge");
    Base::Interpreter().addType(&Path::VoronoiVertexPy  ::Type, pathModule, "VoronoiVertex");

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
    Path::PropertyTool           ::init();
    Path::PropertyTooltable      ::init();
    Path::FeatureCompound        ::init();
    Path::FeatureCompoundPython  ::init();
    Path::FeatureShape           ::init();
    Path::FeatureShapePython     ::init();
    Path::Area                   ::init();
    Path::FeatureArea            ::init();
    Path::FeatureAreaPython      ::init();
    Path::FeatureAreaView        ::init();
    Path::FeatureAreaViewPython  ::init();
    Path::Voronoi                ::init();
    Path::VoronoiCell            ::init();
    Path::VoronoiEdge            ::init();
    Path::VoronoiVertex          ::init();

    PyMOD_Return(pathModule);
}
