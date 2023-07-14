/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAW_CENTERLINE_H
#define TECHDRAW_CENTERLINE_H

#include <App/FeaturePython.h>
#include <Base/Persistence.h>
#include <Base/Vector3D.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "Cosmetic.h"
#include "Geometry.h"


namespace TechDraw {
class DrawViewPart;

class TechDrawExport CenterLine: public Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum CLMODE
    {
        VERTICAL,
        HORIZONTAL,
        ALIGNED
    };
    enum CLTYPE
    {
        FACE,
        EDGE,
        VERTEX
    };

    CenterLine();
    CenterLine(CenterLine* cl);
    //set m_faces after using next 3 ctors
    CenterLine(TechDraw::BaseGeomPtr bg,
               int m = CLMODE::VERTICAL,
               double h = 0.0,
               double v = 0.0,
               double r = 0.0,
               double x = 0.0);
    CenterLine(Base::Vector3d p1, Base::Vector3d p2,
               int m = CLMODE::VERTICAL,
               double h = 0.0,
               double v = 0.0,
               double r = 0.0,
               double x = 0.0);
    ~CenterLine() override;

    TechDraw::BaseGeomPtr BaseGeomPtrFromVectors(Base::Vector3d pt1, Base::Vector3d pt2);

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;

    PyObject *getPyObject() override;
    CenterLine* copy() const;
    CenterLine* clone() const;

    std::string toString() const;

    static CenterLine* CenterLineBuilder(TechDraw::DrawViewPart* partFeat,
                                         std::vector<std::string> subs,
                                         int mode = CenterLine::VERTICAL,
                                         bool flip = false);
    TechDraw::BaseGeomPtr scaledGeometry(TechDraw::DrawViewPart* partFeat);

    static std::tuple<Base::Vector3d, Base::Vector3d> rotatePointsAroundMid(
                                          Base::Vector3d p1,
                                          Base::Vector3d p2,
                                          Base::Vector3d mid,
                                          double rotate);
    static std::pair<Base::Vector3d, Base::Vector3d> calcEndPointsNoRef(
                                                          Base::Vector3d start,
                                                          Base::Vector3d end,
                                                          double scale,
                                                          double ext,
                                                          double hShift, double vShift,
                                                          double rotate);
    static std::pair<Base::Vector3d, Base::Vector3d> calcEndPoints(
                                          TechDraw::DrawViewPart* partFeat,
                                          std::vector<std::string> faceNames,
                                          int mode, double ext,
                                          double m_hShift, double m_vShift,
                                          double rotate);
    static std::pair<Base::Vector3d, Base::Vector3d> calcEndPoints2Lines(
                                          TechDraw::DrawViewPart* partFeat,
                                          std::vector<std::string> faceNames,
                                          int vert, double ext,
                                          double m_hShift, double m_vShift,
                                          double rotate, bool flip);
    static std::pair<Base::Vector3d, Base::Vector3d> calcEndPoints2Points(
                                          TechDraw::DrawViewPart* partFeat,
                                          std::vector<std::string> faceNames,
                                          int vert, double ext,
                                          double m_hShift, double m_vShift,
                                          double rotate, bool flip);
    void dump(const char* title);
    void setShifts(double h, double v);
    double getHShift();
    double getVShift();
    void setRotate(double r);
    double getRotate();
    void setExtend(double e);
    double getExtend();
    void setFlip(bool f);
    bool getFlip();

    Base::Vector3d m_start;
    Base::Vector3d m_end;

    //required to recalculate CL after source geom changes.
    std::vector<std::string> m_faces;
    std::vector<std::string> m_edges;
    std::vector<std::string> m_verts;
    int m_type;  // CLTYPE enum
    int m_mode;  // CLMODE enum
    double m_hShift;
    double m_vShift;
    double m_rotate;
    double m_extendBy;
    LineFormat m_format;
    bool m_flip2Line;

    TechDraw::BaseGeomPtr m_geometry;

    //Uniqueness
    boost::uuids::uuid getTag() const;
    virtual std::string getTagAsString() const;

protected:
    void initialize();

    void createNewTag();
    void assignTag(const TechDraw::CenterLine* cl);

    boost::uuids::uuid tag;

    Py::Object PythonObject;

};

}  // namespace TechDraw

#endif  // TECHDRAW_CENTERLINE_H