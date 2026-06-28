/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#pragma once

#include <Gui/PropertyPage.h>
#include <memory>

namespace Gui
{

class PrefComboBox;

namespace Dialog
{
class Ui_DlgSettingsUI;

/**
 * The DlgSettingsUI class implements a preference page to change theme settings.
 * @author Pierre-Louis Boyer
 */
class DlgSettingsUI: public PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsUI(QWidget* parent = nullptr);
    ~DlgSettingsUI() override;

    void saveSettings() override;
    void loadSettings() override;

    void loadStyleSheet();

protected:
    void changeEvent(QEvent* e) override;

    void populateStylesheets(
        const char* key,
        const char* path,
        PrefComboBox* combo,
        const char* def,
        QStringList filter = QStringList()
    );

    void openThemeEditor();

private:
    std::unique_ptr<Ui_DlgSettingsUI> ui;
};

}  // namespace Dialog
}  // namespace Gui
