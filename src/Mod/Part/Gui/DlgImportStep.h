// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Mod/Part/PartGlobal.h>
#include <Gui/PropertyPage.h>
#include <QDialog>

class QButtonGroup;
class QCheckBox;

namespace PartGui
{

struct StepImportSettings
{
    bool merge = false;
    bool useLinkGroup = false;
    bool useBaseName = true;
    bool importHidden = true;
    bool reduceObjects = false;
    bool showProgress = false;
    bool expandCompound = false;
    int mode = 0;
    int codePage = -1;
};

class Ui_DlgImportStep;
class DlgImportStep: public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    explicit DlgImportStep(QWidget* parent = nullptr);
    ~DlgImportStep() override;

    void saveSettings() override;
    void loadSettings() override;

    StepImportSettings getSettings() const;

protected:
    void changeEvent(QEvent* e) override;

private:
    std::unique_ptr<Ui_DlgImportStep> ui;
};

// ----------------------------------------------------------------------------

class PartGuiExport TaskImportStep: public QDialog
{
    Q_OBJECT

public:
    explicit TaskImportStep(QWidget* parent = nullptr);
    ~TaskImportStep() override;

    bool showDialog() const;
    void accept() override;
    StepImportSettings getSettings() const;

private:
    QCheckBox* showThis;
    std::unique_ptr<DlgImportStep> ui;
};

}  // namespace PartGui
