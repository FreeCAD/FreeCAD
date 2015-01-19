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

    QString stripped(const QString &t, int *pos) const
    {
        QString text = t;
        const int s = text.size();
        text = text.trimmed();
        if (pos)
            (*pos) -= (s - text.size());
        return text;
    }

    Base::Quantity validateAndInterpret(QString& input, int& pos, QValidator::State& state) const
    {
        Base::Quantity res;
        const double max = this->maximum;
        const double min = this->minimum;

        QString copy = input;

        int len = copy.size();
        const bool plus = max >= 0;
        const bool minus = min <= 0;

        switch (len) {
        case 0:
            state = max != min ? QValidator::Intermediate : QValidator::Invalid;
            goto end;
        case 1:
            if (copy.at(0) == locale.decimalPoint()) {
                state = QValidator::Intermediate;
                copy.prepend(QLatin1Char('0'));
                pos++;
                len++;
                goto end;
            }
            else if (copy.at(0) == QLatin1Char('+')) {
                // the quantity parser doesn't allow numbers of the form '+1.0'
                state = QValidator::Invalid;
                goto end;
            }
            else if (copy.at(0) == QLatin1Char('-')) {
                if (minus)
                    state = QValidator::Intermediate;
                else
                    state = QValidator::Invalid;
                goto end;
            }
            break;
        case 2:
            if (copy.at(1) == locale.decimalPoint()
                && (plus && copy.at(0) == QLatin1Char('+'))) {
                state = QValidator::Intermediate;
                goto end;
            }
            if (copy.at(1) == locale.decimalPoint()
                && (minus && copy.at(0) == QLatin1Char('-'))) {
                state = QValidator::Intermediate;
                copy.insert(1, QLatin1Char('0'));
                pos++;
                len++;
                goto end;
            }
            break;
        default: break;
        }

        {
            if (copy.at(0) == locale.groupSeparator()) {
                state = QValidator::Invalid;
                goto end;
            }
            else if (len > 1) {
                const int dec = copy.indexOf(locale.decimalPoint());
                if (dec != -1) {
                    if (dec + 1 < copy.size() && copy.at(dec + 1) == locale.decimalPoint() && pos == dec + 1) {
                        copy.remove(dec + 1, 1);
                    }
                    else if (copy.indexOf(locale.decimalPoint(), dec + 1) != -1) {
                        // trying to add a second decimal point is not allowed
                        state = QValidator::Invalid;
                        goto end;
                    }
                    for (int i=dec + 1; i<copy.size(); ++i) {
                        // a group separator after the decimal point is not allowed
                        if (copy.at(i) == locale.groupSeparator()) {
                            state = QValidator::Invalid;
                            goto end;
                        }
                    }
                }
            }

            bool ok = false;
            double value = min;

            if (locale.negativeSign() != QLatin1Char('-'))
                copy.replace(locale.negativeSign(), QLatin1Char('-'));
            if (locale.positiveSign() != QLatin1Char('+'))
                copy.replace(locale.positiveSign(), QLatin1Char('+'));

            try {
                QString copy2 = copy;
                copy2.remove(locale.groupSeparator());

                res = Base::Quantity::parse(copy2);
                value = res.getValue();
                ok = true;
            }
            catch (Base::Exception&) {
            }

            if (!ok) {
                // input may not be finished
                state = QValidator::Intermediate;
            }
            else if (value >= min && value <= max) {
                if (copy.endsWith(locale.decimalPoint())) {
                    // input shouldn't end with a decimal point
                    state = QValidator::Intermediate;
                }
                else if (res.getUnit().isEmpty() && !this->unit.isEmpty()) {
                    // if not dimensionless the input should have a dimension
                    state = QValidator::Intermediate;
                }
                else if (res.getUnit() != this->unit) {
                    state = QValidator::Invalid;
                }
                else {
                    state = QValidator::Acceptable;
                }
            }
            else if (max == min) { // when max and min is the same the only non-Invalid input is max (or min)
                state = QValidator::Invalid;
            }
            else {
                if ((value >= 0 && value > max) || (value < 0 && value < min)) {
                    state = QValidator::Invalid;
                }
                else {
                    state = QValidator::Intermediate;
                }
            }
        }
end:
        if (state != QValidator::Acceptable) {
            res.setValue(max > 0 ? min : max);
        }

        input = copy;
        return res;
    }

    QLocale locale;
    bool validInput;
    QString validStr;
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
    d_ptr->locale = locale();
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

// Gets called after call of 'validateAndInterpret'
void QuantitySpinBox::userInput(const QString & text)
{
    Q_D(QuantitySpinBox);

    QString tmp = text;
    int pos;
    QValidator::State state;
    Base::Quantity res = d->validateAndInterpret(tmp, pos, state);
    if (state == QValidator::Acceptable) {
        d->validInput = true;
        d->validStr = text;
    }
    else if (state == QValidator::Intermediate) {
        tmp = tmp.trimmed();
        tmp += QLatin1Char(' ');
        tmp += d->unitStr;
        Base::Quantity res2 = d->validateAndInterpret(tmp, pos, state);
        if (state == QValidator::Acceptable) {
            d->validInput = true;
            d->validStr = tmp;
            res = res2;
        }
        else {
            d->validInput = false;
            return;
        }
    }
    else {
        d->validInput = false;
        return;
    }

    double factor;
    res.getUserString(factor,d->unitStr);
    d->unitValue = res.getValue()/factor;
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
    if (isReadOnly()/* || !d->validInput*/)
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

void QuantitySpinBox::focusOutEvent(QFocusEvent * event)
{
    Q_D(QuantitySpinBox);

    int pos;
    QString text = lineEdit()->text();
    QValidator::State state;
    d->validateAndInterpret(text, pos, state);
    if (state != QValidator::Acceptable) {
        lineEdit()->setText(d->validStr);
    }
    QAbstractSpinBox::focusOutEvent(event);
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
    int pos = lineEdit()->cursorPosition();
    QValidator::State state = QValidator::Acceptable;
    Base::Quantity quant = d->validateAndInterpret(copy, pos, state);
    if (state != QValidator::Acceptable) {
        fixup(copy);
        quant = d->validateAndInterpret(copy, pos, state);
    }

    return quant;
}

QValidator::State QuantitySpinBox::validate(QString &text, int &pos) const
{
    Q_D(const QuantitySpinBox);

    QValidator::State state;
    d->validateAndInterpret(text, pos, state);
    return state;
}

void QuantitySpinBox::fixup(QString &input) const
{
    input.remove(locale().groupSeparator());
}


#include "moc_QuantitySpinBox.cpp"
