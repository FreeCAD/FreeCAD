/***************************************************************************
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <QPainter>
#include <QPixmap>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Gui/Command.h>

#include "SketcherSettings.h"
#include "ui_SketcherSettings.h"
#include "ui_SketcherSettingsColors.h"
#include "ui_SketcherSettingsDisplay.h"
#include "ui_SketcherSettingsGrid.h"


using namespace SketcherGui;

/* TRANSLATOR SketcherGui::SketcherSettings */

SketcherSettings::SketcherSettings(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_SketcherSettings)
{
    ui->setupUi(this);
}

/**
 *  Destroys the object and frees any allocated resources
 */
SketcherSettings::~SketcherSettings()
{
    // no need to delete child widgets, Qt does it all for us
}

void SketcherSettings::saveSettings()
{
    // Sketch editing
    ui->checkBoxAdvancedSolverTaskBox->onSave();
    ui->checkBoxRecalculateInitialSolutionWhileDragging->onSave();
    ui->checkBoxEnableEscape->onSave();
    ui->checkBoxNotifyConstraintSubstitutions->onSave();
    ui->checkBoxAutoRemoveRedundants->onSave();

    enum
    {
        DimensionSingleTool,
        DimensionSeparateTools,
        DimensionBoth
    };

    // Dimensioning constraints mode
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/dimensioning");
    bool singleTool = true;
    bool SeparatedTools = false;
    int index = ui->dimensioningMode->currentIndex();
    switch (index) {
        case DimensionSeparateTools:
            singleTool = false;
            SeparatedTools = true;
            break;
        case DimensionBoth:
            singleTool = true;
            SeparatedTools = true;
            break;
    }
    hGrp->SetBool("SingleDimensioningTool", singleTool);
    hGrp->SetBool("SeparatedDimensioningTools", SeparatedTools);

    ui->radiusDiameterMode->setEnabled(index != 1);

    enum
    {
        DimensionAutoRadiusDiam,
        DimensionDiameter,
        DimensionRadius
    };

    bool Diameter = true;
    bool Radius = true;
    index = ui->radiusDiameterMode->currentIndex();
    switch (index) {
        case DimensionDiameter:
            Diameter = true;
            Radius = false;
            break;
        case DimensionRadius:
            Diameter = false;
            Radius = true;
            break;
    }
    hGrp->SetBool("DimensioningDiameter", Diameter);
    hGrp->SetBool("DimensioningRadius", Radius);
}

void SketcherSettings::loadSettings()
{
    // Sketch editing
    ui->checkBoxAdvancedSolverTaskBox->onRestore();
    ui->checkBoxRecalculateInitialSolutionWhileDragging->onRestore();
    ui->checkBoxEnableEscape->onRestore();
    ui->checkBoxNotifyConstraintSubstitutions->onRestore();
    ui->checkBoxAutoRemoveRedundants->onRestore();

    // Dimensioning constraints mode
    ui->dimensioningMode->clear();
    ui->dimensioningMode->addItem(tr("Single tool"));
    ui->dimensioningMode->addItem(tr("Separated tools"));
    ui->dimensioningMode->addItem(tr("Both"));

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/dimensioning");
    bool singleTool = hGrp->GetBool("SingleDimensioningTool", true);
    bool SeparatedTools = hGrp->GetBool("SeparatedDimensioningTools", false);
    int index = SeparatedTools ? (singleTool ? 2 : 1) : 0;
    ui->dimensioningMode->setCurrentIndex(index);
    connect(ui->dimensioningMode,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &SketcherSettings::dimensioningModeChanged);

    ui->radiusDiameterMode->setEnabled(index != 1);

    // Dimensioning constraints mode
    ui->radiusDiameterMode->clear();
    ui->radiusDiameterMode->addItem(tr("Auto"));
    ui->radiusDiameterMode->addItem(tr("Diameter"));
    ui->radiusDiameterMode->addItem(tr("Radius"));

    bool Diameter = hGrp->GetBool("DimensioningDiameter", true);
    bool Radius = hGrp->GetBool("DimensioningRadius", true);
    index = Diameter ? (Radius ? 0 : 1) : 2;
    ui->radiusDiameterMode->setCurrentIndex(index);
}

void SketcherSettings::dimensioningModeChanged(int index)
{
    ui->radiusDiameterMode->setEnabled(index != 1);
    SketcherSettings::requireRestart();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void SketcherSettings::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

/* TRANSLATOR SketcherGui::SketcherSettingsGrid */

SketcherSettingsGrid::SketcherSettingsGrid(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_SketcherSettingsGrid)
{
    ui->setupUi(this);

    QList<QPair<Qt::PenStyle, int>> styles;
    styles << qMakePair(Qt::SolidLine, 0xffff) << qMakePair(Qt::DashLine, 0x0f0f)
           << qMakePair(Qt::DotLine, 0xaaaa);

    ui->gridLinePattern->setIconSize(QSize(80, 12));
    ui->gridDivLinePattern->setIconSize(QSize(80, 12));
    for (QList<QPair<Qt::PenStyle, int>>::iterator it = styles.begin(); it != styles.end(); ++it) {
        QPixmap px(ui->gridLinePattern->iconSize());
        px.fill(Qt::transparent);
        QBrush brush(Qt::black);
        QPen pen(it->first);
        pen.setBrush(brush);
        pen.setWidth(2);

        QPainter painter(&px);
        painter.setPen(pen);
        double mid = ui->gridLinePattern->iconSize().height() / 2.0;
        painter.drawLine(0, mid, ui->gridLinePattern->iconSize().width(), mid);
        painter.end();

        ui->gridLinePattern->addItem(QIcon(px), QString(), QVariant(it->second));
        ui->gridDivLinePattern->addItem(QIcon(px), QString(), QVariant(it->second));
    }
}

SketcherSettingsGrid::~SketcherSettingsGrid()
{
    // no need to delete child widgets, Qt does it all for us
}

void SketcherSettingsGrid::saveSettings()
{
    ui->checkBoxShowGrid->onSave();
    ui->gridSize->onSave();
    ui->checkBoxGridAuto->onSave();
    ui->gridSizePixelThreshold->onSave();
    ui->gridLineColor->onSave();
    ui->gridDivLineColor->onSave();
    ui->gridLineWidth->onSave();
    ui->gridDivLineWidth->onSave();
    ui->gridNumberSubdivision->onSave();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    QVariant data = ui->gridLinePattern->itemData(ui->gridLinePattern->currentIndex());
    int pattern = data.toInt();
    hGrp->SetInt("GridLinePattern", pattern);

    data = ui->gridDivLinePattern->itemData(ui->gridDivLinePattern->currentIndex());
    pattern = data.toInt();
    hGrp->SetInt("GridDivLinePattern", pattern);
}

void SketcherSettingsGrid::loadSettings()
{
    ui->checkBoxShowGrid->onRestore();
    ui->gridSize->onRestore();
    ui->checkBoxGridAuto->onRestore();
    ui->gridSizePixelThreshold->onRestore();
    ui->gridLineColor->onRestore();
    ui->gridDivLineColor->onRestore();
    ui->gridLineWidth->onRestore();
    ui->gridDivLineWidth->onRestore();
    ui->gridNumberSubdivision->onRestore();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    int pattern = hGrp->GetInt("GridLinePattern", 0x0f0f);
    int index = ui->gridLinePattern->findData(QVariant(pattern));
    if (index < 0) {
        index = 1;
    }
    ui->gridLinePattern->setCurrentIndex(index);
    pattern = hGrp->GetInt("GridDivLinePattern", 0xffff);
    index = ui->gridDivLinePattern->findData(QVariant(pattern));
    if (index < 0) {
        index = 0;
    }
    ui->gridDivLinePattern->setCurrentIndex(index);
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void SketcherSettingsGrid::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

/* TRANSLATOR SketcherGui::SketcherSettingsDisplay */

SketcherSettingsDisplay::SketcherSettingsDisplay(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_SketcherSettingsDisplay)
{
    ui->setupUi(this);

    connect(ui->btnTVApply,
            &QPushButton::clicked,
            this,
            &SketcherSettingsDisplay::onBtnTVApplyClicked);
}

/**
 *  Destroys the object and frees any allocated resources
 */
SketcherSettingsDisplay::~SketcherSettingsDisplay()
{
    // no need to delete child widgets, Qt does it all for us
}

void SketcherSettingsDisplay::saveSettings()
{
    ui->EditSketcherFontSize->onSave();
    ui->viewScalingFactor->onSave();
    ui->SegmentsPerGeometry->onSave();
    ui->dialogOnDistanceConstraint->onSave();
    ui->continueMode->onSave();
    ui->constraintMode->onSave();
    ui->checkBoxHideUnits->onSave();
    ui->checkBoxShowCursorCoords->onSave();
    ui->checkBoxUseSystemDecimals->onSave();
    ui->checkBoxShowDimensionalName->onSave();
    ui->prefDimensionalStringFormat->onSave();
    ui->checkBoxTVHideDependent->onSave();
    ui->checkBoxTVShowLinks->onSave();
    ui->checkBoxTVShowSupport->onSave();
    ui->checkBoxTVRestoreCamera->onSave();
    ui->checkBoxTVForceOrtho->onSave();
    ui->checkBoxTVSectionView->onSave();
}

void SketcherSettingsDisplay::loadSettings()
{
    ui->EditSketcherFontSize->onRestore();
    ui->viewScalingFactor->onRestore();
    ui->SegmentsPerGeometry->onRestore();
    ui->dialogOnDistanceConstraint->onRestore();
    ui->continueMode->onRestore();
    ui->constraintMode->onRestore();
    ui->checkBoxHideUnits->onRestore();
    ui->checkBoxShowCursorCoords->onRestore();
    ui->checkBoxUseSystemDecimals->onRestore();
    ui->checkBoxShowDimensionalName->onRestore();
    ui->prefDimensionalStringFormat->onRestore();
    ui->checkBoxTVHideDependent->onRestore();
    ui->checkBoxTVShowLinks->onRestore();
    ui->checkBoxTVShowSupport->onRestore();
    ui->checkBoxTVRestoreCamera->onRestore();
    ui->checkBoxTVForceOrtho->onRestore();
    this->ui->checkBoxTVForceOrtho->setEnabled(this->ui->checkBoxTVRestoreCamera->isChecked());
    ui->checkBoxTVSectionView->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void SketcherSettingsDisplay::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void SketcherSettingsDisplay::onBtnTVApplyClicked(bool)
{
    QString errMsg;
    try {
        Gui::Command::doCommand(Gui::Command::Gui,
                                "for name,doc in App.listDocuments().items():\n"
                                "    for sketch in doc.findObjects('Sketcher::SketchObject'):\n"
                                "        sketch.ViewObject.HideDependent = %s\n"
                                "        sketch.ViewObject.ShowLinks = %s\n"
                                "        sketch.ViewObject.ShowSupport = %s\n"
                                "        sketch.ViewObject.RestoreCamera = %s\n"
                                "        sketch.ViewObject.ForceOrtho = %s\n"
                                "        sketch.ViewObject.SectionView = %s\n",
                                this->ui->checkBoxTVHideDependent->isChecked() ? "True" : "False",
                                this->ui->checkBoxTVShowLinks->isChecked() ? "True" : "False",
                                this->ui->checkBoxTVShowSupport->isChecked() ? "True" : "False",
                                this->ui->checkBoxTVRestoreCamera->isChecked() ? "True" : "False",
                                this->ui->checkBoxTVForceOrtho->isChecked() ? "True" : "False",
                                this->ui->checkBoxTVSectionView->isChecked() ? "True" : "False");
    }
    catch (Base::PyException& e) {
        Base::Console().DeveloperError("SketcherSettings", "error in onBtnTVApplyClicked:\n");
        e.ReportException();
        errMsg = QString::fromLatin1(e.what());
    }
    catch (...) {
        errMsg = tr("Unexpected C++ exception");
    }
    if (errMsg.length() > 0) {
        QMessageBox::warning(this, tr("Sketcher"), errMsg);
    }
}


/* TRANSLATOR SketcherGui::SketcherSettingsColors */

SketcherSettingsColors::SketcherSettingsColors(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_SketcherSettingsColors)
{
    ui->setupUi(this);
}

/**
 *  Destroys the object and frees any allocated resources
 */
SketcherSettingsColors::~SketcherSettingsColors()
{
    // no need to delete child widgets, Qt does it all for us
}

void SketcherSettingsColors::saveSettings()
{
    // Sketcher
    ui->SketchEdgeColor->onSave();
    ui->SketchVertexColor->onSave();
    ui->EditedEdgeColor->onSave();
    ui->EditedVertexColor->onSave();
    ui->ConstructionColor->onSave();
    ui->ExternalColor->onSave();
    ui->InvalidSketchColor->onSave();
    ui->FullyConstrainedColor->onSave();
    ui->InternalAlignedGeoColor->onSave();
    ui->FullyConstraintElementColor->onSave();
    ui->FullyConstraintConstructionElementColor->onSave();
    ui->FullyConstraintInternalAlignmentColor->onSave();
    ui->FullyConstraintConstructionPointColor->onSave();

    ui->ConstrainedColor->onSave();
    ui->NonDrivingConstraintColor->onSave();
    ui->DatumColor->onSave();
    ui->ExprBasedConstrDimColor->onSave();
    ui->DeactivatedConstrDimColor->onSave();

    ui->CursorTextColor->onSave();
    ui->CursorCrosshairColor->onSave();
    ui->CreateLineColor->onSave();
}

void SketcherSettingsColors::loadSettings()
{
    // Sketcher
    ui->SketchEdgeColor->onRestore();
    ui->SketchVertexColor->onRestore();
    ui->EditedEdgeColor->onRestore();
    ui->EditedVertexColor->onRestore();
    ui->ConstructionColor->onRestore();
    ui->ExternalColor->onRestore();
    ui->InvalidSketchColor->onRestore();
    ui->FullyConstrainedColor->onRestore();
    ui->InternalAlignedGeoColor->onRestore();
    ui->FullyConstraintElementColor->onRestore();
    ui->FullyConstraintConstructionElementColor->onRestore();
    ui->FullyConstraintInternalAlignmentColor->onRestore();
    ui->FullyConstraintConstructionPointColor->onRestore();

    ui->ConstrainedColor->onRestore();
    ui->NonDrivingConstraintColor->onRestore();
    ui->DatumColor->onRestore();
    ui->ExprBasedConstrDimColor->onRestore();
    ui->DeactivatedConstrDimColor->onRestore();

    ui->CursorTextColor->onRestore();
    ui->CursorCrosshairColor->onRestore();
    ui->CreateLineColor->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void SketcherSettingsColors::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_SketcherSettings.cpp"
