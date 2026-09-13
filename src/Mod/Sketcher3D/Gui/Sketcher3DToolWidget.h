// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <functional>

#include <QWidget>

#include <Mod/Sketcher3D/Sketcher3DGlobal.h>

namespace Sketcher3DGui
{

class Sketcher3DGuiExport Sketcher3DToolWidget: public QWidget
{
public:
    explicit Sketcher3DToolWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {}
    ~Sketcher3DToolWidget() override = default;

    void setAcceptCallback(std::function<void()> callback)
    {
        onAccept = std::move(callback);
    }

    void accept()
    {
        if (onAccept) {
            onAccept();
        }
    }

private:
    std::function<void()> onAccept;
};

}  // namespace Sketcher3DGui
