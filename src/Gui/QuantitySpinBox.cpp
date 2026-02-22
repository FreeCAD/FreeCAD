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
#include <set>
#include <cctype>
#include <unordered_set>
#include <unordered_map>
#include <QApplication>
#include <QDebug>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QStyle>
#include <QStyleOptionSpinBox>
#include <QToolTip>

#include <sstream>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Expression.h>
#include <App/ExpressionParser.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/UnitsApi.h>
#include <Base/Tools.h>
#include <Base/UnitsSchema.h>

#include "QuantitySpinBox.h"
#include "QuantitySpinBox_p.h"
#include "Dialogs/DlgExpressionInput.h"
#include "Tools.h"
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

    // "A123"/"B123" -> 123
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

    // find first empty row
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
    std::vector<std::string>& usedCells,
    bool includeUsedCells = true
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

    if (includeUsedCells) {
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
    }

    Py_DECREF(pySheet);
    Py_DECREF(pyDoc);
    Py_DECREF(freecadMod);
}

bool hasParameterAlias(App::Document* doc, const std::string& alias)
{
    if (!doc || alias.empty()) {
        return false;
    }

    std::string existingAddress;
    std::vector<std::string> unusedCells;

    readParameterSheetState(doc, alias, existingAddress, unusedCells, false);
    return !existingAddress.empty();
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

bool isIdentifierStart(char c)
{
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool isIdentifierChar(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '@';
}

size_t skipSpaces(const std::string& input, size_t index)
{
    while (index < input.size() && std::isspace(static_cast<unsigned char>(input[index]))) {
        ++index;
    }
    return index;
}

bool startsQualifiedPath(const std::string& input, size_t tokenEnd)
{
    size_t index = skipSpaces(input, tokenEnd);
    return index < input.size() && input[index] == '.';
}

bool isFunctionCall(const std::string& input, size_t tokenEnd)
{
    size_t index = skipSpaces(input, tokenEnd);
    return index < input.size() && input[index] == '(';
}

bool partOfQualifiedPath(const std::string& input, size_t tokenStart)
{
    if (tokenStart == 0) {
        return false;
    }

    size_t index = tokenStart;
    while (index > 0) {
        --index;
        if (!std::isspace(static_cast<unsigned char>(input[index]))) {
            return input[index] == '.';
        }
    }
    return false;
}

size_t endOfObjectReferenceString(const std::string& input, size_t start)
{
    size_t index = start + 2;
    while (index < input.size()) {
        const char c = input[index];
        if (c == '\\') {
            if (index + 1 < input.size()) {
                index += 2;
            }
            else {
                ++index;
            }
            continue;
        }

        if (c == '>' && index + 1 < input.size() && input[index + 1] == '>') {
            return index + 2;
        }
        ++index;
    }
    return input.size();
}

bool isPlainQuantityLiteral(const QString& text)
{
    try {
        return Base::Quantity::parse(text.toStdString()).isValid();
    }
    catch (...) {
        return false;
    }
}

bool isCommonUnitToken(const std::string& token)
{
    // Keep this list intentionally short and practical for inline dimension editing.
    static const std::unordered_set<std::string> commonUnits = {
        "mm",
        "cm",
        "m",
        "km",
        "um",
        "nm",
        "in",
        "ft",
        "yd",
        "deg",
        "rad",
        "s",
        "ms",
        "min",
        "h",
    };

    return commonUnits.find(token) != commonUnits.end();
}

std::string qualifyVariableAliasesInExpression(App::Document* doc, const std::string& input)
{
    if (!doc || input.empty()) {
        return input;
    }

    std::string output;
    output.reserve(input.size() + 16);

    std::unordered_map<std::string, bool> aliasCache;

    for (size_t i = 0; i < input.size();) {
        const char c = input[i];
        if (c == '<' && i + 1 < input.size() && input[i + 1] == '<') {
            const size_t end = endOfObjectReferenceString(input, i);
            output.append(input, i, end - i);
            i = end;
            continue;
        }

        if (!isIdentifierStart(c)) {
            output.push_back(c);
            ++i;
            continue;
        }

        const bool inQualifiedPath = partOfQualifiedPath(input, i);

        size_t j = i + 1;
        while (j < input.size() && isIdentifierChar(input[j])) {
            ++j;
        }

        const std::string token = input.substr(i, j - i);

        const bool tokenStartsQualifiedPath = startsQualifiedPath(input, j);
        const bool tokenIsFunctionCall = isFunctionCall(input, j);

        bool skip = inQualifiedPath || tokenStartsQualifiedPath || tokenIsFunctionCall
            || token == kParametersSheetName || isCommonUnitToken(token)
            || App::ExpressionParser::isTokenAUnit(token)
            || App::ExpressionParser::isTokenAConstant(token);

        if (!skip) {
            auto it = aliasCache.find(token);
            bool hasAlias = false;
            if (it != aliasCache.end()) {
                hasAlias = it->second;
            }
            else {
                hasAlias = hasParameterAlias(doc, token);
                aliasCache.emplace(token, hasAlias);
            }

            if (hasAlias) {
                output += kParametersSheetName;
                output.push_back('.');
            }
        }

        output += token;
        i = j;
    }

    return output;
}

std::string buildQualifiedExpression(App::Document* doc, const QString& expressionText)
{
    std::string parseText = expressionText.toStdString();

    if (!doc) {
        return parseText;
    }

    return qualifyVariableAliasesInExpression(doc, parseText);
}
}  // namespace

namespace Gui
{

class QuantitySpinBoxPrivate
{
public:
    QuantitySpinBoxPrivate(QuantitySpinBox* q)
        : validInput(true)
        , pendingEmit(false)
        , normalize(true)
        , checkRangeInExpression(false)
        , unitValue(0)
        , maximum(std::numeric_limits<double>::max())
        , minimum(-std::numeric_limits<double>::max())
        , singleStep(1.0)
        , q_ptr(q)
    {}
    ~QuantitySpinBoxPrivate() = default;

    QString stripped(const QString& t, int* pos) const
    {
        QString text = t;
        const int s = text.size();
        text = text.trimmed();
        if (pos) {
            (*pos) -= (s - text.size());
        }
        return text;
    }

    bool validate(QString& input, Base::Quantity& result, const App::ObjectIdentifier& path) const
    {
        Q_Q(const QuantitySpinBox);

        // Do not accept empty strings because the parser will consider
        // " unit" as "1 unit" which is not the desired behaviour (see #0004104)
        if (input.isEmpty()) {
            return false;
        }

        bool success = false;
        QString tmp = input;

        auto validateInput = [&](QString& tmp) -> QValidator::State {
            QValidator::State state;
            Base::Quantity res = validateAndInterpret(tmp, state, path);
            res.setFormat(quantity.getFormat());
            if (state == QValidator::Acceptable) {
                success = true;
                result = res;
                input = tmp;
            }
            return state;
        };

        QValidator::State state = validateInput(tmp);
        if (state == QValidator::Intermediate && q->hasExpression()) {
            // Accept the expression as it is but try to add the right unit string
            success = true;

            Base::Quantity quantity;
            double value;
            if (parseString(input, quantity, value, path)) {
                quantity.setUnit(unit);
                result = quantity;

                // Now translate the quantity into its string representation using the user-defined
                // unit system
                input = QString::fromStdString(Base::UnitsApi::schemaTranslate(result));
            }
        }

        return success;
    }
    bool parseString(
        const QString& str,
        Base::Quantity& result,
        double& value,
        const App::ObjectIdentifier& path
    ) const
    {
        App::ObjectIdentifier pathtmp = path;
        try {
            QString copy = str;
            copy.remove(locale.groupSeparator());

            // Expression parser
            std::shared_ptr<Expression> expr(
                ExpressionParser::parse(path.getDocumentObject(), copy.toUtf8().constData())
            );
            if (expr) {

                std::unique_ptr<Expression> res(expr->eval());
                NumberExpression* n = freecad_cast<NumberExpression*>(res.get());
                if (n) {
                    result = n->getQuantity();
                    value = result.getValue();
                    return true;
                }
            }
        }
        catch (Base::Exception&) {
            return false;
        }
        return false;
    }
    Base::Quantity validateAndInterpret(
        QString& input,
        QValidator::State& state,
        const App::ObjectIdentifier& path
    ) const
    {
        Base::Quantity res;
        const double max = this->maximum;
        const double min = this->minimum;

        QString copy = input;
        double value = min;
        bool ok = false;

        QChar plus = QLatin1Char('+'), minus = QLatin1Char('-');

        if (locale.negativeSign() != minus) {
            copy.replace(locale.negativeSign(), minus);
        }
        if (locale.positiveSign() != plus) {
            copy.replace(locale.positiveSign(), plus);
        }

        if (copy.startsWith(QLatin1Char('='))) {
            copy = copy.mid(1).trimmed();
            if (copy.isEmpty()) {
                state = QValidator::Intermediate;
                return res;
            }
        }

        // If the input looks like a variable assignment (e.g. "width=42") or
        // a raw expression with identifiers (e.g. "sin(45)", "width*2"),
        // accept it as Intermediate so the user can type freely.
        // Actual parsing happens on Enter via tryHandleVariableAssignment /
        // tryHandleRawExpression.
        static const QRegularExpression varAssignPat(
            QStringLiteral(R"(^\s*[a-zA-Z_][a-zA-Z0-9_]*\s*=(?!=))")
        );
        if (varAssignPat.match(copy).hasMatch()) {
            state = QValidator::Intermediate;
            return res;
        }

        // For raw expressions: if the text contains identifiers that aren't
        // just trailing unit suffixes, try parsing as an expression first
        // before the unit-mangling logic corrupts it.
        static const QRegularExpression exprIdentPat(
            QStringLiteral(R"(^\s*(?:[a-zA-Z_]|<<)|[+\-*/^(]\s*(?:[a-zA-Z_]|<<))")
        );
        if (exprIdentPat.match(copy).hasMatch()) {
            // Try parsing as a raw expression
            if (parseString(copy, res, value, path)) {
                if (res.isDimensionless()) {
                    res.setUnit(unit);
                }
                if (value >= min && value <= max) {
                    state = QValidator::Acceptable;
                }
                else {
                    state = QValidator::Intermediate;
                }
                return res;
            }
            // Expression didn't parse (yet) — allow typing to continue
            state = QValidator::Intermediate;
            return res;
        }

        QString reverseUnitStr = unitStr;
        std::reverse(reverseUnitStr.begin(), reverseUnitStr.end());

        // Prep for expression parser
        // This regex matches chunks between +,-,$,^ accounting for matching parenthesis.
        QRegularExpression chunkRe(
            QStringLiteral("(?<=^|[\\+\\-])((\\((?>[^()]|(?2))*\\))|[^\\+\\-\n])*(?=$|[\\+\\-])")
        );
        QRegularExpressionMatchIterator expressionChunk = chunkRe.globalMatch(copy);
        unsigned int lengthOffset = 0;
        while (expressionChunk.hasNext()) {
            QRegularExpressionMatch matchChunk = expressionChunk.next();
            QString origionalChunk = matchChunk.captured(0);
            QString copyChunk = origionalChunk;
            std::reverse(copyChunk.begin(), copyChunk.end());

            // Reused regex patterns
            static const std::string regexUnits
                = "sAV|VC|lim|nim|im|hpm|[mf]?bl|°|ged|dar|nog|″|′|rroT[uµm]?|K[uµm]?|A[mkM]?|F["
                  "pnuµm]?|C|S[uµmkM]?|zH[kMGT]?|H[nuµm]?|mhO[kM]?|J[mk]?|Ve[kM]?|V[mk]?|hWk|sW|"
                  "lack?|N[mkM]?|g[uµmk]?|lm?|(?<=\\b|[^a-zA-Z])m[nuµmcdk]?|uoht|ni|\"|'|dy|dc|bW|"
                  "T|t|zo|ts|twc|Wk?|aP[kMG]?|is[pk]|h|G|M|tfc|tfqs|tf|s";
            static const std::string regexUnitlessFunctions
                = "soca|nisa|2nata|nata|hsoc|hnis|hnat|soc|nat|nis|pxe|gol|01gol";
            static const std::string regexConstants = "e|ip|lomm|lom";
            static const std::string regexNumber = "\\d+\\s*\\.?\\s*\\d*|\\.\\s*\\d+";

            // If expression does not contain /*() or ^, this regex will not find anything
            if (copy.contains(QLatin1Char('/')) || copy.contains(QLatin1Char('*'))
                || copy.contains(QLatin1Char('(')) || copy.contains(QLatin1Char(')'))
                || copy.contains(QLatin1Char('^'))) {
                // Find units and replace 1/2mm -> 1/2*(1mm), 1^2mm -> 1^2*(1mm)
                QRegularExpression fixUnits(
                    QString::fromStdString(
                        "(" + regexUnits + ")(\\s*\\)|(?:\\*|(?:\\)(?:(?:\\s*(?:" + regexConstants
                        + "|\\)(?:[^()]|(?R))*\\((?:" + regexUnitlessFunctions + ")|" + regexNumber
                        + "))|(?R))*\\(|(?:\\s*(?:" + regexConstants
                        + "|\\)(?:[^()]|(?R))*\\((?:" + regexUnitlessFunctions + ")|" + regexNumber
                        + "))))+(?:[\\/\\^]|(.*$))(?!(" + regexUnits + ")))"
                    )
                );
                QRegularExpressionMatch fixUnitsMatch = fixUnits.match(copyChunk);

                // 3rd capture group being filled indicates regex bailed out; no match.
                if (fixUnitsMatch.lastCapturedIndex() == 2
                    || (fixUnitsMatch.lastCapturedIndex() == 3
                        && fixUnitsMatch.captured(3).isEmpty())) {
                    QString matchUnits = fixUnitsMatch.captured(1);
                    QString matchNumbers = fixUnitsMatch.captured(2);
                    copyChunk.replace(
                        matchUnits + matchNumbers,
                        QStringLiteral(")") + matchUnits + QStringLiteral("1(*") + matchNumbers
                    );
                }
            }

            // Add default units to string if none are present
            if (!copyChunk.contains(reverseUnitStr)) {  // Fast check
                QRegularExpression unitsRe(
                    QString::fromStdString(
                        "(?<=\\b|[^a-zA-Z])(" + regexUnits
                        + ")(?=\\b|[^a-zA-Z])|°|″|′|\"|'|\\p{L}\\.\\p{L}|\\[\\p{L}"
                    )
                );

                QRegularExpressionMatch match = unitsRe.match(copyChunk);
                if (!match.hasMatch() && !copyChunk.isEmpty()) {  // If no units are found, use
                                                                  // default units
                    copyChunk.prepend(
                        QStringLiteral(")") + reverseUnitStr + QStringLiteral("1(*")
                    );  // Add units to the end of chunk *(1unit)
                }
            }

            std::reverse(copyChunk.begin(), copyChunk.end());

            copy.replace(
                matchChunk.capturedStart() + lengthOffset,
                matchChunk.capturedEnd() - matchChunk.capturedStart(),
                copyChunk
            );
            lengthOffset += copyChunk.length() - origionalChunk.length();
        }

        ok = parseString(copy, res, value, path);

        // If result does not have unit: add default unit
        if (res.isDimensionless()) {
            res.setUnit(unit);
        }

        if (!ok) {
            // input may not be finished
            state = QValidator::Intermediate;
        }
        else if (value >= min && value <= max) {
            state = QValidator::Acceptable;
        }
        else if (max == min) {  // when max and min is the same the only non-Invalid input is max
                                // (or min)
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
        if (state != QValidator::Acceptable) {
            res.setValue(max > 0 ? min : max);
        }

        return res;
    }

    QLocale locale;
    bool validInput;
    bool pendingEmit;
    bool normalize;
    bool checkRangeInExpression;
    QString validStr;
    Base::Quantity quantity;
    Base::Quantity cached;
    Base::Unit unit;
    double unitValue;
    QString unitStr;
    double maximum;
    double minimum;
    double singleStep;
    std::string unboundExpressionText;
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
    connect(this, &QuantitySpinBox::editingFinished, this, [&] { this->handlePendingEmit(true); });
}

QuantitySpinBox::~QuantitySpinBox() = default;

void QuantitySpinBox::bind(const App::ObjectIdentifier& _path)
{
    ExpressionSpinBox::bind(_path);
}

void QuantitySpinBox::showIcon()
{
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

std::string QuantitySpinBox::takeUnboundExpressionText()
{
    Q_D(QuantitySpinBox);
    std::string expression = std::move(d->unboundExpressionText);
    d->unboundExpressionText.clear();
    return expression;
}

bool QuantitySpinBox::commitInlineExpressionText()
{
    QString text = lineEdit()->text().trimmed();
    if (text.isEmpty()) {
        return false;
    }

    return tryHandleVariableAssignment(text) || tryHandleRawExpression(text);
}

void QuantitySpinBox::emitCommittedUnboundValue()
{
    Q_D(const QuantitySpinBox);

    Q_EMIT valueChanged(d->quantity);
    Q_EMIT valueChanged(d->quantity.getValue());
    Q_EMIT textChanged(getUserString(d->quantity));
}

bool QuantitySpinBox::commitInlineExpressionTextForUi()
{
    if (!commitInlineExpressionText()) {
        return false;
    }

    if (!isBound()) {
        emitCommittedUnboundValue();
    }

    return true;
}

void QuantitySpinBox::evaluateExpression()
{
    if (isBound() && getExpression()) {
        showValidExpression(Number::SetIfNumber);
    }
}

void Gui::QuantitySpinBox::setNumberExpression(App::NumberExpression* expr)
{
    updateEdit(getUserString(expr->getQuantity()));
    handlePendingEmit();
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

    if (event->text() == QLatin1String("=") && lineEdit()->text().trimmed().isEmpty()) {
        QAbstractSpinBox::keyPressEvent(event);
        return;
    }

    // On Enter, check for variable assignment syntax (e.g. "width=42")
    // or raw expression syntax (e.g. "sin(45)", "width*2").
    if (isEnter && commitInlineExpressionTextForUi()) {
        Q_EMIT returnPressed();
        return;
    }

    if (d->normalize && isEnter && !isNormalized()) {
        // ensure that input is up to date
        handlePendingEmit();

        normalize();
        return;
    }

    if (!handleKeyEvent(event->text())) {
        QAbstractSpinBox::keyPressEvent(event);
    }

    if (isEnter) {
        returnPressed();
    }
}

void Gui::QuantitySpinBox::paintEvent(QPaintEvent*)
{
    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    drawControl(opt);
}

void QuantitySpinBox::updateText(const Quantity& quant)
{
    Q_D(QuantitySpinBox);

    double dFactor;
    QString txt = getUserString(quant, dFactor, d->unitStr);
    d->unitValue = quant.getValue() / dFactor;
    updateEdit(txt);
    handlePendingEmit();
}

void QuantitySpinBox::updateEdit(const QString& text)
{
    Q_D(QuantitySpinBox);

    QLineEdit* edit = lineEdit();

    bool empty = edit->text().isEmpty();
    int cursor = edit->cursorPosition();
    int selsize = edit->selectedText().size();

    edit->setText(text);

    cursor = qBound(0, cursor, qMax(0, edit->displayText().size() - d->unitStr.size()));
    if (selsize > 0) {
        edit->setSelection(0, cursor);
    }
    else {
        edit->setCursorPosition(empty ? 0 : cursor);
    }
}

void QuantitySpinBox::validateInput()
{
    Q_D(QuantitySpinBox);

    QValidator::State state;
    QString text = lineEdit()->text();
    const App::ObjectIdentifier& path = getPath();
    d->validateAndInterpret(text, state, path);
    if (state != QValidator::Acceptable) {
        updateEdit(d->validStr);
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

void QuantitySpinBox::normalize()
{
    // this does not really change the value, only the representation
    QSignalBlocker blocker(this);

    Q_D(const QuantitySpinBox);
    return setValue(d->quantity);
}

bool QuantitySpinBox::isNormalized()
{
    static const QRegularExpression operators(
        QStringLiteral("[+\\-/*]"),
        QRegularExpression::CaseInsensitiveOption
    );

    Q_D(const QuantitySpinBox);

    // this check is two level
    // 1. We consider every string that does not contain operators as normalized
    // 2. If it does contain operators we check if it differs from normalized input - as some
    //    operators like - can be allowed even in normalized case.
    return !d->validStr.contains(operators)
        || d->validStr.toStdString() == d->quantity.getUserString();
}

void QuantitySpinBox::setValue(const Base::Quantity& value)
{
    Q_D(QuantitySpinBox);
    d->quantity = value;
    // check limits
    if (d->quantity.getValue() > d->maximum) {
        d->quantity.setValue(d->maximum);
    }
    if (d->quantity.getValue() < d->minimum) {
        d->quantity.setValue(d->minimum);
    }

    d->unit = value.getUnit();

    updateText(value);
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

bool QuantitySpinBox::hasValidInput() const
{
    Q_D(const QuantitySpinBox);
    return d->validInput;
}

/**
 * Detects "name=value" variable assignment syntax in dimension input.
 *
 * When the user types something like "width=42" or "h=10mm" or "ratio=x+5",
 * this creates (or updates) a named variable in a Spreadsheet and binds
 * the current property to reference that variable via an expression.
 *
 * This enables Fusion 360-style parametric workflow: define and assign
 * variables inline from any dimension input field.
 *
 * Returns true if the input was handled as a variable assignment.
 */
bool QuantitySpinBox::tryHandleVariableAssignment(const QString& text)
{
    Q_D(QuantitySpinBox);

    // Match: identifier = expression  (e.g. "width=42", "h=10 mm", "r=x+5")
    // The identifier must start with a letter or underscore, followed by alphanumerics/underscores.
    // Must not match things like "=42" (expression prefix) or "42" (plain number).
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

    // Don't treat known units or constants as variable names
    std::string nameStd = varName.toStdString();
    if (App::ExpressionParser::isTokenAUnit(nameStd)
        || App::ExpressionParser::isTokenAConstant(nameStd)) {
        return false;
    }

    // Validate the identifier
    if (nameStd != Base::Tools::getIdentifier(nameStd)) {
        return false;
    }

    App::Document* doc = nullptr;
    if (isBound()) {
        auto* docObj = getPath().getDocumentObject();
        if (docObj) {
            doc = docObj->getDocument();
        }
    }

    if (!doc) {
        doc = App::GetApplication().getActiveDocument();
    }

    if (!doc) {
        return false;
    }

    // Keep plain quantities as values and store everything else as a Spreadsheet formula.
    std::string cellContent = valueExpr.toStdString();
    if (!cellContent.empty() && cellContent[0] != '=') {
        if (isPlainQuantityLiteral(valueExpr)) {
            // Auto-inherit the field unit for dimensionless literals like "x=32" in
            // length/angle inputs. Explicit unit literals (e.g. "32 mm", "45 deg")
            // are preserved.
            try {
                Base::Quantity parsed = Base::Quantity::parse(cellContent);
                if (parsed.isDimensionless() && d->unit != Base::Unit::One) {
                    parsed.setUnit(d->unit);
                    cellContent = Base::UnitsApi::schemaTranslate(parsed);
                }
            }
            catch (...) {
            }
        }
        else {
            cellContent = "=" + cellContent;
        }
    }

    try {
        const std::string escapedDocName = Base::Tools::escapeQuotesFromString(doc->getName());
        const char* sheetName = kParametersSheetName;

        // Find or create a Spreadsheet named "Parameters".
        App::DocumentObject* sheetObj = doc->getObject(sheetName);
        if (!sheetObj) {
            Base::Interpreter().runStringArg(
                "App.getDocument('%s').addObject('Spreadsheet::Sheet', '%s')",
                escapedDocName.c_str(),
                sheetName
            );
            Base::Interpreter().runStringArg(
                "App.getDocument('%s').getObject('%s').Label = '%s'",
                escapedDocName.c_str(),
                sheetName,
                sheetName
            );
            sheetObj = doc->getObject(sheetName);
        }

        if (!sheetObj) {
            return false;
        }

        std::string existingAddr;
        std::vector<std::string> usedCells;
        readParameterSheetState(doc, nameStd, existingAddr, usedCells);

        bool aliasAlreadyExists = !existingAddr.empty();
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

        std::string escapedContent = Base::Tools::escapeQuotesFromString(cellContent);
        Base::Interpreter().runStringArg(
            "App.getDocument('%s').getObject('%s').set('%s', '%s')",
            escapedDocName.c_str(),
            sheetName,
            valueCellAddr.c_str(),
            escapedContent.c_str()
        );

        if (aliasAlreadyExists && existingAddr != valueCellAddr) {
            Base::Interpreter().runStringArg(
                "App.getDocument('%s').getObject('%s').setAlias('%s', '')",
                escapedDocName.c_str(),
                sheetName,
                existingAddr.c_str()
            );
        }

        if (!aliasAlreadyExists || existingAddr != valueCellAddr) {
            Base::Interpreter().runStringArg(
                "App.getDocument('%s').getObject('%s').setAlias('%s', '%s')",
                escapedDocName.c_str(),
                sheetName,
                valueCellAddr.c_str(),
                nameStd.c_str()
            );
        }

        Base::Interpreter().runStringArg(
            "App.getDocument('%s').getObject('%s').set('%s', '%s')",
            escapedDocName.c_str(),
            sheetName,
            aliasCellAddr.c_str(),
            nameStd.c_str()
        );

        // Recompute the spreadsheet to evaluate the cell
        Base::Interpreter().runStringArg(
            "App.getDocument('%s').getObject('%s').recompute()",
            escapedDocName.c_str(),
            sheetName
        );

        App::DocumentObject* contextObj = nullptr;
        if (isBound()) {
            contextObj = getPath().getDocumentObject();
        }
        if (!contextObj) {
            contextObj = doc->getObject(sheetName);
        }
        if (!contextObj && !doc->getObjects().empty()) {
            contextObj = doc->getObjects().front();
        }

        if (contextObj) {
            const std::string exprStr = std::string(sheetName) + "." + nameStd;
            std::shared_ptr<App::Expression> expr(
                App::ExpressionParser::parse(contextObj, exprStr.c_str())
            );
            if (!expr) {
                return false;
            }

            std::unique_ptr<App::Expression> result(expr->eval());
            auto* number = freecad_cast<App::NumberExpression*>(result.get());
            if (!number) {
                return false;
            }

            if (isBound()) {
                d->pendingEmit = false;
                d->validInput = true;
                setExpression(expr);
                updateExpression();
            }
            else {
                d->pendingEmit = false;
                d->validInput = true;

                const Base::Unit targetUnit = d->unit;
                Base::Quantity resolvedQuantity = number->getQuantity();
                const bool needsUnitPromotion = resolvedQuantity.isDimensionless()
                    && targetUnit != Base::Unit::One;
                if (needsUnitPromotion) {
                    resolvedQuantity.setUnit(targetUnit);
                }

                {
                    // Prevent reentrant valueChanged emissions from setValue() while
                    // we are still preparing transient unbound expression state.
                    QSignalBlocker blocker(this);
                    setValue(resolvedQuantity);
                }

                std::string expressionToStore = exprStr;
                if (needsUnitPromotion) {
                    std::string unitFactor = Base::UnitsApi::schemaTranslate(
                        Base::Quantity(1.0, targetUnit)
                    );
                    expressionToStore = "(" + expressionToStore + ")*(" + unitFactor + ")";
                }
                d->unboundExpressionText = std::move(expressionToStore);
            }
        }
        else {
            return false;
        }

        return true;
    }
    catch (const Base::Exception& e) {
        qWarning() << "Variable assignment failed:" << e.what();
        return false;
    }
    catch (...) {
        qWarning() << "Variable assignment failed with unknown error";
        return false;
    }
}

bool QuantitySpinBox::tryHandleRawExpression(const QString& text)
{
    Q_D(QuantitySpinBox);

    QString expressionText = text.trimmed();

    if (expressionText.startsWith(QLatin1Char('='))) {
        expressionText = expressionText.mid(1).trimmed();
    }

    expressionText = trimTrailingStatementDelimiter(expressionText);

    // Skip empty input or plain numbers (let normal validation handle those)
    if (expressionText.isEmpty()) {
        return false;
    }

    // Let plain quantities go through the regular unit path.
    if (isPlainQuantityLiteral(expressionText)) {
        return false;
    }

    // Try to parse as an expression
    App::DocumentObject* docObj = nullptr;
    App::Document* doc = nullptr;
    if (isBound()) {
        docObj = getPath().getDocumentObject();
        if (docObj) {
            doc = docObj->getDocument();
        }
    }

    if (!docObj) {
        doc = App::GetApplication().getActiveDocument();
        if (doc) {
            docObj = doc->getObject(kParametersSheetName);
            if (!docObj && !doc->getObjects().empty()) {
                docObj = doc->getObjects().front();
            }
        }
    }

    std::shared_ptr<App::Expression> expr;
    std::string parseText = buildQualifiedExpression(doc, expressionText);

    try {
        expr.reset(App::ExpressionParser::parse(docObj, parseText.c_str()));
    }
    catch (...) {
    }

    try {
        if (expr) {
            std::unique_ptr<App::Expression> result(expr->eval());
            auto* number = freecad_cast<App::NumberExpression*>(result.get());
            if (!number) {
                return false;
            }

            if (isBound()) {
                d->pendingEmit = false;
                d->validInput = true;
                setExpression(expr);
                updateExpression();
            }
            else {
                d->pendingEmit = false;
                d->validInput = true;

                const Base::Unit targetUnit = d->unit;
                Base::Quantity resolvedQuantity = number->getQuantity();
                const bool needsUnitPromotion = resolvedQuantity.isDimensionless()
                    && targetUnit != Base::Unit::One;
                if (needsUnitPromotion) {
                    resolvedQuantity.setUnit(targetUnit);
                }

                {
                    // Prevent reentrant valueChanged emissions from setValue() while
                    // we are still preparing transient unbound expression state.
                    QSignalBlocker blocker(this);
                    setValue(resolvedQuantity);
                }

                std::string expressionString = expr->toString();
                if (doc) {
                    expressionString = qualifyVariableAliasesInExpression(doc, expressionString);
                }

                if (needsUnitPromotion) {
                    std::string unitFactor = Base::UnitsApi::schemaTranslate(
                        Base::Quantity(1.0, targetUnit)
                    );
                    expressionString = "(" + expressionString + ")*(" + unitFactor + ")";
                }

                d->unboundExpressionText = std::move(expressionString);
            }
            return true;
        }
    }
    catch (...) {
        // Expression parse failed — fall through to normal input handling
    }

    return false;
}

// Gets called after call of 'validateAndInterpret'
void QuantitySpinBox::userInput(const QString& text)
{
    Q_D(QuantitySpinBox);

    if (!isBound()) {
        d->unboundExpressionText.clear();
    }

    d->pendingEmit = true;

    QString tmp = text;
    Base::Quantity res;
    const App::ObjectIdentifier& path = getPath();
    if (d->validate(tmp, res, path)) {
        d->validStr = tmp;
        d->validInput = true;
    }
    else {
        d->validInput = false;

        // only emit signal to reset EditableDatumLabel if the input is truly empty or has
        // no meaningful number don't emit for partially typed numbers like "71." which are
        // temporarily invalid
        const QString trimmedText = text.trimmed();
        static const QRegularExpression partialNumberRegex(QStringLiteral(R"([+-]?(\d+)?(\.,\d*)?)"));
        if ((trimmedText.isEmpty() || !trimmedText.contains(partialNumberRegex))
            && !lineEdit()->hasFocus()) {
            // we have to emit here signal explicitly as validator will not pass
            // this value further but we want to check it to disable isSet flag if
            // it has been set previously
            Q_EMIT valueChanged(d->quantity.getValue());
        }
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
            Q_EMIT valueChanged(res);
            Q_EMIT valueChanged(res.getValue());
            Q_EMIT textChanged(text);
        }
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
    d->quantity.setUnit(unit);
    updateText(d->quantity);
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
    std::string unitStr;
    const std::string str = d->scheme ? val.getUserString(d->scheme.get(), factor, unitStr)
                                      : val.getUserString(factor, unitStr);
    unitString = QString::fromStdString(unitStr);
    return QString::fromStdString(str);
}

QString QuantitySpinBox::getUserString(const Base::Quantity& val) const
{
    Q_D(const QuantitySpinBox);
    std::string str;
    if (d->scheme) {
        double factor;
        std::string unitString;
        str = val.getUserString(d->scheme.get(), factor, unitString);
    }
    else {
        str = val.getUserString();
    }
    return QString::fromStdString(str);
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
    updateFromCache(false);

    double step = d->singleStep * steps;
    double val = d->unitValue + step;
    if (val > d->maximum) {
        val = d->maximum;
    }
    else if (val < d->minimum) {
        val = d->minimum;
    }

    Quantity quant(val, d->unitStr.toStdString());
    quant.setFormat(d->quantity.getFormat());
    updateText(quant);
    updateFromCache(true);
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
    return sizeHintCalculator(lineEdit()->sizeHint().height());
}

QSize QuantitySpinBox::minimumSizeHint() const
{
    return sizeHintCalculator(lineEdit()->minimumSizeHint().height());
}

QSize QuantitySpinBox::sizeHintCalculator(int h) const
{
    Q_D(const QuantitySpinBox);
    ensurePolished();

    const QFontMetrics fm(fontMetrics());
    int w = 0;
    constexpr int maxStrLen = 9;

    QString s;
    QString fixedContent = QLatin1String(" ");

    Base::Quantity q(d->quantity);
    q.setValue(d->maximum);
    s = textFromValue(q);
    s.truncate(maxStrLen);
    s += fixedContent;
    w = qMax(w, QtTools::horizontalAdvance(fm, s));

    w += 2;  // cursor blinking space
    w += iconHeight;

    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    QSize hint(w, h);

    QSize size = style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this);
    return size;
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

    // Some flows commit via focus transfer (e.g. click OK) instead of pressing
    // Enter inside the field. Handle inline expression/assignment here too.
    if (commitInlineExpressionTextForUi()) {
        QToolTip::hideText();
        QAbstractSpinBox::focusOutEvent(event);
        return;
    }

    validateInput();

    if (d->normalize) {
        normalize();
    }

    QToolTip::hideText();
    QAbstractSpinBox::focusOutEvent(event);
}

void QuantitySpinBox::clear()
{
    QAbstractSpinBox::clear();
}

void QuantitySpinBox::selectNumber()
{
    QString expr = QStringLiteral("^([%1%2]?[0-9\\%3]*)\\%4?([0-9]+(%5[%1%2]?[0-9]+)?)")
                       .arg(locale().negativeSign())
                       .arg(locale().positiveSign())
                       .arg(locale().groupSeparator())
                       .arg(locale().decimalPoint())
                       .arg(locale().exponential());
    auto rmatch = QRegularExpression(expr).match(lineEdit()->text());
    if (rmatch.hasMatch()) {
        lineEdit()->setSelection(0, rmatch.capturedLength());
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

    QString copy = text;
    QValidator::State state = QValidator::Acceptable;
    const App::ObjectIdentifier& path = getPath();
    Base::Quantity quant = d->validateAndInterpret(copy, state, path);
    if (state != QValidator::Acceptable) {
        fixup(copy);
        quant = d->validateAndInterpret(copy, state, path);
    }

    return quant;
}

QValidator::State QuantitySpinBox::validate(QString& text, int& pos) const
{
    Q_D(const QuantitySpinBox);
    Q_UNUSED(pos)

    QValidator::State state;
    const App::ObjectIdentifier& path = getPath();
    d->validateAndInterpret(text, state, path);
    return state;
}

void QuantitySpinBox::fixup(QString& input) const
{
    input.remove(locale().groupSeparator());
}


#include "moc_QuantitySpinBox.cpp"
#include "moc_QuantitySpinBox_p.cpp"
