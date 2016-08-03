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
#include <strstream>
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

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Material.h>
#include <Base/Console.h>

#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawHatch.h>

#include "ZVALUE.h"
#include "QGIFace.h"
#include "QGIEdge.h"
#include "QGIVertex.h"
#include "QGICMark.h"
#include "QGIViewPart.h"

using namespace TechDrawGui;
using namespace TechDrawGeometry;

const float lineScaleFactor = 1.;   // temp fiddle for devel
const float vertexScaleFactor = 2.; // temp fiddle for devel

QGIViewPart::QGIViewPart()
{
    setCacheMode(QGraphicsItem::NoCache);
    setHandlesChildEvents(false);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);
}

QGIViewPart::~QGIViewPart()
{
    tidy();
}

QVariant QGIViewPart::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        QList<QGraphicsItem*> items = childItems();
        for(QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); ++it) {
            //Highlight the children if this is highlighted!?  seems to mess up Face selection?
            QGIEdge *edge = dynamic_cast<QGIEdge *>(*it);
            QGIVertex *vert = dynamic_cast<QGIVertex *>(*it);
            QGIFace *face = dynamic_cast<QGIFace *>(*it);
            if(edge) {
                //edge->setHighlighted(isSelected());
            } else if(vert){
                //vert->setHighlighted(isSelected());
            } else if(face){
                //face->setHighlighted(isSelected());
            }
        }
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

    // called from QGVPage
    setViewFeature(static_cast<TechDraw::DrawView *>(obj));
    draw();
}

QPainterPath QGIViewPart::drawPainterPath(TechDrawGeometry::BaseGeom *baseGeom) const
{
    QPainterPath path;

    switch(baseGeom->geomType) {
        case TechDrawGeometry::CIRCLE: {
          TechDrawGeometry::Circle *geom = static_cast<TechDrawGeometry::Circle *>(baseGeom);

          double x = geom->center.fX - geom->radius;
          double y = geom->center.fY - geom->radius;

          path.addEllipse(x, y, geom->radius * 2, geom->radius * 2);            //topleft@(x,y) radx,rady
          //Base::Console().Message("TRACE -drawPainterPath - making an CIRCLE @(%.3f,%.3f) R:%.3f\n",x, y, geom->radius);
        } break;
        case TechDrawGeometry::ARCOFCIRCLE: {
          TechDrawGeometry::AOC  *geom = static_cast<TechDrawGeometry::AOC *>(baseGeom);

          //double x = geom->center.fX - geom->radius;
          //double y = geom->center.fY - geom->radius;
          pathArc(path, geom->radius, geom->radius, 0., geom->largeArc, geom->cw,
                  geom->endPnt.fX, geom->endPnt.fY,
                  geom->startPnt.fX, geom->startPnt.fY);
          //Base::Console().Message("TRACE -drawPainterPath - making an ARCOFCIRCLE @(%.3f,%.3f) R:%.3f\n",x, y, geom->radius);
        } break;
        case TechDrawGeometry::ELLIPSE: {
          TechDrawGeometry::Ellipse *geom = static_cast<TechDrawGeometry::Ellipse *>(baseGeom);

          // Calculate start and end points as ellipse with theta = 0 and pi
          double startX = geom->center.fX + geom->major * cos(geom->angle),
                 startY = geom->center.fY + geom->major * sin(geom->angle),
                 endX = geom->center.fX - geom->major * cos(geom->angle),
                 endY = geom->center.fY - geom->major * sin(geom->angle);

          pathArc(path, geom->major, geom->minor, geom->angle, false, false,
                  endX, endY, startX, startY);

          pathArc(path, geom->major, geom->minor, geom->angle, false, false,
                  startX, startY, endX, endY);

          //Base::Console().Message("TRACE -drawPainterPath - making an ELLIPSE @(%.3f,%.3f) R1:%.3f R2:%.3f\n",x, y, geom->major, geom->minor);
        } break;
        case TechDrawGeometry::ARCOFELLIPSE: {
          TechDrawGeometry::AOE *geom = static_cast<TechDrawGeometry::AOE *>(baseGeom);

          pathArc(path, geom->major, geom->minor, geom->angle, geom->largeArc, geom->cw,
                        geom->endPnt.fX, geom->endPnt.fY,
                        geom->startPnt.fX, geom->startPnt.fY);
          //Base::Console().Message("TRACE -drawPainterPath - making an ARCOFELLIPSE R1:%.3f R2:%.3f From: (%.3f,%.3f) To: (%.3f,%.3f)\n",geom->major, geom->minor,geom->startPnt.fX, geom->startPnt.fY,geom->endPnt.fX, geom->endPnt.fY);

        } break;
        case TechDrawGeometry::BSPLINE: {
          TechDrawGeometry::BSpline *geom = static_cast<TechDrawGeometry::BSpline *>(baseGeom);

          std::vector<TechDrawGeometry::BezierSegment>::const_iterator it = geom->segments.begin();

          // Move painter to the beginning of our first segment
          path.moveTo(it->pnts[0].fX, it->pnts[0].fY);
          //Base::Console().Message("TRACE -drawPainterPath - making an BSPLINE From: (%.3f,%.3f)\n",it->pnts[0].fX,it->pnts[0].fY);

          for ( ; it != geom->segments.end(); ++it) {
              // At this point, the painter is either at the beginning
              // of the first segment, or end of the last
              if ( it->poles == 2 ) {
                  // Degree 1 bezier = straight line...
                  path.lineTo(it->pnts[1].fX, it->pnts[1].fY);

              } else if ( it->poles == 3 ) {
                  path.quadTo(it->pnts[1].fX, it->pnts[1].fY,
                              it->pnts[2].fX, it->pnts[2].fY);

              } else if ( it->poles == 4 ) {
                  path.cubicTo(it->pnts[1].fX, it->pnts[1].fY,
                               it->pnts[2].fX, it->pnts[2].fY,
                               it->pnts[3].fX, it->pnts[3].fY);
              } else {                                                 //can only handle lines,quads,cubes
                  Base::Console().Error("Bad pole count (%d) for BezierSegment of BSpline geometry\n",it->poles);
                  path.lineTo(it->pnts[1].fX, it->pnts[1].fY);         //show something for debugging
              }
          }
        } break;
        case TechDrawGeometry::GENERIC: {
          TechDrawGeometry::Generic *geom = static_cast<TechDrawGeometry::Generic *>(baseGeom);

          path.moveTo(geom->points[0].fX, geom->points[0].fY);
          std::vector<Base::Vector2D>::const_iterator it = geom->points.begin();
          //Base::Console().Message("TRACE -drawPainterPath - making an GENERIC From: (%.3f,%.3f)\n",geom->points[0].fX, geom->points[0].fY);
          for(++it; it != geom->points.end(); ++it) {
              path.lineTo((*it).fX, (*it).fY);
              //Base::Console().Message(">>>> To: (%.3f,%.3f)\n",(*it).fX, (*it).fY);
          }
        } break;
        default:
          Base::Console().Error("Error - drawPainterPath - UNKNOWN geomType: %d\n",baseGeom->geomType);
          break;
      }

    double rot = getViewObject()->Rotation.getValue();
    if (rot) {
        QTransform t;
        t.rotate(-rot);
        path = t.map(path);
    }

    return path;
}

void QGIViewPart::updateView(bool update)
{
    if (getViewObject() == 0 ||
        !getViewObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        return;
    }

    QGIView::updateView(update);

    TechDraw::DrawViewPart *viewPart = dynamic_cast<TechDraw::DrawViewPart *>(getViewObject());

    if (update ||
       viewPart->isTouched() ||
       viewPart->Source.isTouched() ||
       viewPart->Direction.isTouched() ||
       viewPart->XAxisDirection.isTouched() ||
       viewPart->Tolerance.isTouched() ||
       viewPart->Scale.isTouched() ||
       viewPart->ShowHiddenLines.isTouched() ||
       viewPart->ShowSmoothLines.isTouched() ||
       viewPart->ShowSeamLines.isTouched() ) {
        draw();
    } else if (update ||
              viewPart->LineWidth.isTouched() ||
              viewPart->HiddenWidth.isTouched()) {
        QList<QGraphicsItem*> items = childItems();
        for(QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); ++it) {
            QGIEdge *edge = dynamic_cast<QGIEdge *>(*it);
            if(edge  && edge->getHiddenEdge()) {
                edge->setWidth(viewPart->HiddenWidth.getValue() * lineScaleFactor);
            } else {
                edge->setWidth(viewPart->LineWidth.getValue() * lineScaleFactor);
            }
        }
        draw();
    } else {
        QGIView::draw();
    }
}

void QGIViewPart::draw() {
    drawViewPart();
    drawBorder();
}

void QGIViewPart::drawViewPart()
{
    if ( getViewObject() == 0 ||
         !getViewObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        return;
    }

    TechDraw::DrawViewPart *viewPart = dynamic_cast<TechDraw::DrawViewPart *>(getViewObject());

    float lineWidth = viewPart->LineWidth.getValue() * lineScaleFactor;
    float lineWidthHid = viewPart->HiddenWidth.getValue() * lineScaleFactor;

    prepareGeometryChange();
    removePrimitives();                      //clean the slate

#if MOD_TECHDRAW_HANDLE_FACES
    // Draw Faces
    std::vector<TechDraw::DrawHatch*> hatchObjs = viewPart->getHatches();
    const std::vector<TechDrawGeometry::Face *> &faceGeoms = viewPart->getFaceGeometry();
    std::vector<TechDrawGeometry::Face *>::const_iterator fit = faceGeoms.begin();
    for(int i = 0 ; fit != faceGeoms.end(); fit++, i++) {
        QGIFace* newFace = drawFace(*fit,i);
        TechDraw::DrawHatch* fHatch = faceIsHatched(i,hatchObjs);
        if (fHatch) {
            if (!fHatch->HatchPattern.isEmpty()) {
                App::Color hColor = fHatch->HatchColor.getValue();
                newFace->setHatchColor(hColor.asCSSString());
                newFace->setHatch(fHatch->HatchPattern.getValue());
            }
        }
        newFace->setZValue(ZVALUE::FACE);
        newFace->setPrettyNormal();
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
                (((*itEdge)->classOfEdge == ecSMOOTH) && viewPart->ShowSmoothLines.getValue()) ||
                (((*itEdge)->classOfEdge == ecSEAM) && viewPart->ShowSeamLines.getValue())) {
                showEdge = true;
            }
        } else {
            if (viewPart->ShowHiddenLines.getValue()) {
                showEdge = true;
            }
        }
        if (showEdge) {
            item = new QGIEdge(i);
            addToGroup(item);                                                   //item is at scene(0,0), not group(0,0)
            item->setPos(0.0,0.0);                                              //now at group(0,0)
            item->setPath(drawPainterPath(*itEdge));
            item->setWidth(lineWidth);
            item->setZValue(ZVALUE::EDGE);
            if(!(*itEdge)->visible) {
                item->setWidth(lineWidthHid);
                item->setHiddenEdge(true);
                item->setZValue(ZVALUE::HIDEDGE);
            }
            item->setPrettyNormal();
            //debug a path
            //QPainterPath edgePath=drawPainterPath(*itEdge);
            //std::stringstream edgeId;
            //edgeId << "QGIVP.edgePath" << i;
            //dumpPath(edgeId.str().c_str(),edgePath);
         }
    }

    // Draw Vertexs:
    const std::vector<TechDrawGeometry::Vertex *> &verts = viewPart->getVertexGeometry();
    std::vector<TechDrawGeometry::Vertex *>::const_iterator vert = verts.begin();
    bool showCenters = viewPart->ShowCenters.getValue();
    double cAdjust = viewPart->CenterScale.getValue();
    for(int i = 0 ; vert != verts.end(); ++vert, i++) {
        if ((*vert)->isCenter) {
            if (showCenters) {
                QGICMark* cmItem = new QGICMark(i);
                addToGroup(cmItem);
                cmItem->setPos((*vert)->pnt.fX, (*vert)->pnt.fY);                //this is in ViewPart coords
                cmItem->setThick(0.5 * lineWidth * lineScaleFactor);             //need minimum?
                cmItem->setSize( cAdjust * lineWidth * vertexScaleFactor);
                cmItem->setZValue(ZVALUE::VERTEX);
            }
        } else {
            QGIVertex *item = new QGIVertex(i);
            addToGroup(item);
            item->setPos((*vert)->pnt.fX, (*vert)->pnt.fY);                //this is in ViewPart coords
            item->setRadius(lineWidth * vertexScaleFactor);
            item->setZValue(ZVALUE::VERTEX);
        }
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
            wirePath.setFillRule(Qt::WindingFill);
        }
        facePath.addPath(wirePath);
    }
    QGIFace* gFace = new QGIFace(idx);
    addToGroup(gFace);
    gFace->setPos(0.0,0.0);
    gFace->setPath(facePath);
    //debug a path
    //std::stringstream faceId;
    //faceId << "facePath " << idx;
    //dumpPath(faceId.str().c_str(),facePath);

    return gFace;
}

//! Remove all existing QGIPrimPath items(Vertex,Edge,Face)
void QGIViewPart::removePrimitives()
{
    QList<QGraphicsItem*> children = childItems();
    for (auto& c:children) {
         QGIPrimPath* prim = dynamic_cast<QGIPrimPath*>(c);
         if (prim) {
            removeFromGroup(prim);
            scene()->removeItem(prim);
//            deleteItems.append(prim);         //pretty sure we could just delete here since not in scene anymore
            delete prim;
         }
     }
}

// As called by arc of ellipse case:
// pathArc(path, geom->major, geom->minor, geom->angle, geom->largeArc, geom->cw,
//         geom->endPnt.fX, geom->endPnt.fY,
//         geom->startPnt.fX, geom->startPnt.fY);
void QGIViewPart::pathArc(QPainterPath &path, double rx, double ry, double x_axis_rotation,
                                    bool large_arc_flag, bool sweep_flag,
                                    double x, double y,
                                    double curx, double cury) const
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
                                           double rx, double ry, double xAxisRotation) const
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
        (*it)->setCacheMode((state)? NoCache : NoCache);
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

void QGIViewPart::toggleVertices(bool state)
{
    QList<QGraphicsItem*> items = childItems();
    for(QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); it++) {
        QGIVertex *vert = dynamic_cast<QGIVertex *>(*it);
        QGICMark *mark = dynamic_cast<QGICMark *>(*it);

        if(vert) {
            if (!mark) {             //leave center marks showing
                if(state)
                    vert->show();
                else
                    vert->hide();
            }
        }
    }
}

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
                typeName = "Unknown";
            }
            Base::Console().Message(">>>>> element %d: type:%d/%s pos(%.3f,%.3f) M:%d L:%d C:%d\n",iElem,
                                    elem.type,typeName,elem.x,elem.y,elem.isMoveTo(),elem.isLineTo(),elem.isCurveTo());
        }
}

QRectF QGIViewPart::boundingRect() const
{
    return childrenBoundingRect();
}
