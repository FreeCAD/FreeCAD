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

#include <Mod/PartDesign/App/FeaturePad.h>

#include "ui_TaskPadPocketParameters.h"
#include "TaskPadParameters.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPadParameters */

TaskPadParameters::TaskPadParameters(ViewProviderPad *PadView, QWidget *parent, bool newObj)
    : TaskExtrudeParameters(PadView, parent, "PartDesign_Pad", tr("Pad Parameters"))
{
    ui->offsetEdit->setToolTip(tr("Offsets the pad from the face at which the pad will end"));
    ui->checkBoxReversed->setToolTip(tr("Reverses pad direction"));

    // set the history path
    ui->lengthEdit->setEntryName(QByteArray("Length"));
    ui->lengthEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadLength"));
    ui->lengthEdit2->setEntryName(QByteArray("Length2"));
    ui->lengthEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadLength2"));
    ui->offsetEdit->setEntryName(QByteArray("Offset"));
    ui->offsetEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadOffset"));
    ui->taperEdit->setEntryName(QByteArray("TaperAngle"));
    ui->taperEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadTaperAngle"));
    ui->taperEdit2->setEntryName(QByteArray("TaperAngle2"));
    ui->taperEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadTaperAngle2"));

    setupDialog();

    // if it is a newly created object use the last value of the history
    if (newObj) {
        readValuesFromHistory();
    }
}

TaskPadParameters::~TaskPadParameters() = default;

void TaskPadParameters::translateModeList(int index)
{
    ui->changeMode->clear();
    ui->changeMode->addItem(tr("Dimension"));
    ui->changeMode->addItem(tr("To last"));
    ui->changeMode->addItem(tr("To first"));
    ui->changeMode->addItem(tr("Up to face"));
    ui->changeMode->addItem(tr("Two dimensions"));
    ui->changeMode->addItem(tr("Up to shape"));
    ui->changeMode->setCurrentIndex(index);
}

void TaskPadParameters::updateUI(int index)
{
    // update direction combobox
    fillDirectionCombo();
    // set and enable checkboxes
    setCheckboxes(static_cast<Mode>(index), Type::Pad);
}

void TaskPadParameters::onModeChanged(int index)
{
   auto pcPad = getObject<PartDesign::Pad>();

    switch (static_cast<Mode>(index)) {
    case Mode::Dimension:
        pcPad->Type.setValue("Length");
        // Avoid error message
        if (ui->lengthEdit->value() < Base::Quantity(Precision::Confusion(), Base::Unit::Length))
            ui->lengthEdit->setValue(5.0);
        break;
    case Mode::ToLast:
        pcPad->Type.setValue("UpToLast");
        break;
    case Mode::ToFirst:
        pcPad->Type.setValue("UpToFirst");
        break;
    case Mode::ToFace:
        // Note: ui->checkBoxReversed is purposely enabled because the selected face
        // could be a circular one around the sketch
        pcPad->Type.setValue("UpToFace");
        if (ui->lineFaceName->text().isEmpty()) {
            ui->buttonFace->setChecked(true);
            handleLineFaceNameClick(); // sets placeholder text
        }
        break;
    case Mode::TwoDimensions:
        pcPad->Type.setValue("TwoLengths");
        break;
    case Mode::ToShape:
        pcPad->Type.setValue("UpToShape");
        break;
    }

    updateUI(index);
    recomputeFeature();
}

void TaskPadParameters::apply()
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

TaskDlgPadParameters::TaskDlgPadParameters(ViewProviderPad *PadView, bool /*newObj*/)
    : TaskDlgExtrudeParameters(PadView), parameters(new TaskPadParameters(PadView))
{
    Content.push_back(parameters);
}

//==== calls from the TaskView ===============================================================

#include "moc_TaskPadParameters.cpp"
