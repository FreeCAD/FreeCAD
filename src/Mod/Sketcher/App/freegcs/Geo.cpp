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


Vector2D Line::CalculateNormal(Point &p, double* derivparam)
{
    Vector2D p1v(*p1.x, *p1.y);
    Vector2D p2v(*p2.x, *p2.y);

    Vector2D ret(0.0, 0.0);
    if(derivparam){
        if(derivparam==this->p1.x){
            ret.y += -1.0;
            //ret.x += 0;
        };
        if(derivparam==this->p1.y){
            //ret.y += 0;
            ret.x += 1.0;
        };
        if(derivparam==this->p2.x){
            ret.y += 1.0;
            //ret.x += 0;
        };
        if(derivparam==this->p2.y){
            //ret.y += 0;
            ret.x += -1.0;
        };
    } else {
        ret.x = -(p2v.y - p1v.y);
        ret.y = (p2v.x - p1v.x);
    };

    return ret;
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

Vector2D Circle::CalculateNormal(Point &p, double* derivparam)
{
    Vector2D cv (*center.x, *center.y);
    Vector2D pv (*p.x, *p.y);

    Vector2D ret(0.0, 0.0);
    if(derivparam){
        if (derivparam == center.x) {
            ret.x += 1;
            ret.y += 0;
        };
        if (derivparam == center.y) {
            ret.x += 0;
            ret.y += 1;
        };
        if (derivparam == p.x) {
            ret.x += -1;
            ret.y += 0;
        };
        if (derivparam == p.y) {
            ret.x += 0;
            ret.y += -1;
        };
    } else {
        ret.x = cv.x - pv.x;
        ret.y = cv.y - pv.y;
    };

    return ret;
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
Vector2D Ellipse::CalculateNormal(Point &p, double* derivparam)
{
    Vector2D cv (*center.x, *center.y);
    Vector2D f1v (*focus1X, *focus1Y);
    Vector2D pv (*p.x, *p.y);

    Vector2D ret(0.0, 0.0);

    Vector2D f2v ( 2*cv.x - f1v.x, 2*cv.y - f1v.y ); //position of focus2
    if(derivparam){

        Vector2D dc;
        Vector2D df1;
        Vector2D dp;

        if (derivparam == center.x) dc.x = 1.0;
        if (derivparam == center.y) dc.y = 1.0;
        if (derivparam == focus1X) df1.x = 1.0;
        if (derivparam == focus1Y) df1.y = 1.0;
        if (derivparam == p.x) dp.x = 1.0;
        if (derivparam == p.y) dp.y = 1.0;
        //todo: exit if all are zero

        Vector2D pf1 = Vector2D(pv.x - f1v.x, pv.y - f1v.y);//same as during error function calculation. I reuse the values during derivative calculation
        Vector2D pf2 = Vector2D(pv.x - f2v.x, pv.y - f2v.y);
        Vector2D pf1e = pf1.getNormalized();
        Vector2D pf2e = pf2.getNormalized();

        Vector2D df2 (2*dc.x - df1.x, 2*dc.y - df1.y );

        Vector2D dpf1 (dp.x - df1.x, dp.y - df1.y);//derivative before normalization
        Vector2D dpf1e (dpf1.x/pf1.length(), dpf1.y/pf1.length());//first portion of normalization derivative (normalized' = unnormalized'/len + unnormalized*(1/len)')
        dpf1e.x += -pf1.x/pow(pf1.length(),2)*(dpf1.x*pf1e.x + dpf1.y*pf1e.y);//second part of normalization dreivative
        dpf1e.y += -pf1.y/pow(pf1.length(),2)*(dpf1.x*pf1e.x + dpf1.y*pf1e.y);

        Vector2D dpf2 (dp.x - df2.x, dp.y - df2.y);//same stuff for pf2
        Vector2D dpf2e (dpf2.x/pf2.length(), dpf2.y/pf2.length());//first portion of normalization derivative (normalized' = unnormalized'/len + unnormalized*(1/len)')
        dpf2e.x += -pf2.x/pow(pf2.length(),2)*(dpf2.x*pf2e.x + dpf2.y*pf2e.y);//second part of normalization dreivative
        dpf2e.y += -pf2.y/pow(pf2.length(),2)*(dpf2.x*pf2e.x + dpf2.y*pf2e.y);

        ret.x = -(dpf1e.x + dpf2e.x);
        ret.y = -(dpf1e.y + dpf2e.y);//DeepSOIC: derivative calculated manually... error-prone =) Tested, fixed, looks good.

//numeric derivatives for testing
#if 0 //make sure to enable DEBUG_DERIVS when enabling
        double const eps = 0.00001;
        double oldparam = *derivparam;
        Vector2D v0 = this->CalculateNormal(p);
        *derivparam += eps;
        Vector2D vr = this->CalculateNormal(p);
        *derivparam = oldparam - eps;
        Vector2D vl = this->CalculateNormal(p);
        *derivparam = oldparam;
        //If not nasty, real derivative should be between left one and right one
        Vector2D numretl ((v0.x-vl.x)/eps, (v0.y-vl.y)/eps);
        Vector2D numretr ((vr.x-v0.x)/eps, (vr.y-v0.y)/eps);
        assert(ret.x <= std::max(numretl.x,numretr.x) );
        assert(ret.x >= std::min(numretl.x,numretr.x) );
        assert(ret.y <= std::max(numretl.y,numretr.y) );
        assert(ret.y >= std::min(numretl.y,numretr.y) );
#endif

    } else {
        Vector2D pf1 = Vector2D(pv.x - f1v.x, pv.y - f1v.y);
        Vector2D pf2 = Vector2D(pv.x - f2v.x, pv.y - f2v.y);
        Vector2D pf1e = pf1.getNormalized();
        Vector2D pf2e = pf2.getNormalized();
        ret.x = -(pf1e.x + pf2e.x);
        ret.y = -(pf1e.y + pf2e.y);
    };

    return ret;
}

int Ellipse::PushOwnParams(VEC_pD &pvec)
{
    int cnt=0;
    pvec.push_back(center.x); cnt++;
    pvec.push_back(center.y); cnt++;
    pvec.push_back(focus1X); cnt++;
    pvec.push_back(focus1Y); cnt++;
    pvec.push_back(radmin); cnt++;
    return cnt;
}
void Ellipse::ReconstructOnNewPvec(VEC_pD &pvec, int &cnt)
{
    center.x=pvec[cnt]; cnt++;
    center.y=pvec[cnt]; cnt++;
    focus1X=pvec[cnt]; cnt++;
    focus1Y=pvec[cnt]; cnt++;
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
