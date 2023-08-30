/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_UTILITIES_H
#define GUI_UTILITIES_H

#include <vector>
#include <App/Material.h>
#include <Base/Converter.h>
#include <Base/ViewProj.h>
#include <Inventor/SbColor.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbViewVolume.h>


class SbViewVolume;
class QAbstractItemView;

namespace Base {
// Specialization for SbVec3f
template <>
struct vec_traits<SbVec3f> {
    using vec_type = SbVec3f;
    using float_type = float;
    explicit vec_traits(const vec_type& v) : v(v){}
    inline std::tuple<float_type,float_type,float_type> get() const {
        return std::make_tuple(v[0], v[1], v[2]);
    }
private:
    const vec_type& v;
};

// Specialization for SbVec3d
template <>
struct vec_traits<SbVec3d> {
    using vec_type = SbVec3d;
    using float_type = double;
    explicit vec_traits(const vec_type& v) : v(v){}
    inline std::tuple<float_type,float_type,float_type> get() const {
        return std::make_tuple(v[0], v[1], v[2]);
    }
private:
    const vec_type& v;
};

// Specialization for SbRotation
template <>
struct vec_traits<SbRotation> {
    using vec_type = SbRotation;
    using float_type = float;
    explicit vec_traits(const vec_type& v) : v(v){}
    inline std::tuple<float_type,float_type,float_type,float_type> get() const {
        float_type q1,q2,q3,q4;
        v.getValue(q1,q2,q3,q4);
        return std::make_tuple(q1, q2, q3, q4);
    }
private:
    const vec_type& v;
};

// Specialization for SbColor
template <>
struct vec_traits<SbColor> {
    using vec_type = SbColor;
    using float_type = float;
    explicit vec_traits(const vec_type& v) : v(v){}
    inline std::tuple<float_type,float_type,float_type> get() const {
        return std::make_tuple(v[0], v[1], v[2]);
    }
private:
    const vec_type& v;
};

// Specialization for Color
template <>
struct vec_traits<App::Color> {
    using vec_type = App::Color;
    using float_type = float;
    explicit vec_traits(const vec_type& v) : v(v){}
    inline std::tuple<float_type,float_type,float_type> get() const {
        return std::make_tuple(v.r, v.g, v.b);
    }
private:
    const vec_type& v;
};
}

namespace App{ class DocumentObject; }
namespace Gui {

/**
 */
class GuiExport ViewVolumeProjection : public Base::ViewProjMethod
{
public:
    explicit ViewVolumeProjection (const SbViewVolume &vv);
    ~ViewVolumeProjection() override = default;

    Base::Vector3f operator()(const Base::Vector3f &rclPt) const override;
    Base::Vector3d operator()(const Base::Vector3d &rclPt) const override;
    Base::Vector3f inverse (const Base::Vector3f &rclPt) const override;
    Base::Vector3d inverse (const Base::Vector3d &rclPt) const override;

    Base::Matrix4D getProjectionMatrix () const override;

protected:
    SbViewVolume viewVolume;
    SbMatrix matrix;
    SbMatrix invert;
};

class GuiExport Tessellator
{
public:
    explicit Tessellator(const std::vector<SbVec2f>&);
    std::vector<int> tessellate() const;

private:
    static void tessCB(void * v0, void * v1, void * v2, void * cbdata);

private:
    std::vector<SbVec2f> polygon;
};

class GuiExport ItemViewSelection
{
public:
    explicit ItemViewSelection(QAbstractItemView* view);
    void applyFrom(const std::vector<App::DocumentObject*> objs);

private:
    QAbstractItemView* view;
    class MatchName;
};

} // namespace Gui

#endif // GUI_UTILITIES_H
