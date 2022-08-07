/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef SKETCHERGUI_ConstraintMultiFilter_H
#define SKETCHERGUI_ConstraintMultiFilter_H

#include <QDialog>

#include "ConstraintFilters.h"

namespace SketcherGui {

using namespace ConstraintFilter;

class Ui_ConstraintMultiFilterDialog;
class ConstraintMultiFilterDialog : public QDialog
{
    Q_OBJECT

public:
    ConstraintMultiFilterDialog();
    ~ConstraintMultiFilterDialog();

    void setMultiFilter(const FilterValueBitset & bitset);
    FilterValueBitset getMultiFilter();

public Q_SLOTS:
    void on_listMultiFilter_itemChanged(QListWidgetItem * item);
    void on_checkAllButton_clicked(bool);
    void on_uncheckAllButton_clicked(bool);

protected:
    void setCheckStateAll(Qt::CheckState);
private:
    std::unique_ptr<Ui_ConstraintMultiFilterDialog> ui;
};

}

#endif // SKETCHERGUI_ConstraintMultiFilter_H
