/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender <jrheinlaender@users.sourceforge.net>*
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
# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
# include <Standard_Failure.hxx>
# include <boost/bind.hpp>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <ui_DlgReference.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <App/Part.h>
#include <App/ObjectIdentifier.h>
#include <App/PropertyExpressionEngine.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/MainWindow.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/Part/Gui/AttacherTexts.h>

#include "ReferenceSelection.h"
#include "Utils.h"

#include "TaskDatumParameters.h"
#include "TaskFeaturePick.h"

using namespace PartDesignGui;
using namespace Gui;
using namespace Attacher;

/* TRANSLATOR PartDesignGui::TaskDatumParameters */

TaskDatumParameters::TaskDatumParameters(ViewProviderDatum *ViewProvider,QWidget *parent)
    : PartGui::TaskAttacher(ViewProvider, parent, QString::fromLatin1("PartDesign_") + ViewProvider->datumType,
              ViewProvider->datumText + tr(" parameters"))
{
    Gui::Selection().addSelectionGate(new NoDependentsSelection(ViewProvider->getObject()));
    ViewProvider->setPickable(false);
}

TaskDatumParameters::~TaskDatumParameters()
{
    if(this->ViewProvider && this->ViewProvider->isDerivedFrom(ViewProviderDatum::getClassTypeId()))
        static_cast<ViewProviderDatum*>(this->ViewProvider)->setPickable(true);
    Gui::Selection().rmvSelectionGate();
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgDatumParameters::TaskDlgDatumParameters(ViewProviderDatum *ViewProvider)
    : TaskDlgAttacher(ViewProvider, false)
{
    assert(ViewProvider);
    parameter  = new TaskDatumParameters(ViewProvider);
    Content.push_back(parameter);
}

TaskDlgDatumParameters::~TaskDlgDatumParameters()
{

}

bool TaskDlgDatumParameters::reject() {
    
    return PartGui::TaskDlgAttacher::reject();
}


bool TaskDlgDatumParameters::accept() {

    Part::Datum* pcDatum = static_cast<Part::Datum*>(ViewProvider->getObject());
    auto pcActiveBody = PartDesignGui::getBodyFor(pcDatum, false);
    auto pcActivePart = PartDesignGui::getPartFor(pcActiveBody, false);
    std::vector<App::DocumentObject*> copies;

    //see if we are able to assign a mode
    if (parameter->getActiveMapMode() == mmDeactivated) {
        QMessageBox msg;
        msg.setWindowTitle(tr("Incompatible reference set"));
        msg.setText(tr("There is no attachment mode that fits the current set"
        " of references. If you choose to continue, the feature will remain where"
        " it is now, and will not be moved as the references change."
        " Continue?"));
        msg.addButton(QMessageBox::Yes);
        auto btNo =  msg.addButton(QMessageBox::No);
        msg.setDefaultButton(btNo);
        msg.setIcon(QMessageBox::Warning);
        msg.exec();
        if (msg.buttonRole(msg.clickedButton()) == QMessageBox::NoRole)
            return false;
    }

    //see what to do with external references
    //check the prerequisites for the selected objects
    //the user has to decide which option we should take if external references are used
    bool extReference = false;
    for (App::DocumentObject* obj : pcDatum->Support.getValues()) {
        if (!pcActiveBody->hasObject(obj) && !pcActiveBody->getOrigin()->hasObject(obj))
            extReference = true;
    }

    if(extReference) {
        // TODO: rewrite this to be shared with CmdPartDesignNewSketch::activated() (2015-10-20, Fat-Zer)
        QDialog dia(Gui::getMainWindow());
        PartDesignGui::Ui_DlgReference dlg;
        dlg.setupUi(&dia);
        dia.setModal(true);
        int result = dia.exec();
        if (result == QDialog::DialogCode::Rejected)
            return false;
        else if (!dlg.radioXRef->isChecked()) {
            std::vector<App::DocumentObject*> copyObjects;
            std::vector<std::string> copySubValues;
            std::vector<std::string> subs = pcDatum->Support.getSubValues();
            int index = 0;
            for (App::DocumentObject* obj : pcDatum->Support.getValues()) {
                if (!pcActiveBody->hasObject(obj) && !pcActiveBody->getOrigin()->hasObject(obj)) {
                    auto* copy = PartDesignGui::TaskFeaturePick::makeCopy(obj, subs[index], dlg.radioIndependent->isChecked());
                    if (copy) {
                        copyObjects.push_back(copy);
                        copies.push_back(copyObjects.back());
                        copySubValues.push_back(std::string());
                    }
                }
                else {
                    copyObjects.push_back(obj);
                    copySubValues.push_back(subs[index]);
                }

                index++;
            }

            pcDatum->Support.setValues(copyObjects, copySubValues);
        }
    }

    if (!PartGui::TaskDlgAttacher::accept())
        return false;

    //we need to add the copied features to the body after the command action, as otherwise FreeCAD crashes unexplainably
    for(auto obj : copies) {
        if (pcActiveBody)
            pcActiveBody->addObject(obj);
        else if (pcActivePart)
            pcActivePart->addObject(obj);
    }

    return true;
}

#include "moc_TaskDatumParameters.cpp"
