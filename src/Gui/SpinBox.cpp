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

#include <limits>
#include <cmath>
#include <set>
#include <cctype>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QRegularExpression>
#include <QStyle>
#include <QStyleOptionSpinBox>
#include <QStylePainter>

#include <boost/math/special_functions/round.hpp>

#include <App/Application.h>
#include <App/Document.h>
#include <App/ExpressionParser.h>
#include <App/PropertyUnits.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>

#include "SpinBox.h"
#include "Command.h"
#include "Dialogs/DlgExpressionInput.h"
#include "QuantitySpinBox_p.h"
#include "Widgets.h"


using namespace Gui;
using namespace App;
using namespace Base;

namespace
{
bool extractParameterRow(const std::string& address, int& row)
{
    if (address.size() < 2 || (address[0] != 'A' && address[0] != 'B')) {
        return false;
    }

    for (size_t i = 1; i < address.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(address[i]))) {
            return false;
        }
    }

    row = std::stoi(address.substr(1));
    return row > 0;
}

std::string getParameterAliasCellAddressForRow(int row)
{
    return "A" + std::to_string(row);
}

std::string getParameterValueCellAddressForRow(int row)
{
    return "B" + std::to_string(row);
}

std::string getNextFreeParameterValueCellAddress(const std::vector<std::string>& usedCells)
{
    std::set<int> usedRows;
    for (const auto& cell : usedCells) {
        int row = 0;
        if (extractParameterRow(cell, row)) {
            usedRows.insert(row);
        }
    }

    int row = 1;
    while (usedRows.find(row) != usedRows.end()) {
        ++row;
    }

    return getParameterValueCellAddressForRow(row);
}

constexpr const char* kParametersSheetName = "Parameters";

void readParameterSheetState(
    App::Document* doc,
    const std::string& alias,
    std::string& existingAddress,
    std::vector<std::string>& usedCells
)
{
    if (!doc) {
        return;
    }

    Base::PyGILStateLocker lock;
    PyObject* freecadMod = PyImport_ImportModule("FreeCAD");
    if (!freecadMod) {
        PyErr_Clear();
        return;
    }

    PyObject* pyDoc = PyObject_CallMethod(freecadMod, "getDocument", "s", doc->getName());
    if (!pyDoc) {
        PyErr_Clear();
        Py_DECREF(freecadMod);
        return;
    }

    PyObject* pySheet = PyObject_CallMethod(pyDoc, "getObject", "s", kParametersSheetName);
    if (!pySheet || pySheet == Py_None) {
        PyErr_Clear();
        Py_XDECREF(pySheet);
        Py_DECREF(pyDoc);
        Py_DECREF(freecadMod);
        return;
    }

    if (!alias.empty()) {
        PyObject* aliasResult = PyObject_CallMethod(pySheet, "getCellFromAlias", "s", alias.c_str());
        if (aliasResult && PyUnicode_Check(aliasResult)) {
            const char* address = PyUnicode_AsUTF8(aliasResult);
            if (address) {
                existingAddress = address;
            }
        }
        Py_XDECREF(aliasResult);
        PyErr_Clear();
    }

    PyObject* usedCellsResult = PyObject_CallMethod(pySheet, "getUsedCells", nullptr);
    if (usedCellsResult) {
        PyObject* iter = PyObject_GetIter(usedCellsResult);
        if (iter) {
            while (PyObject* item = PyIter_Next(iter)) {
                if (PyUnicode_Check(item)) {
                    const char* cellAddress = PyUnicode_AsUTF8(item);
                    if (cellAddress) {
                        usedCells.emplace_back(cellAddress);
                    }
                }
                Py_DECREF(item);
            }
            Py_DECREF(iter);
        }
    }

    Py_XDECREF(usedCellsResult);
    PyErr_Clear();

    Py_DECREF(pySheet);
    Py_DECREF(pyDoc);
    Py_DECREF(freecadMod);
}

QString trimTrailingStatementDelimiter(QString text)
{
    text = text.trimmed();

    while (text.endsWith(QLatin1Char(';'))) {
        text.chop(1);
        text = text.trimmed();
    }

    return text;
}

bool looksLikeExpressionInput(const QString& input)
{
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    bool isUnsigned = false;
    trimmed.toUInt(&isUnsigned);
    if (isUnsigned) {
        return false;
    }

    if (trimmed.contains(QStringLiteral("<<"))) {
        return true;
    }

    static const QRegularExpression expressionChars(
        QStringLiteral(R"([A-Za-z_=+\-*/^().,;])")
    );
    return expressionChars.match(trimmed).hasMatch();
}
}  // namespace

ExpressionSpinBox::ExpressionSpinBox(QAbstractSpinBox* sb)
    : spinbox(sb)
{
    lineedit = spinbox->findChild<QLineEdit*>();
    // Set Margins
    // https://forum.freecad.org/viewtopic.php?f=8&t=50615
    // vertical margin, otherwise `,` is clipped to a `.` on some OSX versions
    int margin = getMargin();
    lineedit->setTextMargins(margin, margin, margin, margin);
    lineedit->setAlignment(Qt::AlignVCenter);

    makeLabel(lineedit);
    QObject::connect(iconLabel, &ExpressionLabel::clicked, [this]() { this->openFormulaDialog(); });
}

ExpressionSpinBox::~ExpressionSpinBox() = default;

int ExpressionSpinBox::getMargin()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    return lineedit->style()->pixelMetric(QStyle::PM_LineEditIconMargin, nullptr, lineedit) / 2;
#else
    return lineedit->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, lineedit);
#endif
}

void ExpressionSpinBox::bind(const App::ObjectIdentifier& _path)
{
    ExpressionBinding::bind(_path);

    showIcon();
}

void ExpressionSpinBox::showIcon()
{
    iconLabel->show();
}

void ExpressionSpinBox::validateInput()
{}

void ExpressionSpinBox::showInvalidExpression(const QString& tip)
{
    spinbox->setReadOnly(true);
    QPalette p(lineedit->palette());
    p.setColor(QPalette::Active, QPalette::Text, Qt::red);
    lineedit->setPalette(p);
    iconLabel->setToolTip(tip);
    iconLabel->setPixmap(getIcon(":/icons/button_invalid.svg", QSize(iconHeight, iconHeight)));
}

void ExpressionSpinBox::showValidExpression(ExpressionSpinBox::Number number)
{
    try {
        showExpression(number);
    }
    catch (const Base::Exception& e) {
        showInvalidExpression(QString::fromUtf8(e.what()));
    }
}

void ExpressionSpinBox::showExpression(Number number)
{
    std::unique_ptr<Expression> result(getExpression()->eval());
    auto* value = freecad_cast<NumberExpression*>(result.get());

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
    iconLabel->setExpressionText(QString::fromStdString(getExpression()->toString()));
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
    catch (const Base::Exception& e) {
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
    catch (const Base::Exception& e) {
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
    int iconWidth = iconLabel->width() + getMargin();
    iconLabel->move(lineedit->width() - iconWidth, (lineedit->height() - iconLabel->height()) / 2);
    updateExpression();
}

void ExpressionSpinBox::openFormulaDialog()
{
    Q_ASSERT(isBound());

    auto* qprop = freecad_cast<PropertyQuantity*>(getPath().getProperty());
    Unit unit;

    if (qprop) {
        unit = qprop->getUnit();
    }

    auto box = new Gui::Dialog::DlgExpressionInput(getPath(), getExpression(), unit, spinbox);
    QObject::connect(box, &Gui::Dialog::DlgExpressionInput::finished, [this, box]() {
        if (box->result() == QDialog::Accepted) {
            setExpression(box->getExpression());
        }
        else if (box->discardedFormula()) {
            setExpression(std::shared_ptr<Expression>());
        }

        updateExpression();
        box->deleteLater();
    });
    box->show();

    QPoint pos = spinbox->mapToGlobal(QPoint(0, 0));
    box->move(pos - box->expressionPosition());
    Gui::adjustDialogPosition(box);
}

bool ExpressionSpinBox::handleKeyEvent(const QString& text)
{
    if (text == QLatin1String("=") && isBound()) {
        // Only open the formula dialog if the input is empty.
        // When there's already text (e.g. user typing "width=42"),
        // let '=' be inserted as a character.
        if (!lineedit || lineedit->text().trimmed().isEmpty()) {
            openFormulaDialog();
            return true;
        }
        return false;  // let '=' be typed into the input
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

UnsignedValidator::UnsignedValidator(QObject* parent)
    : QValidator(parent)
{
    b = 0;
    t = std::numeric_limits<unsigned>::max();
}

UnsignedValidator::UnsignedValidator(uint minimum, uint maximum, QObject* parent)
    : QValidator(parent)
{
    b = minimum;
    t = maximum;
}

UnsignedValidator::~UnsignedValidator() = default;

QValidator::State UnsignedValidator::validate(QString& input, int&) const
{
    QString stripped = input.trimmed();
    if (stripped.isEmpty()) {
        return Intermediate;
    }
    bool ok;
    uint entered = input.toUInt(&ok);
    if (!ok) {
        return Invalid;
    }
    else if (entered < b) {
        return Intermediate;
    }
    else if (entered > t) {
        return Invalid;
    }
    //  else if ( entered < b || entered > t )
    //	  return Invalid;
    else {
        return Acceptable;
    }
}

void UnsignedValidator::setRange(uint minimum, uint maximum)
{
    b = minimum;
    t = maximum;
}

void UnsignedValidator::setBottom(uint bottom)
{
    setRange(bottom, top());
}

void UnsignedValidator::setTop(uint top)
{
    setRange(bottom(), top);
}

namespace Gui
{
class UIntSpinBoxPrivate
{
public:
    UnsignedValidator* mValidator {nullptr};

    UIntSpinBoxPrivate() = default;
    unsigned mapToUInt(int v) const
    {
        using int_limits = std::numeric_limits<int>;
        using uint_limits = std::numeric_limits<unsigned>;

        unsigned ui;
        if (v == int_limits::min()) {
            ui = 0;
        }
        else if (v == int_limits::max()) {
            ui = uint_limits::max();
        }
        else if (v < 0) {
            v -= int_limits::min();
            ui = static_cast<unsigned>(v);
        }
        else {
            ui = static_cast<unsigned>(v);
            ui -= int_limits::min();
        }
        return ui;
    }
    int mapToInt(unsigned v) const
    {
        using int_limits = std::numeric_limits<int>;
        using uint_limits = std::numeric_limits<unsigned>;

        int in;
        if (v == uint_limits::max()) {
            in = int_limits::max();
        }
        else if (v == 0) {
            in = int_limits::min();
        }
        else if (v > static_cast<unsigned int>(int_limits::max())) {
            v += int_limits::min();
            in = static_cast<int>(v);
        }
        else {
            in = v;
            in += int_limits::min();
        }
        return in;
    }
};

}  // namespace Gui

UIntSpinBox::UIntSpinBox(QWidget* parent)
    : QSpinBox(parent)
    , ExpressionSpinBox(this)
{
    d = new UIntSpinBoxPrivate;
    d->mValidator = new UnsignedValidator(this->minimum(), this->maximum(), this);
    connect(this, qOverload<int>(&QSpinBox::valueChanged), this, &UIntSpinBox::valueChange);
    setRange(0, 99);
    setValue(0);
    updateValidator();
}

UIntSpinBox::~UIntSpinBox()
{
    delete d->mValidator;
    delete d;
    d = nullptr;
}

void UIntSpinBox::setRange(uint minVal, uint maxVal)
{
    int iminVal = d->mapToInt(minVal);
    int imaxVal = d->mapToInt(maxVal);
    QSpinBox::setRange(iminVal, imaxVal);
    updateValidator();
}

QValidator::State UIntSpinBox::validate(QString& input, int& pos) const
{
    QValidator::State state = d->mValidator->validate(input, pos);
    if (state == QValidator::Invalid && looksLikeExpressionInput(input)) {
        return QValidator::Intermediate;
    }

    return state;
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
    if (maxVal < minVal) {
        maxVal = minVal;
    }
    setRange(minVal, maxVal);
}

uint UIntSpinBox::maximum() const
{
    return d->mapToUInt(QSpinBox::maximum());
}

void UIntSpinBox::setMaximum(uint maxVal)
{
    uint minVal = minimum();
    if (minVal > maxVal) {
        minVal = maxVal;
    }
    setRange(minVal, maxVal);
}

QString UIntSpinBox::textFromValue(int v) const
{
    uint val = d->mapToUInt(v);
    QString s;
    s.setNum(val);
    return s;
}

int UIntSpinBox::valueFromText(const QString& text) const
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

bool UIntSpinBox::tryHandleRawExpression(const QString& text)
{
    QString expressionText = text.trimmed();
    if (expressionText.isEmpty()) {
        return false;
    }

    bool isUnsigned = false;
    expressionText.toUInt(&isUnsigned);
    if (isUnsigned) {
        return false;
    }

    if (expressionText.startsWith(QLatin1Char('='))) {
        expressionText = expressionText.mid(1).trimmed();
    }
    expressionText = trimTrailingStatementDelimiter(expressionText);
    if (expressionText.isEmpty()) {
        return false;
    }

    App::DocumentObject* contextObj = nullptr;
    if (isBound()) {
        contextObj = getPath().getDocumentObject();
    }

    if (!contextObj) {
        auto* doc = App::GetApplication().getActiveDocument();
        if (doc && !doc->getObjects().empty()) {
            contextObj = doc->getObjects().front();
        }
    }

    std::shared_ptr<App::Expression> expr;
    try {
        expr.reset(App::ExpressionParser::parse(contextObj, expressionText.toUtf8().constData()));
    }
    catch (...) {
        return false;
    }

    if (!expr) {
        return false;
    }

    try {
        std::unique_ptr<App::Expression> result(expr->eval());
        auto* number = freecad_cast<App::NumberExpression*>(result.get());
        if (!number) {
            return false;
        }

        double roundedValue = boost::math::round(number->getValue());
        if (!std::isfinite(roundedValue) || roundedValue < 0.0
            || roundedValue > static_cast<double>(std::numeric_limits<unsigned>::max())) {
            return false;
        }

        auto valueAsUInt = static_cast<unsigned>(roundedValue);
        if (valueAsUInt < minimum() || valueAsUInt > maximum()) {
            return false;
        }

        if (isBound()) {
            setExpression(std::move(expr));
            updateExpression();
        }
        else {
            setValue(valueAsUInt);
        }

        return true;
    }
    catch (...) {
        return false;
    }
}

bool UIntSpinBox::tryHandleVariableAssignment(const QString& text)
{
    static const QRegularExpression varAssignRegex(
        QStringLiteral(R"(^([a-zA-Z_][a-zA-Z0-9_]*)\s*=(?!=)\s*(.+)$)")
    );

    QRegularExpressionMatch match = varAssignRegex.match(text.trimmed());
    if (!match.hasMatch()) {
        return false;
    }

    QString varName = match.captured(1);
    QString valueExpr = trimTrailingStatementDelimiter(match.captured(2));
    if (varName.isEmpty() || valueExpr.isEmpty()) {
        return false;
    }

    std::string nameStd = varName.toStdString();
    if (App::ExpressionParser::isTokenAUnit(nameStd)
        || App::ExpressionParser::isTokenAConstant(nameStd)) {
        return false;
    }

    if (nameStd != Base::Tools::getIdentifier(nameStd)) {
        return false;
    }

    App::Document* doc = nullptr;
    if (isBound()) {
        if (auto* docObj = getPath().getDocumentObject()) {
            doc = docObj->getDocument();
        }
    }

    if (!doc) {
        doc = App::GetApplication().getActiveDocument();
    }
    if (!doc) {
        return false;
    }

    std::string cellContent = valueExpr.toStdString();
    bool plainUInt = false;
    valueExpr.toUInt(&plainUInt);
    if (!plainUInt && !cellContent.empty() && cellContent[0] != '=') {
        cellContent = "=" + cellContent;
    }

    try {
        const std::string escapedDocName = Base::Tools::escapeQuotesFromString(doc->getName());

        App::DocumentObject* sheetObj = doc->getObject(kParametersSheetName);
        if (!sheetObj) {
            Base::Interpreter().runStringArg(
                "App.getDocument('%s').addObject('Spreadsheet::Sheet', '%s')",
                escapedDocName.c_str(),
                kParametersSheetName
            );
            Base::Interpreter().runStringArg(
                "App.getDocument('%s').getObject('%s').Label = '%s'",
                escapedDocName.c_str(),
                kParametersSheetName,
                kParametersSheetName
            );
            sheetObj = doc->getObject(kParametersSheetName);
        }
        if (!sheetObj) {
            return false;
        }

        std::string existingAddr;
        std::vector<std::string> usedCells;
        readParameterSheetState(doc, nameStd, existingAddr, usedCells);

        const bool aliasAlreadyExists = !existingAddr.empty();
        int row = 0;
        if (aliasAlreadyExists) {
            if (!extractParameterRow(existingAddr, row)) {
                return false;
            }
        }
        else {
            const std::string nextValueCellAddr = getNextFreeParameterValueCellAddress(usedCells);
            if (!extractParameterRow(nextValueCellAddr, row)) {
                return false;
            }
        }

        const std::string valueCellAddr = getParameterValueCellAddressForRow(row);
        const std::string aliasCellAddr = getParameterAliasCellAddressForRow(row);

        const std::string escapedContent = Base::Tools::escapeQuotesFromString(cellContent);
        Base::Interpreter().runStringArg(
            "App.getDocument('%s').getObject('%s').set('%s', '%s')",
            escapedDocName.c_str(),
            kParametersSheetName,
            valueCellAddr.c_str(),
            escapedContent.c_str()
        );

        if (aliasAlreadyExists && existingAddr != valueCellAddr) {
            Base::Interpreter().runStringArg(
                "App.getDocument('%s').getObject('%s').setAlias('%s', '')",
                escapedDocName.c_str(),
                kParametersSheetName,
                existingAddr.c_str()
            );
        }

        if (!aliasAlreadyExists || existingAddr != valueCellAddr) {
            Base::Interpreter().runStringArg(
                "App.getDocument('%s').getObject('%s').setAlias('%s', '%s')",
                escapedDocName.c_str(),
                kParametersSheetName,
                valueCellAddr.c_str(),
                nameStd.c_str()
            );
        }

        Base::Interpreter().runStringArg(
            "App.getDocument('%s').getObject('%s').set('%s', '%s')",
            escapedDocName.c_str(),
            kParametersSheetName,
            aliasCellAddr.c_str(),
            nameStd.c_str()
        );

        Base::Interpreter().runStringArg(
            "App.getDocument('%s').getObject('%s').recompute()",
            escapedDocName.c_str(),
            kParametersSheetName
        );

        App::DocumentObject* contextObj = nullptr;
        if (isBound()) {
            contextObj = getPath().getDocumentObject();
        }
        if (!contextObj) {
            contextObj = doc->getObject(kParametersSheetName);
        }
        if (!contextObj && !doc->getObjects().empty()) {
            contextObj = doc->getObjects().front();
        }
        if (!contextObj) {
            return false;
        }

        const std::string exprStr = std::string(kParametersSheetName) + "." + nameStd;
        std::shared_ptr<App::Expression> expr(App::ExpressionParser::parse(contextObj, exprStr.c_str()));
        if (!expr) {
            return false;
        }

        std::unique_ptr<App::Expression> result(expr->eval());
        auto* number = freecad_cast<App::NumberExpression*>(result.get());
        if (!number) {
            return false;
        }

        double roundedValue = boost::math::round(number->getValue());
        if (!std::isfinite(roundedValue) || roundedValue < 0.0
            || roundedValue > static_cast<double>(std::numeric_limits<unsigned>::max())) {
            return false;
        }

        auto valueAsUInt = static_cast<unsigned>(roundedValue);
        if (valueAsUInt < minimum() || valueAsUInt > maximum()) {
            return false;
        }

        if (isBound()) {
            setExpression(std::move(expr));
            updateExpression();
        }
        else {
            setValue(valueAsUInt);
        }

        return true;
    }
    catch (...) {
        return false;
    }
}

bool UIntSpinBox::commitInlineExpressionTextForUi()
{
    if (!lineedit) {
        return false;
    }

    const QString input = lineedit->text();
    return tryHandleVariableAssignment(input) || tryHandleRawExpression(input);
}

bool UIntSpinBox::apply(const std::string& propName)
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

void UIntSpinBox::resizeEvent(QResizeEvent* event)
{
    QAbstractSpinBox::resizeEvent(event);
    resizeWidget();
}

void UIntSpinBox::keyPressEvent(QKeyEvent* event)
{
    const auto isEnter = event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return;

    if (event->text() == QLatin1String("=") && lineedit && lineedit->text().trimmed().isEmpty()) {
        QAbstractSpinBox::keyPressEvent(event);
        return;
    }

    if (isEnter && commitInlineExpressionTextForUi()) {
        return;
    }

    if (!handleKeyEvent(event->text())) {
        QAbstractSpinBox::keyPressEvent(event);
    }
}

void UIntSpinBox::focusOutEvent(QFocusEvent* event)
{
    commitInlineExpressionTextForUi();
    QAbstractSpinBox::focusOutEvent(event);
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
{}

IntSpinBox::~IntSpinBox() = default;

bool IntSpinBox::apply(const std::string& propName)
{
    if (!ExpressionBinding::apply(propName)) {
        Gui::Command::doCommand(Gui::Command::Doc, "%s = %d", propName.c_str(), value());
        return true;
    }
    else {
        return false;
    }
}

void IntSpinBox::setNumberExpression(App::NumberExpression* expr)
{
    setValue(boost::math::round(expr->getValue()));
}

void IntSpinBox::resizeEvent(QResizeEvent* event)
{
    QAbstractSpinBox::resizeEvent(event);
    resizeWidget();
}

void IntSpinBox::keyPressEvent(QKeyEvent* event)
{
    if (!handleKeyEvent(event->text())) {
        QAbstractSpinBox::keyPressEvent(event);
    }
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
{}

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

void DoubleSpinBox::resizeEvent(QResizeEvent* event)
{
    QAbstractSpinBox::resizeEvent(event);
    resizeWidget();
}

void DoubleSpinBox::keyPressEvent(QKeyEvent* event)
{
    if (!handleKeyEvent(event->text())) {
        QDoubleSpinBox::keyPressEvent(event);
    }
}

void DoubleSpinBox::paintEvent(QPaintEvent*)
{
    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    drawControl(opt);
}

#include "moc_SpinBox.cpp"
