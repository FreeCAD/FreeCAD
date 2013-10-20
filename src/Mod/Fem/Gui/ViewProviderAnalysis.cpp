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

#endif

#include "ViewProviderAnalysis.h"
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Control.h>

#include <Mod/Fem/App/FemAnalysis.h>

#include "TaskDlgAnalysis.h"

using namespace FemGui;







PROPERTY_SOURCE(FemGui::ViewProviderFemAnalysis, Gui::ViewProviderDocumentObject)


ViewProviderFemAnalysis::ViewProviderFemAnalysis()
{


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

//std::vector<App::DocumentObject*> ViewProviderFemAnalysis::claimChildren3D(void)const
//{
//
//    //return static_cast<Assembly::ConstraintGroup*>(getObject())->Constraints.getValues();
//    return std::vector<App::DocumentObject*> ();
//}

void ViewProviderFemAnalysis::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    //QAction* act;
    //act = menu->addAction(QObject::tr("Edit pad"), receiver, member);
    //act->setData(QVariant((int)ViewProvider::Default));
    //PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member);
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
        
        Fem::FemAnalysis* pcAna = static_cast<Fem::FemAnalysis*>(this->getObject());

        Gui::Control().showDialog(new TaskDlgAnalysis(pcAna));

        return true;
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

// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(FemGui::ViewProviderFemAnalysisPython, FemGui::ViewProviderFemAnalysis)
/// @endcond

// explicit template instantiation
template class FemGuiExport ViewProviderPythonFeatureT<ViewProviderFemAnalysis>;
}
