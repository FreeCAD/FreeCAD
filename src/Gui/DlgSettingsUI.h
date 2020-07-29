/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef GUI_DIALOG_DLGSETTINGSUI_H
#define GUI_DIALOG_DLGSETTINGSUI_H

#include "PropertyPage.h"
#include <memory>

namespace Gui {
namespace Dialog {

/**
 * The DlgSettingsPie class implements a preference page to change settings
 * of Pie Menu.
 */
class DlgSettingsUI : public PreferencePage
{ 
  Q_OBJECT
  Q_PROPERTY(qreal offset1 READ offset1 WRITE setOffset1 DESIGNABLE true SCRIPTABLE true)
  Q_PROPERTY(qreal offset2 READ offset2 WRITE setOffset2 DESIGNABLE true SCRIPTABLE true)

public:
  DlgSettingsUI(QWidget* parent = 0);
  ~DlgSettingsUI();

  void saveSettings();
  void loadSettings();

  qreal offset1() const;
  void setOffset1(qreal);
  qreal offset2() const;
  void setOffset2(qreal);

protected:
  void changeEvent(QEvent *e);

protected Q_SLOTS:
  void onStateChanged();
  void onCurrentChanged(int);
  void onTimer();

private:
  class Private;
  std::unique_ptr<Private> ui;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGSETTINGSUI_H
