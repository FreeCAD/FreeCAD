/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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
# include <Standard_math.hxx>
# include <boost/bind.hpp>
# include <QAction>
# include <QMenu>
#endif

#include "ViewProviderAnalysis.h"
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Control.h>
#include <Gui/ActionFunction.h>

#include <Mod/Fem/App/FemAnalysis.h>
#include <Mod/Fem/App/FemSolverObject.h>
#include <Mod/Fem/App/FemMeshObject.h>
#include <Mod/Fem/App/FemSetObject.h>
#include <Mod/Fem/App/FemConstraint.h>
#include <App/MaterialObject.h>

#include "TaskDlgAnalysis.h"

using namespace FemGui;


/* TRANSLATOR FemGui::ViewProviderFemAnalysis */

PROPERTY_SOURCE(FemGui::ViewProviderFemAnalysis, Gui::ViewProviderDocumentObject)


ViewProviderFemAnalysis::ViewProviderFemAnalysis()
{
    sPixmap = "fem-analysis";
}

ViewProviderFemAnalysis::~ViewProviderFemAnalysis()
{

}

bool ViewProviderFemAnalysis::doubleClicked(void)
{
    Gui::Command::assureWorkbench("FemWorkbench");
    Gui::Command::addModule(Gui::Command::Gui,"FemGui");
    Gui::Command::doCommand(Gui::Command::Gui,"FemGui.setActiveAnalysis(App.activeDocument().%s)",this->getObject()->getNameInDocument());
    return true;
}

std::vector<App::DocumentObject*> ViewProviderFemAnalysis::claimChildren(void)const
{
    std::vector<App::DocumentObject*> temp(static_cast<Fem::FemAnalysis*>(getObject())->Member.getValues());

    return temp;
}

void ViewProviderFemAnalysis::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    QAction* act = menu->addAction(tr("Activate analysis"));
    func->trigger(act, boost::bind(&ViewProviderFemAnalysis::doubleClicked, this));
}

bool ViewProviderFemAnalysis::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        //// When double-clicking on the item for this pad the
        //// object unsets and sets its edit mode without closing
        //// the task panel
        //Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        //TaskDlgAnalysis *anaDlg = qobject_cast<TaskDlgAnalysis *>(dlg);
        //if (padDlg && anaDlg->getPadView() != this)
        //    padDlg = 0; // another pad left open its task panel
        //if (dlg && !padDlg) {
        //    QMessageBox msgBox;
        //    msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
        //    msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
        //    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        //    msgBox.setDefaultButton(QMessageBox::Yes);
        //    int ret = msgBox.exec();
        //    if (ret == QMessageBox::Yes)
        //        Gui::Control().closeDialog();
        //    else
        //        return false;
        //}

        // start the edit dialog
//        if (padDlg)
//            Gui::Control().showDialog(padDlg);
//        else
        
        //Fem::FemAnalysis* pcAna = static_cast<Fem::FemAnalysis*>(this->getObject());
        //Gui::Control().showDialog(new TaskDlgAnalysis(pcAna));
        //return true;
        return false;
    }
    else {
        return Gui::ViewProviderDocumentObject::setEdit(ModNum);
    }
}

void ViewProviderFemAnalysis::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        Gui::ViewProviderDocumentObject::unsetEdit(ModNum);
    }
}

bool ViewProviderFemAnalysis::onDelete(const std::vector<std::string> &)
{
    //// get the support and Sketch
    //PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(getObject()); 
    //Sketcher::SketchObject *pcSketch = 0;
    //App::DocumentObject    *pcSupport = 0;
    //if (pcPad->Sketch.getValue()){
    //    pcSketch = static_cast<Sketcher::SketchObject*>(pcPad->Sketch.getValue()); 
    //    pcSupport = pcSketch->Support.getValue();
    //}

    //// if abort command deleted the object the support is visible again
    //if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
    //    Gui::Application::Instance->getViewProvider(pcSketch)->show();
    //if (pcSupport && Gui::Application::Instance->getViewProvider(pcSupport))
    //    Gui::Application::Instance->getViewProvider(pcSupport)->show();

    return true;
}

bool ViewProviderFemAnalysis::canDragObjects() const
{
    return true;
}

bool ViewProviderFemAnalysis::canDragObject(App::DocumentObject* obj) const
{
    if (!obj)
        return false;
    if (obj->getTypeId().isDerivedFrom(Fem::FemMeshObject::getClassTypeId()))
        return true;
    else if (obj->getTypeId().isDerivedFrom(Fem::FemSolverObject::getClassTypeId()))
        return true;
    else if (obj->getTypeId().isDerivedFrom(Fem::Constraint::getClassTypeId()))
        return true;
    else if (obj->getTypeId().isDerivedFrom(Fem::FemSetObject::getClassTypeId()))
        return true;
    else if (obj->getTypeId().isDerivedFrom(Base::Type::fromName("Fem::FeaturePython")))
        return true;
    else if (obj->getTypeId().isDerivedFrom(App::MaterialObject::getClassTypeId()))
        return true;
    else
        return false;
}

void ViewProviderFemAnalysis::dragObject(App::DocumentObject* obj)
{
    Fem::FemAnalysis* analyze = static_cast<Fem::FemAnalysis*>(getObject());
    std::vector<App::DocumentObject*> fem = analyze->Member.getValues();
    for (std::vector<App::DocumentObject*>::iterator it = fem.begin(); it != fem.end(); ++it) {
        if (*it == obj) {
            fem.erase(it);
            analyze->Member.setValues(fem);
            break;
        }
    }
}

bool ViewProviderFemAnalysis::canDropObjects() const
{
    return true;
}

bool ViewProviderFemAnalysis::canDropObject(App::DocumentObject* obj) const
{
    return canDragObject(obj);
}

void ViewProviderFemAnalysis::dropObject(App::DocumentObject* obj)
{
    Fem::FemAnalysis* analyze = static_cast<Fem::FemAnalysis*>(getObject());
    std::vector<App::DocumentObject*> fem = analyze->Member.getValues();
    fem.push_back(obj);
    analyze->Member.setValues(fem);
}

// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(FemGui::ViewProviderFemAnalysisPython, FemGui::ViewProviderFemAnalysis)
/// @endcond

// explicit template instantiation
template class FemGuiExport ViewProviderPythonFeatureT<ViewProviderFemAnalysis>;
}
