/***************************************************************************
 *   Copyright (c) 2013 Luke Parry    <l.parry@warwick.ac.uk>              *
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

#include "PreCompiled.h"
#ifndef _PreComp_
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# include <QAction>
# include <QMenu>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SoFCSelection.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Mod/Drawing/App/DrawProjGroup.h>

#include "TaskProjGroup.h"
#include "ViewProviderProjGroup.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderProjGroup, Gui::ViewProviderDocumentObject)

//**************************************************************************
// Construction/Destruction

ViewProviderProjGroup::ViewProviderProjGroup()
{
    sPixmap = "ProjGroup";
}

ViewProviderProjGroup::~ViewProviderProjGroup()
{
}

void ViewProviderProjGroup::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDocumentObject::attach(pcFeat);
}

void ViewProviderProjGroup::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderProjGroup::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDocumentObject::getDisplayModes();
    StrList.push_back("Drawing");
    return StrList;
}

void ViewProviderProjGroup::updateData(const App::Property* prop)
{
    Gui::ViewProviderDocumentObject::updateData(prop);
 
    if(prop == &(getObject()->Scale) ||
       prop == &(getObject()->ScaleType) ||
       prop == &(getObject()->Views) ||
       prop == &(getObject()->ProjectionType)) {

        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgProjGroup *projDlg = qobject_cast<TaskDlgProjGroup *>(dlg);

        if (projDlg && 
            projDlg->getViewProvider() == dynamic_cast<const ViewProviderProjGroup *>(getObject()) ) {
            projDlg->update();
        }
    } 

 }


void ViewProviderProjGroup::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    //QAction* act;
    //act = menu->addAction(QObject::tr("Show drawing"), receiver, member);
}

bool ViewProviderProjGroup::setEdit(int ModNum)
{
    // When double-clicking on the item for this sketch the
    // object unsets and sets its edit mode without closing
    // the task panel
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    TaskDlgProjGroup *projDlg = qobject_cast<TaskDlgProjGroup *>(dlg);
    if (projDlg && projDlg->getViewProvider() != this)
        projDlg = 0; // another sketch left open its task panel

    // clear the selection (convenience)
    Gui::Selection().clearSelection();

    // start the edit dialog
    if (projDlg)
        Gui::Control().showDialog(projDlg);
    else
        Gui::Control().showDialog(new TaskDlgProjGroup(getObject()));

    return true;
}

void ViewProviderProjGroup::unsetEdit(int ModNum)
{
    Gui::Control().closeDialog();
}

bool ViewProviderProjGroup::doubleClicked(void)
{
    setEdit(0);
    return true;
}

std::vector<App::DocumentObject*> ViewProviderProjGroup::claimChildren(void) const
{
    // Collect any child fields
    std::vector<App::DocumentObject*> temp;
    const std::vector<App::DocumentObject *> &views = getObject()->Views.getValues();
    try {
      for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {
          temp.push_back(*it);
      }
      return temp;
    } catch (...) {
        std::vector<App::DocumentObject*> tmp;
        return tmp;
    }
}


TechDraw::DrawProjGroup* ViewProviderProjGroup::getObject() const
{
    return dynamic_cast<TechDraw::DrawProjGroup*>(pcObject);
}
