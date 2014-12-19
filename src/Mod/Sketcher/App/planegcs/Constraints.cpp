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
: origpvec(0), pvec(0), scale(1.), tag(0), pvecChangedFlag(true)
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
    pvecChangedFlag=true;
}

void Constraint::revertParams()
{
    pvec = origpvec;
    pvecChangedFlag=true;
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

int Constraint::findParamInPvec(double *param)
{
    int ret = -1;
    for( int i=0 ; i<pvec.size() ; i++ ){
        if ( param == pvec[i] ) {
            ret = i;
            break;
        };
    };
    return ret;
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
    pvec.push_back(e.focus1.x);
    pvec.push_back(e.focus1.y);
    pvec.push_back(e.radmin);
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
    this->l = l;
    this->l.PushOwnParams(pvec);

    this->e = e;
    this->e.PushOwnParams(pvec);//DeepSOIC: hopefully, this won't push arc's parameters
    origpvec = pvec;
    pvecChangedFlag = true;
    rescale();
}

void ConstraintEllipseTangentLine::ReconstructGeomPointers()
{
    int i=0;
    l.ReconstructOnNewPvec(pvec, i);
    e.ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

ConstraintType ConstraintEllipseTangentLine::getTypeId()
{
    return TangentEllipseLine;
}

void ConstraintEllipseTangentLine::rescale(double coef)
{
    scale = coef * 1;
}

void ConstraintEllipseTangentLine::errorgrad(double *err, double *grad, double *param)
{
    // DeepSOIC equation
    // http://forum.freecadweb.org/viewtopic.php?f=10&t=7520&start=140

    if (pvecChangedFlag) ReconstructGeomPointers();
    DeriVector2 p1 (l.p1, param);
    DeriVector2 p2 (l.p2, param);
    DeriVector2 f1 (e.focus1, param);
    DeriVector2 c (e.center, param);
    DeriVector2 f2 = c.linCombi(2.0, f1, -1.0); // 2*cv - f1v

    //mirror F1 against the line
    DeriVector2 nl = l.CalculateNormal(l.p1, param).getNormalized();
    double distF1L = 0, ddistF1L = 0; //distance F1 to line
    distF1L = f1.subtr(p1).scalarProd(nl,&ddistF1L);
    DeriVector2 f1m = f1.sum(nl.multD(-2*distF1L,-2*ddistF1L));//f1m = f1 mirrored

    //calculate distance form f1m to f2
    double distF1mF2, ddistF1mF2;
    distF1mF2 = f2.subtr(f1m).length(ddistF1mF2);

    //calculate major radius (to compare the distance to)
    double dradmin = (param == e.radmin) ? 1.0 : 0.0;
    double radmaj, dradmaj;
    radmaj = e.getRadMaj(c,f1,*e.radmin, dradmin, dradmaj);

    if (err)
        *err = distF1mF2 - 2*radmaj;
    if (grad)
        *grad = ddistF1mF2 - 2*dradmaj;
}

double ConstraintEllipseTangentLine::error()
{
    double err;
    errorgrad(&err,0,0);
    return scale * err;
}

double ConstraintEllipseTangentLine::grad(double *param)
{      
    //first of all, check that we need to compute anything.
    if ( findParamInPvec(param) == -1 ) return 0.0;

    double deriv;
    errorgrad(0, &deriv, param);

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


    return deriv*scale;
}

// ConstraintInternalAlignmentPoint2Ellipse
ConstraintInternalAlignmentPoint2Ellipse::ConstraintInternalAlignmentPoint2Ellipse(Ellipse &e, Point &p1, InternalAlignmentType alignmentType)
{
    this->p = p1;
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    this->e = e;
    this->e.PushOwnParams(pvec);
    this->AlignmentType = alignmentType;
    origpvec = pvec;
    rescale();
}

void ConstraintInternalAlignmentPoint2Ellipse::ReconstructGeomPointers()
{
    int i = 0;
    p.x = pvec[i]; i++;
    p.y = pvec[i]; i++;
    e.ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

ConstraintType ConstraintInternalAlignmentPoint2Ellipse::getTypeId()
{
    return InternalAlignmentPoint2Ellipse;
}

void ConstraintInternalAlignmentPoint2Ellipse::rescale(double coef)
{
    scale = coef * 1;
}

void ConstraintInternalAlignmentPoint2Ellipse::errorgrad(double *err, double *grad, double *param)
{
    if (pvecChangedFlag) ReconstructGeomPointers();

    //todo: prefill only what's needed, not everything

    DeriVector2 c(e.center, param);
    DeriVector2 f1(e.focus1, param);
    DeriVector2 emaj = f1.subtr(c).getNormalized();
    DeriVector2 emin = emaj.rotate90ccw();
    DeriVector2 pv (p, param);
    double b, db;//minor radius
    b = *e.radmin; db = (e.radmin == param) ? 1.0 : 0.0;

    //major radius
    double a, da;
    a = e.getRadMaj(c,f1,b,db,da);

    DeriVector2 poa;//point to align to
    bool by_y_not_by_x;//a flag to indicate if the alignment error function is for y (false - x, true - y).

    switch(AlignmentType){
        case EllipsePositiveMajorX:
        case EllipsePositiveMajorY:
            poa = c.sum(emaj.multD(a, da));
            by_y_not_by_x = AlignmentType == EllipsePositiveMajorY;
        break;
        case EllipseNegativeMajorX:
        case EllipseNegativeMajorY:
            poa = c.sum(emaj.multD(-a, -da));
            by_y_not_by_x = AlignmentType == EllipseNegativeMajorY;
        break;
        case EllipsePositiveMinorX:
        case EllipsePositiveMinorY:
            poa = c.sum(emin.multD(b, db));
            by_y_not_by_x = AlignmentType == EllipsePositiveMinorY;
        break;
        case EllipseNegativeMinorX:
        case EllipseNegativeMinorY:
            poa = c.sum(emin.multD(-b, -db));
            by_y_not_by_x = AlignmentType == EllipseNegativeMinorY;
        break;
        case EllipseFocus2X:
        case EllipseFocus2Y:
            poa = c.linCombi(2.0, f1, -1.0);
            by_y_not_by_x = AlignmentType == EllipseFocus2Y;
        break;
        default:
            //shouldn't happen
            poa = pv;//align to the point itself, doing nothing essentially
    }
    if(err)
        *err = by_y_not_by_x ? pv.y - poa.y : pv.x - poa.x;
    if(grad)
        *grad = by_y_not_by_x ? pv.dy - poa.dy : pv.dx - poa.dx;
}

double ConstraintInternalAlignmentPoint2Ellipse::error()
{    
    double err;
    errorgrad(&err,0,0);
    return scale * err;

}

double ConstraintInternalAlignmentPoint2Ellipse::grad(double *param)
{      
    //first of all, check that we need to compute anything.
    if ( findParamInPvec(param) == -1  ) return 0.0;

    double deriv;
    errorgrad(0, &deriv, param);

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

    return deriv*scale;

}

//  ConstraintEqualMajorAxesEllipse
ConstraintEqualMajorAxesEllipse:: ConstraintEqualMajorAxesEllipse(Ellipse &e1, Ellipse &e2)
{
    this->e1 = e1;
    this->e1.PushOwnParams(pvec);
    this->e2 = e2;
    this->e2.PushOwnParams(pvec);
    origpvec = pvec;
    pvecChangedFlag = true;
    rescale();
}

void ConstraintEqualMajorAxesEllipse::ReconstructGeomPointers()
{
    int i =0;
    e1.ReconstructOnNewPvec(pvec, i);
    e2.ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

ConstraintType ConstraintEqualMajorAxesEllipse::getTypeId()
{
    return EqualMajorAxesEllipse;
}

void ConstraintEqualMajorAxesEllipse::rescale(double coef)
{
    scale = coef * 1;
}

void ConstraintEqualMajorAxesEllipse::errorgrad(double *err, double *grad, double *param)
{
    if (pvecChangedFlag) ReconstructGeomPointers();
    double a1, da1;
    a1 = e1.getRadMaj(param, da1);
    double a2, da2;
    a2 = e2.getRadMaj(param, da2);
    if (err)
        *err = a2 - a1;
    if (grad)
        *grad = da2 - da1;
}

double ConstraintEqualMajorAxesEllipse::error()
{    
    double err;
    errorgrad(&err,0,0);
    return scale * err;
}

double ConstraintEqualMajorAxesEllipse::grad(double *param)
{
    //first of all, check that we need to compute anything.
    if ( findParamInPvec(param) == -1  ) return 0.0;

    double deriv;
    errorgrad(0, &deriv, param);

    return deriv * scale;
}

// EllipticalArcRangeToEndPoints
ConstraintEllipticalArcRangeToEndPoints::ConstraintEllipticalArcRangeToEndPoints(Point &p, ArcOfEllipse &a, double *angle_t)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(angle_t);
    e = a;
    e.PushOwnParams(pvec);
    origpvec = pvec;
    rescale();
}

void ConstraintEllipticalArcRangeToEndPoints::ReconstructGeomPointers()
{
    int i=0;
    p.x=pvec[i]; i++;
    p.y=pvec[i]; i++;
    i++;//we have an inline function for the angle
    e.ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

ConstraintType ConstraintEllipticalArcRangeToEndPoints::getTypeId()
{
    return EllipticalArcRangeToEndPoints;
}

void ConstraintEllipticalArcRangeToEndPoints::rescale(double coef)
{
    scale = coef * 1;
}

void ConstraintEllipticalArcRangeToEndPoints::errorgrad(double *err, double *grad, double *param)
{
    if (pvecChangedFlag) ReconstructGeomPointers();

    DeriVector2 c(e.center, param);
    DeriVector2 f1(e.focus1, param);
    DeriVector2 emaj = f1.subtr(c).getNormalized();
    DeriVector2 emin = emaj.rotate90ccw();
    double b, db;
    b = *e.radmin; db = e.radmin==param ? 1.0 : 0.0;
    double a, da;
    a = e.getRadMaj(c,f1,b,db,da);

    DeriVector2 multimaj = emaj.multD(b, db);//a vector to muptiply pc by to yield an x for atan2. This is a minor radius drawn along major axis.
    DeriVector2 multimin = emin.multD(a, da);//to yield y for atan2

    DeriVector2 pv(p, param);
    DeriVector2 pc = pv.subtr(c); //point referenced to ellipse's center

    double x, dx, y, dy;//distorted coordinates of point in ellipse's coordinates, to be fed to atan2 to yiels a t-parameter (called "angle" here)
    x = pc.scalarProd(multimaj, &dx);
    y = pc.scalarProd(multimin, &dy);
    double xylen2 = x*x + y*y ;//square of length of (x,y)

    double si, co;
    si = sin(*angle()); co = cos(*angle());

    double dAngle = param==angle() ? 1.0 : 0.0;

    if (err)
        *err = atan2(-si*x+co*y, co*x+si*y);//instead of calculating atan2(y,x) and subtracting angle, we rotate (x,y) by -angle and calculate atan2 of the result. Hopefully, this will not force angles to zero when x,y happen to be zero. Plus, one atan2 is cheaper to compute than two atan2's.
    if (grad)
        *grad = -dAngle + ( -dx*y / xylen2  +  dy*x / xylen2 );

}

double ConstraintEllipticalArcRangeToEndPoints::error()
{
    double err;
    errorgrad(&err,0,0);
    return scale * err;
}

double ConstraintEllipticalArcRangeToEndPoints::grad(double *param)
{
    //first of all, check that we need to compute anything.
    if ( findParamInPvec(param) == -1  ) return 0.0;

    double deriv;
    errorgrad(0, &deriv, param);

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


    return deriv*scale;
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

// ConstraintAngleViaPoint
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
    pvecChangedFlag=true;
    rescale();
}
ConstraintAngleViaPoint::~ConstraintAngleViaPoint()
{
    delete crv1; crv1 = 0;
    delete crv2; crv2 = 0;
}

void ConstraintAngleViaPoint::ReconstructGeomPointers()
{
    int cnt=0;
    cnt++;//skip angle - we have an inline function for that
    poa.x = pvec[cnt]; cnt++;
    poa.y = pvec[cnt]; cnt++;
    crv1->ReconstructOnNewPvec(pvec,cnt);
    crv2->ReconstructOnNewPvec(pvec,cnt);
    pvecChangedFlag=false;
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
    if (pvecChangedFlag) ReconstructGeomPointers();
    double ang=*angle();
    DeriVector2 n1 = crv1->CalculateNormal(poa);
    DeriVector2 n2 = crv2->CalculateNormal(poa);

    //rotate n1 by angle
    DeriVector2 n1r (n1.x*cos(ang) - n1.y*sin(ang), n1.x*sin(ang) + n1.y*cos(ang) );

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
    if ( findParamInPvec(param) == -1  ) return 0.0;

    double deriv=0.;

    if (pvecChangedFlag) ReconstructGeomPointers();

    if (param == angle()) deriv += -1.0;
    DeriVector2 n1 = crv1->CalculateNormal(poa, param);
    DeriVector2 n2 = crv2->CalculateNormal(poa, param);
    deriv -= ( (-n1.dx)*n1.y / pow(n1.length(),2)  +  n1.dy*n1.x / pow(n1.length(),2) );
    deriv += ( (-n2.dx)*n2.y / pow(n2.length(),2)  +  n2.dy*n2.x / pow(n2.length(),2) );


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

//ConstraintSnell

ConstraintSnell::ConstraintSnell(Curve &ray1, Curve &ray2, Curve &boundary, Point p, double* n1, double* n2, bool flipn1, bool flipn2)
{
    pvec.push_back(n1);
    pvec.push_back(n2);
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    ray1.PushOwnParams(pvec);
    ray2.PushOwnParams(pvec);
    boundary.PushOwnParams(pvec);
    this->ray1 = ray1.Copy();
    this->ray2 = ray2.Copy();
    this->boundary = boundary.Copy();
    origpvec = pvec;
    pvecChangedFlag=true;

    this->flipn1 = flipn1;
    this->flipn2 = flipn2;

    rescale();
}
ConstraintSnell::~ConstraintSnell()
{
    delete ray1; ray1 = 0;
    delete ray2; ray2 = 0;
    delete boundary; boundary = 0;
}

void ConstraintSnell::ReconstructGeomPointers()
{
    int cnt=0;
    cnt++; cnt++;//skip n1, n2 - we have an inline function for that
    poa.x = pvec[cnt]; cnt++;
    poa.y = pvec[cnt]; cnt++;
    ray1->ReconstructOnNewPvec(pvec,cnt);
    ray2->ReconstructOnNewPvec(pvec,cnt);
    boundary->ReconstructOnNewPvec(pvec,cnt);
    pvecChangedFlag=false;
}

ConstraintType ConstraintSnell::getTypeId()
{
    return Snell;
}

void ConstraintSnell::rescale(double coef)
{
    scale = coef * 1.;
}

//error and gradient combined. Values are returned through pointers.
void ConstraintSnell::errorgrad(double *err, double *grad, double* param)
{
    if (pvecChangedFlag) ReconstructGeomPointers();
    DeriVector2 tang1 = ray1->CalculateNormal(poa, param).rotate90cw().getNormalized();
    DeriVector2 tang2 = ray2->CalculateNormal(poa, param).rotate90cw().getNormalized();
    DeriVector2 tangB = boundary->CalculateNormal(poa, param).rotate90cw().getNormalized();
    double sin1, dsin1, sin2, dsin2;
    sin1 = tang1.scalarProd(tangB, &dsin1);//sinus of angle of incidence
    sin2 = tang2.scalarProd(tangB, &dsin2);
    if (flipn1) {sin1 = -sin1; dsin1 = -dsin1;}
    if (flipn2) {sin2 = -sin2; dsin2 = -dsin2;}

    double dn1 = (param == n1()) ? 1.0 : 0.0;
    double dn2 = (param == n2()) ? 1.0 : 0.0;
    if (err)
        *err = *n1()*sin1 - *n2()*sin2;
    if (grad)
        *grad = dn1*sin1 + *n1()*dsin1 - dn2*sin2 - *n2()*dsin2;
}

double ConstraintSnell::error()
{
    double err;
    errorgrad(&err, 0, 0);
    return scale * err;
}

double ConstraintSnell::grad(double *param)
{

    //first of all, check that we need to compute anything.
    if ( findParamInPvec(param) == -1 ) return 0.0;

    double deriv;
    errorgrad(0, &deriv, param);


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
