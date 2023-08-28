/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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
 #include <QMessageBox>
 #include <QTextStream>
#endif

#include <App/DocumentObject.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>

#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>

#include "ViewProviderProjGroupItem.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderProjGroupItem, TechDrawGui::ViewProviderViewPart)

//**************************************************************************
// Construction/Destruction

ViewProviderProjGroupItem::ViewProviderProjGroupItem()
{

}

ViewProviderProjGroupItem::~ViewProviderProjGroupItem()
{
}

void ViewProviderProjGroupItem::updateData(const App::Property* prop)
{
    Gui::ViewProviderDocumentObject::updateData(prop);
    TechDraw::DrawProjGroupItem* proj = getObject();
    if(!proj) {
        return;
    }

    // Set the icon pixmap depending on the orientation
    std::string projType = proj->Type.getValueAsString();

    //TODO: Once we know that ProjType is valid, sPixMap = "Proj" + projType

    if(strcmp(projType.c_str(), "Front") == 0) {
        sPixmap = "TechDraw_ProjFront";
    } else if(strcmp(projType.c_str(), "Rear") == 0) {
        sPixmap = "TechDraw_ProjRear";
    } else if(strcmp(projType.c_str(), "Right") == 0) {
        sPixmap = "TechDraw_ProjRight";
    } else if(strcmp(projType.c_str(), "Left") == 0) {
        sPixmap = "TechDraw_ProjLeft";
    } else if(strcmp(projType.c_str(), "Top") == 0) {
        sPixmap = "TechDraw_ProjTop";
    } else if(strcmp(projType.c_str(), "Bottom") == 0) {
        sPixmap = "TechDraw_ProjBottom";
    } else if(strcmp(projType.c_str(), "FrontTopLeft") == 0) {
        sPixmap = "TechDraw_ProjFrontTopLeft";
    } else if(strcmp(projType.c_str(), "FrontTopRight") == 0) {
        sPixmap = "TechDraw_ProjFrontTopRight";
    } else if(strcmp(projType.c_str(), "FrontBottomRight") == 0) {
        sPixmap = "TechDraw_ProjFrontBottomRight";
    } else if(strcmp(projType.c_str(), "FrontBottomLeft") == 0) {
        sPixmap = "TechDraw_ProjFrontBottomLeft";
    }
 }


void ViewProviderProjGroupItem::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Q_UNUSED(menu);
    Q_UNUSED(receiver);
    Q_UNUSED(member);
    //QAction* act;
    //act = menu->addAction(QObject::tr("Show drawing"), receiver, member);
}

bool ViewProviderProjGroupItem::setEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    doubleClicked();
    return true;
}

void ViewProviderProjGroupItem::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    Gui::Control().closeDialog();
}

bool ViewProviderProjGroupItem::doubleClicked()
{
    return true;
}

bool ViewProviderProjGroupItem::onDelete(const std::vector<std::string> &)
{
    // we cannot delete the anchor view, thus check if the item is the front item
    // we also cannot delete if the item has a section or detail view

    QString bodyMessage;
    QTextStream bodyMessageStream(&bodyMessage);
    bool isAnchor = false;

    // get the item and group
    TechDraw::DrawProjGroupItem* dpgi = static_cast<TechDraw::DrawProjGroupItem*>(getViewObject());
    TechDraw::DrawProjGroup* dpg = dpgi->getPGroup();
    // get the projection
    TechDraw::DrawProjGroupItem* proj = getObject();
    // check if it is the anchor projection
    if (dpg && (dpg->hasProjection(proj->Type.getValueAsString()))
        && (dpg->getAnchor() == dpgi))
        isAnchor = true;

    // get child views
    auto viewSection = getObject()->getSectionRefs();
    auto viewDetail = getObject()->getDetailRefs();
    auto viewLeader = getObject()->getLeaders();

   if (isAnchor)
   {
       // generate dialog
        bodyMessageStream << qApp->translate("Std_Delete",
            "You cannot delete the anchor view of a projection group.");
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("Std_Delete", "Object dependencies"), bodyMessage,
            QMessageBox::Ok);
        // don't allow to delete
        return false;
   }
   else if (!viewSection.empty()) {
       bodyMessageStream << qApp->translate("Std_Delete",
           "You cannot delete this view because it has a section view that would become broken.");
       QMessageBox::warning(Gui::getMainWindow(),
           qApp->translate("Std_Delete", "Object dependencies"), bodyMessage,
           QMessageBox::Ok);
       return false;
   }
   else if (!viewDetail.empty()) {
       bodyMessageStream << qApp->translate("Std_Delete",
           "You cannot delete this view because it has a detail view that would become broken.");
       QMessageBox::warning(Gui::getMainWindow(),
           qApp->translate("Std_Delete", "Object dependencies"), bodyMessage,
           QMessageBox::Ok);
       return false;
   }
   else if (!viewLeader.empty()) {
       bodyMessageStream << qApp->translate("Std_Delete",
           "You cannot delete this view because it has a leader line that would become broken.");
       QMessageBox::warning(Gui::getMainWindow(),
           qApp->translate("Std_Delete", "Object dependencies"), bodyMessage,
           QMessageBox::Ok);
       return false;
   }
   else {
        return true;
   }
}

bool ViewProviderProjGroupItem::canDelete(App::DocumentObject *obj) const
{
    // deletions of objects from a ProjGroupItem don't necessarily destroy anything
    // thus we can pass this action
    // we can warn the user if necessary in the object's ViewProvider in the onDelete() function
    Q_UNUSED(obj)
    return true;
}

TechDraw::DrawProjGroupItem* ViewProviderProjGroupItem::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawProjGroupItem*>(pcObject);
}

TechDraw::DrawProjGroupItem* ViewProviderProjGroupItem::getObject() const
{
    return getViewObject();
}
