/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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
#include <qmath.h>
#endif

#include <Base/Console.h>

#include "PathBuilder.h"
#include "Rez.h"

using namespace TechDraw;
using namespace TechDrawGui;


QPainterPath PathBuilder::geomToPainterPath(BaseGeomPtr baseGeom, double rot) const
{
    Q_UNUSED(rot);
    QPainterPath path;

    if (!baseGeom)
        return path;

    switch (baseGeom->getGeomType()) {
        case CIRCLE: {
            TechDraw::CirclePtr geom = std::static_pointer_cast<TechDraw::Circle>(baseGeom);

            double x = geom->center.x - geom->radius;
            double y = geom->center.y - geom->radius;

            path.addEllipse(Rez::guiX(x), Rez::guiX(y), Rez::guiX(geom->radius * 2),
                            Rez::guiX(geom->radius * 2));//topleft@(x, y) radx, rady
        } break;
        case ARCOFCIRCLE: {
            TechDraw::AOCPtr geom = std::static_pointer_cast<TechDraw::AOC>(baseGeom);
            if (baseGeom->getReversed()) {
                path.moveTo(Rez::guiX(geom->endPnt.x), Rez::guiX(geom->endPnt.y));
                pathArc(path, Rez::guiX(geom->radius), Rez::guiX(geom->radius), 0., geom->largeArc,
                        !geom->cw, Rez::guiX(geom->startPnt.x), Rez::guiX(geom->startPnt.y),
                        Rez::guiX(geom->endPnt.x), Rez::guiX(geom->endPnt.y));
            }
            else {
                path.moveTo(Rez::guiX(geom->startPnt.x), Rez::guiX(geom->startPnt.y));
                pathArc(path, Rez::guiX(geom->radius), Rez::guiX(geom->radius), 0., geom->largeArc,
                        geom->cw, Rez::guiX(geom->endPnt.x), Rez::guiX(geom->endPnt.y),
                        Rez::guiX(geom->startPnt.x), Rez::guiX(geom->startPnt.y));
            }
        } break;
        case TechDraw::ELLIPSE: {
            TechDraw::AOEPtr geom = std::static_pointer_cast<TechDraw::AOE>(baseGeom);

            // Calculate start and end points as ellipse with theta = 0 and pi
            double startX = geom->center.x + geom->major * cos(geom->angle),
                   startY = geom->center.y + geom->major * sin(geom->angle),
                   endX = geom->center.x - geom->major * cos(geom->angle),
                   endY = geom->center.y - geom->major * sin(geom->angle);

            pathArc(path, Rez::guiX(geom->major), Rez::guiX(geom->minor), geom->angle, false, false,
                    Rez::guiX(endX), Rez::guiX(endY), Rez::guiX(startX), Rez::guiX(startY));

            pathArc(path, Rez::guiX(geom->major), Rez::guiX(geom->minor), geom->angle, false, false,
                    Rez::guiX(startX), Rez::guiX(startY), Rez::guiX(endX), Rez::guiX(endY));
        } break;
        case TechDraw::ARCOFELLIPSE: {
            TechDraw::AOEPtr geom = std::static_pointer_cast<TechDraw::AOE>(baseGeom);
            if (baseGeom->getReversed()) {
                path.moveTo(Rez::guiX(geom->endPnt.x), Rez::guiX(geom->endPnt.y));
                pathArc(path, Rez::guiX(geom->major), Rez::guiX(geom->minor), geom->angle,
                        geom->largeArc, !geom->cw, Rez::guiX(geom->startPnt.x),
                        Rez::guiX(geom->startPnt.y), Rez::guiX(geom->endPnt.x),
                        Rez::guiX(geom->endPnt.y));
            }
            else {
                path.moveTo(Rez::guiX(geom->startPnt.x), Rez::guiX(geom->startPnt.y));
                pathArc(path, Rez::guiX(geom->major), Rez::guiX(geom->minor), geom->angle,
                        geom->largeArc, geom->cw, Rez::guiX(geom->endPnt.x),
                        Rez::guiX(geom->endPnt.y), Rez::guiX(geom->startPnt.x),
                        Rez::guiX(geom->startPnt.y));
            }
        } break;
        case TechDraw::BEZIER: {
            TechDraw::BezierSegmentPtr geom =
                std::static_pointer_cast<TechDraw::BezierSegment>(baseGeom);
            if (baseGeom->getReversed()) {
                if (!geom->pnts.empty()) {
                    Base::Vector3d rStart = geom->pnts.back();
                    path.moveTo(Rez::guiX(rStart.x), Rez::guiX(rStart.y));
                }
                if (geom->poles == 2) {
                    // Degree 1 bezier = straight line...
                    path.lineTo(Rez::guiX(geom->pnts[0].x), Rez::guiX(geom->pnts[0].y));
                }
                else if (geom->poles == 3) {
                    path.quadTo(Rez::guiX(geom->pnts[1].x), Rez::guiX(geom->pnts[1].y),
                                Rez::guiX(geom->pnts[0].x), Rez::guiX(geom->pnts[0].y));
                }
                else if (geom->poles == 4) {
                    path.cubicTo(Rez::guiX(geom->pnts[2].x), Rez::guiX(geom->pnts[2].y),
                                 Rez::guiX(geom->pnts[1].x), Rez::guiX(geom->pnts[1].y),
                                 Rez::guiX(geom->pnts[0].x), Rez::guiX(geom->pnts[0].y));
                }
                else {//can only handle lines, quads, cubes
                    Base::Console().Error("Bad pole count (%d) for BezierSegment\n", geom->poles);
                    auto itBez = geom->pnts.begin() + 1;
                    for (; itBez != geom->pnts.end(); itBez++) {
                        path.lineTo(Rez::guiX((*itBez).x),
                                    Rez::guiX((*itBez).y));//show something for debugging
                    }
                }
            }
            else {
                // Move painter to the beginning
                path.moveTo(Rez::guiX(geom->pnts[0].x), Rez::guiX(geom->pnts[0].y));

                if (geom->poles == 2) {
                    // Degree 1 bezier = straight line...
                    path.lineTo(Rez::guiX(geom->pnts[1].x), Rez::guiX(geom->pnts[1].y));
                }
                else if (geom->poles == 3) {
                    path.quadTo(Rez::guiX(geom->pnts[1].x), Rez::guiX(geom->pnts[1].y),
                                Rez::guiX(geom->pnts[2].x), Rez::guiX(geom->pnts[2].y));
                }
                else if (geom->poles == 4) {
                    path.cubicTo(Rez::guiX(geom->pnts[1].x), Rez::guiX(geom->pnts[1].y),
                                 Rez::guiX(geom->pnts[2].x), Rez::guiX(geom->pnts[2].y),
                                 Rez::guiX(geom->pnts[3].x), Rez::guiX(geom->pnts[3].y));
                }
                else {//can only handle lines, quads, cubes
                    Base::Console().Error("Bad pole count (%d) for BezierSegment\n", geom->poles);
                    auto itBez = geom->pnts.begin() + 1;
                    for (; itBez != geom->pnts.end(); itBez++) {
                        path.lineTo(Rez::guiX((*itBez).x),
                                    Rez::guiX((*itBez).y));//show something for debugging
                    }
                }
            }
        } break;
        case TechDraw::BSPLINE: {
            TechDraw::BSplinePtr geom = std::static_pointer_cast<TechDraw::BSpline>(baseGeom);
            if (baseGeom->getReversed()) {
                // Move painter to the end of our last segment
                std::vector<TechDraw::BezierSegment>::const_reverse_iterator it =
                    geom->segments.rbegin();
                Base::Vector3d rStart = it->pnts.back();
                path.moveTo(Rez::guiX(rStart.x), Rez::guiX(rStart.y));

                for (; it != geom->segments.rend(); ++it) {
                    // At this point, the painter is either at the beginning
                    // of the first segment, or end of the last
                    if (it->poles == 2) {
                        // Degree 1 bezier = straight line...
                        path.lineTo(Rez::guiX(it->pnts[0].x), Rez::guiX(it->pnts[0].y));
                    }
                    else if (it->poles == 3) {
                        path.quadTo(Rez::guiX(it->pnts[1].x), Rez::guiX(it->pnts[1].y),
                                    Rez::guiX(it->pnts[0].x), Rez::guiX(it->pnts[0].y));
                    }
                    else if (it->poles == 4) {
                        path.cubicTo(Rez::guiX(it->pnts[2].x), Rez::guiX(it->pnts[2].y),
                                     Rez::guiX(it->pnts[1].x), Rez::guiX(it->pnts[1].y),
                                     Rez::guiX(it->pnts[0].x), Rez::guiX(it->pnts[0].y));
                    }
                    else {//can only handle lines, quads, cubes
                        Base::Console().Error(
                            "Bad pole count (%d) for BezierSegment of B-spline geometry\n",
                            it->poles);
                        path.lineTo(it->pnts[1].x, it->pnts[1].y);//show something for debugging
                    }
                }
            }
            else {
                // Move painter to the beginning of our first segment
                std::vector<TechDraw::BezierSegment>::const_iterator it = geom->segments.begin();
                path.moveTo(Rez::guiX(it->pnts[0].x), Rez::guiX(it->pnts[0].y));

                for (; it != geom->segments.end(); ++it) {
                    // At this point, the painter is either at the beginning
                    // of the first segment, or end of the last
                    if (it->poles == 2) {
                        // Degree 1 bezier = straight line...
                        path.lineTo(Rez::guiX(it->pnts[1].x), Rez::guiX(it->pnts[1].y));
                    }
                    else if (it->poles == 3) {
                        path.quadTo(Rez::guiX(it->pnts[1].x), Rez::guiX(it->pnts[1].y),
                                    Rez::guiX(it->pnts[2].x), Rez::guiX(it->pnts[2].y));
                    }
                    else if (it->poles == 4) {
                        path.cubicTo(Rez::guiX(it->pnts[1].x), Rez::guiX(it->pnts[1].y),
                                     Rez::guiX(it->pnts[2].x), Rez::guiX(it->pnts[2].y),
                                     Rez::guiX(it->pnts[3].x), Rez::guiX(it->pnts[3].y));
                    }
                    else {
                        Base::Console().Error(
                            "Bad pole count (%d) for BezierSegment of B-spline geometry\n",
                            it->poles);
                        path.lineTo(it->pnts[1].x, it->pnts[1].y);//show something for debugging
                    }
                }
            }
        } break;
        case TechDraw::GENERIC: {
            TechDraw::GenericPtr geom = std::static_pointer_cast<TechDraw::Generic>(baseGeom);
            if (baseGeom->getReversed()) {
                if (!geom->points.empty()) {
                    Base::Vector3d rStart = geom->points.back();
                    path.moveTo(Rez::guiX(rStart.x), Rez::guiX(rStart.y));
                }
                std::vector<Base::Vector3d>::const_reverse_iterator it = geom->points.rbegin();
                for (++it; it != geom->points.rend(); ++it) {
                    path.lineTo(Rez::guiX((*it).x), Rez::guiX((*it).y));
                }
            }
            else {
                path.moveTo(Rez::guiX(geom->points[0].x), Rez::guiX(geom->points[0].y));
                std::vector<Base::Vector3d>::const_iterator it = geom->points.begin();
                for (++it; it != geom->points.end(); ++it) {
                    path.lineTo(Rez::guiX((*it).x), Rez::guiX((*it).y));
                }
            }
        } break;
        default: {
            Base::Console().Error("Error - geomToPainterPath - UNKNOWN geomType: %d\n",
                                  static_cast<int>(baseGeom->getGeomType()));
        } break;
    }//sb end of switch

    //old rotate path logic. now done on App side.
    //    if (rot != 0.0) {
    //        QTransform t;
    //        t.rotate(-rot);
    //        path = t.map(path);
    //    }

    return path;
}


// As called by arc of ellipse case:
// pathArc(path, geom->major, geom->minor, geom->angle, geom->largeArc, geom->cw,
//         geom->endPnt.x, geom->endPnt.y,
//         geom->startPnt.x, geom->startPnt.y);
void PathBuilder::pathArc(QPainterPath& path, double rx, double ry, double x_axis_rotation,
                          bool large_arc_flag, bool sweep_flag, double x, double y, double curx,
                          double cury) const
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
    dx1 = cos_th * dx + sin_th * dy;
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

    a00 = cos_th / rx;
    a01 = sin_th / rx;
    a10 = -sin_th / ry;
    a11 = cos_th / ry;
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
        pathArcSegment(path, xc, yc, th0 + i * th_arc / n_segs, th0 + (i + 1) * th_arc / n_segs, rx,
                       ry, x_axis_rotation);
    }
}

void PathBuilder::pathArcSegment(QPainterPath& path, double xc, double yc, double th0, double th1,
                                 double rx, double ry, double xAxisRotation) const
{
    double sinTh, cosTh;
    double a00, a01, a10, a11;
    double x1, y1, x2, y2, x3, y3;
    double t;
    double thHalf;

    sinTh = qSin(xAxisRotation);
    cosTh = qCos(xAxisRotation);

    a00 = cosTh * rx;
    a01 = -sinTh * ry;
    a10 = sinTh * rx;
    a11 = cosTh * ry;

    thHalf = 0.5 * (th1 - th0);
    t = (8.0 / 3.0) * qSin(thHalf * 0.5) * qSin(thHalf * 0.5) / qSin(thHalf);
    x1 = xc + qCos(th0) - t * qSin(th0);
    y1 = yc + qSin(th0) + t * qCos(th0);
    x3 = xc + qCos(th1);
    y3 = yc + qSin(th1);
    x2 = x3 + t * qSin(th1);
    y2 = y3 - t * qCos(th1);

    path.cubicTo(a00 * x1 + a01 * y1, a10 * x1 + a11 * y1, a00 * x2 + a01 * y2, a10 * x2 + a11 * y2,
                 a00 * x3 + a01 * y3, a10 * x3 + a11 * y3);
}

