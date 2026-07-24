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

#include <QDialog>

#include <Base/Unit.h>
#include <Mod/Sketcher3D/Sketcher3DGlobal.h>

namespace Gui
{
class PrefQuantitySpinBox;
}

namespace Sketcher3DGui
{

class Sketcher3DGuiExport DlgEditConstraintValue: public QDialog
{
    Q_OBJECT

public:
    DlgEditConstraintValue(
        const QString& title,
        const QString& label,
        double initialValue,
        const Base::Unit& unit,
        QWidget* parent = nullptr,
        bool allowNegative = false
    );
    ~DlgEditConstraintValue() override;

    double value() const;

private:
    Gui::PrefQuantitySpinBox* spin {nullptr};
};

}  // namespace Sketcher3DGui
