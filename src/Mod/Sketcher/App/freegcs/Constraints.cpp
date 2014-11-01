/***************************************************************************
 *   Copyright (c) Konstantinos Poulios      (logari81@gmail.com) 2011     *
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

#include <cmath>
#include "Constraints.h"
#include <algorithm>

#define DEBUG_DERIVS 0
#if DEBUG_DERIVS
#include <cassert>
#endif

namespace GCS
{

///////////////////////////////////////
// Constraints
///////////////////////////////////////

Constraint::Constraint()
: origpvec(0), pvec(0), scale(1.), tag(0), remapped(true)
{
}

void Constraint::redirectParams(MAP_pD_pD redirectionmap)
{
    int i=0;
    for (VEC_pD::iterator param=origpvec.begin();
         param != origpvec.end(); ++param, i++) {
        MAP_pD_pD::const_iterator it = redirectionmap.find(*param);
        if (it != redirectionmap.end())
            pvec[i] = it->second;
    }
    remapped=true;
}

void Constraint::revertParams()
{
    pvec = origpvec;
    remapped=true;
}

ConstraintType Constraint::getTypeId()
{
    return None;
}

void Constraint::rescale(double coef)
{
    scale = coef * 1.;
}

double Constraint::error()
{
    return 0.;
}

double Constraint::grad(double *param)
{
    return 0.;
}

double Constraint::maxStep(MAP_pD_D &dir, double lim)
{
    return lim;
}

// Equal
ConstraintEqual::ConstraintEqual(double *p1, double *p2)
{
    pvec.push_back(p1);
    pvec.push_back(p2);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintEqual::getTypeId()
{
    return Equal;
}

void ConstraintEqual::rescale(double coef)
{
    scale = coef * 1.;
}

double ConstraintEqual::error()
{
    return scale * (*param1() - *param2());
}

double ConstraintEqual::grad(double *param)
{
    double deriv=0.;
    if (param == param1()) deriv += 1;
    if (param == param2()) deriv += -1;
    return scale * deriv;
}

// Difference
ConstraintDifference::ConstraintDifference(double *p1, double *p2, double *d)
{
    pvec.push_back(p1);
    pvec.push_back(p2);
    pvec.push_back(d);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintDifference::getTypeId()
{
    return Difference;
}

void ConstraintDifference::rescale(double coef)
{
    scale = coef * 1.;
}

double ConstraintDifference::error()
{
    return scale * (*param2() - *param1() - *difference());
}

double ConstraintDifference::grad(double *param)
{
    double deriv=0.;
    if (param == param1()) deriv += -1;
    if (param == param2()) deriv += 1;
    if (param == difference()) deriv += -1;
    return scale * deriv;
}

// P2PDistance
ConstraintP2PDistance::ConstraintP2PDistance(Point &p1, Point &p2, double *d)
{
    pvec.push_back(p1.x);
    pvec.push_back(p1.y);
    pvec.push_back(p2.x);
    pvec.push_back(p2.y);
    pvec.push_back(d);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintP2PDistance::getTypeId()
{
    return P2PDistance;
}

void ConstraintP2PDistance::rescale(double coef)
{
    scale = coef * 1.;
}

double ConstraintP2PDistance::error()
{
    double dx = (*p1x() - *p2x());
    double dy = (*p1y() - *p2y());
    double d = sqrt(dx*dx + dy*dy);
    double dist  = *distance();
    return scale * (d - dist);
}

double ConstraintP2PDistance::grad(double *param)
{
    double deriv=0.;
    if (param == p1x() || param == p1y() ||
        param == p2x() || param == p2y()) {
        double dx = (*p1x() - *p2x());
        double dy = (*p1y() - *p2y());
        double d = sqrt(dx*dx + dy*dy);
        if (param == p1x()) deriv += dx/d;
        if (param == p1y()) deriv += dy/d;
        if (param == p2x()) deriv += -dx/d;
        if (param == p2y()) deriv += -dy/d;
    }
    if (param == distance()) deriv += -1.;

    return scale * deriv;
}

double ConstraintP2PDistance::maxStep(MAP_pD_D &dir, double lim)
{
    MAP_pD_D::iterator it;
    // distance() >= 0
    it = dir.find(distance());
    if (it != dir.end()) {
        if (it->second < 0.)
            lim = std::min(lim, -(*distance()) / it->second);
    }
    // restrict actual distance change
    double ddx=0.,ddy=0.;
    it = dir.find(p1x());
    if (it != dir.end()) ddx += it->second;
    it = dir.find(p1y());
    if (it != dir.end()) ddy += it->second;
    it = dir.find(p2x());
    if (it != dir.end()) ddx -= it->second;
    it = dir.find(p2y());
    if (it != dir.end()) ddy -= it->second;
    double dd = sqrt(ddx*ddx+ddy*ddy);
    double dist  = *distance();
    if (dd > dist) {
        double dx = (*p1x() - *p2x());
        double dy = (*p1y() - *p2y());
        double d = sqrt(dx*dx + dy*dy);
        if (dd > d)
            lim = std::min(lim, std::max(d,dist)/dd);
    }
    return lim;
}

// P2PAngle
ConstraintP2PAngle::ConstraintP2PAngle(Point &p1, Point &p2, double *a, double da_)
: da(da_)
{
    pvec.push_back(p1.x);
    pvec.push_back(p1.y);
    pvec.push_back(p2.x);
    pvec.push_back(p2.y);
    pvec.push_back(a);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintP2PAngle::getTypeId()
{
    return P2PAngle;
}

void ConstraintP2PAngle::rescale(double coef)
{
    scale = coef * 1.;
}

double ConstraintP2PAngle::error()
{
    double dx = (*p2x() - *p1x());
    double dy = (*p2y() - *p1y());
    double a = *angle() + da;
    double ca = cos(a);
    double sa = sin(a);
    double x = dx*ca + dy*sa;
    double y = -dx*sa + dy*ca;
    return scale * atan2(y,x);
}

double ConstraintP2PAngle::grad(double *param)
{
    double deriv=0.;
    if (param == p1x() || param == p1y() ||
        param == p2x() || param == p2y()) {
        double dx = (*p2x() - *p1x());
        double dy = (*p2y() - *p1y());
        double a = *angle() + da;
        double ca = cos(a);
        double sa = sin(a);
        double x = dx*ca + dy*sa;
        double y = -dx*sa + dy*ca;
        double r2 = dx*dx+dy*dy;
        dx = -y/r2;
        dy = x/r2;
        if (param == p1x()) deriv += (-ca*dx + sa*dy);
        if (param == p1y()) deriv += (-sa*dx - ca*dy);
        if (param == p2x()) deriv += ( ca*dx - sa*dy);
        if (param == p2y()) deriv += ( sa*dx + ca*dy);
    }
    if (param == angle()) deriv += -1;

    return scale * deriv;
}

double ConstraintP2PAngle::maxStep(MAP_pD_D &dir, double lim)
{
    // step(angle()) <= pi/18 = 10°
    MAP_pD_D::iterator it = dir.find(angle());
    if (it != dir.end()) {
        double step = std::abs(it->second);
        if (step > M_PI/18.)
            lim = std::min(lim, (M_PI/18.) / step);
    }
    return lim;
}

// P2LDistance
ConstraintP2LDistance::ConstraintP2LDistance(Point &p, Line &l, double *d)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(l.p1.x);
    pvec.push_back(l.p1.y);
    pvec.push_back(l.p2.x);
    pvec.push_back(l.p2.y);
    pvec.push_back(d);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintP2LDistance::getTypeId()
{
    return P2LDistance;
}

void ConstraintP2LDistance::rescale(double coef)
{
    scale = coef;
}

double ConstraintP2LDistance::error()
{
    double x0=*p0x(), x1=*p1x(), x2=*p2x();
    double y0=*p0y(), y1=*p1y(), y2=*p2y();
    double dist = *distance();
    double dx = x2-x1;
    double dy = y2-y1;
    double d = sqrt(dx*dx+dy*dy);
    double area = std::abs(-x0*dy+y0*dx+x1*y2-x2*y1); // = x1y2 - x2y1 - x0y2 + x2y0 + x0y1 - x1y0 = 2*(triangle area)
    return scale * (area/d - dist);
}

double ConstraintP2LDistance::grad(double *param)
{
    double deriv=0.;
    // darea/dx0 = (y1-y2)      darea/dy0 = (x2-x1)
    // darea/dx1 = (y2-y0)      darea/dy1 = (x0-x2)
    // darea/dx2 = (y0-y1)      darea/dy2 = (x1-x0)
    if (param == p0x() || param == p0y() ||
        param == p1x() || param == p1y() ||
        param == p2x() || param == p2y()) {
        double x0=*p0x(), x1=*p1x(), x2=*p2x();
        double y0=*p0y(), y1=*p1y(), y2=*p2y();
        double dx = x2-x1;
        double dy = y2-y1;
        double d2 = dx*dx+dy*dy;
        double d = sqrt(d2);
        double area = -x0*dy+y0*dx+x1*y2-x2*y1;
        if (param == p0x()) deriv += (y1-y2) / d;
        if (param == p0y()) deriv += (x2-x1) / d ;
        if (param == p1x()) deriv += ((y2-y0)*d + (dx/d)*area) / d2;
        if (param == p1y()) deriv += ((x0-x2)*d + (dy/d)*area) / d2;
        if (param == p2x()) deriv += ((y0-y1)*d - (dx/d)*area) / d2;
        if (param == p2y()) deriv += ((x1-x0)*d - (dy/d)*area) / d2;
        if (area < 0)
            deriv *= -1;
    }
    if (param == distance()) deriv += -1;

    return scale * deriv;
}

double ConstraintP2LDistance::maxStep(MAP_pD_D &dir, double lim)
{
    MAP_pD_D::iterator it;
    // distance() >= 0
    it = dir.find(distance());
    if (it != dir.end()) {
        if (it->second < 0.)
            lim = std::min(lim, -(*distance()) / it->second);
    }
    // restrict actual area change
    double darea=0.;
    double x0=*p0x(), x1=*p1x(), x2=*p2x();
    double y0=*p0y(), y1=*p1y(), y2=*p2y();
    it = dir.find(p0x());
    if (it != dir.end()) darea += (y1-y2) * it->second;
    it = dir.find(p0y());
    if (it != dir.end()) darea += (x2-x1) * it->second;
    it = dir.find(p1x());
    if (it != dir.end()) darea += (y2-y0) * it->second;
    it = dir.find(p1y());
    if (it != dir.end()) darea += (x0-x2) * it->second;
    it = dir.find(p2x());
    if (it != dir.end()) darea += (y0-y1) * it->second;
    it = dir.find(p2y());
    if (it != dir.end()) darea += (x1-x0) * it->second;

    darea = std::abs(darea);
    if (darea > 0.) {
        double dx = x2-x1;
        double dy = y2-y1;
        double area = 0.3*(*distance())*sqrt(dx*dx+dy*dy);
        if (darea > area) {
            area = std::max(area, 0.3*std::abs(-x0*dy+y0*dx+x1*y2-x2*y1));
            if (darea > area)
                lim = std::min(lim, area/darea);
        }
    }
    return lim;
}

// PointOnLine
ConstraintPointOnLine::ConstraintPointOnLine(Point &p, Line &l)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(l.p1.x);
    pvec.push_back(l.p1.y);
    pvec.push_back(l.p2.x);
    pvec.push_back(l.p2.y);
    origpvec = pvec;
    rescale();
}

ConstraintPointOnLine::ConstraintPointOnLine(Point &p, Point &lp1, Point &lp2)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(lp1.x);
    pvec.push_back(lp1.y);
    pvec.push_back(lp2.x);
    pvec.push_back(lp2.y);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintPointOnLine::getTypeId()
{
    return PointOnLine;
}

void ConstraintPointOnLine::rescale(double coef)
{
    scale = coef;
}

double ConstraintPointOnLine::error()
{
    double x0=*p0x(), x1=*p1x(), x2=*p2x();
    double y0=*p0y(), y1=*p1y(), y2=*p2y();
    double dx = x2-x1;
    double dy = y2-y1;
    double d = sqrt(dx*dx+dy*dy);
    double area = -x0*dy+y0*dx+x1*y2-x2*y1; // = x1y2 - x2y1 - x0y2 + x2y0 + x0y1 - x1y0 = 2*(triangle area)
    return scale * area/d;
}

double ConstraintPointOnLine::grad(double *param)
{
    double deriv=0.;
    // darea/dx0 = (y1-y2)      darea/dy0 = (x2-x1)
    // darea/dx1 = (y2-y0)      darea/dy1 = (x0-x2)
    // darea/dx2 = (y0-y1)      darea/dy2 = (x1-x0)
    if (param == p0x() || param == p0y() ||
        param == p1x() || param == p1y() ||
        param == p2x() || param == p2y()) {
        double x0=*p0x(), x1=*p1x(), x2=*p2x();
        double y0=*p0y(), y1=*p1y(), y2=*p2y();
        double dx = x2-x1;
        double dy = y2-y1;
        double d2 = dx*dx+dy*dy;
        double d = sqrt(d2);
        double area = -x0*dy+y0*dx+x1*y2-x2*y1;
        if (param == p0x()) deriv += (y1-y2) / d;
        if (param == p0y()) deriv += (x2-x1) / d ;
        if (param == p1x()) deriv += ((y2-y0)*d + (dx/d)*area) / d2;
        if (param == p1y()) deriv += ((x0-x2)*d + (dy/d)*area) / d2;
        if (param == p2x()) deriv += ((y0-y1)*d - (dx/d)*area) / d2;
        if (param == p2y()) deriv += ((x1-x0)*d - (dy/d)*area) / d2;
    }
    return scale * deriv;
}

// PointOnPerpBisector
ConstraintPointOnPerpBisector::ConstraintPointOnPerpBisector(Point &p, Line &l)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(l.p1.x);
    pvec.push_back(l.p1.y);
    pvec.push_back(l.p2.x);
    pvec.push_back(l.p2.y);
    origpvec = pvec;
    rescale();
}

ConstraintPointOnPerpBisector::ConstraintPointOnPerpBisector(Point &p, Point &lp1, Point &lp2)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(lp1.x);
    pvec.push_back(lp1.y);
    pvec.push_back(lp2.x);
    pvec.push_back(lp2.y);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintPointOnPerpBisector::getTypeId()
{
    return PointOnPerpBisector;
}

void ConstraintPointOnPerpBisector::rescale(double coef)
{
    scale = coef;
}

double ConstraintPointOnPerpBisector::error()
{
    double dx1 = *p1x() - *p0x();
    double dy1 = *p1y() - *p0y();
    double dx2 = *p2x() - *p0x();
    double dy2 = *p2y() - *p0y();
    return scale * (sqrt(dx1*dx1+dy1*dy1) - sqrt(dx2*dx2+dy2*dy2));
}

double ConstraintPointOnPerpBisector::grad(double *param)
{
    double deriv=0.;
    if (param == p0x() || param == p0y() ||
        param == p1x() || param == p1y()) {
        double dx1 = *p1x() - *p0x();
        double dy1 = *p1y() - *p0y();
        if (param == p0x()) deriv -= dx1/sqrt(dx1*dx1+dy1*dy1);
        if (param == p0y()) deriv -= dy1/sqrt(dx1*dx1+dy1*dy1);
        if (param == p1x()) deriv += dx1/sqrt(dx1*dx1+dy1*dy1);
        if (param == p1y()) deriv += dy1/sqrt(dx1*dx1+dy1*dy1);
    }
    if (param == p0x() || param == p0y() ||
        param == p2x() || param == p2y()) {
        double dx2 = *p2x() - *p0x();
        double dy2 = *p2y() - *p0y();
        if (param == p0x()) deriv += dx2/sqrt(dx2*dx2+dy2*dy2);
        if (param == p0y()) deriv += dy2/sqrt(dx2*dx2+dy2*dy2);
        if (param == p2x()) deriv -= dx2/sqrt(dx2*dx2+dy2*dy2);
        if (param == p2y()) deriv -= dy2/sqrt(dx2*dx2+dy2*dy2);
    }
    return scale * deriv;
}

// Parallel
ConstraintParallel::ConstraintParallel(Line &l1, Line &l2)
{
    pvec.push_back(l1.p1.x);
    pvec.push_back(l1.p1.y);
    pvec.push_back(l1.p2.x);
    pvec.push_back(l1.p2.y);
    pvec.push_back(l2.p1.x);
    pvec.push_back(l2.p1.y);
    pvec.push_back(l2.p2.x);
    pvec.push_back(l2.p2.y);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintParallel::getTypeId()
{
    return Parallel;
}

void ConstraintParallel::rescale(double coef)
{
    double dx1 = (*l1p1x() - *l1p2x());
    double dy1 = (*l1p1y() - *l1p2y());
    double dx2 = (*l2p1x() - *l2p2x());
    double dy2 = (*l2p1y() - *l2p2y());
    scale = coef / sqrt((dx1*dx1+dy1*dy1)*(dx2*dx2+dy2*dy2));
}

double ConstraintParallel::error()
{
    double dx1 = (*l1p1x() - *l1p2x());
    double dy1 = (*l1p1y() - *l1p2y());
    double dx2 = (*l2p1x() - *l2p2x());
    double dy2 = (*l2p1y() - *l2p2y());
    return scale * (dx1*dy2 - dy1*dx2);
}

double ConstraintParallel::grad(double *param)
{
    double deriv=0.;
    if (param == l1p1x()) deriv += (*l2p1y() - *l2p2y()); // = dy2
    if (param == l1p2x()) deriv += -(*l2p1y() - *l2p2y()); // = -dy2
    if (param == l1p1y()) deriv += -(*l2p1x() - *l2p2x()); // = -dx2
    if (param == l1p2y()) deriv += (*l2p1x() - *l2p2x()); // = dx2

    if (param == l2p1x()) deriv += -(*l1p1y() - *l1p2y()); // = -dy1
    if (param == l2p2x()) deriv += (*l1p1y() - *l1p2y()); // = dy1
    if (param == l2p1y()) deriv += (*l1p1x() - *l1p2x()); // = dx1
    if (param == l2p2y()) deriv += -(*l1p1x() - *l1p2x()); // = -dx1

    return scale * deriv;
}

// Perpendicular
ConstraintPerpendicular::ConstraintPerpendicular(Line &l1, Line &l2)
{
    pvec.push_back(l1.p1.x);
    pvec.push_back(l1.p1.y);
    pvec.push_back(l1.p2.x);
    pvec.push_back(l1.p2.y);
    pvec.push_back(l2.p1.x);
    pvec.push_back(l2.p1.y);
    pvec.push_back(l2.p2.x);
    pvec.push_back(l2.p2.y);
    origpvec = pvec;
    rescale();
}

ConstraintPerpendicular::ConstraintPerpendicular(Point &l1p1, Point &l1p2,
                                                 Point &l2p1, Point &l2p2)
{
    pvec.push_back(l1p1.x);
    pvec.push_back(l1p1.y);
    pvec.push_back(l1p2.x);
    pvec.push_back(l1p2.y);
    pvec.push_back(l2p1.x);
    pvec.push_back(l2p1.y);
    pvec.push_back(l2p2.x);
    pvec.push_back(l2p2.y);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintPerpendicular::getTypeId()
{
    return Perpendicular;
}

void ConstraintPerpendicular::rescale(double coef)
{
    double dx1 = (*l1p1x() - *l1p2x());
    double dy1 = (*l1p1y() - *l1p2y());
    double dx2 = (*l2p1x() - *l2p2x());
    double dy2 = (*l2p1y() - *l2p2y());
    scale = coef / sqrt((dx1*dx1+dy1*dy1)*(dx2*dx2+dy2*dy2));
}

double ConstraintPerpendicular::error()
{
    double dx1 = (*l1p1x() - *l1p2x());
    double dy1 = (*l1p1y() - *l1p2y());
    double dx2 = (*l2p1x() - *l2p2x());
    double dy2 = (*l2p1y() - *l2p2y());
    return scale * (dx1*dx2 + dy1*dy2);
}

double ConstraintPerpendicular::grad(double *param)
{
    double deriv=0.;
    if (param == l1p1x()) deriv += (*l2p1x() - *l2p2x()); // = dx2
    if (param == l1p2x()) deriv += -(*l2p1x() - *l2p2x()); // = -dx2
    if (param == l1p1y()) deriv += (*l2p1y() - *l2p2y()); // = dy2
    if (param == l1p2y()) deriv += -(*l2p1y() - *l2p2y()); // = -dy2

    if (param == l2p1x()) deriv += (*l1p1x() - *l1p2x()); // = dx1
    if (param == l2p2x()) deriv += -(*l1p1x() - *l1p2x()); // = -dx1
    if (param == l2p1y()) deriv += (*l1p1y() - *l1p2y()); // = dy1
    if (param == l2p2y()) deriv += -(*l1p1y() - *l1p2y()); // = -dy1

    return scale * deriv;
}

// L2LAngle
ConstraintL2LAngle::ConstraintL2LAngle(Line &l1, Line &l2, double *a)
{
    pvec.push_back(l1.p1.x);
    pvec.push_back(l1.p1.y);
    pvec.push_back(l1.p2.x);
    pvec.push_back(l1.p2.y);
    pvec.push_back(l2.p1.x);
    pvec.push_back(l2.p1.y);
    pvec.push_back(l2.p2.x);
    pvec.push_back(l2.p2.y);
    pvec.push_back(a);
    origpvec = pvec;
    rescale();
}

ConstraintL2LAngle::ConstraintL2LAngle(Point &l1p1, Point &l1p2,
                                       Point &l2p1, Point &l2p2, double *a)
{
    pvec.push_back(l1p1.x);
    pvec.push_back(l1p1.y);
    pvec.push_back(l1p2.x);
    pvec.push_back(l1p2.y);
    pvec.push_back(l2p1.x);
    pvec.push_back(l2p1.y);
    pvec.push_back(l2p2.x);
    pvec.push_back(l2p2.y);
    pvec.push_back(a);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintL2LAngle::getTypeId()
{
    return L2LAngle;
}

void ConstraintL2LAngle::rescale(double coef)
{
    scale = coef * 1.;
}

double ConstraintL2LAngle::error()
{
    double dx1 = (*l1p2x() - *l1p1x());
    double dy1 = (*l1p2y() - *l1p1y());
    double dx2 = (*l2p2x() - *l2p1x());
    double dy2 = (*l2p2y() - *l2p1y());
    double a = atan2(dy1,dx1) + *angle();
    double ca = cos(a);
    double sa = sin(a);
    double x2 = dx2*ca + dy2*sa;
    double y2 = -dx2*sa + dy2*ca;
    return scale * atan2(y2,x2);
}

double ConstraintL2LAngle::grad(double *param)
{
    double deriv=0.;
    if (param == l1p1x() || param == l1p1y() ||
        param == l1p2x() || param == l1p2y()) {
        double dx1 = (*l1p2x() - *l1p1x());
        double dy1 = (*l1p2y() - *l1p1y());
        double r2 = dx1*dx1+dy1*dy1;
        if (param == l1p1x()) deriv += -dy1/r2;
        if (param == l1p1y()) deriv += dx1/r2;
        if (param == l1p2x()) deriv += dy1/r2;
        if (param == l1p2y()) deriv += -dx1/r2;
    }
    if (param == l2p1x() || param == l2p1y() ||
        param == l2p2x() || param == l2p2y()) {
        double dx1 = (*l1p2x() - *l1p1x());
        double dy1 = (*l1p2y() - *l1p1y());
        double dx2 = (*l2p2x() - *l2p1x());
        double dy2 = (*l2p2y() - *l2p1y());
        double a = atan2(dy1,dx1) + *angle();
        double ca = cos(a);
        double sa = sin(a);
        double x2 = dx2*ca + dy2*sa;
        double y2 = -dx2*sa + dy2*ca;
        double r2 = dx2*dx2+dy2*dy2;
        dx2 = -y2/r2;
        dy2 = x2/r2;
        if (param == l2p1x()) deriv += (-ca*dx2 + sa*dy2);
        if (param == l2p1y()) deriv += (-sa*dx2 - ca*dy2);
        if (param == l2p2x()) deriv += ( ca*dx2 - sa*dy2);
        if (param == l2p2y()) deriv += ( sa*dx2 + ca*dy2);
    }
    if (param == angle()) deriv += -1;

    return scale * deriv;
}

double ConstraintL2LAngle::maxStep(MAP_pD_D &dir, double lim)
{
    // step(angle()) <= pi/18 = 10°
    MAP_pD_D::iterator it = dir.find(angle());
    if (it != dir.end()) {
        double step = std::abs(it->second);
        if (step > M_PI/18.)
            lim = std::min(lim, (M_PI/18.) / step);
    }
    return lim;
}

// MidpointOnLine
ConstraintMidpointOnLine::ConstraintMidpointOnLine(Line &l1, Line &l2)
{
    pvec.push_back(l1.p1.x);
    pvec.push_back(l1.p1.y);
    pvec.push_back(l1.p2.x);
    pvec.push_back(l1.p2.y);
    pvec.push_back(l2.p1.x);
    pvec.push_back(l2.p1.y);
    pvec.push_back(l2.p2.x);
    pvec.push_back(l2.p2.y);
    origpvec = pvec;
    rescale();
}

ConstraintMidpointOnLine::ConstraintMidpointOnLine(Point &l1p1, Point &l1p2, Point &l2p1, Point &l2p2)
{
    pvec.push_back(l1p1.x);
    pvec.push_back(l1p1.y);
    pvec.push_back(l1p2.x);
    pvec.push_back(l1p2.y);
    pvec.push_back(l2p1.x);
    pvec.push_back(l2p1.y);
    pvec.push_back(l2p2.x);
    pvec.push_back(l2p2.y);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintMidpointOnLine::getTypeId()
{
    return MidpointOnLine;
}

void ConstraintMidpointOnLine::rescale(double coef)
{
    scale = coef * 1;
}

double ConstraintMidpointOnLine::error()
{
    double x0=((*l1p1x())+(*l1p2x()))/2;
    double y0=((*l1p1y())+(*l1p2y()))/2;
    double x1=*l2p1x(), x2=*l2p2x();
    double y1=*l2p1y(), y2=*l2p2y();
    double dx = x2-x1;
    double dy = y2-y1;
    double d = sqrt(dx*dx+dy*dy);
    double area = -x0*dy+y0*dx+x1*y2-x2*y1; // = x1y2 - x2y1 - x0y2 + x2y0 + x0y1 - x1y0 = 2*(triangle area)
    return scale * area/d;
}

double ConstraintMidpointOnLine::grad(double *param)
{
    double deriv=0.;
    // darea/dx0 = (y1-y2)      darea/dy0 = (x2-x1)
    // darea/dx1 = (y2-y0)      darea/dy1 = (x0-x2)
    // darea/dx2 = (y0-y1)      darea/dy2 = (x1-x0)
    if (param == l1p1x() || param == l1p1y() ||
        param == l1p2x() || param == l1p2y()||
        param == l2p1x() || param == l2p1y() ||
        param == l2p2x() || param == l2p2y()) {
        double x0=((*l1p1x())+(*l1p2x()))/2;
        double y0=((*l1p1y())+(*l1p2y()))/2;
        double x1=*l2p1x(), x2=*l2p2x();
        double y1=*l2p1y(), y2=*l2p2y();
        double dx = x2-x1;
        double dy = y2-y1;
        double d2 = dx*dx+dy*dy;
        double d = sqrt(d2);
        double area = -x0*dy+y0*dx+x1*y2-x2*y1;
        if (param == l1p1x()) deriv += (y1-y2) / (2*d);
        if (param == l1p1y()) deriv += (x2-x1) / (2*d);
        if (param == l1p2x()) deriv += (y1-y2) / (2*d);
        if (param == l1p2y()) deriv += (x2-x1) / (2*d);
        if (param == l2p1x()) deriv += ((y2-y0)*d + (dx/d)*area) / d2;
        if (param == l2p1y()) deriv += ((x0-x2)*d + (dy/d)*area) / d2;
        if (param == l2p2x()) deriv += ((y0-y1)*d - (dx/d)*area) / d2;
        if (param == l2p2y()) deriv += ((x1-x0)*d - (dy/d)*area) / d2;
    }
    return scale * deriv;
}

// TangentCircumf
ConstraintTangentCircumf::ConstraintTangentCircumf(Point &p1, Point &p2,
                                                   double *rad1, double *rad2, bool internal_)
{
    internal = internal_;
    pvec.push_back(p1.x);
    pvec.push_back(p1.y);
    pvec.push_back(p2.x);
    pvec.push_back(p2.y);
    pvec.push_back(rad1);
    pvec.push_back(rad2);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintTangentCircumf::getTypeId()
{
    return TangentCircumf;
}

void ConstraintTangentCircumf::rescale(double coef)
{
    scale = coef * 1;
}

double ConstraintTangentCircumf::error()
{
    double dx = (*c1x() - *c2x());
    double dy = (*c1y() - *c2y());
    if (internal)
        return scale * (sqrt(dx*dx + dy*dy) - std::abs(*r1() - *r2()));
    else
        return scale * (sqrt(dx*dx + dy*dy) - (*r1() + *r2()));
}

double ConstraintTangentCircumf::grad(double *param)
{
    double deriv=0.;
    if (param == c1x() || param == c1y() ||
        param == c2x() || param == c2y()||
        param == r1() || param == r2()) {
        double dx = (*c1x() - *c2x());
        double dy = (*c1y() - *c2y());
        double d = sqrt(dx*dx + dy*dy);
        if (param == c1x()) deriv += dx/d;
        if (param == c1y()) deriv += dy/d;
        if (param == c2x()) deriv += -dx/d;
        if (param == c2y()) deriv += -dy/d;
        if (internal) {
            if (param == r1()) deriv += (*r1() > *r2()) ? -1 : 1;
            if (param == r2()) deriv += (*r1() > *r2()) ? 1 : -1;
        }
        else {
            if (param == r1()) deriv += -1;
            if (param == r2()) deriv += -1;
        }
    }
    return scale * deriv;
}

// ConstraintPointOnEllipse
ConstraintPointOnEllipse::ConstraintPointOnEllipse(Point &p, Ellipse &e)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(e.center.x);
    pvec.push_back(e.center.y);
    pvec.push_back(e.focus1X);
    pvec.push_back(e.focus1Y);
    pvec.push_back(e.radmin);
    origpvec = pvec;
    rescale();
}

ConstraintPointOnEllipse::ConstraintPointOnEllipse(Point &p, ArcOfEllipse &a)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(a.center.x);
    pvec.push_back(a.center.y);
    pvec.push_back(a.focus1X);
    pvec.push_back(a.focus1Y);
    pvec.push_back(a.radmin);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintPointOnEllipse::getTypeId()
{
    return PointOnEllipse;
}

void ConstraintPointOnEllipse::rescale(double coef)
{
    scale = coef * 1;
}

double ConstraintPointOnEllipse::error()
{    
    double X_0 = *p1x();
    double Y_0 = *p1y();
    double X_c = *cx();
    double Y_c = *cy();     
    double X_F1 = *f1x();
    double Y_F1 = *f1y();
    double b = *rmin();
    
    double err=sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2)) + sqrt(pow(X_0 +
        X_F1 - 2*X_c, 2) + pow(Y_0 + Y_F1 - 2*Y_c, 2)) - 2*sqrt(pow(b, 2) +
        pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
    return scale * err;
}

double ConstraintPointOnEllipse::grad(double *param)
{
    double deriv=0.;
    if (param == p1x() || param == p1y() ||
        param == f1x() || param == f1y() ||
        param == cx() || param == cy() ||
        param == rmin()) {

        double X_0 = *p1x();
        double Y_0 = *p1y();
        double X_c = *cx();
        double Y_c = *cy();
        double X_F1 = *f1x();
        double Y_F1 = *f1y();
        double b = *rmin();

        if (param == p1x())
            deriv += (X_0 - X_F1)/sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2)) +
                (X_0 + X_F1 - 2*X_c)/sqrt(pow(X_0 + X_F1 - 2*X_c, 2) + pow(Y_0 + Y_F1 -
                2*Y_c, 2));
        if (param == p1y())
            deriv += (Y_0 - Y_F1)/sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2)) +
                (Y_0 + Y_F1 - 2*Y_c)/sqrt(pow(X_0 + X_F1 - 2*X_c, 2) + pow(Y_0 + Y_F1 -
                2*Y_c, 2));
        if (param == f1x())
            deriv += -(X_0 - X_F1)/sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2)) -
                2*(X_F1 - X_c)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                + (X_0 + X_F1 - 2*X_c)/sqrt(pow(X_0 + X_F1 - 2*X_c, 2) + pow(Y_0 + Y_F1
                - 2*Y_c, 2));
        if (param == f1y())
            deriv +=-(Y_0 - Y_F1)/sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2)) -
                2*(Y_F1 - Y_c)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                + (Y_0 + Y_F1 - 2*Y_c)/sqrt(pow(X_0 + X_F1 - 2*X_c, 2) + pow(Y_0 + Y_F1
                - 2*Y_c, 2));
        if (param == cx())
            deriv += 2*(X_F1 - X_c)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1
                - Y_c, 2)) - 2*(X_0 + X_F1 - 2*X_c)/sqrt(pow(X_0 + X_F1 - 2*X_c, 2) +
                pow(Y_0 + Y_F1 - 2*Y_c, 2));
        if (param == cy())
            deriv +=2*(Y_F1 - Y_c)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1
                - Y_c, 2)) - 2*(Y_0 + Y_F1 - 2*Y_c)/sqrt(pow(X_0 + X_F1 - 2*X_c, 2) +
                pow(Y_0 + Y_F1 - 2*Y_c, 2));
        if (param == rmin())
            deriv += -2*b/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                2));
    }
    return scale * deriv;
}

// ConstraintEllipseTangentLine
ConstraintEllipseTangentLine::ConstraintEllipseTangentLine(Line &l, Ellipse &e)
{
    pvec.push_back(l.p1.x);
    pvec.push_back(l.p1.y);
    pvec.push_back(l.p2.x);
    pvec.push_back(l.p2.y);
    pvec.push_back(e.center.x);
    pvec.push_back(e.center.y);
    pvec.push_back(e.focus1X);
    pvec.push_back(e.focus1Y);
    pvec.push_back(e.radmin);
    origpvec = pvec;
    rescale();
}

ConstraintEllipseTangentLine::ConstraintEllipseTangentLine(Line &l, ArcOfEllipse &a)
{
    pvec.push_back(l.p1.x);
    pvec.push_back(l.p1.y);
    pvec.push_back(l.p2.x);
    pvec.push_back(l.p2.y);
    pvec.push_back(a.center.x);
    pvec.push_back(a.center.y);
    pvec.push_back(a.focus1X);
    pvec.push_back(a.focus1Y);
    pvec.push_back(a.radmin);
    origpvec = pvec;
    rescale();    
}

ConstraintType ConstraintEllipseTangentLine::getTypeId()
{
    return TangentEllipseLine;
}

void ConstraintEllipseTangentLine::rescale(double coef)
{
    scale = coef * 1;
}

double ConstraintEllipseTangentLine::error()
{
    double X_1 = *p1x();
    double Y_1 = *p1y();
    double X_2 = *p2x();
    double Y_2 = *p2y();
    double X_c = *cx();
    double Y_c = *cy();
    double X_F1 = *f1x();
    double Y_F1 = *f1y(); 
    double b = *rmin();
    
    double err=-2*(sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) -
        2*Y_1*Y_2 + pow(Y_2, 2))*sqrt(pow(X_F1, 2) - 2*X_F1*X_c + pow(X_c, 2) +
        pow(Y_F1, 2) - 2*Y_F1*Y_c + pow(Y_c, 2) + pow(b, 2)) - sqrt(pow(X_1,
        2)*(pow(X_c, 2) + pow(Y_c, 2)) - 2*X_1*X_2*(pow(X_c, 2) + pow(Y_c, 2)) +
        pow(X_2, 2)*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_F1, 2)*(pow(X_1, 2) -
        2*X_1*X_2 + pow(X_2, 2)) - 2*X_F1*(pow(X_1, 2)*X_c - 2*X_1*X_2*X_c +
        pow(X_2, 2)*X_c) + pow(Y_1, 2)*(pow(X_2, 2) - 2*X_2*X_c + pow(X_c, 2) +
        pow(Y_c, 2)) + 2*Y_1*(X_1*X_2*Y_c - pow(X_2, 2)*Y_c - X_F1*(X_1*Y_c -
        X_2*Y_c)) + pow(Y_2, 2)*(pow(X_1, 2) - 2*X_1*X_c + pow(X_c, 2) +
        pow(Y_c, 2)) - 2*Y_2*(pow(X_1, 2)*Y_c - X_1*X_2*Y_c - X_F1*(X_1*Y_c -
        X_2*Y_c) + Y_1*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2) + pow(Y_c, 2)))
        + pow(Y_F1, 2)*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)) -
        2*Y_F1*(pow(Y_1, 2)*Y_c - Y_1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2)) +
        pow(Y_2, 2)*Y_c + Y_2*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2) -
        2*Y_1*Y_c))))/sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) -
        2*Y_1*Y_2 + pow(Y_2, 2));
    return scale * err;
}

double ConstraintEllipseTangentLine::grad(double *param)
{      
    double deriv=0.;
    if (param == p1x() || param == p1y() ||
        param == p2x() || param == p2y() ||
        param == f1x() || param == f1y() ||
        param == cx() || param == cy() ||
        param == rmin()) {
                
        double X_1 = *p1x();
        double Y_1 = *p1y();
        double X_2 = *p2x();
        double Y_2 = *p2y();
        double X_c = *cx();
        double Y_c = *cy();
        double X_F1 = *f1x();
        double Y_F1 = *f1y(); 
        double b = *rmin();
        
        // DeepSOIC equation 
        // http://forum.freecadweb.org/viewtopic.php?f=10&t=7520&start=140
        // Partials:
        
        if (param == p1x()) 
            deriv += 2*(X_1 - X_2)*(sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) +
                pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2))*sqrt(pow(X_F1, 2) - 2*X_F1*X_c +
                pow(X_c, 2) + pow(Y_F1, 2) - 2*Y_F1*Y_c + pow(Y_c, 2) + pow(b, 2)) -
                sqrt(pow(X_1, 2)*(pow(X_c, 2) + pow(Y_c, 2)) - 2*X_1*X_2*(pow(X_c, 2) +
                pow(Y_c, 2)) + pow(X_2, 2)*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_F1,
                2)*(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2)) - 2*X_F1*(pow(X_1, 2)*X_c -
                2*X_1*X_2*X_c + pow(X_2, 2)*X_c) + pow(Y_1, 2)*(pow(X_2, 2) - 2*X_2*X_c
                + pow(X_c, 2) + pow(Y_c, 2)) + 2*Y_1*(X_1*X_2*Y_c - pow(X_2, 2)*Y_c -
                X_F1*(X_1*Y_c - X_2*Y_c)) + pow(Y_2, 2)*(pow(X_1, 2) - 2*X_1*X_c +
                pow(X_c, 2) + pow(Y_c, 2)) - 2*Y_2*(pow(X_1, 2)*Y_c - X_1*X_2*Y_c -
                X_F1*(X_1*Y_c - X_2*Y_c) + Y_1*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2)
                + pow(Y_c, 2))) + pow(Y_F1, 2)*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)) -
                2*Y_F1*(pow(Y_1, 2)*Y_c - Y_1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2)) +
                pow(Y_2, 2)*Y_c + Y_2*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2) -
                2*Y_1*Y_c))))/pow(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) -
                2*Y_1*Y_2 + pow(Y_2, 2), 3.0/2.0) - 2*((X_1 - X_2)*sqrt(pow(X_F1, 2) -
                2*X_F1*X_c + pow(X_c, 2) + pow(Y_F1, 2) - 2*Y_F1*Y_c + pow(Y_c, 2) +
                pow(b, 2))/sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) -
                2*Y_1*Y_2 + pow(Y_2, 2)) - (X_1*(pow(X_c, 2) + pow(Y_c, 2)) -
                X_2*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_F1, 2)*(X_1 - X_2) -
                2*X_F1*(X_1*X_c - X_2*X_c) + Y_1*(X_2*Y_c - X_F1*Y_c) + pow(Y_2, 2)*(X_1
                - X_c) - Y_2*(2*X_1*Y_c - X_2*Y_c - X_F1*Y_c + Y_1*(X_2 - X_c)) +
                Y_F1*(Y_1*(X_F1 - X_c) - Y_2*(X_F1 - X_c)))/sqrt(pow(X_1, 2)*(pow(X_c,
                2) + pow(Y_c, 2)) - 2*X_1*X_2*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_2,
                2)*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_F1, 2)*(pow(X_1, 2) - 2*X_1*X_2 +
                pow(X_2, 2)) - 2*X_F1*(pow(X_1, 2)*X_c - 2*X_1*X_2*X_c + pow(X_2,
                2)*X_c) + pow(Y_1, 2)*(pow(X_2, 2) - 2*X_2*X_c + pow(X_c, 2) + pow(Y_c,
                2)) + 2*Y_1*(X_1*X_2*Y_c - pow(X_2, 2)*Y_c - X_F1*(X_1*Y_c - X_2*Y_c)) +
                pow(Y_2, 2)*(pow(X_1, 2) - 2*X_1*X_c + pow(X_c, 2) + pow(Y_c, 2)) -
                2*Y_2*(pow(X_1, 2)*Y_c - X_1*X_2*Y_c - X_F1*(X_1*Y_c - X_2*Y_c) +
                Y_1*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2) + pow(Y_c, 2))) +
                pow(Y_F1, 2)*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)) - 2*Y_F1*(pow(Y_1,
                2)*Y_c - Y_1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2)) + pow(Y_2, 2)*Y_c +
                Y_2*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2) - 2*Y_1*Y_c))))/sqrt(pow(X_1,
                2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2));
        if (param == p1y()) 
            deriv += 2*(Y_1 - Y_2)*(sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) +
                pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2))*sqrt(pow(X_F1, 2) - 2*X_F1*X_c +
                pow(X_c, 2) + pow(Y_F1, 2) - 2*Y_F1*Y_c + pow(Y_c, 2) + pow(b, 2)) -
                sqrt(pow(X_1, 2)*(pow(X_c, 2) + pow(Y_c, 2)) - 2*X_1*X_2*(pow(X_c, 2) +
                pow(Y_c, 2)) + pow(X_2, 2)*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_F1,
                2)*(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2)) - 2*X_F1*(pow(X_1, 2)*X_c -
                2*X_1*X_2*X_c + pow(X_2, 2)*X_c) + pow(Y_1, 2)*(pow(X_2, 2) - 2*X_2*X_c
                + pow(X_c, 2) + pow(Y_c, 2)) + 2*Y_1*(X_1*X_2*Y_c - pow(X_2, 2)*Y_c -
                X_F1*(X_1*Y_c - X_2*Y_c)) + pow(Y_2, 2)*(pow(X_1, 2) - 2*X_1*X_c +
                pow(X_c, 2) + pow(Y_c, 2)) - 2*Y_2*(pow(X_1, 2)*Y_c - X_1*X_2*Y_c -
                X_F1*(X_1*Y_c - X_2*Y_c) + Y_1*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2)
                + pow(Y_c, 2))) + pow(Y_F1, 2)*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)) -
                2*Y_F1*(pow(Y_1, 2)*Y_c - Y_1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2)) +
                pow(Y_2, 2)*Y_c + Y_2*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2) -
                2*Y_1*Y_c))))/pow(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) -
                2*Y_1*Y_2 + pow(Y_2, 2), 3.0/2.0) - 2*((Y_1 - Y_2)*sqrt(pow(X_F1, 2) -
                2*X_F1*X_c + pow(X_c, 2) + pow(Y_F1, 2) - 2*Y_F1*Y_c + pow(Y_c, 2) +
                pow(b, 2))/sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) -
                2*Y_1*Y_2 + pow(Y_2, 2)) - (X_1*X_2*Y_c - pow(X_2, 2)*Y_c -
                X_F1*(X_1*Y_c - X_2*Y_c) + Y_1*(pow(X_2, 2) - 2*X_2*X_c + pow(X_c, 2) +
                pow(Y_c, 2)) - Y_2*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2) + pow(Y_c,
                2)) + pow(Y_F1, 2)*(Y_1 - Y_2) + Y_F1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 -
                X_2) - 2*Y_1*Y_c + 2*Y_2*Y_c))/sqrt(pow(X_1, 2)*(pow(X_c, 2) + pow(Y_c,
                2)) - 2*X_1*X_2*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_2, 2)*(pow(X_c, 2) +
                pow(Y_c, 2)) + pow(X_F1, 2)*(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2)) -
                2*X_F1*(pow(X_1, 2)*X_c - 2*X_1*X_2*X_c + pow(X_2, 2)*X_c) + pow(Y_1,
                2)*(pow(X_2, 2) - 2*X_2*X_c + pow(X_c, 2) + pow(Y_c, 2)) +
                2*Y_1*(X_1*X_2*Y_c - pow(X_2, 2)*Y_c - X_F1*(X_1*Y_c - X_2*Y_c)) +
                pow(Y_2, 2)*(pow(X_1, 2) - 2*X_1*X_c + pow(X_c, 2) + pow(Y_c, 2)) -
                2*Y_2*(pow(X_1, 2)*Y_c - X_1*X_2*Y_c - X_F1*(X_1*Y_c - X_2*Y_c) +
                Y_1*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2) + pow(Y_c, 2))) +
                pow(Y_F1, 2)*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)) - 2*Y_F1*(pow(Y_1,
                2)*Y_c - Y_1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2)) + pow(Y_2, 2)*Y_c +
                Y_2*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2) - 2*Y_1*Y_c))))/sqrt(pow(X_1,
                2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2));
       if (param == p2x()) 
            deriv += -2*(X_1 - X_2)*(sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) +
                pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2))*sqrt(pow(X_F1, 2) - 2*X_F1*X_c +
                pow(X_c, 2) + pow(Y_F1, 2) - 2*Y_F1*Y_c + pow(Y_c, 2) + pow(b, 2)) -
                sqrt(pow(X_1, 2)*(pow(X_c, 2) + pow(Y_c, 2)) - 2*X_1*X_2*(pow(X_c, 2) +
                pow(Y_c, 2)) + pow(X_2, 2)*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_F1,
                2)*(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2)) - 2*X_F1*(pow(X_1, 2)*X_c -
                2*X_1*X_2*X_c + pow(X_2, 2)*X_c) + pow(Y_1, 2)*(pow(X_2, 2) - 2*X_2*X_c
                + pow(X_c, 2) + pow(Y_c, 2)) + 2*Y_1*(X_1*X_2*Y_c - pow(X_2, 2)*Y_c -
                X_F1*(X_1*Y_c - X_2*Y_c)) + pow(Y_2, 2)*(pow(X_1, 2) - 2*X_1*X_c +
                pow(X_c, 2) + pow(Y_c, 2)) - 2*Y_2*(pow(X_1, 2)*Y_c - X_1*X_2*Y_c -
                X_F1*(X_1*Y_c - X_2*Y_c) + Y_1*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2)
                + pow(Y_c, 2))) + pow(Y_F1, 2)*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)) -
                2*Y_F1*(pow(Y_1, 2)*Y_c - Y_1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2)) +
                pow(Y_2, 2)*Y_c + Y_2*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2) -
                2*Y_1*Y_c))))/pow(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) -
                2*Y_1*Y_2 + pow(Y_2, 2), 3.0/2.0) + 2*((X_1 - X_2)*sqrt(pow(X_F1, 2) -
                2*X_F1*X_c + pow(X_c, 2) + pow(Y_F1, 2) - 2*Y_F1*Y_c + pow(Y_c, 2) +
                pow(b, 2))/sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) -
                2*Y_1*Y_2 + pow(Y_2, 2)) - (X_1*(pow(X_c, 2) + pow(Y_c, 2)) -
                X_2*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_F1, 2)*(X_1 - X_2) -
                2*X_F1*(X_1*X_c - X_2*X_c) - pow(Y_1, 2)*(X_2 - X_c) - Y_1*(X_1*Y_c -
                2*X_2*Y_c + X_F1*Y_c) + Y_2*(-X_1*Y_c + X_F1*Y_c + Y_1*(X_1 - X_c)) +
                Y_F1*(Y_1*(X_F1 - X_c) - Y_2*(X_F1 - X_c)))/sqrt(pow(X_1, 2)*(pow(X_c,
                2) + pow(Y_c, 2)) - 2*X_1*X_2*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_2,
                2)*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_F1, 2)*(pow(X_1, 2) - 2*X_1*X_2 +
                pow(X_2, 2)) - 2*X_F1*(pow(X_1, 2)*X_c - 2*X_1*X_2*X_c + pow(X_2,
                2)*X_c) + pow(Y_1, 2)*(pow(X_2, 2) - 2*X_2*X_c + pow(X_c, 2) + pow(Y_c,
                2)) + 2*Y_1*(X_1*X_2*Y_c - pow(X_2, 2)*Y_c - X_F1*(X_1*Y_c - X_2*Y_c)) +
                pow(Y_2, 2)*(pow(X_1, 2) - 2*X_1*X_c + pow(X_c, 2) + pow(Y_c, 2)) -
                2*Y_2*(pow(X_1, 2)*Y_c - X_1*X_2*Y_c - X_F1*(X_1*Y_c - X_2*Y_c) +
                Y_1*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2) + pow(Y_c, 2))) +
                pow(Y_F1, 2)*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)) - 2*Y_F1*(pow(Y_1,
                2)*Y_c - Y_1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2)) + pow(Y_2, 2)*Y_c +
                Y_2*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2) - 2*Y_1*Y_c))))/sqrt(pow(X_1,
                2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2));
        if (param == p2y()) 
            deriv += -2*(Y_1 - Y_2)*(sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) +
                pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2))*sqrt(pow(X_F1, 2) - 2*X_F1*X_c +
                pow(X_c, 2) + pow(Y_F1, 2) - 2*Y_F1*Y_c + pow(Y_c, 2) + pow(b, 2)) -
                sqrt(pow(X_1, 2)*(pow(X_c, 2) + pow(Y_c, 2)) - 2*X_1*X_2*(pow(X_c, 2) +
                pow(Y_c, 2)) + pow(X_2, 2)*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_F1,
                2)*(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2)) - 2*X_F1*(pow(X_1, 2)*X_c -
                2*X_1*X_2*X_c + pow(X_2, 2)*X_c) + pow(Y_1, 2)*(pow(X_2, 2) - 2*X_2*X_c
                + pow(X_c, 2) + pow(Y_c, 2)) + 2*Y_1*(X_1*X_2*Y_c - pow(X_2, 2)*Y_c -
                X_F1*(X_1*Y_c - X_2*Y_c)) + pow(Y_2, 2)*(pow(X_1, 2) - 2*X_1*X_c +
                pow(X_c, 2) + pow(Y_c, 2)) - 2*Y_2*(pow(X_1, 2)*Y_c - X_1*X_2*Y_c -
                X_F1*(X_1*Y_c - X_2*Y_c) + Y_1*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2)
                + pow(Y_c, 2))) + pow(Y_F1, 2)*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)) -
                2*Y_F1*(pow(Y_1, 2)*Y_c - Y_1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2)) +
                pow(Y_2, 2)*Y_c + Y_2*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2) -
                2*Y_1*Y_c))))/pow(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) -
                2*Y_1*Y_2 + pow(Y_2, 2), 3.0/2.0) + 2*((Y_1 - Y_2)*sqrt(pow(X_F1, 2) -
                2*X_F1*X_c + pow(X_c, 2) + pow(Y_F1, 2) - 2*Y_F1*Y_c + pow(Y_c, 2) +
                pow(b, 2))/sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) -
                2*Y_1*Y_2 + pow(Y_2, 2)) - (pow(X_1, 2)*Y_c - X_1*X_2*Y_c -
                X_F1*(X_1*Y_c - X_2*Y_c) + Y_1*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2)
                + pow(Y_c, 2)) - Y_2*(pow(X_1, 2) - 2*X_1*X_c + pow(X_c, 2) + pow(Y_c,
                2)) + pow(Y_F1, 2)*(Y_1 - Y_2) + Y_F1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 -
                X_2) - 2*Y_1*Y_c + 2*Y_2*Y_c))/sqrt(pow(X_1, 2)*(pow(X_c, 2) + pow(Y_c,
                2)) - 2*X_1*X_2*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_2, 2)*(pow(X_c, 2) +
                pow(Y_c, 2)) + pow(X_F1, 2)*(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2)) -
                2*X_F1*(pow(X_1, 2)*X_c - 2*X_1*X_2*X_c + pow(X_2, 2)*X_c) + pow(Y_1,
                2)*(pow(X_2, 2) - 2*X_2*X_c + pow(X_c, 2) + pow(Y_c, 2)) +
                2*Y_1*(X_1*X_2*Y_c - pow(X_2, 2)*Y_c - X_F1*(X_1*Y_c - X_2*Y_c)) +
                pow(Y_2, 2)*(pow(X_1, 2) - 2*X_1*X_c + pow(X_c, 2) + pow(Y_c, 2)) -
                2*Y_2*(pow(X_1, 2)*Y_c - X_1*X_2*Y_c - X_F1*(X_1*Y_c - X_2*Y_c) +
                Y_1*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2) + pow(Y_c, 2))) +
                pow(Y_F1, 2)*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)) - 2*Y_F1*(pow(Y_1,
                2)*Y_c - Y_1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2)) + pow(Y_2, 2)*Y_c +
                Y_2*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2) - 2*Y_1*Y_c))))/sqrt(pow(X_1,
                2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2));
        if (param == f1x()) 
            deriv += -2*((X_F1 - X_c)*sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) +
                pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2))/sqrt(pow(X_F1, 2) - 2*X_F1*X_c +
                pow(X_c, 2) + pow(Y_F1, 2) - 2*Y_F1*Y_c + pow(Y_c, 2) + pow(b, 2)) +
                (pow(X_1, 2)*X_c - 2*X_1*X_2*X_c + pow(X_2, 2)*X_c - X_F1*(pow(X_1, 2) -
                2*X_1*X_2 + pow(X_2, 2)) + Y_1*(X_1*Y_c - X_2*Y_c) - Y_2*(X_1*Y_c -
                X_2*Y_c) - Y_F1*(Y_1*(X_1 - X_2) - Y_2*(X_1 - X_2)))/sqrt(pow(X_1,
                2)*(pow(X_c, 2) + pow(Y_c, 2)) - 2*X_1*X_2*(pow(X_c, 2) + pow(Y_c, 2)) +
                pow(X_2, 2)*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_F1, 2)*(pow(X_1, 2) -
                2*X_1*X_2 + pow(X_2, 2)) - 2*X_F1*(pow(X_1, 2)*X_c - 2*X_1*X_2*X_c +
                pow(X_2, 2)*X_c) + pow(Y_1, 2)*(pow(X_2, 2) - 2*X_2*X_c + pow(X_c, 2) +
                pow(Y_c, 2)) + 2*Y_1*(X_1*X_2*Y_c - pow(X_2, 2)*Y_c - X_F1*(X_1*Y_c -
                X_2*Y_c)) + pow(Y_2, 2)*(pow(X_1, 2) - 2*X_1*X_c + pow(X_c, 2) +
                pow(Y_c, 2)) - 2*Y_2*(pow(X_1, 2)*Y_c - X_1*X_2*Y_c - X_F1*(X_1*Y_c -
                X_2*Y_c) + Y_1*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2) + pow(Y_c, 2)))
                + pow(Y_F1, 2)*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)) -
                2*Y_F1*(pow(Y_1, 2)*Y_c - Y_1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2)) +
                pow(Y_2, 2)*Y_c + Y_2*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2) -
                2*Y_1*Y_c))))/sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) -
                2*Y_1*Y_2 + pow(Y_2, 2));
        if (param == f1y()) 
            deriv +=-2*((Y_F1 - Y_c)*sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) +
                pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2))/sqrt(pow(X_F1, 2) - 2*X_F1*X_c +
                pow(X_c, 2) + pow(Y_F1, 2) - 2*Y_F1*Y_c + pow(Y_c, 2) + pow(b, 2)) +
                (pow(Y_1, 2)*Y_c - Y_1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2)) +
                pow(Y_2, 2)*Y_c + Y_2*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2) -
                2*Y_1*Y_c) - Y_F1*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)))/sqrt(pow(X_1,
                2)*(pow(X_c, 2) + pow(Y_c, 2)) - 2*X_1*X_2*(pow(X_c, 2) + pow(Y_c, 2)) +
                pow(X_2, 2)*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_F1, 2)*(pow(X_1, 2) -
                2*X_1*X_2 + pow(X_2, 2)) - 2*X_F1*(pow(X_1, 2)*X_c - 2*X_1*X_2*X_c +
                pow(X_2, 2)*X_c) + pow(Y_1, 2)*(pow(X_2, 2) - 2*X_2*X_c + pow(X_c, 2) +
                pow(Y_c, 2)) + 2*Y_1*(X_1*X_2*Y_c - pow(X_2, 2)*Y_c - X_F1*(X_1*Y_c -
                X_2*Y_c)) + pow(Y_2, 2)*(pow(X_1, 2) - 2*X_1*X_c + pow(X_c, 2) +
                pow(Y_c, 2)) - 2*Y_2*(pow(X_1, 2)*Y_c - X_1*X_2*Y_c - X_F1*(X_1*Y_c -
                X_2*Y_c) + Y_1*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2) + pow(Y_c, 2)))
                + pow(Y_F1, 2)*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)) -
                2*Y_F1*(pow(Y_1, 2)*Y_c - Y_1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2)) +
                pow(Y_2, 2)*Y_c + Y_2*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2) -
                2*Y_1*Y_c))))/sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) -
                2*Y_1*Y_2 + pow(Y_2, 2));
        if (param == cx()) 
            deriv += 2*((X_F1 - X_c)*sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) +
                pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2))/sqrt(pow(X_F1, 2) - 2*X_F1*X_c +
                pow(X_c, 2) + pow(Y_F1, 2) - 2*Y_F1*Y_c + pow(Y_c, 2) + pow(b, 2)) +
                (pow(X_1, 2)*X_c - 2*X_1*X_2*X_c + pow(X_2, 2)*X_c - X_F1*(pow(X_1, 2) -
                2*X_1*X_2 + pow(X_2, 2)) - pow(Y_1, 2)*(X_2 - X_c) + Y_1*Y_2*(X_1 + X_2
                - 2*X_c) - pow(Y_2, 2)*(X_1 - X_c) - Y_F1*(Y_1*(X_1 - X_2) - Y_2*(X_1 -
                X_2)))/sqrt(pow(X_1, 2)*(pow(X_c, 2) + pow(Y_c, 2)) -
                2*X_1*X_2*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_2, 2)*(pow(X_c, 2) +
                pow(Y_c, 2)) + pow(X_F1, 2)*(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2)) -
                2*X_F1*(pow(X_1, 2)*X_c - 2*X_1*X_2*X_c + pow(X_2, 2)*X_c) + pow(Y_1,
                2)*(pow(X_2, 2) - 2*X_2*X_c + pow(X_c, 2) + pow(Y_c, 2)) +
                2*Y_1*(X_1*X_2*Y_c - pow(X_2, 2)*Y_c - X_F1*(X_1*Y_c - X_2*Y_c)) +
                pow(Y_2, 2)*(pow(X_1, 2) - 2*X_1*X_c + pow(X_c, 2) + pow(Y_c, 2)) -
                2*Y_2*(pow(X_1, 2)*Y_c - X_1*X_2*Y_c - X_F1*(X_1*Y_c - X_2*Y_c) +
                Y_1*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2) + pow(Y_c, 2))) +
                pow(Y_F1, 2)*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)) - 2*Y_F1*(pow(Y_1,
                2)*Y_c - Y_1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2)) + pow(Y_2, 2)*Y_c +
                Y_2*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2) - 2*Y_1*Y_c))))/sqrt(pow(X_1,
                2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2));
        if (param == cy()) 
            deriv += 2*((Y_F1 - Y_c)*sqrt(pow(X_1, 2) - 2*X_1*X_2 + pow(X_2, 2) +
                pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2))/sqrt(pow(X_F1, 2) - 2*X_F1*X_c +
                pow(X_c, 2) + pow(Y_F1, 2) - 2*Y_F1*Y_c + pow(Y_c, 2) + pow(b, 2)) +
                (pow(X_1, 2)*Y_c - 2*X_1*X_2*Y_c + pow(X_2, 2)*Y_c + pow(Y_1, 2)*Y_c +
                Y_1*(X_1*X_2 - pow(X_2, 2) - X_F1*(X_1 - X_2)) + pow(Y_2, 2)*Y_c -
                Y_2*(pow(X_1, 2) - X_1*X_2 - X_F1*(X_1 - X_2) + 2*Y_1*Y_c) -
                Y_F1*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)))/sqrt(pow(X_1, 2)*(pow(X_c,
                2) + pow(Y_c, 2)) - 2*X_1*X_2*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_2,
                2)*(pow(X_c, 2) + pow(Y_c, 2)) + pow(X_F1, 2)*(pow(X_1, 2) - 2*X_1*X_2 +
                pow(X_2, 2)) - 2*X_F1*(pow(X_1, 2)*X_c - 2*X_1*X_2*X_c + pow(X_2,
                2)*X_c) + pow(Y_1, 2)*(pow(X_2, 2) - 2*X_2*X_c + pow(X_c, 2) + pow(Y_c,
                2)) + 2*Y_1*(X_1*X_2*Y_c - pow(X_2, 2)*Y_c - X_F1*(X_1*Y_c - X_2*Y_c)) +
                pow(Y_2, 2)*(pow(X_1, 2) - 2*X_1*X_c + pow(X_c, 2) + pow(Y_c, 2)) -
                2*Y_2*(pow(X_1, 2)*Y_c - X_1*X_2*Y_c - X_F1*(X_1*Y_c - X_2*Y_c) +
                Y_1*(-X_1*X_c + X_2*(X_1 - X_c) + pow(X_c, 2) + pow(Y_c, 2))) +
                pow(Y_F1, 2)*(pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2)) - 2*Y_F1*(pow(Y_1,
                2)*Y_c - Y_1*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2)) + pow(Y_2, 2)*Y_c +
                Y_2*(-X_1*X_c + X_2*X_c + X_F1*(X_1 - X_2) - 2*Y_1*Y_c))))/sqrt(pow(X_1,
                2) - 2*X_1*X_2 + pow(X_2, 2) + pow(Y_1, 2) - 2*Y_1*Y_2 + pow(Y_2, 2));
        if (param == rmin()) 
            deriv += -2*b/sqrt(pow(X_F1, 2) - 2*X_F1*X_c + pow(X_c, 2) + pow(Y_F1,
                2) - 2*Y_F1*Y_c + pow(Y_c, 2) + pow(b, 2));
    }
    return scale * deriv;
}

// ConstraintInternalAlignmentPoint2Ellipse
ConstraintInternalAlignmentPoint2Ellipse::ConstraintInternalAlignmentPoint2Ellipse(Ellipse &e, Point &p1, InternalAlignmentType alignmentType)
{
    pvec.push_back(p1.x);
    pvec.push_back(p1.y);
    pvec.push_back(e.center.x);
    pvec.push_back(e.center.y);
    pvec.push_back(e.focus1X);
    pvec.push_back(e.focus1Y);
    pvec.push_back(e.radmin);
    origpvec = pvec;
    rescale();
    AlignmentType=alignmentType;
}

ConstraintInternalAlignmentPoint2Ellipse::ConstraintInternalAlignmentPoint2Ellipse(ArcOfEllipse &a, Point &p1, InternalAlignmentType alignmentType)
{
    pvec.push_back(p1.x);
    pvec.push_back(p1.y);
    pvec.push_back(a.center.x);
    pvec.push_back(a.center.y);
    pvec.push_back(a.focus1X);
    pvec.push_back(a.focus1Y);
    pvec.push_back(a.radmin);
    origpvec = pvec;
    rescale();
    AlignmentType=alignmentType;
}

ConstraintType ConstraintInternalAlignmentPoint2Ellipse::getTypeId()
{
    return InternalAlignmentPoint2Ellipse;
}

void ConstraintInternalAlignmentPoint2Ellipse::rescale(double coef)
{
    scale = coef * 1;
}

double ConstraintInternalAlignmentPoint2Ellipse::error()
{    
    // so first is to select the point (X_0,Y_0) in line to calculate
    double X_1 = *p1x();
    double Y_1 = *p1y();
    double X_c = *cx();
    double Y_c = *cy();
    double X_F1 = *f1x();
    double Y_F1 = *f1y(); 
    double b = *rmin();
    
    switch(AlignmentType)
    {
        case EllipsePositiveMajorX:
            return scale * (X_1 - X_c - (X_F1 - X_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)));
            break;
        case EllipsePositiveMajorY:
            return scale * (Y_1 - Y_c - (Y_F1 - Y_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)));
            break;
        case EllipseNegativeMajorX:
            return scale * (X_1 - X_c + (X_F1 - X_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)));
            break;        
        case EllipseNegativeMajorY:
            return scale * (Y_1 - Y_c + (Y_F1 - Y_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)));
            break;        
        case EllipsePositiveMinorX:
            return scale * (X_1 - X_c + b*(Y_F1 - Y_c)/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1
                - Y_c, 2)));
            break;        
        case EllipsePositiveMinorY:
            return scale * (Y_1 - Y_c - b*(X_F1 - X_c)/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1
                - Y_c, 2)));
            break;        
        case EllipseNegativeMinorX:
            return scale * (X_1 - X_c - b*(Y_F1 - Y_c)/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1
                - Y_c, 2)));
            break;        
        case EllipseNegativeMinorY:
            return scale * (Y_1 - Y_c + b*(X_F1 - X_c)/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1
                - Y_c, 2)));
            break;        
        case EllipseFocus2X:
            return scale * (X_1 + X_F1 - 2*X_c);
            break;        
        case EllipseFocus2Y:
            return scale * (Y_1 + Y_F1 - 2*Y_c);
            break; 
        default:
            return 0;
    }
}

double ConstraintInternalAlignmentPoint2Ellipse::grad(double *param)
{      
    double deriv=0.;
    if (param == p1x() || param == p1y() ||
        param == f1x() || param == f1y() ||
        param == cx() || param == cy() ||
        param == rmin()) {
                
        double X_1 = *p1x();
        double Y_1 = *p1y();
        double X_c = *cx();
        double Y_c = *cy();
        double X_F1 = *f1x();
        double Y_F1 = *f1y(); 
        double b = *rmin();
                
        if (param == p1x())
            switch(AlignmentType)
            {
                case EllipsePositiveMajorX:
                    deriv += 1;
                    break;
                case EllipsePositiveMajorY:
                    deriv += 0;
                    break;
                case EllipseNegativeMajorX:
                    deriv += 1;
                    break;        
                case EllipseNegativeMajorY:
                    deriv += 0;
                    break;        
                case EllipsePositiveMinorX:
                    deriv += 1;
                    break;        
                case EllipsePositiveMinorY:
                    deriv += 0;
                    break;        
                case EllipseNegativeMinorX:
                    deriv += 1;
                    break;        
                case EllipseNegativeMinorY:
                    deriv += 0;
                    break;        
                case EllipseFocus2X:
                    deriv += 1;                    
                    break;        
                case EllipseFocus2Y:
                    deriv += 0;
                    break; 
                default:
                    deriv+=0;
            }
        if (param == p1y()) 
           switch(AlignmentType)
            {
                case EllipsePositiveMajorX:
                    deriv += 0;
                    break;
                case EllipsePositiveMajorY:
                    deriv += 1;
                    break;
                case EllipseNegativeMajorX:
                    deriv += 0;
                    break;        
                case EllipseNegativeMajorY:
                    deriv += 1;
                    break;        
                case EllipsePositiveMinorX:
                    deriv += 0;
                    break;        
                case EllipsePositiveMinorY:
                    deriv += 1;
                    break;        
                case EllipseNegativeMinorX:
                    deriv += 0;
                    break;        
                case EllipseNegativeMinorY:
                    deriv += 1;
                    break;        
                case EllipseFocus2X:
                    deriv += 0;
                    break;        
                case EllipseFocus2Y:
                    deriv += 1; 
                    break; 
                default:
                    deriv+=0;
            }
        if (param == f1x()) 
            switch(AlignmentType)
            {
                case EllipsePositiveMajorX:
                    deriv += -pow(X_F1 - X_c, 2)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) +
                        pow(X_F1 - X_c, 2)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0) -
                        sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1
                        - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;
                case EllipsePositiveMajorY:
                    deriv += -(X_F1 - X_c)*(Y_F1 - Y_c)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) +
                        (X_F1 - X_c)*(Y_F1 - Y_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0);
                    break;
                case EllipseNegativeMajorX:
                    deriv += pow(X_F1 - X_c, 2)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) -
                        pow(X_F1 - X_c, 2)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0) +
                        sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1
                        - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipseNegativeMajorY:
                    deriv += (X_F1 - X_c)*(Y_F1 - Y_c)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) -
                        (X_F1 - X_c)*(Y_F1 - Y_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0);
                    break;        
                case EllipsePositiveMinorX:
                    deriv += -b*(X_F1 - X_c)*(Y_F1 - Y_c)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2), 3.0/2.0);
                    break;        
                case EllipsePositiveMinorY:
                    deriv += b*pow(X_F1 - X_c, 2)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2), 3.0/2.0) - b/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipseNegativeMinorX:
                    deriv += b*(X_F1 - X_c)*(Y_F1 - Y_c)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2), 3.0/2.0);
                    break;        
                case EllipseNegativeMinorY:
                    deriv += -b*pow(X_F1 - X_c, 2)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2), 3.0/2.0) + b/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipseFocus2X:
                    deriv += 1;                     
                    break;        
                case EllipseFocus2Y:
                    deriv+=0;                    
                    break; 
                default:
                    deriv+=0;
            }
        if (param == f1y()) 
           switch(AlignmentType)
            {
                case EllipsePositiveMajorX:
                    deriv += -(X_F1 - X_c)*(Y_F1 - Y_c)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) +
                        (X_F1 - X_c)*(Y_F1 - Y_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0);
                    break;
                case EllipsePositiveMajorY:
                    deriv += -pow(Y_F1 - Y_c, 2)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) +
                        pow(Y_F1 - Y_c, 2)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0) -
                        sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1
                        - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;
                case EllipseNegativeMajorX:
                    deriv += (X_F1 - X_c)*(Y_F1 - Y_c)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) -
                        (X_F1 - X_c)*(Y_F1 - Y_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0);
                    break;        
                case EllipseNegativeMajorY:
                    deriv += pow(Y_F1 - Y_c, 2)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) -
                        pow(Y_F1 - Y_c, 2)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0) +
                        sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1
                        - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipsePositiveMinorX:
                    deriv += -b*pow(Y_F1 - Y_c, 2)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2), 3.0/2.0) + b/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipsePositiveMinorY:
                    deriv += b*(X_F1 - X_c)*(Y_F1 - Y_c)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2), 3.0/2.0);
                    break;        
                case EllipseNegativeMinorX:
                    deriv += b*pow(Y_F1 - Y_c, 2)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2), 3.0/2.0) - b/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipseNegativeMinorY:
                    deriv += -b*(X_F1 - X_c)*(Y_F1 - Y_c)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2), 3.0/2.0);
                    break;        
                case EllipseFocus2X:
                    deriv += 0;                    
                    break;        
                case EllipseFocus2Y:
                    deriv += 1;  
                    break; 
                default:
                    deriv+=0;
            }
        if (param == cx()) 
            switch(AlignmentType)
            {
                case EllipsePositiveMajorX:
                    deriv += pow(X_F1 - X_c, 2)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) -
                        pow(X_F1 - X_c, 2)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0) - 1 +
                        sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1
                        - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;
                case EllipsePositiveMajorY:
                    deriv += (X_F1 - X_c)*(Y_F1 - Y_c)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) -
                        (X_F1 - X_c)*(Y_F1 - Y_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0);
                    break;
                case EllipseNegativeMajorX:
                    deriv += -pow(X_F1 - X_c, 2)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) +
                        pow(X_F1 - X_c, 2)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0) - 1 -
                        sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1
                        - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipseNegativeMajorY:
                    deriv += -(X_F1 - X_c)*(Y_F1 - Y_c)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) +
                        (X_F1 - X_c)*(Y_F1 - Y_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0);
                    break;        
                case EllipsePositiveMinorX:
                    deriv += b*(X_F1 - X_c)*(Y_F1 - Y_c)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2), 3.0/2.0) - 1;
                    break;        
                case EllipsePositiveMinorY:
                    deriv += -b*pow(X_F1 - X_c, 2)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2), 3.0/2.0) + b/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipseNegativeMinorX:
                    deriv += -b*(X_F1 - X_c)*(Y_F1 - Y_c)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2), 3.0/2.0) - 1;
                    break;        
                case EllipseNegativeMinorY:
                    deriv += b*pow(X_F1 - X_c, 2)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2), 3.0/2.0) - b/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipseFocus2X:
                    deriv += -2;                    
                    break;        
                case EllipseFocus2Y:
                    deriv+=0;                    
                    break; 
                default:
                    deriv+=0;
            }
        if (param == cy()) 
            switch(AlignmentType)
            {
                case EllipsePositiveMajorX:
                    deriv += (X_F1 - X_c)*(Y_F1 - Y_c)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) -
                        (X_F1 - X_c)*(Y_F1 - Y_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0);
                    break;
                case EllipsePositiveMajorY:
                    deriv += pow(Y_F1 - Y_c, 2)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) -
                        pow(Y_F1 - Y_c, 2)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0) - 1 +
                        sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1
                        - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;
                case EllipseNegativeMajorX:
                    deriv += -(X_F1 - X_c)*(Y_F1 - Y_c)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) +
                        (X_F1 - X_c)*(Y_F1 - Y_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0);
                    break;        
                case EllipseNegativeMajorY:
                    deriv += -pow(Y_F1 - Y_c, 2)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))) +
                        pow(Y_F1 - Y_c, 2)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0) - 1 -
                        sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1
                        - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipsePositiveMinorX:
                    deriv += b*pow(Y_F1 - Y_c, 2)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2), 3.0/2.0) - b/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipsePositiveMinorY:
                    deriv += -b*(X_F1 - X_c)*(Y_F1 - Y_c)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2), 3.0/2.0) - 1;
                    break;        
                case EllipseNegativeMinorX:
                    deriv += -b*pow(Y_F1 - Y_c, 2)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2), 3.0/2.0) + b/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipseNegativeMinorY:
                    deriv += b*(X_F1 - X_c)*(Y_F1 - Y_c)/pow(pow(X_F1 - X_c, 2) + pow(Y_F1
                        - Y_c, 2), 3.0/2.0) - 1;
                    break;        
                case EllipseFocus2X:
                    deriv += 0;                    
                    break;        
                case EllipseFocus2Y:
                    deriv += -2;                     
                    break; 
                default:
                    deriv+=0;
            }
        if (param == rmin()) 
            switch(AlignmentType)
            {
                case EllipsePositiveMajorX:
                    deriv += -b*(X_F1 - X_c)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)));
                    break;
                case EllipsePositiveMajorY:
                    deriv += -b*(Y_F1 - Y_c)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)));
                    break;
                case EllipseNegativeMajorX:
                    deriv += b*(X_F1 - X_c)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)));
                    break;        
                case EllipseNegativeMajorY:
                    deriv += b*(Y_F1 - Y_c)/(sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                        2))*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)));
                    break;        
                case EllipsePositiveMinorX:
                    deriv += (Y_F1 - Y_c)/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipsePositiveMinorY:
                    deriv += -(X_F1 - X_c)/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipseNegativeMinorX:
                    deriv += -(Y_F1 - Y_c)/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipseNegativeMinorY:
                    deriv += (X_F1 - X_c)/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
                    break;        
                case EllipseFocus2X:
                    deriv += 0;                    
                    break;        
                case EllipseFocus2Y:
                    deriv += 0;                    
                    break; 
                default:
                    deriv+=0;
            }
    }
    return scale * deriv;
}

//  ConstraintEqualMajorAxesEllipse
ConstraintEqualMajorAxesEllipse:: ConstraintEqualMajorAxesEllipse(Ellipse &e1, Ellipse &e2)
{
    pvec.push_back(e1.center.x);
    pvec.push_back(e1.center.y);
    pvec.push_back(e1.focus1X);
    pvec.push_back(e1.focus1Y);
    pvec.push_back(e1.radmin);
    pvec.push_back(e2.center.x);
    pvec.push_back(e2.center.y);
    pvec.push_back(e2.focus1X);
    pvec.push_back(e2.focus1Y);
    pvec.push_back(e2.radmin);
    origpvec = pvec;
    rescale();
}

ConstraintEqualMajorAxesEllipse:: ConstraintEqualMajorAxesEllipse(ArcOfEllipse &a1, Ellipse &e2)
{
    pvec.push_back(a1.center.x);
    pvec.push_back(a1.center.y);
    pvec.push_back(a1.focus1X);
    pvec.push_back(a1.focus1Y);
    pvec.push_back(a1.radmin);
    pvec.push_back(e2.center.x);
    pvec.push_back(e2.center.y);
    pvec.push_back(e2.focus1X);
    pvec.push_back(e2.focus1Y);
    pvec.push_back(e2.radmin);
    origpvec = pvec;
    rescale();
}

ConstraintEqualMajorAxesEllipse:: ConstraintEqualMajorAxesEllipse(ArcOfEllipse &a1, ArcOfEllipse &a2)
{
    pvec.push_back(a1.center.x);
    pvec.push_back(a1.center.y);
    pvec.push_back(a1.focus1X);
    pvec.push_back(a1.focus1Y);
    pvec.push_back(a1.radmin);
    pvec.push_back(a2.center.x);
    pvec.push_back(a2.center.y);
    pvec.push_back(a2.focus1X);
    pvec.push_back(a2.focus1Y);
    pvec.push_back(a2.radmin);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintEqualMajorAxesEllipse::getTypeId()
{
    return EqualMajorAxesEllipse;
}

void ConstraintEqualMajorAxesEllipse::rescale(double coef)
{
    scale = coef * 1;
}

double ConstraintEqualMajorAxesEllipse::error()
{    
    double E1X_c = *e1cx();
    double E1Y_c = *e1cy();     
    double E1X_F1 = *e1f1x();
    double E1Y_F1 = *e1f1y();
    double E1b = *e1rmin();
    double E2X_c = *e2cx();
    double E2Y_c = *e2cy();     
    double E2X_F1 = *e2f1x();
    double E2Y_F1 = *e2f1y();
    double E2b = *e2rmin();
    
    double err=sqrt(pow(E1X_F1, 2) - 2*E1X_F1*E1X_c + pow(E1X_c, 2) +
        pow(E1Y_F1, 2) - 2*E1Y_F1*E1Y_c + pow(E1Y_c, 2) + pow(E1b, 2)) -
        sqrt(pow(E2X_F1, 2) - 2*E2X_F1*E2X_c + pow(E2X_c, 2) + pow(E2Y_F1, 2) -
        2*E2Y_F1*E2Y_c + pow(E2Y_c, 2) + pow(E2b, 2));
    return scale * err;
}

double ConstraintEqualMajorAxesEllipse::grad(double *param)
{
    double deriv=0.;
    if (param == e1f1x() || param == e1f1y() ||
        param == e1cx() || param == e1cy() ||
        param == e1rmin() ||
        param == e2f1x() || param == e2f1y() ||
        param == e2cx() || param == e2cy() ||
        param == e2rmin()) {

        double E1X_c = *e1cx();
        double E1Y_c = *e1cy();     
        double E1X_F1 = *e1f1x();
        double E1Y_F1 = *e1f1y();
        double E1b = *e1rmin();
        double E2X_c = *e2cx();
        double E2Y_c = *e2cy();     
        double E2X_F1 = *e2f1x();
        double E2Y_F1 = *e2f1y();
        double E2b = *e2rmin();

        if (param == e1cx())
            deriv += -(E1X_F1 - E1X_c)/sqrt(pow(E1X_F1, 2) - 2*E1X_F1*E1X_c +
                pow(E1X_c, 2) + pow(E1Y_F1, 2) - 2*E1Y_F1*E1Y_c + pow(E1Y_c, 2) +
                pow(E1b, 2));
        if (param == e2cx())
            deriv += (E2X_F1 - E2X_c)/sqrt(pow(E2X_F1, 2) - 2*E2X_F1*E2X_c +
                pow(E2X_c, 2) + pow(E2Y_F1, 2) - 2*E2Y_F1*E2Y_c + pow(E2Y_c, 2) +
                pow(E2b, 2));
        if (param == e1cy())
            deriv += -(E1Y_F1 - E1Y_c)/sqrt(pow(E1X_F1, 2) - 2*E1X_F1*E1X_c +
                pow(E1X_c, 2) + pow(E1Y_F1, 2) - 2*E1Y_F1*E1Y_c + pow(E1Y_c, 2) +
                pow(E1b, 2));
        if (param == e2cy())
            deriv +=(E2Y_F1 - E2Y_c)/sqrt(pow(E2X_F1, 2) - 2*E2X_F1*E2X_c +
                pow(E2X_c, 2) + pow(E2Y_F1, 2) - 2*E2Y_F1*E2Y_c + pow(E2Y_c, 2) +
                pow(E2b, 2));
        if (param == e1f1x())
            deriv += (E1X_F1 - E1X_c)/sqrt(pow(E1X_F1, 2) - 2*E1X_F1*E1X_c +
                pow(E1X_c, 2) + pow(E1Y_F1, 2) - 2*E1Y_F1*E1Y_c + pow(E1Y_c, 2) +
                pow(E1b, 2));
        if (param == e2f1x())
            deriv +=-(E2X_F1 - E2X_c)/sqrt(pow(E2X_F1, 2) - 2*E2X_F1*E2X_c +
                pow(E2X_c, 2) + pow(E2Y_F1, 2) - 2*E2Y_F1*E2Y_c + pow(E2Y_c, 2) +
                pow(E2b, 2));
        if (param == e1f1y())
            deriv += (E1Y_F1 - E1Y_c)/sqrt(pow(E1X_F1, 2) - 2*E1X_F1*E1X_c +
                pow(E1X_c, 2) + pow(E1Y_F1, 2) - 2*E1Y_F1*E1Y_c + pow(E1Y_c, 2) +
                pow(E1b, 2));
        if (param == e2f1y())
            deriv +=-(E2Y_F1 - E2Y_c)/sqrt(pow(E2X_F1, 2) - 2*E2X_F1*E2X_c +
                pow(E2X_c, 2) + pow(E2Y_F1, 2) - 2*E2Y_F1*E2Y_c + pow(E2Y_c, 2) +
                pow(E2b, 2));            
        if (param == e1rmin())
            deriv += E1b/sqrt(pow(E1X_F1, 2) - 2*E1X_F1*E1X_c + pow(E1X_c, 2) +
                pow(E1Y_F1, 2) - 2*E1Y_F1*E1Y_c + pow(E1Y_c, 2) + pow(E1b, 2));
       if (param == e2rmin())
            deriv += -E2b/sqrt(pow(E2X_F1, 2) - 2*E2X_F1*E2X_c + pow(E2X_c, 2) +
                pow(E2Y_F1, 2) - 2*E2Y_F1*E2Y_c + pow(E2Y_c, 2) + pow(E2b, 2));         
    }
    return scale * deriv;
}

// EllipticalArcRangeToEndPoints
ConstraintEllipticalArcRangeToEndPoints::ConstraintEllipticalArcRangeToEndPoints(Point &p, ArcOfEllipse &a, double *angle_t)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(angle_t);
    pvec.push_back(a.center.x);
    pvec.push_back(a.center.y);
    pvec.push_back(a.focus1X);
    pvec.push_back(a.focus1Y);
    pvec.push_back(a.radmin);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintEllipticalArcRangeToEndPoints::getTypeId()
{
    return EllipticalArcRangeToEndPoints;
}

void ConstraintEllipticalArcRangeToEndPoints::rescale(double coef)
{
    scale = coef * 1;
}

double ConstraintEllipticalArcRangeToEndPoints::error()
{    
    double X_0 = *p1x();
    double Y_0 = *p1y();
    double X_c = *cx();
    double Y_c = *cy();     
    double X_F1 = *f1x();
    double Y_F1 = *f1y();
    double b = *rmin();
    double alpha_t = *angle();
    
    double err=atan((((X_0 - X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 -
        Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
        2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
        Y_c))*cos(alpha_t)/b)/(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 -
        Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
        2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
        Y_c))*sin(alpha_t)/b));
    return scale * err;
}

double ConstraintEllipticalArcRangeToEndPoints::grad(double *param)
{
    double deriv=0.;
    if (param == p1x() || param == p1y() ||
        param == f1x() || param == f1y() ||
        param == cx() || param == cy() ||
        param == rmin() || param == angle()) {

        double X_0 = *p1x();
        double Y_0 = *p1y();
        double X_c = *cx();
        double Y_c = *cy();
        double X_F1 = *f1x();
        double Y_F1 = *f1y();
        double b = *rmin();
        double alpha_t = *angle();

        if (param == p1x())
            deriv += -(-((X_F1 - X_c)*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c,
                2) + pow(Y_F1 - Y_c, 2)) + (Y_F1 - Y_c)*cos(alpha_t)/b)/(((X_0 -
                X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b,
                2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 -
                Y_c) + (X_F1 - X_c)*(Y_0 - Y_c))*sin(alpha_t)/b) + ((X_F1 -
                X_c)*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                2)) - (Y_F1 - Y_c)*sin(alpha_t)/b)*(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*cos(alpha_t)/b)/pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1
                - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 -
                Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/b, 2))/(pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*cos(alpha_t)/b, 2)/pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/b, 2) + 1);
        if (param == p1y())
            deriv += -(-((Y_F1 - Y_c)*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c,
                2) + pow(Y_F1 - Y_c, 2)) - (X_F1 - X_c)*cos(alpha_t)/b)/(((X_0 -
                X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b,
                2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 -
                Y_c) + (X_F1 - X_c)*(Y_0 - Y_c))*sin(alpha_t)/b) + ((Y_F1 -
                Y_c)*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                2)) + (X_F1 - X_c)*sin(alpha_t)/b)*(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*cos(alpha_t)/b)/pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1
                - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 -
                Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/b, 2))/(pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*cos(alpha_t)/b, 2)/pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/b, 2) + 1);
        if (param == f1x())
            deriv += -((((X_0 - X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 -
                Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*cos(alpha_t)/b)*((X_0 - X_c)*cos(alpha_t)/sqrt(pow(b, 2) +
                pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)) - (X_F1 - X_c)*((X_0 -
                X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/pow(pow(b, 2)
                + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0) + (Y_0 -
                Y_c)*sin(alpha_t)/b)/pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 -
                Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/b, 2) - ((X_0 - X_c)*sin(alpha_t)/sqrt(pow(b, 2) +
                pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)) - (X_F1 - X_c)*((X_0 -
                X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 - Y_c))*sin(alpha_t)/pow(pow(b, 2)
                + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0) - (Y_0 -
                Y_c)*cos(alpha_t)/b)/(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 -
                Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/b))/(pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*cos(alpha_t)/b, 2)/pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/b, 2) + 1);
        if (param == f1y())
            deriv +=-((((X_0 - X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 -
                Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*cos(alpha_t)/b)*((Y_0 - Y_c)*cos(alpha_t)/sqrt(pow(b, 2) +
                pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)) - (Y_F1 - Y_c)*((X_0 -
                X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/pow(pow(b, 2)
                + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0) - (X_0 -
                X_c)*sin(alpha_t)/b)/pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 -
                Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/b, 2) - ((Y_0 - Y_c)*sin(alpha_t)/sqrt(pow(b, 2) +
                pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)) - (Y_F1 - Y_c)*((X_0 -
                X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 - Y_c))*sin(alpha_t)/pow(pow(b, 2)
                + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0) + (X_0 -
                X_c)*cos(alpha_t)/b)/(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 -
                Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/b))/(pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*cos(alpha_t)/b, 2)/pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/b, 2) + 1);
        if (param == cx())
            deriv += ((((X_0 - X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 -
                Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*cos(alpha_t)/b)*(-(X_F1 - X_c)*((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/pow(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2), 3.0/2.0) + (X_0 + X_F1 -
                2*X_c)*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 -
                Y_c, 2)) + (Y_0 - Y_F1)*sin(alpha_t)/b)/pow(((X_0 - X_c)*(X_F1 - X_c) +
                (Y_0 - Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c,
                2) + pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 -
                X_c)*(Y_0 - Y_c))*sin(alpha_t)/b, 2) - (-(X_F1 - X_c)*((X_0 - X_c)*(X_F1
                - X_c) + (Y_0 - Y_c)*(Y_F1 - Y_c))*sin(alpha_t)/pow(pow(b, 2) + pow(X_F1
                - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0) + (X_0 + X_F1 -
                2*X_c)*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 -
                Y_c, 2)) - (Y_0 - Y_F1)*cos(alpha_t)/b)/(((X_0 - X_c)*(X_F1 - X_c) +
                (Y_0 - Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c,
                2) + pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 -
                X_c)*(Y_0 - Y_c))*sin(alpha_t)/b))/(pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0
                - Y_c)*(Y_F1 - Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*cos(alpha_t)/b, 2)/pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/b, 2) + 1);
        if (param == cy())
            deriv +=((((X_0 - X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 -
                Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*cos(alpha_t)/b)*(-(Y_F1 - Y_c)*((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/pow(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2), 3.0/2.0) + (Y_0 + Y_F1 -
                2*Y_c)*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 -
                Y_c, 2)) - (X_0 - X_F1)*sin(alpha_t)/b)/pow(((X_0 - X_c)*(X_F1 - X_c) +
                (Y_0 - Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c,
                2) + pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 -
                X_c)*(Y_0 - Y_c))*sin(alpha_t)/b, 2) - (-(Y_F1 - Y_c)*((X_0 - X_c)*(X_F1
                - X_c) + (Y_0 - Y_c)*(Y_F1 - Y_c))*sin(alpha_t)/pow(pow(b, 2) + pow(X_F1
                - X_c, 2) + pow(Y_F1 - Y_c, 2), 3.0/2.0) + (Y_0 + Y_F1 -
                2*Y_c)*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 -
                Y_c, 2)) + (X_0 - X_F1)*cos(alpha_t)/b)/(((X_0 - X_c)*(X_F1 - X_c) +
                (Y_0 - Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c,
                2) + pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 -
                X_c)*(Y_0 - Y_c))*sin(alpha_t)/b))/(pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0
                - Y_c)*(Y_F1 - Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*cos(alpha_t)/b, 2)/pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/b, 2) + 1);
        if (param == rmin())
            deriv += ((((X_0 - X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 -
                Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*cos(alpha_t)/b)*(b*((X_0 - X_c)*(X_F1 - X_c) + (Y_0 - Y_c)*(Y_F1 -
                Y_c))*cos(alpha_t)/pow(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
                2), 3.0/2.0) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/pow(b, 2))/pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/b, 2) - (b*((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*sin(alpha_t)/pow(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2), 3.0/2.0) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 -
                X_c)*(Y_0 - Y_c))*cos(alpha_t)/pow(b, 2))/(((X_0 - X_c)*(X_F1 - X_c) +
                (Y_0 - Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c,
                2) + pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 -
                X_c)*(Y_0 - Y_c))*sin(alpha_t)/b))/(pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0
                - Y_c)*(Y_F1 - Y_c))*sin(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) - (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*cos(alpha_t)/b, 2)/pow(((X_0 - X_c)*(X_F1 - X_c) + (Y_0 -
                Y_c)*(Y_F1 - Y_c))*cos(alpha_t)/sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
                pow(Y_F1 - Y_c, 2)) + (-(X_0 - X_c)*(Y_F1 - Y_c) + (X_F1 - X_c)*(Y_0 -
                Y_c))*sin(alpha_t)/b, 2) + 1);
        if (param == angle())
            deriv += 1; 
    }
    return scale * deriv;
}    

double ConstraintEllipticalArcRangeToEndPoints::maxStep(MAP_pD_D &dir, double lim)
{
    // step(angle()) <= pi/18 = 10°
    MAP_pD_D::iterator it = dir.find(angle());
    if (it != dir.end()) {
        double step = std::abs(it->second);
        if (step > M_PI/18.)
            lim = std::min(lim, (M_PI/18.) / step);
    }
    return lim;
}

// L2LAngle
ConstraintAngleViaPoint::ConstraintAngleViaPoint(Curve &acrv1, Curve &acrv2, Point p, double* angle)
{
    pvec.push_back(angle);
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    acrv1.PushOwnParams(pvec);
    acrv2.PushOwnParams(pvec);
    crv1 = acrv1.Copy();
    crv2 = acrv2.Copy();
    origpvec = pvec;
    remapped=true;
    rescale();
}
ConstraintAngleViaPoint::~ConstraintAngleViaPoint()
{
    delete crv1; crv1 = 0;
    delete crv2; crv2 = 0;
}

void ConstraintAngleViaPoint::ReconstructEverything()
{
    int cnt=0;
    cnt++;//skip angle - we have an inline function for that
    poa.x = pvec[cnt]; cnt++;
    poa.y = pvec[cnt]; cnt++;
    crv1->ReconstructOnNewPvec(pvec,cnt);
    crv2->ReconstructOnNewPvec(pvec,cnt);
    remapped=false;
}

ConstraintType ConstraintAngleViaPoint::getTypeId()
{
    return AngleViaPoint;
}

void ConstraintAngleViaPoint::rescale(double coef)
{
    scale = coef * 1.;
}

double ConstraintAngleViaPoint::error()
{
    if (remapped) ReconstructEverything();
    double ang=*angle();
    Vector2D n1 = crv1->CalculateNormal(poa);
    Vector2D n2 = crv2->CalculateNormal(poa);

    //rotate n1 by angle
    Vector2D n1r (n1.x*cos(ang) - n1.y*sin(ang), n1.x*sin(ang) + n1.y*cos(ang) );

    //calculate angle between n1r and n2. Since we have rotated the n1, the angle is the error function.
    //for our atan2, y is a dot product (n2) * (n1r rotated ccw by 90 degrees).
    //               x is a dot product (n2) * (n1r)
    double err = atan2(-n2.x*n1r.y+n2.y*n1r.x, n2.x*n1r.x + n2.y*n1r.y);
    //essentially, the function is equivalent to atan2(n2)-(atan2(n1)+angle). The only difference is behavior when normals are zero (the intended result is also zero in this case).
    return scale * err;
}

double ConstraintAngleViaPoint::grad(double *param)
{
    //first of all, check that we need to compute anything.
    int i;
    for( i=0 ; i<pvec.size() ; i++ ){
        if ( param == pvec[i] ) break;
    };
    if ( i == pvec.size() ) return 0.0;

    double deriv=0.;

    if (remapped) ReconstructEverything();

    if (param == angle()) deriv += -1.0;
    Vector2D n1 = crv1->CalculateNormal(poa);
    Vector2D n2 = crv2->CalculateNormal(poa);

    Vector2D dn1 = crv1->CalculateNormal(poa, param);
    Vector2D dn2 = crv2->CalculateNormal(poa, param);
    deriv -= ( (-dn1.x)*n1.y / pow(n1.length(),2)  +  dn1.y*n1.x / pow(n1.length(),2) );
    deriv += ( (-dn2.x)*n2.y / pow(n2.length(),2)  +  dn2.y*n2.x / pow(n2.length(),2) );


//use numeric for testing
#if 0
    double const eps = 0.00001;
    double oldparam = *param;
    double v0 = this->error();
    *param += eps;
    double vr = this->error();
    *param = oldparam - eps;
    double vl = this->error();
    *param = oldparam;
    //If not nasty, real derivative should be between left one and right one
    double numretl = (v0-vl)/eps;
    double numretr = (vr-v0)/eps;
    assert(deriv <= std::max(numretl,numretr) );
    assert(deriv >= std::min(numretl,numretr) );
#endif

    return scale * deriv;
}


} //namespace GCS
