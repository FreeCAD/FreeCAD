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

using namespace Eigen;

namespace GCS
{

///////////////////////////////////////
// Constraints
///////////////////////////////////////

Constraint::Constraint()
: origpvec(0), pvec(0), scale(1.), tag(0)
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
}

void Constraint::revertParams()
{
    pvec = origpvec;
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


//quattorot and derivatives

Matrix3d rotation(double a, double b, double c, double d) {

    double norm = sqrt(pow(a,2)+pow(b,2)+pow(c,2)+pow(d,2));
    double x=a/norm, y=b/norm, z=c/norm, w=d/norm;

    Matrix3d rot;
    rot(0,0) = 1-2*(pow(y,2)+pow(z,2));
    rot(0,1) = -2.0*w*z + 2.0*x*y;
    rot(0,2) = 2.0*w*y + 2.0*x*z;
    rot(1,0) = 2.0*w*z + 2.0*x*y;
    rot(1,1) = 1-2*(pow(x,2)+pow(z,2));
    rot(1,2) = -2.0*w*x + 2.0*y*z;
    rot(2,0) = -2.0*w*y + 2.0*x*z;
    rot(2,1) = 2.0*w*x + 2.0*y*z;
    rot(2,2) = 1-2*(pow(x,2)+pow(y,2));

    return rot;
}

Matrix3d rotation_da(double a, double b, double c, double d) {

    double no = sqrt(pow(a,2)+pow(b,2)+pow(c,2)+pow(d,2));
    double div = pow(pow(a,2)+pow(b,2)+pow(c,2)+pow(d,2), 3.0/2.0);
    double x=a/no, y=b/no, z=c/no, w=d/no;

    double dxa = 1.0/no - pow(a,2)/pow(no,3);
    double dya = (-1.0*b*a)/div;
    double dza = (-1.0*c*a)/div;
    double dwa = (-1.0*d*a)/div;

    Matrix3d rot;
    rot <<  -4.0*(y*dya+z*dza), 	  -2.0*(w*dza+dwa*z)+2.0*(x*dya+dxa*y), 2.0*(dwa*y+w*dya)+2.0*(dxa*z+x*dza),
    2.0*(w*dza+dwa*z)+2.0*(x*dya+dxa*y),  -4.0*(x*dxa+z*dza),			-2.0*(dwa*x+w*dxa)+2.0*(dya*z+y*dza),
    -2.0*(dwa*y+w*dya)+2.0*(dxa*z+x*dza), 2.0*(dwa*x+w*dxa)+2.0*(dya*z+y*dza),  -4.0*(x*dxa+y*dya);

    return rot;
}

Matrix3d rotation_db(double a, double b, double c, double d) {

    double no = sqrt(pow(a,2)+pow(b,2)+pow(c,2)+pow(d,2));
    double div = pow(pow(a,2)+pow(b,2)+pow(c,2)+pow(d,2), 3.0/2.0);
    double x=a/no, y=b/no, z=c/no, w=d/no;

    double dxb = (-1.0*a*b)/div;
    double dyb = 1.0/no - pow(b,2)/pow(no,3);
    double dzb = (-1.0*c*b)/div;
    double dwb = (-1.0*d*b)/div;

    Matrix3d rot;
    rot <<  -4.0*(y*dyb+z*dzb), 	  -2.0*(w*dzb+dwb*z)+2.0*(x*dyb+dxb*y), 2.0*(dwb*y+w*dyb)+2.0*(dxb*z+x*dzb),
    2.0*(w*dzb+dwb*z)+2.0*(x*dyb+dxb*y),  -4.0*(x*dxb+z*dzb),			-2.0*(dwb*x+w*dxb)+2.0*(dyb*z+y*dzb),
    -2.0*(dwb*y+w*dyb)+2.0*(dxb*z+x*dzb), 2.0*(dwb*x+w*dxb)+2.0*(dyb*z+y*dzb),  -4.0*(x*dxb+y*dyb);

    return rot;
}

Matrix3d rotation_dc(double a, double b, double c, double d) {

    double no = sqrt(pow(a,2)+pow(b,2)+pow(c,2)+pow(d,2));
    double div = pow(pow(a,2)+pow(b,2)+pow(c,2)+pow(d,2), 3.0/2.0);
    double x=a/no, y=b/no, z=c/no, w=d/no;

    double dxc = (-1.0*a*c)/div;
    double dyc = (-1.0*b*c)/div;
    double dzc = 1.0/no - pow(c,2)/pow(no,3);
    double dwc = (-1.0*d*c)/div;

    Matrix3d rot;
    rot <<  -4.0*(y*dyc+z*dzc), 	  -2.0*(w*dzc+dwc*z)+2.0*(x*dyc+dxc*y), 2.0*(dwc*y+w*dyc)+2.0*(dxc*z+x*dzc),
    2.0*(w*dzc+dwc*z)+2.0*(x*dyc+dxc*y),  -4.0*(x*dxc+z*dzc),			-2.0*(dwc*x+w*dxc)+2.0*(dyc*z+y*dzc),
    -2.0*(dwc*y+w*dyc)+2.0*(dxc*z+x*dzc), 2.0*(dwc*x+w*dxc)+2.0*(dyc*z+y*dzc),  -4.0*(x*dxc+y*dyc);

    return rot;
}

Matrix3d rotation_dd(double a, double b, double c, double d) {

    double no = sqrt(pow(a,2)+pow(b,2)+pow(c,2)+pow(d,2));
    double div = pow(pow(a,2)+pow(b,2)+pow(c,2)+pow(d,2), 3.0/2.0);
    double x=a/no, y=b/no, z=c/no, w=d/no;

    double dxd = (-1.0*a*d)/div;
    double dyd = (-1.0*b*d)/div;
    double dzd = (-1.0*c*d)/div;
    double dwd = 1.0/no - pow(d,2)/pow(no,3);

    Matrix3d rot;
    rot <<  -4.0*(y*dyd+z*dzd), 	  -2.0*(w*dzd+dwd*z)+2.0*(x*dyd+dxd*y), 2.0*(dwd*y+w*dyd)+2.0*(dxd*z+x*dzd),
    2.0*(w*dzd+dwd*z)+2.0*(x*dyd+dxd*y),  -4.0*(x*dxd+z*dzd),			-2.0*(dwd*x+w*dxd)+2.0*(dyd*z+y*dzd),
    -2.0*(dwd*y+w*dyd)+2.0*(dxd*z+x*dzd), 2.0*(dwd*x+w*dxd)+2.0*(dyd*z+y*dzd),  -4.0*(x*dxd+y*dyd);

    return rot;
}


//Plane parallel (need to be treated special as angle=0° or angle=180° dos not work with angle constraint
ConstraintParralelFaceAS::ConstraintParralelFaceAS( GCS::Solid& s0, GCS::Solid& s1, ParallelType *t)
{
    type = t;
    
    //get the face placements in the objects coordinate system and calculate the normals
    Vector3d a,b;
    a << s0.n.x, s0.n.y, s0.n.z;
    b << s1.n.x, s1.n.y, s1.n.z;

    //get the normal vector
    n0 = (a).normalized();
    n1 = (b).normalized();
    

    pvec.push_back(s0.q.a);
    pvec.push_back(s0.q.b);
    pvec.push_back(s0.q.c);
    pvec.push_back(s0.q.d);
    pvec.push_back(s1.q.a);
    pvec.push_back(s1.q.b);
    pvec.push_back(s1.q.c);
    pvec.push_back(s1.q.d);
    origpvec = pvec;
    rescale();

}

ConstraintType ConstraintParralelFaceAS::getTypeId()
{
    return ASParallel;
}

void ConstraintParralelFaceAS::rescale(double coef)
{
    scale = coef;
}

double ConstraintParralelFaceAS::error()
{

    Matrix3d rot0, rot1;

    rot0 = rotation(*q0a(), *q0b(), *q0c(), *q0d());
    rot1 = rotation(*q1a(), *q1b(), *q1c(), *q1d());

    Vector3d n0_g = (rot0*n0);
    Vector3d n1_g = (rot1*n1);

    double error = 0;
    if (*type == GCS::NormalSame)
        error = pow((n0_g-n1_g).norm(),2);
    else
        error = pow((n0_g+n1_g).norm(),2);

    return error;
}

double ConstraintParralelFaceAS::grad(double* param)
{

     Vector3d dn;
    Matrix3d r0, r1;
    r0 = rotation(*q0a(), *q0b(), *q0c(), *q0d());
    r1 = rotation(*q1a(), *q1b(), *q1c(), *q1d());

    if (param == q0a()) {
        Matrix3d rot = rotation_da(*q0a(), *q0b(), *q0c(), *q0d());
        dn = rot*n0;
    }
    else if (param == q0b()) {
        Matrix3d rot = rotation_db(*q0a(), *q0b(), *q0c(), *q0d());
        dn = rot*n0;
    }
    else if (param == q0c()) {
        Matrix3d rot = rotation_dc(*q0a(), *q0b(), *q0c(), *q0d());
        dn = rot*n0;
    }
    else if (param == q0d()) {
        Matrix3d rot = rotation_dd(*q0a(), *q0b(), *q0c(), *q0d());
        dn = rot*n0;
    }
    else if (param == q1a()) {
        Matrix3d rot = rotation_da(*q1a(), *q1b(), *q1c(), *q1d());
        dn = rot*n1*-1;
    }
    else if (param == q1b()) {
        Matrix3d rot = rotation_db(*q1a(), *q1b(), *q1c(), *q1d());
        dn = rot*n1*-1;
    }
    else if (param == q1c()) {
        Matrix3d rot = rotation_dc(*q1a(), *q1b(), *q1c(), *q1d());
        dn = rot*n1*-1;
    }
    else if (param == q1d()) {
        Matrix3d rot = rotation_dd(*q1a(), *q1b(), *q1c(), *q1d());
        dn = rot*n1*-1;
    }
    else return 0;


    Vector3d n0n	= r0*n0;
    Vector3d n1n	= r1*n1;

    double div = 0;
    if (*type == NormalSame)
        div = (n0n-n1n).dot(dn)*2;
    else
        div = (n0n+n1n).dot(dn)*2;
	
    return div;
}


//dDistance constraint

ConstraintFaceDistanceAS::ConstraintFaceDistanceAS(GCS::Solid& s0, GCS::Solid& s1, double *d)
{

    n0 << s0.n.x, s0.n.y, s0.n.z;
    n0.normalize();
    p0 << s0.p.x, s0.p.y, s0.p.z;
    p1 << s1.p.x, s1.p.y, s1.p.z;

    //and positions
    pvec.push_back(s0.d.x);
    pvec.push_back(s0.d.y);
    pvec.push_back(s0.d.z);
    pvec.push_back(s1.d.x);
    pvec.push_back(s1.d.y);
    pvec.push_back(s1.d.z);

    //quaternions
    pvec.push_back(s0.q.a);
    pvec.push_back(s0.q.b);
    pvec.push_back(s0.q.c);
    pvec.push_back(s0.q.d);
    pvec.push_back(s1.q.a);
    pvec.push_back(s1.q.b);
    pvec.push_back(s1.q.c);
    pvec.push_back(s1.q.d);
    
    //distance
    dist = d;

    origpvec = pvec;
    rescale();

}

ConstraintType ConstraintFaceDistanceAS::getTypeId()
{
    return ASDistance;
}

void ConstraintFaceDistanceAS::rescale(double coef)
{
    scale = coef;
}

double ConstraintFaceDistanceAS::error()
{

    Matrix3d rot0, rot1;
    Vector3d v0, v1;

    rot0 = rotation(*q0a(), *q0b(), *q0c(), *q0d());
    rot1 = rotation(*q1a(), *q1b(), *q1c(), *q1d());
    v0 << *p0x(), *p0y(), *p0z();
    v1 << *p1x(), *p1y(), *p1z();
    
    double error = std::pow(((rot0*n0).dot(rot1*p1+v1) - (rot0*n0).dot(rot0*p0+v0))/(rot0*n0).norm() - *dist,2);

    return error;
}

double ConstraintFaceDistanceAS::grad(double* param)
{

    Matrix3d r0, r1;
    Vector3d v0, v1;

    v0 << *p0x(), *p0y(), *p0z();
    v1 << *p1x(), *p1y(), *p1z();
    r0 = rotation(*q0a(), *q0b(), *q0c(), *q0d());
    r1 = rotation(*q1a(), *q1b(), *q1c(), *q1d());

    Matrix3d dr0, dr1;
    double div = 0;
    double error=((r0*n0).dot(r1*p1+v1) - (r0*n0).dot(r0*p0+v0))/(r0*n0).norm() - *dist;
    if (param == q0a() || param == q0b() || param == q0c() || param == q0d()) {

        if (param == q0a()) 	 dr0 = rotation_da(*q0a(), *q0b(), *q0c(), *q0d());
        else if (param == q0b()) dr0 = rotation_db(*q0a(), *q0b(), *q0c(), *q0d());
        else if (param == q0c()) dr0 = rotation_dc(*q0a(), *q0b(), *q0c(), *q0d());
        else if (param == q0d()) dr0 = rotation_dd(*q0a(), *q0b(), *q0c(), *q0d());
        
	VectorXd r0n = r0*n0;	
        div  = ( (dr0*n0).dot(r1*p1+v1)*r0n.norm() - r0n.dot(r1*p1+v1)*r0n.dot(dr0*n0)/r0n.norm() );
	div -= ( ((dr0*n0).dot(r0*p0+v0)+r0n.dot(dr0*p0))*r0n.norm() );
	div -= ( r0n.dot(r0*p0+v0)*r0n.dot(dr0*n0)/r0n.norm() );
	div /= pow(r0n.norm(),2);
    }
    else if (param == q1a() || param == q1b() || param == q1c() || param == q1d()) {

        if (param == q1a()) 	 dr1 = rotation_da(*q1a(), *q1b(), *q1c(), *q1d());
        else if (param == q1b()) dr1 = rotation_db(*q1a(), *q1b(), *q1c(), *q1d());
        else if (param == q1c()) dr1 = rotation_dc(*q1a(), *q1b(), *q1c(), *q1d());
        else if (param == q1d()) dr1 = rotation_dd(*q1a(), *q1b(), *q1c(), *q1d());

        div = (r0*n0).dot(dr1*p1)/(r0*n0).norm();
    }
    else if (param == p0x() || param == p0y() || param == p0z()) {

        Vector3d dp_g;
        if (param == p0x()) 		dp_g << 1,0,0;
        else if (param == p0y()) 	dp_g << 0,1,0;
        else if (param == p0z())	dp_g << 0,0,1;

	div = -1*(r0*n0).dot(dp_g)/(r0*n0).norm();
    }
    else if (param == p1x() || param == p1y() || param == p1z()) {

        Vector3d dp_g;
        if (param == p1x()) 		dp_g << 1,0,0;
        else if (param == p1y()) 	dp_g << 0,1,0;
        else if (param == p1z())	dp_g << 0,0,1;
					
	div = (r0*n0).dot(dp_g)/(r0*n0).norm();
    }
    else return 0;

    div *= 2*error;
    return div;
}

} //namespace GCS
