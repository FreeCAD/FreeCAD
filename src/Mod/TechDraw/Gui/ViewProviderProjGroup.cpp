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

#include <Base/Console.h>
#include <Base/Parameter.h>

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

#include <Mod/TechDraw/App/DrawProjGroupItem.h>


#include "TaskProjGroup.h"
#include "ViewProviderProjGroup.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderProjGroup, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderProjGroup::ViewProviderProjGroup()
{
    sPixmap = "TechDraw_Tree_ProjGroup";
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
    ViewProviderDrawingView::updateData(prop);

    if(prop == &(getObject()->Scale) ||
       prop == &(getObject()->ScaleType) ||
       prop == &(getObject()->Views) ||
       prop == &(getObject()->ProjectionType) ||
       prop == &(getObject()->LockPosition) ) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }

 }

void ViewProviderProjGroup::onChanged(const App::Property *prop)
{
    if (prop == &(getViewObject()->Scale)) {
            if (getViewObject()->ScaleType.isValue("Automatic")) {
                    getMDIViewPage()->redraw1View(getViewObject());
            }
    } else if (prop == &(getViewObject()->ScaleType)) {
        getMDIViewPage()->redraw1View(getViewObject());
    }
}

void ViewProviderProjGroup::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Q_UNUSED(menu);
    Q_UNUSED(receiver);
    Q_UNUSED(member);
}

bool ViewProviderProjGroup::setEdit(int ModNum)
{
    Q_UNUSED(ModNum);
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
    if (projDlg) {
        projDlg->setCreateMode(false);
        Gui::Control().showDialog(projDlg);
    } else {
        Gui::Control().showDialog(new TaskDlgProjGroup(getObject(),false));
    }

    return true;
}

void ViewProviderProjGroup::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderDrawingView::unsetEdit(ModNum);
    }
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

TechDraw::DrawProjGroup* ViewProviderProjGroup::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawProjGroup*>(pcObject);
}

TechDraw::DrawProjGroup* ViewProviderProjGroup::getObject() const
{
    return getViewObject();
}
