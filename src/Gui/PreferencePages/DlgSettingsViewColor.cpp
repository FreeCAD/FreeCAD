/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QPushButton>
#endif

#include "DlgSettingsViewColor.h"
#include "ui_DlgSettingsViewColor.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsViewColor */

/**
 *  Constructs a DlgSettingsViewColor which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DlgSettingsViewColor::DlgSettingsViewColor(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsViewColor)
{
    ui->setupUi(this);
    connect(ui->SwitchGradientColors, &QPushButton::pressed, this,
        &DlgSettingsViewColor::onSwitchGradientColorsPressed);

    connect(ui->radioButtonSimple, &QRadioButton::toggled, this,
        &DlgSettingsViewColor::onRadioButtonSimpleToggled);

    connect(ui->radioButtonGradient, &QRadioButton::toggled, this,
        &DlgSettingsViewColor::onRadioButtonGradientToggled);

    connect(ui->rbRadialGradient, &QRadioButton::toggled, this,
        &DlgSettingsViewColor::onRadioButtonRadialGradientToggled);

    connect(ui->checkMidColor, &QCheckBox::toggled, this,
        &DlgSettingsViewColor::onCheckMidColorToggled);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsViewColor::~DlgSettingsViewColor() = default;

void DlgSettingsViewColor::saveSettings()
{
    ui->SelectionColor_Background->onSave();
    ui->backgroundColorFrom->onSave();
    ui->backgroundColorTo->onSave();
    ui->backgroundColorMid->onSave();
    ui->radioButtonSimple->onSave();
    ui->radioButtonGradient->onSave();
    ui->rbRadialGradient->onSave();
    ui->checkMidColor->onSave();
    ui->TreeEditColor->onSave();
    ui->TreeActiveColor->onSave();
    ui->CbLabelColor->onSave();
    ui->CbLabelTextSize->onSave();
}

void DlgSettingsViewColor::loadSettings()
{
    ui->SelectionColor_Background->onRestore();
    ui->backgroundColorFrom->onRestore();
    ui->backgroundColorTo->onRestore();
    ui->backgroundColorMid->onRestore();
    ui->radioButtonSimple->onRestore();
    ui->radioButtonGradient->onRestore();
    ui->rbRadialGradient->onRestore();
    ui->checkMidColor->onRestore();
    ui->TreeEditColor->onRestore();
    ui->TreeActiveColor->onRestore();
    ui->CbLabelColor->onRestore();
    ui->CbLabelTextSize->onRestore();
    
    if (ui->radioButtonSimple->isChecked())
        onRadioButtonSimpleToggled(true);
    else if(ui->radioButtonGradient->isChecked())
        onRadioButtonGradientToggled(true);
    else
        onRadioButtonRadialGradientToggled(true);
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsViewColor::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsViewColor::onSwitchGradientColorsPressed()
{
    QColor tempColor = ui->backgroundColorFrom->color();
    ui->backgroundColorFrom->setColor(ui->backgroundColorTo->color());
    ui->backgroundColorTo->setColor(tempColor);
}

void DlgSettingsViewColor::onCheckMidColorToggled(bool val)
{
    ui->color2Label->setEnabled(val);
    ui->backgroundColorMid->setEnabled(val);
}

void DlgSettingsViewColor::onRadioButtonSimpleToggled(bool val)
{
    setGradientColorVisibility(!val);
}

void DlgSettingsViewColor::onRadioButtonGradientToggled(bool val)
{
    setGradientColorVisibility(val);
    ui->color1Label->setText(tr("Top:"));
    ui->color2Label->setText(tr("Middle:"));
    ui->color3Label->setText(tr("Bottom:"));
}

void DlgSettingsViewColor::onRadioButtonRadialGradientToggled(bool val)
{
    setGradientColorVisibility(val);
    ui->color1Label->setText(tr("Central:"));
    ui->color2Label->setText(tr("Midway:"));
    ui->color3Label->setText(tr("End:"));
}

void DlgSettingsViewColor::setGradientColorVisibility(bool val)
{
    ui->SelectionColor_Background->setVisible(!val);
    ui->color1Label->setVisible(val);
    ui->backgroundColorFrom->setVisible(val);
    ui->color2Label->setVisible(val);
    ui->backgroundColorMid->setVisible(val);
    ui->color3Label->setVisible(val);
    ui->backgroundColorTo->setVisible(val);
    ui->checkMidColor->setVisible(val);
    ui->SwitchGradientColors->setVisible(val);

    if (val)
        onCheckMidColorToggled(ui->checkMidColor->isChecked());
}

#include "moc_DlgSettingsViewColor.cpp"
