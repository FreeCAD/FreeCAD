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

#include <App/Application.h>

#include <Base/Parameter.h>
#include <Base/Console.h>

#include "DrawGuiUtil.h"
#include "PreferencesGui.h"
#include "DlgPrefsTechDrawAnnotationImp.h"
#include "ui_DlgPrefsTechDrawAnnotation.h"
#include <Mod/TechDraw/App/LineGroup.h>


using namespace TechDrawGui;
using namespace TechDraw;


DlgPrefsTechDrawAnnotationImp::DlgPrefsTechDrawAnnotationImp( QWidget* parent )
  : PreferencePage( parent )
  , ui(new Ui_DlgPrefsTechDrawAnnotationImp)
{
    ui->setupUi(this);
    ui->pdsbBalloonKink->setUnit(Base::Unit::Length);
    ui->pdsbBalloonKink->setMinimum(0);
    
    // connect the LineGroup the update the tooltip if index changed
    connect(ui->pcbLineGroup, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onLineGroupChanged(int)));
}

DlgPrefsTechDrawAnnotationImp::~DlgPrefsTechDrawAnnotationImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawAnnotationImp::saveSettings()
{
    ui->cbAutoHoriz->onSave();
    ui->cbPrintCenterMarks->onSave();
    ui->cbPyramidOrtho->onSave();
    ui->cbSectionLineStd->onSave();
    ui->cbShowCenterMarks->onSave();
    ui->pcbLineGroup->onSave();
    ui->pcbBalloonArrow->onSave();
    ui->pcbBalloonShape->onSave();
    ui->pcbCenterStyle->onSave();
    ui->pcbMatting->onSave();
    ui->pcbSectionStyle->onSave();
    ui->pdsbBalloonKink->onSave();
    ui->cbCutSurface->onSave();
    ui->pcbHighlightStyle->onSave();
}

void DlgPrefsTechDrawAnnotationImp::loadSettings()
{
    //set defaults for Quantity widgets if property not found
    //Quantity widgets do not use preset value since they are based on
    //QAbstractSpinBox
    double kinkDefault = 5.0;
    ui->pdsbBalloonKink->setValue(kinkDefault);
    // re-read the available LineGroup files
    ui->pcbLineGroup->clear();
    std::string lgFileName = Preferences::lineGroupFile();
    std::string lgRecord = LineGroup::getGroupNamesFromFile(lgFileName);
    // split collected groups
    std::stringstream ss(lgRecord);
    std::vector<std::string> lgNames;
    while (std::getline(ss, lgRecord, ',')) {
        lgNames.push_back(lgRecord);
    }
    // fill the combobox with the found names
    for (auto it = lgNames.begin(); it < lgNames.end(); ++it) {
        ui->pcbLineGroup->addItem(tr((*it).c_str()));
    }
 
    ui->cbAutoHoriz->onRestore();
    ui->cbPrintCenterMarks->onRestore();
    ui->cbPyramidOrtho->onRestore();
    ui->cbSectionLineStd->onRestore();
    ui->cbShowCenterMarks->onRestore();
    ui->pcbLineGroup->onRestore();
    ui->pcbBalloonArrow->onRestore();
    ui->pcbBalloonShape->onRestore();
    ui->pcbCenterStyle->onRestore();
    ui->pcbMatting->onRestore();
    ui->pcbSectionStyle->onRestore();
    ui->pdsbBalloonKink->onRestore();
    ui->cbCutSurface->onRestore();
    ui->pcbHighlightStyle->onRestore();

    DrawGuiUtil::loadArrowBox(ui->pcbBalloonArrow);
    ui->pcbBalloonArrow->setCurrentIndex(prefBalloonArrow());
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawAnnotationImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        saveSettings();
        ui->retranslateUi(this);
        loadSettings();
    }
    else {
        QWidget::changeEvent(e);
    }
}

int DlgPrefsTechDrawAnnotationImp::prefBalloonArrow(void) const
{
    return Preferences::balloonArrow();
}

/**
 * Updates the tooltip of the LineGroup combobox
 */
void DlgPrefsTechDrawAnnotationImp::onLineGroupChanged(int index)
{
    if (index == -1) { // there is no valid index yet
        ui->pcbLineGroup->setToolTip(QString::fromStdString("Please select a Line Group"));
        return;
    }
    // get the definition the the selected LineGroup (includes the name)
    std::string lgRecord = LineGroup::getRecordFromFile(Preferences::lineGroupFile(), index);
    std::stringstream ss(lgRecord);
    std::vector<std::string> lgNames;
    while (std::getline(ss, lgRecord, ',')) {
        lgNames.push_back(lgRecord);
    }
    // format the tooltip
    std::stringstream TooltipText;
    TooltipText << lgNames.at(0).substr(1) << " defines these line widths:\n"
        << "thin: " << lgNames.at(1) << "\n"
        << "graphic: " << lgNames.at(2) << "\n"
        << "thick: " << lgNames.at(3);
    ui->pcbLineGroup->setToolTip(QString::fromStdString(TooltipText.str()));
}

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawAnnotationImp.cpp>
