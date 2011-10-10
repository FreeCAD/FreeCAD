/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is Drawing of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A DrawingICULAR PURPOSE.  See the         *
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
# include <QTimer>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/SoFCSelection.h>
#include <Gui/Selection.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProviderDocumentObjectGroup.h>


#include "ViewProviderPage.h"
#include <Mod/Drawing/App/FeaturePage.h>

using namespace DrawingGui;

PROPERTY_SOURCE(DrawingGui::ViewProviderDrawingPage, Gui::ViewProviderDocumentObjectGroup)


//**************************************************************************
// Construction/Destruction

ViewProviderDrawingPage::ViewProviderDrawingPage()
  : view(0)
{
    sPixmap = "Page";
    ADD_PROPERTY(HintScale,(10.0));
    ADD_PROPERTY(HintOffsetX,(10.0));
    ADD_PROPERTY(HintOffsetY,(10.0));
}

ViewProviderDrawingPage::~ViewProviderDrawingPage()
{
}

void ViewProviderDrawingPage::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDocumentObject::attach(pcFeat);
}

void ViewProviderDrawingPage::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderDrawingPage::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDocumentObject::getDisplayModes();
    StrList.push_back("Drawing");
    return StrList;
}

void ViewProviderDrawingPage::updateData(const App::Property* prop)
{
    Gui::ViewProviderDocumentObjectGroup::updateData(prop);
    if (prop->getTypeId() == App::PropertyFileIncluded::getClassTypeId()) {
        if (std::string(getPageObject()->PageResult.getValue()) != "") {
            DrawingView* view = showDrawingView();
            view->load(QString::fromUtf8(getPageObject()->PageResult.getValue()));
            if (view->isHidden())
                QTimer::singleShot(300, view, SLOT(viewAll()));
            else
                view->viewAll();
        }
    }
}

void ViewProviderDrawingPage::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Show drawing"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
}

bool ViewProviderDrawingPage::setEdit(int ModNum)
{
    doubleClicked();
    return false;
}

bool ViewProviderDrawingPage::doubleClicked(void)
{
    if (!this->view) {
        showDrawingView();
        this->view->load(QString::fromUtf8(getPageObject()->PageResult.getValue()));
        view->viewAll();
    }
    Gui::getMainWindow()->setActiveWindow(this->view);
    return true;
}

DrawingView* ViewProviderDrawingPage::showDrawingView()
{
    if (!view){
        view = new DrawingView(Gui::getMainWindow());
        view->setWindowIcon(Gui::BitmapFactory().pixmap("actions/drawing-landscape"));
        view->setWindowTitle(QObject::tr("Drawing viewer"));
        Gui::getMainWindow()->addWindow(view);
    }

    return view;
}

Drawing::FeaturePage* ViewProviderDrawingPage::getPageObject() const
{
    return dynamic_cast<Drawing::FeaturePage*>(pcObject);
}
