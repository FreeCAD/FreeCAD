/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2019 Franck Jullien <franck.jullien@gmail.com>          *
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
# include <QAction>
# include <QMenu>
#endif

#include <App/DocumentObject.h>
#include <Gui/ActionFunction.h>
#include <Gui/Control.h>
#include <Gui/Selection.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Mod/TechDraw/App/LineGroup.h>

#include "PreferencesGui.h"
#include "QGIViewBalloon.h"
#include "TaskBalloon.h"
#include "ViewProviderBalloon.h"

using namespace TechDrawGui;
using namespace TechDraw;

PROPERTY_SOURCE(TechDrawGui::ViewProviderBalloon, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderBalloon::ViewProviderBalloon()
{
    sPixmap = "TechDraw_Balloon";

    static const char *group = "Balloon Format";

    ADD_PROPERTY_TYPE(Font, (Preferences::labelFont().c_str()), group, App::Prop_None, "The name of the font to use");
    ADD_PROPERTY_TYPE(Fontsize, (Preferences::dimFontSizeMM()),
                                group, (App::PropertyType)(App::Prop_None), "Balloon text size in units");
    int lgNumber = Preferences::lineGroup();
    auto lg = TechDraw::LineGroup::lineGroupFactory(lgNumber);
    double weight = lg->getWeight("Thin");
    delete lg;                                   //Coverity CID 174670
    ADD_PROPERTY_TYPE(LineWidth, (weight), group, (App::PropertyType)(App::Prop_None), "Leader line width");
    ADD_PROPERTY_TYPE(LineVisible, (true), group, (App::PropertyType)(App::Prop_None), "Balloon line visible or hidden");
    ADD_PROPERTY_TYPE(Color, (PreferencesGui::dimColor()), group, App::Prop_None, "Color of the balloon");
}

ViewProviderBalloon::~ViewProviderBalloon()
{
}

void ViewProviderBalloon::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDrawingView::attach(pcFeat);
}

void ViewProviderBalloon::setDisplayMode(const char* ModeName)
{
    ViewProviderDrawingView::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderBalloon::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDrawingView::getDisplayModes();

    return StrList;
}

bool ViewProviderBalloon::doubleClicked(void)
{
    startDefaultEditMode();
    return true;
}

void ViewProviderBalloon::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    QAction* act = menu->addAction(QObject::tr("Edit %1").arg(QString::fromUtf8(getObject()->Label.getValue())));
    act->setData(QVariant((int)ViewProvider::Default));
    func->trigger(act, boost::bind(&ViewProviderBalloon::startDefaultEditMode, this));

    ViewProviderDrawingView::setupContextMenu(menu, receiver, member);
}

bool ViewProviderBalloon::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        if (Gui::Control().activeDialog())  {
            return false;
        }
        // clear the selection (convenience)
        Gui::Selection().clearSelection();
        auto qgivBalloon(dynamic_cast<QGIViewBalloon*>(getQView()));
        if (qgivBalloon) {
            Gui::Control().showDialog(new TaskDlgBalloon(qgivBalloon, this));
        }
        return true;
    } else {
        return ViewProviderDrawingView::setEdit(ModNum);
    }
    return true;
}

void ViewProviderBalloon::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderDrawingView::unsetEdit(ModNum);
    }
}

void ViewProviderBalloon::updateData(const App::Property* p)
{
    ViewProviderDrawingView::updateData(p);
}

void ViewProviderBalloon::onChanged(const App::Property* p)
{
    if ((p == &Font)  ||
        (p == &Fontsize) ||
        (p == &Color) ||
        (p == &LineWidth) ||
        (p == &LineVisible)) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }
    Gui::ViewProviderDocumentObject::onChanged(p);
}

TechDraw::DrawViewBalloon* ViewProviderBalloon::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawViewBalloon*>(pcObject);
}

void ViewProviderBalloon::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
// transforms properties that had been changed
{
    // property LineWidth had the App::PropertyFloat and was changed to App::PropertyLength
    if (prop == &LineWidth && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat LineWidthProperty;
        // restore the PropertyFloat to be able to set its value
        LineWidthProperty.Restore(reader);
        LineWidth.setValue(LineWidthProperty.getValue());
    }
    else {
        ViewProviderDrawingView::handleChangedPropertyType(reader, TypeName, prop);
    }
}

bool ViewProviderBalloon::canDelete(App::DocumentObject *obj) const
{
    // deletions of a balloon object doesn't destroy anything
    // thus we can pass this action
    Q_UNUSED(obj)
    return true;
}
