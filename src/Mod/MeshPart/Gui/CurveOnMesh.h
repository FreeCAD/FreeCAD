/***************************************************************************
 *   Copyright (c) 2017 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef MESHPARTGUI_CURVEONMESH_H
#define MESHPARTGUI_CURVEONMESH_H

#include <GeomAbs_Shape.hxx>
#include <Geom_BSplineCurve.hxx>
#include <QObject>
#include <memory>

#include <Gui/ViewProviderDocumentObject.h>


class SbVec3f;
class SoCoordinate3;
class SoDrawStyle;
class TopoDS_Edge;
class TopoDS_Wire;

namespace Gui
{
class View3DInventor;
class ViewProvider;
}  // namespace Gui

namespace MeshPartGui
{

class ViewProviderCurveOnMesh: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshPartGui::ViewProviderCurveOnMesh);

public:
    ViewProviderCurveOnMesh();
    ~ViewProviderCurveOnMesh() override;
    void addVertex(const SbVec3f&);
    void clearVertex();
    void setPoints(const std::vector<SbVec3f>&);
    void clearPoints();
    void setDisplayMode(const char* ModeName) override;

private:
    SoCoordinate3* pcCoords;
    SoCoordinate3* pcNodes;
    SoDrawStyle* pcPointStyle;
    SoDrawStyle* pcLinesStyle;
};

class CurveOnMeshHandler: public QObject
{
    Q_OBJECT

public:
    explicit CurveOnMeshHandler(QObject* parent = nullptr);
    ~CurveOnMeshHandler() override;
    void enableApproximation(bool);
    void setParameters(int maxDegree, GeomAbs_Shape cont, double tol3d, double angle);
    void enableCallback(Gui::View3DInventor* viewer);
    void disableCallback();
    void recomputeDocument();

private:
    Handle(Geom_BSplineCurve) approximateSpline(const std::vector<SbVec3f>& points);
    void approximateEdge(const TopoDS_Edge&, double tolerance);
    void displaySpline(const Handle(Geom_BSplineCurve) &);
    bool makePolyline(const std::vector<SbVec3f>& points, TopoDS_Wire& wire);
    void displayPolyline(const TopoDS_Wire& wire);
    std::vector<SbVec3f> getPoints() const;
    std::vector<SbVec3f> getVertexes() const;
    void closeWire();
    bool tryCloseWire(const SbVec3f&) const;

private Q_SLOTS:
    void onContextMenu();
    void onCreate();
    void onClear();
    void onCancel();
    void onCloseWire();

private:
    class Private;
    std::unique_ptr<Private> d_ptr;
};

}  // namespace MeshPartGui

#endif  // MESHPARTGUI_CURVEONMESH_H
