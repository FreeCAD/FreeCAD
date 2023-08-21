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
#endif

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <Gui/View3DInventorViewer.h>

#include "DlgSettings3DViewImp.h"
#include "ui_DlgSettings3DView.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettings3DViewImp */

bool DlgSettings3DViewImp::showMsg = true;

/**
 *  Constructs a DlgSettings3DViewImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DlgSettings3DViewImp::DlgSettings3DViewImp(QWidget* parent)
    : PreferencePage( parent )
    , ui(new Ui_DlgSettings3DView)
{
    ui->setupUi(this);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettings3DViewImp::~DlgSettings3DViewImp() = default;

void DlgSettings3DViewImp::saveSettings()
{
    // must be done as very first because we create a new instance of NavigatorStyle
    // where we set some attributes afterwards
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");

    int index = ui->comboAliasing->currentIndex();
    hGrp->SetInt("AntiAliasing", index);

    index = ui->renderCache->currentIndex();
    hGrp->SetInt("RenderCache", index);

    ui->comboTransparentRender->onSave();

    QVariant const &vBoxMarkerSize = ui->boxMarkerSize->itemData(ui->boxMarkerSize->currentIndex());
    hGrp->SetInt("MarkerSize", vBoxMarkerSize.toInt());

    ui->CheckBox_CornerCoordSystem->onSave();
    ui->SpinBox_CornerCoordSystemSize->onSave();
    ui->CheckBox_ShowAxisCross->onSave();
    ui->CheckBox_ShowFPS->onSave();
    ui->CheckBox_use_SW_OpenGL->onSave();
    ui->CheckBox_useVBO->onSave();
    ui->FloatSpinBox_EyeDistance->onSave();
    ui->checkBoxBacklight->onSave();
    ui->backlightColor->onSave();
    ui->sliderIntensity->onSave();
    ui->radioPerspective->onSave();
    ui->radioOrthographic->onSave();
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
    ui->checkBoxBacklight->onRestore();
    ui->backlightColor->onRestore();
    ui->sliderIntensity->onRestore();
    ui->radioPerspective->onRestore();
    ui->radioOrthographic->onRestore();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");

    int index = hGrp->GetInt("AntiAliasing", int(Gui::View3DInventorViewer::None));
    index = Base::clamp(index, 0, ui->comboAliasing->count()-1);
    ui->comboAliasing->setCurrentIndex(index);
    // connect after setting current item of the combo box
    connect(ui->comboAliasing, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DlgSettings3DViewImp::onAliasingChanged);

    index = hGrp->GetInt("RenderCache", 0);
    ui->renderCache->setCurrentIndex(index);

    ui->comboTransparentRender->onRestore();

    int const current = hGrp->GetInt("MarkerSize", 9L);
    ui->boxMarkerSize->addItem(tr("5px"), QVariant(5));
    ui->boxMarkerSize->addItem(tr("7px"), QVariant(7));
    ui->boxMarkerSize->addItem(tr("9px"), QVariant(9));
    ui->boxMarkerSize->addItem(tr("11px"), QVariant(11));
    ui->boxMarkerSize->addItem(tr("13px"), QVariant(13));
    ui->boxMarkerSize->addItem(tr("15px"), QVariant(15));
    index = ui->boxMarkerSize->findData(QVariant(current));
    if (index < 0) index = 2;
    ui->boxMarkerSize->setCurrentIndex(index);
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
        ui->comboAliasing->setCurrentIndex(aliasing);
        ui->comboAliasing->blockSignals(false);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettings3DViewImp::onAliasingChanged(int index)
{
    if (index < 0 || !isVisible())
        return;
    // Show this message only once per application session to reduce
    // annoyance when showing it too often.
    if (showMsg) {
        showMsg = false;
        QMessageBox::information(this, tr("Anti-aliasing"),
            tr("Open a new viewer or restart %1 to apply anti-aliasing changes.").arg(qApp->applicationName()));
    }
}

#include "moc_DlgSettings3DViewImp.cpp"

