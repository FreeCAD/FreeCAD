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
#include <Gui/PrefWidgets.h>

#include "ui_ConstraintSettingsDialog.h"
#include "ConstraintSettingsDialog.h"

using namespace SketcherGui;

ConstraintSettingsDialog::ConstraintSettingsDialog()
  : QDialog(Gui::getMainWindow()), ui(new Ui_ConstraintSettingsDialog)
{
    ui->setupUi(this);

    { // in case any signal is connected before this
        QSignalBlocker block(this);
        loadSettings();
        snapshotInitialSettings();
    }

    QObject::connect(
        ui->filterInternalAlignment, SIGNAL(stateChanged(int)),
        this                     , SLOT  (on_filterInternalAlignment_stateChanged(int))
        );
    QObject::connect(
        ui->extendedInformation, SIGNAL(stateChanged(int)),
                     this                     , SLOT  (on_extendedInformation_stateChanged(int))
        );
    QObject::connect(
        ui->visualisationTrackingFilter, SIGNAL(stateChanged(int)),
        this                     , SLOT  (on_visualisationTrackingFilter_stateChanged(int))
        );
}

void ConstraintSettingsDialog::saveSettings()
{
    ui->extendedInformation->onSave();
    ui->filterInternalAlignment->onSave();
    ui->visualisationTrackingFilter->onSave();
}

void ConstraintSettingsDialog::loadSettings()
{
    ui->extendedInformation->onRestore();
    ui->filterInternalAlignment->onRestore();
    ui->visualisationTrackingFilter->onRestore();
}

void ConstraintSettingsDialog::snapshotInitialSettings()
{
    auto isChecked = [] (auto prefwidget) {return prefwidget->checkState() == Qt::Checked;};

    extendedInformation = isChecked(ui->extendedInformation);
    filterInternalAlignment = isChecked(ui->filterInternalAlignment);
    visualisationTrackingFilter = isChecked(ui->visualisationTrackingFilter);
}

void ConstraintSettingsDialog::restoreInitialSettings()
{
    auto restoreCheck = [] (auto prefwidget, bool initialvalue) {
        if( initialvalue != (prefwidget->checkState() == Qt::Checked)) // if the state really changed
            initialvalue ? prefwidget->setCheckState(Qt::Checked) : prefwidget->setCheckState(Qt::Unchecked);
    };

    restoreCheck(ui->extendedInformation, extendedInformation);
    restoreCheck(ui->filterInternalAlignment, filterInternalAlignment);
    restoreCheck(ui->visualisationTrackingFilter, visualisationTrackingFilter);
}

void ConstraintSettingsDialog::accept()
{
    saveSettings();
    QDialog::accept();
}

void ConstraintSettingsDialog::reject()
{
    restoreInitialSettings();
    saveSettings();
    QDialog::reject();
}

void ConstraintSettingsDialog::on_filterInternalAlignment_stateChanged(int state)
{
    ui->filterInternalAlignment->onSave();
    Q_EMIT emit_filterInternalAlignment_stateChanged(state);
}

void ConstraintSettingsDialog::on_visualisationTrackingFilter_stateChanged(int state)
{
    ui->visualisationTrackingFilter->onSave();
    Q_EMIT emit_visualisationTrackingFilter_stateChanged(state);
}

void ConstraintSettingsDialog::on_extendedInformation_stateChanged(int state)
{
    ui->extendedInformation->onSave();
    Q_EMIT emit_extendedInformation_stateChanged(state);
}

ConstraintSettingsDialog::~ConstraintSettingsDialog()
{
}

#include "moc_ConstraintSettingsDialog.cpp"
