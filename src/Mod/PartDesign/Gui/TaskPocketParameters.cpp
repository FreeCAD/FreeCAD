// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include <Precision.hxx>


#include <Mod/PartDesign/App/FeaturePocket.h>

#include "ui_TaskPadPocketParameters.h"
#include "TaskPocketParameters.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPocketParameters */

TaskPocketParameters::TaskPocketParameters(ViewProviderPocket* PocketView, QWidget* parent, bool newObj)
    : TaskExtrudeParameters(PocketView, parent, "PartDesign_Pocket", tr("Pocket Parameters"))
{
    ui->offsetEdit->setToolTip(
        tr("Offset from the selected face at which the pocket will end on side 1")
    );
    ui->offsetEdit2->setToolTip(
        tr("Offset from the selected face at which the pocket will end on side 2")
    );
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

void TaskPocketParameters::updateUI(Side side)
{
    // update direction combobox
    fillDirectionCombo();
    // set and enable checkboxes
    updateWholeUI(Type::Pocket, side);
}

void TaskPocketParameters::onModeChanged(int index, Side side)
{
    auto& sideCtrl = getSideController(side);

    switch (static_cast<Mode>(index)) {
        case Mode::Dimension:
            sideCtrl.Type->setValue("Length");
            if (side == Side::First) {
                // Avoid error message
                double L = sideCtrl.lengthEdit->value().getValue();
                Side otherSide = side == Side::First ? Side::Second : Side::First;
                auto& sideCtrl2 = getSideController(otherSide);
                double L2 = static_cast<SidesMode>(getSidesMode()) == SidesMode::TwoSides
                    ? sideCtrl2.lengthEdit->value().getValue()
                    : 0;
                if (std::abs(L + L2) < Precision::Confusion()) {
                    sideCtrl.lengthEdit->setValue(5.0);
                }
            }
            break;
        case Mode::ThroughAll:
            sideCtrl.Type->setValue("ThroughAll");
            break;
        case Mode::ToFirst:
            sideCtrl.Type->setValue("UpToFirst");
            break;
        case Mode::ToFace:
            // Note: ui->checkBoxReversed is purposely enabled because the selected face
            // could be a circular one around the sketch
            // Also note: Because of the code at the beginning of Pocket::execute() which is used
            // to detect broken legacy parts, we must set the length to zero here!
            sideCtrl.Type->setValue("UpToFace");
            if (sideCtrl.lineFaceName->text().isEmpty()) {
                sideCtrl.buttonFace->setChecked(true);
                handleLineFaceNameClick(sideCtrl.lineFaceName);  // sets placeholder text
            }
            break;
        case Mode::ToShape:
            sideCtrl.Type->setValue("UpToShape");
            break;
    }

    updateUI(side);
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

TaskDlgPocketParameters::TaskDlgPocketParameters(ViewProviderPocket* PocketView)
    : TaskDlgExtrudeParameters(PocketView)
    , parameters(new TaskPocketParameters(PocketView))
{
    Content.push_back(parameters);
    Content.push_back(preview);
}

#include "moc_TaskPocketParameters.cpp"
