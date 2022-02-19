/***************************************************************************
 *   Copyright (c) 2022 Pierre-Louis Boyer <pierrelouis.boyer@gmail.com>   *
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
#include <boost_bind_bind.hpp>
#endif

#include "ui_SketcherToolDefaultWidget.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Base/Exception.h>

#include <QEvent>

#include "ViewProviderSketch.h"

#include "SketcherToolDefaultWidget.h"

using namespace SketcherGui;
using namespace Gui::TaskView;
namespace bp = boost::placeholders;

SketcherToolDefaultWidget::SketcherToolDefaultWidget (QWidget *parent, ViewProviderSketch* sketchView)
  : QWidget(parent), ui(new Ui_SketcherToolDefaultWidget), sketchView(sketchView), blockParameterSlots(false)
{
    ui->setupUi(this);

    // connecting the needed signals
    connect(ui->parameterOne, SIGNAL(valueChanged(double)),
        this, SLOT(parameterOne_valueChanged(double)));
    connect(ui->parameterTwo, SIGNAL(valueChanged(double)),
        this, SLOT(parameterTwo_valueChanged(double)));
    connect(ui->parameterThree, SIGNAL(valueChanged(double)),
        this, SLOT(parameterThree_valueChanged(double)));
    connect(ui->parameterFour, SIGNAL(valueChanged(double)),
        this, SLOT(parameterFour_valueChanged(double)));
    connect(ui->parameterFive, SIGNAL(valueChanged(double)),
        this, SLOT(parameterFive_valueChanged(double)));
    connect(ui->parameterSix, SIGNAL(valueChanged(double)),
        this, SLOT(parameterSix_valueChanged(double)));
    connect(ui->checkBoxTS1, SIGNAL(toggled(bool)),
        this, SLOT(checkBoxTS1_toggled(bool)));
    connect(ui->checkBoxTS2, SIGNAL(toggled(bool)),
        this, SLOT(checkBoxTS2_toggled(bool)));
    connect(ui->checkBoxTS3, SIGNAL(toggled(bool)),
        this, SLOT(checkBoxTS3_toggled(bool)));
    connect(ui->checkBoxTS4, SIGNAL(toggled(bool)),
        this, SLOT(checkBoxTS4_toggled(bool)));

    ui->parameterOne->installEventFilter(this);
    ui->parameterTwo->installEventFilter(this);
    ui->parameterThree->installEventFilter(this);
    ui->parameterFour->installEventFilter(this);
    ui->parameterFive->installEventFilter(this);
    ui->parameterSix->installEventFilter(this);

    reset();
}

SketcherToolDefaultWidget::~SketcherToolDefaultWidget(){}

//pre-select the number of the spinbox when it gets the focus.
bool SketcherToolDefaultWidget::eventFilter(QObject* object, QEvent* event)
{
    if(event->type() == QEvent::FocusIn) {
        for (int i = 0; i < nParameters; i++) {
            auto parameterSpinBox = getParameterSpinBox(i);

            if(object == parameterSpinBox){
                parameterSpinBox->selectNumber();
                break;
            }
        }
    }

    return false;
}

void SketcherToolDefaultWidget::reset()
{
    Base::StateLocker lock(blockParameterSlots, true);

    std::fill(isSet.begin(), isSet.end(), false);

    for (int i = 0; i < nParameters; i++) {
        setParameterVisible(i, false);
        setParameter(i, 0.f);
    }

    for (int i = 0; i < nCheckbox; i++) {
        setCheckboxVisible(i, false);
        setCheckboxChecked(i, false);
        setCheckboxPrefEntry(i, "");
    }

    setNoticeVisible(false);
}

void SketcherToolDefaultWidget::setNoticeText(const QString& string)
{
    ui->notice->setText(string);
}
void SketcherToolDefaultWidget::setNoticeVisible(bool visible)
{
    ui->notice->setVisible(visible);
}

//Spinboxes functions
void SketcherToolDefaultWidget::parameterOne_valueChanged(double val)
{
    if(!blockParameterSlots) {
        isSet[Parameter::First] = true;
        setParameterFontStyle(Parameter::First, FontStyle::Bold);
        setParameterFocus(Parameter::Second);
        signalParameterValueChanged(Parameter::First, val);
    }
}
void SketcherToolDefaultWidget::parameterTwo_valueChanged(double val)
{
    if(!blockParameterSlots) {
        isSet[Parameter::Second] = true;
        setParameterFontStyle(Parameter::Second, FontStyle::Bold);
        setParameterFocus(Parameter::Third);
        signalParameterValueChanged(Parameter::Second, val);
    }
}
void SketcherToolDefaultWidget::parameterThree_valueChanged(double val)
{
    if(!blockParameterSlots) {
        isSet[Parameter::Third] = true;
        setParameterFontStyle(Parameter::Third, FontStyle::Bold);
        setParameterFocus(Parameter::Fourth);
        signalParameterValueChanged(Parameter::Third, val);
    }
}
void SketcherToolDefaultWidget::parameterFour_valueChanged(double val)
{
    if(!blockParameterSlots) {
        isSet[Parameter::Fourth] = true;
        setParameterFontStyle(Parameter::Fourth, FontStyle::Bold);
        setParameterFocus(Parameter::Fifth);
        signalParameterValueChanged(Parameter::Fourth, val);
    }
}
void SketcherToolDefaultWidget::parameterFive_valueChanged(double val)
{
    if(!blockParameterSlots) {
        isSet[Parameter::Fifth] = true;
        setParameterFontStyle(Parameter::Fifth, FontStyle::Bold);
        signalParameterValueChanged(Parameter::Fifth, val);
    }
}
void SketcherToolDefaultWidget::parameterSix_valueChanged(double val)
{
    if (!blockParameterSlots) {
        isSet[Parameter::Sixth] = true;
        setParameterFontStyle(Parameter::Sixth, FontStyle::Bold);
        signalParameterValueChanged(Parameter::Sixth, val);
    }
}

void SketcherToolDefaultWidget::initNParameters(int nparameters)
{
    Base::StateLocker lock(blockParameterSlots, true);

    isSet.resize(nparameters);

    std::fill(isSet.begin(), isSet.end(), false);

    for (int i = 0; i < nParameters; i++) {
        setParameterVisible(i, (i < nparameters) ? true : false);
        setParameter(i, 0.f);
        setParameterFontStyle(i, FontStyle::Italic);
    }

    setParameterFocus(Parameter::First);
}

void SketcherToolDefaultWidget::setParameterVisible(int parameterindex, bool visible)
{
    if(parameterindex < nParameters) {
        getParameterLabel(parameterindex)->setVisible(visible);
        getParameterSpinBox(parameterindex)->setVisible(visible);
    }
}

void SketcherToolDefaultWidget::setParameterLabel(int parameterindex, const QString & string)
{
    if(parameterindex < nParameters)
        getParameterLabel(parameterindex)->setText(string);
}

void SketcherToolDefaultWidget::setParameter(int parameterindex, double val)
{
    if (parameterindex < nParameters) {
        getParameterSpinBox(parameterindex)->setValue(Base::Quantity(val, Base::Unit::Length));

        return;
    }

    THROWM(Base::IndexError, QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
}

void SketcherToolDefaultWidget::setParameterEnabled(int parameterindex, bool active)
{
    if (parameterindex < nParameters) {
        getParameterSpinBox(parameterindex)->setEnabled(active);

        return;
    }

    THROWM(Base::IndexError, QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
}

void SketcherToolDefaultWidget::setParameterFocus(int parameterindex)
{
    if (parameterindex < nParameters) {
        auto parameterSpinBox = getParameterSpinBox(parameterindex);
        parameterSpinBox->selectNumber();
        QMetaObject::invokeMethod(parameterSpinBox, "setFocus", Qt::QueuedConnection);

        return;
    }

    THROWM(Base::IndexError, QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
}

void SketcherToolDefaultWidget::setParameterFontStyle(int parameterindex, FontStyle fontStyle)
{
    if (parameterindex < nParameters) {
        auto parameterSpinBox = getParameterSpinBox(parameterindex);

        switch (fontStyle) {
        case FontStyle::Italic:
            parameterSpinBox->setStyleSheet(QStringLiteral("font-weight: normal;"));
            parameterSpinBox->setStyleSheet(QStringLiteral("font-style: italic;"));
            break;
        case FontStyle::Bold:
            parameterSpinBox->setStyleSheet(QStringLiteral("font-style: normal;"));
            parameterSpinBox->setStyleSheet(QStringLiteral("font-weight: bold;"));
            break;
        case FontStyle::Normal:
            parameterSpinBox->setStyleSheet(QStringLiteral("font-style: normal;"));
            parameterSpinBox->setStyleSheet(QStringLiteral("font-weight: normal;"));
            break;
        }

        return;
    }

    THROWM(Base::IndexError, QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
}

QLabel * SketcherToolDefaultWidget::getParameterLabel(int parameterindex)
{
    switch(parameterindex) {
        case Parameter::First:
            return ui->label;
            break;
        case Parameter::Second:
            return ui->label2;
            break;
        case Parameter::Third:
            return ui->label3;
            break;
        case Parameter::Fourth:
            return ui->label4;
            break;
        case Parameter::Fifth:
            return ui->label5;
            break;
        case Parameter::Sixth:
            return ui->label6;
            break;
        default:
            return nullptr;
    }
}

Gui::PrefQuantitySpinBox * SketcherToolDefaultWidget::getParameterSpinBox(int parameterindex)
{
    switch(parameterindex) {
        case Parameter::First:
            return ui->parameterOne;
            break;
        case Parameter::Second:
            return ui->parameterTwo;
            break;
        case Parameter::Third:
            return ui->parameterThree;
            break;
        case Parameter::Fourth:
            return ui->parameterFour;
            break;
        case Parameter::Fifth:
            return ui->parameterFive;
            break;
        case Parameter::Sixth:
            return ui->parameterSix;
            break;
        default:
            return nullptr;
    }
}

double SketcherToolDefaultWidget::getParameter(int parameterindex)
{
    if (parameterindex < nParameters) {
        return getParameterSpinBox(parameterindex)->value().getValue();
    }

    THROWM(Base::IndexError, "ToolWidget parameter index out of range");
}

bool SketcherToolDefaultWidget::isParameterSet(int parameterindex)
{
    if (parameterindex < nParameters) {
        return isSet[parameterindex];
    }

    THROWM(Base::IndexError, "ToolWidget parameter index out of range");
}

void SketcherToolDefaultWidget::updateVisualValue(int parameterindex, double val) {
    if (parameterindex < nParameters) {
        Base::StateLocker lock(blockParameterSlots, true);

        auto parameterSpinBox = getParameterSpinBox(parameterindex);

        parameterSpinBox->setValue(Base::Quantity(val, Base::Unit::Length));

        if (parameterSpinBox->hasFocus()) {
            parameterSpinBox->selectNumber();
        }

        return;
    }

    THROWM(Base::IndexError, QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
}

//checkbox functions
void SketcherToolDefaultWidget::checkBoxTS1_toggled(bool val) {
    if (!blockParameterSlots) {
        if (!isCheckBoxPrefEntryEmpty(Checkbox::FirstBox)) {
            ui->checkBoxTS1->onSave();
        }
        signalCheckboxCheckedChanged(Checkbox::FirstBox, val);
    }
}
void SketcherToolDefaultWidget::checkBoxTS2_toggled(bool val) {
    if (!blockParameterSlots) {
        if (!isCheckBoxPrefEntryEmpty(Checkbox::SecondBox)) {
            ui->checkBoxTS2->onSave();
        }
        signalCheckboxCheckedChanged(Checkbox::SecondBox, val);
    }
}
void SketcherToolDefaultWidget::checkBoxTS3_toggled(bool val) {
    if (!blockParameterSlots) {
        if (!isCheckBoxPrefEntryEmpty(Checkbox::ThirdBox)) {
            ui->checkBoxTS3->onSave();
        }
        signalCheckboxCheckedChanged(Checkbox::ThirdBox, val);
    }
}
void SketcherToolDefaultWidget::checkBoxTS4_toggled(bool val) {
    if (!blockParameterSlots) {
        if (!isCheckBoxPrefEntryEmpty(Checkbox::FourthBox)) {
            ui->checkBoxTS4->onSave();
        }
        signalCheckboxCheckedChanged(Checkbox::FourthBox, val);
    }
}

void SketcherToolDefaultWidget::initNCheckboxes(int ncheckbox)
{
    Base::StateLocker lock(blockParameterSlots, true);

    for (int i = 0; i < nCheckbox; i++) {
        setCheckboxVisible(i, (i < ncheckbox) ? true : false);
        setCheckboxChecked(i, false);
    }
}

void SketcherToolDefaultWidget::setCheckboxVisible(int checkboxindex, bool visible)
{
    if (checkboxindex < nCheckbox) {
        getCheckBox(checkboxindex)->setVisible(visible);
    }
}

void SketcherToolDefaultWidget::setCheckboxChecked(int checkboxindex, bool checked)
{
    if (checkboxindex < nCheckbox) {
        getCheckBox(checkboxindex)->setChecked(checked);
    }
}

void SketcherToolDefaultWidget::setCheckboxLabel(int checkboxindex, const QString& string)
{
    if (checkboxindex < nCheckbox)
        getCheckBox(checkboxindex)->setText(string);
}

Gui::PrefCheckBox* SketcherToolDefaultWidget::getCheckBox(int checkboxindex)
{
    switch (checkboxindex) {
    case Checkbox::FirstBox:
        return ui->checkBoxTS1;
        break;
    case Checkbox::SecondBox:
        return ui->checkBoxTS2;
        break;
    case Checkbox::ThirdBox:
        return ui->checkBoxTS3;
        break;
    case Checkbox::FourthBox:
        return ui->checkBoxTS4;
        break;
    default:
        return nullptr;
    }
}

bool SketcherToolDefaultWidget::getCheckboxChecked(int checkboxindex)
{
    if (checkboxindex < nParameters) {
        return getCheckBox(checkboxindex)->isChecked();
    }

    THROWM(Base::IndexError, "ToolWidget checkbox index out of range");
}

void SketcherToolDefaultWidget::setCheckboxPrefEntry(int checkboxindex, const std::string & prefEntry)
{
    if (checkboxindex < nCheckbox) {
        QByteArray byteArray(prefEntry.c_str(), prefEntry.length());
        getCheckBox(checkboxindex)->setEntryName(byteArray);
    }
}

bool SketcherToolDefaultWidget::isCheckBoxPrefEntryEmpty(int checkboxindex)
{
    return getCheckBox(checkboxindex)->entryName().size() == 0;
}


void SketcherToolDefaultWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}


#include "moc_SketcherToolDefaultWidget.cpp"
