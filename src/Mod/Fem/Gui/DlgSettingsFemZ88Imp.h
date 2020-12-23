 /**************************************************************************
 *   Copyright (c) 2016 FreeCAD Developers                                 *
 *   Author: Bernd Hahnebach <bernd@bimstatik.ch>                          *
 *   Based on src/Mod/Fem/Gui/DlgSettingsFemCcx.h                          *
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


#ifndef FEMGUI_DLGSETTINGSFEMZ88IMP_H
#define FEMGUI_DLGSETTINGSFEMZ88IMP_H

#include "ui_DlgSettingsFemZ88.h"
#include <Gui/PropertyPage.h>

namespace FemGui {

class DlgSettingsFemZ88Imp : public Gui::Dialog::PreferencePage, public Ui_DlgSettingsFemZ88Imp
{
    Q_OBJECT

public:
    DlgSettingsFemZ88Imp( QWidget* parent = 0 );
    ~DlgSettingsFemZ88Imp();

protected:
    void saveSettings();
    void loadSettings();
    void changeEvent(QEvent *e);
};

} // namespace FemGui

#endif // FEMGUI_DLGSETTINGSFEMZ88IMP_H
