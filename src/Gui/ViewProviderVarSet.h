/****************************************************************************
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
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

#ifndef GUI_ViewProviderVarSet_H
#define GUI_ViewProviderVarSet_H

#include "ViewProviderDocumentObject.h"
#include "DlgAddPropertyVarSet.h"

namespace Gui {

/** View provider associated with an App::VarSet
 */
class GuiExport ViewProviderVarSet : public ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderVarSet);
public:
    ViewProviderVarSet();
    ~ViewProviderVarSet() override = default;

    bool isShow() const override { return true; }

    bool doubleClicked() override;

    void onFinished(int);

private:
    std::unique_ptr<Dialog::DlgAddPropertyVarSet> dialog;
};

} // namespace Gui

#endif
