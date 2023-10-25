/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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
#include <Inventor/nodes/SoSeparator.h>
#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QTextStream>
#endif

#include <App/Document.h>
#include <App/MaterialObject.h>
#include <App/TextDocument.h>
#include <Gui/ActionFunction.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Gui/Workbench.h>
#include <Gui/WorkbenchManager.h>
#include <Mod/Fem/App/FemAnalysis.h>
#include <Mod/Fem/App/FemConstraint.h>
#include <Mod/Fem/App/FemMeshObject.h>
#include <Mod/Fem/App/FemResultObject.h>
#include <Mod/Fem/App/FemSetObject.h>
#include <Mod/Fem/App/FemSolverObject.h>
#ifdef FC_USE_VTK
#include <Mod/Fem/App/FemPostObject.h>
#endif

#include "TaskDlgAnalysis.h"
#include "ViewProviderAnalysis.h"


using namespace FemGui;

ViewProviderFemHighlighter::ViewProviderFemHighlighter()
{
    annotate = new SoSeparator();
    annotate->ref();
}

ViewProviderFemHighlighter::~ViewProviderFemHighlighter()
{
    annotate->unref();
}

void ViewProviderFemHighlighter::attach(ViewProviderFemAnalysis* view)
{
    SoGroup* root = view->getRoot();
    root->addChild(annotate);
}

void ViewProviderFemHighlighter::highlightView(Gui::ViewProviderDocumentObject* view)
{
    annotate->removeAllChildren();

    if (view) {
        annotate->addChild(view->getRoot());
    }
}

// ----------------------------------------------------------------------------

/* TRANSLATOR FemGui::ViewProviderFemAnalysis */

PROPERTY_SOURCE(FemGui::ViewProviderFemAnalysis, Gui::ViewProviderDocumentObjectGroup)


ViewProviderFemAnalysis::ViewProviderFemAnalysis()
{
    sPixmap = "FEM_Analysis";
}

ViewProviderFemAnalysis::~ViewProviderFemAnalysis() = default;

void ViewProviderFemAnalysis::attach(App::DocumentObject* obj)
{
    Gui::ViewProviderDocumentObjectGroup::attach(obj);
    extension.attach(this);
    // activate analysis if currently active workbench is FEM
    auto* workbench = Gui::WorkbenchManager::instance()->active();
    if (workbench->name() == "FemWorkbench") {
        doubleClicked();
    }
}

void ViewProviderFemAnalysis::highlightView(Gui::ViewProviderDocumentObject* view)
{
    extension.highlightView(view);
}

bool ViewProviderFemAnalysis::doubleClicked()
{
    Gui::Command::assureWorkbench("FemWorkbench");
    Gui::Command::addModule(Gui::Command::Gui, "FemGui");
    Gui::Command::doCommand(Gui::Command::Gui,
                            "FemGui.setActiveAnalysis(App.activeDocument().%s)",
                            this->getObject()->getNameInDocument());
    // After activation of the analysis the allowed FEM toolbar buttons should become active.
    // To achieve this we must clear the object selection to trigger the selection observer.
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.Selection.clearSelection()");
    // indicate the activated analysis by selecting it
    // especially useful for files with 2 or more analyses but also
    // necessary for the workflow with new files to add a solver as next object
    std::vector<App::DocumentObject*> selVector {};
    selVector.push_back(this->getObject());
    auto* docName = this->getObject()->getDocument()->getName();
    Gui::Selection().setSelection(docName, selVector);
    return true;
}

std::vector<App::DocumentObject*> ViewProviderFemAnalysis::claimChildren() const
{
    return Gui::ViewProviderDocumentObjectGroup::claimChildren();
}

std::vector<std::string> ViewProviderFemAnalysis::getDisplayModes() const
{
    return {"Analysis"};
}

void ViewProviderFemAnalysis::hide()
{
    Gui::ViewProviderDocumentObjectGroup::hide();
}

void ViewProviderFemAnalysis::show()
{
    Gui::ViewProviderDocumentObjectGroup::show();
}

void ViewProviderFemAnalysis::setupContextMenu(QMenu* menu, QObject*, const char*)
{
    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    QAction* act = menu->addAction(tr("Activate analysis"));
    func->trigger(act, [this]() {
        this->doubleClicked();
    });
}

bool ViewProviderFemAnalysis::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // When double-clicking on the item for this pad the object
        // unsets and sets its edit mode without closing the task panel

        // Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        // TaskDlgAnalysis *anaDlg = qobject_cast<TaskDlgAnalysis *>(dlg);
        // if (padDlg && anaDlg->getPadView() != this)
        //     padDlg = 0; // another pad left open its task panel
        // if (dlg && !padDlg) {
        //     QMessageBox msgBox;
        //     msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
        //     msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
        //     msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        //     msgBox.setDefaultButton(QMessageBox::Yes);
        //     int ret = msgBox.exec();
        //     if (ret == QMessageBox::Yes)
        //         Gui::Control().closeDialog();
        //     else
        //         return false;
        // }

        // start the edit dialog
        //        if (padDlg)
        //            Gui::Control().showDialog(padDlg);
        //        else

        // Fem::FemAnalysis* pcAna = static_cast<Fem::FemAnalysis*>(this->getObject());
        // Gui::Control().showDialog(new TaskDlgAnalysis(pcAna));
        // return true;
        return false;
    }
    else {
        return Gui::ViewProviderDocumentObjectGroup::setEdit(ModNum);
    }
}

void ViewProviderFemAnalysis::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        Gui::ViewProviderDocumentObjectGroup::unsetEdit(ModNum);
    }
}

bool ViewProviderFemAnalysis::canDragObjects() const
{
    return true;
}

bool ViewProviderFemAnalysis::canDragObject(App::DocumentObject* obj) const
{
    if (!obj) {
        return false;
    }
    if (obj->getTypeId().isDerivedFrom(Fem::FemMeshObject::getClassTypeId())) {
        return true;
    }
    else if (obj->getTypeId().isDerivedFrom(Fem::FemSolverObject::getClassTypeId())) {
        return true;
    }
    else if (obj->getTypeId().isDerivedFrom(Fem::FemResultObject::getClassTypeId())) {
        return true;
    }
    else if (obj->getTypeId().isDerivedFrom(Fem::Constraint::getClassTypeId())) {
        return true;
    }
    else if (obj->getTypeId().isDerivedFrom(Fem::FemSetObject::getClassTypeId())) {
        return true;
    }
    else if (obj->getTypeId().isDerivedFrom(Base::Type::fromName("Fem::FeaturePython"))) {
        return true;
    }
    else if (obj->getTypeId().isDerivedFrom(App::MaterialObject::getClassTypeId())) {
        return true;
    }
    else if (obj->getTypeId().isDerivedFrom(App::TextDocument::getClassTypeId())) {
        return true;
    }
#ifdef FC_USE_VTK
    else if (obj->getTypeId().isDerivedFrom(Fem::FemPostObject::getClassTypeId())) {
        return true;
    }
#endif
    else {
        return false;
    }
}

void ViewProviderFemAnalysis::dragObject(App::DocumentObject* obj)
{
    ViewProviderDocumentObjectGroup::dragObject(obj);
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
    ViewProviderDocumentObjectGroup::dropObject(obj);
}

bool ViewProviderFemAnalysis::onDelete(const std::vector<std::string>&)
{
    // warn the user if the object has unselected children
    auto objs = claimChildren();
    return checkSelectedChildren(objs, this->getDocument(), "analysis");
}

bool ViewProviderFemAnalysis::checkSelectedChildren(const std::vector<App::DocumentObject*> objs,
                                                    Gui::Document* docGui,
                                                    std::string objectName)
{
    // warn the user if the object has unselected children
    if (!objs.empty()) {
        // check if all children are in the selection
        bool found = false;
        auto selectionList = Gui::Selection().getSelectionEx(docGui->getDocument()->getName());
        for (auto child : objs) {
            found = false;
            for (Gui::SelectionObject selection : selectionList) {
                if (std::string(child->getNameInDocument())
                    == std::string(selection.getFeatName())) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                break;
            }
        }
        if (found) {  // all children are selected too
            return true;
        }

        // generate dialog
        QString bodyMessage;
        QTextStream bodyMessageStream(&bodyMessage);
        bodyMessageStream << qApp->translate("Std_Delete",
                                             ("The " + objectName
                                              + " is not empty, therefore the\nfollowing "
                                                "referencing objects might be lost:")
                                                 .c_str());
        bodyMessageStream << '\n';
        for (auto ObjIterator : objs) {
            bodyMessageStream << '\n' << QString::fromUtf8(ObjIterator->Label.getValue());
        }
        bodyMessageStream << "\n\n" << QObject::tr("Are you sure you want to continue?");
        // show and evaluate the dialog
        int DialogResult =
            QMessageBox::warning(Gui::getMainWindow(),
                                 qApp->translate("Std_Delete", "Object dependencies"),
                                 bodyMessage,
                                 QMessageBox::Yes,
                                 QMessageBox::No);
        if (DialogResult == QMessageBox::Yes) {
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return true;
    }
}

bool ViewProviderFemAnalysis::canDelete(App::DocumentObject* obj) const
{
    // deletions of objects from a FemAnalysis don't necessarily destroy anything
    // thus we can pass this action
    // we can warn the user if necessary in the object's ViewProvider in the onDelete() function
    Q_UNUSED(obj)
    return true;
}

// Python feature -----------------------------------------------------------------------

namespace Gui
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(FemGui::ViewProviderFemAnalysisPython, FemGui::ViewProviderFemAnalysis)
/// @endcond

// explicit template instantiation
template class FemGuiExport ViewProviderPythonFeatureT<ViewProviderFemAnalysis>;
}  // namespace Gui
