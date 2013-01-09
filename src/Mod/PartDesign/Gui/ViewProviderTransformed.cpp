/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include "ViewProviderTransformed.h"
#include "TaskTransformedParameters.h"
#include <Mod/PartDesign/App/FeatureTransformed.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderTransformed,PartDesignGui::ViewProvider)

void ViewProviderTransformed::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr((std::string("Edit ") + featureName + " feature").c_str()), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member);
}

void ViewProviderTransformed::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        PartGui::ViewProviderPart::unsetEdit(ModNum);
    }
}

bool ViewProviderTransformed::onDelete(const std::vector<std::string> &)
{
    PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(getObject());
    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();

    // if abort command deleted the object the originals are visible again
    for (std::vector<App::DocumentObject*>::const_iterator it = originals.begin(); it != originals.end(); ++it)
    {
        if (((*it) != NULL) && Gui::Application::Instance->getViewProvider(*it))
            Gui::Application::Instance->getViewProvider(*it)->show();
    }

    return true;
}

const bool ViewProviderTransformed::checkDlgOpen(TaskDlgTransformedParameters* transformedDlg) {
    // When double-clicking on the item for this feature the
    // object unsets and sets its edit mode without closing
    // the task panel
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    transformedDlg = qobject_cast<TaskDlgTransformedParameters *>(dlg);

    if ((transformedDlg != NULL) && (transformedDlg->getTransformedView() != this))
        transformedDlg = NULL; // another transformed feature left open its task panel

    if ((dlg != NULL) && (transformedDlg == NULL)) {
        QMessageBox msgBox;
        msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
        msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if (ret == QMessageBox::Yes)
            Gui::Control().closeDialog();
        else
            return false;
    }

    // clear the selection (convenience)
    Gui::Selection().clearSelection();

    // Continue (usually in virtual method setEdit())
    return true;
}

void ViewProviderTransformed::recomputeFeature(void)
{
    PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(getObject());
    pcTransformed->getDocument()->recomputeFeature(pcTransformed);
    const std::vector<App::DocumentObjectExecReturn*> log = pcTransformed->getDocument()->getRecomputeLog();
    unsigned rejected = pcTransformed->getRejectedTransformations().size();
    QString msg = QString::fromAscii("%1");
    if (rejected > 0) {
        msg = QString::fromLatin1("<font color='orange'>%1<br/></font>\r\n%2");
        if (rejected == 1)
            msg = msg.arg(QObject::tr("One transformed shape does not intersect support"));
        else {
            msg = msg.arg(QObject::tr("%1 transformed shapes do not intersect support"));
            msg = msg.arg(rejected);
        }
    }
    if (log.size() > 0) {
        msg = msg.arg(QString::fromLatin1("<font color='red'>%1<br/></font>"));
        msg = msg.arg(QString::fromStdString(log.back()->Why));
    } else {
        msg = msg.arg(QString::fromLatin1("<font color='green'>%1<br/></font>"));
        msg = msg.arg(QObject::tr("Transformation succeeded"));
    }
    signalDiagnosis(msg);
}

