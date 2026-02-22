/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <memory>
#include "PropertyPage.h"
#include <QDialog>

class QDoubleSpinBox;

namespace Gui
{
namespace Dialog
{
class Ui_DlgSettingsNavigation;

/**
 * The Ui_DlgSettingsNavigation class implements a preference page to change settings
 * for the Inventor viewer.
 * \author Werner Mayer
 */
class DlgSettingsNavigation: public PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsNavigation(QWidget* parent = nullptr);
    ~DlgSettingsNavigation() override;

    void saveSettings() override;
    void loadSettings() override;
    void resetSettingsToDefaults() override;

private:
    void onMouseButtonClicked();
    void onNewDocViewChanged(int);

protected:
    void changeEvent(QEvent* e) override;
    void retranslate();
    void addOrientations();
    void translateOrientations();

private:
    std::unique_ptr<Ui_DlgSettingsNavigation> ui;
    double q0, q1, q2, q3;
};

class CameraDialog: public QDialog
{
    Q_OBJECT

public:
    explicit CameraDialog(QWidget* parent = nullptr);
    ~CameraDialog() override;
    void setValues(double q0, double q1, double q2, double q3);
    void getValues(double& q0, double& q1, double& q2, double& q3) const;


private:
    void onCurrentViewClicked();

private:
    QDoubleSpinBox* sb0;
    QDoubleSpinBox* sb1;
    QDoubleSpinBox* sb2;
    QDoubleSpinBox* sb3;
};

}  // namespace Dialog
}  // namespace Gui
