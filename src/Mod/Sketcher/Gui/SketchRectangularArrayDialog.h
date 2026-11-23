// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2015 Abdullah Tahiri
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <QDialog>


namespace SketcherGui
{

class Ui_SketchRectangularArrayDialog;
class SketchRectangularArrayDialog: public QDialog
{
    Q_OBJECT

public:
    SketchRectangularArrayDialog();
    ~SketchRectangularArrayDialog() override;

    void accept() override;

    int Rows;
    int Cols;
    bool ConstraintSeparation;
    bool EqualVerticalHorizontalSpacing;
    bool Clone;

protected:
    void updateValues();

private:
    std::unique_ptr<Ui_SketchRectangularArrayDialog> ui;
};

}  // namespace SketcherGui
