 /**************************************************************************
 *   Copyright (c) 2023 Wanderer Fan <wandererfan@gmail.com>               *
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

#ifndef MeasureGui_DlgPrefsAppearanceImp_H
#define MeasureGui_DlgPrefsAppearanceImp_H

#include <memory>

#include <Gui/PropertyPage.h>
#include <Mod/Measure/MeasureGlobal.h>


namespace MeasureGui {

class Ui_DlgPrefsMeasureAppearanceImp;

class DlgPrefsMeasureAppearanceImp : public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    explicit DlgPrefsMeasureAppearanceImp( QWidget* parent = nullptr );
    ~DlgPrefsMeasureAppearanceImp() override;

protected:
    void saveSettings() override;
    void loadSettings() override;
    void changeEvent(QEvent *e) override;

private:
    std::unique_ptr<Ui_DlgPrefsMeasureAppearanceImp> ui;
};

} // namespace MeasureGui

#endif // MeasureGui_DlgPrefsAppearanceImp_H

