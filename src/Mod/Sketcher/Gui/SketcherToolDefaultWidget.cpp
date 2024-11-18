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
#include <Inventor/events/SoKeyboardEvent.h>
#include <QApplication>
#include <QEvent>
#endif

#include "ui_SketcherToolDefaultWidget.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/PrefWidgets.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Base/Exception.h>


#include "ViewProviderSketch.h"

#include "SketcherToolDefaultWidget.h"

using namespace SketcherGui;
using namespace Gui::TaskView;


SketcherToolDefaultWidget::SketcherToolDefaultWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_SketcherToolDefaultWidget)
    , blockParameterSlots(false)
    , blockParameterFocusPassing(false)
{
    ui->setupUi(this);

    // connecting the needed signals
    setupConnections();

    ui->parameterOne->installEventFilter(this);
    ui->parameterTwo->installEventFilter(this);
    ui->parameterThree->installEventFilter(this);
    ui->parameterFour->installEventFilter(this);
    ui->parameterFive->installEventFilter(this);
    ui->parameterSix->installEventFilter(this);

    reset();
}

SketcherToolDefaultWidget::~SketcherToolDefaultWidget() = default;

void SketcherToolDefaultWidget::setupConnections()
{
    connect(ui->parameterOne,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &SketcherToolDefaultWidget::parameterOne_valueChanged);
    connect(ui->parameterTwo,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &SketcherToolDefaultWidget::parameterTwo_valueChanged);
    connect(ui->parameterThree,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &SketcherToolDefaultWidget::parameterThree_valueChanged);
    connect(ui->parameterFour,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &SketcherToolDefaultWidget::parameterFour_valueChanged);
    connect(ui->parameterFive,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &SketcherToolDefaultWidget::parameterFive_valueChanged);
    connect(ui->parameterSix,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &SketcherToolDefaultWidget::parameterSix_valueChanged);
    connect(ui->parameterSeven,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &SketcherToolDefaultWidget::parameterSeven_valueChanged);
    connect(ui->parameterEight,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &SketcherToolDefaultWidget::parameterEight_valueChanged);
    connect(ui->parameterNine,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &SketcherToolDefaultWidget::parameterNine_valueChanged);
    connect(ui->parameterTen,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &SketcherToolDefaultWidget::parameterTen_valueChanged);
    connect(ui->checkBoxTS1,
            &QCheckBox::toggled,
            this,
            &SketcherToolDefaultWidget::checkBoxTS1_toggled);
    connect(ui->checkBoxTS2,
            &QCheckBox::toggled,
            this,
            &SketcherToolDefaultWidget::checkBoxTS2_toggled);
    connect(ui->checkBoxTS3,
            &QCheckBox::toggled,
            this,
            &SketcherToolDefaultWidget::checkBoxTS3_toggled);
    connect(ui->checkBoxTS4,
            &QCheckBox::toggled,
            this,
            &SketcherToolDefaultWidget::checkBoxTS4_toggled);
    connect(ui->comboBox1,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &SketcherToolDefaultWidget::comboBox1_currentIndexChanged);
    connect(ui->comboBox2,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &SketcherToolDefaultWidget::comboBox2_currentIndexChanged);
    connect(ui->comboBox3,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &SketcherToolDefaultWidget::comboBox3_currentIndexChanged);
}

// pre-select the number of the spinbox when it gets the focus.
bool SketcherToolDefaultWidget::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::FocusIn) {
        for (int i = 0; i < nParameters; i++) {
            auto parameterSpinBox = getParameterSpinBox(i);

            if (object == parameterSpinBox) {
                parameterSpinBox->selectNumber();
                break;
            }
        }
    }
    else if (event->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Return) {
            for (int i = 0; i < nParameters; i++) {
                if (object == getParameterSpinBox(i)) {
                    signalParameterTabOrEnterPressed(i);
                    return true;
                }
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
        setParameter(i, 0.F);
    }

    for (int i = 0; i < nCheckbox; i++) {
        setCheckboxVisible(i, false);
        setCheckboxChecked(i, false);
        setCheckboxPrefEntry(i, "");
    }
    for (int i = 0; i < nCombobox; i++) {
        setComboboxVisible(i, false);
        setComboboxIndex(i, 0);
        getComboBox(i)->clear();
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

// Spinboxes functions
void SketcherToolDefaultWidget::parameterOne_valueChanged(double val)
{
    if (!blockParameterSlots) {
        isSet[Parameter::First] = true;
        /*setParameterFontStyle(Parameter::First, FontStyle::Bold);
        if (!blockParameterFocusPassing) {
            setParameterFocus(Parameter::Second);
        }*/
        signalParameterValueChanged(Parameter::First, val);
    }
}
void SketcherToolDefaultWidget::parameterTwo_valueChanged(double val)
{
    if (!blockParameterSlots) {
        isSet[Parameter::Second] = true;
        signalParameterValueChanged(Parameter::Second, val);
    }
}
void SketcherToolDefaultWidget::parameterThree_valueChanged(double val)
{
    if (!blockParameterSlots) {
        isSet[Parameter::Third] = true;
        signalParameterValueChanged(Parameter::Third, val);
    }
}
void SketcherToolDefaultWidget::parameterFour_valueChanged(double val)
{
    if (!blockParameterSlots) {
        isSet[Parameter::Fourth] = true;
        signalParameterValueChanged(Parameter::Fourth, val);
    }
}
void SketcherToolDefaultWidget::parameterFive_valueChanged(double val)
{
    if (!blockParameterSlots) {
        isSet[Parameter::Fifth] = true;
        signalParameterValueChanged(Parameter::Fifth, val);
    }
}
void SketcherToolDefaultWidget::parameterSix_valueChanged(double val)
{
    if (!blockParameterSlots) {
        isSet[Parameter::Sixth] = true;
        signalParameterValueChanged(Parameter::Sixth, val);
    }
}
void SketcherToolDefaultWidget::parameterSeven_valueChanged(double val)
{
    if (!blockParameterSlots) {
        isSet[Parameter::Seventh] = true;
        signalParameterValueChanged(Parameter::Seventh, val);
    }
}
void SketcherToolDefaultWidget::parameterEight_valueChanged(double val)
{
    if (!blockParameterSlots) {
        isSet[Parameter::Eighth] = true;
        signalParameterValueChanged(Parameter::Eighth, val);
    }
}
void SketcherToolDefaultWidget::parameterNine_valueChanged(double val)
{
    if (!blockParameterSlots) {
        isSet[Parameter::Ninth] = true;
        signalParameterValueChanged(Parameter::Ninth, val);
    }
}
void SketcherToolDefaultWidget::parameterTen_valueChanged(double val)
{
    if (!blockParameterSlots) {
        isSet[Parameter::Tenth] = true;
        signalParameterValueChanged(Parameter::Tenth, val);
    }
}

void SketcherToolDefaultWidget::initNParameters(int nparameters, QObject* filteringObject)
{
    Base::StateLocker lock(blockParameterSlots, true);

    isSet.resize(nparameters);

    std::fill(isSet.begin(), isSet.end(), false);

    for (int i = 0; i < nParameters; i++) {
        setParameterVisible(i, (i < nparameters));
        setParameter(i, 0.F);
        setParameterFilteringObject(i, filteringObject);
        // setParameterFontStyle(i, FontStyle::Italic);
    }

    setParameterFocus(Parameter::First);
}

void SketcherToolDefaultWidget::setParameterVisible(int parameterindex, bool visible)
{
    if (parameterindex < nParameters) {
        getParameterLabel(parameterindex)->setVisible(visible);
        getParameterSpinBox(parameterindex)->setVisible(visible);
    }
}

void SketcherToolDefaultWidget::setParameterFilteringObject(int parameterindex,
                                                            QObject* filteringObject)
{
    if (parameterindex < nParameters) {
        getParameterSpinBox(parameterindex)->installEventFilter(filteringObject);

        return;
    }

    THROWM(Base::IndexError,
           QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
}

void SketcherToolDefaultWidget::setParameterLabel(int parameterindex, const QString& string)
{
    if (parameterindex < nParameters) {
        getParameterLabel(parameterindex)->setText(string);
    }
}

void SketcherToolDefaultWidget::setParameter(int parameterindex, double val)
{
    if (parameterindex < nParameters) {
        getParameterSpinBox(parameterindex)->setValue(val);

        return;
    }

    THROWM(Base::IndexError,
           QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
}

void SketcherToolDefaultWidget::setParameterWithoutPassingFocus(int parameterindex, double val)
{
    Base::StateLocker lock(blockParameterFocusPassing, true);
    setParameter(parameterindex, val);
}

void SketcherToolDefaultWidget::configureParameterInitialValue(int parameterindex, double val)
{
    Base::StateLocker lock(blockParameterSlots, true);
    setParameter(parameterindex, val);
}

void SketcherToolDefaultWidget::configureParameterUnit(int parameterindex, const Base::Unit& unit)
{
    // For reference unit can be changed with :
    // setUnit(Base::Unit::Length); Base::Unit::Angle
    Base::StateLocker lock(blockParameterSlots, true);
    if (parameterindex < nParameters) {
        getParameterSpinBox(parameterindex)->setUnit(unit);

        return;
    }

    THROWM(Base::IndexError,
           QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
}

void SketcherToolDefaultWidget::configureParameterDecimals(int parameterindex, int val)
{
    Base::StateLocker lock(blockParameterSlots, true);
    if (parameterindex < nParameters) {
        getParameterSpinBox(parameterindex)->setDecimals(val);

        return;
    }

    THROWM(Base::IndexError,
           QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
}

void SketcherToolDefaultWidget::configureParameterMin(int parameterindex, double val)
{
    Base::StateLocker lock(blockParameterSlots, true);
    if (parameterindex < nParameters) {
        getParameterSpinBox(parameterindex)->setMinimum(val);

        return;
    }

    THROWM(Base::IndexError,
           QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
}

void SketcherToolDefaultWidget::configureParameterMax(int parameterindex, double val)
{
    Base::StateLocker lock(blockParameterSlots, true);
    if (parameterindex < nParameters) {
        getParameterSpinBox(parameterindex)->setMaximum(val);

        return;
    }

    THROWM(Base::IndexError,
           QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
}

void SketcherToolDefaultWidget::setParameterEnabled(int parameterindex, bool active)
{
    if (parameterindex < nParameters) {
        getParameterSpinBox(parameterindex)->setEnabled(active);

        return;
    }

    THROWM(Base::IndexError,
           QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
}

void SketcherToolDefaultWidget::setParameterFocus(int parameterindex)
{
    if (parameterindex < nParameters) {
        auto parameterSpinBox = getParameterSpinBox(parameterindex);
        parameterSpinBox->selectNumber();
        QMetaObject::invokeMethod(parameterSpinBox, "setFocus", Qt::QueuedConnection);

        return;
    }

    THROWM(Base::IndexError,
           QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
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

    THROWM(Base::IndexError,
           QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
}

QLabel* SketcherToolDefaultWidget::getParameterLabel(int parameterindex)
{
    switch (parameterindex) {
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
        case Parameter::Seventh:
            return ui->label7;
            break;
        case Parameter::Eighth:
            return ui->label8;
            break;
        case Parameter::Ninth:
            return ui->label9;
            break;
        case Parameter::Tenth:
            return ui->label10;
            break;
        default:
            THROWM(Base::IndexError, "ToolWidget spinbox index out of range");
    }
}

Gui::PrefQuantitySpinBox* SketcherToolDefaultWidget::getParameterSpinBox(int parameterindex)
{
    switch (parameterindex) {
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
        case Parameter::Seventh:
            return ui->parameterSeven;
            break;
        case Parameter::Eighth:
            return ui->parameterEight;
            break;
        case Parameter::Ninth:
            return ui->parameterNine;
            break;
        case Parameter::Tenth:
            return ui->parameterTen;
            break;
        default:
            THROWM(Base::IndexError, "ToolWidget spinbox index out of range");
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

void SketcherToolDefaultWidget::updateVisualValue(int parameterindex,
                                                  double val,
                                                  const Base::Unit& unit)
{
    if (parameterindex < nParameters) {
        Base::StateLocker lock(blockParameterSlots, true);

        auto parameterSpinBox = getParameterSpinBox(parameterindex);

        parameterSpinBox->setValue(Base::Quantity(val, unit));

        if (parameterSpinBox->hasFocus()) {
            parameterSpinBox->selectNumber();
        }

        return;
    }

    THROWM(Base::IndexError,
           QT_TRANSLATE_NOOP("Exceptions", "ToolWidget parameter index out of range"));
}

// checkbox functions
void SketcherToolDefaultWidget::checkBoxTS1_toggled(bool val)
{
    if (!blockParameterSlots) {
        if (!isCheckBoxPrefEntryEmpty(Checkbox::FirstBox)) {
            ui->checkBoxTS1->onSave();
        }
        signalCheckboxCheckedChanged(Checkbox::FirstBox, val);
    }
}
void SketcherToolDefaultWidget::checkBoxTS2_toggled(bool val)
{
    if (!blockParameterSlots) {
        if (!isCheckBoxPrefEntryEmpty(Checkbox::SecondBox)) {
            ui->checkBoxTS2->onSave();
        }
        signalCheckboxCheckedChanged(Checkbox::SecondBox, val);
    }
}
void SketcherToolDefaultWidget::checkBoxTS3_toggled(bool val)
{
    if (!blockParameterSlots) {
        if (!isCheckBoxPrefEntryEmpty(Checkbox::ThirdBox)) {
            ui->checkBoxTS3->onSave();
        }
        signalCheckboxCheckedChanged(Checkbox::ThirdBox, val);
    }
}
void SketcherToolDefaultWidget::checkBoxTS4_toggled(bool val)
{
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
        setCheckboxVisible(i, (i < ncheckbox));
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
    if (checkboxindex < nCheckbox) {
        getCheckBox(checkboxindex)->setText(string);
    }
}

void SketcherToolDefaultWidget::setCheckboxToolTip(int checkboxindex, const QString& string)
{
    if (checkboxindex < nCheckbox) {
        getCheckBox(checkboxindex)->setToolTip(string);
    }
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
            THROWM(Base::IndexError, "ToolWidget checkbox index out of range");
    }
}

bool SketcherToolDefaultWidget::getCheckboxChecked(int checkboxindex)
{
    if (checkboxindex < nParameters) {
        return getCheckBox(checkboxindex)->isChecked();
    }

    THROWM(Base::IndexError, "ToolWidget checkbox index out of range");
}

void SketcherToolDefaultWidget::setCheckboxPrefEntry(int checkboxindex,
                                                     const std::string& prefEntry)
{
    if (checkboxindex < nCheckbox) {
        QByteArray byteArray(prefEntry.c_str(), prefEntry.length());
        getCheckBox(checkboxindex)->setEntryName(byteArray);
    }
}

void SketcherToolDefaultWidget::restoreCheckBoxPref(int checkboxindex)
{
    if (checkboxindex < nCheckbox) {
        getCheckBox(checkboxindex)->onRestore();
    }
}

void SketcherToolDefaultWidget::setCheckboxIcon(int checkboxindex, QIcon icon)
{
    if (checkboxindex < nCheckbox) {
        getCheckBox(checkboxindex)->setIcon(icon);
    }
}

void SketcherToolDefaultWidget::setComboboxItemIcon(int comboboxindex, int index, QIcon icon)
{
    if (comboboxindex < nCombobox) {
        getComboBox(comboboxindex)->setItemIcon(index, icon);
    }
}

void SketcherToolDefaultWidget::setComboboxPrefEntry(int comboboxindex,
                                                     const std::string& prefEntry)
{
    if (comboboxindex < nCombobox) {
        QByteArray byteArray(prefEntry.c_str(), prefEntry.length());
        getComboBox(comboboxindex)->setEntryName(byteArray);
    }
}

void SketcherToolDefaultWidget::restoreComboboxPref(int comboboxindex)
{
    if (comboboxindex < nCombobox) {
        getComboBox(comboboxindex)->onRestore();
    }
}

bool SketcherToolDefaultWidget::isCheckBoxPrefEntryEmpty(int checkboxindex)
{
    return getCheckBox(checkboxindex)->entryName().size() == 0;
}

// Combobox functions
void SketcherToolDefaultWidget::comboBox1_currentIndexChanged(int val)
{
    if (!blockParameterSlots) {
        signalComboboxSelectionChanged(Combobox::FirstCombo, val);
    }
    ui->comboBox1->onSave();
}
void SketcherToolDefaultWidget::comboBox2_currentIndexChanged(int val)
{
    if (!blockParameterSlots) {
        signalComboboxSelectionChanged(Combobox::SecondCombo, val);
    }
    ui->comboBox2->onSave();
}
void SketcherToolDefaultWidget::comboBox3_currentIndexChanged(int val)
{
    if (!blockParameterSlots) {
        signalComboboxSelectionChanged(Combobox::ThirdCombo, val);
    }
    ui->comboBox3->onSave();
}

void SketcherToolDefaultWidget::initNComboboxes(int ncombobox)
{
    Base::StateLocker lock(blockParameterSlots, true);

    for (int i = 0; i < nCombobox; i++) {
        setComboboxVisible(i, (i < ncombobox));
    }
}

void SketcherToolDefaultWidget::setComboboxVisible(int comboboxindex, bool visible)
{
    if (comboboxindex < nCombobox) {
        getComboBox(comboboxindex)->setVisible(visible);
        getComboBoxLabel(comboboxindex)->setVisible(visible);
    }
}

void SketcherToolDefaultWidget::setComboboxIndex(int comboboxindex, int value)
{
    if (comboboxindex < nCombobox) {
        getComboBox(comboboxindex)->setCurrentIndex(value);
    }
}

void SketcherToolDefaultWidget::setComboboxLabel(int comboboxindex, const QString& string)
{
    if (comboboxindex < nCombobox) {
        getComboBoxLabel(comboboxindex)->setText(string);
    }
}

void SketcherToolDefaultWidget::setComboboxElements(int comboboxindex, const QStringList& names)
{
    if (comboboxindex < nCombobox) {
        getComboBox(comboboxindex)->clear();
        getComboBox(comboboxindex)->addItems(names);
    }
}

Gui::PrefComboBox* SketcherToolDefaultWidget::getComboBox(int comboboxindex)
{
    switch (comboboxindex) {
        case Combobox::FirstCombo:
            return ui->comboBox1;
            break;
        case Combobox::SecondCombo:
            return ui->comboBox2;
            break;
        case Combobox::ThirdCombo:
            return ui->comboBox3;
            break;
        default:
            THROWM(Base::IndexError, "ToolWidget combobox index out of range");
    }
}
QLabel* SketcherToolDefaultWidget::getComboBoxLabel(int comboboxindex)
{
    switch (comboboxindex) {
        case Combobox::FirstCombo:
            return ui->comboLabel1;
            break;
        case Combobox::SecondCombo:
            return ui->comboLabel2;
            break;
        case Combobox::ThirdCombo:
            return ui->comboLabel3;
            break;
        default:
            THROWM(Base::IndexError, "ToolWidget combobox index out of range");
    }
}

int SketcherToolDefaultWidget::getComboboxIndex(int comboboxindex)
{
    if (comboboxindex < nCombobox) {
        return getComboBox(comboboxindex)->currentIndex();
    }

    THROWM(Base::IndexError, "ToolWidget combobox index out of range");
}


void SketcherToolDefaultWidget::changeEvent(QEvent* ev)
{
    QWidget::changeEvent(ev);
    if (ev->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

#include "moc_SketcherToolDefaultWidget.cpp"
