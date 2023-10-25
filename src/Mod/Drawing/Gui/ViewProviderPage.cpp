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
#include <QMenu>
#include <QTimer>
#endif

#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/MainWindow.h>
#include <Gui/ViewProviderDocumentObjectGroup.h>
#include <Mod/Drawing/App/FeaturePage.h>

#include "ViewProviderPage.h"


using namespace DrawingGui;

PROPERTY_SOURCE(DrawingGui::ViewProviderDrawingPage, Gui::ViewProviderDocumentObjectGroup)


//**************************************************************************
// Construction/Destruction

ViewProviderDrawingPage::ViewProviderDrawingPage()
    : view(nullptr)
{
    sPixmap = "Page";
    ADD_PROPERTY(HintScale, (10.0));
    ADD_PROPERTY(HintOffsetX, (10.0));
    ADD_PROPERTY(HintOffsetY, (10.0));

    // do not show this in the property editor
    Visibility.setStatus(App::Property::Hidden, true);
    DisplayMode.setStatus(App::Property::Hidden, true);
}

ViewProviderDrawingPage::~ViewProviderDrawingPage()
{}

void ViewProviderDrawingPage::attach(App::DocumentObject* pcFeat)
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

void ViewProviderDrawingPage::show(void)
{
    // showing the drawing page should not affect its children but opens the MDI view
    // therefore do not call the method of its direct base class
    ViewProviderDocumentObject::show();
    if (!this->view) {
        showDrawingView();
        this->view->load(QString::fromUtf8(getPageObject()->PageResult.getValue()));
        view->viewAll();
    }
}

void ViewProviderDrawingPage::hide(void)
{
    // hiding the drawing page should not affect its children but closes the MDI view
    // therefore do not call the method of its direct base class
    ViewProviderDocumentObject::hide();
    if (view) {
        view->parentWidget()->deleteLater();
    }
}

void ViewProviderDrawingPage::updateData(const App::Property* prop)
{
    Gui::ViewProviderDocumentObjectGroup::updateData(prop);
    if (prop->getTypeId() == App::PropertyFileIncluded::getClassTypeId()) {
        if (std::string(getPageObject()->PageResult.getValue()) != "") {
            if (view) {
                view->load(QString::fromUtf8(getPageObject()->PageResult.getValue()));
                if (view->isHidden()) {
                    QTimer::singleShot(300, view, SLOT(viewAll()));
                }
                else {
                    view->viewAll();
                }
            }
        }
    }
    else if (pcObject && prop == &pcObject->Label) {
        if (view) {
            const char* objname = pcObject->Label.getValue();
            view->setObjectName(QString::fromUtf8(objname));
            Gui::Document* doc = Gui::Application::Instance->getDocument(pcObject->getDocument());
            view->onRelabel(doc);
        }
    }
}

bool ViewProviderDrawingPage::onDelete(const std::vector<std::string>& items)
{
    if (view) {
        view->parentWidget()->deleteLater();
    }

    return ViewProviderDocumentObjectGroup::onDelete(items);
}

void ViewProviderDrawingPage::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    menu->addAction(QObject::tr("Show drawing"), receiver, member);
}

bool ViewProviderDrawingPage::setEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    doubleClicked();
    return false;
}

bool ViewProviderDrawingPage::doubleClicked(void)
{
    show();
    Gui::getMainWindow()->setActiveWindow(this->view);
    return true;
}

DrawingView* ViewProviderDrawingPage::showDrawingView()
{
    if (!view) {
        Gui::Document* doc = Gui::Application::Instance->getDocument(this->pcObject->getDocument());
        view = new DrawingView(doc, Gui::getMainWindow());
        view->setWindowIcon(Gui::BitmapFactory().pixmap("actions/drawing-landscape"));

        const char* objname = pcObject->Label.getValue();
        view->setObjectName(QString::fromUtf8(objname));
        view->onRelabel(doc);
        view->setDocumentObject(pcObject->getNameInDocument());
        Gui::getMainWindow()->addWindow(view);
    }

    return view;
}

Drawing::FeaturePage* ViewProviderDrawingPage::getPageObject() const
{
    return dynamic_cast<Drawing::FeaturePage*>(pcObject);
}
