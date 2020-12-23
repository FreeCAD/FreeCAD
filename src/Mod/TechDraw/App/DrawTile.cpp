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

#include <Mod/TechDraw/App/DrawTilePy.h>  // generated from DrawTilePy.xml
#include "DrawTile.h"

using namespace TechDraw;

//===========================================================================
// DrawTile - attachable tile
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawTile, App::DocumentObject)

DrawTile::DrawTile(void)
{
    static const char *group = "Tile";

    ADD_PROPERTY_TYPE(TileParent,(0),group,(App::PropertyType)(App::Prop_None),
                      "Object to which this tile is attached");
    ADD_PROPERTY_TYPE(TileRow, (0), group, App::Prop_None, "Row in parent");
    ADD_PROPERTY_TYPE(TileColumn, (0), group, App::Prop_None, "Column in parent");
}

DrawTile::~DrawTile()
{
}

void DrawTile::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        //nothing in particular
    }
    DocumentObject::onChanged(prop);

}

short DrawTile::mustExecute() const
{
    return DocumentObject::mustExecute();
}

App::DocumentObjectExecReturn *DrawTile::execute(void)
{ 
//    Base::Console().Message("DT::execute()\n");
    return DocumentObject::execute();
}

DrawView* DrawTile::getParent(void) const
{
//    Base::Console().Message("DT::getParent() - %s\n", getNameInDocument());
    DrawView* result = nullptr;
    App::DocumentObject* baseObj = TileParent.getValue();
    if (baseObj != nullptr) {
        DrawView* cast = dynamic_cast<DrawView*>(baseObj);
        if (cast != nullptr) {
            result = cast;
        }
    }
    return result;
}

PyObject *DrawTile::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawTilePy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawTilePython, TechDraw::DrawTile)
template<> const char* TechDraw::DrawTilePython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderTile";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawTile>;
}

