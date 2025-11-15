// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <Standard_Version.hxx>
#if OCC_VERSION_HEX < 0x070600
# include <BRepAdaptor_HCurve.hxx>
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

#include <regex>

#include <App/Link.h>

#include <Gui/Application.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/ViewProviderLink.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include <App/DocumentObjectGroup.h>
#include <App/PropertyStandard.h>
#include <Base/Color.h>
#include <Base/Vector3D.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>

#include "ImpExpDxfGui.h"

#include <map>

using namespace ImportGui;

ImpExpDxfReadGui::ImpExpDxfReadGui(const std::string& filepath, App::Document* pcDoc)
    : ImpExpDxfRead(filepath, pcDoc)
    , GuiDocument(Gui::Application::Instance->getDocument(pcDoc))
{}

void ImpExpDxfReadGui::ApplyGuiStyles(Part::Feature* object) const
{
    auto view = static_cast<PartGui::ViewProviderPart*>(GuiDocument->getViewProvider(object));
    Base::Color color = ObjectColor(m_entityAttributes.m_Color);
    view->LineColor.setValue(color);
    view->PointColor.setValue(color);
    view->ShapeAppearance.setDiffuseColor(color);
    view->DrawStyle.setValue(GetDrawStyle());
    view->Transparency.setValue(0);
}

void ImpExpDxfReadGui::ApplyGuiStyles(App::Link* object) const
{
    auto view = GuiDocument->getViewProvider(object);

    // The ViewProvider for an App::Link is a ViewProviderLink
    auto* vpLink = dynamic_cast<Gui::ViewProviderLink*>(view);
    if (!vpLink) {
        return;
    }

    if (m_preserveColors) {
        // The user wants to see colors from the DXF file.
        // We style the link by setting its ViewProvider's properties directly,
        // which is the same mechanism used for standard Part::Features.
        Base::Color color = ObjectColor(m_entityAttributes.m_Color);

        // The ViewProviderLink does not have LineColor/PointColor properties itself,
        // but setting them on the base ViewProvider seems to be respected by the renderer.
        // If this does not work, the properties would need to be added to ViewProviderLink.
        if (auto* prop = view->getPropertyByName("LineColor")) {
            static_cast<App::PropertyColor*>(prop)->setValue(color);
        }
        if (auto* prop = view->getPropertyByName("PointColor")) {
            static_cast<App::PropertyColor*>(prop)->setValue(color);
        }
        if (auto* prop = view->getPropertyByName("ShapeColor")) {
            static_cast<App::PropertyColor*>(prop)->setValue(color);
        }
        if (auto* prop = view->getPropertyByName("DrawStyle")) {
            static_cast<App::PropertyEnumeration*>(prop)->setValue(GetDrawStyle());
        }
        if (auto* prop = view->getPropertyByName("Transparency")) {
            static_cast<App::PropertyInteger*>(prop)->setValue(0);
        }
    }
}

void ImpExpDxfReadGui::ApplyGuiStyles(App::FeaturePython* object) const
{
    static Base::Type PropertyColorType = App::PropertyColor::getClassTypeId();

    auto view = static_cast<Gui::ViewProviderDocumentObject*>(GuiDocument->getViewProvider(object));
    Base::Color color = ObjectColor(m_entityAttributes.m_Color);

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
        std::basic_regex("dot.*dash|dash.*dot|^cent(er|re)|^divide|^phantom", std::regex::icase)
    };
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

int ImportGui::getAciForObject(App::DocumentObject* obj, bool isText)
{

    static const std::map<int, Base::Color> AciColorMap = {
        {0, Base::Color(0.0f, 0.0f, 0.0f)},
        {1, Base::Color(0.99609375f, 0.0f, 0.0f)},
        {2, Base::Color(0.99609375f, 0.99609375f, 0.0f)},
        {3, Base::Color(0.0f, 0.99609375f, 0.0f)},
        {4, Base::Color(0.0f, 0.99609375f, 0.99609375f)},
        {5, Base::Color(0.0f, 0.0f, 0.99609375f)},
        {6, Base::Color(0.99609375f, 0.0f, 0.99609375f)},
        {7, Base::Color(0.99609375f, 0.99609375f, 0.99609375f)},
        {8, Base::Color(0.25390625f, 0.25390625f, 0.25390625f)},
        {9, Base::Color(0.5f, 0.5f, 0.5f)},
        {10, Base::Color(0.99609375f, 0.0f, 0.0f)},
        {11, Base::Color(0.99609375f, 0.6640625f, 0.6640625f)},
        {12, Base::Color(0.73828125f, 0.0f, 0.0f)},
        {13, Base::Color(0.73828125f, 0.4921875f, 0.4921875f)},
        {14, Base::Color(0.50390625f, 0.0f, 0.0f)},
        {15, Base::Color(0.50390625f, 0.3359375f, 0.3359375f)},
        {16, Base::Color(0.40625f, 0.0f, 0.0f)},
        {17, Base::Color(0.40625f, 0.26953125f, 0.26953125f)},
        {18, Base::Color(0.30859375f, 0.0f, 0.0f)},
        {19, Base::Color(0.30859375f, 0.20703125f, 0.20703125f)},
        {20, Base::Color(0.99609375f, 0.24609375f, 0.0f)},
        {21, Base::Color(0.99609375f, 0.74609375f, 0.6640625f)},
        {22, Base::Color(0.73828125f, 0.1796875f, 0.0f)},
        {23, Base::Color(0.73828125f, 0.55078125f, 0.4921875f)},
        {24, Base::Color(0.50390625f, 0.12109375f, 0.0f)},
        {25, Base::Color(0.50390625f, 0.375f, 0.3359375f)},
        {26, Base::Color(0.40625f, 0.09765625f, 0.0f)},
        {27, Base::Color(0.40625f, 0.3046875f, 0.26953125f)},
        {28, Base::Color(0.30859375f, 0.07421875f, 0.0f)},
        {29, Base::Color(0.30859375f, 0.23046875f, 0.20703125f)},
        {30, Base::Color(0.99609375f, 0.49609375f, 0.0f)},
        {31, Base::Color(0.99609375f, 0.828125f, 0.6640625f)},
        {32, Base::Color(0.73828125f, 0.3671875f, 0.0f)},
        {33, Base::Color(0.73828125f, 0.61328125f, 0.4921875f)},
        {34, Base::Color(0.50390625f, 0.25f, 0.0f)},
        {35, Base::Color(0.50390625f, 0.41796875f, 0.3359375f)},
        {36, Base::Color(0.40625f, 0.203125f, 0.0f)},
        {37, Base::Color(0.40625f, 0.3359375f, 0.26953125f)},
        {38, Base::Color(0.30859375f, 0.15234375f, 0.0f)},
        {39, Base::Color(0.30859375f, 0.2578125f, 0.20703125f)},
        {40, Base::Color(0.99609375f, 0.74609375f, 0.0f)},
        {41, Base::Color(0.99609375f, 0.9140625f, 0.6640625f)},
        {42, Base::Color(0.73828125f, 0.55078125f, 0.0f)},
        {43, Base::Color(0.73828125f, 0.67578125f, 0.4921875f)},
        {44, Base::Color(0.50390625f, 0.375f, 0.0f)},
        {45, Base::Color(0.50390625f, 0.4609375f, 0.3359375f)},
        {46, Base::Color(0.40625f, 0.3046875f, 0.0f)},
        {47, Base::Color(0.40625f, 0.37109375f, 0.26953125f)},
        {48, Base::Color(0.30859375f, 0.23046875f, 0.0f)},
        {49, Base::Color(0.30859375f, 0.28515625f, 0.20703125f)},
        {50, Base::Color(0.99609375f, 0.99609375f, 0.0f)},
        {51, Base::Color(0.99609375f, 0.99609375f, 0.6640625f)},
        {52, Base::Color(0.73828125f, 0.73828125f, 0.0f)},
        {53, Base::Color(0.73828125f, 0.73828125f, 0.4921875f)},
        {54, Base::Color(0.50390625f, 0.50390625f, 0.0f)},
        {55, Base::Color(0.50390625f, 0.50390625f, 0.3359375f)},
        {56, Base::Color(0.40625f, 0.40625f, 0.0f)},
        {57, Base::Color(0.40625f, 0.40625f, 0.26953125f)},
        {58, Base::Color(0.30859375f, 0.30859375f, 0.0f)},
        {59, Base::Color(0.30859375f, 0.30859375f, 0.20703125f)},
        {60, Base::Color(0.74609375f, 0.99609375f, 0.0f)},
        {61, Base::Color(0.9140625f, 0.99609375f, 0.6640625f)},
        {62, Base::Color(0.55078125f, 0.73828125f, 0.0f)},
        {63, Base::Color(0.67578125f, 0.73828125f, 0.4921875f)},
        {64, Base::Color(0.375f, 0.50390625f, 0.0f)},
        {65, Base::Color(0.4609375f, 0.50390625f, 0.3359375f)},
        {66, Base::Color(0.3046875f, 0.40625f, 0.0f)},
        {67, Base::Color(0.37109375f, 0.40625f, 0.26953125f)},
        {68, Base::Color(0.23046875f, 0.30859375f, 0.0f)},
        {69, Base::Color(0.28515625f, 0.30859375f, 0.20703125f)},
        {70, Base::Color(0.49609375f, 0.99609375f, 0.0f)},
        {71, Base::Color(0.828125f, 0.99609375f, 0.6640625f)},
        {72, Base::Color(0.3671875f, 0.73828125f, 0.0f)},
        {73, Base::Color(0.61328125f, 0.73828125f, 0.4921875f)},
        {74, Base::Color(0.25f, 0.50390625f, 0.0f)},
        {75, Base::Color(0.41796875f, 0.50390625f, 0.3359375f)},
        {76, Base::Color(0.203125f, 0.40625f, 0.0f)},
        {77, Base::Color(0.3359375f, 0.40625f, 0.26953125f)},
        {78, Base::Color(0.15234375f, 0.30859375f, 0.0f)},
        {79, Base::Color(0.2578125f, 0.30859375f, 0.20703125f)},
        {80, Base::Color(0.24609375f, 0.99609375f, 0.0f)},
        {81, Base::Color(0.74609375f, 0.99609375f, 0.6640625f)},
        {82, Base::Color(0.1796875f, 0.73828125f, 0.0f)},
        {83, Base::Color(0.55078125f, 0.73828125f, 0.4921875f)},
        {84, Base::Color(0.12109375f, 0.50390625f, 0.0f)},
        {85, Base::Color(0.375f, 0.50390625f, 0.3359375f)},
        {86, Base::Color(0.09765625f, 0.40625f, 0.0f)},
        {87, Base::Color(0.3046875f, 0.40625f, 0.26953125f)},
        {88, Base::Color(0.07421875f, 0.30859375f, 0.0f)},
        {89, Base::Color(0.23046875f, 0.30859375f, 0.20703125f)},
        {90, Base::Color(0.0f, 0.99609375f, 0.0f)},
        {91, Base::Color(0.6640625f, 0.99609375f, 0.6640625f)},
        {92, Base::Color(0.0f, 0.73828125f, 0.0f)},
        {93, Base::Color(0.4921875f, 0.73828125f, 0.4921875f)},
        {94, Base::Color(0.0f, 0.50390625f, 0.0f)},
        {95, Base::Color(0.3359375f, 0.50390625f, 0.3359375f)},
        {96, Base::Color(0.0f, 0.40625f, 0.0f)},
        {97, Base::Color(0.26953125f, 0.40625f, 0.26953125f)},
        {98, Base::Color(0.0f, 0.30859375f, 0.0f)},
        {99, Base::Color(0.20703125f, 0.30859375f, 0.20703125f)},
        {100, Base::Color(0.0f, 0.99609375f, 0.24609375f)},
        {101, Base::Color(0.6640625f, 0.99609375f, 0.74609375f)},
        {102, Base::Color(0.0f, 0.73828125f, 0.1796875f)},
        {103, Base::Color(0.4921875f, 0.73828125f, 0.55078125f)},
        {104, Base::Color(0.0f, 0.50390625f, 0.12109375f)},
        {105, Base::Color(0.3359375f, 0.50390625f, 0.375f)},
        {106, Base::Color(0.0f, 0.40625f, 0.09765625f)},
        {107, Base::Color(0.26953125f, 0.40625f, 0.3046875f)},
        {108, Base::Color(0.0f, 0.30859375f, 0.07421875f)},
        {109, Base::Color(0.20703125f, 0.30859375f, 0.23046875f)},
        {110, Base::Color(0.0f, 0.99609375f, 0.49609375f)},
        {111, Base::Color(0.6640625f, 0.99609375f, 0.828125f)},
        {112, Base::Color(0.0f, 0.73828125f, 0.3671875f)},
        {113, Base::Color(0.4921875f, 0.73828125f, 0.61328125f)},
        {114, Base::Color(0.0f, 0.50390625f, 0.25f)},
        {115, Base::Color(0.3359375f, 0.50390625f, 0.41796875f)},
        {116, Base::Color(0.0f, 0.40625f, 0.203125f)},
        {117, Base::Color(0.26953125f, 0.40625f, 0.3359375f)},
        {118, Base::Color(0.0f, 0.30859375f, 0.15234375f)},
        {119, Base::Color(0.20703125f, 0.30859375f, 0.2578125f)},
        {120, Base::Color(0.0f, 0.99609375f, 0.74609375f)},
        {121, Base::Color(0.6640625f, 0.99609375f, 0.9140625f)},
        {122, Base::Color(0.0f, 0.73828125f, 0.55078125f)},
        {123, Base::Color(0.4921875f, 0.73828125f, 0.67578125f)},
        {124, Base::Color(0.0f, 0.50390625f, 0.375f)},
        {125, Base::Color(0.3359375f, 0.50390625f, 0.4609375f)},
        {126, Base::Color(0.0f, 0.40625f, 0.3046875f)},
        {127, Base::Color(0.26953125f, 0.40625f, 0.37109375f)},
        {128, Base::Color(0.0f, 0.30859375f, 0.23046875f)},
        {129, Base::Color(0.20703125f, 0.30859375f, 0.28515625f)},
        {130, Base::Color(0.0f, 0.99609375f, 0.99609375f)},
        {131, Base::Color(0.6640625f, 0.99609375f, 0.99609375f)},
        {132, Base::Color(0.0f, 0.73828125f, 0.73828125f)},
        {133, Base::Color(0.4921875f, 0.73828125f, 0.73828125f)},
        {134, Base::Color(0.0f, 0.50390625f, 0.50390625f)},
        {135, Base::Color(0.3359375f, 0.50390625f, 0.50390625f)},
        {136, Base::Color(0.0f, 0.40625f, 0.40625f)},
        {137, Base::Color(0.26953125f, 0.40625f, 0.40625f)},
        {138, Base::Color(0.0f, 0.30859375f, 0.30859375f)},
        {139, Base::Color(0.20703125f, 0.30859375f, 0.30859375f)},
        {140, Base::Color(0.0f, 0.74609375f, 0.99609375f)},
        {141, Base::Color(0.6640625f, 0.9140625f, 0.99609375f)},
        {142, Base::Color(0.0f, 0.55078125f, 0.73828125f)},
        {143, Base::Color(0.4921875f, 0.67578125f, 0.73828125f)},
        {144, Base::Color(0.0f, 0.375f, 0.50390625f)},
        {145, Base::Color(0.3359375f, 0.4609375f, 0.50390625f)},
        {146, Base::Color(0.0f, 0.3046875f, 0.40625f)},
        {147, Base::Color(0.26953125f, 0.37109375f, 0.40625f)},
        {148, Base::Color(0.0f, 0.23046875f, 0.30859375f)},
        {149, Base::Color(0.20703125f, 0.28515625f, 0.30859375f)},
        {150, Base::Color(0.0f, 0.49609375f, 0.99609375f)},
        {151, Base::Color(0.6640625f, 0.828125f, 0.99609375f)},
        {152, Base::Color(0.0f, 0.3671875f, 0.73828125f)},
        {153, Base::Color(0.4921875f, 0.61328125f, 0.73828125f)},
        {154, Base::Color(0.0f, 0.25f, 0.50390625f)},
        {155, Base::Color(0.3359375f, 0.41796875f, 0.50390625f)},
        {156, Base::Color(0.0f, 0.203125f, 0.40625f)},
        {157, Base::Color(0.26953125f, 0.3359375f, 0.40625f)},
        {158, Base::Color(0.0f, 0.15234375f, 0.30859375f)},
        {159, Base::Color(0.20703125f, 0.2578125f, 0.30859375f)},
        {160, Base::Color(0.0f, 0.24609375f, 0.99609375f)},
        {161, Base::Color(0.6640625f, 0.74609375f, 0.99609375f)},
        {162, Base::Color(0.0f, 0.1796875f, 0.73828125f)},
        {163, Base::Color(0.4921875f, 0.55078125f, 0.73828125f)},
        {164, Base::Color(0.0f, 0.12109375f, 0.50390625f)},
        {165, Base::Color(0.3359375f, 0.375f, 0.50390625f)},
        {166, Base::Color(0.0f, 0.09765625f, 0.40625f)},
        {167, Base::Color(0.26953125f, 0.3046875f, 0.40625f)},
        {168, Base::Color(0.0f, 0.07421875f, 0.30859375f)},
        {169, Base::Color(0.20703125f, 0.23046875f, 0.30859375f)},
        {170, Base::Color(0.0f, 0.0f, 0.99609375f)},
        {171, Base::Color(0.6640625f, 0.6640625f, 0.99609375f)},
        {172, Base::Color(0.0f, 0.0f, 0.73828125f)},
        {173, Base::Color(0.4921875f, 0.4921875f, 0.73828125f)},
        {174, Base::Color(0.0f, 0.0f, 0.50390625f)},
        {175, Base::Color(0.3359375f, 0.3359375f, 0.50390625f)},
        {176, Base::Color(0.0f, 0.0f, 0.40625f)},
        {177, Base::Color(0.26953125f, 0.26953125f, 0.40625f)},
        {178, Base::Color(0.0f, 0.0f, 0.30859375f)},
        {179, Base::Color(0.20703125f, 0.20703125f, 0.30859375f)},
        {180, Base::Color(0.24609375f, 0.0f, 0.99609375f)},
        {181, Base::Color(0.74609375f, 0.6640625f, 0.99609375f)},
        {182, Base::Color(0.1796875f, 0.0f, 0.73828125f)},
        {183, Base::Color(0.55078125f, 0.4921875f, 0.73828125f)},
        {184, Base::Color(0.12109375f, 0.0f, 0.50390625f)},
        {185, Base::Color(0.375f, 0.3359375f, 0.50390625f)},
        {186, Base::Color(0.09765625f, 0.0f, 0.40625f)},
        {187, Base::Color(0.3046875f, 0.26953125f, 0.40625f)},
        {188, Base::Color(0.07421875f, 0.0f, 0.30859375f)},
        {189, Base::Color(0.23046875f, 0.20703125f, 0.30859375f)},
        {190, Base::Color(0.49609375f, 0.0f, 0.99609375f)},
        {191, Base::Color(0.828125f, 0.6640625f, 0.99609375f)},
        {192, Base::Color(0.3671875f, 0.0f, 0.73828125f)},
        {193, Base::Color(0.61328125f, 0.4921875f, 0.73828125f)},
        {194, Base::Color(0.25f, 0.0f, 0.50390625f)},
        {195, Base::Color(0.41796875f, 0.3359375f, 0.50390625f)},
        {196, Base::Color(0.203125f, 0.0f, 0.40625f)},
        {197, Base::Color(0.3359375f, 0.26953125f, 0.40625f)},
        {198, Base::Color(0.15234375f, 0.0f, 0.30859375f)},
        {199, Base::Color(0.2578125f, 0.20703125f, 0.30859375f)},
        {200, Base::Color(0.74609375f, 0.0f, 0.99609375f)},
        {201, Base::Color(0.9140625f, 0.6640625f, 0.99609375f)},
        {202, Base::Color(0.55078125f, 0.0f, 0.73828125f)},
        {203, Base::Color(0.67578125f, 0.4921875f, 0.73828125f)},
        {204, Base::Color(0.375f, 0.0f, 0.50390625f)},
        {205, Base::Color(0.4609375f, 0.3359375f, 0.50390625f)},
        {206, Base::Color(0.3046875f, 0.0f, 0.40625f)},
        {207, Base::Color(0.37109375f, 0.26953125f, 0.40625f)},
        {208, Base::Color(0.23046875f, 0.0f, 0.30859375f)},
        {209, Base::Color(0.28515625f, 0.20703125f, 0.30859375f)},
        {210, Base::Color(0.99609375f, 0.0f, 0.99609375f)},
        {211, Base::Color(0.99609375f, 0.6640625f, 0.99609375f)},
        {212, Base::Color(0.73828125f, 0.0f, 0.73828125f)},
        {213, Base::Color(0.73828125f, 0.4921875f, 0.73828125f)},
        {214, Base::Color(0.50390625f, 0.0f, 0.50390625f)},
        {215, Base::Color(0.50390625f, 0.3359375f, 0.50390625f)},
        {216, Base::Color(0.40625f, 0.0f, 0.40625f)},
        {217, Base::Color(0.40625f, 0.26953125f, 0.40625f)},
        {218, Base::Color(0.30859375f, 0.0f, 0.30859375f)},
        {219, Base::Color(0.30859375f, 0.20703125f, 0.30859375f)},
        {220, Base::Color(0.99609375f, 0.0f, 0.74609375f)},
        {221, Base::Color(0.99609375f, 0.6640625f, 0.9140625f)},
        {222, Base::Color(0.73828125f, 0.0f, 0.55078125f)},
        {223, Base::Color(0.73828125f, 0.4921875f, 0.67578125f)},
        {224, Base::Color(0.50390625f, 0.0f, 0.375f)},
        {225, Base::Color(0.50390625f, 0.3359375f, 0.4609375f)},
        {226, Base::Color(0.40625f, 0.0f, 0.3046875f)},
        {227, Base::Color(0.40625f, 0.26953125f, 0.37109375f)},
        {228, Base::Color(0.30859375f, 0.0f, 0.23046875f)},
        {229, Base::Color(0.30859375f, 0.20703125f, 0.28515625f)},
        {230, Base::Color(0.99609375f, 0.0f, 0.49609375f)},
        {231, Base::Color(0.99609375f, 0.6640625f, 0.828125f)},
        {232, Base::Color(0.73828125f, 0.0f, 0.3671875f)},
        {233, Base::Color(0.73828125f, 0.4921875f, 0.61328125f)},
        {234, Base::Color(0.50390625f, 0.0f, 0.25f)},
        {235, Base::Color(0.50390625f, 0.3359375f, 0.41796875f)},
        {236, Base::Color(0.40625f, 0.0f, 0.203125f)},
        {237, Base::Color(0.40625f, 0.26953125f, 0.3359375f)},
        {238, Base::Color(0.30859375f, 0.0f, 0.15234375f)},
        {239, Base::Color(0.30859375f, 0.20703125f, 0.2578125f)},
        {240, Base::Color(0.99609375f, 0.0f, 0.24609375f)},
        {241, Base::Color(0.99609375f, 0.6640625f, 0.74609375f)},
        {242, Base::Color(0.73828125f, 0.0f, 0.1796875f)},
        {243, Base::Color(0.73828125f, 0.4921875f, 0.55078125f)},
        {244, Base::Color(0.50390625f, 0.0f, 0.12109375f)},
        {245, Base::Color(0.50390625f, 0.3359375f, 0.375f)},
        {246, Base::Color(0.40625f, 0.0f, 0.09765625f)},
        {247, Base::Color(0.40625f, 0.26953125f, 0.3046875f)},
        {248, Base::Color(0.30859375f, 0.0f, 0.07421875f)},
        {249, Base::Color(0.30859375f, 0.20703125f, 0.23046875f)},
        {250, Base::Color(0.19921875f, 0.19921875f, 0.19921875f)},
        {251, Base::Color(0.3125f, 0.3125f, 0.3125f)},
        {252, Base::Color(0.41015625f, 0.41015625f, 0.41015625f)},
        {253, Base::Color(0.5078125f, 0.5078125f, 0.5078125f)},
        {254, Base::Color(0.7421875f, 0.7421875f, 0.7421875f)},
        {255, Base::Color(0.99609375f, 0.99609375f, 0.99609375f)}
    };

    if (!Gui::Application::Instance || !obj) {
        return 0;
    }

    Gui::Document* gdoc = Gui::Application::Instance->getDocument(obj->getDocument());
    if (!gdoc) {
        return 0;
    }

    Gui::ViewProvider* vp = gdoc->getViewProvider(obj);
    if (!vp) {
        return 0;
    }

    // Handle BYLAYER for objects inside a Draft Layer (which is a group)
    // ACI value for BYLAYER is 256.
    for (const auto* parent : obj->getInList()) {
        if (parent->isDerivedFrom(App::DocumentObjectGroup::getClassTypeId())) {
            if (auto* vpParent = gdoc->getViewProvider(const_cast<App::DocumentObject*>(parent))) {
                if (vpParent->getPropertyByName("OverrideChildren")
                    && static_cast<App::PropertyBool*>(vpParent->getPropertyByName("OverrideChildren"))
                           ->getValue()) {
                    return 256;
                }
            }
        }
    }

    // Get the object's color from its ViewProvider
    Base::Color objColor;
    if (isText && vp->getPropertyByName("TextColor")) {
        objColor = static_cast<App::PropertyColor*>(vp->getPropertyByName("TextColor"))->getValue();
    }
    else if (vp->getPropertyByName("LineColor")) {
        objColor = static_cast<App::PropertyColor*>(vp->getPropertyByName("LineColor"))->getValue();
    }
    else {
        return 0;  // Default to black if no color property found
    }

    // Find the closest ACI color
    int bestAci = 0;
    float min_dist_sq = 1e9f;

    Base::Vector3f v_obj(objColor.r, objColor.g, objColor.b);

    for (const auto& pair : AciColorMap) {
        Base::Vector3f v_ref(pair.second.r, pair.second.g, pair.second.b);
        Base::Vector3f diff = v_ref - v_obj;
        float dist_sq = diff.Dot(diff);

        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            bestAci = pair.first;
            if (dist_sq == 0.0f) {
                break;  // Exact match found, no need to search further
            }
        }
    }
    return bestAci;
}
