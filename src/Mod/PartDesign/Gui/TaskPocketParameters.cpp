/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <Precision.hxx>
#endif

#include <Mod/PartDesign/App/FeaturePocket.h>

#include "ui_TaskPadPocketParameters.h"
#include "TaskPocketParameters.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPocketParameters */

TaskPocketParameters::TaskPocketParameters(ViewProviderPocket *PocketView,QWidget *parent, bool newObj)
    : TaskExtrudeParameters(PocketView, parent, "PartDesign_Pocket", tr("Pocket Parameters"))
    , oldLength(0)
{
    ui->offsetEdit->setToolTip(tr("Offset from face at which pocket will end"));
    ui->checkBoxReversed->setToolTip(tr("Reverses pocket direction"));

    // set the history path
    ui->lengthEdit->setEntryName(QByteArray("Length"));
    ui->lengthEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketLength"));
    ui->lengthEdit2->setEntryName(QByteArray("Length2"));
    ui->lengthEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketLength2"));
    ui->offsetEdit->setEntryName(QByteArray("Offset"));
    ui->offsetEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketOffset"));
    ui->taperEdit->setEntryName(QByteArray("TaperAngle"));
    ui->taperEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketTaperAngle"));
    ui->taperEdit2->setEntryName(QByteArray("TaperAngle2"));
    ui->taperEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketTaperAngle2"));

    setupDialog();

    // if it is a newly created object use the last value of the history
    if (newObj) {
        readValuesFromHistory();
    }
}

TaskPocketParameters::~TaskPocketParameters() = default;

void TaskPocketParameters::translateModeList(int index)
{
    ui->changeMode->clear();
    ui->changeMode->addItem(tr("Dimension"));
    ui->changeMode->addItem(tr("Through all"));
    ui->changeMode->addItem(tr("To first"));
    ui->changeMode->addItem(tr("Up to face"));
    ui->changeMode->addItem(tr("Two dimensions"));
    ui->changeMode->addItem(tr("Up to shape"));
    ui->changeMode->setCurrentIndex(index);
}

void TaskPocketParameters::updateUI(int index)
{
    // update direction combobox
    fillDirectionCombo();
    // set and enable checkboxes
    setCheckboxes(static_cast<Mode>(index), Type::Pocket);
}

void TaskPocketParameters::onModeChanged(int index)
{
    auto pcPocket = getObject<PartDesign::Pocket>();

    switch (static_cast<Mode>(index)) {
        case Mode::Dimension:
            // Why? See below for "UpToFace"
            if (oldLength < Precision::Confusion())
                oldLength = 5.0;
            pcPocket->Length.setValue(oldLength);
            ui->lengthEdit->setValue(oldLength);
            pcPocket->Type.setValue("Length");
            break;
        case Mode::ThroughAll:
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("ThroughAll");
            break;
        case Mode::ToFirst:
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("UpToFirst");
            break;
        case Mode::ToFace:
            // Note: ui->checkBoxReversed is purposely enabled because the selected face
            // could be a circular one around the sketch
            // Also note: Because of the code at the beginning of Pocket::execute() which is used
            // to detect broken legacy parts, we must set the length to zero here!
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("UpToFace");
            pcPocket->Length.setValue(0.0);
            ui->lengthEdit->setValue(0.0);
            if (ui->lineFaceName->text().isEmpty()) {
                ui->buttonFace->setChecked(true);
                handleLineFaceNameClick(); // sets placeholder text
            }
            break;
        case Mode::TwoDimensions:
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("TwoLengths");
            break;
        case Mode::ToShape:
            pcPocket->Type.setValue("UpToShape");
            break;
    }

    updateUI(index);
    recomputeFeature();
}

void TaskPocketParameters::apply()
{
    QString facename = QStringLiteral("None");
    if (static_cast<Mode>(getMode()) == Mode::ToFace) {
        facename = getFaceName();
    }
    applyParameters(facename);
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPocketParameters::TaskDlgPocketParameters(ViewProviderPocket *PocketView)
    : TaskDlgExtrudeParameters(PocketView), parameters(new TaskPocketParameters(PocketView))
{
    Content.push_back(parameters);
}

#include "moc_TaskPocketParameters.cpp"
