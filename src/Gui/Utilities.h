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
#include <QColor>
#include <App/Material.h>
#include <Base/Converter.h>
#include <Base/ViewProj.h>
#include <Inventor/SbColor.h>
#include <Inventor/SbColor4f.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbViewVolume.h>

class SbViewVolume;
class QAbstractItemView;

namespace Base
{
// Specialization for SbVec3f
template<>
struct vec_traits<SbVec3f>
{
    using vec_type = SbVec3f;
    using float_type = float;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v[0], v[1], v[2]);
    }

private:
    const vec_type& v;
};

// Specialization for SbVec3d
template<>
struct vec_traits<SbVec3d>
{
    using vec_type = SbVec3d;
    using float_type = double;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v[0], v[1], v[2]);
    }

private:
    const vec_type& v;
};

// Specialization for SbRotation
template<>
struct vec_traits<SbRotation>
{
    using vec_type = SbRotation;
    using float_type = float;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type, float_type> get() const
    {
        float_type q1, q2, q3, q4;
        v.getValue(q1, q2, q3, q4);
        return std::make_tuple(q1, q2, q3, q4);
    }

private:
    const vec_type& v;
};

// Specialization for SbColor
template<>
struct vec_traits<SbColor>
{
    using vec_type = SbColor;
    using float_type = float;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v[0], v[1], v[2]);
    }

private:
    const vec_type& v;
};

// Specialization for Color
template<>
struct vec_traits<Base::Color>
{
    using vec_type = Base::Color;
    using float_type = float;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v.r, v.g, v.b);
    }

private:
    const vec_type& v;
};

template<>
struct vec_traits<QColor>
{
    using vec_type = QColor;
    using float_type = float;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v.redF(), v.greenF(), v.blueF());
    }

private:
    const vec_type& v;
};

// Specialization for SbColor
template<>
struct color_traits<SbColor>
{
    using color_type = SbColor;
    color_traits() = default;
    explicit color_traits(const color_type& ct)
        : ct(ct)
    {}
    float redF() const
    {
        return ct[0];
    }
    float greenF() const
    {
        return ct[1];
    }
    float blueF() const
    {
        return ct[2];
    }
    float alphaF() const
    {
        return 1.0F;
    }
    void setRedF(float red)
    {
        ct[0] = red;
    }
    void setGreenF(float green)
    {
        ct[1] = green;
    }
    void setBlueF(float blue)
    {
        ct[2] = blue;
    }
    void setAlphaF(float alpha)
    {
        (void)alpha;
    }
    int red() const
    {
        return int(std::lround(ct[0] * 255.0F));
    }
    int green() const
    {
        return int(std::lround(ct[1] * 255.0F));
    }
    int blue() const
    {
        return int(std::lround(ct[2] * 255.0F));
    }
    int alpha() const
    {
        return 255;
    }
    void setRed(int red)
    {
        ct[0] = static_cast<float>(red) / 255.0F;
    }
    void setGreen(int green)
    {
        ct[1] = static_cast<float>(green) / 255.0F;
    }
    void setBlue(int blue)
    {
        ct[2] = static_cast<float>(blue) / 255.0F;
    }
    void setAlpha(int alpha)
    {
        (void)alpha;
    }
    static color_type makeColor(int red, int green, int blue, int alpha = 255)
    {
        (void)alpha;
        return color_type {
            static_cast<float>(red) / 255.0F,
            static_cast<float>(green) / 255.0F,
            static_cast<float>(blue) / 255.0F
        };
    }

private:
    color_type ct;
};

// Specialization for SbColor4f
template<>
struct color_traits<SbColor4f>
{
    using color_type = SbColor4f;
    color_traits() = default;
    explicit color_traits(const color_type& ct)
        : ct(ct)
    {}
    float redF() const
    {
        return ct[0];
    }
    float greenF() const
    {
        return ct[1];
    }
    float blueF() const
    {
        return ct[2];
    }
    float alphaF() const
    {
        return ct[3];
    }
    void setRedF(float red)
    {
        ct[0] = red;
    }
    void setGreenF(float green)
    {
        ct[1] = green;
    }
    void setBlueF(float blue)
    {
        ct[2] = blue;
    }
    void setAlphaF(float alpha)
    {
        ct[3] = alpha;
    }
    int red() const
    {
        return int(std::lround(ct[0] * 255.0F));
    }
    int green() const
    {
        return int(std::lround(ct[1] * 255.0F));
    }
    int blue() const
    {
        return int(std::lround(ct[2] * 255.0F));
    }
    int alpha() const
    {
        return int(std::lround(ct[3] * 255.0F));
    }
    void setRed(int red)
    {
        ct[0] = static_cast<float>(red) / 255.0F;
    }
    void setGreen(int green)
    {
        ct[1] = static_cast<float>(green) / 255.0F;
    }
    void setBlue(int blue)
    {
        ct[2] = static_cast<float>(blue) / 255.0F;
    }
    void setAlpha(int alpha)
    {
        ct[3] = static_cast<float>(alpha) / 255.0F;
    }
    static color_type makeColor(int red, int green, int blue, int alpha = 255)
    {
        return color_type {
            static_cast<float>(red) / 255.0F,
            static_cast<float>(green) / 255.0F,
            static_cast<float>(blue) / 255.0F,
            static_cast<float>(alpha) / 255.0F
        };
    }

private:
    color_type ct;
};

// Specialization for QColor
template<>
struct color_traits<QColor>
{
    using color_type = QColor;
    color_traits() = default;
    explicit color_traits(const color_type& ct)
        : ct(ct)
    {}
    float redF() const
    {
        return static_cast<float>(ct.redF());
    }
    float greenF() const
    {
        return static_cast<float>(ct.greenF());
    }
    float blueF() const
    {
        return static_cast<float>(ct.blueF());
    }
    float alphaF() const
    {
        return static_cast<float>(ct.alphaF());
    }
    void setRedF(float red)
    {
        ct.setRedF(red);
    }
    void setGreenF(float green)
    {
        ct.setGreenF(green);
    }
    void setBlueF(float blue)
    {
        ct.setBlueF(blue);
    }
    void setAlphaF(float alpha)
    {
        ct.setAlphaF(alpha);
    }
    int red() const
    {
        return ct.red();
    }
    int green() const
    {
        return ct.green();
    }
    int blue() const
    {
        return ct.blue();
    }
    int alpha() const
    {
        return ct.alpha();
    }
    void setRed(int red)
    {
        ct.setRed(red);
    }
    void setGreen(int green)
    {
        ct.setGreen(green);
    }
    void setBlue(int blue)
    {
        ct.setBlue(blue);
    }
    void setAlpha(int alpha)
    {
        ct.setAlpha(alpha);
    }
    static color_type makeColor(int red, int green, int blue, int alpha = 255)
    {
        return color_type {red, green, blue, alpha};
    }

private:
    color_type ct;
};

template<>
inline SbMatrix convertTo<SbMatrix, Base::Matrix4D>(const Base::Matrix4D& vec2)
{
    double dMtrx[16];
    vec2.getGLMatrix(dMtrx);
    return SbMatrix(
        dMtrx[0],
        dMtrx[1],
        dMtrx[2],
        dMtrx[3],  // clazy:exclude=rule-of-two-soft
        dMtrx[4],
        dMtrx[5],
        dMtrx[6],
        dMtrx[7],
        dMtrx[8],
        dMtrx[9],
        dMtrx[10],
        dMtrx[11],
        dMtrx[12],
        dMtrx[13],
        dMtrx[14],
        dMtrx[15]
    );
}

template<>
inline Base::Matrix4D convertTo<Base::Matrix4D, SbMatrix>(const SbMatrix& vec2)
{
    Base::Matrix4D mat;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            mat[i][j] = vec2[j][i];
        }
    }
    return mat;
}
}  // namespace Base

namespace App
{
class DocumentObject;
}
namespace Gui
{

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
