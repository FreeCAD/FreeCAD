 /**************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: WandererFan <wandererfan@gmail.com>                           *
 *   Based on src/Mod/FEM/Gui/DlgPrefsTechDraw5Imp.cpp                     *
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


#ifndef DRAWINGGUI_DLGPREFSTECHDRAWIMP5_H
#define DRAWINGGUI_DLGPREFSTECHDRAWIMP5_H

#include <Mod/TechDraw/Gui/ui_DlgPrefsTechDraw5.h>
#include <Gui/PropertyPage.h>

namespace TechDrawGui {

class DlgPrefsTechDraw5Imp : public Gui::Dialog::PreferencePage, public Ui_DlgPrefsTechDraw5Imp
{
    Q_OBJECT

public:
    DlgPrefsTechDraw5Imp( QWidget* parent = 0 );
    ~DlgPrefsTechDraw5Imp();

protected:
    void saveSettings();
    void loadSettings();
    void changeEvent(QEvent *e);
};

} // namespace TechDrawGui

#endif // DRAWINGGUI_DLGPREFSTECHDRAWIMP5_H
