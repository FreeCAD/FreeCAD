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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QLineEdit>
# include <QFocusEvent>
#endif

#include "QuantitySpinBox.h"
#include <Base/Exception.h>

using namespace Gui;

namespace Gui {
class QuantitySpinBoxPrivate
{
public:
    QuantitySpinBoxPrivate() :
      validInput(true),
      unitValue(0),
      maximum(DOUBLE_MAX),
      minimum(-DOUBLE_MAX),
      singleStep(1.0)
    {
    }
    ~QuantitySpinBoxPrivate()
    {
    }

    Base::Quantity validateAndInterpret(QString& input, int& pos, QValidator::State& state) const
    {
        Base::Quantity res;
        if (input.isEmpty()) {
            state = QValidator::Intermediate;
            return res;
        }

        try {
            res = Base::Quantity::parse(input);

            double factor;
            QString unitStr;
            res.getUserString(factor, unitStr);
            double value = res.getValue()/factor;
            // disallow to enter numbers out of range
            if (value > this->maximum || value < this->minimum)
                state = QValidator::Invalid;
            else
                state = QValidator::Acceptable;
        }
        catch (Base::Exception&) {
            // Actually invalid input but the newInput slot gives
            // some feedback
            state = QValidator::Intermediate;
        }

        return res;
    }

    bool validInput;
    QString errorText;
    Base::Quantity quantity;
    Base::Unit unit;
    double unitValue;
    QString unitStr;
    double maximum;
    double minimum;
    double singleStep;
};
}

QuantitySpinBox::QuantitySpinBox(QWidget *parent)
    : QAbstractSpinBox(parent), d_ptr(new QuantitySpinBoxPrivate())
{
    this->setContextMenuPolicy(Qt::DefaultContextMenu);
    QObject::connect(lineEdit(), SIGNAL(textChanged(QString)),
                     this, SLOT(userInput(QString)));
}

QuantitySpinBox::~QuantitySpinBox()
{
}

void QuantitySpinBox::updateText(const Base::Quantity& quant)
{
    Q_D(QuantitySpinBox);

    double dFactor;
    QString txt = quant.getUserString(dFactor,d->unitStr);
    d->unitValue = quant.getValue()/dFactor;
    lineEdit()->setText(txt);
}

Base::Quantity QuantitySpinBox::value() const
{
    Q_D(const QuantitySpinBox);
    return d->quantity;
}

void QuantitySpinBox::setValue(const Base::Quantity& value)
{
    Q_D(QuantitySpinBox);
    d->quantity = value;
    // check limits
    if (d->quantity.getValue() > d->maximum)
        d->quantity.setValue(d->maximum);
    if (d->quantity.getValue() < d->minimum)
        d->quantity.setValue(d->minimum);

    d->unit = value.getUnit();

    updateText(value);
}

void QuantitySpinBox::setValue(double value)
{
    Q_D(QuantitySpinBox);
    setValue(Base::Quantity(value, d->unit));
}

bool QuantitySpinBox::hasValidInput() const
{
    Q_D(const QuantitySpinBox);
    return d->validInput;
}

void QuantitySpinBox::userInput(const QString & text)
{
    Q_D(QuantitySpinBox);

    if (text.isEmpty()) {
        d->errorText.clear();
        d->validInput = true;
        return;
    }

    Base::Quantity res;
    try {
        QString input = text;
        fixup(input);
        res = Base::Quantity::parse(input);
    }
    catch (Base::Exception &e) {
        d->errorText = QString::fromAscii(e.what());
        parseError(d->errorText);
        d->validInput = false;
        return;
    }

    if (res.getUnit().isEmpty())
        res.setUnit(d->unit);

    // check if unit fits!
    if (!d->unit.isEmpty() && !res.getUnit().isEmpty() && d->unit != res.getUnit()){
        parseError(QString::fromAscii("Wrong unit"));
        d->validInput = false;
        return;
    }

    d->errorText.clear();
    d->validInput = true;

    if (res.getValue() > d->maximum){
        res.setValue(d->maximum);
        d->errorText = tr("Maximum reached");
    }
    if (res.getValue() < d->minimum){
        res.setValue(d->minimum);
        d->errorText = tr("Minimum reached");
    }

    double dFactor;
    res.getUserString(dFactor,d->unitStr);
    d->unitValue = res.getValue()/dFactor;
    d->quantity = res;

    // signaling
    valueChanged(res);
    valueChanged(res.getValue());
}

Base::Unit QuantitySpinBox::unit() const
{
    Q_D(const QuantitySpinBox);
    return d->unit;
}

void QuantitySpinBox::setUnit(const Base::Unit &unit)
{
    Q_D(QuantitySpinBox);

    d->unit = unit;
    d->quantity.setUnit(unit);
    updateText(d->quantity);
}

void QuantitySpinBox::setUnitText(const QString& str)
{
    Base::Quantity quant = Base::Quantity::parse(str);
    setUnit(quant.getUnit());
}

QString QuantitySpinBox::unitText(void)
{
    Q_D(QuantitySpinBox);
    return d->unitStr;
}

double QuantitySpinBox::singleStep() const
{
    Q_D(const QuantitySpinBox);
    return d->singleStep;
}

void QuantitySpinBox::setSingleStep(double value)
{
    Q_D(QuantitySpinBox);

    if (value >= 0) {
        d->singleStep = value;
    }
}

double QuantitySpinBox::minimum() const
{
    Q_D(const QuantitySpinBox);
    return d->minimum;
}

void QuantitySpinBox::setMinimum(double minimum)
{
    Q_D(QuantitySpinBox);
    d->minimum = minimum;
}

double QuantitySpinBox::maximum() const
{
    Q_D(const QuantitySpinBox);
    return d->maximum;
}

void QuantitySpinBox::setMaximum(double maximum)
{
    Q_D(QuantitySpinBox);
    d->maximum = maximum;
}

void QuantitySpinBox::setRange(double minimum, double maximum)
{
    Q_D(QuantitySpinBox);
    d->minimum = minimum;
    d->maximum = maximum;
}

QAbstractSpinBox::StepEnabled QuantitySpinBox::stepEnabled() const
{
    Q_D(const QuantitySpinBox);
    if (isReadOnly() || !d->validInput)
        return StepNone;
    if (wrapping())
        return StepEnabled(StepUpEnabled | StepDownEnabled);
    StepEnabled ret = StepNone;
    if (d->quantity.getValue() < d->maximum) {
        ret |= StepUpEnabled;
    }
    if (d->quantity.getValue() > d->minimum) {
        ret |= StepDownEnabled;
    }
    return ret;
}

void QuantitySpinBox::stepBy(int steps)
{
    Q_D(QuantitySpinBox);

    double step = d->singleStep * steps;
    double val = d->unitValue + step;
    if (val > d->maximum)
        val = d->maximum;
    else if (val < d->minimum)
        val = d->minimum;

    lineEdit()->setText(QString::fromUtf8("%L1 %2").arg(val).arg(d->unitStr));
    update();
    selectNumber();
}

void QuantitySpinBox::showEvent(QShowEvent * event)
{
    Q_D(QuantitySpinBox);

    QAbstractSpinBox::showEvent(event);

    bool selected = lineEdit()->hasSelectedText();
    updateText(d->quantity);
    if (selected)
        selectNumber();
}

void QuantitySpinBox::focusInEvent(QFocusEvent * event)
{
    bool hasSel = lineEdit()->hasSelectedText();
    QAbstractSpinBox::focusInEvent(event);

    if (event->reason() == Qt::TabFocusReason ||
        event->reason() == Qt::BacktabFocusReason  ||
        event->reason() == Qt::ShortcutFocusReason) {
        if (!hasSel)
            selectNumber();
    }
}

void QuantitySpinBox::clear()
{
    QAbstractSpinBox::clear();
}

void QuantitySpinBox::selectNumber()
{
    QString str = lineEdit()->text();
    unsigned int i = 0;

    QChar d = locale().decimalPoint();
    QChar g = locale().groupSeparator();
    QChar n = locale().negativeSign();

    for (QString::iterator it = str.begin(); it != str.end(); ++it) {
        if (it->isDigit())
            i++;
        else if (*it == d)
            i++;
        else if (*it == g)
            i++;
        else if (*it == n)
            i++;
        else // any non-number character
            break;
    }

    lineEdit()->setSelection(0, i);
}

QString QuantitySpinBox::textFromValue(const Base::Quantity& value) const
{
    double factor;
    QString unitStr;
    QString str = value.getUserString(factor, unitStr);
    if (qAbs(value.getValue()) >= 1000.0) {
        str.remove(locale().groupSeparator());
    }
    return str;
}

Base::Quantity QuantitySpinBox::valueFromText(const QString &text) const
{
    Q_D(const QuantitySpinBox);

    QString copy = text;
    fixup( copy );
    int pos = lineEdit()->cursorPosition();
    QValidator::State state = QValidator::Acceptable;
    return d->validateAndInterpret(copy, pos, state);
}

QValidator::State QuantitySpinBox::validate(QString &text, int &pos) const
{
    Q_D(const QuantitySpinBox);

    QValidator::State state;
    QString copy = text;
    fixup(copy);
    d->validateAndInterpret(copy, pos, state);
    return state;
}

void QuantitySpinBox::fixup(QString &input) const
{
    input.remove(locale().groupSeparator());
    if (locale().negativeSign() != QLatin1Char('-'))
        input.replace(locale().negativeSign(), QLatin1Char('-'));
    if (locale().positiveSign() != QLatin1Char('+'))
        input.replace(locale().positiveSign(), QLatin1Char('+'));
}


#include "moc_QuantitySpinBox.cpp"
