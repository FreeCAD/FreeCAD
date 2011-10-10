
/***************************************************************************
                        frames_io.h -  description
                       -------------------------
    begin                : June 2006
    copyright            : (C) 2006 Erwin Aertbelien
    email                : firstname.lastname@mech.kuleuven.ac.be

 History (only major changes)( AUTHOR-Description ) :

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/

#include "utilities/error.h"
#include "utilities/error_stack.h"
#include "frames.hpp"
#include "frames_io.hpp"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <iostream>

namespace KDL {


std::ostream& operator << (std::ostream& os,const Vector& v) {
    os << "[" << std::setw(KDL_FRAME_WIDTH) << v(0) << "," << std::setw(KDL_FRAME_WIDTH)<<v(1)
       << "," << std::setw(KDL_FRAME_WIDTH) << v(2) << "]";
    return os;
}

std::ostream& operator << (std::ostream& os,const Twist& v) {
    os << "[" << std::setw(KDL_FRAME_WIDTH) << v.vel(0)
       << "," << std::setw(KDL_FRAME_WIDTH) << v.vel(1)
       << "," << std::setw(KDL_FRAME_WIDTH) << v.vel(2)
       << "," << std::setw(KDL_FRAME_WIDTH) << v.rot(0)
       << "," << std::setw(KDL_FRAME_WIDTH) << v.rot(1)
       << "," << std::setw(KDL_FRAME_WIDTH) << v.rot(2)
       << "]";
    return os;
}

std::ostream& operator << (std::ostream& os,const Wrench& v) {
    os << "[" << std::setw(KDL_FRAME_WIDTH) << v.force(0)
       << "," << std::setw(KDL_FRAME_WIDTH) << v.force(1)
       << "," << std::setw(KDL_FRAME_WIDTH) << v.force(2)
       << "," << std::setw(KDL_FRAME_WIDTH) << v.torque(0)
       << "," << std::setw(KDL_FRAME_WIDTH) << v.torque(1)
       << "," << std::setw(KDL_FRAME_WIDTH) << v.torque(2)
       << "]";
    return os;
}


std::ostream& operator << (std::ostream& os,const Rotation& R) {
#ifdef KDL_ROTATION_PROPERTIES_RPY
    double r,p,y;
    R.GetRPY(r,p,y);
    os << "[RPY]"<<endl;
    os << "[";
    os << std::setw(KDL_FRAME_WIDTH) << r << ",";
    os << std::setw(KDL_FRAME_WIDTH) << p << ",";
    os << std::setw(KDL_FRAME_WIDTH) << y << "]";
#else
# ifdef KDL_ROTATION_PROPERTIES_EULER
    double z,y,x;
    R.GetEulerZYX(z,y,x);
    os << "[EULERZYX]"<<endl;
    os << "[";
    os << std::setw(KDL_FRAME_WIDTH) << z << ",";
    os << std::setw(KDL_FRAME_WIDTH) << y << ",";
    os << std::setw(KDL_FRAME_WIDTH) << x << "]";
# else
    os << "[";
    for (int i=0;i<=2;i++) {
        os << std::setw(KDL_FRAME_WIDTH) << R(i,0) << "," <<
                       std::setw(KDL_FRAME_WIDTH) << R(i,1) << "," <<
                       std::setw(KDL_FRAME_WIDTH) << R(i,2);
        if (i<2)
            os << ";"<< std::endl << " ";
        else
            os << "]";
    }
# endif
#endif
    return os;
}

std::ostream& operator << (std::ostream& os, const Frame& T)
{
    os << "[" << T.M << std::endl<< T.p << "]";
    return os;
}

std::ostream& operator << (std::ostream& os,const Vector2& v) {
    os << "[" << std::setw(KDL_FRAME_WIDTH) << v(0) << "," << std::setw(KDL_FRAME_WIDTH)<<v(1)
       << "]";
    return os;
}

// Rotation2 gives back an angle in degrees with the << and >> operators.
std::ostream& operator << (std::ostream& os,const Rotation2& R) {
    os << "[" << R.GetRot()*rad2deg << "]";
    return os;
}

std::ostream& operator << (std::ostream& os, const Frame2& T)
{
    os << T.M << T.p;
    return os;
}

std::istream& operator >> (std::istream& is,Vector& v)
{   IOTrace("Stream input Vector (vector or ZERO)");
    char storage[10];
    EatWord(is,"[]",storage,10);
    if (strlen(storage)==0) {
        Eat(is,'[');
        is >> v(0);
        Eat(is,',');
        is >> v(1);
        Eat(is,',');
        is >> v(2);
        EatEnd(is,']');
        IOTracePop();
        return is;
    }
    if (strcmp(storage,"ZERO")==0) {
        v = Vector::Zero();
        IOTracePop();
        return is;
    }
    throw Error_Frame_Vector_Unexpected_id();
}

std::istream& operator >> (std::istream& is,Twist& v)
{   IOTrace("Stream input Twist");
    Eat(is,'[');
    is >> v.vel(0);
    Eat(is,',');
    is >> v.vel(1);
    Eat(is,',');
    is >> v.vel(2);
    Eat(is,',');
    is >> v.rot(0);
    Eat(is,',');
    is >> v.rot(1);
    Eat(is,',');
    is >> v.rot(2);
    EatEnd(is,']');
    IOTracePop();
    return is;
}

std::istream& operator >> (std::istream& is,Wrench& v)
{   IOTrace("Stream input Wrench");
    Eat(is,'[');
    is >> v.force(0);
    Eat(is,',');
    is >> v.force(1);
    Eat(is,',');
    is >> v.force(2);
    Eat(is,',');
    is >> v.torque(0);
    Eat(is,',');
    is >> v.torque(1);
    Eat(is,',');
    is >> v.torque(2);
    EatEnd(is,']');
    IOTracePop();
    return is;
}

std::istream& operator >> (std::istream& is,Rotation& r)
{   IOTrace("Stream input Rotation (Matrix or EULERZYX, EULERZYZ,RPY, ROT, IDENTITY)");
    char storage[10];
    EatWord(is,"[]",storage,10);
    if (strlen(storage)==0) {
        Eat(is,'[');
        for (int i=0;i<3;i++) {
            is >> r(i,0);
            Eat(is,',') ;
            is >> r(i,1);
            Eat(is,',');
            is >> r(i,2);
            if (i<2)
                Eat(is,';');
            else
                EatEnd(is,']');
        }
        IOTracePop();
        return is;
    }
    Vector v;
    if (strcmp(storage,"EULERZYX")==0) {
        is >> v;
        v=v*deg2rad;
        r = Rotation::EulerZYX(v(0),v(1),v(2));
        IOTracePop();
        return is;
    }
    if (strcmp(storage,"EULERZYZ")==0) {
        is >> v;
        v=v*deg2rad;
        r = Rotation::EulerZYZ(v(0),v(1),v(2));
        IOTracePop();
        return is;
    }
    if (strcmp(storage,"RPY")==0) {
        is >> v;
        v=v*deg2rad;
        r = Rotation::RPY(v(0),v(1),v(2));
        IOTracePop();
        return is;
    }
    if (strcmp(storage,"ROT")==0) {
        is >> v;
        double angle;
        Eat(is,'[');
        is >> angle;
        EatEnd(is,']');
        r = Rotation::Rot(v,angle*deg2rad);
        IOTracePop();
        return is;
    }
    if (strcmp(storage,"IDENTITY")==0) {
        r = Rotation::Identity();
        IOTracePop();
        return is;
    }
    throw Error_Frame_Rotation_Unexpected_id();
    return is;
}

std::istream& operator >> (std::istream& is,Frame& T)
{   IOTrace("Stream input Frame (Rotation,Vector) or DH[...]");
    char storage[10];
    EatWord(is,"[",storage,10);
    if (strlen(storage)==0) {
        Eat(is,'[');
        is >> T.M;
        is >> T.p;
        EatEnd(is,']');
        IOTracePop();
        return is;
    }
    if (strcmp(storage,"DH")==0) {
        double a,alpha,d,theta;
        Eat(is,'[');
        is >> a;
        Eat(is,',');
        is >> alpha;
        Eat(is,',');
        is >> d;
        Eat(is,',');
        is >> theta;
        EatEnd(is,']');
        T = Frame::DH(a,alpha*deg2rad,d,theta*deg2rad);
        IOTracePop();
        return is;
    }
    throw Error_Frame_Frame_Unexpected_id();
    return is;
}

std::istream& operator >> (std::istream& is,Vector2& v)
{   IOTrace("Stream input Vector2");
    Eat(is,'[');
    is >> v(0);
    Eat(is,',');
    is >> v(1);
    EatEnd(is,']');
    IOTracePop();
    return is;
}
std::istream& operator >> (std::istream& is,Rotation2& r)
{   IOTrace("Stream input Rotation2");
    Eat(is,'[');
    double val;
    is >> val;
    r.Rot(val*deg2rad);
    EatEnd(is,']');
    IOTracePop();
    return is;
}
std::istream& operator >> (std::istream& is,Frame2& T)
{   IOTrace("Stream input Frame2");
    is >> T.M;
    is >> T.p;
    IOTracePop();
    return is;
}

} // namespace Frame
