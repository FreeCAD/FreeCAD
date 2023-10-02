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
# include <sstream>
#endif

#include <App/Application.h>
#include <App/Document.h>

#include "DrawTileWeld.h"
#include "DrawTileWeldPy.h"  // generated from DrawTileWeldPy.xml
#include "DrawUtil.h"


using namespace TechDraw;

//===========================================================================
// DrawTileWeld - attachable tile
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawTileWeld, TechDraw::DrawTile)

DrawTileWeld::DrawTileWeld()
{
    static const char *group = "TileWeld";

    ADD_PROPERTY_TYPE(LeftText, (""), group, (App::PropertyType)(App::Prop_None),
                      "Text before symbol");
    ADD_PROPERTY_TYPE(RightText, (nullptr), group, App::Prop_None, "Text after symbol");
    ADD_PROPERTY_TYPE(CenterText, (nullptr), group, App::Prop_None, "Text above/below symbol");
    ADD_PROPERTY_TYPE(SymbolFile, (prefSymbol()), group, App::Prop_None, "Symbol File");
    ADD_PROPERTY_TYPE(SymbolIncluded, (""), group, App::Prop_None,
                                            "Embedded Symbol. System use only.");   // n/a to end users

    std::string svgFilter("Symbol files (*.svg *.SVG);;All files (*)");
    SymbolFile.setFilter(svgFilter);
}

DrawTileWeld::~DrawTileWeld()
{
}

void DrawTileWeld::onChanged(const App::Property* prop)
{
    if (isRestoring()) {
        DrawTile::onChanged(prop);
        return;
    }

    if (prop == &SymbolFile) {
        replaceFileIncluded(SymbolFile.getValue());
    }

    DrawTile::onChanged(prop);
}

void DrawTileWeld::replaceFileIncluded(std::string newSymbolFile)
{
//    Base::Console().Message("DTW::replaceFileIncluded(%s)\n", newSymbolFile.c_str());
    if (newSymbolFile.empty()) {
        return;
    }

    Base::FileInfo tfi(newSymbolFile);
    if (tfi.isReadable()) {
        SymbolIncluded.setValue(newSymbolFile.c_str());
    } else {
        throw Base::RuntimeError("Could not read the new symbol file");
    }
}

void DrawTileWeld::setupObject()
{
    //by this point DTW should have a name and belong to a document
    replaceFileIncluded(SymbolFile.getValue());

    DrawTile::setupObject();
}

//standard preference getter (really a default in this case)
std::string DrawTileWeld::prefSymbol()
{
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Symbols/Welding/";
    std::string defaultFileName = defaultDir + "blankTile.svg";
    return defaultFileName;
}

PyObject *DrawTileWeld::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawTileWeldPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawTileWeldPython, TechDraw::DrawTileWeld)
template<> const char* TechDraw::DrawTileWeldPython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderTile";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawTileWeld>;
}

