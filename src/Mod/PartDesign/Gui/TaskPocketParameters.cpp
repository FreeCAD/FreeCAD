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
    : TaskExtrudeParameters(PocketView, parent, "PartDesign_Pocket", tr("Pocket parameters"))
{
    ui->offsetEdit->setToolTip(tr("Offset from the selected face at which the pocket will end on side 1"));

    ui->offsetEdit2->setToolTip(tr("Offset from the selected face at which the pocket will end on side 2"));

    ui->checkBoxReversed->setToolTip(tr("Reverses pocket direction"));

    // set the history path
    ui->lengthEdit->setEntryName(QByteArray("Length"));
    ui->lengthEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketLength"));
    ui->lengthEdit2->setEntryName(QByteArray("Length2"));
    ui->lengthEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketLength2"));
    ui->offsetEdit->setEntryName(QByteArray("Offset"));
    ui->offsetEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketOffset"));
    ui->offsetEdit2->setEntryName(QByteArray("Offset2"));
    ui->offsetEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketOffset2"));
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

void TaskPocketParameters::translateModeList(QComboBox* box, int index)
{
    box->clear();
    box->addItem(tr("Dimension"));
    box->addItem(tr("Through all"));
    box->addItem(tr("To first"));
    box->addItem(tr("Up to face"));
    box->addItem(tr("Up to shape"));
    box->setCurrentIndex(index);
}

void TaskPocketParameters::updateUI(Sides side)
{
    // update direction combobox
    fillDirectionCombo();
    // set and enable checkboxes
    setCheckboxes(Type::Pocket, side);
}

void TaskPocketParameters::onModeChanged(int index)
{
    auto pcPocket = getObject<PartDesign::Pocket>();

    switch (static_cast<Mode>(index)) {
        case Mode::Dimension: {
            pcPocket->Type.setValue("Length");

            // Avoid error message
            double L = ui->lengthEdit->value().getValue();
            double L2 = static_cast<SidesMode>(getSidesMode()) == SidesMode::TwoSides
                ? ui->lengthEdit->value().getValue()
                : 0;
            if (std::abs(L + L2) < Precision::Confusion()) {
                ui->lengthEdit->setValue(5.0);
            }
            break;
        }
        case Mode::ThroughAll:
            pcPocket->Type.setValue("ThroughAll");
            break;
        case Mode::ToFirst:
            pcPocket->Type.setValue("UpToFirst");
            break;
        case Mode::ToFace:
            pcPocket->Type.setValue("UpToFace");
            if (ui->lineFaceName->text().isEmpty()) {
                ui->buttonFace->setChecked(true);
                handleLineFaceNameClick(ui->lineFaceName);  // sets placeholder text
            }
            break;
        case Mode::ToShape:
            pcPocket->Type.setValue("UpToShape");
            break;
    }

    updateUI(Sides::First);
    recomputeFeature();
}

void TaskPocketParameters::onMode2Changed(int index)
{
    auto pcPocket = getObject<PartDesign::Pocket>();

    switch (static_cast<Mode>(index)) {
        case Mode::Dimension:
            pcPocket->Type2.setValue("Length");
            break;
        case Mode::ThroughAll:
            pcPocket->Type2.setValue("ThroughAll");
            break;
        case Mode::ToFirst:
            pcPocket->Type2.setValue("UpToFirst");
            break;
        case Mode::ToFace:
            // Note: ui->checkBoxReversed is purposely enabled because the selected face
            // could be a circular one around the sketch
            // Also note: Because of the code at the beginning of Pocket::execute() which is used
            // to detect broken legacy parts, we must set the length to zero here!
            pcPocket->Type2.setValue("UpToFace");
            if (ui->lineFaceName2->text().isEmpty()) {
                ui->buttonFace2->setChecked(true);
                handleLineFaceNameClick(ui->lineFaceName2);  // sets placeholder text
            }
            break;
        case Mode::ToShape:
            pcPocket->Type2.setValue("UpToShape");
            break;
    }

    updateUI(Sides::Second);
    recomputeFeature();
}

void TaskPocketParameters::apply()
{
    applyParameters();
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
