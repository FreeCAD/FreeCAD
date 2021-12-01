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
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <Precision.hxx>
#endif

#include "ui_TaskPadParameters.h"
#include "TaskPocketParameters.h"
#include <Gui/Command.h>
#include <Gui/ViewProvider.h>
#include <Mod/PartDesign/App/FeaturePocket.h>
#include <Mod/Sketcher/App/SketchObject.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPocketParameters */

TaskPocketParameters::TaskPocketParameters(ViewProviderPocket *PocketView,QWidget *parent, bool newObj)
    : TaskExtrudeParameters(PocketView, parent, "PartDesign_Pocket", tr("Pocket parameters"))
    , oldLength(0)
{
    ui->offsetEdit->setToolTip(tr("Offset from face at which pocket will end"));
    ui->checkBoxReversed->setToolTip(tr("Reverses pocket direction"));

    // set the history path
    ui->lengthEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketLength"));
    ui->lengthEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketLength2"));
    ui->offsetEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketOffset"));

    setupDialog();

    // if it is a newly created object use the last value of the history
    if (newObj) {
        readValuesFromHistory();
    }
}

TaskPocketParameters::~TaskPocketParameters()
{
}

void TaskPocketParameters::translateModeList(int index)
{
    ui->changeMode->clear();
    ui->changeMode->addItem(tr("Dimension"));
    ui->changeMode->addItem(tr("Through all"));
    ui->changeMode->addItem(tr("To first"));
    ui->changeMode->addItem(tr("Up to face"));
    ui->changeMode->addItem(tr("Two dimensions"));
    ui->changeMode->setCurrentIndex(index);
}

void TaskPocketParameters::updateUI(int index)
{
    // update direction combobox
    fillDirectionCombo();

    // disable/hide everything unless we are sure we don't need it
    // exception: the direction parameters are in any case visible
    bool isLengthEditVisible  = false;
    bool isLengthEdit2Visible = false;
    bool isOffsetEditVisible  = false;
    bool isOffsetEditEnabled  = true;
    bool isMidplateEnabled    = false;
    bool isReversedEnabled    = false;
    bool isFaceEditEnabled    = false;

    Modes mode = static_cast<Modes>(index);

    if (mode == Modes::Dimension) {
        isLengthEditVisible = true;
        ui->lengthEdit->selectNumber();
        // Make sure that the spin box has the focus to get key events
        // Calling setFocus() directly doesn't work because the spin box is not
        // yet visible.
        QMetaObject::invokeMethod(ui->lengthEdit, "setFocus", Qt::QueuedConnection);
        isMidplateEnabled = true;
        // Reverse only makes sense if Midplane is not true
        isReversedEnabled = !ui->checkBoxMidplane->isChecked();
    }
    else if (mode == Modes::ThroughAll) {
        isOffsetEditVisible = true;
        isOffsetEditEnabled = false; // offset may have some meaning for through all but it doesn't work
        isMidplateEnabled = true;
        isReversedEnabled = !ui->checkBoxMidplane->isChecked();
    }
    else if (mode == Modes::ToFirst) {
        isOffsetEditVisible = true;
        isReversedEnabled = true;       // Will change the direction it seeks for its first face?
            // It may work not quite as expected but useful if sketch oriented upside-down.
            // (may happen in bodies)
            // FIXME: Fix probably lies somewhere in IF block on line 125 of FeaturePocket.cpp
    }
    else if (mode == Modes::ToFace) {
        isOffsetEditVisible = true;
        isReversedEnabled = true;
        isFaceEditEnabled    = true;
        QMetaObject::invokeMethod(ui->lineFaceName, "setFocus", Qt::QueuedConnection);
        // Go into reference selection mode if no face has been selected yet
        if (ui->lineFaceName->property("FeatureName").isNull())
            onButtonFace(true);
    }
    else if (mode == Modes::TwoDimensions) {
        isLengthEditVisible = true;
        isLengthEdit2Visible = true;
        isReversedEnabled = true;
    }

    ui->lengthEdit->setVisible( isLengthEditVisible );
    ui->lengthEdit->setEnabled( isLengthEditVisible );
    ui->labelLength->setVisible( isLengthEditVisible );
    ui->checkBoxAlongDirection->setVisible(isLengthEditVisible);

    ui->lengthEdit2->setVisible( isLengthEdit2Visible );
    ui->lengthEdit2->setEnabled( isLengthEdit2Visible );
    ui->labelLength2->setVisible( isLengthEdit2Visible );

    ui->offsetEdit->setVisible( isOffsetEditVisible );
    ui->offsetEdit->setEnabled( isOffsetEditVisible && isOffsetEditEnabled );
    ui->labelOffset->setVisible( isOffsetEditVisible );

    ui->checkBoxMidplane->setEnabled( isMidplateEnabled );

    ui->checkBoxReversed->setEnabled( isReversedEnabled );

    ui->buttonFace->setEnabled( isFaceEditEnabled );
    ui->lineFaceName->setEnabled( isFaceEditEnabled );
    if (!isFaceEditEnabled) {
        onButtonFace(false);
    }
}

void TaskPocketParameters::onModeChanged(int index)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());

    switch (static_cast<Modes>(index)) {
        case Modes::Dimension:
            // Why? See below for "UpToFace"
            if (oldLength < Precision::Confusion())
                oldLength = 5.0;
            pcPocket->Length.setValue(oldLength);
            ui->lengthEdit->setValue(oldLength);
            pcPocket->Type.setValue("Length");
            break;
        case Modes::ThroughAll:
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("ThroughAll");
            break;
        case Modes::ToFirst:
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("UpToFirst");
            break;
        case Modes::ToFace:
            // Because of the code at the beginning of Pocket::execute() which is used to detect
            // broken legacy parts, we must set the length to zero here!
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("UpToFace");
            pcPocket->Length.setValue(0.0);
            ui->lengthEdit->setValue(0.0);
            break;
        case Modes::TwoDimensions:
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("TwoLengths");
            break;
    }

    updateUI(index);
    recomputeFeature();
}

void TaskPocketParameters::apply()
{
    auto obj = vp->getObject();

    ui->lengthEdit->apply();
    ui->lengthEdit2->apply();

    FCMD_OBJ_CMD(obj,"Type = " << getMode());
    QString facename = QString::fromLatin1("None");
    if (static_cast<Modes>(getMode()) == Modes::ToFace) {
        facename = getFaceName();
    }
    FCMD_OBJ_CMD(obj,"UpToFace = " << facename.toLatin1().data());
    FCMD_OBJ_CMD(obj,"Reversed = " << (getReversed()?1:0));
    FCMD_OBJ_CMD(obj,"Midplane = " << (getMidplane()?1:0));
    FCMD_OBJ_CMD(obj,"Offset = " << getOffset());
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPocketParameters::TaskDlgPocketParameters(ViewProviderPocket *PocketView)
    : TaskDlgSketchBasedParameters(PocketView)
{
    assert(vp);
    Content.push_back ( new TaskPocketParameters(PocketView ) );
}

#include "moc_TaskPocketParameters.cpp"
