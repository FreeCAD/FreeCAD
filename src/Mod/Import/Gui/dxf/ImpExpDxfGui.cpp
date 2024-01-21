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

// #include <App/Annotation.h>
// #include <App/Application.h>
// #include <App/Document.h>
// #include <Base/Console.h>
// #include <Base/Interpreter.h>
// #include <Base/Matrix.h>
// #include <Base/Parameter.h>
// #include <Base/Vector3D.h>
// #include <Base/PlacementPy.h>
// #include <Mod/Part/App/PartFeature.h>
#include <Gui/Application.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include "ImpExpDxfGui.h"

using namespace ImportGui;

ImpExpDxfReadGui::ImpExpDxfReadGui(std::string filepath, App::Document* pcDoc)
    : ImpExpDxfRead(filepath, pcDoc)
    , GuiDocument(Gui::Application::Instance->getDocument(pcDoc))
{}

void ImpExpDxfReadGui::ApplyGuiStyles(Part::Feature* object)
{
    PartGui::ViewProviderPart* view =
        static_cast<PartGui::ViewProviderPart*>(GuiDocument->getViewProvider(object));
    App::Color color = ObjectColor();
    view->LineColor.setValue(color);
    view->PointColor.setValue(color);
    view->ShapeColor.setValue(color);
    view->Transparency.setValue(0);
}

void ImpExpDxfReadGui::ApplyGuiStyles(App::FeaturePython* object)
{
    static Base::Type PropertyColorType = App::PropertyColor::getClassTypeId();

    auto view = static_cast<Gui::ViewProviderDocumentObject*>(GuiDocument->getViewProvider(object));
    App::Color color = ObjectColor();

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
}
