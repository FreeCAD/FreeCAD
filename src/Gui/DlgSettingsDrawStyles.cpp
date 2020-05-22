/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include <array>

#include <App/Material.h>
#include "DlgSettingsDrawStyles.h"
#include "ui_DlgSettingsDrawStyles.h"
#include "ViewParams.h"
#include "Application.h"
#include "Document.h"
#include "View3DInventorViewer.h"
#include "View3DInventor.h"
#include "PrefWidgets.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsDrawStyles */

struct LinePattern {
    Qt::PenStyle style;
    int value;
    const char *name;

    LinePattern(Qt::PenStyle s, int v, const char *n)
        :style(s), value(v), name(n)
    {}
};
static const std::array<LinePattern, 5> _LinePatterns {{
    {Qt::SolidLine, 0, "No change"},
    {Qt::SolidLine, 0xffff, "Solid"},
    {Qt::DashLine, 0xf0f0, "Dash"},
    {Qt::DotLine, 0xaaaa, "Dot"},
    {Qt::DashDotLine, 0x33ff, "Dash Dot"},
}};

/**
 *  Constructs a DlgSettingsDrawStyles which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
DlgSettingsDrawStyles::DlgSettingsDrawStyles(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsDrawStyles)
{
    ui->setupUi(this);
    ui->checkBoxShaded->setChecked(ViewParams::getHiddenLineShaded());
    ui->checkBoxLineColor->setChecked(ViewParams::getHiddenLineOverrideColor());
    ui->checkBoxFaceColor->setChecked(ViewParams::getHiddenLineOverrideFaceColor());
    ui->checkBoxBackground->setChecked(ViewParams::getHiddenLineOverrideBackground());

    ui->LineColor->setEnabled(ui->checkBoxLineColor->isChecked());
    ui->FaceColor->setEnabled(ui->checkBoxFaceColor->isChecked());
    ui->BackgroundColor->setEnabled(ui->checkBoxBackground->isChecked());

    ui->LineColor->setColor(App::Color(
                (uint32_t)ViewParams::getHiddenLineColor()).asValue<QColor>());

    ui->FaceColor->setColor(App::Color(
                (uint32_t)ViewParams::getHiddenLineFaceColor()).asValue<QColor>());

    ui->BackgroundColor->setColor(App::Color(
                (uint32_t)ViewParams::getHiddenLineBackground()).asValue<QColor>());

    ui->spinTransparency->setValue(ViewParams::getHiddenLineTransparency());

    ui->comboLinePattern->setIconSize (QSize(80,12));
    for(auto &v : _LinePatterns) {
        if(!v.value) {
            ui->comboLinePattern->addItem(QString::fromLatin1(v.name),v.value);
            continue;
        }
        QPixmap px(ui->comboLinePattern->iconSize());
        px.fill(Qt::white);
        QBrush brush(Qt::black);
        QPen pen(v.style);
        pen.setBrush(brush);
        pen.setWidth(2);

        QPainter painter(&px);
        painter.setPen(pen);
        double mid = ui->comboLinePattern->iconSize().height() / 2.0;
        painter.drawLine(0, mid, ui->comboLinePattern->iconSize().width(), mid);
        painter.end();

        ui->comboLinePattern->addItem(QIcon(px),QString::fromLatin1(v.name),v.value);
    }
    ui->comboLinePattern->setEditable(true);
    
    int pattern = ViewParams::getSelectionLinePattern();
    bool found = false;
    int i=-1;
    for(auto &v : _LinePatterns) {
        ++i;
        if(pattern == v.value) {
            found = true;
            ui->comboLinePattern->setCurrentIndex(i);
            break;
        }
    }
    if(!found) {
        ui->comboLinePattern->setEditText(
                QString::fromLatin1("0x%1").arg(pattern,0,16));
    }

    ui->checkBoxSelectionOnTop->setChecked(ViewParams::getShowSelectionOnTop());
    ui->spinTransparencyOnTop->setValue(ViewParams::getTransparencyOnTop());
    ui->spinLineWidthMultiplier->setValue(ViewParams::getSelectionLineThicken());
    ui->spinSelectionHiddenLineWidth->setValue(ViewParams::getSelectionHiddenLineWidth());

    ui->checkBoxSpotLight->setChecked(ViewParams::getShadowSpotLight());
    ui->checkBoxShowGround->setChecked(ViewParams::getShadowShowGround());
    ui->checkBoxFlatLines->setChecked(ViewParams::getShadowFlatLines());

    ui->spinBoxLightIntensity->setValue(ViewParams::getShadowLightIntensity());
    ui->spinBoxGroundScale->setValue(ViewParams::getShadowGroundScale());

    ui->LightColor->setColor(App::Color(
                (uint32_t)ViewParams::getShadowLightColor()).asValue<QColor>());

    ui->GroundColor->setColor(App::Color(
                (uint32_t)ViewParams::getShadowGroundColor()).asValue<QColor>());

    ui->spinGroundTransparency->setValue(ViewParams::getShadowGroundTransparency());
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsDrawStyles::~DlgSettingsDrawStyles()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsDrawStyles::saveSettings()
{
    int idx = ui->comboLinePattern->currentIndex();
    if(idx >= 0) {
        ViewParams::setSelectionLinePattern(
                ui->comboLinePattern->itemData(idx).toInt());
    } else {
        QString pattern = ui->comboLinePattern->currentText();
        bool res;
        ViewParams::setSelectionLinePattern(pattern.toInt(&res,0));
        if(!res) {
            Base::Console().Warning("Invalid line pattern: %s'\n",
                    pattern.toLatin1().constData());
        }
    }
    ui->checkBoxSelectionOnTop->onSave();
    ui->spinTransparencyOnTop->onSave();
    ui->spinLineWidthMultiplier->onSave();
    ui->spinSelectionHiddenLineWidth->onSave();

    ui->checkBoxShaded->onSave();
    ui->checkBoxFaceColor->onSave();
    ui->checkBoxLineColor->onSave();
    ui->checkBoxBackground->onSave();
    ui->BackgroundColor->onSave();
    ui->FaceColor->onSave();
    ui->LineColor->onSave();
    ui->spinTransparency->onSave();

    ui->checkBoxSpotLight->onSave();
    ui->checkBoxShowGround->onSave();
    ui->checkBoxFlatLines->onSave();
    ui->spinBoxLightIntensity->onSave();
    ui->spinBoxGroundScale->onSave();
    ui->LightColor->onSave();
    ui->GroundColor->onSave();
    ui->spinGroundTransparency->onSave();

    for(auto doc : App::GetApplication().getDocuments()) {
        for(auto v : Application::Instance->getDocument(doc)->getMDIViews()) {
            View3DInventor* view = qobject_cast<View3DInventor*>(v);
            if(!view)
                continue;
            auto viewer = view->getViewer();
            if(viewer->getOverrideMode() != "As Is")
                viewer->applyOverrideMode();
        }
    }
}

void DlgSettingsDrawStyles::loadSettings()
{
    int pattern = ViewParams::getSelectionLinePattern();
    bool found = false;
    int i=-1;
    for(auto &v : _LinePatterns) {
        ++i;
        if(pattern == v.value) {
            ui->comboLinePattern->setCurrentIndex(i);
            found = true;
            break;
        }
    }
    if(!found)
        ui->comboLinePattern->setEditText(QString::fromLatin1("0x%1").arg(pattern));

    ui->checkBoxSelectionOnTop->onRestore();
    ui->spinTransparencyOnTop->onRestore();
    ui->spinLineWidthMultiplier->onRestore();
    ui->spinSelectionHiddenLineWidth->onRestore();

    ui->checkBoxShaded->onRestore();
    ui->checkBoxFaceColor->onRestore();
    ui->checkBoxLineColor->onRestore();
    ui->checkBoxBackground->onRestore();
    ui->BackgroundColor->onRestore();
    ui->FaceColor->onRestore();
    ui->LineColor->onRestore();
    ui->spinTransparency->onRestore();

    ui->checkBoxSpotLight->onRestore();
    ui->checkBoxShowGround->onRestore();
    ui->checkBoxFlatLines->onRestore();
    ui->spinBoxLightIntensity->onRestore();
    ui->spinBoxGroundScale->onRestore();
    ui->LightColor->onRestore();
    ui->GroundColor->onRestore();
    ui->spinGroundTransparency->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsDrawStyles::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsDrawStyles.cpp"

