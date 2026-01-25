 /**************************************************************************
 *   Copyright (c) 2020 FreeCAD Developers                                 *
 *   Author: Uwe Stöhr <uwestoehr@lyx.org>                                 *
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

#include <memory>
#include <Gui/PrefWidgets.h>
#include <Gui/PropertyPage.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

namespace TechDraw {
class LineGenerator;
enum class ArrowType : int;
}

namespace TechDrawGui {
class Ui_DlgPrefsTechDrawAnnotationImp;

class DlgPrefsTechDrawAnnotationImp : public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    explicit DlgPrefsTechDrawAnnotationImp( QWidget* parent = nullptr );
    ~DlgPrefsTechDrawAnnotationImp() override;

public Q_SLOTS:
    void onLineGroupChanged(int);
    void onLineStandardChanged(int);

protected:
    void saveSettings() override;
    void loadSettings() override;
    void changeEvent(QEvent *e) override;

    TechDraw::ArrowType prefBalloonArrow() const;
    int prefBalloonShape() const;
    int prefMattingStyle() const;
    void loadLineStyleBoxes();

private:
    std::unique_ptr<Ui_DlgPrefsTechDrawAnnotationImp> ui;
    TechDraw::LineGenerator* m_lineGenerator;
};

} // namespace TechDrawGui