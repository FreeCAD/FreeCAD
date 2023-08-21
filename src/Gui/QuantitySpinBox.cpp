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
# include <QLineEdit>
# include <QRegularExpression>
# include <QRegularExpressionMatch>
# include <QStyle>
# include <QStyleOptionSpinBox>
# include <QToolTip>
#endif

#include <sstream>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <Base/Exception.h>
#include <Base/UnitsApi.h>
#include <Base/Tools.h>

#include "QuantitySpinBox.h"
#include "QuantitySpinBox_p.h"
#include "Command.h"
#include "DlgExpressionInput.h"
#include "Tools.h"


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
      checkRangeInExpression(false),
      unitValue(0),
      maximum(DOUBLE_MAX),
      minimum(-DOUBLE_MAX),
      singleStep(1.0),
      q_ptr(q)
    {
    }
    ~QuantitySpinBoxPrivate() = default;

    QString stripped(const QString &t, int *pos) const
    {
        QString text = t;
        const int s = text.size();
        text = text.trimmed();
        if (pos)
            (*pos) -= (s - text.size());
        return text;
    }

    bool validate(QString& input, Base::Quantity& result, const App::ObjectIdentifier& path) const
    {
        Q_Q(const QuantitySpinBox);

        // Do not accept empty strings because the parser will consider
        // " unit" as "1 unit" which is not the desired behaviour (see #0004104)
        if (input.isEmpty())
            return false;

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

                // Now translate the quantity into its string representation using the user-defined unit system
                input = Base::UnitsApi::schemaTranslate(result);
            }
        }

        return success;
    }
    bool parseString(const QString& str, Base::Quantity& result, double& value, const App::ObjectIdentifier& path) const
    {
        App::ObjectIdentifier pathtmp = path;
        try {
            QString copy = str;
            copy.remove(locale.groupSeparator());

            //Expression parser
            std::shared_ptr<Expression> expr(ExpressionParser::parse(path.getDocumentObject(), copy.toUtf8().constData()));
            if (expr) {

                std::unique_ptr<Expression> res(expr->eval());
                NumberExpression * n = Base::freecad_dynamic_cast<NumberExpression>(res.get());
                if (n){
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
    Base::Quantity validateAndInterpret(QString& input, QValidator::State& state, const App::ObjectIdentifier& path) const
    {
        Base::Quantity res;
        const double max = this->maximum;
        const double min = this->minimum;

        QString copy = input;
        double value = min;
        bool ok = false;

        QChar plus = QLatin1Char('+'), minus = QLatin1Char('-');

        if (locale.negativeSign() != minus)
            copy.replace(locale.negativeSign(), minus);
        if (locale.positiveSign() != plus)
            copy.replace(locale.positiveSign(), plus);

        QString reverseUnitStr = unitStr;
        std::reverse(reverseUnitStr.begin(), reverseUnitStr.end());

        //Prep for expression parser
        //This regex matches chunks between +,-,$,^ accounting for matching parenthesis.
        QRegularExpression chunkRe(QString::fromUtf8("(?<=^|[\\+\\-])((\\((?>[^()]|(?2))*\\))|[^\\+\\-\n])*(?=$|[\\+\\-])"));
        QRegularExpressionMatchIterator expressionChunk = chunkRe.globalMatch(copy);
        unsigned int lengthOffset = 0;
        while (expressionChunk.hasNext()) {
            QRegularExpressionMatch matchChunk = expressionChunk.next();
            QString origionalChunk = matchChunk.captured(0);
            QString copyChunk = origionalChunk;
            std::reverse(copyChunk.begin(), copyChunk.end());

            //Reused regex patterns
            static const std::string regexUnits = "sAV|VC|lim|nim|im|hpm|[mf]?bl|°|ged|dar|nog|″|′|rroT[uµm]?|K[uµm]?|A[mkM]?|F[pnuµm]?|C|S[uµmkM]?|zH[kMGT]?|H[nuµm]?|mhO[kM]?|J[mk]?|Ve[kM]?|V[mk]?|hWk|sW|lack?|N[mkM]?|g[uµmk]?|lm?|(?<=\\b|[^a-zA-Z])m[nuµmcdk]?|uoht|ni|\"|'|dy|dc|bW|T|t|zo|ts|twc|Wk?|aP[kMG]?|is[pk]|h|G|M|tfc|tfqs|tf|s";
            static const std::string regexUnitlessFunctions = "soca|nisa|2nata|nata|hsoc|hnis|hnat|soc|nat|nis|pxe|gol|01gol";
            static const std::string regexConstants = "e|ip|lomm|lom";
            static const std::string regexNumber = "\\d+\\s*\\.?\\s*\\d*|\\.\\s*\\d+";

            // If expression does not contain /*() or ^, this regex will not find anything
            if (copy.contains(QLatin1Char('/')) || copy.contains(QLatin1Char('*')) || copy.contains(QLatin1Char('(')) || copy.contains(QLatin1Char(')')) || copy.contains(QLatin1Char('^'))){
                //Find units and replace 1/2mm -> 1/2*(1mm), 1^2mm -> 1^2*(1mm)
                QRegularExpression fixUnits(QString::fromStdString("("+regexUnits+")(\\s*\\)|(?:\\*|(?:\\)(?:(?:\\s*(?:"+regexConstants+"|\\)(?:[^()]|(?R))*\\((?:"+regexUnitlessFunctions+")|"+regexNumber+"))|(?R))*\\(|(?:\\s*(?:"+regexConstants+"|\\)(?:[^()]|(?R))*\\((?:"+regexUnitlessFunctions+")|"+regexNumber+"))))+(?:[\\/\\^]|(.*$))(?!("+regexUnits+")))"));
                QRegularExpressionMatch fixUnitsMatch = fixUnits.match(copyChunk);

                //3rd capture group being filled indicates regex bailed out; no match.
                if (fixUnitsMatch.lastCapturedIndex() == 2 || (fixUnitsMatch.lastCapturedIndex() == 3 && fixUnitsMatch.captured(3).isEmpty())){
                    QString matchUnits = fixUnitsMatch.captured(1);
                    QString matchNumbers = fixUnitsMatch.captured(2);
                    copyChunk.replace(matchUnits+matchNumbers, QString::fromUtf8(")")+matchUnits+QString::fromUtf8("1(*")+matchNumbers);
                }
            }

            //Add default units to string if none are present
            if (!copyChunk.contains(reverseUnitStr)){ // Fast check
                QRegularExpression unitsRe(QString::fromStdString("(?<=\\b|[^a-zA-Z])("+regexUnits+")(?=\\b|[^a-zA-Z])|°|″|′|\"|'|\\p{L}\\.\\p{L}|\\[\\p{L}"));

                QRegularExpressionMatch match = unitsRe.match(copyChunk);
                if (!match.hasMatch() && !copyChunk.isEmpty()) //If no units are found, use default units
                    copyChunk.prepend(QString::fromUtf8(")")+reverseUnitStr+QString::fromUtf8("1(*")); // Add units to the end of chunk *(1unit)
            }

            std::reverse(copyChunk.begin(), copyChunk.end());

            copy.replace(matchChunk.capturedStart() + lengthOffset,
                    matchChunk.capturedEnd() - matchChunk.capturedStart(), copyChunk);
            lengthOffset += copyChunk.length() - origionalChunk.length();
        }

        ok = parseString(copy, res, value, path);

        // If result does not have unit: add default unit
        if (res.getUnit().isEmpty()){
            res.setUnit(unit);
        }

        if (!ok) {
            // input may not be finished
            state = QValidator::Intermediate;
        }
        else if (value >= min && value <= max) {
                state = QValidator::Acceptable;
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
        if (state != QValidator::Acceptable) {
            res.setValue(max > 0 ? min : max);
        }

        return res;
    }

    QLocale locale;
    bool validInput;
    bool pendingEmit;
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
    connect(lineEdit(), &QLineEdit::textChanged,
            this, &QuantitySpinBox::userInput);
    connect(this, &QuantitySpinBox::editingFinished,
            this, [&]{
        this->handlePendingEmit(true);
    });

    // When a style sheet is set the text margins for top/bottom must be set to avoid to squash the widget
#ifndef Q_OS_MAC
    lineEdit()->setTextMargins(0, 2, 0, 2);
#else
    // https://forum.freecad.org/viewtopic.php?f=8&t=50615
    lineEdit()->setTextMargins(0, 2, 0, 0);
#endif
}

QuantitySpinBox::~QuantitySpinBox() = default;

void QuantitySpinBox::bind(const App::ObjectIdentifier &_path)
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

        for (const auto & it : list) {
            path << App::ObjectIdentifier::Component::SimpleComponent(it.toLatin1().constData());
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
    updateEdit(getUserString(expr->getQuantity()));
    handlePendingEmit();
}

bool QuantitySpinBox::apply(const std::string & propName)
{
    if (!ExpressionBinding::apply(propName)) {
        double dValue = value().getValue();
        return assignToProperty(propName, dValue);
    }

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

    cursor = qBound(0, cursor, edit->displayText().size() - d->unitStr.size());
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
    const App::ObjectIdentifier & path = getPath();
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
    const App::ObjectIdentifier & path = getPath();
    if (d->validate(tmp, res, path)) {
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
    auto box = new Gui::Dialog::DlgExpressionInput(getPath(), getExpression(), d->unit, this);
    if (d->checkRangeInExpression) {
        box->setRange(d->minimum, d->maximum);
    }
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
    ExpressionSpinBox::setExpression(expr);
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

QSize QuantitySpinBox::sizeForText(const QString& txt) const
{
    const QFontMetrics fm(fontMetrics());
    int h = lineEdit()->sizeHint().height();
    int w = QtTools::horizontalAdvance(fm, txt);

    w += 2; // cursor blinking space
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
    QSize size = style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this);
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

    QSize size = style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this);
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
            auto helpEvent = new QHelpEvent(QEvent::ToolTip, QPoint( 0, rect().height() ), mapToGlobal( QPoint( 0, rect().height() ) ));
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
    QString expr = QString::fromLatin1("^([%1%2]?[0-9\\%3]*)\\%4?([0-9]+(%5[%1%2]?[0-9]+)?)")
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
    QValidator::State state = QValidator::Acceptable;
    const App::ObjectIdentifier & path = getPath();
    Base::Quantity quant = d->validateAndInterpret(copy, state, path);
    if (state != QValidator::Acceptable) {
        fixup(copy);
        quant = d->validateAndInterpret(copy, state, path);
    }

    return quant;
}

QValidator::State QuantitySpinBox::validate(QString &text, int &pos) const
{
    Q_D(const QuantitySpinBox);
    Q_UNUSED(pos)

    QValidator::State state;
    const App::ObjectIdentifier & path = getPath();
    d->validateAndInterpret(text, state, path);
    return state;
}

void QuantitySpinBox::fixup(QString &input) const
{
    input.remove(locale().groupSeparator());
}


#include "moc_QuantitySpinBox.cpp"
#include "moc_QuantitySpinBox_p.cpp"
