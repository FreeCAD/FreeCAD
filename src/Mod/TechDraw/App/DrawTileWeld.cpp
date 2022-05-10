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

#include <sstream>
#include <fstream>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>

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
                      "Text before symbol");
    ADD_PROPERTY_TYPE(RightText, (nullptr), group, App::Prop_None, "Text after symbol");
    ADD_PROPERTY_TYPE(CenterText, (nullptr), group, App::Prop_None, "Text above/below symbol");
    ADD_PROPERTY_TYPE(SymbolFile, (prefSymbol()), group, App::Prop_None, "Symbol File");
    ADD_PROPERTY_TYPE(SymbolIncluded, (""), group, App::Prop_None,
                                            "Embedded Symbol. System use only.");   // n/a to end users

//    SymbolFile.setStatus(App::Property::ReadOnly,true);

    std::string svgFilter("Symbol files (*.svg *.SVG);;All files (*)");
    SymbolFile.setFilter(svgFilter);
}

DrawTileWeld::~DrawTileWeld()
{
}

void DrawTileWeld::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        App::Document* doc = getDocument();
        if ((prop == &SymbolFile) &&
            (doc != nullptr) ) {
            if (!SymbolFile.isEmpty()) {
                Base::FileInfo fi(SymbolFile.getValue());
                if (fi.isReadable()) {
                    replaceSymbolIncluded(SymbolFile.getValue());
                }
            }
        }
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

void DrawTileWeld::replaceSymbolIncluded(std::string newSymbolFile)
{
//    Base::Console().Message("DTW::replaceSymbolIncluded(%s)\n", newSymbolFile.c_str());
    if (SymbolIncluded.isEmpty()) {
        setupSymbolIncluded();
    } else {
        std::string tempName = SymbolIncluded.getExchangeTempFile();
        DrawUtil::copyFile(newSymbolFile, tempName);
        SymbolIncluded.setValue(tempName.c_str());
    }
}

void DrawTileWeld::onDocumentRestored() 
{
//    Base::Console().Message("DTW::onDocumentRestored()\n");
    if (SymbolIncluded.isEmpty()) {
        if (!SymbolFile.isEmpty()) {
            std::string symbolFileName = SymbolFile.getValue();
            Base::FileInfo tfi(symbolFileName);
            if (tfi.isReadable()) {
                if (SymbolIncluded.isEmpty()) {
                    setupSymbolIncluded();
                }
            }
        }
    }
    DrawTile::onDocumentRestored();
}

void DrawTileWeld::setupObject()
{
    //by this point DTW should have a name and belong to a document
    setupSymbolIncluded();

    DrawTile::setupObject();
}

void DrawTileWeld::setupSymbolIncluded(void)
{
//    Base::Console().Message("DTW::setupSymbolIncluded()\n");
    App::Document* doc = getDocument();
    std::string special = getNameInDocument();
    special += "Symbol.svg";
    std::string dir = doc->TransientDir.getValue();
    std::string symbolName = dir + special;

    //first Time
    std::string symbolIncluded = SymbolIncluded.getValue();
    if (symbolIncluded.empty()) {
        DrawUtil::copyFile(std::string(), symbolName);
        SymbolIncluded.setValue(symbolName.c_str());
    }

    std::string symbolFile = SymbolFile.getValue();
    if (!symbolFile.empty()) {
        std::string exchName = SymbolIncluded.getExchangeTempFile();
        DrawUtil::copyFile(symbolFile, exchName);
        Base::FileInfo fi(exchName);
        SymbolIncluded.setValue(exchName.c_str(), special.c_str());
    }
}

//standard preference getter (really a default in this case)
std::string DrawTileWeld::prefSymbol(void)
{
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Symbols/Welding/";
    std::string defaultFileName = defaultDir + "blankTile.svg";
    return defaultFileName;
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

