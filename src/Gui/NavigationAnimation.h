// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2023 Bas Ruigrok (Rexbas) <Rexbas@proton.me>             *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef GUI_NAVIGATIONANIMATION_H
#define GUI_NAVIGATIONANIMATION_H

#include "NavigationStyle.h"
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>
#include <QVariantAnimation>

namespace Gui
{

class GuiExport NavigationAnimation : protected QVariantAnimation
{
public:
    explicit NavigationAnimation(NavigationStyle* navigation);

protected:
    NavigationStyle* navigation;

    virtual void initialize() = 0;
    virtual void update(const QVariant& value) = 0;
    virtual void stopAnimation();

private:
    bool started;

    void startAnimation(QAbstractAnimation::DeletionPolicy policy = KeepWhenStopped);
    void updateCurrentValue(const QVariant& value) override;

    friend class NavigationAnimator;
};

class GuiExport FixedTimeAnimation : public NavigationAnimation
{
public:
    explicit FixedTimeAnimation(NavigationStyle* navigation, const SbRotation& orientation,
                                const SbVec3f& rotationCenter, const SbVec3f& translation,
                                int duration);

private:
    float angularVelocity; // [rad/ms]
    SbVec3f linearVelocity; // [/ms]

    SbRotation targetOrientation;
    SbVec3f targetTranslation;

    float prevAngle;
    SbVec3f prevTranslation;

    SbVec3f rotationCenter;
    SbVec3f rotationAxis;

    void initialize() override;
    void update(const QVariant& value) override;
};

class GuiExport SpinningAnimation : public NavigationAnimation
{
public:
    explicit SpinningAnimation(NavigationStyle* navigation, const SbVec3f& axis, float velocity);

private:
    SbVec3f rotationAxis;
    float prevAngle;

    void initialize() override;
    void update(const QVariant& value) override;
    void stopAnimation() override;
};

} // namespace Gui

#endif // GUI_NAVIGATIONANIMATION_H
