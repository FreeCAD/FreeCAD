// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
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

#pragma once

#include <optional>

#include <FCGlobal.h>

#include "StatusBarLabel.h"
#include "InputHint.h"

namespace Gui
{
class GuiExport InputHintWidget: public StatusBarLabel
{
    Q_OBJECT

public:
    explicit InputHintWidget(QWidget* parent);

    void showHints(const std::list<InputHint>& hints);
    void clearHints();

private:
    static std::optional<const char*> getCustomIconPath(InputHint::UserInput key);
    static QString inputRepresentation(InputHint::UserInput key);
    QPixmap generateKeyIcon(InputHint::UserInput key, QColor color, int height = 24);
};

}  // Namespace Gui
