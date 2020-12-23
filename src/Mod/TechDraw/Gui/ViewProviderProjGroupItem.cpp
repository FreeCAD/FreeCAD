/***************************************************************************
 *   Copyright (c) 2014 Luke Parry    <l.parry@warwick.ac.uk>              *
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
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>


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

void ViewProviderProjGroupItem::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderViewPart::attach(pcFeat);
}

void ViewProviderProjGroupItem::setDisplayMode(const char* ModeName)
{
    ViewProviderViewPart::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderProjGroupItem::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderViewPart::getDisplayModes();
    StrList.push_back("Drawing");
    return StrList;
}

void ViewProviderProjGroupItem::updateData(const App::Property* prop)
{
    Gui::ViewProviderDocumentObject::updateData(prop);
    TechDraw::DrawProjGroupItem* proj = getObject();

    if(proj) {
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

bool ViewProviderProjGroupItem::doubleClicked(void)
{
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
