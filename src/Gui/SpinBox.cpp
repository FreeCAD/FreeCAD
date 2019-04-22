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
# include <QDebug>
# include <climits>
# include <QStyle>
# include <QLineEdit>
# include <QKeyEvent>
#endif

#include "SpinBox.h"
#include "DlgExpressionInput.h"
#include "Command.h"
#include <Base/Tools.h>
#include <App/ExpressionParser.h>
#include <boost/math/special_functions/round.hpp>
#include "QuantitySpinBox_p.h"
#include <App/PropertyUnits.h>

using namespace Gui;
using namespace App;
using namespace Base;

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

UnsignedValidator::~UnsignedValidator()
{

}

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
    UnsignedValidator * mValidator;

    UIntSpinBoxPrivate() : mValidator(0)
    {
    }
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
{
    d = new UIntSpinBoxPrivate;
    d->mValidator =  new UnsignedValidator(this->minimum(), this->maximum(), this);
    connect(this, SIGNAL(valueChanged(int)),
            this, SLOT(valueChange(int)));
    setRange(0, 99);
    setValue(0);
    updateValidator();

    defaultPalette = lineEdit()->palette();

    /* Icon for f(x) */
    QFontMetrics fm(lineEdit()->font());
    int frameWidth = style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
    iconHeight = fm.height() - frameWidth;
    iconLabel = new ExpressionLabel(lineEdit());
    iconLabel->setCursor(Qt::ArrowCursor);
    QPixmap pixmap = getIcon(":/icons/bound-expression-unset.svg", QSize(iconHeight, iconHeight));
    iconLabel->setPixmap(pixmap);
    iconLabel->setStyleSheet(QString::fromLatin1("QLabel { border: none; padding: 0px; padding-top: %2px; width: %1px; height: %1px }").arg(iconHeight).arg(frameWidth/2));
    iconLabel->hide();
    lineEdit()->setStyleSheet(QString::fromLatin1("QLineEdit { padding-right: %1px } ").arg(iconHeight+frameWidth));

    QObject::connect(iconLabel, SIGNAL(clicked()), this, SLOT(openFormulaDialog()));
}

UIntSpinBox::~UIntSpinBox()
{
    delete d->mValidator;
    delete d; d = 0;
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
    valueChanged(d->mapToUInt(value));
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

void UIntSpinBox::bind(const App::ObjectIdentifier &_path)
{
    ExpressionBinding::bind(_path);

    int frameWidth = style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
    lineEdit()->setStyleSheet(QString::fromLatin1("QLineEdit { padding-right: %1px } ").arg(iconLabel->sizeHint().width() + frameWidth + 1));

    iconLabel->show();
}

void UIntSpinBox::setExpression(boost::shared_ptr<Expression> expr)
{
    Q_ASSERT(isBound());

    try {
        ExpressionBinding::setExpression(expr);
    }
    catch (const Base::Exception & e) {
        setReadOnly(true);
        QPalette p(lineEdit()->palette());
        p.setColor(QPalette::Active, QPalette::Text, Qt::red);
        lineEdit()->setPalette(p);
        iconLabel->setToolTip(QString::fromLatin1(e.what()));
    }
}

void UIntSpinBox::onChange() {
    
    if (getExpression()) {
        std::unique_ptr<Expression> result(getExpression()->eval());
        NumberExpression * value = freecad_dynamic_cast<NumberExpression>(result.get());

        if (value) {
            setValue(boost::math::round(value->getValue()));
            setReadOnly(true);
            iconLabel->setPixmap(getIcon(":/icons/bound-expression.svg", QSize(iconHeight, iconHeight)));

            QPalette p(lineEdit()->palette());
            p.setColor(QPalette::Text, Qt::lightGray);
            lineEdit()->setPalette(p);
        }
        setToolTip(Base::Tools::fromStdString(getExpression()->toString()));
    }
    else {
        setReadOnly(false);
        iconLabel->setPixmap(getIcon(":/icons/bound-expression-unset.svg", QSize(iconHeight, iconHeight)));
        QPalette p(lineEdit()->palette());
        p.setColor(QPalette::Active, QPalette::Text, defaultPalette.color(QPalette::Text));
        lineEdit()->setPalette(p);

    }
    iconLabel->setToolTip(QString());
}


bool UIntSpinBox::apply(const std::string & propName)
{
    if (!ExpressionBinding::apply(propName)) {
        Gui::Command::doCommand(Gui::Command::Doc, "%s = %u", propName.c_str(), value());
        return true;
    }
    else
        return false;
}

bool UIntSpinBox::apply()
{
    return ExpressionBinding::apply();
}

void UIntSpinBox::resizeEvent(QResizeEvent * event)
{
    QAbstractSpinBox::resizeEvent(event);

    int frameWidth = style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);

    QSize sz = iconLabel->sizeHint();
    iconLabel->move(lineEdit()->rect().right() - frameWidth - sz.width(), 0);

    try {
        if (isBound() && getExpression()) {
            std::unique_ptr<Expression> result(getExpression()->eval());
            NumberExpression * value = freecad_dynamic_cast<NumberExpression>(result.get());

            if (value) {
                setReadOnly(true);
                QPixmap pixmap = getIcon(":/icons/bound-expression.svg", QSize(iconHeight, iconHeight));
                iconLabel->setPixmap(pixmap);

                QPalette p(lineEdit()->palette());
                p.setColor(QPalette::Text, Qt::lightGray);
                lineEdit()->setPalette(p);
            }
            setToolTip(Base::Tools::fromStdString(getExpression()->toString()));
        }
        else {
            setReadOnly(false);
            QPixmap pixmap = getIcon(":/icons/bound-expression-unset.svg", QSize(iconHeight, iconHeight));
            iconLabel->setPixmap(pixmap);

            QPalette p(lineEdit()->palette());
            p.setColor(QPalette::Active, QPalette::Text, defaultPalette.color(QPalette::Text));
            lineEdit()->setPalette(p);

        }
        iconLabel->setToolTip(QString());
    }
    catch (const Base::Exception & e) {
        setReadOnly(true);
        QPalette p(lineEdit()->palette());
        p.setColor(QPalette::Active, QPalette::Text, Qt::red);
        lineEdit()->setPalette(p);
        iconLabel->setToolTip(QString::fromLatin1(e.what()));
    }

}

void UIntSpinBox::openFormulaDialog()
{
    Q_ASSERT(isBound());

    PropertyQuantity *  qprop = freecad_dynamic_cast<PropertyQuantity>(getPath().getProperty());
    Unit unit;

    if (qprop != 0)
        unit = qprop->getUnit();

    Gui::Dialog::DlgExpressionInput* box = new Gui::Dialog::DlgExpressionInput(getPath(), getExpression(), unit, this);
    connect(box, SIGNAL(finished(int)), this, SLOT(finishFormulaDialog()));
    box->show();

    QPoint pos = mapToGlobal(QPoint(0,0));
    box->move(pos-box->expressionPosition());
    box->setExpressionInputSize(width(), height());
}

void UIntSpinBox::finishFormulaDialog()
{
    Gui::Dialog::DlgExpressionInput* box = qobject_cast<Gui::Dialog::DlgExpressionInput*>(sender());
    if (!box) {
        qWarning() << "Sender is not a Gui::Dialog::DlgExpressionInput";
        return;
    }

    if (box->result() == QDialog::Accepted)
        setExpression(box->getExpression());
    else if (box->discardedFormula())
        setExpression(boost::shared_ptr<Expression>());

    box->deleteLater();
}

void UIntSpinBox::keyPressEvent(QKeyEvent *event)
{
    if (event->text() == QString::fromUtf8("=") && isBound())
        openFormulaDialog();
    else {
        if (!hasExpression())
            QAbstractSpinBox::keyPressEvent(event);
    }
}


IntSpinBox::IntSpinBox(QWidget* parent) : QSpinBox(parent) {

    defaultPalette = lineEdit()->palette();

    /* Icon for f(x) */
    QFontMetrics fm(lineEdit()->font());
    int frameWidth = style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
    iconHeight = fm.height() - frameWidth;
    iconLabel = new ExpressionLabel(lineEdit());
    iconLabel->setCursor(Qt::ArrowCursor);
    QPixmap pixmap = getIcon(":/icons/bound-expression-unset.svg", QSize(iconHeight, iconHeight));
    iconLabel->setPixmap(pixmap);
    iconLabel->setStyleSheet(QString::fromLatin1("QLabel { border: none; padding: 0px; padding-top: %2px; width: %1px; height: %1px }").arg(iconHeight).arg(frameWidth/2));
    iconLabel->hide();
    lineEdit()->setStyleSheet(QString::fromLatin1("QLineEdit { padding-right: %1px } ").arg(iconHeight+frameWidth));

    QObject::connect(iconLabel, SIGNAL(clicked()), this, SLOT(openFormulaDialog()));
}

IntSpinBox::~IntSpinBox() {

}


bool IntSpinBox::apply(const std::string& propName) {
    
    if (!ExpressionBinding::apply(propName)) {
        Gui::Command::doCommand(Gui::Command::Doc, "%s = %u", propName.c_str(), value());
        return true;
    }
    else
        return false;
}

void IntSpinBox::bind(const ObjectIdentifier& _path) {
    
    ExpressionBinding::bind(_path);

    int frameWidth = style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
    lineEdit()->setStyleSheet(QString::fromLatin1("QLineEdit { padding-right: %1px } ").arg(iconLabel->sizeHint().width() + frameWidth + 1));

    iconLabel->show();
}

void IntSpinBox::setExpression(boost::shared_ptr<Expression> expr)
{
    Q_ASSERT(isBound());

    try {
        ExpressionBinding::setExpression(expr);
    }
    catch (const Base::Exception & e) {
        setReadOnly(true);
        QPalette p(lineEdit()->palette());
        p.setColor(QPalette::Active, QPalette::Text, Qt::red);
        lineEdit()->setPalette(p);
        iconLabel->setToolTip(QString::fromLatin1(e.what()));
    }
}

void IntSpinBox::onChange() {
    
    if (getExpression()) {
        std::unique_ptr<Expression> result(getExpression()->eval());
        NumberExpression * value = freecad_dynamic_cast<NumberExpression>(result.get());

        if (value) {
            setValue(boost::math::round(value->getValue()));
            setReadOnly(true);
            iconLabel->setPixmap(getIcon(":/icons/bound-expression.svg", QSize(iconHeight, iconHeight)));

            QPalette p(lineEdit()->palette());
            p.setColor(QPalette::Text, Qt::lightGray);
            lineEdit()->setPalette(p);
        }
        setToolTip(Base::Tools::fromStdString(getExpression()->toString()));
    }
    else {
        setReadOnly(false);
        iconLabel->setPixmap(getIcon(":/icons/bound-expression-unset.svg", QSize(iconHeight, iconHeight)));
        QPalette p(lineEdit()->palette());
        p.setColor(QPalette::Active, QPalette::Text, defaultPalette.color(QPalette::Text));
        lineEdit()->setPalette(p);

    }
    iconLabel->setToolTip(QString());
}

void IntSpinBox::resizeEvent(QResizeEvent * event)
{
    QAbstractSpinBox::resizeEvent(event);

    int frameWidth = style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);

    QSize sz = iconLabel->sizeHint();
    iconLabel->move(lineEdit()->rect().right() - frameWidth - sz.width(), 0);

    try {
        if (isBound() && getExpression()) {
            std::unique_ptr<Expression> result(getExpression()->eval());
            NumberExpression * value = freecad_dynamic_cast<NumberExpression>(result.get());

            if (value) {
                setReadOnly(true);
                QPixmap pixmap = getIcon(":/icons/bound-expression.svg", QSize(iconHeight, iconHeight));
                iconLabel->setPixmap(pixmap);

                QPalette p(lineEdit()->palette());
                p.setColor(QPalette::Text, Qt::lightGray);
                lineEdit()->setPalette(p);
            }
            setToolTip(Base::Tools::fromStdString(getExpression()->toString()));
        }
        else {
            setReadOnly(false);
            QPixmap pixmap = getIcon(":/icons/bound-expression-unset.svg", QSize(iconHeight, iconHeight));
            iconLabel->setPixmap(pixmap);

            QPalette p(lineEdit()->palette());
            p.setColor(QPalette::Active, QPalette::Text, defaultPalette.color(QPalette::Text));
            lineEdit()->setPalette(p);

        }
        iconLabel->setToolTip(QString());
    }
    catch (const Base::Exception & e) {
        setReadOnly(true);
        QPalette p(lineEdit()->palette());
        p.setColor(QPalette::Active, QPalette::Text, Qt::red);
        lineEdit()->setPalette(p);
        iconLabel->setToolTip(QString::fromLatin1(e.what()));
    }

}

void IntSpinBox::openFormulaDialog()
{
    Q_ASSERT(isBound());

    PropertyQuantity *  qprop = freecad_dynamic_cast<PropertyQuantity>(getPath().getProperty());
    Unit unit;

    if (qprop != 0)
        unit = qprop->getUnit();

    Gui::Dialog::DlgExpressionInput* box = new Gui::Dialog::DlgExpressionInput(getPath(), getExpression(),unit, this);
    connect(box, SIGNAL(finished(int)), this, SLOT(finishFormulaDialog()));
    box->show();

    QPoint pos = mapToGlobal(QPoint(0,0));
    box->move(pos-box->expressionPosition());
    box->setExpressionInputSize(width(), height());
}

void IntSpinBox::finishFormulaDialog()
{
    Gui::Dialog::DlgExpressionInput* box = qobject_cast<Gui::Dialog::DlgExpressionInput*>(sender());
    if (!box) {
        qWarning() << "Sender is not a Gui::Dialog::DlgExpressionInput";
        return;
    }

    if (box->result() == QDialog::Accepted)
        setExpression(box->getExpression());
    else if (box->discardedFormula())
        setExpression(boost::shared_ptr<Expression>());

    box->deleteLater();
}

void IntSpinBox::keyPressEvent(QKeyEvent *event)
{
    if (event->text() == QString::fromUtf8("=") && isBound())
        openFormulaDialog();
    else {
        if (!hasExpression())
            QAbstractSpinBox::keyPressEvent(event);
    }
}


DoubleSpinBox::DoubleSpinBox(QWidget* parent): QDoubleSpinBox(parent) {

    defaultPalette = lineEdit()->palette();

    /* Icon for f(x) */
    QFontMetrics fm(lineEdit()->font());
    int frameWidth = style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
    iconHeight = fm.height() - frameWidth;
    iconLabel = new ExpressionLabel(lineEdit());
    iconLabel->setCursor(Qt::ArrowCursor);
    QPixmap pixmap = getIcon(":/icons/bound-expression-unset.svg", QSize(iconHeight, iconHeight));
    iconLabel->setPixmap(pixmap);
    iconLabel->setStyleSheet(QString::fromLatin1("QLabel { border: none; padding: 0px; padding-top: %2px; width: %1px; height: %1px }").arg(iconHeight).arg(frameWidth/2));
    iconLabel->hide();
    lineEdit()->setStyleSheet(QString::fromLatin1("QLineEdit { padding-right: %1px } ").arg(iconHeight+frameWidth));

    QObject::connect(iconLabel, SIGNAL(clicked()), this, SLOT(openFormulaDialog()));
}

DoubleSpinBox::~DoubleSpinBox() {

}


bool DoubleSpinBox::apply(const std::string& propName) {
    
    if (!ExpressionBinding::apply(propName)) {
        Gui::Command::doCommand(Gui::Command::Doc, "%s = %u", propName.c_str(), value());
        return true;
    }
    else
        return false;
}

void DoubleSpinBox::bind(const ObjectIdentifier& _path) {
    
    ExpressionBinding::bind(_path);

    int frameWidth = style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
    lineEdit()->setStyleSheet(QString::fromLatin1("QLineEdit { padding-right: %1px } ").arg(iconLabel->sizeHint().width() + frameWidth + 1));

    iconLabel->show();
}

void DoubleSpinBox::setExpression(boost::shared_ptr<Expression> expr)
{
    Q_ASSERT(isBound());

    try {
        ExpressionBinding::setExpression(expr);
    }
    catch (const Base::Exception & e) {
        setReadOnly(true);
        QPalette p(lineEdit()->palette());
        p.setColor(QPalette::Active, QPalette::Text, Qt::red);
        lineEdit()->setPalette(p);
        iconLabel->setToolTip(QString::fromLatin1(e.what()));
    }
}

void DoubleSpinBox::onChange() {
    
    if (getExpression()) {
        std::unique_ptr<Expression> result(getExpression()->eval());
        NumberExpression * value = freecad_dynamic_cast<NumberExpression>(result.get());

        if (value) {
            setValue(boost::math::round(value->getValue()));
            setReadOnly(true);
            iconLabel->setPixmap(getIcon(":/icons/bound-expression.svg", QSize(iconHeight, iconHeight)));

            QPalette p(lineEdit()->palette());
            p.setColor(QPalette::Text, Qt::lightGray);
            lineEdit()->setPalette(p);
        }
        setToolTip(Base::Tools::fromStdString(getExpression()->toString()));
    }
    else {
        setReadOnly(false);
        iconLabel->setPixmap(getIcon(":/icons/bound-expression-unset.svg", QSize(iconHeight, iconHeight)));
        QPalette p(lineEdit()->palette());
        p.setColor(QPalette::Active, QPalette::Text, defaultPalette.color(QPalette::Text));
        lineEdit()->setPalette(p);

    }
    iconLabel->setToolTip(QString());
}

void DoubleSpinBox::resizeEvent(QResizeEvent * event)
{
    QAbstractSpinBox::resizeEvent(event);

    int frameWidth = style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);

    QSize sz = iconLabel->sizeHint();
    iconLabel->move(lineEdit()->rect().right() - frameWidth - sz.width(), 0);

    try {
        if (isBound() && getExpression()) {
            std::unique_ptr<Expression> result(getExpression()->eval());
            NumberExpression * value = freecad_dynamic_cast<NumberExpression>(result.get());

            if (value) {
                setReadOnly(true);
                QPixmap pixmap = getIcon(":/icons/bound-expression.svg", QSize(iconHeight, iconHeight));
                iconLabel->setPixmap(pixmap);

                QPalette p(lineEdit()->palette());
                p.setColor(QPalette::Text, Qt::lightGray);
                lineEdit()->setPalette(p);
            }
            setToolTip(Base::Tools::fromStdString(getExpression()->toString()));
        }
        else {
            setReadOnly(false);
            QPixmap pixmap = getIcon(":/icons/bound-expression-unset.svg", QSize(iconHeight, iconHeight));
            iconLabel->setPixmap(pixmap);

            QPalette p(lineEdit()->palette());
            p.setColor(QPalette::Active, QPalette::Text, defaultPalette.color(QPalette::Text));
            lineEdit()->setPalette(p);

        }
        iconLabel->setToolTip(QString());
    }
    catch (const Base::Exception & e) {
        setReadOnly(true);
        QPalette p(lineEdit()->palette());
        p.setColor(QPalette::Active, QPalette::Text, Qt::red);
        lineEdit()->setPalette(p);
        iconLabel->setToolTip(QString::fromLatin1(e.what()));
    }

}

void DoubleSpinBox::openFormulaDialog()
{
    Q_ASSERT(isBound());

    PropertyQuantity *  qprop = freecad_dynamic_cast<PropertyQuantity>(getPath().getProperty());
    Unit unit;

    if (qprop != 0)
        unit = qprop->getUnit();

    Gui::Dialog::DlgExpressionInput* box = new Gui::Dialog::DlgExpressionInput(getPath(), getExpression(), unit, this);
    connect(box, SIGNAL(finished(int)), this, SLOT(finishFormulaDialog()));
    box->show();

    QPoint pos = mapToGlobal(QPoint(0,0));
    box->move(pos-box->expressionPosition());
    box->setExpressionInputSize(width(), height());
}

void DoubleSpinBox::finishFormulaDialog()
{
    Gui::Dialog::DlgExpressionInput* box = qobject_cast<Gui::Dialog::DlgExpressionInput*>(sender());
    if (!box) {
        qWarning() << "Sender is not a Gui::Dialog::DlgExpressionInput";
        return;
    }

    if (box->result() == QDialog::Accepted)
        setExpression(box->getExpression());
    else if (box->discardedFormula())
        setExpression(boost::shared_ptr<Expression>());

    box->deleteLater();
}

void DoubleSpinBox::keyPressEvent(QKeyEvent *event)
{
    if (event->text() == QString::fromUtf8("=") && isBound())
        openFormulaDialog();
    else {
        if (!hasExpression())
            QAbstractSpinBox::keyPressEvent(event);
    }
}

#include "moc_SpinBox.cpp"
