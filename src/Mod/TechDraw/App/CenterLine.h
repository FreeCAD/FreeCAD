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

#pragma once

#include <App/FeaturePython.h>
#include <Base/Persistence.h>
#include <Base/Vector3D.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "Tag.h"
#include "Cosmetic.h"
#include "Geometry.h"


namespace TechDraw {
class DrawViewPart;

class TechDrawExport CenterLine: public Base::Persistence, public TechDraw::Tag
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum class Mode
    {
        VERTICAL,
        HORIZONTAL,
        ALIGNED
    };
    // TODO: when C++ 20
    // using Mode;
    enum class Type
    {
        FACE,
        EDGE,
        VERTEX
    };  // TODO: Make this global
    // TODO: when C++ 20
    // using Mode;

    template <typename U, std::enable_if_t<
        std::is_same_v<CenterLine::Type, U> ||
        std::is_same_v<CenterLine::Mode, U>,
        bool
    > =true>
    friend std::ostream& operator<<(std::ostream& out, const U& type) {
        out << static_cast<int>(type);
        return out;
    }

    CenterLine();
    CenterLine(const CenterLine* cl);
    //set m_faces after using next 3 ctors
    CenterLine(const TechDraw::BaseGeomPtr& bg,
               const Mode m = Mode::VERTICAL,
               const double h = 0.0,
               const double v = 0.0,
               const double r = 0.0,
               const double x = 0.0);
    CenterLine(const Base::Vector3d& p1, const Base::Vector3d& p2,
               const Mode m = Mode::VERTICAL,
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
                                         const Mode mode = Mode::VERTICAL,
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
                                          const Mode mode,
                                          const double ext,
                                          const double m_hShift,
                                          const double m_vShift,
                                          const double rotate);
    static std::pair<Base::Vector3d, Base::Vector3d> calcEndPoints2Lines(
                                          const TechDraw::DrawViewPart* partFeat,
                                          const std::vector<std::string>& faceNames,
                                          const Mode mode,
                                          const double ext,
                                          const double m_hShift,
                                          const double m_vShift,
                                          const double rotate,
                                          const bool flip);
    static std::pair<Base::Vector3d, Base::Vector3d> calcEndPoints2Points(
                                          const TechDraw::DrawViewPart* partFeat,
                                          const std::vector<std::string>& faceNames,
                                          const Mode mode,
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
    void setType(const Type type) { m_type = type; };
    Type getType() const { return m_type; }

    Base::Vector3d m_start;
    Base::Vector3d m_end;

    //required to recalculate CL after source geom changes.
    std::vector<std::string> m_faces;
    std::vector<std::string> m_edges;
    std::vector<std::string> m_verts;
    Type m_type;
    Mode m_mode;
    double m_hShift;
    double m_vShift;
    double m_rotate;
    double m_extendBy;
    LineFormat m_format;
    bool m_flip2Line;

    TechDraw::BaseGeomPtr m_geometry;

protected:
    void initialize();

    Py::Object PythonObject;
};

}  // namespace TechDraw