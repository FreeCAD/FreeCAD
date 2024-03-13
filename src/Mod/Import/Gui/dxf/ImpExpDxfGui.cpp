/***************************************************************************
 *   Copyright (c) 2015 Yorik van Havre (yorik@uncreated.net)              *
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

#include <FCConfig.h>
#ifndef _PreComp_
#include <Standard_Version.hxx>
#if OCC_VERSION_HEX < 0x070600
#include <BRepAdaptor_HCurve.hxx>
#endif
#include <Approx_Curve3d.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRep_Builder.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <Geom_BSplineCurve.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Elips.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#endif
#include <regex>

#include <Gui/Application.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include "ImpExpDxfGui.h"

using namespace ImportGui;

ImpExpDxfReadGui::ImpExpDxfReadGui(const std::string& filepath, App::Document* pcDoc)
    : ImpExpDxfRead(filepath, pcDoc)
    , GuiDocument(Gui::Application::Instance->getDocument(pcDoc))
{}

void ImpExpDxfReadGui::ApplyGuiStyles(Part::Feature* object) const
{
    auto view = static_cast<PartGui::ViewProviderPart*>(GuiDocument->getViewProvider(object));
    App::Color color = ObjectColor(m_entityAttributes.m_Color);
    view->LineColor.setValue(color);
    view->PointColor.setValue(color);
    view->ShapeColor.setValue(color);
    view->DrawStyle.setValue(GetDrawStyle());
    view->Transparency.setValue(0);
}

void ImpExpDxfReadGui::ApplyGuiStyles(App::FeaturePython* object) const
{
    static Base::Type PropertyColorType = App::PropertyColor::getClassTypeId();

    auto view = static_cast<Gui::ViewProviderDocumentObject*>(GuiDocument->getViewProvider(object));
    App::Color color = ObjectColor(m_entityAttributes.m_Color);

    // The properties on this object depend on which Python object is wrapped around it.
    // For now we look for the two colors we expect in text and dimensions, and check that they
    // exist and have the correct type before setting them.
    // A more general choice would be to iterate over all the properties and set all the ones of
    // this type, or perhaps only if their name ends in "Color"
    auto prop = view->getPropertyByName("TextColor");
    if (prop != nullptr && prop->getTypeId() == PropertyColorType) {
        static_cast<App::PropertyColor*>(prop)->setValue(color);
    }
    prop = view->getPropertyByName("LineColor");
    if (prop != nullptr && prop->getTypeId() == PropertyColorType) {
        static_cast<App::PropertyColor*>(prop)->setValue(color);
    }
    prop = view->getPropertyByName("DrawStyle");
    if (prop != nullptr && prop->getTypeId() == PropertyColorType) {
        static_cast<App::PropertyColor*>(prop)->setValue(GetDrawStyle());
    }
}

int ImpExpDxfReadGui::GetDrawStyle() const
{
    // The DXF line type can be quite complex, including both patterns, in-line text (for instance,
    // so a line can mark itself as "Gas Main"), and shapes from a .SHX file (so a line can have,
    // say, diamond shapes along it). The coin package we use to render objects into the screen has
    // no facility for doing the in-line text or shapes, and has limited control over line/space
    // patterns. The line pattern is divided into 16 pieces which can be either drawn or not. There
    // is also support for line pattern scaling (to match the LTSCALE DXF property). Unfortunately
    // neither of these is exposed at the FreeCAD level; line types are dumbed down to four choices:
    // 0  "Solid"   0xffff i.e. solid
    // 1  "Dashed"  0xf00f i.e. coarse dashes
    // 2  "Dotted"  0x0f0f i.e. fine (half-size) dashes
    // 3  "DashDot" 0xff88 i.e. long dash/dot (like a center line)
    // Right now we infer these from the DXF LINETYPE name. In the long run we should look at the
    // actual pattern, but the CDxfRead class does nothing with the LTYPE table. In the longer run,
    // FreeCAD should expose the line pattern (the 16-bit number) for more versatility.

    // Make an array of patterns whose index is the "draw style" enum value for the pattern.
    static std::array<std::regex, 4> matchers {
        // Starts with "cont" (i.e. continuous) : return solid
        // I can't find anything that actually says what happens if you have a top-level (i.e. not
        // in a BLOCK) object whose line type is BYBLOCK. We treat this as Continuous.
        std::basic_regex("^cont|^byblock$", std::regex::icase),
        // starts with hidden, border, dash : return dashed
        std::basic_regex("^hidden|^border|^dash", std::regex::icase),
        // starts with dot : return dotted
        std::basic_regex("^dot", std::regex::icase),
        // dash & dot or center/centre/divide/phantom : return DashDot
        std::basic_regex("dot.*dash|dash.*dot|^cent(er|re)|^divide|^phantom", std::regex::icase)};
    // Some line type names can match several patterns, in particular dotdash can also match ^dot
    // and dashdot can match ^dash Rather than code the latter patterns to forbid subsequent "dash"
    // or "dot" so they are mutually exclusive, we just match the more specific pattern, which is
    // the last one, first.
    for (int i = matchers.size(); --i >= 0;) {
        if (regex_search(m_entityAttributes.m_LineType, matchers.at(i))) {
            return i;
        }
    }
    // If we don't understand it, return solid
    return 0;
}
