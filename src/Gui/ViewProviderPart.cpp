/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <QMenu>
# include <QPixmap>
# include <boost/bind.hpp>
# include <Inventor/nodes/SoMaterial.h>
#endif

#include <App/Part.h>
#include <App/Document.h>

#include "ActiveObjectList.h"
#include "ActionFunction.h"
#include "BitmapFactory.h"
#include "Command.h"

#include "ViewProviderPart.h"
#include "Application.h"
#include "MDIView.h"
#include "ViewParams.h"
#include "TaskElementColors.h"
#include "Control.h"
#include "ViewProviderLink.h"

using namespace Gui;


PROPERTY_SOURCE_WITH_EXTENSIONS(Gui::ViewProviderPart, Gui::ViewProviderGeometryObject)


/**
 * Creates the view provider for an object group.
 */
ViewProviderPart::ViewProviderPart()
{ 
    initExtension(this);

    ADD_PROPERTY_TYPE(OverrideMaterial, (false), 0, App::Prop_None, "Override part material");

    ADD_PROPERTY(OverrideColorList,());

    sPixmap = "Geofeaturegroup.svg";
}

ViewProviderPart::~ViewProviderPart()
{ }

App::PropertyLinkSub *ViewProviderPart::getColoredElementsProperty() const {
    if(!getObject())
        return 0;
    return Base::freecad_dynamic_cast<App::PropertyLinkSub>(
            getObject()->getPropertyByName("ColoredElements"));
}

/**
 * TODO
 * Whenever a property of the group gets changed then the same property of all
 * associated view providers of the objects of the object group get changed as well.
 */
void ViewProviderPart::onChanged(const App::Property* prop) {
    if (prop == &OverrideMaterial)
        pcShapeMaterial->setOverride(OverrideMaterial.getValue());
    else if(!isRestoring()) {
        if(prop == &OverrideColorList)
            applyColors();
    }
    inherited::onChanged(prop);
}

void ViewProviderPart::updateData(const App::Property *prop) {
    if(prop && !isRestoring() && !pcObject->isRestoring()) {
        if(prop == getColoredElementsProperty()) 
            applyColors();
    }
    inherited::updateData(prop);
}

void ViewProviderPart::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    QAction* act = menu->addAction(QObject::tr("Toggle active part"));
    func->trigger(act, boost::bind(&ViewProviderPart::doubleClicked, this));

    if(getColoredElementsProperty()) {
        act = menu->addAction(QObject::tr("Override colors..."), receiver, member);
        act->setData(QVariant((int)ViewProvider::Color));
    }

    inherited::setupContextMenu(menu, receiver, member);
}

bool ViewProviderPart::doubleClicked(void)
{
    //make the part the active one

    //first, check if the part is already active.
    App::DocumentObject* activePart = nullptr;
    auto activeDoc = Gui::Application::Instance->activeDocument();
    if(!activeDoc)
        activeDoc = getDocument();
    auto activeView = activeDoc->setActiveView(this);
    if(!activeView) 
        return false;

    activePart = activeView->getActiveObject<App::DocumentObject*> (PARTKEY);

    if (activePart == this->getObject()){
        //active part double-clicked. Deactivate.
        Gui::Command::doCommand(Gui::Command::Gui,
                "Gui.ActiveDocument.ActiveView.setActiveObject('%s', None)",
                PARTKEY);
    } else {
        //set new active part
        Gui::Command::doCommand(Gui::Command::Gui,
                "Gui.ActiveDocument.ActiveView.setActiveObject('%s', App.getDocument('%s').getObject('%s'))",
                PARTKEY,
                this->getObject()->getDocument()->getName(),
                this->getObject()->getNameInDocument());
    }

    return true;
}

bool ViewProviderPart::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Color) {
        TaskView::TaskDialog *dlg = Control().activeDialog();
        if (dlg) {
            Control().showDialog(dlg);
            return false;
        }
        Selection().clearSelection();
        return true;
    }
    return inherited::setEdit(ModNum);
}

void ViewProviderPart::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    if (ModNum == ViewProvider::Color) {
        Gui::Control().showDialog(new TaskElementColors(this));
        return;
    }
    return inherited::setEditViewer(viewer,ModNum);
}

std::map<std::string, App::Color> ViewProviderPart::getElementColors(const char *subname) const {
    std::map<std::string, App::Color> res;
    if(!getObject())
        return res;
    auto prop = getColoredElementsProperty();
    if(!prop)
        return res;
    const auto &mat = ShapeMaterial.getValue();
    return ViewProviderLink::getElementColorsFrom(*this,subname,*prop,
            OverrideColorList, OverrideMaterial.getValue(), &mat);
}

void ViewProviderPart::setElementColors(const std::map<std::string, App::Color> &colorMap) {
    if(!getObject())
        return;
    auto prop = getColoredElementsProperty();
    if(!prop)
        return;
    ViewProviderLink::setElementColorsTo(*this,colorMap,*prop,OverrideColorList, 
            &OverrideMaterial, &ShapeMaterial);
}

void ViewProviderPart::applyColors() {
    ViewProviderLink::applyColorsTo(*this);
}

void ViewProviderPart::buildChildren3D() {
    ViewProviderGeoFeatureGroupExtension::buildChildren3D();
    if(getObject() && !getObject()->isRestoring())
        applyColors();
}

void ViewProviderPart::finishRestoring() {
    inherited::finishRestoring();
    applyColors();
}

// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderPartPython, Gui::ViewProviderPart)
/// @endcond

// explicit template instantiation
template class GuiExport ViewProviderPythonFeatureT<ViewProviderPart>;
}
