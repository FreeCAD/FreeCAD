/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2014              *
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


#ifndef PATHGUI_DIALOG_DLGSETTINGSPATHCOLOR_H
#define PATHGUI_DIALOG_DLGSETTINGSPATHCOLOR_H

#include "ui_DlgSettingsPathColor.h"
#include <Gui/PropertyPage.h>

namespace PathGui {

class DlgSettingsPathColor : public Gui::Dialog::PreferencePage, public Ui_DlgSettingsPathColor
{ 
  Q_OBJECT

public:
  DlgSettingsPathColor(QWidget* parent = 0);
  ~DlgSettingsPathColor();

  void saveSettings();
  void loadSettings();

protected:
  void changeEvent(QEvent *e);
};

} // namespace PathGui

#endif // PATHGUI_DIALOG_DLGSETTINGSPATHCOLOR_H
