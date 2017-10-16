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

#include <Base/ViewProj.h>
#include <App/Material.h>
#include <vector>
#include <Inventor/SbColor.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbViewVolume.h>

class SbViewVolume;
class QAbstractItemView;

namespace Base {
// Specialization for SbVec3f
template <>
struct vec_traits<SbVec3f> {
    typedef SbVec3f vec_type;
    typedef float float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline float_type x() { return v[0]; }
    inline float_type y() { return v[1]; }
    inline float_type z() { return v[2]; }
private:
    const vec_type& v;
};

// Specialization for SbVec3d
template <>
struct vec_traits<SbVec3d> {
    typedef SbVec3d vec_type;
    typedef double float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline float_type x() { return v[0]; }
    inline float_type y() { return v[1]; }
    inline float_type z() { return v[2]; }
private:
    const vec_type& v;
};

// Specialization for SbColor
template <>
struct vec_traits<SbColor> {
    typedef SbColor vec_type;
    typedef float float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline float_type x() { return v[0]; }
    inline float_type y() { return v[1]; }
    inline float_type z() { return v[2]; }
private:
    const vec_type& v;
};

// Specialization for Color
template <>
struct vec_traits<App::Color> {
    typedef App::Color vec_type;
    typedef float float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline float_type x() { return v.r; }
    inline float_type y() { return v.g; }
    inline float_type z() { return v.b; }
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
    ViewVolumeProjection (const SbViewVolume &vv);
    virtual ~ViewVolumeProjection(){}

    Base::Vector3f operator()(const Base::Vector3f &rclPt) const;
    Base::Vector3d operator()(const Base::Vector3d &rclPt) const;
    Base::Vector3f inverse (const Base::Vector3f &rclPt) const;
    Base::Vector3d inverse (const Base::Vector3d &rclPt) const;

    void setTransform(const Base::Matrix4D&);
    Base::Matrix4D getProjectionMatrix () const;

protected:
    SbViewVolume viewVolume;
    bool hasTransform;
    Base::Matrix4D transform;
};

class GuiExport Tessellator
{
public:
    Tessellator(const std::vector<SbVec2f>&);
    std::vector<int> tessellate() const;

private:
    static void tessCB(void * v0, void * v1, void * v2, void * cbdata);

private:
    std::vector<SbVec2f> polygon;
};

class GuiExport ItemViewSelection
{
public:
    ItemViewSelection(QAbstractItemView* view);
    void applyFrom(const std::vector<App::DocumentObject*> objs);

private:
    QAbstractItemView* view;
    class MatchName;
};

} // namespace Gui

#endif // GUI_UTILITIES_H
