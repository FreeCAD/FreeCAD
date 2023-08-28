/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QAction>
# include <QMenu>

# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Geom_BezierCurve.hxx>
# include <Geom_BezierSurface.hxx>
# include <Geom_BSplineCurve.hxx>
# include <Geom_BSplineSurface.hxx>
# include <gp_Pnt.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Shape.hxx>
# include <TopoDS_Shell.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>

# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
#endif

#include <Gui/ActionFunction.h>
#include <Gui/BitmapFactory.h>

#include "ViewProviderSpline.h"
#include "SoFCShapeObject.h"


using namespace PartGui;
namespace sp = std::placeholders;


PROPERTY_SOURCE(PartGui::ViewProviderSpline, PartGui::ViewProviderPartExt)

ViewProviderSpline::ViewProviderSpline()
{
    sPixmap = "Part_Spline_Parametric";
    extension.initExtension(this);
}

ViewProviderSpline::~ViewProviderSpline() = default;

QIcon ViewProviderSpline::getIcon() const
{
    return Gui::BitmapFactory().pixmap(sPixmap);
}

// ----------------------------------------------------------------------------

EXTENSION_PROPERTY_SOURCE(PartGui::ViewProviderSplineExtension, Gui::ViewProviderExtension)


ViewProviderSplineExtension::ViewProviderSplineExtension()
{
    initExtensionType(ViewProviderSplineExtension::getExtensionClassTypeId());
    EXTENSION_ADD_PROPERTY(ControlPoints,(false));
}

void ViewProviderSplineExtension::toggleControlPoints(bool on)
{
    ControlPoints.setValue(on);
}

void ViewProviderSplineExtension::extensionSetupContextMenu(QMenu* menu, QObject*, const char*)
{
    // toggle command to display components
    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    QAction* act = menu->addAction(QObject::tr("Show control points"));
    act->setCheckable(true);
    act->setChecked(ControlPoints.getValue());
    func->toggle(act, [this](bool on) {
        this->toggleControlPoints(on);
    });
}

void ViewProviderSplineExtension::extensionUpdateData(const App::Property* prop)
{
    Gui::ViewProviderExtension::extensionUpdateData(prop);
    if (prop->getTypeId() == Part::PropertyPartShape::getClassTypeId() && strcmp(prop->getName(), "Shape") == 0) {
        // update control points if there
        if (pcControlPoints) {
            Gui::coinRemoveAllChildren(pcControlPoints);
            showControlPoints(this->ControlPoints.getValue(), prop);
        }
    }
}

void ViewProviderSplineExtension::extensionOnChanged(const App::Property* prop)
{
    if (prop == &ControlPoints) {
        App::DocumentObject* obj = getExtendedViewProvider()->getObject();
        App::Property* shape = obj->getPropertyByName("Shape");
        showControlPoints(ControlPoints.getValue(), shape);
    }
    else {
        Gui::ViewProviderExtension::extensionOnChanged(prop);
    }
}

void ViewProviderSplineExtension::showControlPoints(bool show, const App::Property* prop)
{
    if (!pcControlPoints && show) {
        pcControlPoints = new SoSwitch();
        SoSeparator* root = getExtendedViewProvider()->getRoot();
        root->addChild(pcControlPoints);
    }

    if (pcControlPoints) {
        pcControlPoints->whichChild = (show ? SO_SWITCH_ALL : SO_SWITCH_NONE);
    }

    if (!show || !pcControlPoints || pcControlPoints->getNumChildren() > 0)
        return;

    // ask for the property we are interested in
    if (prop && prop->getTypeId() == Part::PropertyPartShape::getClassTypeId()) {
        const TopoDS_Shape& shape = static_cast<const Part::PropertyPartShape*>(prop)->getValue();
        if (shape.IsNull())
            return; // empty shape

        for (TopExp_Explorer xp(shape, TopAbs_SHELL); xp.More(); xp.Next()) {
            const TopoDS_Shell& shell = TopoDS::Shell(xp.Current());
            for (TopExp_Explorer xp2(shell, TopAbs_FACE); xp2.More(); xp2.Next()) {
                const TopoDS_Face& face = TopoDS::Face(xp2.Current());
                showControlPointsOfFace(face);
            }
        }
        for (TopExp_Explorer xp(shape, TopAbs_FACE, TopAbs_SHELL); xp.More(); xp.Next()) {
            const TopoDS_Face& face = TopoDS::Face(xp.Current());
            showControlPointsOfFace(face);
        }
        for (TopExp_Explorer xp(shape, TopAbs_WIRE, TopAbs_FACE); xp.More(); xp.Next()) {
            const TopoDS_Wire& wire = TopoDS::Wire(xp.Current());
            for (TopExp_Explorer xp2(wire, TopAbs_EDGE); xp2.More(); xp2.Next()) {
                const TopoDS_Edge& edge = TopoDS::Edge(xp2.Current());
                showControlPointsOfEdge(edge);
            }
        }
        for (TopExp_Explorer xp(shape, TopAbs_EDGE, TopAbs_WIRE); xp.More(); xp.Next()) {
            const TopoDS_Edge& edge = TopoDS::Edge(xp.Current());
            showControlPointsOfEdge(edge);
        }
    }
}

void ViewProviderSplineExtension::showControlPointsOfEdge(const TopoDS_Edge& edge)
{
    std::list<gp_Pnt> poles, knots;
    Standard_Integer nCt=0;

    TopoDS_Edge edge_loc(edge);
    TopLoc_Location aLoc;
    edge_loc.Location(aLoc);

    BRepAdaptor_Curve curve(edge_loc);
    switch (curve.GetType())
    {
    case GeomAbs_BezierCurve:
        {
            Handle(Geom_BezierCurve) hBezier = curve.Bezier();
            nCt = hBezier->NbPoles();
            for (Standard_Integer i = 1; i <= nCt; i++)
                poles.push_back(hBezier->Pole(i));
            if (hBezier->IsClosed()) {
                nCt++;
                poles.push_back(hBezier->Pole(1));
            }
        }   break;
    case GeomAbs_BSplineCurve:
        {
            Handle(Geom_BSplineCurve) hBSpline = curve.BSpline();
            nCt = hBSpline->NbPoles();
            for (Standard_Integer i = 1; i <= nCt; i++)
                poles.push_back(hBSpline->Pole(i));
            if (hBSpline->IsClosed()) {
                nCt++;
                poles.push_back(hBSpline->Pole(1));
            }
            for (Standard_Integer i = hBSpline->FirstUKnotIndex()+1; i <= hBSpline->LastUKnotIndex()-1; i++)
                knots.push_back(hBSpline->Value(hBSpline->Knot(i)));
        }   break;
    default:
        break;
    }

    if (poles.empty())
        return; // nothing to do

    SoCoordinate3 * controlcoords = new SoCoordinate3;
    controlcoords->point.setNum(nCt + knots.size());

    int index=0;
    SbVec3f* verts = controlcoords->point.startEditing();
    for (const auto & pole : poles) {
        verts[index++].setValue((float)pole.X(), (float)pole.Y(), (float)pole.Z());
    }
    for (const auto & knot : knots) {
        verts[index++].setValue((float)knot.X(), (float)knot.Y(), (float)knot.Z());
    }
    controlcoords->point.finishEditing();


    SoFCControlPoints* controlpoints = new SoFCControlPoints();
    controlpoints->numPolesU = nCt;
    controlpoints->numPolesV = 1;

    SoSeparator* nodes = new SoSeparator();
    nodes->addChild(controlcoords);
    nodes->addChild(controlpoints);

    pcControlPoints->addChild(nodes);
}

void ViewProviderSplineExtension::showControlPointsOfFace(const TopoDS_Face& face)
{
    std::list<gp_Pnt> knots;
    std::vector<std::vector<gp_Pnt> > poles;
    Standard_Integer nCtU=0, nCtV=0;

    TopoDS_Face face_loc(face);
    TopLoc_Location aLoc;
    face_loc.Location(aLoc);

    BRepAdaptor_Surface surface(face_loc);
    switch (surface.GetType())
    {
    case GeomAbs_BezierSurface:
        {
            Handle(Geom_BezierSurface) hBezier = surface.Bezier();
            nCtU = hBezier->NbUPoles();
            nCtV = hBezier->NbVPoles();
            poles.resize(nCtU);
            for (Standard_Integer u = 1; u <= nCtU; u++) {
                poles[u-1].resize(nCtV);
                for (Standard_Integer v = 1; v <= nCtV; v++)
                    poles[u-1][v-1] = hBezier->Pole(u, v);
            }
        }   break;
    case GeomAbs_BSplineSurface:
        {
            Handle(Geom_BSplineSurface) hBSpline = surface.BSpline();
            nCtU = hBSpline->NbUPoles();
            nCtV = hBSpline->NbVPoles();
            poles.resize(nCtU);
            for (Standard_Integer u = 1; u <= nCtU; u++) {
                poles[u-1].resize(nCtV);
                for (Standard_Integer v = 1; v <= nCtV; v++)
                    poles[u-1][v-1] = hBSpline->Pole(u, v);
            }

            //Standard_Integer nKnU = hBSpline->NbUKnots();
            //Standard_Integer nKnV = hBSpline->NbVKnots();
            for (Standard_Integer u = 1; u <= hBSpline->NbUKnots(); u++) {
                for (Standard_Integer v = 1; v <= hBSpline->NbVKnots(); v++)
                    knots.push_back(hBSpline->Value(hBSpline->UKnot(u), hBSpline->VKnot(v)));
            }
        }   break;
    default:
        break;
    }

    if (poles.empty())
        return; // nothing to do

    SoCoordinate3 * coords = new SoCoordinate3;
    coords->point.setNum(nCtU * nCtV + knots.size());

    int index=0;
    SbVec3f* verts = coords->point.startEditing();
    for (const auto & pole : poles) {
        for (const auto& v : pole) {
            verts[index++].setValue((float)v.X(), (float)v.Y(), (float)v.Z());
        }
    }
    for (const auto & knot : knots) {
        verts[index++].setValue((float)knot.X(), (float)knot.Y(), (float)knot.Z());
    }
    coords->point.finishEditing();


    SoFCControlPoints* control = new SoFCControlPoints();
    control->numPolesU = nCtU;
    control->numPolesV = nCtV;

    //if (knots.size() > 0) {
    //    control->numKnotsU = nKnU;
    //    control->numKnotsV = nKnV;
    //}

    SoSeparator* nodes = new SoSeparator();
    nodes->addChild(coords);
    nodes->addChild(control);

    pcControlPoints->addChild(nodes);
}

namespace Gui {
    EXTENSION_PROPERTY_SOURCE_TEMPLATE(PartGui::ViewProviderSplineExtensionPython, PartGui::ViewProviderSplineExtension)

// explicit template instantiation
    template class PartGuiExport ViewProviderExtensionPythonT<PartGui::ViewProviderSplineExtension>;
}
