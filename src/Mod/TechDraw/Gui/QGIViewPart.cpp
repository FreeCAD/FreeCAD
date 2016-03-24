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
#endif // #ifndef _PreComp_


#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "../App/DrawUtil.h"
#include "../App/DrawViewPart.h"
#include "../App/DrawHatch.h"

#include "ZVALUE.h"
#include "QGIViewPart.h"

using namespace TechDrawGui;
using namespace TechDrawGeometry;

void _dumpPath(const char* text,QPainterPath path);

const float lineScaleFactor = 1.;   // temp fiddle for devel
const float vertexScaleFactor = 2.; // temp fiddle for devel

QGIViewPart::QGIViewPart(const QPoint &pos, QGraphicsScene *scene)
                :QGIView(pos, scene)
{
    setHandlesChildEvents(false);
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsMovable, true);

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Drawing/Colors");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("HiddenColor", 0x08080800));
    m_colHid = fcColor.asQColor();
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
            QGIEdge *edge = dynamic_cast<QGIEdge *>(*it);
            QGIVertex *vert = dynamic_cast<QGIVertex *>(*it);
            if(edge) {
                edge->setHighlighted(isSelected());
            } else if(vert){
                vert->setHighlighted(isSelected());
            }
        }
    } else if(change == ItemSceneChange && scene()) {
           tidy();
    }
    return QGIView::itemChange(change, value);
}

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
    // called from QGVPage
    setViewFeature(static_cast<TechDraw::DrawView *>(obj));
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

    if(update ||
       viewPart->isTouched() ||
       viewPart->Source.isTouched() ||
       viewPart->Direction.isTouched() ||
       viewPart->Tolerance.isTouched() ||
       viewPart->Scale.isTouched() ||
       viewPart->ShowHiddenLines.isTouched()) {
        // Remove all existing graphical representations (QGIxxxx)  otherwise BRect only grows, never shrinks?
        prepareGeometryChange();
        QList<QGraphicsItem*> items = childItems();
        for(QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); ++it) {
            if (dynamic_cast<QGIEdge *> (*it) ||
                dynamic_cast<QGIFace *>(*it) ||
                dynamic_cast<QGIVertex *>(*it) ||
                dynamic_cast<QGIHatch *>(*it)) {
                removeFromGroup(*it);
                scene()->removeItem(*it);

                // We store these and delete till later to prevent rendering crash ISSUE
                deleteItems.append(*it);
            }
        }
        draw();
    } else if(viewPart->LineWidth.isTouched() ||
              viewPart->HiddenWidth.isTouched()) {
        QList<QGraphicsItem*> items = childItems();
        for(QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); ++it) {
            QGIEdge *edge = dynamic_cast<QGIEdge *>(*it);
            if(edge  && edge->getHiddenEdge()) {
                edge->setStrokeWidth(viewPart->HiddenWidth.getValue() * lineScaleFactor);
            } else {
                edge->setStrokeWidth(viewPart->LineWidth.getValue() * lineScaleFactor);
            }
        }
        draw();
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

#if MOD_TECHDRAW_HANDLE_FACES
    // Draw Faces
    const std::vector<TechDrawGeometry::Face *> &faceGeoms = viewPart->getFaceGeometry();
    std::vector<TechDrawGeometry::Face *>::const_iterator fit = faceGeoms.begin();
    QPen facePen;
    facePen.setCosmetic(true);
    //QBrush faceBrush;
    for(int i = 0 ; fit != faceGeoms.end(); fit++, i++) {
        QGIFace* newFace = drawFace(*fit);
        newFace->setPen(facePen);
        newFace->setZValue(ZVALUE::FACE);
        //newFace->setBrush(faceBrush);
    }
    //debug a path
    //std::stringstream faceId;
    //faceId << "facePath" << i;
    //_dumpPath(faceId.str().c_str(),facePath);
#endif //#if MOD_TECHDRAW_HANDLE_FACES

    // Draw Hatches
    std::vector<TechDraw::DrawHatch*> hatchObjs = viewPart->getHatches();
    if (!hatchObjs.empty()) {
        std::vector<TechDraw::DrawHatch*>::iterator itHatch = hatchObjs.begin();
        for(; itHatch != hatchObjs.end(); itHatch++) {
            //if hatchdirection == viewPartdirection {
            TechDraw::DrawHatch* feat = (*itHatch);
            const std::vector<std::string> &edgeNames = feat->Edges.getSubValues();
            std::vector<std::string>::const_iterator itEdge = edgeNames.begin();
            std::vector<TechDrawGeometry::BaseGeom*> unChained;

            //get all edge geometries for this hatch
            for (; itEdge != edgeNames.end(); itEdge++) {
                int idxEdge = DrawUtil::getIndexFromName((*itEdge));
                TechDrawGeometry::BaseGeom* edgeGeom = viewPart->getProjEdgeByIndex(idxEdge);
                if (!edgeGeom) {
                    Base::Console().Log("Error - qgivp::drawViewPart - edgeGeom: %d is NULL\n",idxEdge);
                }
                unChained.push_back(edgeGeom);
            }

            //chain edges tail to nose into a closed region
            std::vector<TechDrawGeometry::BaseGeom*> chained = TechDrawGeometry::chainGeoms(unChained);

            //iterate through the chain to make QPainterPath
            std::vector<TechDrawGeometry::BaseGeom*>::iterator itChain = chained.begin();
            QPainterPath hatchPath;
            for (; itChain != chained.end(); itChain++) {
                QPainterPath subPath;
                if ((*itChain)->reversed) {
                    subPath = drawPainterPath((*itChain)).toReversed();
                } else {
                    subPath = drawPainterPath((*itChain));
                }
                hatchPath.connectPath(subPath);
                //_dumpPath("subpath",subPath);
            }

            QGIHatch* hatch = new QGIHatch(feat->getNameInDocument());
            addToGroup(hatch);
            hatch->setPos(0.0,0.0);
            hatch->setPath(hatchPath);
            hatch->setFill(feat->HatchPattern.getValue());
            hatch->setColor(feat->HatchColor.getValue());
            //_dumpPath("hatchPath",hatchPath);
            hatch->setFlag(QGraphicsItem::ItemIsSelectable, true);
            hatch->setZValue(ZVALUE::HATCH);
        }
    }

    // Draw Edges
    const std::vector<TechDrawGeometry::BaseGeom *> &geoms = viewPart->getEdgeGeometry();
    const std::vector<int> &refs = viewPart->getEdgeReferences();
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
            item->setReference(refs.at(i));
            addToGroup(item);                                                   //item is at scene(0,0), not group(0,0)
            item->setPos(0.0,0.0);
            item->setPath(drawPainterPath(*itEdge));
            item->setStrokeWidth(lineWidth);
            item->setZValue(ZVALUE::EDGE);
            item->setFlag(QGraphicsItem::ItemIsSelectable, true);
            item->setAcceptHoverEvents(true);
            if(!(*itEdge)->visible) {
                item->setStrokeWidth(lineWidthHid);
                item->setHiddenEdge(true);
                item->setZValue(ZVALUE::HIDEDGE);
            }
            item->setPrettyNormal();
            //debug a path
            //QPainterPath edgePath=drawPainterPath(*itEdge);
            //std::stringstream edgeId;
            //edgeId << "QGIVP.edgePath" << i;
            //_dumpPath(edgeId.str().c_str(),edgePath);
         }
    }

    // Draw Vertexs:
    const std::vector<TechDrawGeometry::Vertex *> &verts = viewPart->getVertexGeometry();
    const std::vector<int> &vertRefs                    = viewPart->getVertexReferences();
    std::vector<TechDrawGeometry::Vertex *>::const_iterator vert = verts.begin();
    for(int i = 0 ; vert != verts.end(); ++vert, i++) {
        QGIVertex *item = new QGIVertex(i);
        item->setReference(vertRefs.at(i));
        addToGroup(item);
        item->setPos((*vert)->pnt.fX, (*vert)->pnt.fY);                //this is in ViewPart coords
        item->setRadius(lineWidth * vertexScaleFactor);
        item->setZValue(ZVALUE::VERTEX);
     }
}

QGIFace* QGIViewPart::drawFace(TechDrawGeometry::Face* f)
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
    QGIFace* gFace = new QGIFace(-1);
    addToGroup(gFace);
    gFace->setPos(0.0,0.0);
    gFace->setPath(facePath);
    //_dumpPath("QGIVP.facePath",facePath);

    //gFace->setFlag(QGraphicsItem::ItemIsSelectable, true);   ???
    return gFace;
}

std::vector<TechDraw::DrawHatch*> QGIViewPart::getHatchesForView(TechDraw::DrawViewPart* viewPart)
{
    std::vector<App::DocumentObject*> docObjs = viewPart->getDocument()->getObjectsOfType(TechDraw::DrawHatch::getClassTypeId());
    std::vector<TechDraw::DrawHatch*> hatchObjs;
    std::string viewName = viewPart->getNameInDocument();
    std::vector<App::DocumentObject*>::iterator itDoc = docObjs.begin();
    for(; itDoc != docObjs.end(); itDoc++) {
        TechDraw::DrawHatch* hatch = dynamic_cast<TechDraw::DrawHatch*>(*itDoc);
        if (viewName.compare((hatch->PartView.getValue())->getNameInDocument()) == 0) {
            hatchObjs.push_back(hatch);
        }
    }
    return hatchObjs;
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

QGIEdge * QGIViewPart::findRefEdge(int idx)
{
    QList<QGraphicsItem*> items = childItems();
    for(QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); it++) {
        QGIEdge *edge = dynamic_cast<QGIEdge *>(*it);
        if(edge && edge->getReference() == idx)
            return edge;
    }
    return 0;
}

QGIVertex * QGIViewPart::findRefVertex(int idx)
{
    QList<QGraphicsItem*> items = childItems();
    for(QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); it++) {
        QGIVertex *vert = dynamic_cast<QGIVertex *>(*it);
        if(vert && vert->getReference() == idx)
            return vert;
    }
    return 0;
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
        if(vert) {
            if(state)
                vert->show();
            else
                vert->hide();
        }
    }
}

QRectF QGIViewPart::boundingRect() const
{
    //return childrenBoundingRect().adjusted(-2.,-2.,2.,6.);             //just a bit bigger than the children need
    return childrenBoundingRect();
}

void _dumpPath(const char* text,QPainterPath path)
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


#include "moc_QGIViewPart.cpp"
