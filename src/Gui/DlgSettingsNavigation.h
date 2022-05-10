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


#ifndef GUI_DIALOG_DLGSETTINGSNAVIGATION_H
#define GUI_DIALOG_DLGSETTINGSNAVIGATION_H

#include <memory>
#include "PropertyPage.h"
#include <QDialog>

class QDoubleSpinBox;

namespace Gui {
namespace Dialog {
class Ui_DlgSettingsNavigation;

/**
 * The Ui_DlgSettingsNavigation class implements a preference page to change settings
 * for the Inventor viewer.
 * \author Werner Mayer
 */
class DlgSettingsNavigation : public PreferencePage
{
    Q_OBJECT

public:
    DlgSettingsNavigation(QWidget* parent = nullptr);
    ~DlgSettingsNavigation();

    void saveSettings();
    void loadSettings();

private Q_SLOTS:
    void on_mouseButton_clicked();
    void onNewDocViewChanged(int);

protected:
    void changeEvent(QEvent *e);
    void retranslate();

private:
    std::unique_ptr<Ui_DlgSettingsNavigation> ui;
    double q0, q1, q2, q3;
};

class CameraDialog : public QDialog
{
    Q_OBJECT

public:
    CameraDialog(QWidget* parent=nullptr);
    ~CameraDialog();
    void setValues(double q0, double q1, double q2, double q3);
    void getValues(double& q0, double& q1, double& q2, double& q3) const;


private Q_SLOTS:
    void on_currentView_clicked();

private:
    QDoubleSpinBox* sb0;
    QDoubleSpinBox* sb1;
    QDoubleSpinBox* sb2;
    QDoubleSpinBox* sb3;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGSETTINGSNAVIGATION_H
