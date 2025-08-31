/***************************************************************************
 *   Copyright (c) 2020 FreeCAD Developers                                 *
 *   Author: Uwe Stöhr <uwestoehr@lyx.org>                                 *
 *   Based on src/Mod/FEM/Gui/DlgSettingsFEMImp.cpp                        *
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
# include <vector>
#endif

#include <Base/Tools.h>

#include <Mod/TechDraw/App/LineGroup.h>
#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/App/LineGenerator.h>

#include "DlgPrefsTechDrawAnnotationImp.h"
#include "ui_DlgPrefsTechDrawAnnotation.h"
#include "DrawGuiUtil.h"

using namespace TechDrawGui;
using namespace TechDraw;


DlgPrefsTechDrawAnnotationImp::DlgPrefsTechDrawAnnotationImp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgPrefsTechDrawAnnotationImp)
{
    ui->setupUi(this);
    ui->pdsbBalloonKink->setUnit(Base::Unit::Length);
    ui->pdsbBalloonKink->setMinimum(0);

    // stylesheet override to defeat behaviour of non-editable combobox to ignore
    // maxVisibleItems property
    QString ssOverride = QStringLiteral("combobox-popup: 0;");
    ui->pcbSectionStyle->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->pcbSectionStyle->setStyleSheet(ssOverride);
    ui->pcbCenterStyle->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->pcbCenterStyle->setStyleSheet(ssOverride);
    ui->pcbHighlightStyle->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->pcbHighlightStyle->setStyleSheet(ssOverride);
    ui->pcbHiddenStyle->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->pcbHiddenStyle->setStyleSheet(ssOverride);

    // connect the LineGroup the update the tooltip if index changed
    connect(ui->pcbLineGroup,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &DlgPrefsTechDrawAnnotationImp::onLineGroupChanged);

    // NOTE that we block onChanged processing while loading the Line Standard combobox
    connect(ui->pcbLineStandard,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &DlgPrefsTechDrawAnnotationImp::onLineStandardChanged);

    m_lineGenerator = new LineGenerator();
}

DlgPrefsTechDrawAnnotationImp::~DlgPrefsTechDrawAnnotationImp()
{
    // no need to delete child widgets, Qt does it all for us
    delete m_lineGenerator;
}

void DlgPrefsTechDrawAnnotationImp::saveSettings()
{
    ui->cbAutoHoriz->onSave();
    ui->cbPrintCenterMarks->onSave();
    ui->cbPyramidOrtho->onSave();
    ui->cbComplexMarks->onSave();
    ui->cbShowCenterMarks->onSave();
    ui->pcbBalloonArrow->onSave();
    ui->pcbBalloonShape->onSave();
    ui->pcbMatting->onSave();
    ui->pdsbBalloonKink->onSave();
    ui->cbCutSurface->onSave();
    ui->cb_ShowSectionLine->onSave();
    ui->cb_IncludeCutLine->onSave();


    // don't save invalid parameter values
    // the comboboxes are properly loaded.
    if (ui->pcbLineGroup->currentIndex() >= 0) {
        ui->pcbLineGroup->onSave();
    }
    if (ui->pcbLineStandard->currentIndex() >= 0) {
        ui->pcbLineStandard->onSave();
    }
    if (ui->pcbSectionStyle->currentIndex() >= 0) {
        ui->pcbSectionStyle->onSave();
    }
    if (ui->pcbCenterStyle->currentIndex() >= 0) {
        ui->pcbCenterStyle->onSave();
    }
    if (ui->pcbHighlightStyle->currentIndex() >= 0) {
        ui->pcbHighlightStyle->onSave();
    }
    if (ui->cbEndCap->currentIndex() >= 0) {
        ui->cbEndCap->onSave();
    }
    if (ui->pcbHiddenStyle->currentIndex() >= 0) {
        ui->pcbHiddenStyle->onSave();
    }

    ui->pcbDetailMatting->onSave();
    ui->pcbDetailHighlight->onSave();

    ui->pcbBreakType->onSave();
    ui->pcbBreakStyle->onSave();
    ui->cbISODates->onSave();
}

void DlgPrefsTechDrawAnnotationImp::loadSettings()
{
    // set defaults for Quantity widgets if property not found
    // Quantity widgets do not use preset value since they are based on
    // QAbstractSpinBox
    double kinkDefault = 5.0;
    ui->pdsbBalloonKink->setValue(kinkDefault);

    ui->cbAutoHoriz->onRestore();
    ui->cbPrintCenterMarks->onRestore();
    ui->cbPyramidOrtho->onRestore();
    ui->cbComplexMarks->onRestore();
    ui->cbShowCenterMarks->onRestore();

    ui->pdsbBalloonKink->onRestore();
    ui->cbCutSurface->onRestore();
    ui->pcbDetailMatting->onRestore();
    ui->pcbDetailHighlight->onRestore();

    ui->cb_ShowSectionLine->onRestore();
    ui->cb_IncludeCutLine->onRestore();

    ui->pcbLineGroup->onRestore();
    DrawGuiUtil::loadLineGroupChoices(ui->pcbLineGroup);
    ui->pcbLineGroup->setCurrentIndex(Preferences::lineGroup());

    ui->pcbMatting->onRestore();
    DrawGuiUtil::loadMattingStyleBox(ui->pcbMatting);
    ui->pcbMatting->setCurrentIndex(prefMattingStyle());

    ui->pcbBalloonShape->onRestore();
    DrawGuiUtil::loadBalloonShapeBox(ui->pcbBalloonShape);
    ui->pcbBalloonShape->setCurrentIndex(prefBalloonShape());

    ui->pcbBalloonArrow->onRestore();
    DrawGuiUtil::loadArrowBox(ui->pcbBalloonArrow);
    ui->pcbBalloonArrow->setCurrentIndex(static_cast<int>(prefBalloonArrow()));

    ui->cbEndCap->onRestore();

    // prevent onChanged processing while loading comboBox
    ui->pcbLineStandard->blockSignals(true);
    DrawGuiUtil::loadLineStandardsChoices(ui->pcbLineStandard);
    ui->pcbLineStandard->blockSignals(false);

    if (ui->pcbLineStandard->count() > Preferences::lineStandard()) {
        ui->pcbLineStandard->setCurrentIndex(Preferences::lineStandard());
    }

    ui->pcbSectionStyle->onRestore();
    ui->pcbCenterStyle->onRestore();
    ui->pcbHighlightStyle->onRestore();
    ui->pcbHiddenStyle->onRestore();
    ui->pcbBreakStyle->onRestore();
    loadLineStyleBoxes();

    ui->pcbBreakType->onRestore();
    ui->cbISODates->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawAnnotationImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->pcbLineStandard->blockSignals(true);
        int currentIndex = ui->pcbLineStandard->currentIndex();
        ui->retranslateUi(this);
        ui->pcbLineStandard->setCurrentIndex(currentIndex);
        ui->pcbLineStandard->blockSignals(false);
    }
    else {
        QWidget::changeEvent(e);
    }
}

TechDraw::ArrowType DlgPrefsTechDrawAnnotationImp::prefBalloonArrow() const
{
    return Preferences::balloonArrow();
}

int DlgPrefsTechDrawAnnotationImp::prefBalloonShape() const
{
    return Preferences::balloonShape();
}

int DlgPrefsTechDrawAnnotationImp::prefMattingStyle() const
{
    return Preferences::mattingStyle();
}

/**
 * Updates the tooltip of the LineGroup combobox
 */
void DlgPrefsTechDrawAnnotationImp::onLineGroupChanged(int index)
{
    if (index == -1) { // there is no valid index yet
        ui->pcbLineGroup->setToolTip(QObject::tr("Select a line group"));
        return;
    }
    // get the definition of the selected LineGroup (includes the name)
    std::string lgRecord = LineGroup::getRecordFromFile(Preferences::lineGroupFile(), index);
    std::stringstream ss(lgRecord);
    std::vector<std::string> lgNames;
    while (std::getline(ss, lgRecord, ',')) {
        lgNames.push_back(lgRecord);
    }
    ui->pcbLineGroup->setToolTip(
        QObject::tr("%1 defines these line widths:\n thin: %2\n graphic: %3\n"
                    " thick: %4")
            .arg(QString::fromStdString(lgNames.at(0).substr(1)),
                 QString::fromStdString(lgNames.at(1)),
                 QString::fromStdString(lgNames.at(2)),
                 QString::fromStdString(lgNames.at(3))));
}

//! we must set the current line group preference when it changes so that the
//! line style comboboxes are filled for the correct standard.
void DlgPrefsTechDrawAnnotationImp::onLineStandardChanged(int index)
{
    if (index < 0) {
        // do not process invalid index
        return;
    }
    Preferences::setLineStandard(index);
    m_lineGenerator->reloadDescriptions();
    loadLineStyleBoxes();
}

//! fill the various line style comboboxes
void DlgPrefsTechDrawAnnotationImp::loadLineStyleBoxes()
{
    // note: line numbering starts at 1, not 0.  we set the preference to the
    // currentIndex in saveSettings, Preferences returns the actual line number,
    // so we need to subtract 1 here to get the index.
    DrawGuiUtil::loadLineStyleChoices(ui->pcbSectionStyle, m_lineGenerator);
    if (ui->pcbSectionStyle->count() > Preferences::SectionLineStyle()) {
        ui->pcbSectionStyle->setCurrentIndex(Preferences::SectionLineStyle() - 1);
    }

    DrawGuiUtil::loadLineStyleChoices(ui->pcbCenterStyle, m_lineGenerator);
    if (ui->pcbCenterStyle->count() > Preferences::CenterLineStyle()) {
        ui->pcbCenterStyle->setCurrentIndex(Preferences::CenterLineStyle() - 1);
    }

    DrawGuiUtil::loadLineStyleChoices(ui->pcbHighlightStyle, m_lineGenerator);
    if (ui->pcbHighlightStyle->count() > Preferences::HighlightLineStyle()) {
        ui->pcbHighlightStyle->setCurrentIndex(Preferences::HighlightLineStyle() - 1);
    }

    DrawGuiUtil::loadLineStyleChoices(ui->pcbHiddenStyle, m_lineGenerator);
    if (ui->pcbHiddenStyle->count() > Preferences::HiddenLineStyle()) {
        ui->pcbHiddenStyle->setCurrentIndex(Preferences::HiddenLineStyle() - 1);
    }

    DrawGuiUtil::loadLineStyleChoices(ui->pcbBreakStyle, m_lineGenerator);
    if (ui->pcbBreakStyle->count() > Preferences::BreakLineStyle()) {
        ui->pcbBreakStyle->setCurrentIndex(Preferences::BreakLineStyle() - 1);
    }
}

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawAnnotationImp.cpp>
