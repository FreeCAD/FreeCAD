/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef PARTGUI_DLGSETTINGS3DVIEWIMP_H
#define PARTGUI_DLGSETTINGS3DVIEWIMP_H

#include <Gui/PropertyPage.h>
#include <memory>

namespace PartGui {
class Ui_DlgSettings3DViewPart;
/**
 * The DlgSettings3DViewPartImp class implements a preference page to change settings
 * for the Inventor viewer.
 * \author Jürgen Riegel
 */
class DlgSettings3DViewPart : public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettings3DViewPart(QWidget* parent = nullptr);
    ~DlgSettings3DViewPart() override;

protected:
    void saveSettings() override;
    void loadSettings() override;
    void changeEvent(QEvent *e) override;

private:
    void onMaxDeviationValueChanged(double);

private:
    std::unique_ptr<Ui_DlgSettings3DViewPart> ui;
    bool checkValue;
};

} // namespace Gui

#endif // PARTGUI_DLGSETTINGS3DVIEWIMP_H
