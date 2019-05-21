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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QMenu>
# include <QPointer>
# include <QStatusBar>
# include <QTimer>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/details/SoLineDetail.h>
# include <Inventor/details/SoPointDetail.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoPointSet.h>
# include <Inventor/nodes/SoSeparator.h>
# include <BRepBuilderAPI_MakePolygon.hxx>
#endif

#include "CurveOnMesh.h"
#include <App/Document.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Grid.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/Projection.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/Gui/ViewProvider.h>
#include <Mod/Part/App/PartFeature.h>

#ifndef HAVE_ACOSH
#define HAVE_ACOSH
#endif
#ifndef HAVE_ASINH
#define HAVE_ASINH
#endif
#ifndef HAVE_ATANH
#define HAVE_ATANH
#endif

#include <gp_Pnt.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Standard_Failure.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <Poly_Polygon3D.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>

/* XPM */
static const char *cursor_curveonmesh[]={
"32 32 3 1",
"+ c white",
"# c red",
". c None",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"................................",
"+++++...+++++...................",
"................................",
"......+...............###.......",
"......+...............#.#.......",
"......+...............###.......",
"......+..............#..#.......",
"......+.............#....#......",
"....................#.+..#......",
"..................+#+..+..#...+.",
"................++#.....+.#..+..",
"......+........+..#......++#+...",
".......+......+..#.........#....",
"........++..++..#..........###..",
"..........++....#..........#.#..",
"......#........#...........###..",
".......#......#.................",
"........#.....#.................",
".........#...#..................",
"..........###...................",
"..........#.#...................",
"..........###...................",
"................................",
"................................",
"................................",
"................................",
"................................"};

using namespace MeshPartGui;

PROPERTY_SOURCE(MeshPartGui::ViewProviderCurveOnMesh, Gui::ViewProviderDocumentObject)

ViewProviderCurveOnMesh::ViewProviderCurveOnMesh()
{
    // the lines
    pcCoords = new SoCoordinate3;
    pcCoords->ref();
    pcCoords->point.setNum(0);

    pcLinesStyle = new SoDrawStyle;
    pcLinesStyle->style = SoDrawStyle::LINES;
    pcLinesStyle->lineWidth = 3;
    pcLinesStyle->ref();

    SoGroup* pcLineRoot = new SoSeparator();
    pcLineRoot->addChild(pcLinesStyle);
    SoBaseColor * linecol = new SoBaseColor;
    linecol->rgb.setValue(1.0f, 1.0f, 0.0f);
    pcLineRoot->addChild(linecol);
    pcLineRoot->addChild(pcCoords);
    pcLineRoot->addChild(new SoLineSet);

    // the nodes
    pcNodes = new SoCoordinate3;
    pcNodes->ref();
    pcNodes->point.setNum(0);

    pcPointStyle = new SoDrawStyle;
    pcPointStyle->style = SoDrawStyle::POINTS;
    pcPointStyle->pointSize = 15;
    pcPointStyle->ref();

    SoGroup* pcPointRoot = new SoSeparator();
    pcPointRoot->addChild(pcPointStyle);
    SoBaseColor * pointcol = new SoBaseColor;
    pointcol->rgb.setValue(1.0f, 0.5f, 0.0f);
    pcPointRoot->addChild(pointcol);
    pcPointRoot->addChild(pcNodes);
    pcPointRoot->addChild(new SoPointSet);

    SoGroup* group = new SoGroup;
    group->addChild(pcLineRoot);
    group->addChild(pcPointRoot);
    addDisplayMaskMode(group, "Point");
}

ViewProviderCurveOnMesh::~ViewProviderCurveOnMesh()
{
    pcCoords->unref();
    pcLinesStyle->unref();
    pcNodes->unref();
    pcPointStyle->unref();
}

void ViewProviderCurveOnMesh::setDisplayMode(const char* ModeName)
{
    setDisplayMaskMode(ModeName);
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

void ViewProviderCurveOnMesh::addVertex(const SbVec3f& v)
{
    int num = pcNodes->point.getNum();
    pcNodes->point.set1Value(num, v);
}

void ViewProviderCurveOnMesh::clearVertex()
{
    pcNodes->point.setNum(0);
}

void ViewProviderCurveOnMesh::setPoints(const std::vector<SbVec3f>& pts)
{
    pcCoords->point.setNum(pts.size());
    SbVec3f* coords = pcCoords->point.startEditing();
    int index = 0;
    for (std::vector<SbVec3f>::const_iterator it = pts.begin(); it != pts.end(); ++it) {
        coords[index] = *it;
        index++;
    }
    pcCoords->point.finishEditing();
}

void ViewProviderCurveOnMesh::clearPoints()
{
    pcCoords->point.setNum(0);
}

// ------------------------------------------------------------------

class CurveOnMeshHandler::Private
{
public:
    struct PickedPoint
    {
        unsigned long facet;
        SbVec3f point;
        SbVec3f normal;
    };

    struct ApproxPar
    {
        double weight1;
        double weight2;
        double weight3;
        double tol3d;
        int maxDegree;
        GeomAbs_Shape cont;

        ApproxPar() {
            weight1 = 0.2;
            weight2 = 0.4;
            weight3 = 0.2;
            tol3d = 1.0e-2;
            maxDegree = 5;
            cont = GeomAbs_C2;
        }
    };
    Private()
        : wireClosed(false)
        , distance(1.0)
        , cosAngle(0.7071) // 45 degree
        , approximate(true)
        , curve(new ViewProviderCurveOnMesh)
        , mesh(0)
        , grid(0)
        , viewer(0)
        , editcursor(QPixmap(cursor_curveonmesh), 7, 7)
    {
    }
    ~Private()
    {
        delete curve;
        delete grid;
    }
    static void vertexCallback(void * ud, SoEventCallback * n);
    std::vector<SbVec3f> convert(const std::vector<Base::Vector3f>& points) const
    {
        std::vector<SbVec3f> pts;
        pts.reserve(points.size());
        for (auto it = points.begin(); it != points.end(); ++it) {
            pts.push_back(Base::convertTo<SbVec3f>(*it));
        }
        return pts;
    }
    void createGrid()
    {
        Mesh::Feature* mf = static_cast<Mesh::Feature*>(mesh->getObject());
        const Mesh::MeshObject& meshObject = mf->Mesh.getValue();
        kernel = meshObject.getKernel();
        kernel.Transform(meshObject.getTransform());

        MeshCore::MeshAlgorithm alg(kernel);
        float fAvgLen = alg.GetAverageEdgeLength();
        grid = new MeshCore::MeshFacetGrid(kernel, 5.0f * fAvgLen);
    }
    bool projectLineOnMesh(const PickedPoint& pick)
    {
        PickedPoint last = pickedPoints.back();
        std::vector<Base::Vector3f> polyline;

        MeshCore::MeshProjection meshProjection(kernel);
        Base::Vector3f v1 = Base::convertTo<Base::Vector3f>(last.point);
        Base::Vector3f v2 = Base::convertTo<Base::Vector3f>(pick.point);
        Base::Vector3f vd = Base::convertTo<Base::Vector3f>(viewer->getViewer()->getViewDirection());
        if (meshProjection.projectLineOnMesh(*grid, v1, last.facet, v2, pick.facet, vd, polyline)) {
            if (polyline.size() > 1) {
                if (cutLines.empty()) {
                    cutLines.push_back(polyline);
                }
                else {
                    SbVec3f dir1;
                    SbVec3f dir2 = pick.point - last.point;
                    dir2.normalize();
                    std::size_t num = pickedPoints.size();
                    if (num >= 2) {
                        dir1 = pickedPoints[num-1].point - pickedPoints[num-2].point;
                        dir1.normalize();
                    }

                    // if the angle between two line segments is greater than the angle
                    // split the curve in this position
                    if (dir1.dot(dir2) < cosAngle) {
                        cutLines.push_back(polyline);
                    }
                    else {
                        std::vector<Base::Vector3f>& segm = cutLines.back();
                        segm.insert(segm.end(), polyline.begin()+1, polyline.end());
                    }
                }

                return true;
            }
        }

        return false;
    }

    std::vector<PickedPoint> pickedPoints;
    std::list<std::vector<Base::Vector3f> > cutLines;
    bool wireClosed;
    double distance;
    double cosAngle;
    bool approximate;
    ViewProviderCurveOnMesh* curve;
    Gui::ViewProviderDocumentObject* mesh;
    MeshCore::MeshFacetGrid* grid;
    MeshCore::MeshKernel kernel;
    QPointer<Gui::View3DInventor> viewer;
    QCursor editcursor;
    ApproxPar par;
};

CurveOnMeshHandler::CurveOnMeshHandler(QObject* parent)
  : QObject(parent), d_ptr(new Private)
{
}

CurveOnMeshHandler::~CurveOnMeshHandler()
{
    disableCallback();
}

void CurveOnMeshHandler::enableApproximation(bool on)
{
    d_ptr->approximate = on;
}

void CurveOnMeshHandler::setParameters(int maxDegree, GeomAbs_Shape cont, double tol3d, double angle)
{
    d_ptr->par.maxDegree = maxDegree;
    d_ptr->par.cont = cont;
    d_ptr->par.tol3d = tol3d;
    d_ptr->cosAngle = cos(angle);
}

void CurveOnMeshHandler::onContextMenu()
{
    QMenu menu;
    menu.addAction(tr("Create"), this, SLOT(onCreate()));
    if (!d_ptr->wireClosed && d_ptr->pickedPoints.size() >= 3) {
        menu.addAction(tr("Close wire"), this, SLOT(onCloseWire()));
    }
    menu.addAction(tr("Clear"), this, SLOT(onClear()));
    menu.addAction(tr("Cancel"), this, SLOT(onCancel()));
    menu.exec(QCursor::pos());
}

void CurveOnMeshHandler::onCreate()
{
    for (auto it = d_ptr->cutLines.begin(); it != d_ptr->cutLines.end(); ++it) {
        std::vector<SbVec3f> segm = d_ptr->convert(*it);
        if (d_ptr->approximate) {
            Handle(Geom_BSplineCurve) spline = approximateSpline(segm);
            if (!spline.IsNull())
                displaySpline(spline);
        }
        else {
            TopoDS_Wire wire;
            if (makePolyline(segm, wire))
                displayPolyline(wire);
        }
    }

    d_ptr->curve->clearVertex();
    d_ptr->curve->clearPoints();

    d_ptr->pickedPoints.clear();
    d_ptr->cutLines.clear();
    d_ptr->wireClosed = false;

    disableCallback();
}

void CurveOnMeshHandler::onCloseWire()
{
    if (d_ptr->wireClosed || d_ptr->pickedPoints.size() < 3) {
        return;
    }

    closeWire();
}

void CurveOnMeshHandler::onClear()
{
    d_ptr->curve->clearVertex();
    d_ptr->curve->clearPoints();

    d_ptr->pickedPoints.clear();
    d_ptr->cutLines.clear();
    d_ptr->wireClosed = false;
}

void CurveOnMeshHandler::onCancel()
{
    d_ptr->curve->clearVertex();
    d_ptr->curve->clearPoints();

    d_ptr->pickedPoints.clear();
    d_ptr->cutLines.clear();
    d_ptr->wireClosed = false;

    disableCallback();
}

void CurveOnMeshHandler::enableCallback(Gui::View3DInventor* v)
{
    if (v && !d_ptr->viewer) {
        d_ptr->viewer = v;
        Gui::View3DInventorViewer* view3d = d_ptr->viewer->getViewer();
        view3d->addEventCallback(SoEvent::getClassTypeId(), Private::vertexCallback, this);
        view3d->addViewProvider(d_ptr->curve);
        view3d->setEditing(true);

        view3d->setEditingCursor(d_ptr->editcursor);

        d_ptr->curve->setDisplayMode("Point");
    }
}

void CurveOnMeshHandler::disableCallback()
{
    if (d_ptr->viewer) {
        Gui::View3DInventorViewer* view3d = d_ptr->viewer->getViewer();
        view3d->setEditing(false);
        view3d->removeViewProvider(d_ptr->curve);
        view3d->removeEventCallback(SoEvent::getClassTypeId(), Private::vertexCallback, this);
    }
    d_ptr->viewer = 0;
}

std::vector<SbVec3f> CurveOnMeshHandler::getVertexes() const
{
    std::vector<SbVec3f> pts;
    pts.reserve(d_ptr->pickedPoints.size());
    for (std::vector<Private::PickedPoint>::const_iterator it = d_ptr->pickedPoints.begin(); it != d_ptr->pickedPoints.end(); ++it)
        pts.push_back(it->point);
    return pts;
}

std::vector<SbVec3f> CurveOnMeshHandler::getPoints() const
{
    std::vector<SbVec3f> pts;
    for (auto it = d_ptr->cutLines.begin(); it != d_ptr->cutLines.end(); ++it) {
        std::vector<SbVec3f> segm = d_ptr->convert(*it);
        pts.insert(pts.end(), segm.begin(), segm.end());
    }
    return pts;
}

Handle(Geom_BSplineCurve) CurveOnMeshHandler::approximateSpline(const std::vector<SbVec3f>& points)
{
    TColgp_Array1OfPnt pnts(1,points.size());
    Standard_Integer index = 1;
    for (std::vector<SbVec3f>::const_iterator it = points.begin(); it != points.end(); ++it) {
        float x,y,z;
        it->getValue(x,y,z);
        pnts(index++) = gp_Pnt(x,y,z);
    }

    try {
        //GeomAPI_PointsToBSpline fit(pnts, 1, 2, GeomAbs_C0, 1.0e-3);
        //GeomAPI_PointsToBSpline fit(pnts, d_ptr->par.weight1, d_ptr->par.weight2, d_ptr->par.weight3,
        //                            d_ptr->par.maxDegree, d_ptr->par.cont, d_ptr->par.tol3d);
        GeomAPI_PointsToBSpline fit(pnts, 1, d_ptr->par.maxDegree, d_ptr->par.cont, d_ptr->par.tol3d);
        Handle(Geom_BSplineCurve) spline = fit.Curve();
        return spline;
    }
    catch (...) {
        return Handle(Geom_BSplineCurve)();
    }
}

void CurveOnMeshHandler::approximateEdge(const TopoDS_Edge& edge, double tolerance)
{
    BRepMesh_IncrementalMesh(edge, tolerance);
    TopLoc_Location loc;
    Handle(Poly_Polygon3D) aPoly = BRep_Tool::Polygon3D(edge, loc);
    if (!aPoly.IsNull()) {
        int numNodes = aPoly->NbNodes();
        const TColgp_Array1OfPnt& aNodes = aPoly->Nodes();
        std::vector<SbVec3f> pts;
        pts.reserve(numNodes);
        for (int i=aNodes.Lower(); i<=aNodes.Upper(); i++) {
            const gp_Pnt& p = aNodes.Value(i);
            pts.push_back(SbVec3f(static_cast<float>(p.X()),
                                  static_cast<float>(p.Y()),
                                  static_cast<float>(p.Z())));
        }

        d_ptr->curve->setPoints(pts);
    }
}

void CurveOnMeshHandler::displaySpline(const Handle(Geom_BSplineCurve)& spline)
{
    if (d_ptr->viewer) {
        double u = spline->FirstParameter();
        double v = spline->LastParameter();
        BRepBuilderAPI_MakeEdge mkBuilder(spline, u, v);
        TopoDS_Edge edge = mkBuilder.Edge();

        Gui::View3DInventorViewer* view3d = d_ptr->viewer->getViewer();
        App::Document* doc = view3d->getDocument()->getDocument();
        doc->openTransaction("Add spline");
        Part::Feature* part = static_cast<Part::Feature*>(doc->addObject("Part::Spline", "Spline"));
        part->Shape.setValue(edge);
        doc->commitTransaction();
    }
}

bool CurveOnMeshHandler::makePolyline(const std::vector<SbVec3f>& points, TopoDS_Wire& wire)
{
    BRepBuilderAPI_MakePolygon mkPoly;
    for (std::vector<SbVec3f>::const_iterator it = points.begin(); it != points.end(); ++it) {
        float x,y,z;
        it->getValue(x,y,z);
        mkPoly.Add(gp_Pnt(x,y,z));
    }

    if (mkPoly.IsDone()) {
        wire = mkPoly.Wire();
        return true;
    }

    return false;
}

void CurveOnMeshHandler::displayPolyline(const TopoDS_Wire& wire)
{
    if (d_ptr->viewer) {
        Gui::View3DInventorViewer* view3d = d_ptr->viewer->getViewer();
        App::Document* doc = view3d->getDocument()->getDocument();
        doc->openTransaction("Add polyline");
        Part::Feature* part = static_cast<Part::Feature*>(doc->addObject("Part::Feature", "Polyline"));
        part->Shape.setValue(wire);
        doc->commitTransaction();
    }
}

bool CurveOnMeshHandler::tryCloseWire(const SbVec3f& p) const
{
    if (d_ptr->pickedPoints.size() >= 3) {
        Private::PickedPoint first = d_ptr->pickedPoints.front();
        // if the distance of the first and last points is small enough (~1mm)
        // the curve can be closed.
        float len = (first.point - p).length();
        if (len < d_ptr->distance) {
            return true;
        }
    }

    return false;
}

void CurveOnMeshHandler::closeWire()
{
    Private::PickedPoint pick = d_ptr->pickedPoints.front();
    if (d_ptr->projectLineOnMesh(pick)) {
        d_ptr->curve->setPoints(getPoints());
        d_ptr->wireClosed = true;
    }
}

void CurveOnMeshHandler::Private::vertexCallback(void * ud, SoEventCallback * n)
{
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());
    const SoEvent* ev = n->getEvent();
    if (ev->getTypeId() == SoMouseButtonEvent::getClassTypeId()) {
        // set as handled
        n->setHandled();

        const SoMouseButtonEvent * mbe = static_cast<const SoMouseButtonEvent *>(ev);
        if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::DOWN) {
            const SoPickedPoint * pp = n->getPickedPoint();
            if (pp) {
                CurveOnMeshHandler* self = static_cast<CurveOnMeshHandler*>(ud);
                if (!self->d_ptr->wireClosed) {
                    Gui::ViewProvider* vp = static_cast<Gui::ViewProvider*>(view->getViewProviderByPath(pp->getPath()));
                    if (vp && vp->getTypeId().isDerivedFrom(MeshGui::ViewProviderMesh::getClassTypeId())) {
                        MeshGui::ViewProviderMesh* mesh = static_cast<MeshGui::ViewProviderMesh*>(vp);
                        const SoDetail* detail = pp->getDetail();
                        if (detail && detail->getTypeId() == SoFaceDetail::getClassTypeId()) {
                            // get the mesh and build a grid
                            if (!self->d_ptr->mesh) {
                                self->d_ptr->mesh = mesh;
                                self->d_ptr->createGrid();
                            }
                            else if (self->d_ptr->mesh != mesh) {
                                Gui::getMainWindow()->statusBar()->showMessage(
                                    tr("Wrong mesh picked"));
                                return;
                            }

                            const SbVec3f& p = pp->getPoint();
                            const SbVec3f& n = pp->getNormal();

                            Private::PickedPoint pick;
                            pick.facet = static_cast<const SoFaceDetail*>(detail)->getFaceIndex();
                            pick.point = p;
                            pick.normal = n;

                            if (self->d_ptr->pickedPoints.empty()) {
                                self->d_ptr->pickedPoints.push_back(pick);
                                self->d_ptr->curve->addVertex(p);
                            }
                            else {
                                // check to auto-complete the curve
                                if (self->tryCloseWire(p)) {
                                    self->closeWire();
                                }
                                else if (self->d_ptr->projectLineOnMesh(pick)) {
                                    self->d_ptr->curve->setPoints(self->getPoints());
                                    self->d_ptr->pickedPoints.push_back(pick);
                                    self->d_ptr->curve->addVertex(p);
                                }
                            }
                        }
                    }
                    // try to 'complete' the curve
                    else if (vp && vp->getTypeId().isDerivedFrom(ViewProviderCurveOnMesh::getClassTypeId())) {
                        const SbVec3f& p = pp->getPoint();
                        if (self->tryCloseWire(p)) {
                            self->closeWire();
                        }
                    }
                }
            }
            else {
                Gui::getMainWindow()->statusBar()->showMessage(
                    tr("No point was picked"));
            }
        }
        else if (mbe->getButton() == SoMouseButtonEvent::BUTTON2 && mbe->getState() == SoButtonEvent::UP) {
            CurveOnMeshHandler* self = static_cast<CurveOnMeshHandler*>(ud);
            QTimer::singleShot(100, self, SLOT(onContextMenu()));
        }
    }
}

void CurveOnMeshHandler::recomputeDocument()
{
    if (d_ptr->viewer) {
        Gui::View3DInventorViewer* view3d = d_ptr->viewer->getViewer();
        App::Document* doc = view3d->getDocument()->getDocument();
        doc->recompute();
    }
}

#include "moc_CurveOnMesh.cpp"
