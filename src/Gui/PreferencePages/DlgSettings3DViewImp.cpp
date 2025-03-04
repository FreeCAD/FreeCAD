/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QApplication>
# include <QMessageBox>
# include <QOffscreenSurface>
# include <QOpenGLContext>
# include <QSurfaceFormat>
#endif

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <Gui/Multisample.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewParams.h>

#include "DlgSettings3DViewImp.h"
#include "ui_DlgSettings3DView.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettings3DViewImp */

DlgSettings3DViewImp::DlgSettings3DViewImp(QWidget* parent)
    : PreferencePage( parent )
    , ui(new Ui_DlgSettings3DView)
{
    ui->setupUi(this);
    addAntiAliasing();
}

DlgSettings3DViewImp::~DlgSettings3DViewImp() = default;

void DlgSettings3DViewImp::saveSettings()
{
    saveAntiAliasing();
    saveRenderCache();
    saveMarkerSize();

    ui->comboTransparentRender->onSave();
    ui->CheckBox_CornerCoordSystem->onSave();
    ui->SpinBox_CornerCoordSystemSize->onSave();
    ui->CheckBox_ShowAxisCross->onSave();
    ui->CheckBox_ShowFPS->onSave();
    ui->CheckBox_use_SW_OpenGL->onSave();
    ui->CheckBox_useVBO->onSave();
    ui->FloatSpinBox_EyeDistance->onSave();
    ui->axisLetterColor->onSave();
    ui->radioPerspective->onSave();
    ui->radioOrthographic->onSave();
    ui->xAxisColor->onSave();
    ui->yAxisColor->onSave();
    ui->zAxisColor->onSave();
}

void DlgSettings3DViewImp::loadSettings()
{
    ui->CheckBox_CornerCoordSystem->onRestore();
    ui->SpinBox_CornerCoordSystemSize->onRestore();
    ui->CheckBox_ShowAxisCross->onRestore();
    ui->CheckBox_ShowFPS->onRestore();
    ui->CheckBox_use_SW_OpenGL->onRestore();
    ui->CheckBox_useVBO->onRestore();
    ui->FloatSpinBox_EyeDistance->onRestore();
    ui->axisLetterColor->onRestore();
    ui->radioPerspective->onRestore();
    ui->radioOrthographic->onRestore();
    ui->comboTransparentRender->onRestore();
    ui->xAxisColor->onRestore();
    ui->yAxisColor->onRestore();
    ui->zAxisColor->onRestore();

    loadAntiAliasing();
    loadRenderCache();
    loadMarkerSize();
}

void DlgSettings3DViewImp::addAntiAliasing()
{
    ui->comboAliasing->clear();

    // Do the samples checks only once
    static std::vector<std::pair<QString, AntiAliasing>> modes;
    static bool formatCheck = true;
    if (formatCheck) {
        formatCheck = false;

        Multisample check;
        modes = check.supported();
    }

    for (const auto& it : modes) {
        ui->comboAliasing->addItem(it.first, int(it.second));
    }
}

void DlgSettings3DViewImp::saveAntiAliasing()
{
    int index = ui->comboAliasing->currentIndex();
    int aliasing = ui->comboAliasing->itemData(index).toInt();
    Multisample::writeMSAAToSettings(static_cast<AntiAliasing>(aliasing));
}

void DlgSettings3DViewImp::loadAntiAliasing()
{
    int aliasing = int(Multisample::readMSAAFromSettings());
    int index = ui->comboAliasing->findData(aliasing);
    if (index != -1) {
        ui->comboAliasing->setCurrentIndex(index);
    }

    // connect after setting current item of the combo box
    connect(ui->comboAliasing, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DlgSettings3DViewImp::onAliasingChanged);
}

void DlgSettings3DViewImp::saveRenderCache()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");

    int cache = ui->renderCache->currentIndex();
    hGrp->SetInt("RenderCache", cache);
}

void DlgSettings3DViewImp::loadRenderCache()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");

    long cache = hGrp->GetInt("RenderCache", 0);
    ui->renderCache->setCurrentIndex(int(cache));
}

void DlgSettings3DViewImp::saveMarkerSize()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");

    QVariant const &vBoxMarkerSize = ui->boxMarkerSize->itemData(ui->boxMarkerSize->currentIndex());
    hGrp->SetInt("MarkerSize", vBoxMarkerSize.toInt());
}

void DlgSettings3DViewImp::loadMarkerSize()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");

    // NOLINTBEGIN
    int marker = hGrp->GetInt("MarkerSize", 9L);
    ui->boxMarkerSize->addItem(tr("5px"), QVariant(5));
    ui->boxMarkerSize->addItem(tr("7px"), QVariant(7));
    ui->boxMarkerSize->addItem(tr("9px"), QVariant(9));
    ui->boxMarkerSize->addItem(tr("11px"), QVariant(11));
    ui->boxMarkerSize->addItem(tr("13px"), QVariant(13));
    ui->boxMarkerSize->addItem(tr("15px"), QVariant(15));
    ui->boxMarkerSize->addItem(tr("20px"), QVariant(20));
    ui->boxMarkerSize->addItem(tr("25px"), QVariant(25));
    ui->boxMarkerSize->addItem(tr("30px"), QVariant(30));
    marker = ui->boxMarkerSize->findData(QVariant(marker));
    if (marker < 0) {
        marker = 2;
    }
    ui->boxMarkerSize->setCurrentIndex(marker);
    // NOLINTEND
}

void DlgSettings3DViewImp::resetSettingsToDefaults()
{
    ParameterGrp::handle hGrp;
    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    //reset "AntiAliasing" parameter
    hGrp->RemoveInt("AntiAliasing");
    //reset "RenderCache" parameter
    hGrp->RemoveInt("RenderCache");
    //reset "MarkerSize" parameter
    hGrp->RemoveInt("MarkerSize");

    //finally reset all the parameters associated to Gui::Pref* widgets
    PreferencePage::resetSettingsToDefaults();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettings3DViewImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->comboAliasing->blockSignals(true);
        int aliasing = ui->comboAliasing->currentIndex();
        ui->retranslateUi(this);
        addAntiAliasing();
        ui->comboAliasing->setCurrentIndex(aliasing);
        ui->comboAliasing->blockSignals(false);
    }
    else {
        PreferencePage::changeEvent(e);
    }
}

void DlgSettings3DViewImp::onAliasingChanged(int index)
{
    if (index < 0 || !isVisible()) {
        return;
    }

    // Show this message only once per application session to reduce
    // annoyance when showing it too often.
    static bool showMsg = true;
    if (showMsg) {
        showMsg = false;
        QMessageBox::information(this, tr("Anti-aliasing"),
            tr("Open a new viewer or restart %1 to apply anti-aliasing changes.")
            .arg(qApp->applicationName()));
    }
}

#include "moc_DlgSettings3DViewImp.cpp"


