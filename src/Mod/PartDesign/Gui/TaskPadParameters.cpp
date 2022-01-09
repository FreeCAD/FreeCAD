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

#include "ui_TaskPadPocketParameters.h"
#include "TaskPadParameters.h"
#include <Gui/Command.h>
#include <Gui/ViewProvider.h>
#include <Mod/PartDesign/App/FeaturePad.h>
#include <Mod/Sketcher/App/SketchObject.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPadParameters */

TaskPadParameters::TaskPadParameters(ViewProviderPad *PadView, QWidget *parent, bool newObj)
    : TaskExtrudeParameters(PadView, parent, "PartDesign_Pad", tr("Pad parameters"))
{
    ui->offsetEdit->setToolTip(tr("Offset from face at which pad will end"));
    ui->checkBoxReversed->setToolTip(tr("Reverses pad direction"));

    // set the history path
    ui->lengthEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadLength"));
    ui->lengthEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadLength2"));
    ui->offsetEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadOffset"));

    setupDialog();

    // if it is a newly created object use the last value of the history
    if (newObj) {
        readValuesFromHistory();
    }
}

TaskPadParameters::~TaskPadParameters()
{
}

void TaskPadParameters::translateModeList(int index)
{
    ui->changeMode->clear();
    ui->changeMode->addItem(tr("Dimension"));
    ui->changeMode->addItem(tr("To last"));
    ui->changeMode->addItem(tr("To first"));
    ui->changeMode->addItem(tr("Up to face"));
    ui->changeMode->addItem(tr("Two dimensions"));
    ui->changeMode->setCurrentIndex(index);
}

void TaskPadParameters::updateUI(int index)
{
    // update direction combobox
    fillDirectionCombo();
    
    // disable/hide everything unless we are sure we don't need it
    // exception: the direction parameters are in any case visible
    bool isLengthEditVisible  = false;
    bool isLengthEdit2Visible = false;
    bool isOffsetEditVisible  = false;
    bool isMidplaneEnabled    = false;
    bool isMidplaneVisible    = false;
    bool isReversedEnabled    = false;
    bool isFaceEditEnabled    = false;

    Modes mode = static_cast<Modes>(index);

    if (mode == Modes::Dimension) {
        isLengthEditVisible = true;
        ui->lengthEdit->selectNumber();
        QMetaObject::invokeMethod(ui->lengthEdit, "setFocus", Qt::QueuedConnection);
        isMidplaneEnabled = !ui->checkBoxReversed->isChecked();
        isMidplaneVisible = true;
        // Reverse only makes sense if Midplane is not true
        isReversedEnabled = !ui->checkBoxMidplane->isChecked();
    }
    else if (mode == Modes::ToLast || mode == Modes::ToFirst) {
        isOffsetEditVisible = true;
        isReversedEnabled = true;
    }
    else if (mode == Modes::ToFace) {
        isOffsetEditVisible = true;
        isFaceEditEnabled   = true;
        QMetaObject::invokeMethod(ui->lineFaceName, "setFocus", Qt::QueuedConnection);
        // Go into reference selection mode if no face has been selected yet
        if (ui->lineFaceName->property("FeatureName").isNull())
            onButtonFace(true);
        isReversedEnabled = true;
    }
    else if (mode == Modes::TwoDimensions) {
        isLengthEditVisible  = true;
        isLengthEdit2Visible = true;
        isReversedEnabled    = true;
    }

    ui->lengthEdit->setVisible( isLengthEditVisible );
    ui->lengthEdit->setEnabled( isLengthEditVisible );
    ui->labelLength->setVisible( isLengthEditVisible );
    ui->checkBoxAlongDirection->setVisible( isLengthEditVisible );

    ui->offsetEdit->setVisible( isOffsetEditVisible );
    ui->offsetEdit->setEnabled( isOffsetEditVisible );
    ui->labelOffset->setVisible( isOffsetEditVisible );

    ui->checkBoxMidplane->setEnabled( isMidplaneEnabled );
    ui->checkBoxMidplane->setVisible( isMidplaneVisible );

    ui->checkBoxReversed->setEnabled( isReversedEnabled );

    ui->lengthEdit2->setVisible( isLengthEdit2Visible );
    ui->lengthEdit2->setEnabled( isLengthEdit2Visible );
    ui->labelLength2->setVisible( isLengthEdit2Visible );

    ui->buttonFace->setEnabled( isFaceEditEnabled );
    ui->lineFaceName->setEnabled( isFaceEditEnabled );
    if (!isFaceEditEnabled) {
        onButtonFace(false);
    }
}

void TaskPadParameters::onModeChanged(int index)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());

    switch (static_cast<Modes>(index)) {
    case Modes::Dimension:
        pcPad->Type.setValue("Length");
        // Avoid error message
        if (ui->lengthEdit->value() < Base::Quantity(Precision::Confusion(), Base::Unit::Length))
            ui->lengthEdit->setValue(5.0);
        break;
    case Modes::ToLast:
        pcPad->Type.setValue("UpToLast");
        break;
    case Modes::ToFirst:
        pcPad->Type.setValue("UpToFirst");
        break;
    case Modes::ToFace:
        pcPad->Type.setValue("UpToFace");
        break;
    case Modes::TwoDimensions:
        pcPad->Type.setValue("TwoLengths");
        break;
    }

    updateUI(index);
    recomputeFeature();
}

void TaskPadParameters::apply()
{
    QString facename = QString::fromLatin1("None");
    if (static_cast<Modes>(getMode()) == Modes::ToFace) {
        facename = getFaceName();
    }
    applyParameters(facename);
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPadParameters::TaskDlgPadParameters(ViewProviderPad *PadView, bool /*newObj*/)
    : TaskDlgSketchBasedParameters(PadView)
{
    assert(vp);
    Content.push_back ( new TaskPadParameters(PadView ) );
}

//==== calls from the TaskView ===============================================================

#include "moc_TaskPadParameters.cpp"
