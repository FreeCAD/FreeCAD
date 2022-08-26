/***************************************************************************
 *   Copyright (c) 2022                                                    *
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


#ifndef PARTGUI_DIALOG_DLGSETTINGSMEASURE_H
#define PARTGUI_DIALOG_DLGSETTINGSMEASURE_H

#include <Gui/PropertyPage.h>
#include <memory>

namespace PartGui {
class Ui_DlgSettingsMeasure;

/**
 * The DlgSettingsMeasure class implements a preference page to change color
 *   and font settings for Measure Dimensions
 */
class DlgSettingsMeasure : public Gui::Dialog::PreferencePage
{
  Q_OBJECT

public:
  explicit DlgSettingsMeasure(QWidget* parent = nullptr);
  ~DlgSettingsMeasure() override;

  void saveSettings() override;
  void loadSettings() override;

protected:
  void changeEvent(QEvent *e) override;

private:
  std::unique_ptr<Ui_DlgSettingsMeasure> ui;
  void onMeasureRefresh();
};

} // namespace PartGui

#endif // PARTGUI_DIALOG_DLGSETTINGSMEASURE_H
