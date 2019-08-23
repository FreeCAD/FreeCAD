/***************************************************************************
 *   Copyright (c) 2019 Wanderer Fan <wandererfan@gmail.com>               *
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

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>

#include "DrawUtil.h"

#include <Mod/TechDraw/App/DrawTileWeldPy.h>  // generated from DrawTileWeldPy.xml
#include "DrawTileWeld.h"

using namespace TechDraw;

//===========================================================================
// DrawTileWeld - attachable tile
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawTileWeld, TechDraw::DrawTile)

DrawTileWeld::DrawTileWeld(void)
{
    static const char *group = "TileWeld";

    ADD_PROPERTY_TYPE(LeftText,(""),group,(App::PropertyType)(App::Prop_None),
                      "Text LHS");
    ADD_PROPERTY_TYPE(RightText, (0), group, App::Prop_None, "Text RHS");
    ADD_PROPERTY_TYPE(CenterText, (0), group, App::Prop_None, "Text above Symbol");
    ADD_PROPERTY_TYPE(SymbolFile, (""), group, App::Prop_None, "Svg Symbol File");
}

DrawTileWeld::~DrawTileWeld()
{
}

void DrawTileWeld::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        //nothing in particular
    }
    DrawTile::onChanged(prop);

}

short DrawTileWeld::mustExecute() const
{
    return DrawTile::mustExecute();
}

App::DocumentObjectExecReturn *DrawTileWeld::execute(void)
{ 
//    Base::Console().Message("DTW::execute()\n");
    return DrawTile::execute();
}

PyObject *DrawTileWeld::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawTileWeldPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawTileWeldPython, TechDraw::DrawTileWeld)
template<> const char* TechDraw::DrawTileWeldPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderTile";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawTileWeld>;
}

