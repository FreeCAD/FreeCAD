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

#pragma once

#include <vector>

#include <Inventor/SbMatrix.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbViewVolume.h>

#include <FCGlobal.h>
#include <Base/ViewProj.h>

class SbViewVolume;
class QAbstractItemView;

namespace App
{
class DocumentObject;
}
namespace Gui
{

[[nodiscard]] GuiExport bool isInternalGuiTestRun();

/**
 */
class GuiExport ViewVolumeProjection: public Base::ViewProjMethod
{
public:
    explicit ViewVolumeProjection(const SbViewVolume& vv);
    ~ViewVolumeProjection() override = default;

    Base::Vector3f operator()(const Base::Vector3f& rclPt) const override;
    Base::Vector3d operator()(const Base::Vector3d& rclPt) const override;
    Base::Vector3f inverse(const Base::Vector3f& rclPt) const override;
    Base::Vector3d inverse(const Base::Vector3d& rclPt) const override;

    Base::Matrix4D getProjectionMatrix() const override;

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
    static void tessCB(void* v0, void* v1, void* v2, void* cbdata);

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

#define FC_ADD_CATALOG_ENTRY(__part__, __partclass__, __parent__) \
    SO_KIT_ADD_CATALOG_ENTRY(__part__, __partclass__, TRUE, __parent__, "", TRUE);

#define FC_SET_SWITCH(__name__, __state__) \
    do { \
        SoSwitch* sw = SO_GET_ANY_PART(this, __name__, SoSwitch); \
        assert(sw); \
        sw->whichChild = __state__; \
    } while (0)

#define FC_SET_TOGGLE_SWITCH(__name__, __state__) \
    do { \
        SoToggleSwitch* sw = SO_GET_ANY_PART(this, __name__, SoToggleSwitch); \
        assert(sw); \
        sw->on = __state__; \
    } while (0)

struct RotationComponents
{
    float angle;
    SbVec3f axis;
};

[[nodiscard]] inline RotationComponents getRotationComponents(const SbRotation& rotation)
{
    RotationComponents comps;
    rotation.getValue(comps.axis, comps.angle);

    return comps;
}

struct TransformComponents
{
    SbVec3f translation;
    SbVec3f scale;
    SbRotation rotation;
    SbRotation scaleOrientation;
};

[[nodiscard]] inline TransformComponents getMatrixTransform(const SbMatrix& matrix)
{
    TransformComponents comps;
    matrix.getTransform(comps.translation, comps.rotation, comps.scale, comps.scaleOrientation);

    return comps;
}

}  // namespace Gui
