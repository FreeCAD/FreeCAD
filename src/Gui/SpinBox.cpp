/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <climits>
# include <QKeyEvent>
# include <QLineEdit>
# include <QStyle>
# include <QStyleOptionSpinBox>
# include <QStylePainter>
#endif

#include <boost/math/special_functions/round.hpp>

#include <App/ExpressionParser.h>
#include <App/PropertyUnits.h>
#include <Base/Tools.h>

#include "SpinBox.h"
#include "Command.h"
#include "DlgExpressionInput.h"
#include "QuantitySpinBox_p.h"


using namespace Gui;
using namespace App;
using namespace Base;

ExpressionSpinBox::ExpressionSpinBox(QAbstractSpinBox* sb)
  : spinbox(sb)
{
    lineedit = spinbox->findChild<QLineEdit*>();
    makeLabel(lineedit);
    QObject::connect(iconLabel, &ExpressionLabel::clicked, [=]() {
        this->openFormulaDialog();
    });
}

ExpressionSpinBox::~ExpressionSpinBox() = default;

void ExpressionSpinBox::bind(const App::ObjectIdentifier &_path)
{
    ExpressionBinding::bind(_path);

    showIcon();
}

void ExpressionSpinBox::showIcon()
{
    int frameWidth = spinbox->style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
    lineedit->setStyleSheet(QString::fromLatin1("QLineEdit { padding-right: %1px } ").arg(iconLabel->sizeHint().width() + frameWidth + 1));

    iconLabel->show();
}

void ExpressionSpinBox::validateInput()
{
}

void ExpressionSpinBox::showInvalidExpression(const QString& tip)
{
    spinbox->setReadOnly(true);
    QPalette p(lineedit->palette());
    p.setColor(QPalette::Active, QPalette::Text, Qt::red);
    lineedit->setPalette(p);
    iconLabel->setToolTip(tip);
}

void ExpressionSpinBox::showValidExpression(ExpressionSpinBox::Number number)
{
    std::unique_ptr<Expression> result(getExpression()->eval());
    auto * value = freecad_dynamic_cast<NumberExpression>(result.get());

    if (value) {
        switch (number) {
        case Number::SetIfNumber:
            setNumberExpression(value);
            break;
        case Number::KeepCurrent:
            break;
        }

        spinbox->setReadOnly(true);
        iconLabel->setPixmap(getIcon(":/icons/bound-expression.svg", QSize(iconHeight, iconHeight)));

        QPalette p(lineedit->palette());
        p.setColor(QPalette::Text, Qt::lightGray);
        lineedit->setPalette(p);
    }
    iconLabel->setExpressionText(Base::Tools::fromStdString(getExpression()->toString()));
}

void ExpressionSpinBox::clearExpression()
{
    spinbox->setReadOnly(false);
    QPixmap pixmap = getIcon(":/icons/bound-expression-unset.svg", QSize(iconHeight, iconHeight));
    iconLabel->setPixmap(pixmap);

    QPalette p(lineedit->palette());
    p.setColor(QPalette::Active, QPalette::Text, defaultPalette.color(QPalette::Text));
    lineedit->setPalette(p);
    iconLabel->setExpressionText(QString());
}

void ExpressionSpinBox::updateExpression()
{
    try {
        if (isBound() && getExpression()) {
            showValidExpression(Number::KeepCurrent);
        }
        else {
            clearExpression();
        }
    }
    catch (const Base::Exception & e) {
        showInvalidExpression(QString::fromLatin1(e.what()));
    }
}

void ExpressionSpinBox::setExpression(std::shared_ptr<Expression> expr)
{
    Q_ASSERT(isBound());

    try {
        ExpressionBinding::setExpression(expr);
        validateInput();
    }
    catch (const Base::Exception & e) {
        showInvalidExpression(QString::fromLatin1(e.what()));
    }
}

void ExpressionSpinBox::onChange()
{
    Q_ASSERT(isBound());

    if (getExpression()) {
        showValidExpression(Number::SetIfNumber);
    }
    else {
        clearExpression();
    }
}

void ExpressionSpinBox::resizeWidget()
{
    int frameWidth = spinbox->style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);

    QSize sz = iconLabel->sizeHint();
    iconLabel->move(lineedit->rect().right() - frameWidth - sz.width(), 0);
    updateExpression();
}

void ExpressionSpinBox::openFormulaDialog()
{
    Q_ASSERT(isBound());

    auto * qprop = freecad_dynamic_cast<PropertyQuantity>(getPath().getProperty());
    Unit unit;

    if (qprop)
        unit = qprop->getUnit();

    auto box = new Gui::Dialog::DlgExpressionInput(getPath(), getExpression(), unit, spinbox);
    QObject::connect(box, &Gui::Dialog::DlgExpressionInput::finished, [=]() {
        if (box->result() == QDialog::Accepted)
            setExpression(box->getExpression());
        else if (box->discardedFormula())
            setExpression(std::shared_ptr<Expression>());

        box->deleteLater();
    });
    box->show();

    QPoint pos = spinbox->mapToGlobal(QPoint(0,0));
    box->move(pos-box->expressionPosition());
    box->setExpressionInputSize(spinbox->width(), spinbox->height());
}

bool ExpressionSpinBox::handleKeyEvent(const QString& text)
{
    if (text == QLatin1String("=") && isBound()) {
        openFormulaDialog();
        return true;
    }

    return false;
}

void ExpressionSpinBox::drawControl(QStyleOptionSpinBox& opt)
{
    if (hasExpression()) {
        opt.activeSubControls &= ~QStyle::SC_SpinBoxUp;
        opt.activeSubControls &= ~QStyle::SC_SpinBoxDown;
        opt.state &= ~QStyle::State_Active;
        opt.stepEnabled = QAbstractSpinBox::StepNone;
    }

    QStylePainter p(spinbox);
    p.drawComplexControl(QStyle::CC_SpinBox, opt);
}

// ----------------------------------------------------------------------------

UnsignedValidator::UnsignedValidator( QObject * parent )
  : QValidator( parent )
{
    b =  0;
    t =  UINT_MAX;
}

UnsignedValidator::UnsignedValidator( uint minimum, uint maximum, QObject * parent )
  : QValidator( parent )
{
    b = minimum;
    t = maximum;
}

UnsignedValidator::~UnsignedValidator() = default;

QValidator::State UnsignedValidator::validate( QString & input, int & ) const
{
    QString stripped = input.trimmed();
    if ( stripped.isEmpty() )
        return Intermediate;
    bool ok;
    uint entered = input.toUInt( &ok );
    if ( !ok )
        return Invalid;
    else if ( entered < b )
        return Intermediate;
    else if ( entered > t )
        return Invalid;
    //  else if ( entered < b || entered > t )
    //	  return Invalid;
    else
        return Acceptable;
}

void UnsignedValidator::setRange( uint minimum, uint maximum )
{
    b = minimum;
    t = maximum;
}

void UnsignedValidator::setBottom( uint bottom )
{
    setRange( bottom, top() );
}

void UnsignedValidator::setTop( uint top )
{
    setRange( bottom(), top );
}

namespace Gui {
class UIntSpinBoxPrivate
{
public:
    UnsignedValidator * mValidator{nullptr};

    UIntSpinBoxPrivate() = default;
    uint mapToUInt( int v ) const
    {
        uint ui;
        if ( v == INT_MIN ) {
            ui = 0;
        } else if ( v == INT_MAX ) {
            ui = UINT_MAX;
        } else if ( v < 0 ) {
            v -= INT_MIN; ui = (uint)v;
        } else {
            ui = (uint)v; ui -= INT_MIN;
        } return ui;
    }
    int mapToInt( uint v ) const
    {
        int in;
        if ( v == UINT_MAX ) {
            in = INT_MAX;
        } else if ( v == 0 ) {
            in = INT_MIN;
        } else if ( v > INT_MAX ) {
            v += INT_MIN; in = (int)v;
        } else {
            in = v; in += INT_MIN;
        } return in;
    }
};

} // namespace Gui

UIntSpinBox::UIntSpinBox (QWidget* parent)
  : QSpinBox (parent)
  , ExpressionSpinBox(this)
{
    d = new UIntSpinBoxPrivate;
    d->mValidator =  new UnsignedValidator(this->minimum(), this->maximum(), this);
    connect(this, qOverload<int>(&QSpinBox::valueChanged), this, &UIntSpinBox::valueChange);
    setRange(0, 99);
    setValue(0);
    updateValidator();
}

UIntSpinBox::~UIntSpinBox()
{
    delete d->mValidator;
    delete d; d = nullptr;
}

void UIntSpinBox::setRange(uint minVal, uint maxVal)
{
    int iminVal = d->mapToInt(minVal);
    int imaxVal = d->mapToInt(maxVal);
    QSpinBox::setRange(iminVal, imaxVal);
    updateValidator();
}

QValidator::State UIntSpinBox::validate (QString & input, int & pos) const
{
    return d->mValidator->validate(input, pos);
}

uint UIntSpinBox::value() const
{
    return d->mapToUInt(QSpinBox::value());
}

void UIntSpinBox::setValue(uint value)
{
    QSpinBox::setValue(d->mapToInt(value));
}

void UIntSpinBox::valueChange(int value)
{
    Q_EMIT unsignedChanged(d->mapToUInt(value));
}

uint UIntSpinBox::minimum() const
{
    return d->mapToUInt(QSpinBox::minimum());
}

void UIntSpinBox::setMinimum(uint minVal)
{
    uint maxVal = maximum();
    if (maxVal < minVal)
        maxVal = minVal;
    setRange(minVal, maxVal);
}

uint UIntSpinBox::maximum() const
{
    return d->mapToUInt(QSpinBox::maximum());
}

void UIntSpinBox::setMaximum(uint maxVal)
{
    uint minVal = minimum();
    if (minVal > maxVal)
        minVal = maxVal;
    setRange(minVal, maxVal);
}

QString UIntSpinBox::textFromValue (int v) const
{
    uint val = d->mapToUInt(v);
    QString s;
    s.setNum(val);
    return s;
}

int UIntSpinBox::valueFromText (const QString & text) const
{
    bool ok;
    QString s = text;
    uint newVal = s.toUInt(&ok);
    if (!ok && !(prefix().isEmpty() && suffix().isEmpty())) {
        s = cleanText();
        newVal = s.toUInt(&ok);
    }

    return d->mapToInt(newVal);
}

void UIntSpinBox::updateValidator()
{
    d->mValidator->setRange(this->minimum(), this->maximum());
}

bool UIntSpinBox::apply(const std::string & propName)
{
    if (!ExpressionBinding::apply(propName)) {
        Gui::Command::doCommand(Gui::Command::Doc, "%s = %u", propName.c_str(), value());
        return true;
    }

    return false;
}

void UIntSpinBox::setNumberExpression(App::NumberExpression* expr)
{
    setValue(boost::math::round(expr->getValue()));
}

void UIntSpinBox::resizeEvent(QResizeEvent * event)
{
    QAbstractSpinBox::resizeEvent(event);
    resizeWidget();
}

void UIntSpinBox::keyPressEvent(QKeyEvent *event)
{
    if (!handleKeyEvent(event->text()))
        QAbstractSpinBox::keyPressEvent(event);
}

void UIntSpinBox::paintEvent(QPaintEvent*)
{
    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    drawControl(opt);
}

// ----------------------------------------------------------------------------

IntSpinBox::IntSpinBox(QWidget* parent)
    : QSpinBox(parent)
    , ExpressionSpinBox(this)
{
}

IntSpinBox::~IntSpinBox() = default;

bool IntSpinBox::apply(const std::string& propName)
{
    if (!ExpressionBinding::apply(propName)) {
        Gui::Command::doCommand(Gui::Command::Doc, "%s = %d", propName.c_str(), value());
        return true;
    }
    else
        return false;
}

void IntSpinBox::setNumberExpression(App::NumberExpression* expr)
{
    setValue(boost::math::round(expr->getValue()));
}

void IntSpinBox::resizeEvent(QResizeEvent * event)
{
    QAbstractSpinBox::resizeEvent(event);
    resizeWidget();
}

void IntSpinBox::keyPressEvent(QKeyEvent *event)
{
    if (!handleKeyEvent(event->text()))
        QAbstractSpinBox::keyPressEvent(event);
}

void IntSpinBox::paintEvent(QPaintEvent*)
{
    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    drawControl(opt);
}

// ----------------------------------------------------------------------------

DoubleSpinBox::DoubleSpinBox(QWidget* parent)
    : QDoubleSpinBox(parent)
    , ExpressionSpinBox(this)
{
}

DoubleSpinBox::~DoubleSpinBox() = default;

bool DoubleSpinBox::apply(const std::string& propName)
{
    if (!ExpressionBinding::apply(propName)) {
        Gui::Command::doCommand(Gui::Command::Doc, "%s = %f", propName.c_str(), value());
        return true;
    }

    return false;
}

void DoubleSpinBox::setNumberExpression(App::NumberExpression* expr)
{
    setValue(expr->getValue());
}

void DoubleSpinBox::resizeEvent(QResizeEvent * event)
{
    QAbstractSpinBox::resizeEvent(event);
    resizeWidget();
}

void DoubleSpinBox::keyPressEvent(QKeyEvent *event)
{
    if (!handleKeyEvent(event->text()))
        QDoubleSpinBox::keyPressEvent(event);
}

void DoubleSpinBox::paintEvent(QPaintEvent*)
{
    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    drawControl(opt);
}

#include "moc_SpinBox.cpp"
