/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>

#include "DrawWeldSymbol.h"
#include "DrawWeldSymbolPy.h"  // generated from DrawWeldSymbolPy.xml
#include "DrawLeaderLine.h"
#include "DrawTileWeld.h"
#include "DrawUtil.h"

using namespace TechDraw;

//===========================================================================
// DrawWeldSymbol - welding symbol
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawWeldSymbol, TechDraw::DrawView)

DrawWeldSymbol::DrawWeldSymbol()
{
    static const char *group = "Weld Symbol";

    ADD_PROPERTY_TYPE(Leader, (nullptr), group, (App::PropertyType)(App::Prop_None), "Parent Leader");
    ADD_PROPERTY_TYPE(AllAround, (false), group, App::Prop_None, "All Around Symbol on/off");
    ADD_PROPERTY_TYPE(FieldWeld, (false), group, App::Prop_None, "Field Weld Symbol on/off");
    ADD_PROPERTY_TYPE(AlternatingWeld, (false), group, App::Prop_None, "Alternating Weld true/false");
    ADD_PROPERTY_TYPE(TailText, (""), group, App::Prop_None, "Text at tail of symbol");

    Caption.setStatus(App::Property::Hidden, true);
    Scale.setStatus(App::Property::Hidden, true);
    ScaleType.setStatus(App::Property::Hidden, true);
    Rotation.setStatus(App::Property::Hidden, true);
}

//DWS always has exactly 2 child tiles - ArrowSide and OtherSide.
//OtherSide tile may be hidden;
//once DWS has been added to the document, add 2x DrawTileWeld
//but if this is a restore of an existing DWS, the tiles will loaded elsewhere
void DrawWeldSymbol::onSettingDocument()
{
//    Base::Console().Message("DWS::onSettingDocument() - doc: %s\n", getDocument()->getName());
    App::Document* doc = getDocument();

    if (doc->testStatus(App::Document::Status::Restoring)) {
//        Base::Console().Message("DWS::onSettingDocument() - restoring!\n");
        return;
    }

    std::vector<DrawTileWeld*> existingTiles = getTiles();
    if (!existingTiles.empty()) {
        return;
    }

    std::string tileName1 = doc->getUniqueObjectName("TileWeld");
    auto tile1Obj( doc->addObject( "TechDraw::DrawTileWeld", tileName1.c_str() ) );
    DrawTileWeld* tile1 = dynamic_cast<DrawTileWeld*>(tile1Obj);
    if (tile1) {
        tile1->Label.setValue(DrawUtil::translateArbitrary("DrawTileWeld",  "TileWeld",  tileName1));
        tile1->TileParent.setValue(this);
    }

    std::string tileName2 = doc->getUniqueObjectName("TileWeld");
    auto tile2Obj( doc->addObject( "TechDraw::DrawTileWeld", tileName2.c_str() ) );
    DrawTileWeld* tile2 = dynamic_cast<DrawTileWeld*>(tile2Obj);
    if (tile2) {
        tile2->Label.setValue(DrawUtil::translateArbitrary("DrawTileWeld",  "TileWeld",  tileName2));
        tile2->TileParent.setValue(this);
        tile2->TileRow.setValue(-1);   //other side is row -1
    }

    DrawView::onSettingDocument();
}

void DrawWeldSymbol::onChanged(const App::Property* prop)
{
    DrawView::onChanged(prop);
}

short DrawWeldSymbol::mustExecute() const
{
    return DrawView::mustExecute();
}

App::DocumentObjectExecReturn *DrawWeldSymbol::execute()
{
//    Base::Console().Message("DWS::execute()\n");
    if (!keepUpdated()) {
        return DrawView::execute();
    }

    overrideKeepUpdated(false);
    return DrawView::execute();
}

std::vector<DrawTileWeld*> DrawWeldSymbol::getTiles() const
{
//    Base::Console().Message("DWS::getTiles()\n");
    std::vector<DrawTileWeld*> result;

    std::vector<App::DocumentObject*> tiles = getInList();
    if (tiles.empty()) {
        return result;
    }

    for(std::vector<App::DocumentObject *>::iterator it = tiles.begin(); it != tiles.end(); it++) {
        if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawTileWeld::getClassTypeId())) {
            App::DocumentObject* doTemp = (*it);
            DrawTileWeld* temp = static_cast<DrawTileWeld*>(doTemp);
            result.push_back(temp);
        }
    }
    return result;
}

bool DrawWeldSymbol::isTailRightSide()
{
    App::DocumentObject* obj = Leader.getValue();
    TechDraw::DrawLeaderLine* realLeader = dynamic_cast<TechDraw::DrawLeaderLine*>(obj);
    if (realLeader) {
        Base::Vector3d tail = realLeader->getTailPoint();
        Base::Vector3d kink = realLeader->getKinkPoint();
        if (tail.x < kink.x)  {   //tail is to left
            return false;
        }
    }
    return true;
}


PyObject *DrawWeldSymbol::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawWeldSymbolPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawWeldSymbolPython, TechDraw::DrawWeldSymbol)
template<> const char* TechDraw::DrawWeldSymbolPython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderWeld";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawWeldSymbol>;
}

