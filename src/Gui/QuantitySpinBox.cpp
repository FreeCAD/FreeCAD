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
# include <QApplication>
# include <QDebug>
# include <QFocusEvent>
# include <QFontMetrics>
# include <QHBoxLayout>
# include <QLabel>
# include <QLineEdit>
# include <QMouseEvent>
# include <QPixmapCache>
# include <QStyle>
# include <QStyleOptionSpinBox>
# include <QStylePainter>
# include <QToolTip>
#endif

#include "QuantitySpinBox.h"
#include "QuantitySpinBox_p.h"
#include "DlgExpressionInput.h"
#include "propertyeditor/PropertyItem.h"
#include "BitmapFactory.h"
#include "Tools.h"
#include "Command.h"
#include <Base/Tools.h>
#include <Base/Exception.h>
#include <Base/UnitsApi.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <App/PropertyGeo.h>
#include <sstream>
#include <boost/math/special_functions/round.hpp>

using namespace Gui;
using namespace App;
using namespace Base;

namespace Gui {

class QuantitySpinBoxPrivate
{
public:
    QuantitySpinBoxPrivate(QuantitySpinBox *q) :
      validInput(true),
      pendingEmit(false),
      unitValue(0),
      maximum(DOUBLE_MAX),
      minimum(-DOUBLE_MAX),
      singleStep(1.0),
      q_ptr(q)
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

    bool validate(QString& input, Base::Quantity& result) const
    {
        Q_Q(const QuantitySpinBox);

        // Do not accept empty strings because the parser will consider
        // " unit" as "1 unit" which is not the desired behaviour (see #0004104)
        if (input.isEmpty())
            return false;

        bool success = false;
        QString tmp = input;

        auto validateInput = [&](QString& tmp) -> QValidator::State {
            int pos = 0;
            QValidator::State state;
            Base::Quantity res = validateAndInterpret(tmp, pos, state);
            res.setFormat(quantity.getFormat());
            if (state == QValidator::Acceptable) {
                success = true;
                result = res;
                input = tmp;
            }
            return state;
        };

        QValidator::State state = validateInput(tmp);
        if (state == QValidator::Intermediate) {
            if (!q->hasExpression()) {
                tmp = tmp.trimmed();
                tmp += QLatin1Char(' ');
                tmp += unitStr;
                validateInput(tmp);
            }
            else {
                // Accept the expression as it is but try to add the right unit string
                success = true;

                Base::Quantity quantity;
                double value;
                if (parseString(input, quantity, value)) {
                    quantity.setUnit(unit);
                    result = quantity;

                    // Now translate the quantity into its string representation using the user-defined unit system
                    input = Base::UnitsApi::schemaTranslate(result);
                }
            }
        }

        return success;
    }
    bool parseString(const QString& str, Base::Quantity& result, double& value) const
    {
        try {
            QString copy = str;
            copy.remove(locale.groupSeparator());

            result = Base::Quantity::parse(copy);
            value = result.getValue();
            return true;
        }
        catch (Base::ParserError&) {
            return false;
        }
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
                bool decOccurred = false;
                for (int i = 0; i<copy.size(); i++) {
                    if (copy.at(i) == locale.decimalPoint()) {
                        // Disallow multiple decimal points within the same numeric substring
                        if (decOccurred) {
                            state = QValidator::Invalid;
                            goto end;
                        }
                        decOccurred = true;
                    }
                    // Reset decOcurred if non-numeric character found
                    else if (!(copy.at(i) == locale.groupSeparator() || copy.at(i).isDigit())) {
                        decOccurred = false;
                    }
                }
            }

            if (locale.negativeSign() != QLatin1Char('-'))
                copy.replace(locale.negativeSign(), QLatin1Char('-'));
            if (locale.positiveSign() != QLatin1Char('+'))
                copy.replace(locale.positiveSign(), QLatin1Char('+'));

            double value = min;
            bool ok = parseString(copy, res, value);

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
                    // If the user input is of the form "number * unit", "number + unit"
                    // or "number - unit" it's rejected by the quantity parser and it's
                    // assumed that the user input is not complete yet (Intermediate).
                    // However, if the user input is of the form "number / unit" it's accepted
                    // by the parser but because the units mismatch it's considered as invalid
                    // and the last valid input will be restored.
                    // See #0004422: PartDesign value input does not accept trailing slash
                    // To work around this issue of the quantity parser it's checked if the
                    // inversed unit matches and if yes the input is also considered as not
                    // complete.
                    if (res.getUnit().pow(-1) == this->unit)
                        state = QValidator::Intermediate;
                    else
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
    bool pendingEmit;
    QString validStr;
    Base::Quantity quantity;
    Base::Quantity cached;
    Base::Unit unit;
    double unitValue;
    QString unitStr;
    double maximum;
    double minimum;
    double singleStep;
    QuantitySpinBox *q_ptr;
    std::unique_ptr<Base::UnitsSchema> scheme;
    Q_DECLARE_PUBLIC(QuantitySpinBox)
};
}

QuantitySpinBox::QuantitySpinBox(QWidget *parent)
    : QAbstractSpinBox(parent),
      ExpressionSpinBox(this),
      d_ptr(new QuantitySpinBoxPrivate(this))
{
    d_ptr->locale = locale();
    this->setContextMenuPolicy(Qt::DefaultContextMenu);
    QObject::connect(lineEdit(), SIGNAL(textChanged(QString)),
                     this, SLOT(userInput(QString)));
    QObject::connect(this, SIGNAL(editingFinished()),
                     this, SLOT(handlePendingEmit()));

    // When a style sheet is set the text margins for top/bottom must be set to avoid to squash the widget
#ifndef Q_OS_MAC
    lineEdit()->setTextMargins(0, 2, 0, 2);
#else
    // https://forum.freecadweb.org/viewtopic.php?f=8&t=50615
    lineEdit()->setTextMargins(0, 2, 0, 0);
#endif
}

QuantitySpinBox::~QuantitySpinBox()
{
}

void QuantitySpinBox::bind(const App::ObjectIdentifier &_path)
{
    ExpressionBinding::bind(_path);

    iconLabel->show();
}

QString QuantitySpinBox::boundToName() const
{
    if (isBound()) {
        std::string path = getPath().toString();
        return QString::fromStdString(path);
    }
    return QString();
}

/**
 * @brief Create an object identifier by name.
 *
 * An identifier is written as document#documentobject.property.subproperty1...subpropertyN
 * document# may be dropped, in this case the active document is used.
 */
void QuantitySpinBox::setBoundToByName(const QString &name)
{
    try {
        // get document
        App::Document *doc = App::GetApplication().getActiveDocument();
        QStringList list = name.split(QLatin1Char('#'));
        if (list.size() > 1) {
            doc = App::GetApplication().getDocument(list.front().toLatin1());
            list.pop_front();
        }

        if (!doc) {
            qDebug() << "No such document";
            return;
        }

        // first element is assumed to be the document name
        list = list.front().split(QLatin1Char('.'));

        // get object
        App::DocumentObject* obj = doc->getObject(list.front().toLatin1());
        if (!obj) {
            qDebug() << "No object " << list.front() << " in document";
            return;
        }
        list.pop_front();

        // the rest of the list defines the property and eventually subproperties
        App::ObjectIdentifier path(obj);
        path.setDocumentName(std::string(doc->getName()), true);
        path.setDocumentObjectName(std::string(obj->getNameInDocument()), true);

        for (QStringList::iterator it = list.begin(); it != list.end(); ++it) {
            path << App::ObjectIdentifier::Component::SimpleComponent(it->toLatin1().constData());
        }

        if (path.getProperty())
            bind(path);
    }
    catch (const Base::Exception& e) {
        qDebug() << e.what();
    }
}

QString Gui::QuantitySpinBox::expressionText() const
{
    try {
        if (hasExpression()) {
            return QString::fromStdString(getExpressionString());
        }
    }
    catch (const Base::Exception& e) {
        qDebug() << e.what();
    }
    return QString();
}

void Gui::QuantitySpinBox::setNumberExpression(App::NumberExpression* expr)
{
    lineEdit()->setText(getUserString(expr->getQuantity()));
    handlePendingEmit();
}

bool QuantitySpinBox::apply(const std::string & propName)
{
    if (!ExpressionBinding::apply(propName)) {
        double dValue = value().getValue();
        if (isBound()) {
            const App::ObjectIdentifier & path = getPath();
            const Property * prop = path.getProperty();

            /* Skip update if property is bound and we know it is read-only */
            if (prop && prop->isReadOnly())
                return true;

            if (prop && prop->getTypeId().isDerivedFrom(App::PropertyPlacement::getClassTypeId())) {
                std::string p = path.getSubPathStr();
                if (p == ".Rotation.Angle") {
                    dValue = Base::toRadians(dValue);
                }
            }
        }

        Gui::Command::doCommand(Gui::Command::Doc, "%s = %f", propName.c_str(), dValue);
        return true;
    }
    else
        return false;
}

void QuantitySpinBox::resizeEvent(QResizeEvent * event)
{
    QAbstractSpinBox::resizeEvent(event);
    resizeWidget();
}

void Gui::QuantitySpinBox::keyPressEvent(QKeyEvent *event)
{
    if (!handleKeyEvent(event->text()))
        QAbstractSpinBox::keyPressEvent(event);
}

void Gui::QuantitySpinBox::paintEvent(QPaintEvent*)
{
    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    drawControl(opt);
}

void QuantitySpinBox::updateText(const Quantity &quant)
{
    Q_D(QuantitySpinBox);

    double dFactor;
    QString txt = getUserString(quant, dFactor, d->unitStr);
    d->unitValue = quant.getValue()/dFactor;
    lineEdit()->setText(txt);
    handlePendingEmit();
}

void QuantitySpinBox::validateInput()
{
    Q_D(QuantitySpinBox);

    int pos = 0;
    QValidator::State state;
    QString text = lineEdit()->text();
    d->validateAndInterpret(text, pos, state);
    if (state != QValidator::Acceptable) {
        lineEdit()->setText(d->validStr);
    }

    handlePendingEmit();
}

Base::Quantity QuantitySpinBox::value() const
{
    Q_D(const QuantitySpinBox);
    return d->quantity;
}

double QuantitySpinBox::rawValue() const
{
    Q_D(const QuantitySpinBox);
    return d->quantity.getValue();
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

    d->pendingEmit = true;

    QString tmp = text;
    Base::Quantity res;
    if (d->validate(tmp, res)) {
        d->validStr = tmp;
        d->validInput = true;
    }
    else {
        d->validInput = false;
        return;
    }

    if (keyboardTracking()) {
        d->cached = res;
        handlePendingEmit(false);
    }
    else {
        d->cached = res;
    }
}

void QuantitySpinBox::openFormulaDialog()
{
    Q_ASSERT(isBound());

    Q_D(const QuantitySpinBox);
    Gui::Dialog::DlgExpressionInput* box = new Gui::Dialog::DlgExpressionInput(getPath(), getExpression(), d->unit, this);
    QObject::connect(box, &Gui::Dialog::DlgExpressionInput::finished, [=]() {
        if (box->result() == QDialog::Accepted)
            setExpression(box->getExpression());
        else if (box->discardedFormula())
            setExpression(std::shared_ptr<Expression>());

        box->deleteLater();
        Q_EMIT showFormulaDialog(false);
    });
    box->show();

    QPoint pos = mapToGlobal(QPoint(0,0));
    box->move(pos-box->expressionPosition());
    box->setExpressionInputSize(width(), height());

    Q_EMIT showFormulaDialog(true);
}

void QuantitySpinBox::handlePendingEmit(bool updateUnit /* = true */)
{
    updateFromCache(true, updateUnit);
}

void QuantitySpinBox::updateFromCache(bool notify, bool updateUnit /* = true */)
{
    Q_D(QuantitySpinBox);
    if (d->pendingEmit) {
        double factor;
        const Base::Quantity& res = d->cached;
        auto tmpUnit(d->unitStr);
        QString text = getUserString(res, factor, updateUnit ? d->unitStr : tmpUnit);
        d->unitValue = res.getValue() / factor;
        d->quantity = res;

        // signaling
        if (notify) {
            d->pendingEmit = false;
            valueChanged(res);
            valueChanged(res.getValue());
            textChanged(text);
        }
    }
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
    try {
        Base::Quantity quant = Base::Quantity::parse(str);
        setUnit(quant.getUnit());
    }
    catch (const Base::ParserError&) {
    }
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

int QuantitySpinBox::decimals() const
{
    Q_D(const QuantitySpinBox);
    return d->quantity.getFormat().precision;
}

void QuantitySpinBox::setDecimals(int v)
{
    Q_D(QuantitySpinBox);
    Base::QuantityFormat f = d->quantity.getFormat();
    f.precision = v;
    d->quantity.setFormat(f);
    updateText(d->quantity);
}

void QuantitySpinBox::setSchema(const Base::UnitSystem& s)
{
    Q_D(QuantitySpinBox);
    d->scheme = Base::UnitsApi::createSchema(s);
    updateText(d->quantity);
}

void QuantitySpinBox::clearSchema()
{
    Q_D(QuantitySpinBox);
    d->scheme = nullptr;
    updateText(d->quantity);
}

QString QuantitySpinBox::getUserString(const Base::Quantity& val, double& factor, QString& unitString) const
{
    Q_D(const QuantitySpinBox);
    if (d->scheme) {
        return val.getUserString(d->scheme.get(), factor, unitString);
    }
    else {
        return val.getUserString(factor, unitString);
    }
}

QString QuantitySpinBox::getUserString(const Base::Quantity& val) const
{
    Q_D(const QuantitySpinBox);
    if (d->scheme) {
        double factor;
        QString unitString;
        return val.getUserString(d->scheme.get(), factor, unitString);
    }
    else {
        return val.getUserString();
    }
}

void QuantitySpinBox::setExpression(std::shared_ptr<Expression> expr)
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
    updateFromCache(false);

    double step = d->singleStep * steps;
    double val = d->unitValue + step;
    if (val > d->maximum)
        val = d->maximum;
    else if (val < d->minimum)
        val = d->minimum;

    Quantity quant(val, d->unitStr);
    updateText(quant);
    updateFromCache(true);
    update();
    selectNumber();
}

QSize QuantitySpinBox::sizeHint() const
{
    Q_D(const QuantitySpinBox);
    ensurePolished();

    const QFontMetrics fm(fontMetrics());
    int h = lineEdit()->sizeHint().height();
    int w = 0;

    QString s;
    QString fixedContent = QLatin1String(" ");

    Base::Quantity q(d->quantity);
    q.setValue(d->maximum);
    s = textFromValue(q);
    s.truncate(18);
    s += fixedContent;
    w = qMax(w, QtTools::horizontalAdvance(fm, s));

    w += 2; // cursor blinking space
    w += iconHeight;

    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    QSize hint(w, h);
    QSize size = style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this)
                        .expandedTo(QApplication::globalStrut());
    return size;
}

QSize QuantitySpinBox::minimumSizeHint() const
{
    Q_D(const QuantitySpinBox);
    ensurePolished();

    const QFontMetrics fm(fontMetrics());
    int h = lineEdit()->minimumSizeHint().height();
    int w = 0;

    QString s;
    QString fixedContent = QLatin1String(" ");

    Base::Quantity q(d->quantity);
    q.setValue(d->maximum);
    s = textFromValue(q);
    s.truncate(18);
    s += fixedContent;
    w = qMax(w, QtTools::horizontalAdvance(fm, s));

    w += 2; // cursor blinking space
    w += iconHeight;

    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    QSize hint(w, h);

    QSize size = style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this)
                               .expandedTo(QApplication::globalStrut());
    return size;
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

void QuantitySpinBox::hideEvent(QHideEvent * event)
{
    handlePendingEmit();
    QAbstractSpinBox::hideEvent(event);
}

void QuantitySpinBox::closeEvent(QCloseEvent * event)
{
    handlePendingEmit();
    QAbstractSpinBox::closeEvent(event);
}

bool QuantitySpinBox::event(QEvent * event)
{
    // issue #0004059: Tooltips for Gui::QuantitySpinBox not showing
    // Here we must not try to show the tooltip of the icon label
    // because it would override a custom tooltip set to this widget.
    //
    // We could also check if the text of this tooltip is empty but
    // it will fail in cases where the widget is embedded into the
    // property editor and the corresponding item has set a tooltip.
    // Instead of showing the item's tooltip it will again show the
    // tooltip of the icon label.
#if 0
    if (event->type() == QEvent::ToolTip) {
        if (isBound() && getExpression() && lineEdit()->isReadOnly()) {
            QHelpEvent * helpEvent = static_cast<QHelpEvent*>(event);

            QToolTip::showText( helpEvent->globalPos(), Base::Tools::fromStdString(getExpression()->toString()), this);
            event->accept();
            return true;
        }
    }
#endif

    return QAbstractSpinBox::event(event);
}

void QuantitySpinBox::focusInEvent(QFocusEvent * event)
{
    bool hasSel = lineEdit()->hasSelectedText();
    QAbstractSpinBox::focusInEvent(event);

    if (event->reason() == Qt::TabFocusReason ||
        event->reason() == Qt::BacktabFocusReason  ||
        event->reason() == Qt::ShortcutFocusReason) {

        if (isBound() && getExpression() && lineEdit()->isReadOnly()) {
            QHelpEvent * helpEvent = new QHelpEvent(QEvent::ToolTip, QPoint( 0, rect().height() ), mapToGlobal( QPoint( 0, rect().height() ) ));
            QApplication::postEvent(this, helpEvent);
            lineEdit()->setSelection(0, 0);
        }
        else {
            if (!hasSel)
                selectNumber();
        }
    }
}

void QuantitySpinBox::focusOutEvent(QFocusEvent * event)
{
    validateInput();

    QToolTip::hideText();
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
    QString str = getUserString(value, factor, unitStr);
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
#include "moc_QuantitySpinBox_p.cpp"
