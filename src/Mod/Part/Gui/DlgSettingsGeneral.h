// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <Gui/PropertyPage.h>

class QButtonGroup;

namespace PartGui
{

class Ui_DlgSettingsGeneral;
class DlgSettingsGeneral: public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsGeneral(QWidget* parent = nullptr);
    ~DlgSettingsGeneral() override;

protected:
    void saveSettings() override;
    void loadSettings() override;
    void changeEvent(QEvent* e) override;

private:
    std::unique_ptr<Ui_DlgSettingsGeneral> ui;
};

class Ui_DlgImportExportIges;
class DlgImportExportIges: public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    explicit DlgImportExportIges(QWidget* parent = nullptr);
    ~DlgImportExportIges() override;

protected:
    void saveSettings() override;
    void loadSettings() override;
    void changeEvent(QEvent* e) override;

private:
    std::unique_ptr<Ui_DlgImportExportIges> ui;
    QButtonGroup* bg;
};

class DlgExportStep;
class DlgImportStep;
class DlgExportHeaderStep;
class DlgImportExportStep: public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    explicit DlgImportExportStep(QWidget* parent = nullptr);
    ~DlgImportExportStep() override;

protected:
    void saveSettings() override;
    void loadSettings() override;
    void changeEvent(QEvent* e) override;

private:
    DlgExportStep* exportStep;
    DlgImportStep* importStep;
    DlgExportHeaderStep* headerStep;
};

}  // namespace PartGui
