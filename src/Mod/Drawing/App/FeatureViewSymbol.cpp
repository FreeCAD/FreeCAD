/***************************************************************************
 *   Copyright (c) Yorik van Havre          (yorik@uncreated.net) 2013     *
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

#include <iomanip>

#include <Base/Exception.h>
#include <Base/FileInfo.h>

#include "FeatureViewSymbol.h"

using namespace Drawing;
using namespace std;


//===========================================================================
// FeatureViewSymbol
//===========================================================================

PROPERTY_SOURCE(Drawing::FeatureViewSymbol, Drawing::FeatureView)


FeatureViewSymbol::FeatureViewSymbol(void) 
{
    static const char *vgroup = "Drawing view";

    ADD_PROPERTY_TYPE(Symbol,(""),vgroup,App::Prop_Hidden,"The SVG code defining this symbol");

}

FeatureViewSymbol::~FeatureViewSymbol()
{
}

App::DocumentObjectExecReturn *FeatureViewSymbol::execute(void)
{
    std::stringstream result;
    result  << "<g transform=\"translate(" << X.getValue() << "," << Y.getValue() << ")"
            << " rotate(" << Rotation.getValue() << ")"
            << " scale(" << Scale.getValue() << ")\">" << endl
            << Symbol.getValue() << endl
            << "</g>" << endl;

    // Apply the resulting fragment
    ViewResult.setValue(result.str().c_str());

    return App::DocumentObject::StdReturn;
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Drawing::FeatureViewSymbolPython, Drawing::FeatureViewSymbol)
template<> const char* Drawing::FeatureViewSymbolPython::getViewProviderName(void) const {
    return "DrawingGui::ViewProviderDrawingView";
}
/// @endcond

// explicit template instantiation
template class DrawingExport FeaturePythonT<Drawing::FeatureViewSymbol>;
}
