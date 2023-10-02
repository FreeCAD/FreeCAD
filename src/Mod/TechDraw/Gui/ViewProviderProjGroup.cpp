/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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
# include <QMenu>
# include <QMessageBox>
# include <QTextStream>
#endif

#include <App/DocumentObject.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>

#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawViewDetail.h>
#include <Mod/TechDraw/App/DrawViewSection.h>

#include "TaskProjGroup.h"
#include "ViewProviderProjGroup.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderProjGroup, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderProjGroup::ViewProviderProjGroup()
{
    sPixmap = "TechDraw_TreeProjGroup";
}

ViewProviderProjGroup::~ViewProviderProjGroup()
{
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
        projDlg = nullptr; // another sketch left open its task panel

    // clear the selection (convenience)
    Gui::Selection().clearSelection();

    // start the edit dialog
    if (projDlg) {
        projDlg->setCreateMode(false);
        Gui::Control().showDialog(projDlg);
    } else {
        Gui::Control().showDialog(new TaskDlgProjGroup(getObject(), false));
    }

    return true;
}

bool ViewProviderProjGroup::doubleClicked()
{
    setEdit(0);
    return true;
}

bool ViewProviderProjGroup::onDelete(const std::vector<std::string> &)
{
    // warn the user if the ProjGroup is not empty

    QString bodyMessage;
    QTextStream bodyMessageStream(&bodyMessage);
    TechDraw::DrawProjGroupItem* Item = nullptr;
    std::vector<std::string> ViewList;

    // get the items in the group
    auto objs = claimChildren();

    // iterate over all item to check which ones have a section or detail view
    for (auto ObjectIterator : objs) {
        // get item
        Item = static_cast<TechDraw::DrawProjGroupItem*>(ObjectIterator);
        // get its section views
        auto viewSection = Item->getSectionRefs();
        // add names to a list
        if (!viewSection.empty()) {
            for (auto SecIterator : viewSection) {
                ViewList.emplace_back(SecIterator->Label.getValue());
            }
        }
        // get its detail views
        auto viewDetail = Item->getDetailRefs();
        if (!viewDetail.empty()) {
            for (auto DetIterator : viewDetail) {
                ViewList.emplace_back(DetIterator->Label.getValue());
            }
        }
        // get its leader lines
        auto viewLead = Item->getLeaders();
        if (!viewLead.empty()) {
            for (auto LeadIterator : viewLead) {
                ViewList.emplace_back(LeadIterator->Label.getValue());
            }
        }
    }

    // if there are section or detail views we cannot delete because this would break them
    if (!ViewList.empty()) {
        bodyMessageStream << qApp->translate("Std_Delete",
            "The group cannot be deleted because its items have the following\nsection or detail views, or leader lines that would get broken:");
        bodyMessageStream << '\n';
        for (const auto& ListIterator : ViewList)
            bodyMessageStream << '\n' << QString::fromUtf8(ListIterator.c_str());
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("Std_Delete", "Object dependencies"), bodyMessage,
            QMessageBox::Ok);
        return false;
    }

    if (!objs.empty())
    {
        // generate dialog
        bodyMessageStream << qApp->translate("Std_Delete",
            "The projection group is not empty, therefore\nthe following referencing objects might be lost:");
        bodyMessageStream << '\n';
        for (auto ObjIterator : objs)
            bodyMessageStream << '\n' << QString::fromUtf8(ObjIterator->Label.getValue());
        bodyMessageStream << "\n\n" << QObject::tr("Are you sure you want to continue?");
        // show and evaluate dialog
        int DialogResult = QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("Std_Delete", "Object dependencies"), bodyMessage,
            QMessageBox::Yes, QMessageBox::No);
        if (DialogResult == QMessageBox::Yes)
            return true;
        else
            return false;
    }
    else
        return true;
}

bool ViewProviderProjGroup::canDelete(App::DocumentObject *obj) const
{
    // deletions of views from a ProjGroup don't necessarily destroy anything
    // thus we can pass this action
    // we can warn the user if necessary in the object's ViewProvider in the onDelete() function
    Q_UNUSED(obj)
    return true;
}

std::vector<App::DocumentObject*> ViewProviderProjGroup::claimChildren() const
{
    // Collect any child fields
    std::vector<App::DocumentObject*> temp;
    const std::vector<App::DocumentObject *> &views = getObject()->Views.getValues();
    try {
      for (std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {
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
