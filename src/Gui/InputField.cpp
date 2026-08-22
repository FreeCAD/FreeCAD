/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <optional>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPixmapCache>

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <App/PropertyUnits.h>
#include <App/QuantityInput.h>
#include <Base/Exception.h>
#include <Base/Quantity.h>
#include <Base/UnitsApi.h>

#include "InputField.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "NumericLocale.h"
#include "QuantitySpinBox_p.h"


using namespace Gui;
using namespace App;
using namespace Base;

// --------------------------------------------------------------------

namespace Gui
{
class InputValidator: public QValidator
{
public:
    explicit InputValidator(InputField* parent);
    ~InputValidator() override;

    State validate(QString& input, int& pos) const override;

private:
    InputField* dptr;
};
}  // namespace Gui

// --------------------------------------------------------------------

InputField::InputField(QWidget* parent)
    : ExpressionLineEdit(parent)
    , ExpressionWidget()
    , validInput(true)
    , actUnitValue(0)
    , Maximum(std::numeric_limits<double>::max())
    , Minimum(-std::numeric_limits<double>::max())
    , StepSize(1.0)
    , HistorySize(5)
    , SaveSize(5)
{
    setValidator(new InputValidator(this));
    if (!App::GetApplication()
             .GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")
             ->GetBool("ComboBoxWheelEventFilter", false)) {
        setFocusPolicy(Qt::WheelFocus);
    }
    else {
        setFocusPolicy(Qt::StrongFocus);
    }
    iconLabel = new ExpressionLabel(this);
    iconLabel->setCursor(Qt::ArrowCursor);
    QFontMetrics fm(font());
    int iconSize = fm.height();
    QPixmap pixmap = getValidationIcon(":/icons/button_invalid.svg", QSize(iconSize, iconSize));
    iconLabel->setPixmap(pixmap);
    iconLabel->hide();
    connect(this, &QLineEdit::textChanged, this, &InputField::updateIconLabel);

    // Set Margins
    // vertical margin, such that `,` won't be clipped to a `.` and similar font descents. Relevant
    // on some OSX versions horizontal margin, such that text will not be behind `fx` icon
    int margin = getMargin();
    setTextMargins(margin, margin, margin + iconSize, margin);

    this->setContextMenuPolicy(Qt::DefaultContextMenu);

    connect(this, &QLineEdit::textChanged, this, &InputField::newInput);
}

int InputField::getMargin()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    return style()->pixelMetric(QStyle::PM_LineEditIconMargin, nullptr, this) / 2;
#else
    return style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, this);
#endif
}

InputField::~InputField() = default;

void InputField::bind(const App::ObjectIdentifier& _path)
{
    ExpressionBinding::bind(_path);

    auto* prop = freecad_cast<PropertyQuantity*>(getPath().getProperty());

    if (prop) {
        actQuantity = Base::Quantity(prop->getValue());
    }

    DocumentObject* docObj = getPath().getDocumentObject();

    if (docObj) {
        std::shared_ptr<const Expression> expr(docObj->getExpression(getPath()).expression);

        if (expr) {
            newInput(QString::fromStdString(expr->toString()));
        }
    }

    // Create document object, to initialize completer
    setDocumentObject(docObj);
}

bool InputField::apply(const std::string& propName)
{
    if (!ExpressionBinding::apply(propName)) {
        Gui::Command::doCommand(Gui::Command::Doc, "%s = %f", propName.c_str(), getQuantity().getValue());
        return true;
    }
    else {
        return false;
    }
}

bool InputField::apply()
{
    return ExpressionBinding::apply();
}

QPixmap InputField::getValidationIcon(const char* name, const QSize& size) const
{
    QString key
        = QStringLiteral("%1_%2x%3").arg(QString::fromLatin1(name)).arg(size.width()).arg(size.height());
    QPixmap icon;
    if (QPixmapCache::find(key, &icon)) {
        return icon;
    }

    icon = BitmapFactory().pixmapFromSvg(name, size);
    if (!icon.isNull()) {
        QPixmapCache::insert(key, icon);
    }
    return icon;
}

void InputField::updateText(const Base::Quantity& quant)
{
    if (isBound()) {
        std::shared_ptr<const Expression> e(
            getPath().getDocumentObject()->getExpression(getPath()).expression
        );

        if (e) {
            setText(QString::fromStdString(e->toString()));
            return;
        }
    }

    double dFactor;
    std::string unitStr;
    const auto formatting = Gui::numericLocaleContextFor(locale());
    auto displayQuantity = quant;
    displayQuantity.setFormat(Gui::editableQuantityFormat(quant.getFormat(), formatting));
    std::string txt = Base::UnitsApi::schemaTranslate(displayQuantity, formatting, dFactor, unitStr);
    actUnitValue = quant.getValue() / dFactor;
    // Block signals to prevent newInput from re-parsing the display text
    // and overwriting actQuantity with a precision-truncated value.
    QSignalBlocker blocker(this);
    setText(QString::fromStdString(txt));
}

App::QuantityInputResult InputField::interpretInput(const QString& input, const App::InputPhase phase) const
{
    const auto formatting = Gui::numericLocaleContextFor(locale());
    App::QuantityConstraints constraints;
    if (actUnit != Unit::One) {
        constraints.requiredUnit = actUnit;
    }
    constraints.minimum = Minimum;
    constraints.maximum = Maximum;

    const auto grammar = isBound() ? App::QuantityInputGrammar::Expression
                                   : App::QuantityInputGrammar::Quantity;
    const auto parse = [&](const App::QuantityInputGrammar selectedGrammar) {
        return App::interpretQuantityInput(
            input.toUtf8().toStdString(),
            selectedGrammar,
            getPath(),
            actUnit,
            formatting,
            phase,
            constraints
        );
    };

    auto result = parse(grammar);
    // Keep quantity-only syntax on the quantity parser, but preserve the established arithmetic
    // behavior of unbound fields when the quantity grammar reports a syntax failure.
    if (grammar == App::QuantityInputGrammar::Quantity && result.status == App::InputStatus::Invalid
        && result.diagnostic
        && result.diagnostic->kind == App::InputDiagnosticKind::ExpressionSyntax) {
        result = parse(App::QuantityInputGrammar::Expression);
    }
    return result;
}

void InputField::notifyValueChanged()
{
    updateText(actQuantity);
    Q_EMIT valueChanged(actQuantity);
    Q_EMIT valueChanged(actQuantity.getValue());
}

void InputField::resizeEvent(QResizeEvent* /*event*/)
{
    QSize iconSize = iconLabel->sizeHint();
    iconLabel->move(width() - (iconSize.width() + 2 * getMargin()), (height() - iconSize.height()) / 2);
}

void InputField::changeEvent(QEvent* event)
{
    QLineEdit::changeEvent(event);
    if (event->type() == QEvent::LocaleChange && validInput) {
        updateText(actQuantity);
    }
}

void InputField::updateIconLabel(const QString& text)
{
    iconLabel->setVisible(text.isEmpty());
}

void InputField::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu* editMenu = createStandardContextMenu();
    editMenu->setTitle(tr("Edit"));
    auto menu = new QMenu(QStringLiteral("InputFieldContextmenu"));

    menu->addMenu(editMenu);
    menu->addSeparator();

    // datastructure to remember actions for values
    std::vector<QString> values;
    std::vector<QAction*> actions;

    // add the history menu part...
    std::vector<QString> history = getHistory();

    for (const auto& it : history) {
        actions.push_back(menu->addAction(it));
        values.push_back(it);
    }

    // add the save value portion of the menu
    menu->addSeparator();
    QAction* SaveValueAction = menu->addAction(tr("Save Value"));
    std::vector<QString> savedValues = getSavedValues();

    for (const auto& savedValue : savedValues) {
        actions.push_back(menu->addAction(savedValue));
        values.push_back(savedValue);
    }

    // call the menu and wait until its back
    QAction* saveAction = menu->exec(event->globalPos());

    // look what the user has chosen
    if (saveAction == SaveValueAction) {
        pushToSavedValues();
    }
    else {
        int i = 0;
        for (auto it = actions.begin(); it != actions.end(); ++it, i++) {
            if (*it == saveAction) {
                this->setText(values[i]);
            }
        }
    }

    delete menu;
}

void InputField::newInput(const QString& text)
{
    const auto formatting = Gui::numericLocaleContextFor(locale());
    const auto result = interpretInput(text, App::InputPhase::Editing);

    if (result.status != App::InputStatus::Acceptable) {
        validInput = false;
        if (result.status == App::InputStatus::Invalid && result.diagnostic) {
            Q_EMIT parseError(Gui::numericInputDiagnosticText(result.diagnostic->kind));
        }
        return;
    }

    Quantity res = *result.quantity;

    double dFactor;
    std::string unitStr;
    Base::UnitsApi::schemaTranslate(res, formatting, dFactor, unitStr);
    actUnitValue = res.getValue() / dFactor;
    // Preserve previous format
    res.setFormat(this->actQuantity.getFormat());

    // Commit the expression and value only after parsing, evaluation, unit validation, and range
    // handling have all succeeded.
    if (result.expression) {
        setExpression(result.expression);
    }
    actQuantity = res;

    if (iconLabel->isVisible()) {
        iconLabel->setVisible(false);
    }
    validInput = true;

    // signaling
    Q_EMIT valueChanged(res);
    Q_EMIT valueChanged(res.getValue());
}

void InputField::commitInput()
{
    const auto result = interpretInput(text(), App::InputPhase::Commit);
    if (result.status != App::InputStatus::Acceptable) {
        validInput = false;
        if (result.diagnostic) {
            Q_EMIT parseError(Gui::numericInputDiagnosticText(result.diagnostic->kind));
        }
        return;
    }

    if (result.expression) {
        setExpression(result.expression);
    }
    auto quantity = *result.quantity;
    quantity.setFormat(actQuantity.getFormat());
    actQuantity = quantity;
    validInput = true;
}

void InputField::pushToHistory(const QString& valueq)
{
    QString val;
    if (valueq.isEmpty()) {
        val = this->text();
    }
    else {
        val = valueq;
    }

    // check if already in:
    std::vector<QString> hist = InputField::getHistory();
    for (const auto& it : hist) {
        if (it == val) {
            return;
        }
    }

    std::string value(val.toUtf8());
    if (_handle.isValid()) {
        char hist1[21];
        char hist0[21];
        for (int i = HistorySize - 1; i >= 0; i--) {
            snprintf(hist1, 20, "Hist%i", i + 1);
            snprintf(hist0, 20, "Hist%i", i);
            std::string tHist = _handle->GetASCII(hist0, "");
            if (!tHist.empty()) {
                _handle->SetASCII(hist1, tHist.c_str());
            }
        }
        _handle->SetASCII("Hist0", value.c_str());
    }
}

std::vector<QString> InputField::getHistory()
{
    std::vector<QString> res;

    if (_handle.isValid()) {
        std::string tmp;
        char hist[21];
        for (int i = 0; i < HistorySize; i++) {
            snprintf(hist, 20, "Hist%i", i);
            tmp = _handle->GetASCII(hist, "");
            if (!tmp.empty()) {
                res.push_back(QString::fromUtf8(tmp.c_str()));
            }
            else {
                break;  // end of history reached
            }
        }
    }
    return res;
}

void InputField::setToLastUsedValue()
{
    std::vector<QString> hist = getHistory();
    if (!hist.empty()) {
        this->setText(hist[0]);
    }
}

void InputField::pushToSavedValues(const QString& valueq)
{
    const QByteArray valueUtf8 = valueq.isEmpty() ? this->text().toUtf8() : valueq.toUtf8();
    std::string value(valueUtf8.constData(), valueUtf8.size());

    if (_handle.isValid()) {
        char hist1[21];
        char hist0[21];
        for (int i = SaveSize - 1; i >= 0; i--) {
            snprintf(hist1, 20, "Save%i", i + 1);
            snprintf(hist0, 20, "Save%i", i);
            std::string tHist = _handle->GetASCII(hist0, "");
            if (!tHist.empty()) {
                _handle->SetASCII(hist1, tHist.c_str());
            }
        }
        _handle->SetASCII("Save0", value.c_str());
    }
}

std::vector<QString> InputField::getSavedValues()
{
    std::vector<QString> res;

    if (_handle.isValid()) {
        std::string tmp;
        char hist[21];
        for (int i = 0; i < SaveSize; i++) {
            snprintf(hist, 20, "Save%i", i);
            tmp = _handle->GetASCII(hist, "");
            if (!tmp.empty()) {
                res.push_back(QString::fromUtf8(tmp.c_str()));
            }
            else {
                break;  // end of history reached
            }
        }
    }
    return res;
}

/** Sets the preference path to \a path. */
void InputField::setParamGrpPath(const QByteArray& path)
{
    _handle = App::GetApplication().GetParameterGroupByPath(path);
    if (_handle.isValid()) {
        sGroupString = (const char*)path;
    }
}

/** Returns the widget's preferences path. */
QByteArray InputField::paramGrpPath() const
{
    if (_handle.isValid()) {
        return sGroupString.c_str();
    }
    return {};
}

/// sets the field with a quantity
void InputField::setValue(const Base::Quantity& quant)
{
    actQuantity = quant;
    // check limits
    if (actQuantity.getValue() > Maximum) {
        actQuantity.setValue(Maximum);
    }
    if (actQuantity.getValue() < Minimum) {
        actQuantity.setValue(Minimum);
    }

    actUnit = quant.getUnit();

    notifyValueChanged();
}

void InputField::setValue(const double& value)
{
    setValue(Base::Quantity(value, actUnit));
}

double InputField::rawValue() const
{
    return this->actQuantity.getValue();
}

void InputField::setUnit(const Base::Unit& unit)
{
    actUnit = unit;
    actQuantity.setUnit(unit);
    updateText(actQuantity);
}

const Base::Unit& InputField::getUnit() const
{
    return actUnit;
}

/// get stored, valid quantity as a string
QString InputField::getQuantityString() const
{
    double factor;
    std::string unitString;
    const auto formatting = Gui::numericLocaleContextFor(locale());
    return QString::fromStdString(
        Base::UnitsApi::schemaTranslate(actQuantity, formatting, factor, unitString)
    );
}

/// set, validate and display quantity from a string. Must match existing units.
void InputField::setQuantityString(const QString& text)
{
    // Input and then format the quantity
    newInput(text);
    updateText(actQuantity);
}

/// return the quantity in C locale, i.e. decimal separator is a dot.
QString InputField::rawText() const
{
    double factor;
    std::string unit;
    double value = actQuantity.getValue();
    const auto formatting = Gui::numericLocaleContextFor(locale());
    Base::UnitsApi::schemaTranslate(actQuantity, formatting, factor, unit);
    return QStringLiteral("%1 %2").arg(value / factor).arg(QString::fromStdString(unit));
}

/// expects the string in C locale and internally converts it into the OS-specific locale
void InputField::setRawText(const QString& text)
{
    Base::Quantity quant = Base::Quantity::parse(text.toStdString());
    // Input and then format the quantity
    newInput(QString::fromStdString(quant.getSafeUserString()));
    updateText(actQuantity);
}

/// get the value of the singleStep property
double InputField::singleStep() const
{
    return StepSize;
}

/// set the value of the singleStep property
void InputField::setSingleStep(double s)
{
    StepSize = s;
}

/// get the value of the maximum property
double InputField::maximum() const
{
    return Maximum;
}

/// set the value of the maximum property
void InputField::setMaximum(double m)
{
    Maximum = m;
    if (actQuantity.getValue() > Maximum) {
        actQuantity.setValue(Maximum);
        notifyValueChanged();
    }
}

/// get the value of the minimum property
double InputField::minimum() const
{
    return Minimum;
}

/// set the value of the minimum property
void InputField::setMinimum(double m)
{
    Minimum = m;
    if (actQuantity.getValue() < Minimum) {
        actQuantity.setValue(Minimum);
        notifyValueChanged();
    }
}

void InputField::setUnitText(const QString& str)
{
    try {
        Base::Quantity quant = Base::Quantity::parse(str.toStdString());
        setUnit(quant.getUnit());
    }
    catch (...) {
        // ignore exceptions
    }
}

QString InputField::getUnitText()
{
    double dFactor;
    std::string unitStr;
    actQuantity.getUserString(dFactor, unitStr);
    return QString::fromStdString(unitStr);
}

int InputField::getPrecision() const
{
    return this->actQuantity.getFormat().getPrecision();
}

void InputField::setPrecision(const int precision)
{
    Base::QuantityFormat format = actQuantity.getFormat();
    format.setPrecision(precision);
    actQuantity.setFormat(format);
    updateText(actQuantity);
}

QString InputField::getFormat() const
{
    return {QChar::fromLatin1(actQuantity.getFormat().toFormat())};
}

void InputField::setFormat(const QString& format)
{
    if (format.isEmpty()) {
        return;
    }
    QChar c = format[0];
    Base::QuantityFormat f = this->actQuantity.getFormat();
    f.format = Base::QuantityFormat::toFormat(c.toLatin1());
    actQuantity.setFormat(f);
    updateText(actQuantity);
}

// get the value of the minimum property
int InputField::historySize() const
{
    return HistorySize;
}

// set the value of the minimum property
void InputField::setHistorySize(int i)
{
    assert(i >= 0);
    assert(i < 100);

    HistorySize = i;
}

void InputField::selectNumber()
{
    const auto length
        = Gui::numericInputSelectionLength(text(), Gui::numericLocaleContextFor(locale()));
    if (length > 0) {
        setSelection(0, length);
    }
}

void InputField::showEvent(QShowEvent* event)
{
    QLineEdit::showEvent(event);

    bool selected = this->hasSelectedText();
    updateText(actQuantity);
    if (selected) {
        selectNumber();
    }
}

void InputField::focusInEvent(QFocusEvent* event)
{
    if (event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason
        || event->reason() == Qt::ShortcutFocusReason) {
        if (!this->hasSelectedText()) {
            selectNumber();
        }
    }

    QLineEdit::focusInEvent(event);
}

void InputField::focusOutEvent(QFocusEvent* event)
{
    commitInput();
    if (validInput) {
        updateText(actQuantity);
    }
    QLineEdit::focusOutEvent(event);
}

void InputField::keyPressEvent(QKeyEvent* event)
{
    if (isReadOnly()) {
        QLineEdit::keyPressEvent(event);
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        commitInput();
        if (validInput) {
            Q_EMIT returnPressed();
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        QSignalBlocker blocker(this);
        updateText(actQuantity);
        validInput = true;
        event->accept();
        return;
    }

    double val = actUnitValue;

    switch (event->key()) {
        case Qt::Key_Up:
            val += StepSize;
            if (val > Maximum) {
                val = Maximum;
            }
            break;
        case Qt::Key_Down:
            val -= StepSize;
            if (val < Minimum) {
                val = Minimum;
            }
            break;
        default:
            QLineEdit::keyPressEvent(event);
            return;
    }

    double dFactor;
    std::string unitStr;
    const auto formatting = Gui::numericLocaleContextFor(locale());
    Base::UnitsApi::schemaTranslate(actQuantity, formatting, dFactor, unitStr);
    this->setText(QStringLiteral("%L1 %2").arg(val).arg(QString::fromStdString(unitStr)));
    event->accept();
}

void InputField::wheelEvent(QWheelEvent* event)
{
    if (!hasFocus()) {
        return;
    }

    if (isReadOnly()) {
        QLineEdit::wheelEvent(event);
        return;
    }

    double factor = event->modifiers() & Qt::ControlModifier ? 10 : 1;
    double step = event->angleDelta().y() > 0 ? StepSize : -StepSize;
    double val = actUnitValue + factor * step;
    if (val > Maximum) {
        val = Maximum;
    }
    else if (val < Minimum) {
        val = Minimum;
    }

    double dFactor;
    std::string unitStr;
    const auto formatting = Gui::numericLocaleContextFor(locale());
    Base::UnitsApi::schemaTranslate(actQuantity, formatting, dFactor, unitStr);

    this->setText(QStringLiteral("%L1 %2").arg(val).arg(QString::fromStdString(unitStr)));
    selectNumber();
    event->accept();
}

QValidator::State InputField::validate(QString& input, int& pos) const
{
    Q_UNUSED(pos);
    const auto result = interpretInput(input, App::InputPhase::Editing);
    return result.status == App::InputStatus::Acceptable ? QValidator::Acceptable
                                                         : QValidator::Intermediate;
}

// --------------------------------------------------------------------

InputValidator::InputValidator(InputField* parent)
    : QValidator(parent)
    , dptr(parent)
{}

InputValidator::~InputValidator() = default;

QValidator::State InputValidator::validate(QString& input, int& pos) const
{
    return dptr->validate(input, pos);
}


#include "moc_InputField.cpp"
