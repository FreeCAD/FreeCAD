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
#include "ui_SketcherSettingsAppearance.h"
#include "ui_SketcherSettingsDisplay.h"
#include "ui_SketcherSettingsGrid.h"


using namespace SketcherGui;

/* TRANSLATOR SketcherGui::SketcherSettings */

QList<int> getPenStyles()
{
    QList<int> styles;
    styles << 0b1111111111111111   // solid
           << 0b1110111011101110   // dashed 3:1
           << 0b1111110011111100   // dashed 6:2
           << 0b0000111100001111   // dashed 4:4
           << 0b1010101010101010   // point 1:1
           << 0b1110010011100100   // dash point
           << 0b1111111100111100;  // dash long-dash
    return styles;
}

const QVector<qreal> binaryPatternToDashPattern(int binaryPattern)
{
    QVector<qreal> dashPattern;
    int count = 0;
    bool isDash = (binaryPattern & 0x8000) != 0;  // Check the highest bit

    for (int i = 0; i < 16; ++i) {
        bool currentBit = (binaryPattern & (0x8000 >> i)) != 0;
        if (currentBit == isDash) {
            ++count;  // Counting dashes or spaces
        }
        else {
            // Adjust count to be odd for dashes and even for spaces (see qt doc)
            count = (count % 2 == (isDash ? 0 : 1)) ? count + 1 : count;
            dashPattern << count;
            count = 1;  // Reset count for next dash/space
            isDash = !isDash;
        }
    }
    count = (count % 2 == (isDash ? 0 : 1)) ? count + 1 : count;
    dashPattern << count;  // Add the last count

    if ((dashPattern.size() % 2) == 1) {
        // prevent this error : qWarning("QPen::setDashPattern: Pattern not of even length");
        dashPattern << 1;
    }

    return dashPattern;
}

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
    ui->checkBoxUnifiedCoincident->onSave();
    ui->checkBoxHorVerAuto->onSave();
    ui->checkBoxLineGroup->onSave();
    ui->checkBoxAddExtGeo->onSave();

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

    index = ui->autoScaleMode->currentIndex();
    hGrp->SetInt("AutoScaleMode", index);

    hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/Tools");

    index = ui->ovpVisibility->currentIndex();
    hGrp->SetInt("OnViewParameterVisibility", index);

    checkForRestart();
}

void SketcherSettings::loadSettings()
{
    // Sketch editing
    ui->checkBoxAdvancedSolverTaskBox->onRestore();
    ui->checkBoxRecalculateInitialSolutionWhileDragging->onRestore();
    ui->checkBoxEnableEscape->onRestore();
    ui->checkBoxNotifyConstraintSubstitutions->onRestore();
    ui->checkBoxAutoRemoveRedundants->onRestore();
    ui->checkBoxUnifiedCoincident->onRestore();
    setProperty("checkBoxUnifiedCoincident", ui->checkBoxUnifiedCoincident->isChecked());
    ui->checkBoxHorVerAuto->onRestore();
    setProperty("checkBoxHorVerAuto", ui->checkBoxHorVerAuto->isChecked());
    ui->checkBoxAddExtGeo->onRestore();
    setProperty("checkBoxLineGroup", ui->checkBoxLineGroup->isChecked());
    ui->checkBoxAddExtGeo->onRestore();

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
    setProperty("dimensioningMode", index);
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


    // The items have to be added in the same order
    // as the AutoScaleMode enum
    ui->autoScaleMode->clear();
    ui->autoScaleMode->addItem(tr("Always"));
    ui->autoScaleMode->addItem(tr("Never"));
    ui->autoScaleMode->addItem(tr("When no scale feature is visible"));
    index = hGrp->GetInt("AutoScaleMode", static_cast<int>(AutoScaleMode::Always));
    ui->autoScaleMode->setCurrentIndex(index);

    hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/Tools");
    ui->ovpVisibility->clear();
    ui->ovpVisibility->addItem(tr("None"));
    ui->ovpVisibility->addItem(tr("Dimensions only"));
    ui->ovpVisibility->addItem(tr("Position and dimensions"));

    index = hGrp->GetInt("OnViewParameterVisibility", 1);
    ui->ovpVisibility->setCurrentIndex(index);
}

void SketcherSettings::dimensioningModeChanged(int index)
{
    ui->radiusDiameterMode->setEnabled(index != 1);
}

void SketcherSettings::checkForRestart()
{
    if (property("dimensioningMode").toInt() != ui->dimensioningMode->currentIndex()) {
        SketcherSettings::requireRestart();
    }
    if (property("checkBoxUnifiedCoincident").toBool()
        != ui->checkBoxUnifiedCoincident->isChecked()) {
        SketcherSettings::requireRestart();
    }
    if (property("checkBoxHorVerAuto").toBool() != ui->checkBoxHorVerAuto->isChecked()) {
        SketcherSettings::requireRestart();
    }
    if (property("checkBoxLineGroup").toBool() != ui->checkBoxLineGroup->isChecked()) {
        SketcherSettings::requireRestart();
    }
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

void SketcherSettings::resetSettingsToDefaults()
{
    ParameterGrp::handle hGrp;

    hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/dimensioning");
    // reset "Dimension tools" parameters
    hGrp->RemoveBool("SingleDimensioningTool");
    hGrp->RemoveBool("SeparatedDimensioningTools");

    // reset "radius/diameter mode for dimensioning" parameter
    hGrp->RemoveBool("DimensioningDiameter");
    hGrp->RemoveBool("DimensioningRadius");

    hGrp->RemoveInt("AutoScaleMode");

    hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/Tools");
    // reset "OVP visibility" parameter
    hGrp->RemoveInt("OnViewParameterVisibility");

    // finally reset all the parameters associated to Gui::Pref* widgets
    PreferencePage::resetSettingsToDefaults();
}

/* TRANSLATOR SketcherGui::SketcherSettingsGrid */

SketcherSettingsGrid::SketcherSettingsGrid(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_SketcherSettingsGrid)
{
    ui->setupUi(this);

    QList<int> styles = getPenStyles();

    ui->gridLinePattern->setIconSize(QSize(80, 12));
    ui->gridDivLinePattern->setIconSize(QSize(80, 12));
    for (auto& style : styles) {
        QPixmap px(ui->gridLinePattern->iconSize());
        px.fill(Qt::transparent);
        QBrush brush(Qt::black);

        QPen pen;
        pen.setDashPattern(binaryPatternToDashPattern(style));
        pen.setBrush(brush);
        pen.setWidth(2);

        QPainter painter(&px);
        painter.setPen(pen);
        double mid = ui->gridLinePattern->iconSize().height() / 2.0;
        painter.drawLine(0, mid, ui->gridLinePattern->iconSize().width(), mid);
        painter.end();

        ui->gridLinePattern->addItem(QIcon(px), QString(), QVariant(style));
        ui->gridDivLinePattern->addItem(QIcon(px), QString(), QVariant(style));
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
    int pattern = hGrp->GetInt("GridLinePattern", 0b0000111100001111);
    int index = ui->gridLinePattern->findData(QVariant(pattern));
    if (index < 0) {
        index = 1;
    }
    ui->gridLinePattern->setCurrentIndex(index);
    pattern = hGrp->GetInt("GridDivLinePattern", 0b1111111111111111);
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
        Base::Console().developerError("SketcherSettings", "error in onBtnTVApplyClicked:\n");
        e.reportException();
        errMsg = QString::fromLatin1(e.what());
    }
    catch (...) {
        errMsg = tr("Unexpected C++ exception");
    }
    if (errMsg.length() > 0) {
        QMessageBox::warning(this, tr("Sketcher"), errMsg);
    }
}


/* TRANSLATOR SketcherGui::SketcherSettingsAppearance */

SketcherSettingsAppearance::SketcherSettingsAppearance(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_SketcherSettingsAppearance)
{
    ui->setupUi(this);

    QList<int> styles = getPenStyles();

    ui->EdgePattern->setIconSize(QSize(70, 12));
    ui->ConstructionPattern->setIconSize(QSize(70, 12));
    ui->InternalPattern->setIconSize(QSize(70, 12));
    ui->ExternalPattern->setIconSize(QSize(70, 12));
    ui->ExternalDefiningPattern->setIconSize(QSize(70, 12));
    for (auto& style : styles) {
        QPixmap px(ui->EdgePattern->iconSize());
        px.fill(Qt::transparent);
        QBrush brush(Qt::black);
        QPen pen;
        pen.setDashPattern(binaryPatternToDashPattern(style));
        pen.setBrush(brush);
        pen.setWidth(2);

        QPainter painter(&px);
        painter.setPen(pen);
        double mid = ui->EdgePattern->iconSize().height() / 2.0;
        painter.drawLine(0, mid, ui->EdgePattern->iconSize().width(), mid);
        painter.end();

        ui->EdgePattern->addItem(QIcon(px), QString(), QVariant(style));
        ui->ConstructionPattern->addItem(QIcon(px), QString(), QVariant(style));
        ui->InternalPattern->addItem(QIcon(px), QString(), QVariant(style));
        ui->ExternalPattern->addItem(QIcon(px), QString(), QVariant(style));
        ui->ExternalDefiningPattern->addItem(QIcon(px), QString(), QVariant(style));
    }
}

/**
 *  Destroys the object and frees any allocated resources
 */
SketcherSettingsAppearance::~SketcherSettingsAppearance()
{
    // no need to delete child widgets, Qt does it all for us
}

void SketcherSettingsAppearance::saveSettings()
{
    // Sketcher
    ui->SketchEdgeColor->onSave();
    ui->SketchVertexColor->onSave();
    ui->EditedEdgeColor->onSave();
    ui->ConstructionColor->onSave();
    ui->ExternalColor->onSave();
    ui->ExternalDefiningColor->onSave();
    ui->InvalidSketchColor->onSave();
    ui->FullyConstrainedColor->onSave();
    ui->InternalAlignedGeoColor->onSave();
    ui->FullyConstraintElementColor->onSave();
    ui->FullyConstraintConstructionElementColor->onSave();
    ui->FullyConstraintInternalAlignmentColor->onSave();

    ui->ConstrainedColor->onSave();
    ui->NonDrivingConstraintColor->onSave();
    ui->DatumColor->onSave();
    ui->ExprBasedConstrDimColor->onSave();
    ui->DeactivatedConstrDimColor->onSave();

    ui->CursorTextColor->onSave();
    ui->CursorCrosshairColor->onSave();
    ui->CreateLineColor->onSave();

    ui->EdgeWidth->onSave();
    ui->ConstructionWidth->onSave();
    ui->InternalWidth->onSave();
    ui->ExternalWidth->onSave();
    ui->ExternalDefiningWidth->onSave();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/View");
    QVariant data = ui->EdgePattern->itemData(ui->EdgePattern->currentIndex());
    int pattern = data.toInt();
    hGrp->SetInt("EdgePattern", pattern);

    data = ui->ConstructionPattern->itemData(ui->ConstructionPattern->currentIndex());
    pattern = data.toInt();
    hGrp->SetInt("ConstructionPattern", pattern);

    data = ui->InternalPattern->itemData(ui->InternalPattern->currentIndex());
    pattern = data.toInt();
    hGrp->SetInt("InternalPattern", pattern);

    data = ui->ExternalPattern->itemData(ui->ExternalPattern->currentIndex());
    pattern = data.toInt();
    hGrp->SetInt("ExternalPattern", pattern);

    data = ui->ExternalDefiningPattern->itemData(ui->ExternalDefiningPattern->currentIndex());
    pattern = data.toInt();
    hGrp->SetInt("ExternalDefiningPattern", pattern);
}

void SketcherSettingsAppearance::loadSettings()
{
    // Sketcher
    ui->SketchEdgeColor->onRestore();
    ui->SketchVertexColor->onRestore();
    ui->EditedEdgeColor->onRestore();
    ui->ConstructionColor->onRestore();
    ui->ExternalColor->onRestore();
    ui->ExternalDefiningColor->onRestore();
    ui->InvalidSketchColor->onRestore();
    ui->FullyConstrainedColor->onRestore();
    ui->InternalAlignedGeoColor->onRestore();
    ui->FullyConstraintElementColor->onRestore();
    ui->FullyConstraintConstructionElementColor->onRestore();
    ui->FullyConstraintInternalAlignmentColor->onRestore();

    ui->ConstrainedColor->onRestore();
    ui->NonDrivingConstraintColor->onRestore();
    ui->DatumColor->onRestore();
    ui->ExprBasedConstrDimColor->onRestore();
    ui->DeactivatedConstrDimColor->onRestore();

    ui->CursorTextColor->onRestore();
    ui->CursorCrosshairColor->onRestore();
    ui->CreateLineColor->onRestore();

    ui->EdgeWidth->onRestore();
    ui->ConstructionWidth->onRestore();
    ui->InternalWidth->onRestore();
    ui->ExternalWidth->onRestore();
    ui->ExternalDefiningWidth->onRestore();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/View");
    int pattern = hGrp->GetInt("EdgePattern", 0b1111111111111111);
    int index = ui->EdgePattern->findData(QVariant(pattern));
    if (index < 0) {
        index = 0;
    }
    ui->EdgePattern->setCurrentIndex(index);

    pattern = hGrp->GetInt("ConstructionPattern", 0b1111110011111100);
    index = ui->ConstructionPattern->findData(QVariant(pattern));
    if (index < 0) {
        index = 0;
    }
    ui->ConstructionPattern->setCurrentIndex(index);

    pattern = hGrp->GetInt("InternalPattern", 0b1111110011111100);
    index = ui->InternalPattern->findData(QVariant(pattern));
    if (index < 0) {
        index = 0;
    }
    ui->InternalPattern->setCurrentIndex(index);

    pattern = hGrp->GetInt("ExternalPattern", 0b1111110011111100);
    index = ui->ExternalPattern->findData(QVariant(pattern));
    if (index < 0) {
        index = 0;
    }
    ui->ExternalPattern->setCurrentIndex(index);

    pattern = hGrp->GetInt("ExternalDefiningPattern", 0b1111111111111111);
    index = ui->ExternalDefiningPattern->findData(QVariant(pattern));
    if (index < 0) {
        index = 0;
    }
    ui->ExternalDefiningPattern->setCurrentIndex(index);
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void SketcherSettingsAppearance::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_SketcherSettings.cpp"
