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
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>

#include "DrawUtil.h"

#include <Mod/TechDraw/App/DrawWeldSymbolPy.h>  // generated from DrawWeldSymbolPy.xml

#include "DrawLeaderLine.h"
#include "DrawTile.h"
#include "DrawTileWeld.h"
#include "DrawWeldSymbol.h"

using namespace TechDraw;

//===========================================================================
// DrawWeldSymbol - welding symbol
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawWeldSymbol, TechDraw::DrawView)

DrawWeldSymbol::DrawWeldSymbol(void)
{
    static const char *group = "Weld Symbol";

    ADD_PROPERTY_TYPE(Leader,(0),group,(App::PropertyType)(App::Prop_None), "Parent Leader");
    ADD_PROPERTY_TYPE(AllAround, (false), group, App::Prop_None, "All Around Symbol on/off");
    ADD_PROPERTY_TYPE(FieldWeld, (false), group, App::Prop_None, "Field Weld Symbol on/off");
    ADD_PROPERTY_TYPE(AlternatingWeld, (false), group, App::Prop_None, "Alternating Weld true/false");
    ADD_PROPERTY_TYPE(TailText, (""), group, App::Prop_None, "Text at tail of symbol");

    Caption.setStatus(App::Property::Hidden,true);
    Scale.setStatus(App::Property::Hidden,true);
    ScaleType.setStatus(App::Property::Hidden,true);

}

DrawWeldSymbol::~DrawWeldSymbol()
{
}

void DrawWeldSymbol::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        //nothing in particular
    }
    DrawView::onChanged(prop);
}

short DrawWeldSymbol::mustExecute() const
{
    return DrawView::mustExecute();
}

App::DocumentObjectExecReturn *DrawWeldSymbol::execute(void)
{ 
//    Base::Console().Message("DWS::execute()\n");
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    return DrawView::execute();
}

std::vector<DrawTileWeld*> DrawWeldSymbol::getTiles(void) const
{
//    Base::Console().Message("DWS::getTiles()\n");
//    std::vector<App::DocumentObject*> temp;
    std::vector<DrawTileWeld*> result;

    std::vector<App::DocumentObject*> tiles = getInList();
    if (!tiles.empty()) {
        for(std::vector<App::DocumentObject *>::iterator it = tiles.begin(); it != tiles.end(); it++) {
            if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawTileWeld::getClassTypeId())) {
                App::DocumentObject* doTemp = (*it);
                DrawTileWeld* temp = static_cast<DrawTileWeld*>(doTemp);
                result.push_back(temp);
            }
        }
    }
    return result;
}

bool DrawWeldSymbol::isTailRightSide()
{
    bool result = true;
    App::DocumentObject* obj = Leader.getValue();
    TechDraw::DrawLeaderLine* realLeader = dynamic_cast<TechDraw::DrawLeaderLine*>(obj);
    if (realLeader != nullptr) {
        Base::Vector3d tail = realLeader->getTailPoint();
        Base::Vector3d kink = realLeader->getKinkPoint();
        if (tail.x < kink.x)  {   //tail is to left
            result = false;
        }
    }
    return result;
}


PyObject *DrawWeldSymbol::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawWeldSymbolPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawWeldSymbolPython, TechDraw::DrawWeldSymbol)
template<> const char* TechDraw::DrawWeldSymbolPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderWeld";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawWeldSymbol>;
}

