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
# include <TopExp.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
#endif

#include "ui_TaskPadParameters.h"
#include "TaskPadParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePad.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include "TaskSketchBasedParameters.h"
#include "ReferenceSelection.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPadParameters */

TaskPadParameters::TaskPadParameters(ViewProviderPad *PadView, QWidget *parent, bool newObj)
    : TaskSketchBasedParameters(PadView, parent, "PartDesign_Pad",tr("Pad parameters"))
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPadParameters();
    ui->setupUi(proxy);
#if QT_VERSION >= 0x040700
    ui->lineEdgeName->setPlaceholderText(tr("No edge selected"));
    ui->lineFaceName->setPlaceholderText(tr("No face selected"));
#endif

    this->groupLayout()->addWidget(proxy);

    // set the history path
    ui->lengthEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadLength"));
    ui->lengthEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadLength2"));
    ui->offsetEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadOffset"));

    // Get the feature data
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());
    Base::Quantity l = pcPad->Length.getQuantityValue();
    Base::Quantity l2 = pcPad->Length2.getQuantityValue();
    bool alongCustom = pcPad->AlongCustomVector.getValue();
    bool useCustom = pcPad->UseCustomVector.getValue();
    double xs = pcPad->Direction.getValue().x;
    double ys = pcPad->Direction.getValue().y;
    double zs = pcPad->Direction.getValue().z;
    Base::Quantity off = pcPad->Offset.getQuantityValue();
    bool midplane = pcPad->Midplane.getValue();
    int index = pcPad->Type.getValue(); // must extract value here, clear() kills it!
    App::DocumentObject* faceObj = pcPad->UpToFace.getValue();
    std::vector<std::string> subStringsFace = pcPad->UpToFace.getSubValues();
    std::string upToFace;
    int faceId = -1;
    if ((faceObj != NULL) && !subStringsFace.empty()) {
        upToFace = subStringsFace.front();
        if (upToFace.substr(0, 4) == "Face")
            faceId = std::atoi(&upToFace[4]);
    }
    App::DocumentObject* edgeObj = pcPad->Edge.getValue();
    std::vector<std::string> subStringsEdge = pcPad->Edge.getSubValues();
    std::string edgeName;
    int edgeId = -1;
    if ((edgeObj != NULL) && !subStringsEdge.empty()) {
        edgeName = subStringsEdge.front();
        if (edgeName.substr(0, 4) == "Edge")
            edgeId = std::atoi(&edgeName[4]);
    }

    // set decimals for the direction edits
    int UserDecimals = Base::UnitsApi::getDecimals();
    ui->XDirectionEdit->setDecimals(UserDecimals);
    ui->YDirectionEdit->setDecimals(UserDecimals);
    ui->ZDirectionEdit->setDecimals(UserDecimals);

    // Fill data into dialog elements
    ui->lengthEdit->setValue(l);
    ui->lengthEdit2->setValue(l2);
    ui->groupBoxDirection->setChecked(useCustom);
    ui->checkBoxAlongDirection->setChecked(alongCustom);
    // is only enabled if custom direction is used
    ui->checkBoxAlongDirection->setEnabled(useCustom);
    ui->XDirectionEdit->setValue(xs);
    ui->YDirectionEdit->setValue(ys);
    ui->ZDirectionEdit->setValue(zs);
    ui->offsetEdit->setValue(off);

    // Bind input fields to properties
    ui->lengthEdit->bind(pcPad->Length);
    ui->lengthEdit2->bind(pcPad->Length2);
    ui->XDirectionEdit->bind(App::ObjectIdentifier::parse(pcPad, std::string("Direction.x")));
    ui->YDirectionEdit->bind(App::ObjectIdentifier::parse(pcPad, std::string("Direction.y")));
    ui->ZDirectionEdit->bind(App::ObjectIdentifier::parse(pcPad, std::string("Direction.z")));
    ui->offsetEdit->bind(pcPad->Offset);

    ui->checkBoxMidplane->setChecked(midplane);

    // Set object labels
    if (edgeObj && PartDesign::Feature::isDatum(edgeObj)) {
        ui->lineEdgeName->setText(QString::fromUtf8(edgeObj->Label.getValue()));
        ui->lineEdgeName->setProperty("FeatureNameEdge", QByteArray(edgeObj->getNameInDocument()));
    }
    else if (edgeObj && edgeId >= 0) {
        ui->lineEdgeName->setText(QString::fromLatin1("%1:%2%3")
            .arg(QString::fromUtf8(edgeObj->Label.getValue()))
            .arg(tr("Edge"))
            .arg(edgeId));
        ui->lineEdgeName->setProperty("FeatureNameEdge", QByteArray(edgeObj->getNameInDocument()));
    }
    else {
        ui->lineEdgeName->clear();
        ui->lineEdgeName->setProperty("FeatureNameEdge", QVariant());
    }
    ui->lineEdgeName->setProperty("EdgeName", QByteArray(edgeName.c_str()));

    if (faceObj && PartDesign::Feature::isDatum(faceObj)) {
        ui->lineFaceName->setText(QString::fromUtf8(faceObj->Label.getValue()));
        ui->lineFaceName->setProperty("FeatureNameFace", QByteArray(faceObj->getNameInDocument()));
    }
    else if (faceObj && faceId >= 0) {
        ui->lineFaceName->setText(QString::fromLatin1("%1:%2%3")
                                  .arg(QString::fromUtf8(faceObj->Label.getValue()))
                                  .arg(tr("Face"))
                                  .arg(faceId));
        ui->lineFaceName->setProperty("FeatureNameFace", QByteArray(faceObj->getNameInDocument()));
    }
    else {
        ui->lineFaceName->clear();
        ui->lineFaceName->setProperty("FeatureNameFace", QVariant());
    }
    ui->lineFaceName->setProperty("FaceName", QByteArray(upToFace.c_str()));

    ui->changeMode->clear();
    ui->changeMode->insertItem(0, tr("Dimension"));
    ui->changeMode->insertItem(1, tr("To last"));
    ui->changeMode->insertItem(2, tr("To first"));
    ui->changeMode->insertItem(3, tr("Up to face"));
    ui->changeMode->insertItem(4, tr("Two dimensions"));
    ui->changeMode->setCurrentIndex(index);

    QMetaObject::connectSlotsByName(this);

    connect(ui->lengthEdit, SIGNAL(valueChanged(double)),
            this, SLOT(onLengthChanged(double)));
    connect(ui->lengthEdit2, SIGNAL(valueChanged(double)),
            this, SLOT(onLength2Changed(double)));
    connect(ui->checkBoxAlongDirection, SIGNAL(toggled(bool)),
        this, SLOT(onCBAlongDirectionChanged(bool)));
    connect(ui->groupBoxDirection, SIGNAL(toggled(bool)),
        this, SLOT(onGBDirectionChanged(bool)));
    connect(ui->XDirectionEdit, SIGNAL(valueChanged(double)),
        this, SLOT(onXDirectionEditChanged(double)));
    connect(ui->YDirectionEdit, SIGNAL(valueChanged(double)),
        this, SLOT(onYDirectionEditChanged(double)));
    connect(ui->ZDirectionEdit, SIGNAL(valueChanged(double)),
        this, SLOT(onZDirectionEditChanged(double)));
    connect(ui->buttonReverse, SIGNAL(clicked()),
        this, SLOT(onButtonReverse()));
    connect(ui->offsetEdit, SIGNAL(valueChanged(double)),
            this, SLOT(onOffsetChanged(double)));
    connect(ui->checkBoxMidplane, SIGNAL(toggled(bool)),
            this, SLOT(onMidplaneChanged(bool)));
    connect(ui->changeMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onModeChanged(int)));
    connect(ui->buttonEdge, SIGNAL(clicked()),
        this, SLOT(onButtonEdge()));
    connect(ui->lineEdgeName, SIGNAL(textChanged(QString)),
        this, SLOT(onEdgeNameChanged(QString)));
    connect(ui->lineEdgeName, SIGNAL(textEdited(QString)),
        this, SLOT(onEdgeNameEdited(QString)));
    connect(ui->buttonFace, SIGNAL(clicked()),
            this, SLOT(onButtonFace()));
    connect(ui->lineFaceName, SIGNAL(textEdited(QString)),
            this, SLOT(onFaceName(QString)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    // Due to signals attached after changes took took into effect we should update the UI now.
    updateUI(index);

    // if it is a newly created object use the last value of the history
    // TODO: newObj doesn't supplied normally by any caller (2015-07-24, Fat-Zer)
    if(newObj){
        ui->lengthEdit->setToLastUsedValue();
        ui->lengthEdit->selectNumber();
        ui->lengthEdit2->setToLastUsedValue();
        ui->lengthEdit2->selectNumber();
        ui->offsetEdit->setToLastUsedValue();
        ui->offsetEdit->selectNumber();
    }
}

void TaskPadParameters::updateUI(int index)
{
    // disable/hide everything unless we are sure we don't need it
    // exception: the direction parameters are in any case visible
    bool isLengthEditVisable  = false;
    bool isLengthEdit2Visable = false;
    bool isOffsetEditVisable  = false;
    bool isMidplateEnabled    = false;
    bool isReversedEnabled    = true;
    bool isReversedVisible    = true;
    bool isFaceEditEnabled    = false;

    // dimension
    if (index == 0) {
        isLengthEditVisable = true;
        ui->lengthEdit->selectNumber();
        // Make sure that the spin box has the focus to get key events
        // Calling setFocus() directly doesn't work because the spin box is not
        // yet visible.
        QMetaObject::invokeMethod(ui->lengthEdit, "setFocus", Qt::QueuedConnection);
        isMidplateEnabled = true;
        // Reverse only makes sense if Midplane is not true
        isReversedEnabled = !ui->checkBoxMidplane->isChecked();
    }
    // up to first/last
    else if (index == 1 || index == 2) {
        isOffsetEditVisable = true;
        isReversedEnabled   = false;
        isReversedVisible   = false;
    }
    // up to face
    else if (index == 3) {
        isOffsetEditVisable = true;
        isFaceEditEnabled   = true;
        isReversedEnabled   = false;
        isReversedVisible   = false;
        QMetaObject::invokeMethod(ui->lineFaceName, "setFocus", Qt::QueuedConnection);
        // Go into face selection mode if no face has been selected yet
        if (ui->lineFaceName->property("FeatureNameFace").isNull())
            onButtonFace(true);
    }
    // two dimensions
    else {
        isLengthEditVisable  = true;
        isLengthEdit2Visable = true;
    }

    ui->groupBoxLength->setVisible( isLengthEditVisable );
    ui->lengthEdit->setEnabled( isLengthEditVisable );
    ui->labelLength->setVisible( isLengthEditVisable );

    ui->offsetEdit->setVisible( isOffsetEditVisable );
    ui->offsetEdit->setEnabled( isOffsetEditVisable );
    ui->labelOffset->setVisible( isOffsetEditVisable );

    ui->checkBoxMidplane->setEnabled( isMidplateEnabled );

    ui->buttonReverse->setEnabled( isReversedEnabled );
    ui->buttonReverse->setVisible( isReversedVisible );

    ui->lengthEdit2->setVisible( isLengthEdit2Visable );
    ui->lengthEdit2->setEnabled( isLengthEdit2Visable );
    ui->labelLength2->setVisible( isLengthEdit2Visable );

    ui->buttonFace->setEnabled( isFaceEditEnabled );
    ui->lineFaceName->setEnabled( isFaceEditEnabled );
    if (!isFaceEditEnabled) {
        onButtonFace(false);
    }
}

void TaskPadParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (QString::fromLatin1(msg.pObjectName).length() > 0) {
            if (selectionMode == edge) {
                ui->lineEdgeName->blockSignals(true);
                ui->lineEdgeName->setProperty("FeatureNameEdge", QByteArray(msg.pObjectName));
                ui->lineEdgeName->setProperty("EdgeName", QByteArray(msg.pSubName));
                // set text for lineEdgeName
                QString LineEditText = QString::fromLatin1(msg.pObjectName) + QString::fromLatin1(":")
                    + QString::fromLatin1(msg.pSubName);
                ui->lineEdgeName->setText(LineEditText);
                ui->lineEdgeName->blockSignals(false);
                QString edgeName = ui->lineEdgeName->property("EdgeName").toString();
                QString FeatureNameEdge = ui->lineEdgeName->property("FeatureNameEdge").toString();
                auto featureObj = vp->getObject()->getDocument()->getObject(msg.pObjectName);
                // get part of selected feature
                PartDesign::Body* body = PartDesign::Body::findBodyOf(featureObj);
                const Part::TopoShape& shape = static_cast<const Part::Feature*>(body)->Shape.getValue();
                TopoDS_Shape sh = shape.getSubShape(msg.pSubName);
                const TopoDS_Edge& edgeShape = TopoDS::Edge(sh);
                // safe guard that shape really exists
                if (!edgeShape.IsNull()) {
                    // get coordinates of the endpoints
                    TopoDS_Vertex v1 = TopExp::FirstVertex(edgeShape, Standard_False);
                    TopoDS_Vertex v2 = TopExp::LastVertex(edgeShape, Standard_False);
                    gp_Pnt p1 = BRep_Tool::Pnt(v1);
                    gp_Pnt p2 = BRep_Tool::Pnt(v2);
                    // calculate vector
                    Base::Vector3d padDirection;
                    padDirection.Set(p2.X() - p1.X(), p2.Y() - p1.Y(), p2.Z() - p1.Z());
                    ui->XDirectionEdit->setValue(padDirection.x);
                    ui->YDirectionEdit->setValue(padDirection.y);
                    ui->ZDirectionEdit->setValue(padDirection.z);
                }
                // turn off selection mode
                onButtonEdge(false);
            }
            if (selectionMode == face) {
                ui->lineFaceName->blockSignals(true);
                ui->lineFaceName->setText(onAddSelection(msg));
                ui->lineFaceName->setProperty("FeatureNameFace", QByteArray(msg.pObjectName));
                ui->lineFaceName->setProperty("FaceName", QByteArray(msg.pSubName));
                ui->lineFaceName->blockSignals(false);
                // Turn off selection mode
                onButtonFace(false);
            }
        } else { // empty selection
            if (selectionMode == edge) {
                ui->lineEdgeName->blockSignals(true);
                ui->lineEdgeName->clear();
                ui->lineEdgeName->setProperty("FeatureNameEdge", QVariant());
                ui->lineEdgeName->setProperty("EdgeName", QVariant());
                ui->lineEdgeName->blockSignals(false);
            }
            if (selectionMode == face) {
                ui->lineFaceName->blockSignals(true);
                ui->lineFaceName->clear();
                ui->lineFaceName->setProperty("FeatureNameFace", QVariant());
                ui->lineFaceName->setProperty("FaceName", QVariant());
                ui->lineFaceName->blockSignals(false);
            }
        }
    } else if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        if (selectionMode == edge) {
            ui->lineEdgeName->blockSignals(true);
            ui->lineEdgeName->clear();
            ui->lineEdgeName->setProperty("FeatureNameEdge", QVariant());
            ui->lineEdgeName->setProperty("EdgeName", QVariant());
            ui->lineEdgeName->blockSignals(false);
        }
        if (selectionMode == face) {
            ui->lineFaceName->blockSignals(true);
            ui->lineFaceName->clear();
            ui->lineFaceName->setProperty("FeatureNameFace", QVariant());
            ui->lineFaceName->setProperty("FaceName", QVariant());
            ui->lineFaceName->blockSignals(false);
        }
    }
}

void TaskPadParameters::onLengthChanged(double len)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());
    pcPad->Length.setValue(len);
    recomputeFeature();
}

void TaskPadParameters::onLength2Changed(double len)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());
    pcPad->Length2.setValue(len);
    recomputeFeature();
}

void TaskPadParameters::onCBAlongDirectionChanged(bool on)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());
    pcPad->AlongCustomVector.setValue(on);
    recomputeFeature();
}

void TaskPadParameters::onGBDirectionChanged(bool on)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());
    pcPad->UseCustomVector.setValue(on);
    // dis/enable length direction
    ui->checkBoxAlongDirection->setEnabled(on);
    if (!on)
        ui->checkBoxAlongDirection->setChecked(!on);
    recomputeFeature();
    // the calculation of the sketch's normal vector is done in FeaturePad.cpp
    // if this vector was used for the recomputation we must fill the direction
    // vector edit fields. Therefore update
    updateDirectionEdits();
}

void TaskPadParameters::onXDirectionEditChanged(double len)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());
    pcPad->Direction.setValue(len, pcPad->Direction.getValue().y, pcPad->Direction.getValue().z);
    // empty lineEdgeName because we don't have any longer its direction
    // this will also reset the Edge property in TaskPadParameters::onEdgeNameChanged
    if (selectionMode != edge)
        ui->lineEdgeName->setText(QString());
    recomputeFeature();
    // checking for case of a null vector is done in FeaturePad.cpp
    // if there was a null vector, the normal vector of the sketch is used.
    // therefore the vector component edits must be updated
    updateDirectionEdits();
}

void TaskPadParameters::onYDirectionEditChanged(double len)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());
    pcPad->Direction.setValue(pcPad->Direction.getValue().x, len, pcPad->Direction.getValue().z);
    if (selectionMode != edge)
        ui->lineEdgeName->setText(QString());
    recomputeFeature();
    updateDirectionEdits();
}

void TaskPadParameters::onZDirectionEditChanged(double len)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());
    pcPad->Direction.setValue(pcPad->Direction.getValue().x, pcPad->Direction.getValue().y, len);
    if (selectionMode != edge)
        ui->lineEdgeName->setText(QString());
    recomputeFeature();
    updateDirectionEdits();
}

void TaskPadParameters::updateDirectionEdits(void)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());
    // we don't want to execute the onChanged edits, but just update their contents
    ui->XDirectionEdit->blockSignals(true);
    ui->YDirectionEdit->blockSignals(true);
    ui->ZDirectionEdit->blockSignals(true);
    ui->XDirectionEdit->setValue(pcPad->Direction.getValue().x);
    ui->YDirectionEdit->setValue(pcPad->Direction.getValue().y);
    ui->ZDirectionEdit->setValue(pcPad->Direction.getValue().z);
    ui->XDirectionEdit->blockSignals(false);
    ui->YDirectionEdit->blockSignals(false);
    ui->ZDirectionEdit->blockSignals(false);
}

void TaskPadParameters::onButtonReverse(const bool pressed)
{
    this->blockConnection(!pressed);

    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());

    // calculate opposite direction
    // Note: we explicitly don't use setValue because this can lead to rounding errors
    // we nee it precisely, independent of the user's digits
    onXDirectionEditChanged(-1.0f * pcPad->Direction.getValue().x);
    onYDirectionEditChanged(-1.0f * pcPad->Direction.getValue().y);
    onZDirectionEditChanged(-1.0f * pcPad->Direction.getValue().z);

    // the new direction does no longer fit selected edge
    ui->lineEdgeName->setText(QString());
}

void TaskPadParameters::onOffsetChanged(double len)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());
    pcPad->Offset.setValue(len);
    recomputeFeature();
}

void TaskPadParameters::onMidplaneChanged(bool on)
{
    // udapte the dialog because turing on midplane must disable the rverse button
    updateUI(0); 
    
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());
    pcPad->Midplane.setValue(on);
    recomputeFeature();
}

void TaskPadParameters::onModeChanged(int index)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());

    switch (index) {
        case 0:
            pcPad->Type.setValue("Length");
            // Avoid error message
            if (ui->lengthEdit->value() < Base::Quantity(Precision::Confusion(), Base::Unit::Length))
                ui->lengthEdit->setValue(5.0);
            break;
        case 1: pcPad->Type.setValue("UpToLast"); break;
        case 2: pcPad->Type.setValue("UpToFirst"); break;
        case 3: pcPad->Type.setValue("UpToFace"); break;
        default: pcPad->Type.setValue("TwoLengths");
    }

    updateUI(index);
    recomputeFeature();
}

void TaskPadParameters::onButtonEdge(const bool pressed)
{
    this->blockConnection(!pressed);

    // only straight edges are allowed
    TaskSketchBasedParameters::onSelectReference(pressed, true, false, true);

    // Update button if onButtonEdge() is called explicitly
    ui->buttonEdge->setChecked(pressed);
    if (pressed)
        selectionMode = edge;
    else
        exitSelectionMode();
}

void TaskPadParameters::onEdgeNameChanged(const QString& text)
{
    if (text.isEmpty()) {
        // if user cleared the text field then also clear the properties
        ui->lineEdgeName->setProperty("FeatureNameEdge", QVariant());
        ui->lineEdgeName->setProperty("EdgeName", QVariant());
    }
    // if not empty is handled in TaskPadParameters::onEdgeNameEdited because
    // here we just need a method to empty the property if lineEdgeName was emptied 
}

void TaskPadParameters::onEdgeNameEdited(const QString& text)
{
    if (text.isEmpty()) {
        // if user cleared the text field then also clear the properties
        ui->lineEdgeName->setProperty("FeatureNameEdge", QVariant());
        ui->lineEdgeName->setProperty("EdgeName", QVariant());
    }
    else {
        // expect that the label of an object is used
        QStringList parts = text.split(QChar::fromLatin1(':'));
        QString label = parts[0];
        QVariant name = objectNameByLabel(label, ui->lineEdgeName->property("FeatureNameEdge"));
        if (name.isValid()) {
            parts[0] = name.toString();
            QString edge = parts.join(QString::fromLatin1(":"));
            ui->lineEdgeName->setProperty("FeatureNameEdge", name);
            ui->lineEdgeName->setProperty("EdgeName", QVariant(edge));
        }
        else {
            ui->lineEdgeName->setProperty("FeatureNameEdge", QVariant());
            ui->lineEdgeName->setProperty("EdgeName", QVariant());
        }
    }
}

void TaskPadParameters::onButtonFace(const bool pressed)
{
    this->blockConnection(!pressed);

    // only faces are allowed
    TaskSketchBasedParameters::onSelectReference(pressed, false, true, false);

    // Update button if onButtonFace() is called explicitly
    ui->buttonFace->setChecked(pressed);
    if (pressed)
        selectionMode = face;
    else
        exitSelectionMode();
}

void TaskPadParameters::onFaceName(const QString& text)
{
    if (text.isEmpty()) {
        // if user cleared the text field then also clear the properties
        ui->lineFaceName->setProperty("FeatureNameFace", QVariant());
        ui->lineFaceName->setProperty("FaceName", QVariant());
    }
    else {
        // expect that the label of an object is used
        QStringList parts = text.split(QChar::fromLatin1(':'));
        QString label = parts[0];
        QVariant name = objectNameByLabel(label, ui->lineFaceName->property("FeatureNameFace"));
        if (name.isValid()) {
            parts[0] = name.toString();
            QString uptoface = parts.join(QString::fromLatin1(":"));
            ui->lineFaceName->setProperty("FeatureNameFace", name);
            ui->lineFaceName->setProperty("FaceName", setUpToFace(uptoface));
        }
        else {
            ui->lineFaceName->setProperty("FeatureNameFace", QVariant());
            ui->lineFaceName->setProperty("FaceName", QVariant());
        }
    }
}

double TaskPadParameters::getLength(void) const
{
    return ui->lengthEdit->value().getValue();
}

double TaskPadParameters::getLength2(void) const
{
    return ui->lengthEdit2->value().getValue();
}

bool   TaskPadParameters::alongCustom(void) const
{
    return ui->checkBoxAlongDirection->isChecked();
}

bool   TaskPadParameters::getCustom(void) const
{
    return ui->groupBoxDirection->isChecked();
}

double TaskPadParameters::getXDirection(void) const
{
    return ui->XDirectionEdit->value();
}

double TaskPadParameters::getYDirection(void) const
{
    return ui->YDirectionEdit->value();
}

double TaskPadParameters::getZDirection(void) const
{
    return ui->ZDirectionEdit->value();
}

double TaskPadParameters::getOffset(void) const
{
    return ui->offsetEdit->value().getValue();
}

bool   TaskPadParameters::getMidplane(void) const
{
    return ui->checkBoxMidplane->isChecked();
}

int TaskPadParameters::getMode(void) const
{
    return ui->changeMode->currentIndex();
}

QString TaskPadParameters::getEdgeName(void) const
{
    QVariant featureName = ui->lineEdgeName->property("FeatureNameEdge");
    if (featureName.isValid()) {
        QString edgeName = ui->lineEdgeName->property("EdgeName").toString();
        // we can use the routine to get faces because it also delivers edges
        return getFaceReference(featureName.toString(), edgeName);
    }
    return QString::fromLatin1("None");
}

QString TaskPadParameters::getFaceName(void) const
{
    // 'Up to face' mode
    if (getMode() == 3) {
        QVariant featureName = ui->lineFaceName->property("FeatureNameFace");
        if (featureName.isValid()) {
            QString faceName = ui->lineFaceName->property("FaceName").toString();
            return getFaceReference(featureName.toString(), faceName);
        }
    }
    return QString::fromLatin1("None");
}

TaskPadParameters::~TaskPadParameters()
{
    delete ui;
}

void TaskPadParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->lengthEdit->blockSignals(true);
        ui->lengthEdit2->blockSignals(true);
        ui->XDirectionEdit->blockSignals(true);
        ui->YDirectionEdit->blockSignals(true);
        ui->ZDirectionEdit->blockSignals(true);
        ui->offsetEdit->blockSignals(true);
        ui->lineEdgeName->blockSignals(true);
        ui->lineFaceName->blockSignals(true);
        ui->changeMode->blockSignals(true);
        int index = ui->changeMode->currentIndex();
        ui->retranslateUi(proxy);
        ui->changeMode->clear();
        ui->changeMode->addItem(tr("Dimension"));
        ui->changeMode->addItem(tr("To last"));
        ui->changeMode->addItem(tr("To first"));
        ui->changeMode->addItem(tr("Up to face"));
        ui->changeMode->addItem(tr("Two dimensions"));
        ui->changeMode->setCurrentIndex(index);

#if QT_VERSION >= 0x040700
        ui->lineEdgeName->setPlaceholderText(tr("No edge selected"));
        ui->lineFaceName->setPlaceholderText(tr("No face selected"));
#endif
        QVariant featureNameEdge = ui->lineEdgeName->property("FeatureNameEdge");
        if (featureNameEdge.isValid()) {
            QStringList partsEdge = ui->lineEdgeName->text().split(QChar::fromLatin1(':'));
            QByteArray edge = ui->lineEdgeName->property("EdgeName").toByteArray();
            int edgeId = -1;
            bool edgeOK = false;
            if (edge.indexOf("Edge") == 0) {
                edgeId = edge.remove(0, 4).toInt(&edgeOK);
            }

            if (edgeOK) {
                ui->lineEdgeName->setText(QString::fromLatin1("%1:%2%3")
                                          .arg(partsEdge[0])
                                          .arg(tr("Edge"))
                                          .arg(edgeId));
            }
            else {
                ui->lineEdgeName->setText(partsEdge[0]);
            }
        }
        QVariant featureNameFace = ui->lineFaceName->property("FeatureNameFace");
        if (featureNameFace.isValid()) {
            QStringList partsFace = ui->lineFaceName->text().split(QChar::fromLatin1(':'));
            QByteArray upToFace = ui->lineFaceName->property("FaceName").toByteArray();
            int faceId = -1;
            bool faceOK = false;
            if (upToFace.indexOf("Face") == 0) {
                faceId = upToFace.remove(0, 4).toInt(&faceOK);
            }

            if (faceOK) {
                ui->lineFaceName->setText(QString::fromLatin1("%1:%2%3")
                    .arg(partsFace[0])
                    .arg(tr("Face"))
                    .arg(faceId));
            }
            else {
                ui->lineFaceName->setText(partsFace[0]);
            }
        }

        ui->lengthEdit->blockSignals(false);
        ui->lengthEdit2->blockSignals(false);
        ui->XDirectionEdit->blockSignals(false);
        ui->YDirectionEdit->blockSignals(false);
        ui->ZDirectionEdit->blockSignals(false);
        ui->offsetEdit->blockSignals(false);
        ui->lineEdgeName->blockSignals(false);
        ui->lineFaceName->blockSignals(false);
        ui->changeMode->blockSignals(false);
    }
}

void TaskPadParameters::saveHistory(void)
{
    // save the user values to history
    ui->lengthEdit->pushToHistory();
    ui->lengthEdit2->pushToHistory();
    ui->offsetEdit->pushToHistory();
}

void TaskPadParameters::apply()
{
    auto obj = vp->getObject();

    ui->lengthEdit->apply();
    ui->lengthEdit2->apply();
    FCMD_OBJ_CMD(obj, "UseCustomVector = " << (getCustom() ? 1 : 0));
    // the direction is already written to the property when changing or setting it
    //FCMD_OBJ_CMD(obj, "Direction = ("
    //    << getXDirection() << ", " << getYDirection() << ", " << getZDirection() << ")");
    FCMD_OBJ_CMD(obj, "AlongCustomVector = " << (alongCustom() ? 1 : 0));
    QString edgename = getEdgeName();
    FCMD_OBJ_CMD(obj, "Edge = " << edgename.toLatin1().data());
    FCMD_OBJ_CMD(obj,"Type = " << getMode());
    QString facename = getFaceName();
    FCMD_OBJ_CMD(obj,"UpToFace = " << facename.toLatin1().data());
    FCMD_OBJ_CMD(obj,"Midplane = " << (getMidplane()?1:0));
    FCMD_OBJ_CMD(obj,"Offset = " << getOffset());
}

void TaskPadParameters::exitSelectionMode()
{
    try {
        auto obj = vp->getObject();
        ui->buttonEdge->setChecked(false);
        ui->buttonFace->setChecked(false);
        selectionMode = none;
        Gui::Selection().rmvSelectionGate();
        try {
            FCMD_OBJ_SHOW(obj); // show the feature
        }
        catch (Base::Exception& e) {
            e.ReportException();
        }
    }
    catch (Base::Exception& e) {
        e.ReportException();
    }
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
