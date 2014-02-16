/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_DIALOG_DLGSETTINGSUnitsIMP_H
#define GUI_DIALOG_DLGSETTINGSUnitsIMP_H

#include "PropertyPage.h"

namespace Gui {
namespace Dialog {

/**
 * The DlgSettingsUnitsImp class implements a preference page to change settings
 * for the Unit system.
 * \author Jürgen Riegel
 */
class Ui_DlgSettingsUnits;
class DlgSettingsUnitsImp : public PreferencePage
{
    Q_OBJECT

public:
    DlgSettingsUnitsImp(QWidget* parent = 0);
    ~DlgSettingsUnitsImp();

    void saveSettings();
    void loadSettings();

protected:
    void changeEvent(QEvent *e);

    void fillUpListBox(void);

public Q_SLOTS:
    void on_comboBox_ViewSystem_currentIndexChanged(int index);

private:
    Ui_DlgSettingsUnits* ui;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGSETTINGS3DVIEWIMP_H
