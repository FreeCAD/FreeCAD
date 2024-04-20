/***************************************************************************
 *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License (LGPL)    *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <vector>

#include <App/Application.h>
#include <Base/Parameter.h>

#include "PathSegmentWalker.h"


#define ARC_MIN_SEGMENTS   20.0  // minimum # segments to interpolate an arc

#ifndef M_PI
    #define M_PI 3.14159265358979323846
    #define M_PI    3.14159265358979323846 /* pi */
#endif

#ifndef M_PI_2
    #define M_PI_2  1.57079632679489661923 /* pi/2 */
#endif


namespace Path
{

Base::Vector3d compensateRotation(const Base::Vector3d &pt, const Base::Rotation &rot, const Base::Vector3d &center)
{
    Base::Vector3d ptRotated;
    rot.multVec(pt - center, ptRotated);
    return ptRotated + center;
}

Base::Rotation yawPitchRoll(double a, double b, double c)
{
    Base::Rotation rot;
    rot.setYawPitchRoll(-c, -b, -a);
    return rot;
}

PathSegmentVisitor::~PathSegmentVisitor()
{
}

void PathSegmentVisitor::setup(const Base::Vector3d &last)
{
    (void)last;
}

void PathSegmentVisitor::g0(int id, const Base::Vector3d &last, const Base::Vector3d &next, const std::deque<Base::Vector3d> &pts)
{
    (void)id;
    (void)last;
    (void)next;
    (void)pts;
}

void PathSegmentVisitor::g1(int id, const Base::Vector3d &last, const Base::Vector3d &next, const std::deque<Base::Vector3d> &pts)
{
    (void)id;
    (void)last;
    (void)next;
    (void)pts;
}

void PathSegmentVisitor::g23(int id, const Base::Vector3d &last, const Base::Vector3d &next, const std::deque<Base::Vector3d> &pts, const Base::Vector3d &center)
{
    (void)id;
    (void)last;
    (void)next;
    (void)pts;
    (void)center;
}

void PathSegmentVisitor::g8x(int id, const Base::Vector3d &last, const Base::Vector3d &next, const std::deque<Base::Vector3d> &pts,
                 const std::deque<Base::Vector3d> &p, const std::deque<Base::Vector3d> &q)
{
    (void)id;
    (void)last;
    (void)next;
    (void)pts;
    (void)p;
    (void)q;
}

void PathSegmentVisitor::g38(int id, const Base::Vector3d &last, const Base::Vector3d &next)
{
    (void)id;
    (void)last;
    (void)next;
}

PathSegmentWalker::PathSegmentWalker(const Toolpath &tp_)
    :tp(tp_)
{}


void PathSegmentWalker::walk(PathSegmentVisitor &cb, const Base::Vector3d &startPosition)
{
    if(tp.getSize()==0) {
        return;
    }

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Part");
    float deviation = hGrp->GetFloat("MeshDeviation",0.2);

    Base::Vector3d rotCenter = tp.getCenter();
    Base::Vector3d last(startPosition);
    Base::Rotation lrot;
    double A = 0.0;
    double B = 0.0;
    double C = 0.0;

    bool absolute = true;
    bool absolutecenter = false;

    // for mapping the coordinates to XY plane
    double Base::Vector3d::*pz = &Base::Vector3d::z;

    cb.setup(last);

    for (unsigned int  i = 0; i < tp.getSize(); i++) {
        std::deque<Base::Vector3d> points;

        const Path::Command &cmd = tp.getCommand(i);
        const std::string &name = cmd.Name;
        Base::Vector3d next = cmd.getPlacement().getPosition();
        double a = A;
        double b = B;
        double c = C;

        if (!absolute)
            next = last + next;
        if (!cmd.has("X")) next.x = last.x;
        if (!cmd.has("Y")) next.y = last.y;
        if (!cmd.has("Z")) next.z = last.z;
        if ( cmd.has("A")) a = cmd.getValue("A");
        if ( cmd.has("B")) b = cmd.getValue("B");
        if ( cmd.has("C")) c = cmd.getValue("C");

        Base::Rotation nrot = yawPitchRoll(a, b, c);

        Base::Vector3d rnext = compensateRotation(next, nrot, rotCenter);

        if ( (name == "G0") || (name == "G00") || (name == "G1") || (name == "G01") ) {
            // straight line
            if (nrot != lrot) {
                double amax = std::max(fmod(fabs(a - A), 360), std::max(fmod(fabs(b - B), 360), fmod(fabs(c - C), 360)));
                double angle = amax / 180 * M_PI;
                int segments = std::max(ARC_MIN_SEGMENTS, 3.0/(deviation/angle));

                double da = (a - A) / segments;
                double db = (b - B) / segments;
                double dc = (c - C) / segments;

                Base::Vector3d dnext = (next - last) / segments;

                for (int j = 1; j < segments; j++) {
                    Base::Vector3d inter = last + dnext * j;

                    Base::Rotation rot = yawPitchRoll(A + da*j, B + db*j, C + dc*j);
                    Base::Vector3d rinter = compensateRotation(inter, rot, rotCenter);

                    points.push_back(rinter);
                }
            }

            if ("G0" == name || "G00" == name) {
                cb.g0(i, last, rnext, points);
            } else {
                cb.g1(i, last, rnext, points);
            }

            last = next;
            A = a;
            B = b;
            C = c;
            lrot = nrot;

        } else if ( (name == "G2") || (name == "G02") || (name == "G3") || (name == "G03") ) {
            // arc
            Base::Vector3d norm;
            Base::Vector3d center;

            if ( (name == "G2") || (name == "G02") )
                norm.*pz = -1.0;
            else
                norm.*pz = 1.0;

            if (absolutecenter)
                center = cmd.getCenter();
            else
                center = (last + cmd.getCenter());
            Base::Vector3d next0(next);
            next0.*pz = 0.0;
            Base::Vector3d last0(last);
            last0.*pz = 0.0;
            Base::Vector3d center0(center);
            center0.*pz = 0.0;
            //double radius = (last - center).Length();
            double angle = (next0 - center0).GetAngle(last0 - center0);
            // GetAngle will always return the minor angle. Switch if needed
            Base::Vector3d anorm = (last0 - center0) % (next0 - center0);
            if (anorm.*pz < 0) {
                if(name == "G3" || name == "G03")
                    angle = M_PI * 2 - angle;
            } else if(anorm.*pz > 0) {
                if(name == "G2" || name == "G02")
                    angle = M_PI * 2 - angle;
            } else if (angle == 0)
                angle = M_PI * 2;

            double amax = std::max(fmod(fabs(a - A), 360), std::max(fmod(fabs(b - B), 360), fmod(fabs(c - C), 360)));

            int segments = std::max(ARC_MIN_SEGMENTS, 3.0/(deviation/std::max(angle, amax))); //we use a rather simple rule here, provisorily
            double dZ = (next.*pz - last.*pz)/segments; //How far each segment will helix in Z

            double dangle = angle/segments;
            double da = (a - A) / segments;
            double db = (b - B) / segments;
            double dc = (c - C) / segments;

            for (int j = 1; j < segments; j++) {
                Base::Vector3d inter;
                Base::Rotation rot(norm, dangle*j);
                rot.multVec((last0 - center0), inter);
                inter.*pz = last.*pz + dZ * j; //Enable displaying helices

                Base::Rotation arot = yawPitchRoll(A + da*j, B + db*j, C + dc*j);
                Base::Vector3d rinter = compensateRotation(center0 + inter, arot, rotCenter);

                points.push_back(rinter);
            }

            cb.g23(i, last, rnext, points, center);

            last = next;
            A = a;
            B = b;
            C = c;
            lrot = nrot;

        } else if (name == "G90") {
            // absolute mode
            absolute = true;

        } else if (name == "G91") {
            // relative mode
            absolute = false;

        } else if (name == "G90.1") {
            // absolute mode
            absolutecenter = true;

        } else if (name == "G91.1") {
            // relative mode
            absolutecenter = false;

        } else if ((name=="G73")||(name=="G81")||(name=="G82")||(name=="G83")||(name=="G84")||(name=="G85")||(name=="G86")||(name=="G89")){
            // drill,tap,bore
            double r = 0;
            if (cmd.has("R"))
                r = cmd.getValue("R");

            std::deque<Base::Vector3d> plist;
            std::deque<Base::Vector3d> qlist;

            Base::Vector3d p1(next);
            p1.*pz = last.*pz;

            if (nrot != lrot) {
                double amax = std::max(fmod(fabs(a - A), 360), std::max(fmod(fabs(b - B), 360), fmod(fabs(c - C), 360)));
                double angle = amax / 180 * M_PI;
                int segments = std::max(ARC_MIN_SEGMENTS, 3.0/(deviation/angle));

                double da = (a - A) / segments;
                double db = (b - B) / segments;
                double dc = (c - C) / segments;

                Base::Vector3d dnext = (p1 - last) / segments;

                for (int j = 1; j < segments; j++) {
                    Base::Vector3d inter = last + dnext * j;

                    Base::Rotation rot = yawPitchRoll(A + da*j, B + db*j, C + dc*j);
                    Base::Vector3d rinter = compensateRotation(inter, rot, rotCenter);

                    points.push_back(rinter);
                }
            }

            Base::Vector3d p1r = compensateRotation(p1, nrot, rotCenter);
            Base::Vector3d p2(next);
            p2.*pz = r;
            Base::Vector3d p2r = compensateRotation(p2, nrot, rotCenter);

            double q;
            if (cmd.has("Q")) {
                q = cmd.getValue("Q");
                if (q>0) {
                    Base::Vector3d temp(next);
                    for(temp.*pz=r;temp.*pz>next.*pz;temp.*pz-=q) {
                        Base::Vector3d pr = compensateRotation(temp, nrot, rotCenter);
                        qlist.push_back(pr);
                    }
                }
            }

            Base::Vector3d p3(next);
            if (retract_mode == 99)  // G81,G83 need to account for G99 and retract to R only
                p3.*pz = p2.*pz;
            else
                p3.*pz = last.*pz;


            Base::Vector3d p3r = compensateRotation(p3, nrot, rotCenter);

            plist.push_back(p1r);
            plist.push_back(p2r);
            plist.push_back(p3r);

            cb.g8x(i, last, next, points, plist, qlist);

            last = p3;
            A = a;
            B = b;
            C = c;
            lrot = nrot;


        } else if ((name=="G38.2")||(name=="G38.3")||(name=="G38.4")||(name=="G38.5")){
            // Straight probe
            cb.g38(i, last, next);
            last = next;
        } else if(name=="G17") {
            pz = &Base::Vector3d::z;
        } else if(name=="G18") {
            pz = &Base::Vector3d::y;
        } else if(name=="G19") {
            pz = &Base::Vector3d::x;
        } else if(name=="G98") {
            retract_mode = 98;
        } else if(name=="G99") {
            retract_mode = 99;
        }
    }
}


}
