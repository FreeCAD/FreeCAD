 /**************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: WandererFan <wandererfan@gmail.com>                           *
 *   Based on src/Mod/FEM/Gui/DlgPrefsTechDraw1Imp.cpp                      *
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


#ifndef DRAWINGGUI_DLGPREFSTECHDRAWIMP1_H
#define DRAWINGGUI_DLGPREFSTECHDRAWIMP1_H

#include <Mod/TechDraw/Gui/ui_DlgPrefsTechDraw1.h>
#include <Gui/PropertyPage.h>

namespace TechDrawGui {

class DlgPrefsTechDraw1Imp : public Gui::Dialog::PreferencePage, public Ui_DlgPrefsTechDraw1Imp
{
    Q_OBJECT

public:
    DlgPrefsTechDraw1Imp( QWidget* parent = 0 );
    ~DlgPrefsTechDraw1Imp();

protected:
    void saveSettings();
    void loadSettings();
    void changeEvent(QEvent *e);
};

} // namespace TechDrawGui

#endif // DRAWINGGUI_DLGPREFSTECHDRAWIMP1_H
