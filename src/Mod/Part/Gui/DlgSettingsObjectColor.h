/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PARTGUI_DIALOG_DLGSETTINGSOBJECTCOLOR_H
#define PARTGUI_DIALOG_DLGSETTINGSOBJECTCOLOR_H

#include <Gui/PropertyPage.h>
#include <memory>

namespace PartGui {
class Ui_DlgSettingsObjectColor;

/**
 * The DlgSettingsObjectColor class implements a preference page to change color settings
 * for data objects.
 * @author Werner Mayer
 */
class DlgSettingsObjectColor : public Gui::Dialog::PreferencePage
{ 
  Q_OBJECT

public:
  DlgSettingsObjectColor(QWidget* parent = nullptr);
  ~DlgSettingsObjectColor();

  void saveSettings();
  void loadSettings();

protected:
  void changeEvent(QEvent *e);

private:
  std::unique_ptr<Ui_DlgSettingsObjectColor> ui;
};

} // namespace PartGui

#endif // PARTGUI_DIALOG_DLGSETTINGSOBJECTCOLOR_H
