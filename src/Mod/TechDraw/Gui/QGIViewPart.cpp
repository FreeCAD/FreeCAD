/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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
#include <cmath>
#include <qmath.h>
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QGraphicsScene>
#include <QMenu>
#include <QMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QPainterPathStroker>
#include <QPainter>
#include <QPainterPath>
#include <QTextOption>
#include <QBitmap>
#include <QImage>
#include <QString>
#include <QSvgRenderer>
#endif // #ifndef _PreComp_

#include <chrono>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Vector3D.h>
#include <Gui/ViewProvider.h>

#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSection.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/DrawViewDetail.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/Cosmetic.h>
//#include <Mod/TechDraw/App/Preferences.h>

#include "Rez.h"
#include "ZVALUE.h"
#include "PreferencesGui.h"
#include "QGIFace.h"
#include "QGIEdge.h"
#include "QGIVertex.h"
#include "QGICMark.h"
#include "QGISectionLine.h"
#include "QGICenterLine.h"
#include "QGIHighlight.h"
#include "QGCustomBorder.h"
#include "QGCustomLabel.h"
#include "QGCustomRect.h"
#include "QGIMatting.h"
#include "QGIViewPart.h"
#include "ViewProviderGeomHatch.h"
#include "ViewProviderHatch.h"
#include "ViewProviderViewPart.h"
#include "MDIViewPage.h"

using namespace TechDraw;
using namespace TechDrawGui;
using namespace std;

#define GEOMETRYEDGE 0
#define COSMETICEDGE 1
#define CENTERLINE   2


const float lineScaleFactor = Rez::guiX(1.);   // temp fiddle for devel

QGIViewPart::QGIViewPart() :
    m_isExporting(false)
{
    setCacheMode(QGraphicsItem::NoCache);
    setHandlesChildEvents(false);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);

    showSection = false;
}

QGIViewPart::~QGIViewPart()
{
    tidy();
}

QVariant QGIViewPart::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        //There's nothing special for QGIVP to do when selection changes!
    } else if(change == ItemSceneChange && scene()) {
           tidy();
    }
    return QGIView::itemChange(change, value);
}

//obs?
void QGIViewPart::tidy()
{
    //Delete any leftover items
    for(QList<QGraphicsItem*>::iterator it = deleteItems.begin(); it != deleteItems.end(); ++it) {
        delete *it;
    }
    deleteItems.clear();
}

void QGIViewPart::setViewPartFeature(TechDraw::DrawViewPart *obj)
{
    if (!obj)
        return;

    setViewFeature(static_cast<TechDraw::DrawView *>(obj));
}

QPainterPath QGIViewPart::drawPainterPath(TechDraw::BaseGeom *baseGeom) const
{
    double rot = getViewObject()->Rotation.getValue();
    return geomToPainterPath(baseGeom,rot);
}


QPainterPath QGIViewPart::geomToPainterPath(TechDraw::BaseGeom *baseGeom, double rot)
{
    Q_UNUSED(rot);
    QPainterPath path;

    if (baseGeom == nullptr) {
        return path;
    }

    switch(baseGeom->geomType) {
        case CIRCLE: {
            TechDraw::Circle *geom = static_cast<TechDraw::Circle *>(baseGeom);

            double x = geom->center.x - geom->radius;
            double y = geom->center.y - geom->radius;

            path.addEllipse(Rez::guiX(x),
                            Rez::guiX(y),
                            Rez::guiX(geom->radius * 2),
                            Rez::guiX(geom->radius * 2));            //topleft@(x,y) radx,rady
            }
            break;
        case ARCOFCIRCLE:  {
            TechDraw::AOC  *geom = static_cast<TechDraw::AOC *>(baseGeom);
            if (baseGeom->reversed) {
                path.moveTo(Rez::guiX(geom->endPnt.x),
                            Rez::guiX(geom->endPnt.y));
                pathArc(path,
                        Rez::guiX(geom->radius),
                        Rez::guiX(geom->radius),
                        0.,
                        geom->largeArc,
                        !geom->cw,
                        Rez::guiX(geom->startPnt.x),
                        Rez::guiX(geom->startPnt.y),
                        Rez::guiX(geom->endPnt.x),
                        Rez::guiX(geom->endPnt.y));
            } else {
                path.moveTo(Rez::guiX(geom->startPnt.x),
                            Rez::guiX(geom->startPnt.y));
                pathArc(path,
                        Rez::guiX(geom->radius),
                        Rez::guiX(geom->radius),
                        0.,
                        geom->largeArc,
                        geom->cw,
                        Rez::guiX(geom->endPnt.x),
                        Rez::guiX(geom->endPnt.y),
                        Rez::guiX(geom->startPnt.x),
                        Rez::guiX(geom->startPnt.y));
            }
            }
            break;
        case TechDraw::ELLIPSE: {
            TechDraw::Ellipse *geom = static_cast<TechDraw::Ellipse *>(baseGeom);

            // Calculate start and end points as ellipse with theta = 0 and pi
            double startX = geom->center.x + geom->major * cos(geom->angle),
                   startY = geom->center.y + geom->major * sin(geom->angle),
                   endX = geom->center.x - geom->major * cos(geom->angle),
                   endY = geom->center.y - geom->major * sin(geom->angle);

            pathArc(path,
                    Rez::guiX(geom->major),
                    Rez::guiX(geom->minor),
                    geom->angle,
                    false,
                    false,
                    Rez::guiX(endX),
                    Rez::guiX(endY),
                    Rez::guiX(startX),
                    Rez::guiX(startY));

            pathArc(path,
                    Rez::guiX(geom->major),
                    Rez::guiX(geom->minor),
                    geom->angle,
                    false,
                    false,
                    Rez::guiX(startX),
                    Rez::guiX(startY),
                    Rez::guiX(endX),
                    Rez::guiX(endY));
            }
            break;
        case TechDraw::ARCOFELLIPSE: {
            TechDraw::AOE *geom = static_cast<TechDraw::AOE *>(baseGeom);
            if (baseGeom->reversed) {
                path.moveTo(Rez::guiX(geom->endPnt.x),
                            Rez::guiX(geom->endPnt.y));
                pathArc(path,
                        Rez::guiX(geom->major),
                        Rez::guiX(geom->minor),
                        geom->angle,
                        geom->largeArc,
                        !geom->cw,
                        Rez::guiX(geom->startPnt.x),
                        Rez::guiX(geom->startPnt.y),
                        Rez::guiX(geom->endPnt.x),
                        Rez::guiX(geom->endPnt.y));
            } else {
                path.moveTo(Rez::guiX(geom->startPnt.x),
                            Rez::guiX(geom->startPnt.y));
                pathArc(path,
                        Rez::guiX(geom->major),
                        Rez::guiX(geom->minor),
                        geom->angle,
                        geom->largeArc,
                        geom->cw,
                        Rez::guiX(geom->endPnt.x),
                        Rez::guiX(geom->endPnt.y),
                        Rez::guiX(geom->startPnt.x),
                        Rez::guiX(geom->startPnt.y));
            }
            }
            break;
        case TechDraw::BEZIER: {
            TechDraw::BezierSegment *geom = static_cast<TechDraw::BezierSegment *>(baseGeom);
            if (baseGeom->reversed) {
                if (!geom->pnts.empty()) {
                    Base::Vector3d rStart = geom->pnts.back();
                    path.moveTo(Rez::guiX(rStart.x), Rez::guiX(rStart.y));
                }
                if ( geom->poles == 2 ) {
                    // Degree 1 bezier = straight line...
                    path.lineTo(Rez::guiX(geom->pnts[0].x), Rez::guiX(geom->pnts[0].y));

                } else if ( geom->poles == 3 ) {
                    path.quadTo(Rez::guiX(geom->pnts[1].x), Rez::guiX(geom->pnts[1].y),
                                Rez::guiX(geom->pnts[0].x), Rez::guiX(geom->pnts[0].y));
                } else if ( geom->poles == 4 ) {
                    path.cubicTo(Rez::guiX(geom->pnts[2].x), Rez::guiX(geom->pnts[2].y),
                                 Rez::guiX(geom->pnts[1].x), Rez::guiX(geom->pnts[1].y),
                                 Rez::guiX(geom->pnts[0].x), Rez::guiX(geom->pnts[0].y));
                } else {                                                 //can only handle lines,quads,cubes
                    Base::Console().Error("Bad pole count (%d) for BezierSegment\n",geom->poles);
                    auto itBez = geom->pnts.begin() + 1;
                    for (; itBez != geom->pnts.end();itBez++)  {
                        path.lineTo(Rez::guiX((*itBez).x), Rez::guiX((*itBez).y));   //show something for debugging
                    }
                }
            } else {
                // Move painter to the beginning
                path.moveTo(Rez::guiX(geom->pnts[0].x), Rez::guiX(geom->pnts[0].y));

                if ( geom->poles == 2 ) {
                    // Degree 1 bezier = straight line...
                    path.lineTo(Rez::guiX(geom->pnts[1].x), Rez::guiX(geom->pnts[1].y));

                } else if ( geom->poles == 3 ) {
                    path.quadTo(Rez::guiX(geom->pnts[1].x), Rez::guiX(geom->pnts[1].y),
                                Rez::guiX(geom->pnts[2].x), Rez::guiX(geom->pnts[2].y));

                } else if ( geom->poles == 4 ) {
                    path.cubicTo(Rez::guiX(geom->pnts[1].x), Rez::guiX(geom->pnts[1].y),
                                 Rez::guiX(geom->pnts[2].x), Rez::guiX(geom->pnts[2].y),
                                 Rez::guiX(geom->pnts[3].x), Rez::guiX(geom->pnts[3].y));
                } else {                                            //can only handle lines,quads,cubes
                    Base::Console().Error("Bad pole count (%d) for BezierSegment\n",geom->poles);
                    auto itBez = geom->pnts.begin() + 1;
                    for (; itBez != geom->pnts.end();itBez++)  {
                      path.lineTo(Rez::guiX((*itBez).x), Rez::guiX((*itBez).y));         //show something for debugging
                    }
                }
            }
            }
            break;
        case TechDraw::BSPLINE: {
            TechDraw::BSpline *geom = static_cast<TechDraw::BSpline *>(baseGeom);
            if (baseGeom->reversed) {
            // Move painter to the end of our last segment
                std::vector<TechDraw::BezierSegment>::const_reverse_iterator it = geom->segments.rbegin();
                Base::Vector3d rStart = it->pnts.back();
                path.moveTo(Rez::guiX(rStart.x), Rez::guiX(rStart.y));

                for ( ; it != geom->segments.rend(); ++it) {
                    // At this point, the painter is either at the beginning
                    // of the first segment, or end of the last
                    if ( it->poles == 2 ) {
                        // Degree 1 bezier = straight line...
                        path.lineTo(Rez::guiX(it->pnts[0].x), Rez::guiX(it->pnts[0].y));

                    } else if ( it->poles == 3 ) {
                        path.quadTo(Rez::guiX(it->pnts[1].x), Rez::guiX(it->pnts[1].y),
                                    Rez::guiX(it->pnts[0].x), Rez::guiX(it->pnts[0].y));
                    } else if ( it->poles == 4 ) {
                        path.cubicTo(Rez::guiX(it->pnts[2].x), Rez::guiX(it->pnts[2].y),
                                     Rez::guiX(it->pnts[1].x), Rez::guiX(it->pnts[1].y),
                                     Rez::guiX(it->pnts[0].x), Rez::guiX(it->pnts[0].y));
                    } else {                                                 //can only handle lines,quads,cubes
                        Base::Console().Error("Bad pole count (%d) for BezierSegment of B-spline geometry\n",
                                              it->poles);
                        path.lineTo(it->pnts[1].x, it->pnts[1].y);         //show something for debugging
                    }
                }
            } else {
                // Move painter to the beginning of our first segment
                std::vector<TechDraw::BezierSegment>::const_iterator it = geom->segments.begin();
                path.moveTo(Rez::guiX(it->pnts[0].x), Rez::guiX(it->pnts[0].y));

                for ( ; it != geom->segments.end(); ++it) {
                    // At this point, the painter is either at the beginning
                    // of the first segment, or end of the last
                    if ( it->poles == 2 ) {
                        // Degree 1 bezier = straight line...
                        path.lineTo(Rez::guiX(it->pnts[1].x), Rez::guiX(it->pnts[1].y));
                    } else if ( it->poles == 3 ) {
                        path.quadTo(Rez::guiX(it->pnts[1].x), Rez::guiX(it->pnts[1].y),
                                    Rez::guiX(it->pnts[2].x), Rez::guiX(it->pnts[2].y));
                    } else if ( it->poles == 4 ) {
                        path.cubicTo(Rez::guiX(it->pnts[1].x), Rez::guiX(it->pnts[1].y),
                                     Rez::guiX(it->pnts[2].x), Rez::guiX(it->pnts[2].y),
                                     Rez::guiX(it->pnts[3].x), Rez::guiX(it->pnts[3].y));
                    } else {
                        Base::Console().Error("Bad pole count (%d) for BezierSegment of B-spline geometry\n",
                                              it->poles);
                        path.lineTo(it->pnts[1].x, it->pnts[1].y);         //show something for debugging
                    }
                } 
            } 
            }
            break;
        case TechDraw::GENERIC: {
            TechDraw::Generic *geom = static_cast<TechDraw::Generic *>(baseGeom);
            if (baseGeom->reversed) {
                if (!geom->points.empty()) {
                    Base::Vector3d rStart = geom->points.back();
                    path.moveTo(Rez::guiX(rStart.x), Rez::guiX(rStart.y));
                }
                std::vector<Base::Vector3d>::const_reverse_iterator it = geom->points.rbegin();
                for(++it; it != geom->points.rend(); ++it) {
                    path.lineTo(Rez::guiX((*it).x), Rez::guiX((*it).y));
                }
            } else {
               path.moveTo(Rez::guiX(geom->points[0].x), Rez::guiX(geom->points[0].y));
                std::vector<Base::Vector3d>::const_iterator it = geom->points.begin();
                for(++it; it != geom->points.end(); ++it) {
                    path.lineTo(Rez::guiX((*it).x), Rez::guiX((*it).y));
                }
            }
            }
            break;
        default: {
            Base::Console().Error("Error - geomToPainterPath - UNKNOWN geomType: %d\n",baseGeom->geomType);
            }
            break;
    } //sb end of switch

//old rotate path logic. now done on App side.
//    if (rot != 0.0) {
//        QTransform t;
//        t.rotate(-rot);
//        path = t.map(path);
//    }

    return path;
}

void QGIViewPart::updateView(bool update)
{
//    Base::Console().Message("QGIVP::updateView()\n");
    auto viewPart( dynamic_cast<TechDraw::DrawViewPart *>(getViewObject()) );
    if( viewPart == nullptr ) {
        return;
    }
    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    if (update ) {
        draw();
    }
    QGIView::updateView(update);
}

void QGIViewPart::draw() {
    if (!isVisible()) {
        return;
    }

    drawViewPart();
    drawMatting();
    //this is old C/L
    drawCenterLines(true);   //have to draw centerlines after border to get size correct.
    drawAllSectionLines();   //same for section lines
}

void QGIViewPart::drawViewPart()
{
    auto viewPart( dynamic_cast<TechDraw::DrawViewPart *>(getViewObject()) );
    if ( viewPart == nullptr ) {
        return;
    }
//    Base::Console().Message("QGIVP::DVP() - %s / %s\n", viewPart->getNameInDocument(), viewPart->Label.getValue());
    if (!viewPart->hasGeometry()) {
        removePrimitives();                      //clean the slate
        removeDecorations();
        return;
    }

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    float lineWidth = vp->LineWidth.getValue() * lineScaleFactor;
    float lineWidthHid = vp->HiddenWidth.getValue() * lineScaleFactor;
    float lineWidthIso = vp->IsoWidth.getValue() * lineScaleFactor;
//    float lineWidthExtra = viewPart->ExtraWidth.getValue() * lineScaleFactor;
    bool showAll = vp->ShowAllEdges.getValue();

    prepareGeometryChange();
    removePrimitives();                      //clean the slate
    removeDecorations();

#if MOD_TECHDRAW_HANDLE_FACES
    if (viewPart->handleFaces()) {
        // Draw Faces
        std::vector<TechDraw::DrawHatch*> hatchObjs = viewPart->getHatches();
        std::vector<TechDraw::DrawGeomHatch*> geomObjs = viewPart->getGeomHatches();
        const std::vector<TechDraw::FacePtr> &faceGeoms = viewPart->getFaceGeometry();
        std::vector<TechDraw::FacePtr>::const_iterator fit = faceGeoms.begin();
        for(int i = 0 ; fit != faceGeoms.end(); fit++, i++) {
            QGIFace* newFace = drawFace(*fit,i);
            newFace->isHatched(false);
            newFace->setFillMode(QGIFace::PlainFill);
//            newFace->setFill(QColor(Qt::red), Qt::SolidPattern);  //this overrides the QGIF defaults
            TechDraw::DrawHatch* fHatch = faceIsHatched(i,hatchObjs);
            TechDraw::DrawGeomHatch* fGeom = faceIsGeomHatched(i,geomObjs);
            if (fGeom) {
                const std::vector<std::string> &sourceNames = fGeom->Source.getSubValues();
                if (!sourceNames.empty()) {
                    std::vector<LineSet> lineSets = fGeom->getTrimmedLines(i);
                    if (!lineSets.empty()) {
                        newFace->clearLineSets();
                        for (auto& ls: lineSets) {
                            newFace->addLineSet(ls);
                        }
                        newFace->isHatched(true);
                        newFace->setFillMode(QGIFace::GeomHatchFill);
                        double hatchScale = fGeom->ScalePattern.getValue();
                        if (hatchScale > 0.0) {
                            newFace->setHatchScale(fGeom->ScalePattern.getValue());
                        }
                        newFace->setHatchFile(fGeom->PatIncluded.getValue());
                        Gui::ViewProvider* gvp = QGIView::getViewProvider(fGeom);
                        ViewProviderGeomHatch* geomVp = dynamic_cast<ViewProviderGeomHatch*>(gvp);
                        if (geomVp != nullptr) {
                            newFace->setHatchColor(geomVp->ColorPattern.getValue());
                            newFace->setLineWeight(geomVp->WeightPattern.getValue());
                        }
                    }
                }
            } else if (fHatch) {
                if (!fHatch->SvgIncluded.isEmpty()) {
                    if (getExporting()) {
                        newFace->hideSvg(true);
                    } else {
                        newFace->hideSvg(false);
                    }
                    newFace->isHatched(true);
                    newFace->setFillMode(QGIFace::SvgFill);
                    newFace->setHatchFile(fHatch->SvgIncluded.getValue());
                    Gui::ViewProvider* gvp = QGIView::getViewProvider(fHatch);
                    ViewProviderHatch* hatchVp = dynamic_cast<ViewProviderHatch*>(gvp);
                    if (hatchVp != nullptr) {
                        double hatchScale = hatchVp->HatchScale.getValue();
                        if (hatchScale > 0.0) {
                            newFace->setHatchScale(hatchVp->HatchScale.getValue());
                        }
                        newFace->setHatchColor(hatchVp->HatchColor.getValue());
                    }
                }
            }
            bool drawEdges = prefFaceEdges();
            newFace->setDrawEdges(drawEdges);                                        //pref. for debugging only
            newFace->setZValue(ZVALUE::FACE);
            newFace->setPrettyNormal();
            newFace->draw();
        }
    }
#endif //#if MOD_TECHDRAW_HANDLE_FACES

    // Draw Edges
    QColor edgeColor = PreferencesGui::normalQColor();

    const std::vector<TechDraw::BaseGeom *> &geoms = viewPart->getEdgeGeometry();
    std::vector<TechDraw::BaseGeom *>::const_iterator itGeom = geoms.begin();
    QGIEdge* item;
    for(int i = 0 ; itGeom != geoms.end(); itGeom++, i++) {
        bool showEdge = false;
        if ((*itGeom)->hlrVisible) {
            if (((*itGeom)->classOfEdge == ecHARD) ||
                ((*itGeom)->classOfEdge == ecOUTLINE) ||
                (((*itGeom)->classOfEdge == ecSMOOTH) && viewPart->SmoothVisible.getValue()) ||
                (((*itGeom)->classOfEdge == ecSEAM) && viewPart->SeamVisible.getValue())    ||
                (((*itGeom)->classOfEdge == ecUVISO) && viewPart->IsoVisible.getValue())) {
                showEdge = true;
            }
        } else {
            if ( (((*itGeom)->classOfEdge == ecHARD) && (viewPart->HardHidden.getValue())) ||
                 (((*itGeom)->classOfEdge == ecOUTLINE) && (viewPart->HardHidden.getValue())) ||
                 (((*itGeom)->classOfEdge == ecSMOOTH) && (viewPart->SmoothHidden.getValue())) ||
                 (((*itGeom)->classOfEdge == ecSEAM) && (viewPart->SeamHidden.getValue()))    ||
                 (((*itGeom)->classOfEdge == ecUVISO) && (viewPart->IsoHidden.getValue())) ) {
                showEdge = true;
            }
        }
        bool showItem = true;
        if (showEdge) {                     //based on hard/seam/hidden/etc
            item = new QGIEdge(i);
            item->setWidth(lineWidth);
            item->setNormalColor(edgeColor);
            item->setStyle(Qt::SolidLine);
            if ((*itGeom)->cosmetic == true) {
                int source = (*itGeom)->source();
                if (source == COSMETICEDGE) {
                    std::string cTag = (*itGeom)->getCosmeticTag();
                    showItem = formatGeomFromCosmetic(cTag, item);
                } else if (source == CENTERLINE) {
                    std::string cTag = (*itGeom)->getCosmeticTag();
                    showItem = formatGeomFromCenterLine(cTag, item);
                } else {
                    Base::Console().Message("QGIVP::drawVP - edge: %d is confused - source: %d\n",i,source);
                }
            } else {
                TechDraw::GeomFormat* gf = viewPart->getGeomFormatBySelection(i);
                if (gf != nullptr) {
                    item->setNormalColor(gf->m_format.m_color.asValue<QColor>());
                    item->setWidth(gf->m_format.m_weight * lineScaleFactor);
                    item->setStyle(gf->m_format.m_style);
                    showItem = gf->m_format.m_visible;
                }
            }

            addToGroup(item);                       //item is at scene(0,0), not group(0,0)
            item->setPos(0.0,0.0);                  //now at group(0,0)
            item->setPath(drawPainterPath(*itGeom));
            item->setZValue(ZVALUE::EDGE);
            if(!(*itGeom)->hlrVisible) {
                item->setWidth(lineWidthHid);
                item->setHiddenEdge(true);
                item->setZValue(ZVALUE::HIDEDGE);
            }
            if ((*itGeom)->classOfEdge == ecUVISO) {
                item->setWidth(lineWidthIso);
            }
            item->setPrettyNormal();
            if (!showAll) {             //view level "show" status
                if (!showItem) {        //individual edge "show" status
                    item->hide();
                }
            }
            //debug a path
//            QPainterPath edgePath=drawPainterPath(*itGeom);
//            std::stringstream edgeId;
//            edgeId << "QGIVP.edgePath" << i;
//            dumpPath(edgeId.str().c_str(),edgePath);
         }
    }


    // Draw Vertexs:
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    double vertexScaleFactor = hGrp->GetFloat("VertexScale", 3.0);
    QColor vertexColor = PreferencesGui::vertexQColor();

    bool showVertices = true;
    bool showCenterMarks = true;
    if (getFrameState()) {              //frames are on
        if (viewPart->CoarseView.getValue()) {
            showVertices = false;
        }
        if (!vp->ArcCenterMarks.getValue()) {
            showCenterMarks = false;
        }
    } else {                            //frames are off
        showVertices = false;
        if (!prefPrintCenters()) {             //based on preference (!frame && !pref)
            showCenterMarks = false;
        }
        if (!vp->ArcCenterMarks.getValue()) {  //based on property (!frame && !prop)
            showCenterMarks = false;
        }
    }

    const std::vector<TechDraw::VertexPtr> &verts = viewPart->getVertexGeometry();
    std::vector<TechDraw::VertexPtr>::const_iterator vert = verts.begin();
    double cAdjust = vp->CenterScale.getValue();

    for(int i = 0 ; vert != verts.end(); ++vert, i++) {
        if ((*vert)->isCenter) {
            if (showCenterMarks) {
                QGICMark* cmItem = new QGICMark(i);
                addToGroup(cmItem);
                cmItem->setPos(Rez::guiX((*vert)->pnt.x), Rez::guiX((*vert)->pnt.y));
                cmItem->setThick(0.5 * lineWidth);             //need minimum?
                cmItem->setSize( cAdjust * lineWidth * vertexScaleFactor);
                cmItem->setPrettyNormal();
                cmItem->setZValue(ZVALUE::VERTEX);
            }
        } else {        //regular Vertex
            if (showVertices) {
                QGIVertex *item = new QGIVertex(i);
                TechDraw::CosmeticVertex* cv = viewPart->getCosmeticVertexBySelection(i);
//                TechDraw::CosmeticVertex* cv = viewPart->getCosmeticVertexByGeom(i);
                if (cv != nullptr) {
                    item->setNormalColor(cv->color.asValue<QColor>());
                    item->setRadius(Rez::guiX(cv->size));
                } else {
                    item->setNormalColor(vertexColor);
                    item->setFillColor(vertexColor);
                    item->setRadius(lineWidth * vertexScaleFactor);
                }
                addToGroup(item);
                item->setPos(Rez::guiX((*vert)->point().x), Rez::guiX((*vert)->point().y));
                item->setPrettyNormal();
                item->setZValue(ZVALUE::VERTEX);
            }
        }
    }

    //draw detail highlights
    auto drefs = viewPart->getDetailRefs();
    for (auto& r:drefs) {
        drawHighlight(r, true);
    }
}

bool QGIViewPart::formatGeomFromCosmetic(std::string cTag, QGIEdge* item)
{
//    Base::Console().Message("QGIVP::formatGeomFromCosmetic(%s)\n", cTag.c_str());
    bool result = true;
    auto partFeat( dynamic_cast<TechDraw::DrawViewPart *>(getViewObject()) );
    TechDraw::CosmeticEdge* ce = partFeat ? partFeat->getCosmeticEdge(cTag) : nullptr;
    if (ce != nullptr) {
        item->setNormalColor(ce->m_format.m_color.asValue<QColor>());
        item->setWidth(ce->m_format.m_weight * lineScaleFactor);
        item->setStyle(ce->m_format.m_style);
        result = ce->m_format.m_visible;
    }
    return result;
}


bool QGIViewPart::formatGeomFromCenterLine(std::string cTag, QGIEdge* item)
{
//    Base::Console().Message("QGIVP::formatGeomFromCenterLine(%d)\n",sourceIndex);
    bool result = true;
    auto partFeat( dynamic_cast<TechDraw::DrawViewPart *>(getViewObject()) );
    TechDraw::CenterLine* cl = partFeat ? partFeat->getCenterLine(cTag) : nullptr;
    if (cl != nullptr) {
        item->setNormalColor(cl->m_format.m_color.asValue<QColor>());
        item->setWidth(cl->m_format.m_weight * lineScaleFactor);
        item->setStyle(cl->m_format.m_style);
        result = cl->m_format.m_visible;
    }
    return result;
}

QGIFace* QGIViewPart::drawFace(TechDraw::FacePtr f, int idx)
{
//    Base::Console().Message("QGIVP::drawFace - %d\n", idx);
    std::vector<TechDraw::Wire *> fWires = f->wires;
    QPainterPath facePath;
    for(std::vector<TechDraw::Wire *>::iterator wire = fWires.begin(); wire != fWires.end(); ++wire) {
        std::vector<TechDraw::BaseGeom*> geoms = (*wire)->geoms;
        if (geoms.empty())
            continue;

        TechDraw::BaseGeom* firstGeom = geoms.front();
        QPainterPath wirePath;
        //QPointF startPoint(firstGeom->getStartPoint().x, firstGeom->getStartPoint().y);
        //wirePath.moveTo(startPoint);
        QPainterPath firstSeg = drawPainterPath(firstGeom);
        wirePath.connectPath(firstSeg);
        for(std::vector<TechDraw::BaseGeom *>::iterator edge = ((*wire)->geoms.begin()) + 1; edge != (*wire)->geoms.end(); ++edge) {
            QPainterPath edgePath = drawPainterPath(*edge);
            //handle section faces differently
            if (idx == -1) {
                    QPointF wEnd = wirePath.currentPosition();
                    auto element = edgePath.elementAt(0);
                    QPointF eStart(element.x, element.y);
                    QPointF eEnd = edgePath.currentPosition();
                    QPointF sVec = wEnd - eStart;
                    QPointF eVec = wEnd - eEnd;
                    double sDist2 = sVec.x() * sVec.x() + sVec.y() * sVec.y();
                    double eDist2 = eVec.x() * eVec.x() + eVec.y() * eVec.y();
                    if (sDist2 > eDist2) {
                        edgePath = edgePath.toReversed();
                    }
           }
            wirePath.connectPath(edgePath);
        }
//        dumpPath("wirePath:",wirePath);
        facePath.addPath(wirePath);
    }
    facePath.setFillRule(Qt::OddEvenFill);

    QGIFace* gFace = new QGIFace(idx);
    addToGroup(gFace);
    gFace->setPos(0.0,0.0);
    gFace->setOutline(facePath);
    //debug a path
    //std::stringstream faceId;
    //faceId << "facePath " << idx;
    //dumpPath(faceId.str().c_str(),facePath);

    return gFace;
}

//! Remove all existing QGIPrimPath items(Vertex,Edge,Face)
//note this triggers scene selectionChanged signal if vertex/edge/face is selected
void QGIViewPart::removePrimitives()
{
    QList<QGraphicsItem*> children = childItems();
    MDIViewPage* mdi = getMDIViewPage();
    if (mdi != nullptr) {
        getMDIViewPage()->blockSceneSelection(true);
    }
    for (auto& c:children) {
         QGIPrimPath* prim = dynamic_cast<QGIPrimPath*>(c);
         if (prim) {
            prim->hide();
            scene()->removeItem(prim);
            delete prim;
         }
     }
    if (mdi != nullptr) {
        getMDIViewPage()->blockSceneSelection(false);
    }
}

//! Remove all existing QGIDecoration items(SectionLine,SectionMark,...)
void QGIViewPart::removeDecorations()
{
    QList<QGraphicsItem*> children = childItems();
    for (auto& c:children) {
         QGIDecoration* decor = dynamic_cast<QGIDecoration*>(c);
         QGIMatting* mat = dynamic_cast<QGIMatting*>(c);
         if (decor) {
            decor->hide();
            scene()->removeItem(decor);
            delete decor;
         } else if (mat) {
            mat->hide();
            scene()->removeItem(mat);
            delete mat;
         }

     }
}

void QGIViewPart::drawAllSectionLines(void)
{
    TechDraw::DrawViewPart *viewPart = static_cast<TechDraw::DrawViewPart *>(getViewObject());
    if (!viewPart)  {
        return;
    }

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }
    if (vp->ShowSectionLine.getValue()) {
        auto refs = viewPart->getSectionRefs();
        for (auto& r:refs) {
            drawSectionLine(r, true);
        }
    }
}

void QGIViewPart::drawSectionLine(TechDraw::DrawViewSection* viewSection, bool b)
{
    TechDraw::DrawViewPart *viewPart = static_cast<TechDraw::DrawViewPart *>(getViewObject());
    if (!viewPart)  {
        return;
    }
    if (viewSection == nullptr) {
        return;
    }

    if (!viewSection->hasGeometry()) {
        return;
    }

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    if (b) {
        QGISectionLine* sectionLine = new QGISectionLine();
        addToGroup(sectionLine);
        sectionLine->setSymbol(const_cast<char*>(viewSection->SectionSymbol.getValue()));
        sectionLine->setSectionStyle(vp->SectionLineStyle.getValue());
        sectionLine->setSectionColor(vp->SectionLineColor.getValue().asValue<QColor>());

        //find the ends of the section line
        double scale = viewPart->getScale();
        std::pair<Base::Vector3d, Base::Vector3d> sLineEnds = viewSection->sectionLineEnds();
        Base::Vector3d l1 = Rez::guiX(sLineEnds.first) * scale;
        Base::Vector3d l2 = Rez::guiX(sLineEnds.second) * scale;

        //which way to the arrows point?
        Base::Vector3d lineDir = l2 - l1;
        lineDir.Normalize();
        Base::Vector3d normalDir = viewSection->SectionNormal.getValue();
        Base::Vector3d projNormal = viewPart->projectPoint(normalDir);
        projNormal.Normalize();
        Base::Vector3d arrowDir = viewSection->SectionNormal.getValue();
        arrowDir = - viewPart->projectPoint(arrowDir);  //arrows point reverse of sectionNormal(extrusion dir)
        sectionLine->setDirection(arrowDir.x, -arrowDir.y);           //invert Y

        //make the section line a little longer
        double fudge = Rez::guiX(2.0 * Preferences::dimFontSizeMM());
        sectionLine->setEnds(l1 - lineDir * fudge, 
                             l2 + lineDir * fudge);

        //set the general parameters
        sectionLine->setPos(0.0, 0.0);
        sectionLine->setWidth(Rez::guiX(vp->LineWidth.getValue()));
        double fontSize = Preferences::dimFontSizeMM();
        sectionLine->setFont(m_font, fontSize);
        sectionLine->setZValue(ZVALUE::SECTIONLINE);
        sectionLine->setRotation(- viewPart->Rotation.getValue());
        sectionLine->draw();
    }
}

//TODO: use Cosmetic::CenterLine object for this to make it usable for dims.
void QGIViewPart::drawCenterLines(bool b)
{
    TechDraw::DrawViewPart *viewPart = dynamic_cast<TechDraw::DrawViewPart *>(getViewObject());
    if (!viewPart)  {
        return;
    }

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    if (b) {
        bool horiz = vp->HorizCenterLine.getValue();
        bool vert = vp->VertCenterLine.getValue();

        QGICenterLine* centerLine;
        double sectionSpan;
        double sectionFudge = Rez::guiX(10.0);
        double xVal, yVal;
        if (horiz)  {
            centerLine = new QGICenterLine();
            addToGroup(centerLine);
            centerLine->setPos(0.0,0.0);
            //this should work from the viewPart's bbox, not the border
//            double scale = viewPart->getScale();
            double width = Rez::guiX(viewPart->getBoxX());
            sectionSpan = width + sectionFudge;
//            sectionSpan = m_border->rect().width() + sectionFudge;
            xVal = sectionSpan / 2.0;
            yVal = 0.0;
            centerLine->setIntersection(horiz && vert);
            centerLine->setBounds(-xVal,-yVal,xVal,yVal);
            centerLine->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));
            centerLine->setZValue(ZVALUE::SECTIONLINE);
//            centerLine->setRotation(viewPart->Rotation.getValue());
            centerLine->draw();
        }
        if (vert) {
            centerLine = new QGICenterLine();
            addToGroup(centerLine);
            centerLine->setPos(0.0,0.0);
//            double scale = viewPart->getScale();
            double height = Rez::guiX(viewPart->getBoxY());
            sectionSpan = height + sectionFudge;
//            sectionSpan = (m_border->rect().height() - m_label->boundingRect().height()) + sectionFudge;
            xVal = 0.0;
            yVal = sectionSpan / 2.0;
            centerLine->setIntersection(horiz && vert);
            centerLine->setBounds(-xVal,-yVal,xVal,yVal);
            centerLine->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));
            centerLine->setZValue(ZVALUE::SECTIONLINE);
//            centerLine->setRotation(viewPart->Rotation.getValue());
            centerLine->draw();
        }
    }
}

void QGIViewPart::drawHighlight(TechDraw::DrawViewDetail* viewDetail, bool b)
{
    TechDraw::DrawViewPart *viewPart = static_cast<TechDraw::DrawViewPart *>(getViewObject());
    if (!viewPart ||
        !viewDetail)  {
        return;
    }

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    if (b) {
//        double fontSize = getPrefFontSize();
        double fontSize = Preferences::labelFontSizeMM();
        QGIHighlight* highlight = new QGIHighlight();
        addToGroup(highlight);
        highlight->setPos(0.0,0.0);   //sb setPos(center.x,center.y)?
        highlight->setReference(const_cast<char*>(viewDetail->Reference.getValue()));
        highlight->setStyle((Qt::PenStyle)vp->HighlightLineStyle.getValue());
        highlight->setColor(vp->HighlightLineColor.getValue().asValue<QColor>());

        Base::Vector3d center = viewDetail->AnchorPoint.getValue() * viewPart->getScale();

        double radius = viewDetail->Radius.getValue() * viewPart->getScale();
        highlight->setBounds(center.x - radius, center.y + radius,center.x + radius, center.y - radius);
        highlight->setWidth(Rez::guiX(vp->IsoWidth.getValue()));
        highlight->setFont(m_font, fontSize);
        highlight->setZValue(ZVALUE::HIGHLIGHT);

        QPointF rotCenter = highlight->mapFromParent(transformOriginPoint());
        highlight->setTransformOriginPoint(rotCenter);

        double rotation = viewPart->Rotation.getValue() + 
                          vp->HighlightAdjust.getValue();
        highlight->setRotation(rotation);
        highlight->draw();
    }
}

void QGIViewPart::drawMatting()
{
    auto viewPart( dynamic_cast<TechDraw::DrawViewPart *>(getViewObject()) );
    TechDraw::DrawViewDetail* dvd = nullptr;
    if (viewPart && viewPart->isDerivedFrom(TechDraw::DrawViewDetail::getClassTypeId())) {
        dvd = static_cast<TechDraw::DrawViewDetail*>(viewPart);
    } else {
        return;
    }

    double scale = dvd->getScale();
    double radius = dvd->Radius.getValue() * scale;
    QGIMatting* mat = new QGIMatting();
    addToGroup(mat);
    mat->setRadius(Rez::guiX(radius));
    mat->setPos(0.0,0.0);
    mat->draw();
    mat->show();
}

// As called by arc of ellipse case:
// pathArc(path, geom->major, geom->minor, geom->angle, geom->largeArc, geom->cw,
//         geom->endPnt.x, geom->endPnt.y,
//         geom->startPnt.x, geom->startPnt.y);
void QGIViewPart::pathArc(QPainterPath &path, double rx, double ry, double x_axis_rotation,
                                    bool large_arc_flag, bool sweep_flag,
                                    double x, double y,
                                    double curx, double cury)
{
    double sin_th, cos_th;
    double a00, a01, a10, a11;
    double x0, y0, x1, y1, xc, yc;
    double d, sfactor, sfactor_sq;
    double th0, th1, th_arc;
    int i, n_segs;
    double dx, dy, dx1, dy1, Pr1, Pr2, Px, Py, check;

    rx = qAbs(rx);
    ry = qAbs(ry);

    sin_th = qSin(x_axis_rotation);
    cos_th = qCos(x_axis_rotation);

    dx = (curx - x) / 2.0;
    dy = (cury - y) / 2.0;
    dx1 =  cos_th * dx + sin_th * dy;
    dy1 = -sin_th * dx + cos_th * dy;
    Pr1 = rx * rx;
    Pr2 = ry * ry;
    Px = dx1 * dx1;
    Py = dy1 * dy1;
    /* Spec : check if radii are large enough */
    check = Px / Pr1 + Py / Pr2;
    if (check > 1) {
        rx = rx * qSqrt(check);
        ry = ry * qSqrt(check);
    }

    a00 =  cos_th / rx;
    a01 =  sin_th / rx;
    a10 = -sin_th / ry;
    a11 =  cos_th / ry;
    x0 = a00 * curx + a01 * cury;
    y0 = a10 * curx + a11 * cury;
    x1 = a00 * x + a01 * y;
    y1 = a10 * x + a11 * y;
    /* (x0, y0) is current point in transformed coordinate space.
       (x1, y1) is new point in transformed coordinate space.

       The arc fits a unit-radius circle in this space.
    */
    d = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    sfactor_sq = 1.0 / d - 0.25;
    if (sfactor_sq < 0)
        sfactor_sq = 0;

    sfactor = qSqrt(sfactor_sq);

    if (sweep_flag == large_arc_flag)
        sfactor = -sfactor;

    xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0);
    yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0);
    /* (xc, yc) is center of the circle. */

    th0 = qAtan2(y0 - yc, x0 - xc);
    th1 = qAtan2(y1 - yc, x1 - xc);

    th_arc = th1 - th0;
    if (th_arc < 0 && sweep_flag)
        th_arc += 2 * M_PI;
    else if (th_arc > 0 && !sweep_flag)
        th_arc -= 2 * M_PI;

    n_segs = qCeil(qAbs(th_arc / (M_PI * 0.5 + 0.001)));

    path.moveTo(curx, cury);

    for (i = 0; i < n_segs; i++) {
        pathArcSegment(path, xc, yc,
                       th0 + i * th_arc / n_segs,
                       th0 + (i + 1) * th_arc / n_segs,
                       rx, ry, x_axis_rotation);
    }
}

void QGIViewPart::pathArcSegment(QPainterPath &path,
                                           double xc, double yc,
                                           double th0, double th1,
                                           double rx, double ry, double xAxisRotation)
{
    double sinTh, cosTh;
    double a00, a01, a10, a11;
    double x1, y1, x2, y2, x3, y3;
    double t;
    double thHalf;

    sinTh = qSin(xAxisRotation);
    cosTh = qCos(xAxisRotation);

    a00 =  cosTh * rx;
    a01 = -sinTh * ry;
    a10 =  sinTh * rx;
    a11 =  cosTh * ry;

    thHalf = 0.5 * (th1 - th0);
    t = (8.0 / 3.0) * qSin(thHalf * 0.5) * qSin(thHalf * 0.5) / qSin(thHalf);
    x1 = xc + qCos(th0) - t * qSin(th0);
    y1 = yc + qSin(th0) + t * qCos(th0);
    x3 = xc + qCos(th1);
    y3 = yc + qSin(th1);
    x2 = x3 + t * qSin(th1);
    y2 = y3 - t * qCos(th1);

    path.cubicTo(a00 * x1 + a01 * y1, a10 * x1 + a11 * y1,
                 a00 * x2 + a01 * y2, a10 * x2 + a11 * y2,
                 a00 * x3 + a01 * y3, a10 * x3 + a11 * y3);
}

void QGIViewPart::toggleCache(bool state)
{
  QList<QGraphicsItem*> items = childItems();
    for(QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); it++) {
        //(*it)->setCacheMode((state)? DeviceCoordinateCache : NoCache);        //TODO: fiddle cache settings if req'd for performance
        Q_UNUSED(state);
        (*it)->setCacheMode(NoCache);
        (*it)->update();
    }
}

void QGIViewPart::toggleCosmeticLines(bool state)
{
  QList<QGraphicsItem*> items = childItems();
    for(QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); it++) {
        QGIEdge *edge = dynamic_cast<QGIEdge *>(*it);
        if(edge) {
            edge->setCosmetic(state);
        }
    }
}

//get hatchObj for face i if it exists
TechDraw::DrawHatch* QGIViewPart::faceIsHatched(int i,std::vector<TechDraw::DrawHatch*> hatchObjs) const
{
    TechDraw::DrawHatch* result = nullptr;
    bool found = false;
    for (auto& h:hatchObjs) {
        const std::vector<std::string> &sourceNames = h->Source.getSubValues();
        for (auto& s: sourceNames) {
            int fdx = TechDraw::DrawUtil::getIndexFromName(s);
            if (fdx == i) {
                result = h;
                found = true;
                break;
            }
        }
        if (found) {
            break;
        }
    }
    return result;
}

TechDraw::DrawGeomHatch* QGIViewPart::faceIsGeomHatched(int i,std::vector<TechDraw::DrawGeomHatch*> geomObjs) const
{
    TechDraw::DrawGeomHatch* result = nullptr;
    bool found = false;
    for (auto& h:geomObjs) {
        const std::vector<std::string> &sourceNames = h->Source.getSubValues();
        for (auto& sn: sourceNames) {
            int fdx = TechDraw::DrawUtil::getIndexFromName(sn);
            if (fdx == i) {
                result = h;
                found = true;
                break;
            }
            if (found) {
                break;
            }
        }
    }
    return result;
}


void QGIViewPart::dumpPath(const char* text,QPainterPath path)
{
        QPainterPath::Element elem;
        Base::Console().Message(">>>%s has %d elements\n",text,path.elementCount());
        char* typeName;
        for(int iElem = 0; iElem < path.elementCount(); iElem++) {
            elem = path.elementAt(iElem);
            if(elem.isMoveTo()) {
                typeName = "MoveTo";
            } else if (elem.isLineTo()) {
                typeName = "LineTo";
            } else if (elem.isCurveTo()) {
                typeName = "CurveTo";
            } else {
                typeName = "CurveData";
            }
            Base::Console().Message(">>>>> element %d: type:%d/%s pos(%.3f,%.3f) M:%d L:%d C:%d\n",
                                    iElem, elem.type, typeName, elem.x, elem.y, elem.isMoveTo(), elem.isLineTo(),
                                     elem.isCurveTo());
        }
}

QRectF QGIViewPart::boundingRect() const
{
//    return childrenBoundingRect();
//    return customChildrenBoundingRect();
    return QGIView::boundingRect();
}
void QGIViewPart::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->drawRect(boundingRect());          //good for debugging

    QGIView::paint (painter, &myOption, widget);
}

//QGIViewPart derived classes do not need a rotate view method as rotation is handled on App side.
void QGIViewPart::rotateView(void)
{
}

bool QGIViewPart::prefFaceEdges(void)
{
    bool result = false;
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    result = hGrp->GetBool("DrawFaceEdges", 0l);
    return result;
}

bool QGIViewPart::prefPrintCenters(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    bool   printCenters = hGrp->GetBool("PrintCenterMarks", false);   //true matches v0.18 behaviour
    return printCenters;
}
