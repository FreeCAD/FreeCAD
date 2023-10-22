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
#include "NavigationAnimator.h"
#include "NavigationAnimation.h"
#include <QEventLoop>

using namespace Gui;

NavigationAnimator::NavigationAnimator()
    : activeAnimation(nullptr)
{}

NavigationAnimator::~NavigationAnimator()
{
    stop();
}

/**
 * @brief Start an animation
 *
 * @param animation The animation to start
 */
void NavigationAnimator::start(const std::shared_ptr<NavigationAnimation>& animation)
{
    stop();
    activeAnimation = animation;
    activeAnimation->initialize();

    connect(activeAnimation.get(), &NavigationAnimation::finished, this, &NavigationAnimator::reset);
    activeAnimation->start();
}

/**
 * @brief Start an animation and wait for it to finish
 *
 * @param animation The animation to start
 * @return True if the animation finished, false if interrupted
 */
bool NavigationAnimator::startAndWait(const std::shared_ptr<NavigationAnimation>& animation)
{
    stop();
    bool finished = true;
    QEventLoop loop;
    loop.connect(animation.get(), &NavigationAnimation::finished,
                 [&loop, &finished, &animation]() { // clazy:exclude=lambda-in-connect
                     if (animation->state() == QAbstractAnimation::State::Running) {
                         finished = false;
                     }

                     loop.quit();
                 });
    start(animation);
    loop.exec();
    return finished;
}

/**
 * @brief Stops an active animation
 */
void NavigationAnimator::stop()
{
    if (activeAnimation != nullptr && activeAnimation->state() != QAbstractAnimation::State::Stopped) {
        Q_EMIT activeAnimation->finished();
    }
}

/**
 * @brief Stops the animation and releases ownership of the animation
 *
 * Is called when the animation finished() signal is received which is triggered when the animation
 * is finished or when the animation is interrupted by NavigationAnimator::stop()
 */
void NavigationAnimator::reset() {
    disconnect(activeAnimation.get(), &NavigationAnimation::finished, 0, 0);
    activeAnimation->stopAnimation();
    activeAnimation.reset();
}
