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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
# include <cstring>
# include <cstdlib>
# include <exception>
# include <boost/regex.hpp>
# include <QString>
# include <QStringList>
# include <QRegExp>
# include <QMessageBox>

#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Type.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/MainWindow.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Spreadsheet/App/Sheet.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "QGVPage.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "DrawGuiUtil.h"

using namespace TechDrawGui;

//===========================================================================
// validate helper routines
//===========================================================================
TechDraw::DrawPage* DrawGuiUtil::findPage(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = 0;
    //check if a DrawPage is currently displayed
    Gui::MainWindow* w = Gui::getMainWindow();
    Gui::MDIView* mv = w->activeWindow();
    MDIViewPage* mvp = dynamic_cast<MDIViewPage*>(mv);
    if (mvp) {
        QGVPage* qp = mvp->getQGVPage();
        page = qp->getDrawPage();
    } else {
        //DrawPage not displayed, check Selection and/or Document for a DrawPage
        std::vector<App::DocumentObject*> selPages = cmd->getSelection().getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
        if (selPages.empty()) {                                            //no page in selection
            selPages = cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
            if (selPages.empty()) {                                        //no page in document
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No page found"),
                                     QObject::tr("Create a page first."));
                return page;
            } else if (selPages.size() > 1) {                              //multiple pages in document
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Too many pages"),
                                     QObject::tr("Can not determine correct page."));
                return page;
            } else {                                                       //use only page in document
                page = static_cast<TechDraw::DrawPage*>(selPages.front());
            }
        } else if (selPages.size() > 1) {                                  //multiple pages in selection
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Too many pages"),
                                 QObject::tr("Select exactly 1 page."));
            return page;
        } else {                                                           //use only page in selection
            page = static_cast<TechDraw::DrawPage*>(selPages.front());
        }
    }
    return page;
}

bool DrawGuiUtil::needPage(Gui::Command* cmd)
{
    //need a Document and a Page
    bool active = false;
    if (cmd->hasActiveDocument()) {
        auto drawPageType( TechDraw::DrawPage::getClassTypeId() );
        auto selPages = cmd->getDocument()->getObjectsOfType(drawPageType);
        if (!selPages.empty()) {
            active = true;
        }
    }
    return active;
}

bool DrawGuiUtil::needView(Gui::Command* cmd, bool partOnly)
{
    bool haveView = false;
    if (cmd->hasActiveDocument()) {
        if (partOnly) {
            auto drawPartType (TechDraw::DrawViewPart::getClassTypeId());
            auto selParts = cmd->getDocument()->getObjectsOfType(drawPartType);
            if (!selParts.empty()) {
                haveView = true;
            }
        } else {
            auto drawViewType (TechDraw::DrawView::getClassTypeId());
            auto selParts = cmd->getDocument()->getObjectsOfType(drawViewType);
            if (!selParts.empty()) {
                haveView = true;
            }
        }
    }
    return haveView;
}
