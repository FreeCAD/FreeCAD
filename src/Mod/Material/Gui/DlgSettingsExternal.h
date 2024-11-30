/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#ifndef MATGUI_DLGSETTINGSEXTERNAL_H
#define MATGUI_DLGSETTINGSEXTERNAL_H

#include <Gui/PropertyPage.h>
#include <memory>


namespace MatGui
{
class Ui_DlgSettingsExternal;

class DlgSettingsExternal: public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsExternal(QWidget* parent = nullptr);
    ~DlgSettingsExternal() override;

protected:
    void saveSettings() override;
    void loadSettings() override;
    void loadInterfaces();
    void changeEvent(QEvent* e) override;

    std::string getPreferences() const;
    std::string getPreferencesInterfaces() const;

private:
    QString toPerCent(double value) const;

    std::unique_ptr<Ui_DlgSettingsExternal> ui;
};

}  // namespace MatGui

#endif  // MATGUI_DLGSETTINGSDATABASE_H
