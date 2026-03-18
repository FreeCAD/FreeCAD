// SPDX - License - Identifier: LGPL - 2.1 - or -later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 FreeCAD Project Association                         *
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

#ifndef SKETCHERGUI_EDITGROUPDIALOG_H
#define SKETCHERGUI_EDITGROUPDIALOG_H

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QDialog>
#endif

#include <cstdint>

namespace Ui
{
class EditGroupDialog;
}

namespace SketcherGui
{

class ViewProviderSketch;

class EditGroupDialog: public QDialog
{
    Q_OBJECT

public:
    explicit EditGroupDialog(
        ViewProviderSketch* viewProvider,
        int constraintIndex,
        QWidget* parent = nullptr
    );
    ~EditGroupDialog() override;

private Q_SLOTS:
    void on_buttonBox_accepted();

private:
    Ui::EditGroupDialog* ui;
    ViewProviderSketch* sketchView;
    int constrIndex;
};

}  // namespace SketcherGui

#endif  // SKETCHERGUI_EDITGROUPDIALOG_H
