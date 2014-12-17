/***************************************************************************
 *   Copyright (c) Victor Titov (DeepSOIC)                                 *
 *                                           (vv.titov@gmail.com) 2014     *
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

#define DEBUG_DERIVS 0
#if DEBUG_DERIVS
#include <cassert>
#endif

#include "Geo.h"
namespace GCS{

DeriVector2::DeriVector2(const Point &p, double *derivparam)
{
    x=*p.x; y=*p.y;
    dx=0.0; dy=0.0;
    if (derivparam == p.x)
        dx = 1.0;
    if (derivparam == p.y)
        dy = 1.0;
}

double DeriVector2::length(double &dlength) const
{
    double l = length();
    if(l==0){
        dlength = 1.0;
        return l;
    } else {
        dlength = (x*dx + y*dy)/l;
        return l;
    }
}

DeriVector2 DeriVector2::getNormalized() const
{
    double l=length();
    if(l==0.0) {
        return DeriVector2(0, 0, dx/0.0, dy/0.0);
    } else {
        DeriVector2 rtn;
        rtn.x = x/l;
        rtn.y = y/l;
        //first, simply scale the derivative accordingly.
        rtn.dx = dx/l;
        rtn.dy = dy/l;
        //next, remove the collinear part of dx,dy (make a projection onto a normal)
        double dsc = rtn.dx*rtn.x + rtn.dy*rtn.y;//scalar product d*v
        rtn.dx -= dsc*rtn.x;//subtract the projection
        rtn.dy -= dsc*rtn.y;
        return rtn;
    }
}

double DeriVector2::scalarProd(const DeriVector2 &v2, double *dprd) const
{
    if (dprd) {
        *dprd = dx*v2.x + x*v2.dx + dy*v2.y + y*v2.dy;
    };
    return x*v2.x + y*v2.y;
}

DeriVector2 DeriVector2::divD(double val, double dval) const
{
    return DeriVector2(x/val,y/val,
                       dx/val - x*dval/(val*val),
                       dy/val - y*dval/(val*val)
                       );
}

DeriVector2 Line::CalculateNormal(Point &p, double* derivparam)
{
    DeriVector2 p1v(p1, derivparam);
    DeriVector2 p2v(p2, derivparam);

    return p2v.subtr(p1v).rotate90ccw();
}

int Line::PushOwnParams(VEC_pD &pvec)
{
    int cnt=0;
    pvec.push_back(p1.x); cnt++;
    pvec.push_back(p1.y); cnt++;
    pvec.push_back(p2.x); cnt++;
    pvec.push_back(p2.y); cnt++;
    return cnt;
}
void Line::ReconstructOnNewPvec(VEC_pD &pvec, int &cnt)
{
    p1.x=pvec[cnt]; cnt++;
    p1.y=pvec[cnt]; cnt++;
    p2.x=pvec[cnt]; cnt++;
    p2.y=pvec[cnt]; cnt++;
}
Line* Line::Copy()
{
    Line* crv = new Line(*this);
    return crv;
}


//---------------circle

DeriVector2 Circle::CalculateNormal(Point &p, double* derivparam)
{
    DeriVector2 cv (center, derivparam);
    DeriVector2 pv (p, derivparam);

    return cv.subtr(pv);
}

int Circle::PushOwnParams(VEC_pD &pvec)
{
    int cnt=0;
    pvec.push_back(center.x); cnt++;
    pvec.push_back(center.y); cnt++;
    pvec.push_back(rad); cnt++;
    return cnt;
}
void Circle::ReconstructOnNewPvec(VEC_pD &pvec, int &cnt)
{
    center.x=pvec[cnt]; cnt++;
    center.y=pvec[cnt]; cnt++;
    rad=pvec[cnt]; cnt++;
}
Circle* Circle::Copy()
{
    Circle* crv = new Circle(*this);
    return crv;
}

//------------arc
int Arc::PushOwnParams(VEC_pD &pvec)
{
    int cnt=0;
    cnt += Circle::PushOwnParams(pvec);
    pvec.push_back(start.x); cnt++;
    pvec.push_back(start.y); cnt++;
    pvec.push_back(end.x); cnt++;
    pvec.push_back(end.y); cnt++;
    pvec.push_back(startAngle); cnt++;
    pvec.push_back(endAngle); cnt++;
    return cnt;
}
void Arc::ReconstructOnNewPvec(VEC_pD &pvec, int &cnt)
{
    Circle::ReconstructOnNewPvec(pvec,cnt);
    start.x=pvec[cnt]; cnt++;
    start.y=pvec[cnt]; cnt++;
    end.x=pvec[cnt]; cnt++;
    end.y=pvec[cnt]; cnt++;
    startAngle=pvec[cnt]; cnt++;
    endAngle=pvec[cnt]; cnt++;
}
Arc* Arc::Copy()
{
    Arc* crv = new Arc(*this);
    return crv;
}


//--------------ellipse

//this function is exposed to allow reusing pre-filled derivectors in constraints code
double Ellipse::getRadMaj(const DeriVector2 &center, const DeriVector2 &f1, double b, double db, double &ret_dRadMaj)
{
    double cf, dcf;
    cf = f1.subtr(center).length(dcf);
    DeriVector2 hack (b, cf,
                      db, dcf);//hack = a nonsense vector to calculate major radius with derivatives, useful just because the calculation formula is the same as  vector length formula
    return hack.length(ret_dRadMaj);
}

//returns major radius. The derivative by derivparam is returned into ret_dRadMaj argument.
double Ellipse::getRadMaj(double *derivparam, double &ret_dRadMaj)
{
    DeriVector2 c(center, derivparam);
    DeriVector2 f1(focus1, derivparam);
    return getRadMaj(c, f1, *radmin, radmin==derivparam ? 1.0 : 0.0, ret_dRadMaj);
}

//returns the major radius (plain value, no derivatives)
double Ellipse::getRadMaj()
{
    double dradmaj;//dummy
    return getRadMaj(0,dradmaj);
}

DeriVector2 Ellipse::CalculateNormal(Point &p, double* derivparam)
{
    //fill some vectors in
    DeriVector2 cv (center, derivparam);
    DeriVector2 f1v (focus1, derivparam);
    DeriVector2 pv (p, derivparam);

    //calculation.
    //focus2:
    DeriVector2 f2v = cv.linCombi(2.0, f1v, -1.0); // 2*cv - f1v

    //pf1, pf2 = vectors from p to focus1,focus2
    DeriVector2 pf1 = f1v.subtr(pv);
    DeriVector2 pf2 = f2v.subtr(pv);
    //return sum of normalized pf2, pf2
    DeriVector2 ret = pf1.getNormalized().sum(pf2.getNormalized());

    //numeric derivatives for testing
    #if 0 //make sure to enable DEBUG_DERIVS when enabling
        if(derivparam) {
            double const eps = 0.00001;
            double oldparam = *derivparam;
            DeriVector2 v0 = this->CalculateNormal(p);
            *derivparam += eps;
            DeriVector2 vr = this->CalculateNormal(p);
            *derivparam = oldparam - eps;
            DeriVector2 vl = this->CalculateNormal(p);
            *derivparam = oldparam;
            //If not nasty, real derivative should be between left one and right one
            DeriVector2 numretl ((v0.x-vl.x)/eps, (v0.y-vl.y)/eps);
            DeriVector2 numretr ((vr.x-v0.x)/eps, (vr.y-v0.y)/eps);
            assert(ret.dx <= std::max(numretl.x,numretr.x) );
            assert(ret.dx >= std::min(numretl.x,numretr.x) );
            assert(ret.dy <= std::max(numretl.y,numretr.y) );
            assert(ret.dy >= std::min(numretl.y,numretr.y) );
        }
    #endif

    return ret;
}

int Ellipse::PushOwnParams(VEC_pD &pvec)
{
    int cnt=0;
    pvec.push_back(center.x); cnt++;
    pvec.push_back(center.y); cnt++;
    pvec.push_back(focus1.x); cnt++;
    pvec.push_back(focus1.y); cnt++;
    pvec.push_back(radmin); cnt++;
    return cnt;
}
void Ellipse::ReconstructOnNewPvec(VEC_pD &pvec, int &cnt)
{
    center.x=pvec[cnt]; cnt++;
    center.y=pvec[cnt]; cnt++;
    focus1.x=pvec[cnt]; cnt++;
    focus1.y=pvec[cnt]; cnt++;
    radmin=pvec[cnt]; cnt++;
}
Ellipse* Ellipse::Copy()
{
    Ellipse* crv = new Ellipse(*this);
    return crv;
}


//---------------arc of ellipse
int ArcOfEllipse::PushOwnParams(VEC_pD &pvec)
{
    int cnt=0;
    cnt += Ellipse::PushOwnParams(pvec);
    pvec.push_back(start.x); cnt++;
    pvec.push_back(start.y); cnt++;
    pvec.push_back(end.x); cnt++;
    pvec.push_back(end.y); cnt++;
    pvec.push_back(startAngle); cnt++;
    pvec.push_back(endAngle); cnt++;
    return cnt;

}
void ArcOfEllipse::ReconstructOnNewPvec(VEC_pD &pvec, int &cnt)
{
    Ellipse::ReconstructOnNewPvec(pvec,cnt);
    start.x=pvec[cnt]; cnt++;
    start.y=pvec[cnt]; cnt++;
    end.x=pvec[cnt]; cnt++;
    end.y=pvec[cnt]; cnt++;
    startAngle=pvec[cnt]; cnt++;
    endAngle=pvec[cnt]; cnt++;
}
ArcOfEllipse* ArcOfEllipse::Copy()
{
    ArcOfEllipse* crv = new ArcOfEllipse(*this);
    return crv;
}


}//namespace GCS
