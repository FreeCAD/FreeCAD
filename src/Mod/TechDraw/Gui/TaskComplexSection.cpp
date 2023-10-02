/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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
#include <gp_Pnt.hxx>
#endif// #ifndef _PreComp_

#include <App/Document.h>
#include <App/Link.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/WaitCursor.h>

#include "Widgets/CompassWidget.h"
#include "Widgets/VectorEditWidget.h"
#include <Mod/TechDraw/App/DrawComplexSection.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "DrawGuiUtil.h"
#include "TaskComplexSection.h"
#include "ui_TaskComplexSection.h"


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;
using DU = DrawUtil;

//ctor for creation
TaskComplexSection::TaskComplexSection(TechDraw::DrawPage* page, TechDraw::DrawViewPart* baseView,
                                       std::vector<App::DocumentObject*> shapes,
                                       std::vector<App::DocumentObject*> xShapes,
                                       App::DocumentObject* profileObject,
                                       std::vector<std::string> profileSubs) :
    ui(new Ui_TaskComplexSection),
    m_page(page),
    m_baseView(baseView),
    m_section(nullptr),
    m_shapes(shapes),
    m_xShapes(xShapes),
    m_profileObject(profileObject),
    m_profileSubs(profileSubs),
    m_dirName("Aligned"),
    m_createMode(true),
    m_applyDeferred(0),
    m_angle(0.0),
    m_directionIsSet(false),
    m_modelIsDirty(false)
{
    m_sectionName = std::string();
    if (m_page) {
        m_doc = m_page->getDocument();
        m_savePageName = m_page->getNameInDocument();
    }
    if (m_baseView) {
        m_saveBaseName = m_baseView->getNameInDocument();
    }
    ui->setupUi(this);

    saveSectionState();
    setUiPrimary();

    m_applyDeferred = 0;//setting the direction widgets causes an increment of the deferred count,
                        //so we reset the counter and the message.
}

//ctor for edit
TaskComplexSection::TaskComplexSection(TechDraw::DrawComplexSection* complexSection) :
    ui(new Ui_TaskComplexSection),
    m_page(nullptr),
    m_baseView(nullptr),
    m_section(complexSection),
    m_profileObject(nullptr),
    m_dirName("Aligned"),
    m_createMode(false),
    m_applyDeferred(0),
    m_angle(0.0),
    m_directionIsSet(true),
    m_modelIsDirty(false)
{
    m_sectionName = m_section->getNameInDocument();
    m_doc = m_section->getDocument();
    m_page = m_section->findParentPage();
    m_savePageName = m_page->getNameInDocument();

    m_baseView = dynamic_cast<TechDraw::DrawViewPart*>(m_section->BaseView.getValue());
    if (m_baseView) {
        m_saveBaseName = m_baseView->getNameInDocument();
    }

    m_shapes = m_section->Source.getValues();
    m_xShapes = m_section->XSource.getValues();
    m_profileObject = m_section->CuttingToolWireObject.getValue();

    ui->setupUi(this);

    saveSectionState();
    setUiEdit();

    m_applyDeferred = 0;//setting the direction widgets causes an increment of the deferred count,
                        //so we reset the counter and the message.
    ui->lPendingUpdates->setText(QString());
}

void TaskComplexSection::setUiPrimary()
{
    setWindowTitle(QObject::tr("New Complex Section"));
    if (m_baseView) {
        ui->sbScale->setValue(m_baseView->getScale());
        ui->cmbScaleType->setCurrentIndex(m_baseView->ScaleType.getValue());
    }
    else {
        ui->sbScale->setValue(Preferences::scale());
        ui->cmbScaleType->setCurrentIndex(Preferences::scaleType());
    }
    ui->cmbStrategy->setCurrentIndex(0);

    setUiCommon();

    if (m_baseView) {
        ui->leBaseView->setText(Base::Tools::fromStdString(m_baseView->getNameInDocument()));
        //if there is a baseView, we don't know the sectionNormal yet and have to wait until
        //one is picked in the dialog
        Base::Vector3d defaultNormal(-1.0, 0.0, 0.0);
        m_saveNormal = defaultNormal;
        m_saveXDir = Base::Vector3d(0.0, 1.0, 0.0);
        ui->leBaseView->setText(Base::Tools::fromStdString(m_baseView->getNameInDocument()));
        m_compass->setDialAngle(0.0);
        m_viewDirectionWidget->setValueNoNotify(Base::Vector3d(1.0, 0.0, 0.0));
    }
    else {
        //if there is no baseView, we use the 3d view to determine the SectionNormal
        //and XDirection.
        std::pair<Base::Vector3d, Base::Vector3d> dirs = DrawGuiUtil::get3DDirAndRot();
        m_saveNormal = dirs.first;
        m_saveXDir = dirs.second;
        m_viewDirectionWidget->setValue(m_saveNormal * -1.0);//this will propagate to m_compass
    }

    //don't allow updates until a direction is picked
    ui->pbUpdateNow->setEnabled(false);
    ui->cbLiveUpdate->setEnabled(false);
    ui->lPendingUpdates->setText(tr("No direction set"));
}

void TaskComplexSection::setUiEdit()
{
    setWindowTitle(QObject::tr("Edit Complex Section"));

    if (m_baseView) {
        ui->leBaseView->setText(Base::Tools::fromStdString(m_baseView->getNameInDocument()));
    }
    ui->cmbStrategy->setCurrentIndex(m_section->ProjectionStrategy.getValue());
    ui->leSymbol->setText(Base::Tools::fromStdString(m_section->SectionSymbol.getValue()));
    ui->sbScale->setValue(m_section->Scale.getValue());
    ui->cmbScaleType->setCurrentIndex(m_section->ScaleType.getValue());

    setUiCommon();

    Base::Vector3d sectionNormalVec = m_section->SectionNormal.getValue();
    if (m_baseView) {
        ui->leBaseView->setText(Base::Tools::fromStdString(m_baseView->getNameInDocument()));
        Base::Vector3d projectedViewDirection = m_baseView->projectPoint(sectionNormalVec, false);
        double viewAngle = atan2(-projectedViewDirection.y, -projectedViewDirection.x);
        m_compass->setDialAngle(viewAngle * 180.0 / M_PI);
        m_viewDirectionWidget->setValueNoNotify(projectedViewDirection * -1.0);
    }
    else {
        //no local angle makes sense if there is no baseView?
        m_viewDirectionWidget->setValue(sectionNormalVec * -1.0);
    }
}

void TaskComplexSection::setUiCommon()
{
    ui->leSectionObjects->setText(sourcesToString());
    ui->leProfileObject->setText(Base::Tools::fromStdString(m_profileObject->getNameInDocument())
                                 + QString::fromUtf8(" / ")
                                 + Base::Tools::fromStdString(m_profileObject->Label.getValue()));

    m_compass = new CompassWidget(this);
    auto layout = ui->compassLayout;
    layout->addWidget(m_compass);

    m_viewDirectionWidget = new VectorEditWidget(this);
    m_viewDirectionWidget->setLabel(QObject::tr("Current View Direction"));
    m_viewDirectionWidget->setToolTip(QObject::tr("The view direction in BaseView coordinates"));
    auto editLayout = ui->viewDirectionLayout;
    editLayout->addWidget(m_viewDirectionWidget);


    connect(m_compass, &CompassWidget::angleChanged, this, &TaskComplexSection::slotChangeAngle);

    connect(ui->pbUp, &QPushButton::clicked, this, &TaskComplexSection::onUpClicked);
    connect(ui->pbDown, &QPushButton::clicked, this, &TaskComplexSection::onDownClicked);
    connect(ui->pbRight, &QPushButton::clicked, this, &TaskComplexSection::onRightClicked);
    connect(ui->pbLeft, &QPushButton::clicked, this, &TaskComplexSection::onLeftClicked);

    connect(ui->pbUpdateNow, &QPushButton::clicked, this, &TaskComplexSection::updateNowClicked);
    connect(ui->cbLiveUpdate, &QCheckBox::clicked, this, &TaskComplexSection::liveUpdateClicked);

    connect(ui->pbSectionObjects, &QPushButton::clicked, this,
            &TaskComplexSection::onSectionObjectsUseSelectionClicked);
    connect(ui->pbProfileObject, &QPushButton::clicked, this,
            &TaskComplexSection::onProfileObjectsUseSelectionClicked);

    connect(m_viewDirectionWidget, &VectorEditWidget::valueChanged, this,
            &TaskComplexSection::slotViewDirectionChanged);
}

//save the start conditions
void TaskComplexSection::saveSectionState()
{
    //    Base::Console().Message("TCS::saveSectionState()\n");
    if (m_section) {
        m_saveSymbol = m_section->SectionSymbol.getValue();
        m_saveScale = m_section->getScale();
        m_saveScaleType = m_section->ScaleType.getValue();
        m_saveNormal = m_section->SectionNormal.getValue();
        m_saveDirection = m_section->Direction.getValue();
        m_saveXDir = m_section->XDirection.getValue();
        m_saveOrigin = m_section->SectionOrigin.getValue();
        m_saveDirName = m_section->SectionDirection.getValueAsString();
        m_saved = true;
    }
    if (m_baseView) {
        m_shapes = m_baseView->Source.getValues();
        m_xShapes = m_baseView->XSource.getValues();
    }
}

//restore the start conditions
void TaskComplexSection::restoreSectionState()
{
    //    Base::Console().Message("TCS::restoreSectionState()\n");
    if (!m_section)
        return;

    m_section->SectionSymbol.setValue(m_saveSymbol);
    m_section->Scale.setValue(m_saveScale);
    m_section->ScaleType.setValue(m_saveScaleType);
    m_section->SectionNormal.setValue(m_saveNormal);
    m_section->Direction.setValue(m_saveDirection);
    m_section->XDirection.setValue(m_saveXDir);
    m_section->SectionOrigin.setValue(m_saveOrigin);
    m_section->SectionDirection.setValue(m_saveDirName.c_str());
}

void TaskComplexSection::onSectionObjectsUseSelectionClicked()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    std::vector<App::DocumentObject*> newSelection;
    std::vector<App::DocumentObject*> newXSelection;
    for (auto& sel : selection) {
        if (sel.getObject()->isDerivedFrom(App::LinkElement::getClassTypeId())
            || sel.getObject()->isDerivedFrom(App::LinkGroup::getClassTypeId())
            || sel.getObject()->isDerivedFrom(App::Link::getClassTypeId())) {
            newXSelection.push_back(sel.getObject());
        }
        else {
            newSelection.push_back(sel.getObject());
        }
    }
    m_shapes = newSelection;
    m_xShapes = newXSelection;
    ui->leSectionObjects->setText(sourcesToString());
}

//the VectorEditWidget reports a change in direction
void TaskComplexSection::slotViewDirectionChanged(Base::Vector3d newDirection)
{
    //    Base::Console().Message("TCS::slotViewDirectionChanged(%s)\n",
    //                            DrawUtil::formatVector(newDirection).c_str());
    Base::Vector3d projectedViewDirection = newDirection;
    if (m_baseView) {
        projectedViewDirection = m_baseView->projectPoint(newDirection, false);
    }
    projectedViewDirection.Normalize();
    double viewAngle = atan2(projectedViewDirection.y, projectedViewDirection.x);
    m_compass->setDialAngle(viewAngle * 180.0 / M_PI);
    checkAll(false);
    applyAligned();
}

//the CompassWidget reports the view direction.  This is the reverse of the
//SectionNormal
void TaskComplexSection::slotChangeAngle(double newAngle)
{
    //    Base::Console().Message("TCS::slotAngleChanged(%.3f)\n", newAngle);
    double angleRadians = newAngle * M_PI / 180.0;
    double unitX = cos(angleRadians);
    double unitY = sin(angleRadians);
    Base::Vector3d localUnit(unitX, unitY, 0.0);
    m_viewDirectionWidget->setValueNoNotify(localUnit);
    checkAll(false);
    applyAligned();
}

void TaskComplexSection::onUpClicked()
{
    //    Base::Console().Message("TCS::onUpClicked()\n");
    checkAll(false);
    m_compass->setToNorth();
    m_viewDirectionWidget->setValueNoNotify(Base::Vector3d(0.0, 1.0, 0.0));
    applyAligned();
}

void TaskComplexSection::onDownClicked()
{
    //    Base::Console().Message("TCS::onDownClicked()\n");
    checkAll(false);
    m_compass->setToSouth();
    m_viewDirectionWidget->setValueNoNotify(Base::Vector3d(0.0, -1.0, 0.0));
    applyAligned();
}

void TaskComplexSection::onLeftClicked()
{
    //    Base::Console().Message("TCS::onLeftClicked()\n");
    checkAll(false);
    m_compass->setToWest();
    m_viewDirectionWidget->setValueNoNotify(Base::Vector3d(-1.0, 0.0, 0.0));
    applyAligned();
}

void TaskComplexSection::onRightClicked()
{
    //    Base::Console().Message("TCS::onRightClicked()\n");
    checkAll(false);
    m_compass->setToEast();
    m_viewDirectionWidget->setValueNoNotify(Base::Vector3d(1.0, 0.0, 0.0));
    applyAligned();
}

void TaskComplexSection::onIdentifierChanged()
{
    checkAll(false);
    apply();
}

void TaskComplexSection::onScaleChanged()
{
    checkAll(false);
    apply();
}

void TaskComplexSection::onProfileObjectsUseSelectionClicked()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    //check for single selection and ability to make profile from selected object
    if (!selection.empty()) {
        m_profileObject = selection.front().getObject();
        ui->leProfileObject->setText(
            Base::Tools::fromStdString(m_profileObject->getNameInDocument())
            + QString::fromUtf8(" / ")
            + Base::Tools::fromStdString(m_profileObject->Label.getValue()));
    }
}
void TaskComplexSection::scaleTypeChanged(int index)
{
    if (index == 0) {
        // Page Scale Type
        ui->sbScale->setEnabled(false);
        if (m_baseView->findParentPage()) {
            ui->sbScale->setValue(m_baseView->findParentPage()->Scale.getValue());
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

void TaskComplexSection::checkAll(bool check)
{
    ui->pbUp->setChecked(check);
    ui->pbDown->setChecked(check);
    ui->pbRight->setChecked(check);
    ui->pbLeft->setChecked(check);
}

void TaskComplexSection::enableAll(bool enable)
{
    ui->leSymbol->setEnabled(enable);
    ui->sbScale->setEnabled(enable);
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

void TaskComplexSection::liveUpdateClicked() { apply(true); }

void TaskComplexSection::updateNowClicked() { apply(true); }

QString TaskComplexSection::sourcesToString()
{
    QString result;
    QString separator(QString::fromUtf8(", "));
    QString currentSeparator;
    if (m_baseView) {
        for (auto& obj : m_baseView->Source.getValues()) {
            result += currentSeparator + Base::Tools::fromStdString(obj->getNameInDocument())
                + QString::fromUtf8(" / ") + Base::Tools::fromStdString(obj->Label.getValue());
            currentSeparator = separator;
        }
        currentSeparator = QString();
        for (auto& obj : m_baseView->XSource.getValues()) {
            result += currentSeparator + Base::Tools::fromStdString(obj->getNameInDocument())
                + QString::fromUtf8(" / ") + Base::Tools::fromStdString(obj->Label.getValue());
        }
    }
    else {
        for (auto& obj : m_shapes) {
            result += currentSeparator + Base::Tools::fromStdString(obj->getNameInDocument())
                + QString::fromUtf8(" / ") + Base::Tools::fromStdString(obj->Label.getValue());
        }
        currentSeparator = QString();
        for (auto& obj : m_xShapes) {
            result += currentSeparator + Base::Tools::fromStdString(obj->getNameInDocument())
                + QString::fromUtf8(" / ") + Base::Tools::fromStdString(obj->Label.getValue());
        }
    }
    return result;
}

//******************************************************************************
bool TaskComplexSection::apply(bool forceUpdate)
{
    //    Base::Console().Message("TCS::apply() - liveUpdate: %d force: %d\n",
    //                            ui->cbLiveUpdate->isChecked(), forceUpdate);
    if (!ui->cbLiveUpdate->isChecked() && !forceUpdate) {
        //nothing to do
        m_applyDeferred++;
        QString msgLiteral =
            QString::fromUtf8(QT_TRANSLATE_NOOP("TaskPojGroup", " updates pending"));
        QString msgNumber = QString::number(m_applyDeferred);
        ui->lPendingUpdates->setText(msgNumber + msgLiteral);
        return false;
    }

    Base::Vector3d localUnit = m_viewDirectionWidget->value();
    if (m_baseView) {
        if (!DrawComplexSection::canBuild(m_baseView->localVectorToCS(localUnit),
                                          m_profileObject)) {
            Base::Console().Error(
                "Can not build Complex Section with this profile and direction (1)\n");
            return false;
        }
    }
    else {
        gp_Pnt stdOrigin(0.0, 0.0, 0.0);
        gp_Ax2 sectionCS(stdOrigin, DrawUtil::togp_Dir(m_saveNormal),
                         DrawUtil::togp_Dir(m_saveXDir));
        if (!DrawComplexSection::canBuild(sectionCS, m_profileObject)) {
            Base::Console().Error(
                "Can not build Complex Section with this profile and direction (2)\n");
            return false;
        }
    }

    Gui::WaitCursor wc;
    m_modelIsDirty = true;

    if (!m_section) {
        createComplexSection();
    }

    if (isSectionValid()) {
        updateComplexSection();
    }
    else {
        failNoObject();
    }

    m_section->recomputeFeature();
    if (isBaseValid()) {
        m_baseView->requestPaint();
    }

    enableAll(true);
    checkAll(false);

    wc.restoreCursor();
    m_applyDeferred = 0;
    ui->lPendingUpdates->setText(QString());
    return true;
}

void TaskComplexSection::applyAligned()
{
    //    Base::Console().Message("TCS::applyAligned()\n");
    m_dirName = "Aligned";
    enableAll(true);
    m_directionIsSet = true;
    ui->pbUpdateNow->setEnabled(true);
    ui->cbLiveUpdate->setEnabled(true);
    apply();
}

//*******************************************************************

//pointer to created view is not returned, but stored in m_section
void TaskComplexSection::createComplexSection()
{
    //    Base::Console().Message("TCS::createComplexSection()\n");

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create ComplexSection"));
    if (!m_section) {
        const std::string objectName{QT_TR_NOOP("ComplexSection")};
        m_sectionName = m_page->getDocument()->getUniqueObjectName(objectName.c_str());
        Command::doCommand(Command::Doc, "App.ActiveDocument.addObject('TechDraw::DrawComplexSection', '%s')",
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
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.addView(App.ActiveDocument.%s)",
                           m_page->getNameInDocument(), m_sectionName.c_str());

        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Scale = %0.6f",
                           m_sectionName.c_str(), ui->sbScale->value());
        int scaleType = ui->cmbScaleType->currentIndex();
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.ScaleType = %d",
                           m_sectionName.c_str(), scaleType);
        int projectionStrategy = ui->cmbStrategy->currentIndex();
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.ProjectionStrategy = %d",
                           m_sectionName.c_str(), projectionStrategy);

        Command::doCommand(Command::Doc,
                           "App.activeDocument().%s.SectionOrigin = FreeCAD.Vector(0.0, 0.0, 0.0)",
                           m_sectionName.c_str());
        Command::doCommand(Command::Doc, "App.activeDocument().%s.SectionDirection = 'Aligned'",
                           m_sectionName.c_str());

        App::DocumentObject* newObj = m_page->getDocument()->getObject(m_sectionName.c_str());
        m_section = dynamic_cast<TechDraw::DrawComplexSection*>(newObj);
        if (!newObj || !m_section) {
            throw Base::RuntimeError("TaskComplexSection - new section object not found");
        }
        Base::Vector3d localUnit = m_viewDirectionWidget->value();
        if (m_baseView) {
            Command::doCommand(Command::Doc,
                               "App.ActiveDocument.%s.BaseView = App.ActiveDocument.%s",
                               m_sectionName.c_str(), m_baseView->getNameInDocument());
            m_section->setCSFromBase(localUnit * -1.0);
            m_section->Source.setValues(m_baseView->Source.getValues());
            m_section->XSource.setValues(m_baseView->XSource.getValues());
        }
        else {
            if (m_directionIsSet) {
                //if we have changed the direction, use the local unit to create a CS
                m_section->setCSFromLocalUnit(localUnit * -1.0);
            }
            else {
                //if we have not changed the direction, we should use the 3d directions saved in the
                //constructor
                m_section->SectionNormal.setValue(m_saveNormal);
                m_section->XDirection.setValue(m_saveXDir);
            }
            m_section->Source.setValues(m_shapes);
            m_section->XSource.setValues(m_xShapes);
        }
        m_section->CuttingToolWireObject.setValue(m_profileObject);
        m_section->SectionDirection.setValue("Aligned");
        m_section->Source.setValues(m_shapes);
        m_section->XSource.setValues(m_xShapes);

        //auto orientation of view relative to base view
        double viewDirectionAngle = m_compass->positiveValue();
        double rotation = requiredRotation(viewDirectionAngle);
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Rotation = %.6f",
                           m_sectionName.c_str(), rotation);

    }
    Gui::Command::commitCommand();
}

void TaskComplexSection::updateComplexSection()
{
    //    Base::Console().Message("TCS:;updateComplexSection()\n");
    if (!isSectionValid()) {
        failNoObject();
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Edit SectionView"));
    if (m_section) {
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
                           m_sectionName.c_str(), ui->sbScale->value());
        int scaleType = ui->cmbScaleType->currentIndex();
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.ScaleType = %d",
                           m_sectionName.c_str(), scaleType);
        int projectionStrategy = ui->cmbStrategy->currentIndex();
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.ProjectionStrategy = %d",
                           m_sectionName.c_str(), projectionStrategy);
        Command::doCommand(Command::Doc, "App.activeDocument().%s.SectionDirection = 'Aligned'",
                           m_sectionName.c_str());
        m_section->CuttingToolWireObject.setValue(m_profileObject);
        m_section->SectionDirection.setValue("Aligned");
        Base::Vector3d localUnit = m_viewDirectionWidget->value();
        m_section->setCSFromBase(localUnit * -1.0);
        if (m_baseView) {
            m_section->Source.setValues(m_baseView->Source.getValues());
            m_section->XSource.setValues(m_baseView->XSource.getValues());
        }
        else {
            //without a baseView, our choice of SectionNormal and XDirection may well be wrong
            m_section->Source.setValues(m_shapes);
            m_section->XSource.setValues(m_xShapes);
        }

        //auto orientation of view relative to base view
        double viewDirectionAngle = m_compass->positiveValue();
        double rotation = requiredRotation(viewDirectionAngle);
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Rotation = %.6f",
                           m_sectionName.c_str(), rotation);
    }
    Gui::Command::commitCommand();
}

std::string TaskComplexSection::makeSectionLabel(QString symbol)
{
    const std::string objectName{QT_TR_NOOP("ComplexSection")};
    std::string uniqueSuffix{m_sectionName.substr(objectName.length(), std::string::npos)};
    std::string uniqueLabel = "Section" + uniqueSuffix;
    std::string temp = Base::Tools::toStdString(symbol);
    return ( uniqueLabel + " " + temp + " - " + temp );
}

void TaskComplexSection::failNoObject(void)
{
    QString qsectionName = Base::Tools::fromStdString(m_sectionName);
    QString qbaseName = Base::Tools::fromStdString(m_saveBaseName);
    QString msg = tr("Can not continue. Object * %1 or %2 not found.").arg(qsectionName, qbaseName);
    QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Operation Failed"), msg);
    Gui::Control().closeDialog();
}

bool TaskComplexSection::isBaseValid()
{
    if (!m_baseView)
        return false;

    App::DocumentObject* baseObj = m_doc->getObject(m_saveBaseName.c_str());
    if (!baseObj)
        return false;

    return true;
}

bool TaskComplexSection::isSectionValid()
{
    if (!m_section)
        return false;

    App::DocumentObject* sectionObj = m_doc->getObject(m_sectionName.c_str());
    if (!sectionObj)
        return false;

    return true;
}

//get required rotation from input angle in [0, 360]
//NOTE: shared code with simple section - reuse opportunity
double TaskComplexSection::requiredRotation(double inputAngle)
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
bool TaskComplexSection::accept()
{
    //    Base::Console().Message("TCS::accept()\n");
    apply(true);
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    return true;
}

bool TaskComplexSection::reject()
{
    if (!m_section) {//no section created, nothing to undo
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
        return false;
    }

    if (!isSectionValid()) {//section !exist. nothing to undo
        if (isBaseValid()) {
            m_baseView->requestPaint();
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
        m_baseView->requestPaint();
    }
    Gui::Command::updateActive();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return false;
}

void TaskComplexSection::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgComplexSection::TaskDlgComplexSection(TechDraw::DrawPage* page,
                                             TechDraw::DrawViewPart* baseView,
                                             std::vector<App::DocumentObject*> shapes,
                                             std::vector<App::DocumentObject*> xShapes,
                                             App::DocumentObject* profileObject,
                                             std::vector<std::string> profileSubs)
    : TaskDialog()
{
    widget = new TaskComplexSection(page, baseView, shapes, xShapes, profileObject, profileSubs);
    taskbox =
        new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_ComplexSection"),
                                   widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgComplexSection::TaskDlgComplexSection(TechDraw::DrawComplexSection* complexSection)
    : TaskDialog()
{
    widget = new TaskComplexSection(complexSection);
    taskbox =
        new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_ComplexSection"),
                                   widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgComplexSection::~TaskDlgComplexSection() {}

void TaskDlgComplexSection::update()
{
    //    widget->updateTask();
}

//==== calls from the TaskView ===============================================================
void TaskDlgComplexSection::open() {}

bool TaskDlgComplexSection::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgComplexSection::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskComplexSection.cpp>
