/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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


#include <App/DocumentObject.h>
#include <Gui/Control.h>
#include "TaskSpreadsheetView.h"
#include "ViewProviderSpreadsheet.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderSpreadsheet, TechDrawGui::ViewProviderSymbol)

//**************************************************************************
// Construction/Destruction

ViewProviderSpreadsheet::ViewProviderSpreadsheet()
{
    sPixmap = "TechDraw_TreeSpreadsheet";
    ADD_PROPERTY_TYPE(ClaimSheetAsChild,
                      (false),
                      "Display Options",
                      App::Prop_None,
                      "Claim (or not) the spreadsheet source as a child of the view.");
    LegacyScaling.setValue(true);
}

ViewProviderSpreadsheet::~ViewProviderSpreadsheet()
{
}

TechDraw::DrawViewSpreadsheet* ViewProviderSpreadsheet::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawViewSpreadsheet*>(pcObject);
}

std::vector<App::DocumentObject*> ViewProviderSpreadsheet::claimChildren() const
{
    std::vector<App::DocumentObject*> temp;

    if (ClaimSheetAsChild.getValue()) {
        temp.push_back(getViewObject()->Source.getValue());
    }

    return temp;
}

bool ViewProviderSpreadsheet::setEdit(int ModNum)
{
    if (ModNum != Gui::ViewProvider::Default) {
        return Gui::ViewProviderDocumentObject::setEdit(ModNum);
    }
    if (auto* activeDialog = Gui::Control().activeDialog(getViewObject()->getDocument())) {
        // The creation command opens the task dialog before registering the newly-created
        // view as the document's edit object. In that case the correct dialog is already open.
        auto* spreadsheetDialog = qobject_cast<TaskDlgSpreadsheetView*>(activeDialog);
        return spreadsheetDialog && spreadsheetDialog->getViewObject() == getViewObject();
    }

    Gui::Control().showDialog(
        new TaskDlgSpreadsheetView(getViewObject()->findParentPage(), getViewObject()),
        getViewObject()->getDocument());
    return true;
}

void ViewProviderSpreadsheet::unsetEdit(int ModNum)
{
    if (ModNum == Gui::ViewProvider::Default) {
        Gui::Control().closeDialog(getViewObject()->getDocument());
    }
    else {
        Gui::ViewProviderDocumentObject::unsetEdit(ModNum);
    }
}

bool ViewProviderSpreadsheet::doubleClicked()
{
    startDefaultEditMode();
    return true;
}
