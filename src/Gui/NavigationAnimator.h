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

#ifndef GUI_NAVIGATIONANIMATOR_H
#define GUI_NAVIGATIONANIMATOR_H

#include "NavigationStyle.h"
#include <QObject>
#include <memory>

namespace Gui
{

class NavigationAnimation;

class GuiExport NavigationAnimator : public QObject
{
    Q_OBJECT
public:
    NavigationAnimator();
    ~NavigationAnimator();
    void start(const std::shared_ptr<NavigationAnimation>& animation);
    bool startAndWait(const std::shared_ptr<NavigationAnimation>& animation);
    void stop();

private Q_SLOTS:
    void reset();

private:
    std::shared_ptr<NavigationAnimation> activeAnimation;
};

} // namespace Gui

#endif // GUI_NAVIGATIONANIMATOR_H
