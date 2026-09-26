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

#include <limits>
#include <QApplication>
#include <QDebug>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QLineEdit>
#include <QtCore/QScopedValueRollback>
#include <QStyle>
#include <QStyleOptionSpinBox>
#include <QToolTip>

#include <sstream>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <App/QuantityInput.h>
#include <Base/Exception.h>
#include <Base/NumericFormatting.h>
#include <Base/NumericInput.h>
#include <Base/UnitsApi.h>
#include <Base/UnitsSchema.h>

#include "QuantitySpinBox.h"
#include "QuantitySpinBox_p.h"
#include "Command.h"
#include "Dialogs/DlgExpressionInput.h"
#include "NumericLocale.h"
#include "Tools.h"
#include "Widgets.h"


using namespace Gui;
using namespace App;
using namespace Base;

namespace Gui
{

class QuantitySpinBoxPrivate
{
public:
    QuantitySpinBoxPrivate(QuantitySpinBox* q)
        : validInput(true)
        , pendingEmit(false)
        , updatingText(false)
        , normalize(true)
        , checkRangeInExpression(false)
        , adjustableWidth(false)
        , maxExpectedDigits(4)
        , addIconSpace(false)
        , displayUnit(Base::Unit::One)
        , maximum(std::numeric_limits<double>::max())
        , minimum(-std::numeric_limits<double>::max())
        , singleStep(1.0)
        , q_ptr(q)
    {}
    ~QuantitySpinBoxPrivate() = default;

    App::QuantityInputResult interpretInput(
        const QString& input,
        const App::ObjectIdentifier& path,
        const App::QuantityInputGrammar grammar,
        const App::InputPhase phase
    ) const
    {
        Q_Q(const QuantitySpinBox);
        App::QuantityConstraints constraints;
        if (unit != Base::Unit::One) {
            constraints.requiredUnit = unit;
        }
        constraints.minimum = minimum;
        constraints.maximum = maximum;
        const auto parse = [&](const App::QuantityInputGrammar selectedGrammar) {
            return App::interpretQuantityInput(
                input.toUtf8().toStdString(),
                selectedGrammar,
                path,
                displayUnit,
                Gui::numericLocaleContextFor(q->locale()),
                phase,
                constraints
            );
        };

        auto result = parse(grammar);
        // Preserve the quantity parser as the authority for quantity-only syntax, while keeping
        // the established unbound-field support for arithmetic expressions. A parser failure is
        // the only case that permits the expression grammar fallback; malformed numbers and
        // incompatible units must retain their original diagnostics.
        if (grammar == App::QuantityInputGrammar::Quantity
            && result.status == App::InputStatus::Invalid && result.diagnostic
            && result.diagnostic->kind == App::InputDiagnosticKind::ExpressionSyntax) {
            result = parse(App::QuantityInputGrammar::Expression);
        }
        return result;
    }

    QLocale locale;
    bool validInput;
    bool pendingEmit;
    bool updatingText;
    bool normalize;
    bool checkRangeInExpression;
    bool adjustableWidth;
    int maxExpectedDigits;
    bool addIconSpace;
    QString validStr;
    QString lastRejectedText;
    Base::Quantity quantity;
    Base::Quantity cached;
    Base::Unit unit;
    App::QuantityInputUnit displayUnit;
    double maximum;
    double minimum;
    double singleStep;
    QuantitySpinBox* q_ptr;
    std::unique_ptr<Base::UnitsSchema> scheme;
    Q_DECLARE_PUBLIC(QuantitySpinBox)
};
}  // namespace Gui

QuantitySpinBox::QuantitySpinBox(QWidget* parent)
    : QAbstractSpinBox(parent)
    , ExpressionSpinBox(this)
    , d_ptr(new QuantitySpinBoxPrivate(this))
{
    d_ptr->locale = locale();
    this->setContextMenuPolicy(Qt::DefaultContextMenu);
    connect(lineEdit(), &QLineEdit::textChanged, this, &QuantitySpinBox::userInput);
}

QuantitySpinBox::~QuantitySpinBox() = default;

void QuantitySpinBox::bind(const App::ObjectIdentifier& _path)
{
    ExpressionSpinBox::bind(_path);
}

void QuantitySpinBox::showIcon()
{
    addIconSpace(true);

    adjustSize();
    iconLabel->show();
}

QString QuantitySpinBox::boundToName() const
{
    if (isBound()) {
        std::string path = getPath().toString();
        return QString::fromStdString(path);
    }
    return {};
}

/**
 * @brief Create an object identifier by name.
 *
 * An identifier is written as document#documentobject.property.subproperty1...subpropertyN
 * document# may be dropped, in this case the active document is used.
 */
void QuantitySpinBox::setBoundToByName(const QString& name)
{
    try {
        // get document
        App::Document* doc = App::GetApplication().getActiveDocument();
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

        for (const auto& it : list) {
            path << App::ObjectIdentifier::Component::SimpleComponent(it.toLatin1().constData());
        }

        if (path.getProperty()) {
            bind(path);
        }
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
    return {};
}

void QuantitySpinBox::evaluateExpression()
{
    if (isBound() && getExpression()) {
        showValidExpression(Number::SetIfNumber);
    }
}

void Gui::QuantitySpinBox::setNumberExpression(App::NumberExpression* expr)
{
    commitQuantity(expr->getQuantity(), TextPolicy::ReformatEditor, false);
}

bool QuantitySpinBox::apply(const std::string& propName)
{
    if (!ExpressionBinding::apply(propName)) {
        double dValue = value().getValue();
        return assignToProperty(propName, dValue);
    }

    return false;
}

void QuantitySpinBox::resizeEvent(QResizeEvent* event)
{
    QAbstractSpinBox::resizeEvent(event);
    resizeWidget();
}

void Gui::QuantitySpinBox::keyPressEvent(QKeyEvent* event)
{
    Q_D(QuantitySpinBox);

    const auto isEnter = event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return;

    if (event->key() == Qt::Key_Escape) {
        d->pendingEmit = false;
        d->validInput = true;
        d->lastRejectedText.clear();
        QToolTip::hideText();
        lineEdit()->setToolTip(QString());
        lineEdit()->setProperty("numericInputInvalid", false);
        {
            const QSignalBlocker blocker(lineEdit());
            updateText(d->quantity);
        }
        // Restore the editor here, then leave cancellation to an enclosing task panel.
        event->ignore();
        return;
    }

    if (isEnter) {
        validateInput();
        if (d->validInput && d->normalize && !isNormalized()) {
            normalize();
        }
        if (d->validInput) {
            // This handler deliberately consumes Return after committing the text. Preserve the
            // QAbstractSpinBox signal contract for callers that use editingFinished().
            Q_EMIT returnPressed();
            Q_EMIT editingFinished();
            // A successful local commit must not hide Return from an enclosing task panel.
            event->ignore();
        }
        else {
            // Rejected input must not accept the surrounding task.
            event->accept();
        }
        return;
    }

    if (!handleKeyEvent(event->text())) {
        QAbstractSpinBox::keyPressEvent(event);
    }
}

void Gui::QuantitySpinBox::paintEvent(QPaintEvent*)
{
    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    drawControl(opt);
}

void QuantitySpinBox::commitQuantity(Base::Quantity quantity, const TextPolicy textPolicy, const bool notify)
{
    Q_D(QuantitySpinBox);

    quantity.setFormat(d->quantity.getFormat());

    // Limits are stored in canonical units, just like Base::Quantity values.
    if (quantity.getValue() > d->maximum) {
        quantity.setValue(d->maximum);
    }
    if (quantity.getValue() < d->minimum) {
        quantity.setValue(d->minimum);
    }

    d->quantity = quantity;
    d->cached = quantity;
    d->pendingEmit = false;
    d->validInput = true;
    d->lastRejectedText.clear();
    lineEdit()->setToolTip(QString());
    lineEdit()->setProperty("numericInputInvalid", false);

    if (textPolicy == TextPolicy::ReformatEditor) {
        updateText(quantity);
    }
    else {
        // The editor contains the user's candidate text. It is already known to represent the
        // committed quantity, so keep it verbatim while making it the validated text.
        d->validStr = lineEdit()->text();
    }

    if (notify) {
        Q_EMIT valueChanged(quantity);
        Q_EMIT valueChanged(quantity.getValue());

        // Preserve QuantitySpinBox's custom textChanged signal without sending generated text
        // back through userInput(). Rendering and semantic commits are deliberately separate.
        QScopedValueRollback<bool> updatingGuard(d->updatingText, true);
        Q_EMIT textChanged(lineEdit()->text());
    }
}

void QuantitySpinBox::updateText(const Quantity& quant)
{
    Q_D(QuantitySpinBox);

    // Rendering updates the editor and the paired displayed-unit state only. It must never use
    // the generated text as a way to mutate or notify the semantic quantity.
    double displayFactor;
    QString displayUnit;
    QString txt = getUserString(quant, displayFactor, displayUnit);
    Q_UNUSED(displayFactor);
    d->displayUnit = App::QuantityInputUnit(quant.getUnit(), displayUnit.toStdString());
    updateEdit(txt);
    d->validStr = txt;
    d->validInput = true;
    d->lastRejectedText.clear();
}

void QuantitySpinBox::updateEdit(const QString& text)
{
    Q_D(QuantitySpinBox);
    QLineEdit* edit = lineEdit();

    int cursor = edit->cursorPosition();
    int selStart = edit->selectionStart();
    int selLen = edit->selectionLength();

    // setText resets cursor/selection so save it and restore it
    // A schema can intentionally display a precise quantity in a coarser user unit (for
    // example, 12345.67 mm as 12.35 m). Do not feed that display representation back through
    // the input parser or the stored quantity will change when the editor updates itself.
    QScopedValueRollback<bool> updatingGuard(d->updatingText, true);
    edit->setText(text);

    const QString displayUnit = QString::fromStdString(d->displayUnit.getSymbol());
    int maxPos = qMax(0, edit->displayText().size() - displayUnit.size());

    int newCursor = qBound(0, cursor, maxPos);

    if (selLen > 0) {
        int newStart = qBound(0, selStart, maxPos);
        int newLen = qBound(0, selLen, maxPos - newStart);
        edit->setSelection(newStart, newLen);
    }
    else {
        edit->setCursorPosition(newCursor);
    }
}

void QuantitySpinBox::validateInput()
{
    Q_D(QuantitySpinBox);

    const QString text = lineEdit()->text();
    if (d->validInput && !d->pendingEmit && !text.isEmpty() && text == d->validStr) {
        return;
    }
    const App::ObjectIdentifier& path = getPath();
    const auto grammar = isBound() ? App::QuantityInputGrammar::Expression
                                   : App::QuantityInputGrammar::Quantity;
    const auto result = d->interpretInput(text, path, grammar, App::InputPhase::Commit);
    if (result.status == App::InputStatus::Acceptable) {
        const bool needsEmit = !d->validInput || d->validStr != text || d->pendingEmit;
        commitQuantity(
            *result.quantity,
            keyboardTracking() ? TextPolicy::PreserveEditorText : TextPolicy::ReformatEditor,
            needsEmit
        );
        return;
    }

    d->pendingEmit = false;
    d->validInput = false;

    if (!result.diagnostic || d->lastRejectedText == text) {
        return;
    }

    const QString message = Gui::numericInputDiagnosticText(result.diagnostic->kind);

    // Input errors use a dedicated line-edit state. The expression icon belongs to the formula
    // editor and showing it here changes the text margin and leaves stale UI state behind.
    lineEdit()->setProperty("numericInputInvalid", true);
    lineEdit()->setToolTip(message);
    const auto& diagnostic = *result.diagnostic;
    const QByteArray utf8 = text.toUtf8();
    const auto offsetBytes = qMin<int>(static_cast<int>(diagnostic.offsetBytes), utf8.size());
    const auto lengthBytes
        = qMin<int>(static_cast<int>(diagnostic.lengthBytes), utf8.size() - offsetBytes);
    const int errorStartUtf16 = QString::fromUtf8(utf8.constData(), offsetBytes).size();
    const int errorLengthUtf16 = QString::fromUtf8(utf8.mid(offsetBytes, lengthBytes)).size();
    lineEdit()->setSelection(errorStartUtf16, errorLengthUtf16);
    QToolTip::showText(lineEdit()->mapToGlobal(QPoint(0, lineEdit()->height())), message, lineEdit());
    d->lastRejectedText = text;
    Q_EMIT inputRejected(message, errorStartUtf16, errorLengthUtf16);
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

void QuantitySpinBox::normalize()
{
    // this does not really change the value, only the representation
    QSignalBlocker blocker(this);

    Q_D(const QuantitySpinBox);
    return setValue(d->quantity);
}

bool QuantitySpinBox::isNormalized()
{
    Q_D(const QuantitySpinBox);

    // check if the input is exactly the same as the normalized string
    if (d->validStr.toStdString() == d->quantity.getUserString()) {
        return true;
    }

    // check if the input is simplified to a solution or if further calculation
    // has to be done

    try {
        auto expr = ExpressionParser::parse(
            getPath().getDocumentObject(),
            d->validStr.toUtf8().constData()
        );

        // plain numbers
        if (freecad_cast<NumberExpression*>(expr.get())) {
            return true;
        }

        auto operatorExpr = freecad_cast<OperatorExpression*>(expr.get());
        if (!operatorExpr) {
            return false;
        }

        if (operatorExpr->getOperator() == OperatorExpression::UNIT
            && freecad_cast<UnitExpression*>(operatorExpr->getRight())
            && freecad_cast<NumberExpression*>(operatorExpr->getLeft())) {
            // numbers without sign but with unit
            return true;
        }

        if ((operatorExpr->getOperator() != OperatorExpression::NEG
             && operatorExpr->getOperator() != OperatorExpression::POS)) {
            return false;
        }

        // numbers with positive or negative sign without unit
        if (freecad_cast<NumberExpression*>(operatorExpr->getLeft())) {
            return true;
        }

        auto innerOperatorExpr = freecad_cast<OperatorExpression*>(operatorExpr->getLeft());
        if (!innerOperatorExpr) {
            return false;
        }

        if (innerOperatorExpr->getOperator() != OperatorExpression::UNIT) {
            return false;
        }
        if (!freecad_cast<UnitExpression*>(innerOperatorExpr->getRight())) {
            return false;
        }

        // numbers with positive or negative sign and unit
        auto left = innerOperatorExpr->getLeft();
        if (freecad_cast<NumberExpression*>(left)) {
            return true;
        }
        auto leftOp = freecad_cast<OperatorExpression*>(left);
        if (leftOp
            && (leftOp->getOperator() == OperatorExpression::NEG
                || leftOp->getOperator() == OperatorExpression::POS)
            && freecad_cast<NumberExpression*>(leftOp->getLeft())) {
            return true;
        }
    }
    catch (const Base::Exception&) {
        // The exception is intentionally ignored here and should be handled,
        // when the value is assigned
        return false;
    }
    return false;
}

void QuantitySpinBox::setValue(const Base::Quantity& value)
{
    Q_D(QuantitySpinBox);
    const bool changed = value.getValue() != d->quantity.getValue()
        || value.getUnit() != d->quantity.getUnit();
    d->unit = value.getUnit();
    // Preserve the format supplied by programmatic callers. Parsed and stepped values instead
    // inherit the format of the already committed quantity in commitQuantity().
    d->quantity.setFormat(value.getFormat());
    // Programmatic updates are semantic changes too. Signal blockers remain the explicit way for
    // callers that need to update the widget without notifying connected model/view code.
    commitQuantity(value, TextPolicy::ReformatEditor, changed);
}

void QuantitySpinBox::setValue(double value)
{
    Q_D(QuantitySpinBox);

    Base::QuantityFormat currentformat = d->quantity.getFormat();
    auto quantity = Base::Quantity(value, d->unit);
    quantity.setFormat(currentformat);

    setValue(quantity);
}

bool QuantitySpinBox::autoNormalize() const
{
    Q_D(const QuantitySpinBox);
    return d->normalize;
}

void QuantitySpinBox::setAutoNormalize(bool normalize)
{
    Q_D(QuantitySpinBox);
    d->normalize = normalize;
}

bool QuantitySpinBox::autoAdjustWidth() const
{
    Q_D(const QuantitySpinBox);
    return d->adjustableWidth;
}

void QuantitySpinBox::setAutoAdjustWidth(bool adjust)
{
    Q_D(QuantitySpinBox);
    d->adjustableWidth = adjust;
}

bool QuantitySpinBox::isIconSpaceAdded() const
{
    Q_D(const QuantitySpinBox);
    return d->addIconSpace;
}

void QuantitySpinBox::addIconSpace(bool addIconSpace)
{
    Q_D(QuantitySpinBox);
    d->addIconSpace = addIconSpace;
}

int QuantitySpinBox::getMaxExpectedDigits()
{
    Q_D(const QuantitySpinBox);
    return d->maxExpectedDigits;
}

void QuantitySpinBox::setMaxExpectedDigits(int digits)
{
    Q_D(QuantitySpinBox);
    d->maxExpectedDigits = digits;
}

bool QuantitySpinBox::hasValidInput() const
{
    Q_D(const QuantitySpinBox);
    return d->validInput;
}

// Parse edits without changing the last committed quantity until the edit is complete.
void QuantitySpinBox::userInput(const QString& text)
{
    Q_D(QuantitySpinBox);
    if (d->updatingText) {
        return;
    }

    const App::ObjectIdentifier& path = getPath();
    const auto grammar = isBound() ? App::QuantityInputGrammar::Expression
                                   : App::QuantityInputGrammar::Quantity;
    const auto result = d->interpretInput(text, path, grammar, App::InputPhase::Editing);
    if (text.trimmed().isEmpty()) {
        Q_EMIT inputCleared();
    }
    d->lastRejectedText.clear();
    QToolTip::hideText();
    lineEdit()->setToolTip(QString());
    lineEdit()->setProperty("numericInputInvalid", false);

    if (result.status == App::InputStatus::Acceptable) {
        auto quantity = *result.quantity;
        quantity.setFormat(d->quantity.getFormat());

        if (keyboardTracking()) {
            commitQuantity(quantity, TextPolicy::PreserveEditorText, true);
        }
        else {
            d->cached = quantity;
            d->pendingEmit = true;
            d->validStr = text;
            d->validInput = true;
        }
    }
    else {
        d->pendingEmit = false;
        d->validInput = false;
    }
}

void QuantitySpinBox::openFormulaDialog()
{
    Q_ASSERT(isBound());

    Q_EMIT showFormulaDialog(true);

    Q_D(const QuantitySpinBox);
    auto box = new Gui::Dialog::DlgExpressionInput(getPath(), getExpression(), d->unit, this);
    if (d->checkRangeInExpression) {
        box->setRange(d->minimum, d->maximum);
    }
    QObject::connect(box, &Gui::Dialog::DlgExpressionInput::finished, [this, box]() {
        if (box->result() == QDialog::Accepted) {
            setExpression(box->getExpression());
        }
        else if (box->discardedFormula()) {
            setExpression(std::shared_ptr<Expression>());
        }

        updateExpression();
        box->deleteLater();
        Q_EMIT showFormulaDialog(false);
    });
    box->show();

    QPoint pos = mapToGlobal(QPoint(0, 0));
    box->move(pos - box->expressionPosition());
    Gui::adjustDialogPosition(box);
}

void QuantitySpinBox::handlePendingEmit(bool updateUnit /* = true */)
{
    updateFromCache(true, updateUnit);
}

void QuantitySpinBox::updateFromCache(bool notify, bool updateUnit /* = true */)
{
    Q_D(QuantitySpinBox);
    if (d->pendingEmit) {
        commitQuantity(
            d->cached,
            updateUnit ? TextPolicy::ReformatEditor : TextPolicy::PreserveEditorText,
            notify
        );
    }
}

Base::Unit QuantitySpinBox::unit() const
{
    Q_D(const QuantitySpinBox);
    return d->unit;
}

void QuantitySpinBox::setUnit(const Base::Unit& unit)
{
    Q_D(QuantitySpinBox);

    d->unit = unit;
    auto quantity = d->quantity;
    quantity.setUnit(unit);
    commitQuantity(quantity, TextPolicy::ReformatEditor, false);
}

void QuantitySpinBox::setUnitText(const QString& str)
{
    try {
        Base::Quantity quant = Base::Quantity::parse(str.toStdString());
        setUnit(quant.getUnit());
    }
    catch (const Base::ParserError&) {
    }
}

QString QuantitySpinBox::unitText()
{
    Q_D(QuantitySpinBox);
    return QString::fromStdString(d->displayUnit.getSymbol());
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

void QuantitySpinBox::checkRangeInExpression(bool on)
{
    Q_D(QuantitySpinBox);
    d->checkRangeInExpression = on;
}

bool QuantitySpinBox::isCheckedRangeInExpresion() const
{
    Q_D(const QuantitySpinBox);
    return d->checkRangeInExpression;
}


int QuantitySpinBox::decimals() const
{
    Q_D(const QuantitySpinBox);
    return d->quantity.getFormat().getPrecision();
}

void QuantitySpinBox::setDecimals(int v)
{
    Q_D(QuantitySpinBox);
    Base::QuantityFormat f = d->quantity.getFormat();
    f.setPrecision(v);
    d->quantity.setFormat(f);
    updateText(d->quantity);
}

void QuantitySpinBox::setSchema(const int s)
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
    const auto formatting = Gui::numericLocaleContextFor(locale());
    auto displayQuantity = val;
    displayQuantity.setFormat(Gui::editableQuantityFormat(val.getFormat(), formatting));
    std::string unitStr;
    const std::string str = d->scheme
        ? d->scheme->translate(displayQuantity, formatting, factor, unitStr)
        : Base::UnitsApi::schemaTranslate(displayQuantity, formatting, factor, unitStr);
    unitString = QString::fromStdString(unitStr);
    return QString::fromStdString(str);
}

QString QuantitySpinBox::getUserString(const Base::Quantity& val) const
{
    double factor;
    QString unitString;
    return getUserString(val, factor, unitString);
}

void QuantitySpinBox::setExpression(std::shared_ptr<Expression> expr)
{
    ExpressionSpinBox::setExpression(expr);
}

QAbstractSpinBox::StepEnabled QuantitySpinBox::stepEnabled() const
{
    Q_D(const QuantitySpinBox);
    if (isReadOnly() /* || !d->validInput*/) {
        return StepNone;
    }
    if (wrapping()) {
        return StepEnabled(StepUpEnabled | StepDownEnabled);
    }
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

    // A valid pending edit is the user's intended current value. Invalid and incomplete edits do
    // not set pendingEmit, so they naturally fall back to the last committed quantity.
    const Base::Quantity base = d->pendingEmit ? d->cached : d->quantity;

    // Re-select the display unit for the value being stepped. A pending edit can cross a
    // magnitude-dependent schema threshold, in which case the cached display unit is stale.
    double displayFactor;
    QString displaySymbol;
    getUserString(base, displayFactor, displaySymbol);
    const App::QuantityInputUnit steppingUnit {base.getUnit(), displaySymbol.toStdString()};
    Q_UNUSED(displayFactor);

    const double displayScale = steppingUnit.getScale().getValue();
    const double displayedValue = base.getValue() / displayScale;
    const double steppedValue = displayedValue + steps * d->singleStep;
    Quantity quant(steppedValue * displayScale, steppingUnit.getUnit());
    commitQuantity(quant, TextPolicy::ReformatEditor, true);
    update();
    selectNumber();
}

QSize QuantitySpinBox::sizeForText(const QString& txt) const
{
    const QFontMetrics fm(fontMetrics());
    int h = lineEdit()->sizeHint().height();
    int w = QtTools::horizontalAdvance(fm, txt);

    w += 2;  // cursor blinking space
    w += iconHeight;

    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    QSize hint(w, h);
    QSize size = style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this);
    return size;
}

QSize QuantitySpinBox::sizeHint() const
{
    Q_D(const QuantitySpinBox);
    auto le = lineEdit();
    if (le && d->adjustableWidth) {
        // limit number of typed characters to keep unit visible
        le->setMaxLength(getMaxStrLength(d->maxExpectedDigits));
    }
    return sizeHintForDigits(d->maxExpectedDigits);
}

QSize QuantitySpinBox::sizeHintForDigits(int digits) const
{
    Q_D(const QuantitySpinBox);
    ensurePolished();

    QString txt;
    auto le = lineEdit();

    if (d->adjustableWidth && le) {
        txt = le->text();
    }
    else {
        const int maxLen = getMaxStrLength(digits);
        txt = QStringLiteral("8").repeated(maxLen);
    }
    // font-proportional padding for cursor blinking
    txt += QLatin1Char('8');

    const QFontMetrics fm(fontMetrics());
    int w = qMax(0, QtTools::horizontalAdvance(fm, txt));
    if (d->addIconSpace) {
        w += iconHeight;
    }
    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    QSize hint(w, le ? le->sizeHint().height() : 0);

    QSize size = style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this);
    return size;
}

int QuantitySpinBox::getMaxStrLength(int digits) const
{
    // Calculates the length of the longest string allowed
    Q_D(const QuantitySpinBox);
    ensurePolished();
    QString unit = QString::fromStdString(d->unit.getString());
    int decimals = App::GetApplication()
                       .GetUserParameter()
                       .GetGroup("BaseApp/Preferences/Units")
                       ->GetInt("Decimals", 2);
    return digits + 1 /*separator*/ + decimals + 1 /*space*/ + unit.length();
}

void QuantitySpinBox::showEvent(QShowEvent* event)
{
    Q_D(QuantitySpinBox);

    QAbstractSpinBox::showEvent(event);

    bool selected = lineEdit()->hasSelectedText();
    updateText(d->quantity);
    if (selected) {
        selectNumber();
    }
}

void QuantitySpinBox::hideEvent(QHideEvent* event)
{
    handlePendingEmit();
    QAbstractSpinBox::hideEvent(event);
}

void QuantitySpinBox::closeEvent(QCloseEvent* event)
{
    handlePendingEmit();
    QAbstractSpinBox::closeEvent(event);
}

bool QuantitySpinBox::event(QEvent* event)
{
    return QAbstractSpinBox::event(event);
}

void QuantitySpinBox::focusInEvent(QFocusEvent* event)
{
    bool hasSel = lineEdit()->hasSelectedText();
    QAbstractSpinBox::focusInEvent(event);

    if (event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason
        || event->reason() == Qt::ShortcutFocusReason) {

        if (isBound() && getExpression() && lineEdit()->isReadOnly()) {
            auto helpEvent = new QHelpEvent(
                QEvent::ToolTip,
                QPoint(0, rect().height()),
                mapToGlobal(QPoint(0, rect().height()))
            );
            QApplication::postEvent(this, helpEvent);
            lineEdit()->setSelection(0, 0);
        }
        else {
            if (!hasSel) {
                selectNumber();
            }
        }
    }
}

void QuantitySpinBox::focusOutEvent(QFocusEvent* event)
{
    Q_D(const QuantitySpinBox);

    validateInput();

    if (d->validInput && d->normalize) {
        normalize();
    }

    QToolTip::hideText();
    QAbstractSpinBox::focusOutEvent(event);
}

void QuantitySpinBox::changeEvent(QEvent* event)
{
    Q_D(QuantitySpinBox);
    QAbstractSpinBox::changeEvent(event);

    if (event->type() == QEvent::LocaleChange && d->validInput) {
        const QSignalBlocker blocker(lineEdit());
        updateText(d->quantity);
    }
}

void QuantitySpinBox::clear()
{
    QAbstractSpinBox::clear();
}

void QuantitySpinBox::selectNumber()
{
    const auto length = Gui::numericInputSelectionLengthUtf16(
        lineEdit()->text(),
        Gui::numericLocaleContextFor(locale())
    );
    if (length > 0) {
        lineEdit()->setSelection(0, length);
    }
}

QString QuantitySpinBox::textFromValue(const Base::Quantity& value) const
{
    QString str = getUserString(value);
    if (qAbs(value.getValue()) >= 1000.0) {
        str.remove(locale().groupSeparator());
    }
    return str;
}

Base::Quantity QuantitySpinBox::valueFromText(const QString& text) const
{
    Q_D(const QuantitySpinBox);

    const App::ObjectIdentifier& path = getPath();
    const auto grammar = isBound() ? App::QuantityInputGrammar::Expression
                                   : App::QuantityInputGrammar::Quantity;
    const auto result = d->interpretInput(text, path, grammar, App::InputPhase::Commit);
    return result.quantity.value_or(Base::Quantity());
}

QValidator::State QuantitySpinBox::validate(QString& text, int& pos) const
{
    Q_D(const QuantitySpinBox);
    Q_UNUSED(pos)

    const App::ObjectIdentifier& path = getPath();
    const auto grammar = isBound() ? App::QuantityInputGrammar::Expression
                                   : App::QuantityInputGrammar::Quantity;
    const auto result = d->interpretInput(text, path, grammar, App::InputPhase::Editing);
    return result.status == App::InputStatus::Acceptable ? QValidator::Acceptable
                                                         : QValidator::Intermediate;
}

#include "moc_QuantitySpinBox.cpp"
#include "moc_QuantitySpinBox_p.cpp"
