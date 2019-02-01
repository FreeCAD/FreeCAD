/***************************************************************************
 *   Copyright (c) York van Havre 2016 yorik@uncreated.net                 *
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

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>

#include "DrawViewArch.h"

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewArch
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewArch, TechDraw::DrawViewSymbol)

const char* DrawViewArch::RenderModeEnums[]= {"Wireframe",
                                      "Solid",
                                      NULL};

DrawViewArch::DrawViewArch(void)
{
    static const char *group = "Arch view";

    ADD_PROPERTY_TYPE(Source ,(0),group,App::Prop_None,"Section Plane object for this view");
    Source.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(AllOn ,(false),group,App::Prop_None,"If hidden objects must be shown or not");
    RenderMode.setEnums(RenderModeEnums);
    ADD_PROPERTY_TYPE(RenderMode, ((long)0),group,App::Prop_None,"The render mode to use");
    ADD_PROPERTY_TYPE(ShowHidden ,(false),group,App::Prop_None,"If the hidden geometry behind the section plane is shown or not");
    ADD_PROPERTY_TYPE(ShowFill ,(false),group,App::Prop_None,"If cut areas must be filled with a hatch pattern or not");
    ADD_PROPERTY_TYPE(LineWidth,(0.35),group,App::Prop_None,"Line width of this view");
    ADD_PROPERTY_TYPE(FontSize,(12.0),group,App::Prop_None,"Text size for this view");
    ScaleType.setValue("Custom");
}

DrawViewArch::~DrawViewArch()
{
}

void DrawViewArch::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &Source ||
            prop == &AllOn ||
            prop == &RenderMode ||
            prop == &ShowHidden ||
            prop == &ShowFill ||
            prop == &LineWidth ||
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

App::DocumentObjectExecReturn *DrawViewArch::execute(void)
{
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    App::DocumentObject* sourceObj = Source.getValue();
    if (sourceObj) {
        std::string svgFrag;
        std::string svgHead = getSVGHead();
        std::string svgTail = getSVGTail();
        std::string FeatName = getNameInDocument();
        std::string SourceName = sourceObj->getNameInDocument();
        // ArchSectionPlane.getSVG(section,allOn=False,renderMode="Wireframe",showHidden=False,showFill=False,scale=1,linewidth=1,fontsize=1):

        std::stringstream paramStr;
        paramStr << ",allOn=" << (AllOn.getValue() ? "True" : "False")
                 << ",renderMode=" << RenderMode.getValue()
                 << ",showHidden=" << (ShowHidden.getValue() ? "True" : "False")
                 << ",showFill=" << (ShowFill.getValue() ? "True" : "False")
                 << ",scale=" << getScale()
                 << ",linewidth=" << LineWidth.getValue()
                 << ",fontsize=" << FontSize.getValue()
                 << ",techdraw=True"
                 << ",rotation=" << Rotation.getValue();

        Base::Interpreter().runString("import ArchSectionPlane");
        Base::Interpreter().runStringArg("svgBody = ArchSectionPlane.getSVG(App.activeDocument().%s %s)",
                                         SourceName.c_str(),paramStr.str().c_str());
        Base::Interpreter().runStringArg("App.activeDocument().%s.Symbol = '%s' + svgBody + '%s'",
                                          FeatName.c_str(),svgHead.c_str(),svgTail.c_str());
    }
    requestPaint();
    return DrawView::execute();
}

std::string DrawViewArch::getSVGHead(void)
{
    std::string head = std::string("<svg\\n") +
                       std::string("	xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"\\n") +
                       std::string("	xmlns:freecad=\"http://www.freecadweb.org/wiki/index.php?title=Svg_Namespace\">\\n");
    return head;
}

std::string DrawViewArch::getSVGTail(void)
{
    std::string tail = "\\n</svg>";
    return tail;
}

