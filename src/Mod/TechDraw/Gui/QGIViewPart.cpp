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
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/Cosmetic.h>

#include "Rez.h"
#include "ZVALUE.h"
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
using namespace TechDrawGeometry;

const float lineScaleFactor = Rez::guiX(1.);   // temp fiddle for devel

QGIViewPart::QGIViewPart()
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

QPainterPath QGIViewPart::drawPainterPath(TechDrawGeometry::BaseGeom *baseGeom) const
{
    double rot = getViewObject()->Rotation.getValue();
    return geomToPainterPath(baseGeom,rot);
}


QPainterPath QGIViewPart::geomToPainterPath(TechDrawGeometry::BaseGeom *baseGeom, double rot)
{
    Q_UNUSED(rot);
    QPainterPath path;

    switch(baseGeom->geomType) {
        case TechDrawGeometry::CIRCLE: {
          TechDrawGeometry::Circle *geom = static_cast<TechDrawGeometry::Circle *>(baseGeom);

          double x = geom->center.x - geom->radius;
          double y = geom->center.y - geom->radius;

          path.addEllipse(Rez::guiX(x),
                          Rez::guiX(y),
                          Rez::guiX(geom->radius * 2),
                          Rez::guiX(geom->radius * 2));            //topleft@(x,y) radx,rady
        } break;
        case TechDrawGeometry::ARCOFCIRCLE: {
          TechDrawGeometry::AOC  *geom = static_cast<TechDrawGeometry::AOC *>(baseGeom);

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
        } break;
        case TechDrawGeometry::ELLIPSE: {
          TechDrawGeometry::Ellipse *geom = static_cast<TechDrawGeometry::Ellipse *>(baseGeom);

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

        } break;
        case TechDrawGeometry::ARCOFELLIPSE: {
          TechDrawGeometry::AOE *geom = static_cast<TechDrawGeometry::AOE *>(baseGeom);

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

        } break;
        case TechDrawGeometry::BEZIER: {
          TechDrawGeometry::BezierSegment *geom = static_cast<TechDrawGeometry::BezierSegment *>(baseGeom);

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
          } else {                                                 //can only handle lines,quads,cubes
              Base::Console().Error("Bad pole count (%d) for BezierSegment\n",geom->poles);
              auto itBez = geom->pnts.begin() + 1;
              for (; itBez != geom->pnts.end();itBez++)  {
                path.lineTo(Rez::guiX((*itBez).x), Rez::guiX((*itBez).y));         //show something for debugging
              }
          }
        } break;
        case TechDrawGeometry::BSPLINE: {
          TechDrawGeometry::BSpline *geom = static_cast<TechDrawGeometry::BSpline *>(baseGeom);

          std::vector<TechDrawGeometry::BezierSegment>::const_iterator it = geom->segments.begin();

          // Move painter to the beginning of our first segment
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
              } else {                                                 //can only handle lines,quads,cubes
                  Base::Console().Error("Bad pole count (%d) for BezierSegment of B-spline geometry\n",it->poles);
                  path.lineTo(it->pnts[1].x, it->pnts[1].y);         //show something for debugging
              }
          }
        } break;
        case TechDrawGeometry::GENERIC: {
          TechDrawGeometry::Generic *geom = static_cast<TechDrawGeometry::Generic *>(baseGeom);

          path.moveTo(Rez::guiX(geom->points[0].x), Rez::guiX(geom->points[0].y));
          std::vector<Base::Vector2d>::const_iterator it = geom->points.begin();
          for(++it; it != geom->points.end(); ++it) {
              path.lineTo(Rez::guiX((*it).x), Rez::guiX((*it).y));
          }
        } break;
        default:
          Base::Console().Error("Error - geomToPainterPath - UNKNOWN geomType: %d\n",baseGeom->geomType);
          break;
      }

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
    auto start = std::chrono::high_resolution_clock::now();
    auto viewPart( dynamic_cast<TechDraw::DrawViewPart *>(getViewObject()) );
    if( viewPart == nullptr ) {
        return;
    }
    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    QGIView::updateView(update);

    if (update ) {
        draw();
    }

    auto end   = std::chrono::high_resolution_clock::now();
    auto diff  = end - start;
    double diffOut = std::chrono::duration <double, std::milli> (diff).count();
    Base::Console().Log("TIMING - QGIVP::updateView - %s - total %.3f millisecs\n",getViewName(),diffOut);
}

void QGIViewPart::draw() {
    if (!isVisible()) {
        return;
    }

    drawViewPart();
    drawMatting();
    QGIView::draw();
    drawCenterLines(true);   //have to draw centerlines after border to get size correct.
    drawAllSectionLines();   //same for section lines
}

void QGIViewPart::drawViewPart()
{
    auto viewPart( dynamic_cast<TechDraw::DrawViewPart *>(getViewObject()) );
    if ( viewPart == nullptr ) {
        return;
    }
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

    prepareGeometryChange();
    removePrimitives();                      //clean the slate
    removeDecorations();

#if MOD_TECHDRAW_HANDLE_FACES
    if (viewPart->handleFaces()) {
        // Draw Faces
        std::vector<TechDraw::DrawHatch*> hatchObjs = viewPart->getHatches();
        std::vector<TechDraw::DrawGeomHatch*> geomObjs = viewPart->getGeomHatches();
        const std::vector<TechDrawGeometry::Face *> &faceGeoms = viewPart->getFaceGeometry();
        std::vector<TechDrawGeometry::Face *>::const_iterator fit = faceGeoms.begin();
        for(int i = 0 ; fit != faceGeoms.end(); fit++, i++) {
            QGIFace* newFace = drawFace(*fit,i);
            newFace->isHatched(false);
            newFace->setFillMode(QGIFace::PlainFill);
            TechDraw::DrawHatch* fHatch = faceIsHatched(i,hatchObjs);
            TechDraw::DrawGeomHatch* fGeom = faceIsGeomHatched(i,geomObjs);
            if (fGeom) {
                const std::vector<std::string> &sourceNames = fGeom->Source.getSubValues();
                if (!sourceNames.empty()) {
                    int fdx = TechDraw::DrawUtil::getIndexFromName(sourceNames.at(0));
                    std::vector<LineSet> lineSets = fGeom->getTrimmedLines(fdx);
                    if (!lineSets.empty()) {
                        newFace->clearLineSets();
                        for (auto& ls: lineSets) {
                            newFace->addLineSet(ls);
                        }
                        newFace->isHatched(true);
                        newFace->setFillMode(QGIFace::GeomHatchFill);
                        newFace->setHatchScale(fGeom->ScalePattern.getValue());
                        newFace->setHatchFile(fGeom->FilePattern.getValue());
                        Gui::ViewProvider* gvp = QGIView::getViewProvider(fGeom);
                        ViewProviderGeomHatch* geomVp = dynamic_cast<ViewProviderGeomHatch*>(gvp);
                        if (geomVp != nullptr) {
                            newFace->setHatchColor(geomVp->ColorPattern.getValue());
                            newFace->setLineWeight(geomVp->WeightPattern.getValue());
                        }
                    }
                }
            } else if (fHatch) {
                if (!fHatch->HatchPattern.isEmpty()) {
                    newFace->isHatched(true);
                    newFace->setFillMode(QGIFace::FromFile);
                    newFace->setHatchFile(fHatch->HatchPattern.getValue());
                    Gui::ViewProvider* gvp = QGIView::getViewProvider(fHatch);
                    ViewProviderHatch* hatchVp = dynamic_cast<ViewProviderHatch*>(gvp);
                    if (hatchVp != nullptr) {
                        newFace->setHatchScale(hatchVp->HatchScale.getValue());
                        newFace->setHatchColor(hatchVp->HatchColor.getValue());
                    }
                }
            }
            bool drawEdges = getFaceEdgesPref();
            newFace->setDrawEdges(drawEdges);                                        //pref. for debugging only
            newFace->setZValue(ZVALUE::FACE);
            newFace->draw();
            newFace->setPrettyNormal();
        }
    }
#endif //#if MOD_TECHDRAW_HANDLE_FACES

    // Draw Edges
    const std::vector<TechDrawGeometry::BaseGeom *> &geoms = viewPart->getEdgeGeometry();
    std::vector<TechDrawGeometry::BaseGeom *>::const_iterator itEdge = geoms.begin();
    QGIEdge* item;
    for(int i = 0 ; itEdge != geoms.end(); itEdge++, i++) {
        bool showEdge = false;
        if ((*itEdge)->visible) {
            if (((*itEdge)->classOfEdge == ecHARD) ||
                ((*itEdge)->classOfEdge == ecOUTLINE) ||
                (((*itEdge)->classOfEdge == ecSMOOTH) && viewPart->SmoothVisible.getValue()) ||
                (((*itEdge)->classOfEdge == ecSEAM) && viewPart->SeamVisible.getValue())    ||
                (((*itEdge)->classOfEdge == ecUVISO) && viewPart->IsoVisible.getValue())) {
                showEdge = true;
            }
        } else {
            if ( (((*itEdge)->classOfEdge == ecHARD) && (viewPart->HardHidden.getValue())) ||
                 (((*itEdge)->classOfEdge == ecOUTLINE) && (viewPart->HardHidden.getValue())) ||
                 (((*itEdge)->classOfEdge == ecSMOOTH) && (viewPart->SmoothHidden.getValue())) ||
                 (((*itEdge)->classOfEdge == ecSEAM) && (viewPart->SeamHidden.getValue()))    ||
                 (((*itEdge)->classOfEdge == ecUVISO) && (viewPart->IsoHidden.getValue())) ) {
                showEdge = true;
            }
        }
        if (showEdge) {
            item = new QGIEdge(i);
            addToGroup(item);                         //item is at scene(0,0), not group(0,0)
            item->setPos(0.0,0.0);                    //now at group(0,0)
            item->setPath(drawPainterPath(*itEdge));
            item->setWidth(lineWidth);
            item->setZValue(ZVALUE::EDGE);
            if(!(*itEdge)->visible) {
                item->setWidth(lineWidthHid);
                item->setHiddenEdge(true);
                item->setZValue(ZVALUE::HIDEDGE);
            }
            if ((*itEdge)->classOfEdge == ecUVISO) {
                item->setWidth(lineWidthIso);
            }
            item->setPrettyNormal();
            //debug a path
//            QPainterPath edgePath=drawPainterPath(*itEdge);
//            std::stringstream edgeId;
//            edgeId << "QGIVP.edgePath" << i;
//            dumpPath(edgeId.str().c_str(),edgePath);
         }
    }
    // Draw Cosmetic Edges
//    int cosmoEdgeStart = 1000;
//    const std::vector<TechDrawGreometry::CosmeticEdge *> &cEdges = viewPart->getEdgeCosmetic();
//    std::vector<TechDrawGreometry::CosmeticEdge *>::const_iterator itcEdge = cEdges.begin();
//    QGIEdge* item;
//    for(int i = 0 ; itcEdge != cEdges.end(); itcEdge++, i++) {
//            item = new QGIEdge(cosmoEdgeStart + i);
//            addToGroup(item);
//            item->setPos(0.0,0.0);
////            item->setPath(drawPainterPath(*itcEdge));  //this won't work
//            item->setWidth((*itcEdge)->width);
//            item->setColor((*itcEdge)->color.asValue<QColor>();
//            item->setZValue(ZVALUE::EDGE);
//            item->setPrettyNormal();
//    
//    }


    // Draw Vertexs:
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    double vertexScaleFactor = hGrp->GetFloat("VertexScale", 3.0);
    hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("VertexColor", 0x00000000));
    QColor vertexColor = fcColor.asValue<QColor>();

    if (getFrameState()) {
        bool usePolygonHLR = viewPart->CoarseView.getValue();
        const std::vector<TechDrawGeometry::Vertex *> &verts = viewPart->getVertexGeometry();
        std::vector<TechDrawGeometry::Vertex *>::const_iterator vert = verts.begin();
        bool showCenters = vp->ArcCenterMarks.getValue();
        double cAdjust = vp->CenterScale.getValue();
        for(int i = 0 ; vert != verts.end(); ++vert, i++) {
            if ((*vert)->isCenter) {
                if (showCenters) {
                    QGICMark* cmItem = new QGICMark(i);
                    addToGroup(cmItem);
                    cmItem->setPos(Rez::guiX((*vert)->pnt.x), Rez::guiX((*vert)->pnt.y));
                    cmItem->setThick(0.5 * lineWidth);             //need minimum?
                    cmItem->setSize( cAdjust * lineWidth * vertexScaleFactor);
                    cmItem->setZValue(ZVALUE::VERTEX);
                }
            } else if(!usePolygonHLR){ //Disable dots WHEN usePolygonHLR
                QGIVertex *item = new QGIVertex(i);
                TechDraw::CosmeticVertex* cv = viewPart->getCosmeticVertexByLink(i);
                if (cv != nullptr) {
                    item->setNormalColor(cv->color.asValue<QColor>());
                    item->setRadius(cv->size);
                } else {
                    item->setNormalColor(vertexColor);
                    item->setRadius(lineWidth * vertexScaleFactor);
                }
                item->setPrettyNormal();
                addToGroup(item);
                item->setPos(Rez::guiX((*vert)->pnt.x), Rez::guiX((*vert)->pnt.y));
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

QGIFace* QGIViewPart::drawFace(TechDrawGeometry::Face* f, int idx)
{
    std::vector<TechDrawGeometry::Wire *> fWires = f->wires;
    QPainterPath facePath;
    for(std::vector<TechDrawGeometry::Wire *>::iterator wire = fWires.begin(); wire != fWires.end(); ++wire) {
        QPainterPath wirePath;
        for(std::vector<TechDrawGeometry::BaseGeom *>::iterator edge = (*wire)->geoms.begin(); edge != (*wire)->geoms.end(); ++edge) {
            //Save the start Position
            QPainterPath edgePath = drawPainterPath(*edge);
            // If the current end point matches the shape end point the new edge path needs reversing
            QPointF shapePos = (wirePath.currentPosition()- edgePath.currentPosition());
            if(sqrt(shapePos.x() * shapePos.x() + shapePos.y()*shapePos.y()) < 0.05) {    //magic tolerance
                edgePath = edgePath.toReversed();
            }
            wirePath.connectPath(edgePath);
        }
        //dumpPath("wirePath:",wirePath);
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
        getMDIViewPage()->blockSelection(true);
    }
    for (auto& c:children) {
         QGIPrimPath* prim = dynamic_cast<QGIPrimPath*>(c);
         if (prim) {
            removeFromGroup(prim);
            scene()->removeItem(prim);
            delete prim;
         }
     }
    if (mdi != nullptr) {
        getMDIViewPage()->blockSelection(false);
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
            removeFromGroup(decor);
            scene()->removeItem(decor);
            delete decor;
         } else if (mat) {
            removeFromGroup(mat);
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

        //TODO: handle oblique section lines?
        //find smallest internal angle(normalDir,get?Dir()) and use -1*get?Dir() +/- angle
        //Base::Vector3d normalDir = viewSection->SectionNormal.getValue();
        Base::Vector3d arrowDir(0,1,0);                //for drawing only, not geom
        Base::Vector3d lineDir(1,0,0);
        bool horiz = false;

        //this is a hack we can use since we don't support oblique section lines yet.
        //better solution will be need if oblique is ever implemented
        double rot = viewPart->Rotation.getValue();
        bool switchWH = false;
        if (TechDraw::DrawUtil::fpCompare(fabs(rot), 90.0)) {
            switchWH = true;
        }

        if (viewSection->SectionDirection.isValue("Right")) {
            arrowDir = Base::Vector3d(1,0,0);
            lineDir = Base::Vector3d(0,1,0);
        } else if (viewSection->SectionDirection.isValue("Left")) {
            arrowDir = Base::Vector3d(-1,0,0);
            lineDir = Base::Vector3d(0,-1,0);
        } else if (viewSection->SectionDirection.isValue("Up")) {
            arrowDir = Base::Vector3d(0,1,0);
            lineDir = Base::Vector3d(1,0,0);
            horiz = true;
        } else if (viewSection->SectionDirection.isValue("Down")) {
            arrowDir = Base::Vector3d(0,-1,0);
            lineDir = Base::Vector3d(-1,0,0);
            horiz = true;
        }
        sectionLine->setDirection(arrowDir.x,arrowDir.y);

        Base::Vector3d org = viewSection->SectionOrigin.getValue();
        double scale = viewPart->getScale();
        Base::Vector3d pOrg = scale * viewPart->projectPoint(org);
        //now project pOrg onto arrowDir
        Base::Vector3d displace;
        displace.ProjectToLine(pOrg, arrowDir);
        Base::Vector3d offset = pOrg + displace;

        sectionLine->setPos(Rez::guiX(offset.x),Rez::guiX(offset.y));
        double sectionSpan;
        double sectionFudge = Rez::guiX(10.0);
        double xVal, yVal;
//        double fontSize = getPrefFontSize();
        double fontSize = getDimFontSize();
        if (horiz)  {
            double width = Rez::guiX(viewPart->getBoxX());
            double height = Rez::guiX(viewPart->getBoxY());
            if (switchWH) {
                sectionSpan = height + sectionFudge;
            } else {
                sectionSpan = width + sectionFudge;
            }
            xVal = sectionSpan / 2.0;
            yVal = 0.0;
        } else {
            double width = Rez::guiX(viewPart->getBoxX());
            double height = Rez::guiX(viewPart->getBoxY());
            if (switchWH) {
                sectionSpan = width + sectionFudge;
            } else {
                sectionSpan = height + sectionFudge;
            }
//            sectionSpan = (m_border->rect().height() - m_label->boundingRect().height()) + sectionFudge;
            xVal = 0.0;
            yVal = sectionSpan / 2.0;
        }
        sectionLine->setBounds(-xVal,-yVal,xVal,yVal);
        sectionLine->setWidth(Rez::guiX(vp->LineWidth.getValue()));
        sectionLine->setFont(m_font, fontSize);
        sectionLine->setZValue(ZVALUE::SECTIONLINE);
        sectionLine->setRotation(viewPart->Rotation.getValue());
        sectionLine->draw();
    }
}

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
        double fontSize = getPrefFontSize();
        QGIHighlight* highlight = new QGIHighlight();
        addToGroup(highlight);
        highlight->setPos(0.0,0.0);   //sb setPos(center.x,center.y)?
        highlight->setReference(const_cast<char*>(viewDetail->Reference.getValue()));

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

//// is there any circumstance where vertices need to be in a different state from frames?
//void QGIViewPart::toggleVertices(bool state)
//{
//    Base::Console().Message("QGIVP::toggleVertices(%d) - %s\n",state,getViewName());
////    QList<QGraphicsItem*> items = childItems();
////    for(QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); it++) {
////        QGIVertex *vert = dynamic_cast<QGIVertex *>(*it);
////        QGICMark *mark = dynamic_cast<QGICMark *>(*it);

////        if(vert) {
////            if (!mark) {             //leave center marks showing
////                if(state)
////                    vert->show();
////                else
////                    vert->hide();
////            }
////        }
////    }
//}

TechDraw::DrawHatch* QGIViewPart::faceIsHatched(int i,std::vector<TechDraw::DrawHatch*> hatchObjs) const
{
    TechDraw::DrawHatch* result = nullptr;
    for (auto& h:hatchObjs) {
        const std::vector<std::string> &sourceNames = h->Source.getSubValues();
        int fdx = TechDraw::DrawUtil::getIndexFromName(sourceNames.at(0));
        if (fdx == i) {
            result = h;
            break;
        }
    }
    return result;
}

TechDraw::DrawGeomHatch* QGIViewPart::faceIsGeomHatched(int i,std::vector<TechDraw::DrawGeomHatch*> geomObjs) const
{
    TechDraw::DrawGeomHatch* result = nullptr;
    for (auto& h:geomObjs) {
        const std::vector<std::string> &sourceNames = h->Source.getSubValues();
        int fdx = TechDraw::DrawUtil::getIndexFromName(sourceNames.at(0));
        if (fdx == i) {
            result = h;
            break;
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
            Base::Console().Message(">>>>> element %d: type:%d/%s pos(%.3f,%.3f) M:%d L:%d C:%d\n",iElem,
                                    elem.type,typeName,elem.x,elem.y,elem.isMoveTo(),elem.isLineTo(),elem.isCurveTo());
        }
}

QRectF QGIViewPart::boundingRect() const
{
//    return childrenBoundingRect();
//    return customChildrenBoundingRect();
    return QGIView::boundingRect();
}

//QGIViewPart derived classes do not need a rotate view method as rotation is handled on App side.
void QGIViewPart::rotateView(void)
{
}

bool QGIViewPart::getFaceEdgesPref(void)
{
    bool result = false;
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    result = hGrp->GetBool("DrawFaceEdges", 0l);
    return result;
}
