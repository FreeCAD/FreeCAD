/***************************************************************************
 *   Copyright (c) 2020 Chris Hennes (chennes@pioneerlibrarysystem.org)    *
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


#ifndef GUI_DIALOG_DLGSETTINGSWORKBENCHES_IMP_H
#define GUI_DIALOG_DLGSETTINGSWORKBENCHES_IMP_H

#include "PropertyPage.h"
#include <memory>

namespace Gui::Dialog {
class Ui_DlgSettingsWorkbenches;

/**
 * The DlgSettingsWorkbenchesImp class implements a pseudo-preference page explain why
 * the remaining preference pages aren't loaded yet, and to help the user do so on demand.
 * \author Jürgen Riegel
 */
class DlgSettingsWorkbenchesImp : public PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsWorkbenchesImp( QWidget* parent = nullptr );
    ~DlgSettingsWorkbenchesImp() override;

    void saveSettings() override;
    void loadSettings() override;

    static QStringList getEnabledWorkbenches();
    static QStringList getDisabledWorkbenches();

    void setStartWorkbenchComboItems();

    std::vector<std::string> _backgroundAutoloadedModules;
    std::string _startupModule;

protected:
    void buildWorkbenchList();
    void changeEvent(QEvent *e) override;

private:
    void addWorkbench(const QString& it, bool enabled);

    void saveWorkbenchSelector();
    void loadWorkbenchSelector();

    void onStartWbChangedClicked(int index);

    std::unique_ptr<Ui_DlgSettingsWorkbenches> ui;
};

} // namespace Gui::Dialog

#endif // GUI_DIALOG_DLGSETTINGSWORKBENCHES_IMP_H
