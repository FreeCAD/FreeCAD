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
# include <QLabel>
# include <QCheckBox>
# include <QSpinBox>
# include <QApplication>
# include <QGridLayout>
# include <QVBoxLayout>
# include <QHBoxLayout>
#endif

#include "DlgSettingsPieMenu.h"
#include "ViewParams.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsPieMenu */

class DlgSettingsPieMenu::Private
{
public:
#define FC_PIEMENU_PARAMS \
    FC_PIEMENU_PARAM2(PieMenuIconSize, "Icon size", QSpinBox, 0, 64, 1, value, setValue) \
    FC_PIEMENU_PARAM2(PieMenuRadius, "Radius", QSpinBox, 10, 500, 10, value, setValue) \
    FC_PIEMENU_PARAM2(PieMenuTriggerRadius, "Trigger radius", QSpinBox, 10, 500, 10, value, setValue) \
    FC_PIEMENU_PARAM2(PieMenuFontSize, "Font size", QSpinBox, 0, 32, 1, value, setValue) \
    FC_PIEMENU_PARAM2(PieMenuTriggerDelay, "Trigger delay (ms)", QSpinBox, 0, 10000, 100, value, setValue) \
    FC_PIEMENU_PARAM(PieMenuTriggerAction, "Trigger action", QCheckBox, isChecked, setChecked) \
    FC_PIEMENU_PARAM2(PieMenuAnimationDuration, "Animation duration (ms)", QSpinBox, 0, 1000, 100, value, setValue) \

#undef FC_PIEMENU_PARAM
#define FC_PIEMENU_PARAM(_name, _label, _type, _getter, _setter) \
    _type * _name = nullptr;\
    QLabel *label##_name = nullptr;

#undef FC_PIEMENU_PARAM2
#define FC_PIEMENU_PARAM2(_name, _label, _type, _min, _max, _step, _setter, _getter) \
    FC_PIEMENU_PARAM(_name, _label, _type, _setter, _getter);

    FC_PIEMENU_PARAMS

#undef FC_PIEMENU_PARAM
#define FC_PIEMENU_PARAM(_name, _label, _type, _getter, _setter) do {\
        label##_name->setText(tr(_label));\
        QString tooltip = QApplication::translate("ViewParams", ViewParams::doc##_name()); \
        if (!tooltip.isEmpty()) {\
            _name->setToolTip(tooltip); \
            label##_name->setToolTip(tooltip);\
        }\
    }while(0);

    void init()
    {
        FC_PIEMENU_PARAMS
    }
};

DlgSettingsPieMenu::DlgSettingsPieMenu(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Private)
{
    auto vlayout = new QVBoxLayout(this);
    auto hlayout = new QHBoxLayout();
    vlayout->addLayout(hlayout);
    vlayout->addStretch();
    auto layout = new QGridLayout();
    hlayout->addLayout(layout);
    hlayout->addStretch();

    setWindowTitle(tr("Pie menu"));

    int row = 0;

#undef FC_PIEMENU_PARAM
#define FC_PIEMENU_PARAM(_name, _label, _type, _getter, _setter) do {\
        ui->_name = new _type(parent);\
        ui->label##_name = new QLabel(parent);\
        ui->label##_name->setMinimumSize(240,0);\
        layout->addWidget(ui->label##_name, row, 0);\
        layout->addWidget(ui->_name, row, 1);\
        ++row;\
    }while(0);

#undef FC_PIEMENU_PARAM2
#define FC_PIEMENU_PARAM2(_name, _label, _type, _min, _max, _step, _getter, _setter) \
    FC_PIEMENU_PARAM(_name, _label, _type, _getter, _setter)\
    ui->_name->setMaximum(_max);\
    ui->_name->setMinimum(_min);\
    ui->_name->setSingleStep(_step);

    FC_PIEMENU_PARAMS;

#undef FC_PIEMENU_PARAM2
#define FC_PIEMENU_PARAM2(_name, _label, _type, _min, _max, _step, _setter, _getter) \
    FC_PIEMENU_PARAM(_name, _label, _type, _setter, _getter)

    ui->init();

#undef FC_PIEMENU_PARAM
#define FC_PIEMENU_PARAM(_name, _label, _type, _getter, _setter) \
    ui->_name->_setter(ViewParams::get##_name());\

    FC_PIEMENU_PARAMS;
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsPieMenu::~DlgSettingsPieMenu()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsPieMenu::loadSettings()
{
    FC_PIEMENU_PARAMS;
}

void DlgSettingsPieMenu::saveSettings()
{
#undef FC_PIEMENU_PARAM
#define FC_PIEMENU_PARAM(_name, _label, _type, _getter, _setter) \
    ViewParams::set##_name(ui->_name->_getter());\

    FC_PIEMENU_PARAMS;
}


/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsPieMenu::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->init();
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsPieMenu.cpp"

