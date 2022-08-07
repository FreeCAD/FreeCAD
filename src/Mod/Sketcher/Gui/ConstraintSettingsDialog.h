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

#ifndef SKETCHERGUI_ConstraintSettingsDialog_H
#define SKETCHERGUI_ConstraintSettingsDialog_H

#include <QDialog>

#include "ConstraintFilters.h"

namespace SketcherGui {

using namespace ConstraintFilter;

class Ui_ConstraintSettingsDialog;
class ConstraintSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    ConstraintSettingsDialog();
    ~ConstraintSettingsDialog();

Q_SIGNALS:
    void emit_filterInternalAlignment_stateChanged(int);
    void emit_extendedInformation_stateChanged(int);
    void emit_visualisationTrackingFilter_stateChanged(int);

public Q_SLOTS:
    void accept();
    void reject();
    void on_filterInternalAlignment_stateChanged(int state);
    void on_extendedInformation_stateChanged(int state);
    void on_visualisationTrackingFilter_stateChanged(int state);

private:
    void saveSettings();
    void loadSettings();
    void snapshotInitialSettings();
    void restoreInitialSettings();

private:
    std::unique_ptr<Ui_ConstraintSettingsDialog> ui;
    bool extendedInformation;
    bool filterInternalAlignment;
    bool visualisationTrackingFilter;
};

}

#endif // SKETCHERGUI_ConstraintSettingsDialog_H
