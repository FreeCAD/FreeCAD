/***************************************************************************
 *   Copyright (c) 2018 FreeCAD Developers                                 *
 *   Author: Bernd Hahnebach <bernd@bimstatik.ch>                          *
 *   Based on src/Mod/Fem/Gui/DlgSettingsFemExportAbaqusCcxImp.h           *
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

#ifndef FEMGUI_DLGSETTINGSFEMINOUTVTKIMP_H
#define FEMGUI_DLGSETTINGSFEMINOUTVTKIMP_H

#include <memory>

#include <Gui/PropertyPage.h>


namespace FemGui
{

class Ui_DlgSettingsFemInOutVtk;
class DlgSettingsFemInOutVtkImp: public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsFemInOutVtkImp(QWidget* parent = nullptr);
    ~DlgSettingsFemInOutVtkImp() override;

protected:
    void saveSettings() override;
    void loadSettings() override;
    void changeEvent(QEvent* e) override;

private:
    std::unique_ptr<Ui_DlgSettingsFemInOutVtk> ui;
};

}  // namespace FemGui

#endif  // FEMGUI_DLGSETTINGSFEMINOUTVTKIMP_H
