/***************************************************************************
 *   Copyright (c) 2015 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef SKETCHERGUI_SketchRectangularArrayDialog_H
#define SKETCHERGUI_SketchRectangularArrayDialog_H

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

#endif  // SKETCHERGUI_SketchRectangularArrayDialog_H
