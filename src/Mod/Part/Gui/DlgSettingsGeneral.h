/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PARTGUI_DLGSETTINGSGENERAL_H
#define PARTGUI_DLGSETTINGSGENERAL_H

#include <Gui/PropertyPage.h>

class QButtonGroup;

namespace PartGui {

class Ui_DlgSettingsGeneral;
class DlgSettingsGeneral : public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    DlgSettingsGeneral(QWidget* parent = nullptr);
    ~DlgSettingsGeneral();

protected:
    void saveSettings();
    void loadSettings();
    void changeEvent(QEvent *e);

private:
    std::unique_ptr<Ui_DlgSettingsGeneral> ui;
};

class Ui_DlgImportExportIges;
class DlgImportExportIges : public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    DlgImportExportIges(QWidget* parent = nullptr);
    ~DlgImportExportIges();

protected:
    void saveSettings();
    void loadSettings();
    void changeEvent(QEvent *e);

private:
    std::unique_ptr<Ui_DlgImportExportIges> ui;
    QButtonGroup* bg;
};

class Ui_DlgImportExportStep;
class DlgImportExportStep : public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    DlgImportExportStep(QWidget* parent = nullptr);
    ~DlgImportExportStep();

protected:
    void saveSettings();
    void loadSettings();
    void changeEvent(QEvent *e);

private:
    std::unique_ptr<Ui_DlgImportExportStep> ui;
};

} // namespace Gui

#endif // PARTGUI_DLGSETTINGSGENERAL_H
