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
    CenterLine(const CenterLine* cl);
    //set m_faces after using next 3 ctors
    CenterLine(const TechDraw::BaseGeomPtr& bg,
               const int m = CLMODE::VERTICAL,
               const double h = 0.0,
               const double v = 0.0,
               const double r = 0.0,
               const double x = 0.0);
    CenterLine(const Base::Vector3d& p1, const Base::Vector3d& p2,
               const int m = CLMODE::VERTICAL,
               const double h = 0.0,
               const double v = 0.0,
               const double r = 0.0,
               const double x = 0.0);
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

    static CenterLine* CenterLineBuilder(const TechDraw::DrawViewPart* partFeat,
                                         const std::vector<std::string>& subs,
                                         const int mode = 0,
                                         const bool flip = false);
    TechDraw::BaseGeomPtr scaledGeometry(const TechDraw::DrawViewPart* partFeat);
    TechDraw::BaseGeomPtr scaledAndRotatedGeometry(TechDraw::DrawViewPart* partFeat);

    static std::pair<Base::Vector3d, Base::Vector3d> rotatePointsAroundMid(
                                          const Base::Vector3d& p1,
                                          const Base::Vector3d& p2,
                                          const Base::Vector3d& mid,
                                          const double rotate);
    static std::pair<Base::Vector3d, Base::Vector3d> calcEndPointsNoRef(
                                                          const Base::Vector3d& start,
                                                          const Base::Vector3d& end,
                                                          const double scale,
                                                          const double ext,
                                                          const double hShift,
                                                          const double vShift,
                                                          const double rotate,
                                                          const double viewAngleDeg);
    static std::pair<Base::Vector3d, Base::Vector3d> calcEndPoints(
                                          const TechDraw::DrawViewPart* partFeat,
                                          const std::vector<std::string>& faceNames,
                                          const int mode,
                                          const double ext,
                                          const double m_hShift,
                                          const double m_vShift,
                                          const double rotate);
    static std::pair<Base::Vector3d, Base::Vector3d> calcEndPoints2Lines(
                                          const TechDraw::DrawViewPart* partFeat,
                                          const std::vector<std::string>& faceNames,
                                          const int vert,
                                          const double ext,
                                          const double m_hShift,
                                          const double m_vShift,
                                          const double rotate,
                                          const bool flip);
    static std::pair<Base::Vector3d, Base::Vector3d> calcEndPoints2Points(
                                          const TechDraw::DrawViewPart* partFeat,
                                          const std::vector<std::string>& faceNames,
                                          const int vert,
                                          const double ext,
                                          const double m_hShift,
                                          const double m_vShift,
                                          const double rotate,
                                          const bool flip);
    void dump(const char* title);
    void setShifts(const double h, const double v);
    double getHShift() const;
    double getVShift() const;
    void setRotate(const double r);
    double getRotate() const;
    void setExtend(const double e);
    double getExtend() const;
    void setFlip(const bool f);
    bool getFlip() const;
    void setType(const int type) { m_type = type; };
    int getType() const { return m_type; }

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
