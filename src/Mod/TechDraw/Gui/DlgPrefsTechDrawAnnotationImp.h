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


#ifndef DRAWINGGUI_DLGPREFSTECHDRAWIMPANNOTATION_H
#define DRAWINGGUI_DLGPREFSTECHDRAWIMPANNOTATION_H

#include <Gui/PropertyPage.h>
#include <memory>

namespace TechDrawGui {
class Ui_DlgPrefsTechDrawAnnotationImp;

class DlgPrefsTechDrawAnnotationImp : public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    DlgPrefsTechDrawAnnotationImp( QWidget* parent = nullptr );
    ~DlgPrefsTechDrawAnnotationImp();

public Q_SLOTS:
    void onLineGroupChanged(int);

protected:
    void saveSettings();
    void loadSettings();
    void changeEvent(QEvent *e);

    int prefBalloonArrow(void) const;

private:
    std::unique_ptr<Ui_DlgPrefsTechDrawAnnotationImp> ui;
};

} // namespace TechDrawGui

#endif // DRAWINGGUI_DLGPREFSTECHDRAWIMPANNOTATION_H
