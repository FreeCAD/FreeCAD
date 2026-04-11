/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#include <cmath>

#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/draggers/SoDragPointDragger.h>
#include <Inventor/draggers/SoHandleBoxDragger.h>
#include <Inventor/draggers/SoJackDragger.h>
#include <Inventor/manips/SoCenterballManip.h>
#include <Inventor/manips/SoHandleBoxManip.h>
#include <Inventor/manips/SoJackManip.h>
#include <Inventor/manips/SoTransformManip.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>

#include <QApplication>
#include <QMessageBox>
#include <QTextStream>

#include <App/Document.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Mod/Fem/App/FemPostFunction.h>

#include "FemSettings.h"
#include "TaskPostBoxes.h"
#include "ViewProviderAnalysis.h"
#include "ViewProviderFemPostFunction.h"


using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemPostFunctionProvider, Gui::ViewProviderDocumentObject)

ViewProviderFemPostFunctionProvider::ViewProviderFemPostFunctionProvider() = default;

ViewProviderFemPostFunctionProvider::~ViewProviderFemPostFunctionProvider() = default;

void ViewProviderFemPostFunctionProvider::onChanged(const App::Property* prop)
{
    Gui::ViewProviderDocumentObject::onChanged(prop);

    updateSize();
}

void ViewProviderFemPostFunctionProvider::updateData(const App::Property* prop)
{
    Gui::ViewProviderDocumentObject::updateData(prop);
    Fem::FemPostFunctionProvider* obj = getObject<Fem::FemPostFunctionProvider>();
    if (prop == &obj->Group) {
        updateSize();
    }
}

void ViewProviderFemPostFunctionProvider::updateSize()
{
    std::vector<App::DocumentObject*> vec = claimChildren();
    for (auto it : vec) {
        if (!it->isDerivedFrom<Fem::FemPostFunction>()) {
            continue;
        }

        ViewProviderFemPostFunction* vp = static_cast<FemGui::ViewProviderFemPostFunction*>(
            Gui::Application::Instance->getViewProvider(it)
        );
        vp->AutoScaleFactorX.setValue(SizeX.getValue());
        vp->AutoScaleFactorY.setValue(SizeY.getValue());
        vp->AutoScaleFactorZ.setValue(SizeZ.getValue());
    }
}

bool ViewProviderFemPostFunctionProvider::onDelete(const std::vector<std::string>&)
{
    // warn the user if the object has unselected children
    auto objs = claimChildren();
    return ViewProviderFemAnalysis::checkSelectedChildren(objs, this->getDocument(), "functions list");
}

bool ViewProviderFemPostFunctionProvider::canDelete(App::DocumentObject* obj) const
{
    // deletions of objects from a FemFunction don't necessarily destroy anything
    // thus we can pass this action
    // we can warn the user if necessary in the object's ViewProvider in the onDelete() function
    Q_UNUSED(obj)
    return true;
}


// ***************************************************************************

PROPERTY_SOURCE(FemGui::ViewProviderFemPostFunction, Gui::ViewProviderDocumentObject)

ViewProviderFemPostFunction::ViewProviderFemPostFunction()
    : m_autoscale(false)
{
    ADD_PROPERTY_TYPE(AutoScaleFactorX, (1), "AutoScale", App::Prop_Hidden, "Automatic scaling factor");
    ADD_PROPERTY_TYPE(AutoScaleFactorY, (1), "AutoScale", App::Prop_Hidden, "Automatic scaling factor");
    ADD_PROPERTY_TYPE(AutoScaleFactorZ, (1), "AutoScale", App::Prop_Hidden, "Automatic scaling factor");
}

ViewProviderFemPostFunction::~ViewProviderFemPostFunction()
{}

bool ViewProviderFemPostFunction::doubleClicked()
{
    Gui::Application::Instance->activeDocument()->setEdit(this, (int)ViewProvider::Default);
    return true;
}

std::vector<std::string> ViewProviderFemPostFunction::getDisplayModes() const
{
    std::vector<std::string> StrList;
    StrList.emplace_back("Default");
    return StrList;
}

bool ViewProviderFemPostFunction::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default || ModNum == 1) {

        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
        TaskDlgPost* postDlg = qobject_cast<TaskDlgPost*>(dlg);
        if (postDlg && postDlg->getView() != this) {
            postDlg = nullptr;  // another pad left open its task panel
        }
        if (dlg && !postDlg) {
            QMessageBox msgBox(Gui::getMainWindow());
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes) {
                Gui::Control().reject();
            }
            else {
                return false;
            }
        }

        // start the edit dialog
        if (postDlg) {
            Gui::Control().showDialog(postDlg);
        }
        else {
            postDlg = new TaskDlgPost(this);
            auto panel = new TaskPostFunction(this);
            postDlg->addTaskBox(panel->windowIcon().pixmap(32), panel);
            Gui::Control().showDialog(postDlg);
        }

        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);
    }
}

void ViewProviderFemPostFunction::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderDocumentObject::unsetEdit(ModNum);
    }
}

void ViewProviderFemPostFunction::onChanged(const App::Property* prop)
{
    Gui::ViewProviderDocumentObject::onChanged(prop);

    if (m_autoscale) {
        auto scale = getExtension<ViewProviderShapeExtension>()->getScaleNode();
        scale->scaleFactor = SbVec3f(
            AutoScaleFactorX.getValue(),
            AutoScaleFactorY.getValue(),
            AutoScaleFactorZ.getValue()
        );
    }
}

ShapeWidget* ViewProviderFemPostFunction::createControlWidget()
{
    auto extension = getExtension<ViewProviderShapeExtension>();
    if (!extension) {
        throw Base::AttributeError("ViewProvider does not have a ShapeExtension");
    }

    return extension->createShapeWidget();
}


// ***************************************************************************

PROPERTY_SOURCE_WITH_EXTENSIONS(FemGui::ViewProviderFemPostBoxFunction, FemGui::ViewProviderFemPostFunction)

ViewProviderFemPostBoxFunction::ViewProviderFemPostBoxFunction()
{
    ViewProviderBoxExtension::initExtension(this);

    sPixmap = "fem-post-geo-box";
    setAutoScale(false);
}

ViewProviderFemPostBoxFunction::~ViewProviderFemPostBoxFunction() = default;


// ***************************************************************************

PROPERTY_SOURCE_WITH_EXTENSIONS(
    FemGui::ViewProviderFemPostCylinderFunction,
    FemGui::ViewProviderFemPostFunction
)

ViewProviderFemPostCylinderFunction::ViewProviderFemPostCylinderFunction()
{
    ViewProviderCylinderExtension::initExtension(this);

    sPixmap = "fem-post-geo-cylinder";

    setAutoScale(false);
}

ViewProviderFemPostCylinderFunction::~ViewProviderFemPostCylinderFunction() = default;


// ***************************************************************************

PROPERTY_SOURCE_WITH_EXTENSIONS(
    FemGui::ViewProviderFemPostPlaneFunction,
    FemGui::ViewProviderFemPostFunction
)

ViewProviderFemPostPlaneFunction::ViewProviderFemPostPlaneFunction()
{
    ViewProviderPlaneExtension::initExtension(this);

    sPixmap = "fem-post-geo-plane";
    setAutoScale(false);
}

ViewProviderFemPostPlaneFunction::~ViewProviderFemPostPlaneFunction() = default;


// ***************************************************************************

PROPERTY_SOURCE_WITH_EXTENSIONS(
    FemGui::ViewProviderFemPostSphereFunction,
    FemGui::ViewProviderFemPostFunction
)

ViewProviderFemPostSphereFunction::ViewProviderFemPostSphereFunction()
{
    ViewProviderSphereExtension::initExtension(this);

    sPixmap = "fem-post-geo-sphere";
    setAutoScale(false);
}

ViewProviderFemPostSphereFunction::~ViewProviderFemPostSphereFunction() = default;


#include "moc_ViewProviderFemPostFunction.cpp"
