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

#include "PreCompiled.h"
#include "NavigationAnimation.h"
#include <Inventor/nodes/SoCamera.h>

#include <numbers>

using namespace Gui;

NavigationAnimation::NavigationAnimation(NavigationStyle* navigation)
    : navigation(navigation)
{}

void NavigationAnimation::updateCurrentValue(const QVariant& value)
{
    if (state() != QAbstractAnimation::State::Running) {
        return;
    }
    update(value);
}

void NavigationAnimation::onStop([[maybe_unused]] bool finished)
{}

FixedTimeAnimation::FixedTimeAnimation(NavigationStyle* navigation, const SbRotation& orientation,
                                       const SbVec3f& rotationCenter, const SbVec3f& translation,
                                       int duration, const QEasingCurve::Type easingCurve)
    : NavigationAnimation(navigation)
    , targetOrientation(orientation)
    , targetTranslation(translation)
    , rotationCenter(rotationCenter)
{
    setDuration(duration);
    setStartValue(0.0);
    setEndValue(duration * 1.0);
    setEasingCurve(easingCurve);
}

void FixedTimeAnimation::initialize()
{
#if (COIN_MAJOR_VERSION * 100 + COIN_MINOR_VERSION * 10 + COIN_MICRO_VERSION < 403)
    navigation->findBoundingSphere();
#endif

    prevAngle = 0;
    prevTranslation = SbVec3f(0, 0, 0);

    // Find an axis and angle to rotate from the camera orientation to the target orientation using post-multiplication
    SbVec3f rotationAxisPost;
    float angle;
    SbRotation(navigation->getCamera()->orientation.getValue().inverse() * targetOrientation).getValue(rotationAxisPost, angle);
    if (angle > std::numbers::pi) {
        angle -= float(2 * std::numbers::pi);
    }

    // Convert post-multiplication axis to a pre-multiplication axis
    navigation->getCamera()->orientation.getValue().inverse().multVec(rotationAxisPost, rotationAxis);

    angularVelocity = angle / duration();
    linearVelocity = targetTranslation / duration();
}

/**
 * @param value The elapsed time
 */
void FixedTimeAnimation::update(const QVariant& value)
{
    SoCamera* camera = navigation->getCamera();
    if (!camera) {
        return;
    }

    float angle = value.toFloat() * angularVelocity;
    SbVec3f translation = value.toFloat() * linearVelocity;

    SbRotation rotation(rotationAxis, angle - prevAngle);

    camera->position = camera->position.getValue() - prevTranslation;
    navigation->reorientCamera(camera, rotation, rotationCenter);
    camera->position = camera->position.getValue() + translation;

    prevAngle = angle;
    prevTranslation = translation;
}

/**
 * @param finished True when the animation is finished, false when interrupted
 */
void FixedTimeAnimation::onStop(bool finished)
{
    if (finished) {
        SoCamera* camera = navigation->getCamera();
        if (!camera) {
            return;
        }

        // Set exact target orientation
        camera->orientation = targetOrientation;
        camera->position = camera->position.getValue() + targetTranslation - prevTranslation;
    }
}

/**
 * @param navigation The navigation style
 * @param axis The rotation axis in screen coordinates
 * @param velocity The angular velocity in radians per second
 */
SpinningAnimation::SpinningAnimation(NavigationStyle* navigation, const SbVec3f& axis,
                                     float velocity)
    : NavigationAnimation(navigation)
    , rotationAxis(axis)
{
    setDuration((2 * std::numbers::pi / velocity) * 1000.0);
    setStartValue(0.0);
    setEndValue(2 * std::numbers::pi);
    setLoopCount(-1);
}

void SpinningAnimation::initialize()
{
#if (COIN_MAJOR_VERSION * 100 + COIN_MINOR_VERSION * 10 + COIN_MICRO_VERSION < 403)
    navigation->findBoundingSphere();
#endif

    prevAngle = 0;

    navigation->setViewing(true);
    navigation->setViewingMode(NavigationStyle::SPINNING);
}

/**
 * @param value The angle in radians
 */
void SpinningAnimation::update(const QVariant& value)
{
    SoCamera* camera = navigation->getCamera();
    if (!camera) {
        return;
    }

    SbRotation deltaRotation = SbRotation(rotationAxis, value.toFloat() - prevAngle);
    navigation->reorientCamera(camera, deltaRotation);

    prevAngle = value.toFloat();
}

/**
 * @param finished True when the animation is finished, false when interrupted
 */
void SpinningAnimation::onStop([[maybe_unused]] bool finished)
{
    if (navigation->getViewingMode() != NavigationStyle::SPINNING) {
        return;
    }
    navigation->setViewingMode(navigation->isViewing() ? NavigationStyle::IDLE : NavigationStyle::INTERACT);
}
