/***************************************************************************
 *   Copyright (c) WandererFan - 2016    (wandererfan@gmail.com)           *
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
#include <boost/regex.hpp>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>

#include "DrawViewDraft.h"

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewDraft
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewDraft, TechDraw::DrawViewSymbol)


DrawViewDraft::DrawViewDraft(void)
{
    static const char *group = "Draft view";

    ADD_PROPERTY_TYPE(Source ,(0),group,App::Prop_None,"Draft object for this view");
    ADD_PROPERTY_TYPE(LineScale,(1.0),group,App::Prop_None,"Line width adjustment factor for this view");
    ADD_PROPERTY_TYPE(FontSize,(12.0),group,App::Prop_None,"Text size for this view");
    ScaleType.setValue("Custom");
}

DrawViewDraft::~DrawViewDraft()
{
}

void DrawViewDraft::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &Source ||
            prop == &LineScale ||
            prop == &FontSize) {
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
        }
    }
    TechDraw::DrawViewSymbol::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawViewDraft::execute(void)
{
    App::DocumentObject* sourceObj = Source.getValue();
    if (sourceObj) {
        std::string svgFrag;
        std::string svgHead = getSVGHead();
        std::string svgTail = getSVGTail();
        std::string FeatName = getNameInDocument();
        std::string SourceName = sourceObj->getNameInDocument();

        std::stringstream paramStr;
        paramStr << ",scale=" << LineScale.getValue() << ",fontsize=" << FontSize.getValue();

// this is ok for a starting point, but should eventually make dedicated Draft functions that build the svg for all the special cases
// (Arch section, etc)
// like Draft.makeDrawingView, but we don't need to create the actual document objects in Draft, just the svg.
        Base::Interpreter().runString("import Draft");
        Base::Interpreter().runStringArg("svgBody = Draft.getSVG(App.activeDocument().%s %s)",
                                         SourceName.c_str(),paramStr.str().c_str());
//        Base::Interpreter().runString("print svgBody");
        Base::Interpreter().runStringArg("App.activeDocument().%s.Symbol = '%s' + svgBody + '%s'",
                                          FeatName.c_str(),svgHead.c_str(),svgTail.c_str());
        }
    return DrawView::execute();
}

std::string DrawViewDraft::getSVGHead(void)
{
    std::string head = std::string("<svg\\n") +
                       std::string("	xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"\\n") +
                       std::string("	xmlns:freecad=\"http://www.freecadweb.org/wiki/index.php?title=Svg_Namespace\">\\n");
    return head;
}

std::string DrawViewDraft::getSVGTail(void)
{
    std::string tail = "\\n</svg>";
    return tail;
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewDraftPython, TechDraw::DrawViewDraft)
template<> const char* TechDraw::DrawViewDraftPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderSymbol";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewDraft>;
}
