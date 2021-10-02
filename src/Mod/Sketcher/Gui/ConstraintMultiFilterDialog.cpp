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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QPixmap>
# include <QDialog>
#endif

#include <Gui/BitmapFactory.h>
#include <Gui/MainWindow.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include "ui_ConstraintMultiFilterDialog.h"
#include "ConstraintMultiFilterDialog.h"

using namespace SketcherGui;

ConstraintMultiFilterDialog::ConstraintMultiFilterDialog(void)
  : QDialog(Gui::getMainWindow()), ui(new Ui_ConstraintMultiFilterDialog)
{
    ui->setupUi(this);

    // make filter items checkable
    ui->listMultiFilter->blockSignals(true);
    for(int i = 0; i < ui->listMultiFilter->count(); i++) {
        QListWidgetItem* item = ui->listMultiFilter->item(i);

        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

        item->setCheckState(Qt::Unchecked);
    }
    ui->listMultiFilter->blockSignals(false);

    QMetaObject::connectSlotsByName(this);
}

ConstraintMultiFilterDialog::~ConstraintMultiFilterDialog()
{
}

void ConstraintMultiFilterDialog::setMultiFilter(const std::bitset<FilterValue::NumFilterValue> & bitset)
{
    ui->listMultiFilter->blockSignals(true);
    for(int i = 0; i < ui->listMultiFilter->count(); i++) {
        QListWidgetItem* item = ui->listMultiFilter->item(i);

        if(bitset[i])
            item->setCheckState(Qt::Checked);
        else
            item->setCheckState(Qt::Unchecked);
    }
    ui->listMultiFilter->blockSignals(false);
}

std::bitset<FilterValue::NumFilterValue> ConstraintMultiFilterDialog::getMultiFilter()
{
    std::bitset<FilterValue::NumFilterValue> tmpBitset;

    for(int i = 0; i < ui->listMultiFilter->count(); i++) {
        QListWidgetItem* item = ui->listMultiFilter->item(i);

        if(item->checkState() == Qt::Checked)
            tmpBitset.set(i);
    }

    return tmpBitset;

}

void ConstraintMultiFilterDialog::on_listMultiFilter_itemChanged(QListWidgetItem * item)
{
    int filterindex = ui->listMultiFilter->row(item);

    auto aggregate = filterAggregates[filterindex];

    ui->listMultiFilter->blockSignals(true);
    if(item->checkState() == Qt::Checked) {
        for(int i = 0; i < ui->listMultiFilter->count(); i++) {
            if(aggregate[i])
                ui->listMultiFilter->item(i)->setCheckState(Qt::Checked);
        }
    }
    ui->listMultiFilter->blockSignals(false);
}

void ConstraintMultiFilterDialog::setCheckStateAll(Qt::CheckState state)
{
    ui->listMultiFilter->blockSignals(true);
    for(int i = 0; i < ui->listMultiFilter->count(); i++) {
       ui->listMultiFilter->item(i)->setCheckState(state);
    }
    ui->listMultiFilter->blockSignals(false);
}

void ConstraintMultiFilterDialog::on_checkAllButton_clicked(bool)
{
    setCheckStateAll(Qt::Checked);
}

void ConstraintMultiFilterDialog::on_uncheckAllButton_clicked(bool)
{
    setCheckStateAll(Qt::Unchecked);
}

#include "moc_ConstraintMultiFilterDialog.cpp"
