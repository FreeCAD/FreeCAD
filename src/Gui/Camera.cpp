// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "Camera.h"
#include "Utilities.h"

#include <App/Application.h>

#include <cmath>

using namespace Gui;


SbRotation Camera::top()
{
    return {0, 0, 0, 1};
}

SbRotation Camera::bottom()
{
    return {1, 0, 0, 0};
}

SbRotation Camera::front()
{
    auto root = sqrtf(2.0) / 2.0f;
    return {root, 0, 0, root};
}

SbRotation Camera::rear()
{
    auto root = sqrtf(2.0) / 2.0f;
    return {0, root, root, 0};
}

SbRotation Camera::right()
{
    return {0.5, 0.5, 0.5, 0.5};
}

SbRotation Camera::left()
{
    return {-0.5, 0.5, 0.5, -0.5};
}

SbRotation Camera::isometric()
{
    // The values here are precalculated as our quaternion implementation
    // does not support calculating the values in compile time.
    // The values are verified with unit tests.
    return {0.424708F, 0.17592F, 0.339851F, 0.820473F};
}

SbRotation Camera::dimetric()
{
    // The values here are precalculated as our quaternion implementation
    // does not support calculating the values in compile time.
    // The values are verified with unit tests.

    // While there are multiple ways to calculate the dimetric rotation,
    // we use one which is similar to other CAD applications.
    return {0.567952F, 0.103751F, 0.146726F, 0.803205F};
}

SbRotation Camera::trimetric()
{
    // The values here are precalculated as our quaternion implementation
    // does not support calculating the values in compile time.
    // The values are verified with unit tests.

    // While there are multiple ways to calculate the trimetric rotation,
    // we use one which is similar to other CAD applications.
    return {0.446015F, 0.119509F, 0.229575F, 0.856787F};
}

SbRotation Camera::rotation(Camera::Orientation view)
{
    switch (view) {
        case Top:
            return top();
        case Bottom:
            return bottom();
        case Front:
            return front();
        case Rear:
            return rear();
        case Right:
            return right();
        case Left:
            return left();
        case Isometric:
            return isometric();
        case Dimetric:
            return dimetric();
        case Trimetric:
            return trimetric();
        default:
            return top();
    }
}

SbRotation Camera::rotation(const std::string& view, Camera::Orientation fallback)
{
    if (view == "Top") {
        return rotation(Top);
    }
    if (view == "Bottom") {
        return rotation(Bottom);
    }
    if (view == "Front") {
        return rotation(Front);
    }
    if (view == "Rear") {
        return rotation(Rear);
    }
    if (view == "Left") {
        return rotation(Left);
    }
    if (view == "Right") {
        return rotation(Right);
    }
    if (view == "Isometric") {
        return rotation(Isometric);
    }
    if (view == "Dimetric") {
        return rotation(Dimetric);
    }
    if (view == "Trimetric") {
        return rotation(Trimetric);
    }
    if (view == "Custom") {
        auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View/Custom"
        );
        SbRotation rot;
        rot.setValue(
            static_cast<float>(hGrp->GetFloat("Q0", 0)),
            static_cast<float>(hGrp->GetFloat("Q1", 0)),
            static_cast<float>(hGrp->GetFloat("Q2", 0)),
            static_cast<float>(hGrp->GetFloat("Q3", 1))
        );
        return rot;
    }

    return rotation(fallback);
}

SbRotation Camera::defaultOrientation(const char* fallbackView)
{
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    return rotation(hGrp->GetASCII("NewDocumentCameraOrientation", fallbackView), Top);
}

bool Camera::rotationsMatch(const SbRotation& lhs, const SbRotation& rhs, float squaredTolerance)
{
    float l0 {};
    float l1 {};
    float l2 {};
    float l3 {};
    float r0 {};
    float r1 {};
    float r2 {};
    float r3 {};
    lhs.getValue(l0, l1, l2, l3);
    rhs.getValue(r0, r1, r2, r3);
    const float dot = l0 * r0 + l1 * r1 + l2 * r2 + l3 * r3;
    const float absDot = std::fabs(dot);
    // For unit quaternions, q and -q encode the same rotation, so compare
    // against the closer sign.
    const float squaredDistance = 2.0F * (1.0F - absDot);
    return squaredDistance <= squaredTolerance;
}

Base::Rotation Camera::convert(Camera::Orientation view)
{
    return convert(Camera::rotation(view));
}

Base::Rotation Camera::convert(const SbRotation& rot)
{
    return Base::convertTo<Base::Rotation>(rot);
}

SbRotation Camera::convert(const Base::Rotation& rot)
{
    return Base::convertTo<SbRotation>(rot);
}
