/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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
#include <QMessageBox>
#include <cmath>
#endif// #ifndef _PreComp_

#include "Widgets/CompassWidget.h"
#include "Widgets/VectorEditWidget.h"
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSection.h>

// clang-format off
#include "ui_TaskSectionView.h"
#include "TaskSectionView.h"
// clang-format on


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//ctor for create
TaskSectionView::TaskSectionView(TechDraw::DrawViewPart* base) :
    ui(new Ui_TaskSectionView),
    m_base(base),
    m_section(nullptr),
    m_saveScale(1.0),
    m_dirName(""),
    m_doc(nullptr),
    m_createMode(true),
    m_saved(false),
    m_applyDeferred(0),
    m_directionIsSet(false),
    m_modelIsDirty(false)
{
    //existence of base is guaranteed by CmdTechDrawSectionView (Command.cpp)

    m_sectionName = std::string();
    m_doc = m_base->getDocument();

    m_saveBaseName = m_base->getNameInDocument();
    m_savePageName = m_base->findParentPage()->getNameInDocument();

    ui->setupUi(this);
    setUiPrimary();

    m_applyDeferred = 0;//setting the direction widgets causes an increment of the deferred count,
                        //so we reset the counter and the message.
}

//ctor for edit
TaskSectionView::TaskSectionView(TechDraw::DrawViewSection* section) :
    ui(new Ui_TaskSectionView),
    m_base(nullptr),
    m_section(section),
    m_saveScale(1.0),
    m_doc(nullptr),
    m_createMode(false),
    m_saved(false),
    m_applyDeferred(0),
    m_directionIsSet(true),
    m_modelIsDirty(false)
{
    //existence of section is guaranteed by ViewProviderViewSection.setEdit

    m_doc = m_section->getDocument();
    m_sectionName = m_section->getNameInDocument();
    App::DocumentObject* newObj = m_section->BaseView.getValue();
    m_base = dynamic_cast<TechDraw::DrawViewPart*>(newObj);
    if (!newObj || !m_base) {
        throw Base::RuntimeError("TaskSectionView - BaseView not found");
    }

    m_saveBaseName = m_base->getNameInDocument();
    m_savePageName = m_base->findParentPage()->getNameInDocument();

    ui->setupUi(this);

    m_dirName = m_section->SectionDirection.getValueAsString();
    saveSectionState();
    setUiEdit();

    m_applyDeferred = 0;//setting the direction widgets causes an increment of the deferred count,
                        //so we reset the counter and the message.
    ui->lPendingUpdates->setText(QString());
}

void TaskSectionView::setUiPrimary()
{
    //    Base::Console().Message("TSV::setUiPrimary()\n");
    setWindowTitle(QObject::tr("Create Section View"));

    ui->sbScale->setValue(m_base->getScale());
    ui->cmbScaleType->setCurrentIndex(m_base->ScaleType.getValue());

    //Allow or prevent scale changing initially
    if (m_base->ScaleType.isValue("Custom")) {
        ui->sbScale->setEnabled(true);
    }
    else {
        ui->sbScale->setEnabled(false);
    }

    Base::Vector3d origin = m_base->getOriginalCentroid();
    setUiCommon(origin);

    m_compass->setDialAngle(0.0);
    m_viewDirectionWidget->setValueNoNotify(Base::Vector3d(1.0, 0.0, 0.0));

    //don't allow updates until a direction is picked
    ui->pbUpdateNow->setEnabled(false);
    ui->cbLiveUpdate->setEnabled(false);
    QString msgLiteral =
        QString::fromUtf8(QT_TRANSLATE_NOOP("TaskSectionView", "No direction set"));
    ui->lPendingUpdates->setText(msgLiteral);
}

void TaskSectionView::setUiEdit()
{
    //    Base::Console().Message("TSV::setUiEdit()\n");
    setWindowTitle(QObject::tr("Edit Section View"));
    std::string temp = m_section->SectionSymbol.getValue();
    QString qTemp = Base::Tools::fromStdString(temp);
    ui->leSymbol->setText(qTemp);

    ui->sbScale->setValue(m_section->getScale());
    ui->cmbScaleType->setCurrentIndex(m_section->ScaleType.getValue());
    //Allow or prevent scale changing initially
    if (m_section->ScaleType.isValue("Custom")) {
        ui->sbScale->setEnabled(true);
    }
    else {
        ui->sbScale->setEnabled(false);
    }

    Base::Vector3d origin = m_section->SectionOrigin.getValue();
    setUiCommon(origin);

    // convert section normal to view angle
    Base::Vector3d sectionNormalVec = m_section->SectionNormal.getValue();
    sectionNormalVec.Normalize();
    Base::Vector3d projectedViewDirection = m_base->projectPoint(sectionNormalVec, false);
    projectedViewDirection.Normalize();
    double viewAngle = atan2(-projectedViewDirection.y, -projectedViewDirection.x);
    m_compass->setDialAngle(viewAngle * 180.0 / M_PI);
    m_viewDirectionWidget->setValueNoNotify(sectionNormalVec * -1.0);
}

void TaskSectionView::setUiCommon(Base::Vector3d origin)
{
    std::string temp = m_base->getNameInDocument();
    QString qTemp = Base::Tools::fromStdString(temp);
    ui->leBaseView->setText(qTemp);

    ui->sbOrgX->setUnit(Base::Unit::Length);
    ui->sbOrgX->setValue(origin.x);
    ui->sbOrgY->setUnit(Base::Unit::Length);
    ui->sbOrgY->setValue(origin.y);
    ui->sbOrgZ->setUnit(Base::Unit::Length);
    ui->sbOrgZ->setValue(origin.z);

    enableAll(false);

    connect(ui->leSymbol, &QLineEdit::editingFinished, this, &TaskSectionView::onIdentifierChanged);

    //TODO: use event filter instead of keyboard tracking to capture enter/return keys
    // the UI file uses keyboardTracking = false so that a recomputation
    // will only be triggered when the arrow keys of the spinboxes are used
    //if this is not done, recomputes are triggered on each key press giving
    //unaccceptable UX
    connect(ui->sbScale, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskSectionView::onScaleChanged);
    connect(ui->sbOrgX, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskSectionView::onXChanged);
    connect(ui->sbOrgY, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskSectionView::onYChanged);
    connect(ui->sbOrgZ, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskSectionView::onZChanged);

    connect(ui->cmbScaleType, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskSectionView::scaleTypeChanged);

    connect(ui->pbUp, &QToolButton::clicked, this, &TaskSectionView::onUpClicked);
    connect(ui->pbDown, &QToolButton::clicked, this, &TaskSectionView::onDownClicked);
    connect(ui->pbRight, &QToolButton::clicked, this, &TaskSectionView::onRightClicked);
    connect(ui->pbLeft, &QToolButton::clicked, this, &TaskSectionView::onLeftClicked);

    connect(ui->pbUpdateNow, &QToolButton::clicked, this, &TaskSectionView::updateNowClicked);
    connect(ui->cbLiveUpdate, &QToolButton::clicked, this, &TaskSectionView::liveUpdateClicked);

    m_compass = new CompassWidget(this);
    auto layout = ui->compassLayout;
    layout->addWidget(m_compass);
    connect(m_compass, &CompassWidget::angleChanged, this, &TaskSectionView::slotChangeAngle);

    m_viewDirectionWidget = new VectorEditWidget(this);
    m_viewDirectionWidget->setLabel(QObject::tr("Current View Direction"));
    m_viewDirectionWidget->setToolTip(QObject::tr("The view direction in BaseView coordinates"));
    auto editLayout = ui->viewDirectionLayout;
    editLayout->addWidget(m_viewDirectionWidget);
    connect(m_viewDirectionWidget, &VectorEditWidget::valueChanged, this,
            &TaskSectionView::slotViewDirectionChanged);
}

//save the start conditions
void TaskSectionView::saveSectionState()
{
    //    Base::Console().Message("TSV::saveSectionState()\n");
    if (m_section) {
        m_saveSymbol = m_section->SectionSymbol.getValue();
        m_saveScale = m_section->getScale();
        m_saveScaleType = m_section->ScaleType.getValue();
        m_saveNormal = m_section->SectionNormal.getValue();
        m_normal = m_saveNormal;
        m_saveDirection = m_section->Direction.getValue();
        m_saveOrigin = m_section->SectionOrigin.getValue();
        m_saveDirName = m_section->SectionDirection.getValueAsString();
        m_saved = true;
    }
}

//restore the start conditions
void TaskSectionView::restoreSectionState()
{
    //    Base::Console().Message("TSV::restoreSectionState()\n");
    if (!m_section)
        return;

    m_section->SectionSymbol.setValue(m_saveSymbol);
    m_section->Scale.setValue(m_saveScale);
    m_section->ScaleType.setValue(m_saveScaleType);
    m_section->SectionNormal.setValue(m_saveNormal);
    m_section->Direction.setValue(m_saveDirection);
    m_section->SectionOrigin.setValue(m_saveOrigin);
    m_section->SectionDirection.setValue(m_saveDirName.c_str());
}

//the VectorEditWidget reports a change in direction
void TaskSectionView::slotViewDirectionChanged(Base::Vector3d newDirection)
{
    //    Base::Console().Message("TSV::slotViewDirectionChanged(%s)\n",
    //                            DrawUtil::formatVector(newDirection).c_str());
    Base::Vector3d projectedViewDirection = m_base->projectPoint(newDirection, false);
    projectedViewDirection.Normalize();
    double viewAngle = atan2(projectedViewDirection.y, projectedViewDirection.x);
    m_compass->setDialAngle(viewAngle * 180.0 / M_PI);
    checkAll(false);
    applyAligned();
}

//the CompassWidget reports that the view direction angle has changed
void TaskSectionView::slotChangeAngle(double newAngle)
{
    //    Base::Console().Message("TSV::slotChangeAngle(%.3f)\n", newAngle);
    double angleRadians = newAngle * M_PI / 180.0;
    double unitX = cos(angleRadians);
    double unitY = sin(angleRadians);
    Base::Vector3d localUnit(unitX, unitY, 0.0);
    m_viewDirectionWidget->setValueNoNotify(localUnit);
    checkAll(false);
    applyAligned();
}

//preset view directions
void TaskSectionView::onUpClicked()
{
    //    Base::Console().Message("TSV::onUpClicked()\n");
    checkAll(false);
    m_compass->setToNorth();
    m_viewDirectionWidget->setValueNoNotify(Base::Vector3d(0.0, 1.0, 0.0));
    applyAligned();
}

void TaskSectionView::onDownClicked()
{
    //    Base::Console().Message("TSV::onDownClicked()\n");
    checkAll(false);
    m_compass->setToSouth();
    m_viewDirectionWidget->setValueNoNotify(Base::Vector3d(0.0, -1.0, 0.0));
    applyAligned();
}

void TaskSectionView::onLeftClicked()
{
    //    Base::Console().Message("TSV::onLeftClicked()\n");
    checkAll(false);
    m_compass->setToWest();
    m_viewDirectionWidget->setValueNoNotify(Base::Vector3d(-1.0, 0.0, 0.0));
    applyAligned();
}

void TaskSectionView::onRightClicked()
{
    //    Base::Console().Message("TSV::onRightClicked()\n");
    checkAll(false);
    m_compass->setToEast();
    m_viewDirectionWidget->setValueNoNotify(Base::Vector3d(1.0, 0.0, 0.0));
    applyAligned();
}

void TaskSectionView::onIdentifierChanged()
{
    checkAll(false);
    apply();
}

void TaskSectionView::onScaleChanged()
{
    checkAll(false);
    apply();
}

//SectionOrigin changed
void TaskSectionView::onXChanged()
{
    checkAll(false);
    apply();
}
void TaskSectionView::onYChanged()
{
    checkAll(false);
    apply();
}
void TaskSectionView::onZChanged()
{
    checkAll(false);
    apply();
}

void TaskSectionView::scaleTypeChanged(int index)
{
    if (index == 0) {
        // Page Scale Type
        ui->sbScale->setEnabled(false);
        if (m_base->findParentPage()) {
            ui->sbScale->setValue(m_base->findParentPage()->Scale.getValue());
            ui->sbScale->setEnabled(false);
        }
    }
    else if (index == 1) {
        // Automatic Scale Type
        ui->sbScale->setEnabled(false);
        if (m_section) {
            ui->sbScale->setValue(m_section->autoScale());
        }
    }
    else if (index == 2) {
        // Custom Scale Type
        ui->sbScale->setEnabled(true);
        if (m_section) {
            ui->sbScale->setValue(m_section->Scale.getValue());
            ui->sbScale->setEnabled(true);
        }
    }
    else {
        return;
    }
}

void TaskSectionView::checkAll(bool check)
{
    ui->pbUp->setChecked(check);
    ui->pbDown->setChecked(check);
    ui->pbRight->setChecked(check);
    ui->pbLeft->setChecked(check);
}

void TaskSectionView::enableAll(bool enable)
{
    ui->leSymbol->setEnabled(enable);
    ui->sbScale->setEnabled(enable);
    ui->sbOrgX->setEnabled(enable);
    ui->sbOrgY->setEnabled(enable);
    ui->sbOrgZ->setEnabled(enable);
    ui->cmbScaleType->setEnabled(enable);
    QString qScaleType = ui->cmbScaleType->currentText();
    //Allow or prevent scale changing initially
    if (qScaleType == QString::fromUtf8("Custom")) {
        ui->sbScale->setEnabled(true);
    }
    else {
        ui->sbScale->setEnabled(false);
    }
}

void TaskSectionView::liveUpdateClicked() { apply(true); }

void TaskSectionView::updateNowClicked() { apply(true); }

//******************************************************************************
bool TaskSectionView::apply(bool forceUpdate)
{
//    Base::Console().Message("TSV::apply() - liveUpdate: %d force: %d deferred: %d\n",
//                            ui->cbLiveUpdate->isChecked(), forceUpdate, m_applyDeferred);
    if (!ui->cbLiveUpdate->isChecked() && !forceUpdate) {
        //nothing to do
        m_applyDeferred++;
        QString msgLiteral =
            QString::fromUtf8(QT_TRANSLATE_NOOP("TaskPojGroup", " updates pending"));
        QString msgNumber = QString::number(m_applyDeferred);
        ui->lPendingUpdates->setText(msgNumber + msgLiteral);
        return false;
    }

    Gui::WaitCursor wc;
    m_modelIsDirty = true;

    if (m_dirName.empty()) {
        //this should never happen
        std::string msg =
            Base::Tools::toStdString(tr("Nothing to apply. No section direction picked yet"));
        Base::Console().Error((msg + "\n").c_str());
        return false;
    }
    if (!m_section) {
        m_section = createSectionView();
    }

    if (isSectionValid()) {
        updateSectionView();
    }
    else {
        failNoObject();
    }

    m_section->recomputeFeature();
    if (isBaseValid()) {
        m_base->requestPaint();
    }

    enableAll(true);
    checkAll(false);

    wc.restoreCursor();
    m_applyDeferred = 0;
    ui->lPendingUpdates->setText(QString());
    return true;
}

void TaskSectionView::applyQuick(std::string dir)
{
    //    Base::Console().Message("TSV::applyQuick(%s)\n", dir.c_str());
    m_dirName = dir;
    enableAll(true);
    apply();
}

void TaskSectionView::applyAligned()
{
    //    Base::Console().Message("TSV::applyAligned()\n");
    m_dirName = "Aligned";
    enableAll(true);
    m_directionIsSet = true;
    ui->pbUpdateNow->setEnabled(true);
    ui->cbLiveUpdate->setEnabled(true);
    apply();
}

//*********************************************************************

TechDraw::DrawViewSection* TaskSectionView::createSectionView(void)
{
    //    Base::Console().Message("TSV::createSectionView()\n");
    if (!isBaseValid()) {
        failNoObject();
        return nullptr;
    }

    std::string baseName = m_base->getNameInDocument();

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create SectionView"));
    if (!m_section) {
        const std::string objectName("SectionView");
        m_sectionName = m_base->getDocument()->getUniqueObjectName(objectName.c_str());
        Command::doCommand(Command::Doc, "App.ActiveDocument.addObject('TechDraw::DrawViewSection', '%s')",
                           m_sectionName.c_str());

        // section labels (Section A-A) are not unique, and are not the same as the object name (SectionView)
        // we pluck the generated suffix from the object name and append it to "Section" to generate
        // unique Labels
        QString qTemp = ui->leSymbol->text();
        std::string temp = Base::Tools::toStdString(qTemp);
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.SectionSymbol = '%s'",
                           m_sectionName.c_str(), temp.c_str());

        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Label = '%s'",
                           m_sectionName.c_str(),
                           makeSectionLabel(qTemp).c_str());
        Command::doCommand(Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewSection', 'Section', '%s')",
              m_sectionName.c_str(), makeSectionLabel(qTemp).c_str());


        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.addView(App.ActiveDocument.%s)",
                           m_savePageName.c_str(), m_sectionName.c_str());
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.BaseView = App.ActiveDocument.%s",
                           m_sectionName.c_str(), baseName.c_str());
        Command::doCommand(Command::Doc,
                           "App.ActiveDocument.%s.Source = App.ActiveDocument.%s.Source",
                           m_sectionName.c_str(), baseName.c_str());
        Command::doCommand(Command::Doc,
                           "App.ActiveDocument.%s.SectionOrigin = FreeCAD.Vector(%.6f, %.6f, %.6f)",
                           m_sectionName.c_str(), ui->sbOrgX->value().getValue(),
                           ui->sbOrgY->value().getValue(), ui->sbOrgZ->value().getValue());
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Scale = %0.6f",
                           m_sectionName.c_str(), ui->sbScale->value().getValue());
        int scaleType = ui->cmbScaleType->currentIndex();
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.ScaleType = %d",
                           m_sectionName.c_str(), scaleType);
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.SectionDirection = '%s'",
                           m_sectionName.c_str(), m_dirName.c_str());

        App::DocumentObject* newObj = m_base->getDocument()->getObject(m_sectionName.c_str());
        m_section = dynamic_cast<TechDraw::DrawViewSection*>(newObj);
        if (!newObj || !m_section) {
            throw Base::RuntimeError("TaskSectionView - new section object not found");
        }
        Base::Vector3d localUnit = m_viewDirectionWidget->value();
        localUnit.Normalize();
        if (m_dirName == "Aligned") {
            //localUnit is a view direction so we need to reverse it to make a
            //section normal
            m_section->setCSFromBase(localUnit * -1.0);
        }
        else {
            //Note: DirectionName is to be deprecated in the future
            m_section->setCSFromBase(m_dirName.c_str());
        }
        //auto orientation of view relative to base view
        double viewDirectionAngle = m_compass->positiveValue();
        double rotation = requiredRotation(viewDirectionAngle);
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Rotation = %.6f",
                           m_sectionName.c_str(), rotation);

    }
    Gui::Command::commitCommand();
    return m_section;
}

void TaskSectionView::updateSectionView()
{
//    Base::Console().Message("TSV::updateSectionView() - m_sectionName: %s\n", m_sectionName.c_str());
    if (!isSectionValid()) {
        failNoObject();
        return;
    }

    const std::string objectName("SectionView");
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Edit SectionView"));
    if (m_section) {
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.SectionDirection = '%s'",
                           m_sectionName.c_str(), m_dirName.c_str());
        Command::doCommand(Command::Doc,
                           "App.ActiveDocument.%s.SectionOrigin = FreeCAD.Vector(%.3f, %.3f, %.3f)",
                           m_sectionName.c_str(), ui->sbOrgX->value().getValue(),
                           ui->sbOrgY->value().getValue(), ui->sbOrgZ->value().getValue());

        QString qTemp = ui->leSymbol->text();
        std::string temp = Base::Tools::toStdString(qTemp);
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.SectionSymbol = '%s'",
                           m_sectionName.c_str(), temp.c_str());

        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Label = '%s'",
                           m_sectionName.c_str(),
                           makeSectionLabel(qTemp).c_str());
        Command::doCommand(Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewSection', 'Section', '%s')",
              m_sectionName.c_str(), makeSectionLabel(qTemp).c_str());

        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Scale = %0.6f",
                           m_sectionName.c_str(), ui->sbScale->value().getValue());
        int scaleType = ui->cmbScaleType->currentIndex();
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.ScaleType = %d",
                           m_sectionName.c_str(), scaleType);
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.SectionDirection = '%s'",
                           m_sectionName.c_str(), m_dirName.c_str());
        Base::Vector3d localUnit = m_viewDirectionWidget->value();
        localUnit.Normalize();
        if (m_dirName == "Aligned") {
            //localUnit is a view direction so we need to reverse it to make a
            //section normal
            m_section->setCSFromBase(localUnit * -1.0);
        }
        else {
            //Note: DirectionName is to be deprecated in the future
            m_section->setCSFromBase(m_dirName.c_str());
        }
        //auto orientation of view relative to base view
        double viewDirectionAngle = m_compass->positiveValue();
        double rotation = requiredRotation(viewDirectionAngle);
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Rotation = %.6f",
                           m_sectionName.c_str(), rotation);
    }
    Gui::Command::commitCommand();
}

std::string TaskSectionView::makeSectionLabel(QString symbol)
{
    const std::string objectName("SectionView");
    std::string uniqueSuffix{m_sectionName.substr(objectName.length(), std::string::npos)};
    std::string uniqueLabel = "Section" + uniqueSuffix;
    std::string temp = Base::Tools::toStdString(symbol);
    return ( uniqueLabel + " " + temp + " - " + temp );
}

void TaskSectionView::failNoObject(void)
{
    QString qsectionName = Base::Tools::fromStdString(m_sectionName);
    QString qbaseName = Base::Tools::fromStdString(m_baseName);
    QString msg = tr("Can not continue. Object * %1 or %2 not found.").arg(qsectionName, qbaseName);
    QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Operation Failed"), msg);
    Gui::Control().closeDialog();
}

bool TaskSectionView::isBaseValid()
{
    if (!m_base)
        return false;

    App::DocumentObject* baseObj = m_doc->getObject(m_saveBaseName.c_str());
    if (!baseObj)
        return false;

    return true;
}

bool TaskSectionView::isSectionValid()
{
    if (!m_section)
        return false;

    App::DocumentObject* sectionObj = m_doc->getObject(m_sectionName.c_str());
    if (!sectionObj)
        return false;

    return true;
}

//get required rotation from input angle in [0, 360]
double TaskSectionView::requiredRotation(double inputAngle)
{
    double rotation = inputAngle - 90.0;
    if (rotation == 180.0) {
        //if the view direction is 90/270, then the section is drawn properly and no
        //rotation is needed.  90.0 becomes 0.0, but 270.0 needs special handling.
        rotation = 0.0;
    }
    return rotation;
}

//******************************************************************************

bool TaskSectionView::accept()
{
    //    Base::Console().Message("TSV::accept()\n");
    apply(true);
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    return true;
}

bool TaskSectionView::reject()
{
    //    Base::Console().Message("TSV::reject()\n");
    if (!m_section) {//no section created, nothing to undo
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
        return false;
    }

    if (!isSectionValid()) {//section !exist. nothing to undo
        if (isBaseValid()) {
            m_base->requestPaint();
        }
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
        return false;
    }

    if (m_createMode) {
        std::string SectionName = m_section->getNameInDocument();
        Gui::Command::doCommand(Gui::Command::Gui,
                                "App.ActiveDocument.%s.removeView(App.ActiveDocument.%s)",
                                m_savePageName.c_str(), SectionName.c_str());
        Gui::Command::doCommand(Gui::Command::Gui, "App.ActiveDocument.removeObject('%s')",
                                SectionName.c_str());

    } else {
        if (m_modelIsDirty) {
            restoreSectionState();
            m_section->recomputeFeature();
            m_section->requestPaint();
        }
    }

    if (isBaseValid()) {
        m_base->requestPaint();
    }
    Gui::Command::updateActive();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return false;
}

void TaskSectionView::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgSectionView::TaskDlgSectionView(TechDraw::DrawViewPart* base) : TaskDialog()
{
    widget = new TaskSectionView(base);
    taskbox =
        new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_SectionView"),
                                   widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgSectionView::TaskDlgSectionView(TechDraw::DrawViewSection* section) : TaskDialog()
{
    widget = new TaskSectionView(section);
    taskbox =
        new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_SectionView"),
                                   widget->windowTitle(), true, nullptr);

    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}
TaskDlgSectionView::~TaskDlgSectionView() {}

void TaskDlgSectionView::update()
{
    //widget->updateTask();
}

//==== calls from the TaskView ===============================================================
void TaskDlgSectionView::open() {}

bool TaskDlgSectionView::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgSectionView::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskSectionView.cpp>
