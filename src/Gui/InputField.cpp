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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <limits>
# include <QContextMenuEvent>
# include <QMenu>
# include <QPixmapCache>
# include <QRegularExpression>
# include <QRegularExpressionMatch>
#endif

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <App/PropertyUnits.h>
#include <Base/Exception.h>
#include <Base/Quantity.h>

#include "InputField.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "QuantitySpinBox_p.h"


using namespace Gui;
using namespace App;
using namespace Base;

// --------------------------------------------------------------------

namespace Gui {
class InputValidator : public QValidator
{
public:
    explicit InputValidator(InputField* parent);
    ~InputValidator() override;

    void fixup(QString& input) const override;
    State validate(QString& input, int& pos) const override;

private:
    InputField* dptr;
};
}

// --------------------------------------------------------------------

InputField::InputField(QWidget * parent)
  : ExpressionLineEdit(parent),
    ExpressionWidget(),
    validInput(true),
    actUnitValue(0),
    Maximum(std::numeric_limits<double>::max()),
    Minimum(-std::numeric_limits<double>::max()),
    StepSize(1.0),
    HistorySize(5),
    SaveSize(5)
{
    setValidator(new InputValidator(this));
    if (!App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->GetBool("ComboBoxWheelEventFilter",false)) {
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
    // vertical margin, such that `,` won't be clipped to a `.` and similar font descents. Relevant on some OSX versions
    // horizontal margin, such that text will not be behind `fx` icon
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

void InputField::bind(const App::ObjectIdentifier &_path)
{
    ExpressionBinding::bind(_path);

    auto * prop = freecad_cast<PropertyQuantity*>(getPath().getProperty());

    if (prop)
        actQuantity = Base::Quantity(prop->getValue());

    DocumentObject * docObj = getPath().getDocumentObject();

    if (docObj) {
        std::shared_ptr<const Expression> expr(docObj->getExpression(getPath()).expression);

        if (expr)
            newInput(QString::fromStdString(expr->toString()));
    }

    // Create document object, to initialize completer
    setDocumentObject(docObj);
}

bool InputField::apply(const std::string &propName)
{
    if (!ExpressionBinding::apply(propName)) {
        Gui::Command::doCommand(Gui::Command::Doc,"%s = %f", propName.c_str(), getQuantity().getValue());
        return true;
    }
    else
        return false;
}

bool InputField::apply()
{
    return ExpressionBinding::apply();
}

QPixmap InputField::getValidationIcon(const char* name, const QSize& size) const
{
    QString key = QStringLiteral("%1_%2x%3")
        .arg(QString::fromLatin1(name))
        .arg(size.width())
        .arg(size.height());
    QPixmap icon;
    if (QPixmapCache::find(key, &icon))
        return icon;

    icon = BitmapFactory().pixmapFromSvg(name, size);
    if (!icon.isNull())
        QPixmapCache::insert(key, icon);
    return icon;
}

void InputField::updateText(const Base::Quantity& quant)
{
    if (isBound()) {
        std::shared_ptr<const Expression> e(getPath().getDocumentObject()->getExpression(getPath()).expression);

        if (e) {
            setText(QString::fromStdString(e->toString()));
            return;
        }
    }

    double dFactor;
    std::string unitStr;
    std::string txt = quant.getUserString(dFactor, unitStr);
    actUnitValue = quant.getValue()/dFactor;
    setText(QString::fromStdString(txt));
}

void InputField::resizeEvent(QResizeEvent * /*event*/)
{
    QSize iconSize = iconLabel->sizeHint();
    iconLabel->move(width() - (iconSize.width() + 2 * getMargin()), (height() - iconSize.height()) / 2);
}

void InputField::updateIconLabel(const QString& text)
{
    iconLabel->setVisible(text.isEmpty());
}

void InputField::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *editMenu = createStandardContextMenu();
    editMenu->setTitle(tr("Edit"));
    auto menu = new QMenu(QStringLiteral("InputFieldContextmenu"));

    menu->addMenu(editMenu);
    menu->addSeparator();

    // datastructure to remember actions for values
    std::vector<QString> values;
    std::vector<QAction *> actions;

    // add the history menu part...
    std::vector<QString> history = getHistory();

    for(const auto & it : history){
        actions.push_back(menu->addAction(it));
        values.push_back(it);
    }

    // add the save value portion of the menu
    menu->addSeparator();
    QAction *SaveValueAction = menu->addAction(tr("Save Value"));
    std::vector<QString> savedValues = getSavedValues();

    for(const auto & savedValue : savedValues){
        actions.push_back(menu->addAction(savedValue));
        values.push_back(savedValue);
    }

    // call the menu and wait until its back
    QAction *saveAction = menu->exec(event->globalPos());

    // look what the user has chosen
    if(saveAction == SaveValueAction)
        pushToSavedValues();
    else{
        int i=0;
        for(auto it = actions.begin();it!=actions.end();++it,i++)
            if(*it == saveAction)
                this->setText(values[i]);
    }

    delete menu;
}

void InputField::newInput(const QString & text)
{
    Quantity res;
    try {
        QString input = text;
        fixup(input);

        if (isBound()) {
            std::shared_ptr<Expression> e(ExpressionParser::parse(getPath().getDocumentObject(), input.toUtf8()));

            setExpression(e);

            std::unique_ptr<Expression> evalRes(getExpression()->eval());

            auto * value = freecad_cast<NumberExpression*>(evalRes.get());
            if (value) {
                res.setValue(value->getValue());
                res.setUnit(value->getUnit());
            }
        }
        else
            res = Quantity::parse(input.toStdString());
    }
    catch(Base::Exception &e){
        QString errorText = QString::fromLatin1(e.what());
        if (iconLabel->isHidden()) {
            iconLabel->setVisible(true);
        }
        Q_EMIT parseError(errorText);
        validInput = false;
        return;
    }

    if (res.isDimensionless())
        res.setUnit(this->actUnit);

    // check if unit fits!
    if (actUnit != Unit::One && !res.isDimensionless() && actUnit != res.getUnit()){
        if (iconLabel->isHidden()) {
            iconLabel->setVisible(true);
        }
        Q_EMIT parseError(QStringLiteral("Wrong unit"));
        validInput = false;
        return;
    }

    if (iconLabel->isVisible()) {
        iconLabel->setVisible(false);
    }
    validInput = true;

    if (res.getValue() > Maximum){
        res.setValue(Maximum);
    }
    if (res.getValue() < Minimum){
        res.setValue(Minimum);
    }

    double dFactor;
    std::string unitStr;
    res.getUserString(dFactor, unitStr);
    actUnitValue = res.getValue()/dFactor;
    // Preserve previous format
    res.setFormat(this->actQuantity.getFormat());
    actQuantity = res;

    // signaling
    Q_EMIT valueChanged(res);
    Q_EMIT valueChanged(res.getValue());
}

void InputField::pushToHistory(const QString &valueq)
{
    QString val;
    if(valueq.isEmpty())
        val = this->text();
    else
        val = valueq;

    // check if already in:
    std::vector<QString> hist = InputField::getHistory();
    for(const auto & it : hist)
        if( it == val)
            return;

    std::string value(val.toUtf8());
    if(_handle.isValid()){
        char hist1[21];
        char hist0[21];
        for(int i = HistorySize -1 ; i>=0 ;i--){
            snprintf(hist1,20,"Hist%i",i+1);
            snprintf(hist0,20,"Hist%i",i);
            std::string tHist = _handle->GetASCII(hist0,"");
            if(!tHist.empty())
                _handle->SetASCII(hist1,tHist.c_str());
        }
        _handle->SetASCII("Hist0",value.c_str());
    }
}

std::vector<QString> InputField::getHistory()
{
    std::vector<QString> res;

    if(_handle.isValid()){
        std::string tmp;
        char hist[21];
        for(int i = 0 ; i< HistorySize ;i++){
            snprintf(hist,20,"Hist%i",i);
            tmp = _handle->GetASCII(hist,"");
            if( !tmp.empty())
                res.push_back(QString::fromUtf8(tmp.c_str()));
            else
                break; // end of history reached
        }
    }
    return res;
}

void InputField::setToLastUsedValue()
{
    std::vector<QString> hist = getHistory();
    if(!hist.empty())
        this->setText(hist[0]);
}

void InputField::pushToSavedValues(const QString &valueq)
{
    std::string value;
    if(valueq.isEmpty())
        value = this->text().toUtf8().constData();
    else
        value = valueq.toUtf8().constData();

    if(_handle.isValid()){
        char hist1[21];
        char hist0[21];
        for(int i = SaveSize -1 ; i>=0 ;i--){
            snprintf(hist1,20,"Save%i",i+1);
            snprintf(hist0,20,"Save%i",i);
            std::string tHist = _handle->GetASCII(hist0,"");
            if(!tHist.empty())
                _handle->SetASCII(hist1,tHist.c_str());
        }
        _handle->SetASCII("Save0",value.c_str());
    }
}

std::vector<QString> InputField::getSavedValues()
{
    std::vector<QString> res;

    if(_handle.isValid()){
        std::string tmp;
        char hist[21];
        for(int i = 0 ; i< SaveSize ;i++){
            snprintf(hist,20,"Save%i",i);
            tmp = _handle->GetASCII(hist,"");
            if( !tmp.empty())
                res.push_back(QString::fromUtf8(tmp.c_str()));
            else
                break; // end of history reached
        }
    }
    return res;
}

/** Sets the preference path to \a path. */
void InputField::setParamGrpPath( const QByteArray& path )
{
    _handle = App::GetApplication().GetParameterGroupByPath( path);
    if (_handle.isValid())
        sGroupString = (const char*)path;
}

/** Returns the widget's preferences path. */
QByteArray InputField::paramGrpPath() const
{
    if(_handle.isValid())
        return sGroupString.c_str();
    return {};
}

/// sets the field with a quantity
void InputField::setValue(const Base::Quantity& quant)
{
    actQuantity = quant;
    // check limits
    if (actQuantity.getValue() > Maximum)
        actQuantity.setValue(Maximum);
    if (actQuantity.getValue() < Minimum)
        actQuantity.setValue(Minimum);

    actUnit = quant.getUnit();

    updateText(quant);
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
    return QString::fromStdString(actQuantity.getUserString());
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
    double  factor;
    std::string unit;
    double value = actQuantity.getValue();
    actQuantity.getUserString(factor, unit);
    return QStringLiteral("%1 %2").arg(value / factor).arg(QString::fromStdString(unit));
}

/// expects the string in C locale and internally converts it into the OS-specific locale
void InputField::setRawText(const QString& text)
{
    Base::Quantity quant = Base::Quantity::parse(text.toStdString());
    // Input and then format the quantity
    newInput(QString::fromStdString(quant.getUserString()));
    updateText(actQuantity);
}

/// get the value of the singleStep property
double InputField::singleStep()const
{
    return StepSize;
}

/// set the value of the singleStep property
void InputField::setSingleStep(double s)
{
    StepSize = s;
}

/// get the value of the maximum property
double InputField::maximum()const
{
    return Maximum;
}

/// set the value of the maximum property
void InputField::setMaximum(double m)
{
    Maximum = m;
    if (actQuantity.getValue() > Maximum) {
        actQuantity.setValue(Maximum);
        updateText(actQuantity);
    }
}

/// get the value of the minimum property
double InputField::minimum()const
{
    return Minimum;
}

/// set the value of the minimum property
void InputField::setMinimum(double m)
{
    Minimum = m;
    if (actQuantity.getValue() < Minimum) {
        actQuantity.setValue(Minimum);
        updateText(actQuantity);
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
    return this->actQuantity.getFormat().precision;
}

void InputField::setPrecision(const int precision)
{
    Base::QuantityFormat format = actQuantity.getFormat();
    format.precision = precision;
    actQuantity.setFormat(format);
    updateText(actQuantity);
}

QString InputField::getFormat() const
{
    return {QChar::fromLatin1(actQuantity.getFormat().toFormat())};
}

void InputField::setFormat(const QString& format)
{
    if (format.isEmpty())
        return;
    QChar c = format[0];
    Base::QuantityFormat f = this->actQuantity.getFormat();
    f.format = Base::QuantityFormat::toFormat(c.toLatin1());
    actQuantity.setFormat(f);
    updateText(actQuantity);
}

// get the value of the minimum property
int InputField::historySize()const
{
    return HistorySize;
}

// set the value of the minimum property
void InputField::setHistorySize(int i)
{
    assert(i>=0);
    assert(i<100);

    HistorySize = i;
}

void InputField::selectNumber()
{
    QString expr = QStringLiteral("^([%1%2]?[0-9\\%3]*)\\%4?([0-9]+(%5[%1%2]?[0-9]+)?)")
                   .arg(locale().negativeSign())
                   .arg(locale().positiveSign())
                   .arg(locale().groupSeparator())
                   .arg(locale().decimalPoint())
                   .arg(locale().exponential());
    auto rmatch = QRegularExpression(expr).match(text());
    if (rmatch.hasMatch()) {
        setSelection(0, rmatch.capturedLength());
    }
}

void InputField::showEvent(QShowEvent * event)
{
    QLineEdit::showEvent(event);

    bool selected = this->hasSelectedText();
    updateText(actQuantity);
    if (selected)
        selectNumber();
}

void InputField::focusInEvent(QFocusEvent *event)
{
    if (event->reason() == Qt::TabFocusReason ||
        event->reason() == Qt::BacktabFocusReason  ||
        event->reason() == Qt::ShortcutFocusReason) {
        if (!this->hasSelectedText())
            selectNumber();
    }

    QLineEdit::focusInEvent(event);
}

void InputField::focusOutEvent(QFocusEvent *event)
{
    try {
        if (Quantity::parse(this->text().toStdString()).isDimensionless()) {
            // if user didn't enter a unit, we virtually compensate
            // the multiplication factor induced by user unit system
            double factor;
            std::string unitStr;
            actQuantity.getUserString(factor, unitStr);
            actQuantity = actQuantity * factor;
        }
    }
    catch (const Base::ParserError&) {
        // do nothing, let apply the last known good value
    }
    this->setText(QString::fromStdString(actQuantity.getUserString()));
    QLineEdit::focusOutEvent(event);
}

void InputField::keyPressEvent(QKeyEvent *event)
{
    if (isReadOnly()) {
        QLineEdit::keyPressEvent(event);
        return;
    }

    double val = actUnitValue;

    switch (event->key()) {
    case Qt::Key_Up:
        val += StepSize;
        if (val > Maximum)
            val = Maximum;
        break;
    case Qt::Key_Down:
        val -= StepSize;
        if (val < Minimum)
            val = Minimum;
        break;
    default:
        QLineEdit::keyPressEvent(event);
        return;
    }

    double dFactor;
    std::string unitStr;
    actQuantity.getUserString(dFactor, unitStr);
    this->setText(QStringLiteral("%L1 %2").arg(val).arg(QString::fromStdString(unitStr)));
    event->accept();
}

void InputField::wheelEvent (QWheelEvent * event)
{
    if (!hasFocus())
        return;

    if (isReadOnly()) {
        QLineEdit::wheelEvent(event);
        return;
    }

    double factor = event->modifiers() & Qt::ControlModifier ? 10 : 1;
    double step = event->angleDelta().y() > 0 ? StepSize : -StepSize;
    double val = actUnitValue + factor * step;
    if (val > Maximum)
        val = Maximum;
    else if (val < Minimum)
        val = Minimum;

    double dFactor;
    std::string unitStr;
    actQuantity.getUserString(dFactor, unitStr);

    this->setText(QStringLiteral("%L1 %2").arg(val).arg(QString::fromStdString(unitStr)));
    selectNumber();
    event->accept();
}

void InputField::fixup(QString& input) const
{
    input.remove(locale().groupSeparator());

    QString asciiMinus(QStringLiteral("-"));
    QString localeMinus(locale().negativeSign());
    if (localeMinus != asciiMinus) {
        input.replace(localeMinus, asciiMinus);
    }

    QString asciiPlus(QStringLiteral("+"));
    QString localePlus(locale().positiveSign());
    if (localePlus != asciiPlus) {
        input.replace(localePlus, asciiPlus);
    }
}

QValidator::State InputField::validate(QString& input, int& pos) const
{
    Q_UNUSED(pos);
    try {
        Quantity res;
        QString text = input;
        fixup(text);
        res = Quantity::parse(text.toStdString());

        double factor;
        std::string unitStr;
        res.getUserString(factor, unitStr);
        double value = res.getValue()/factor;
        // disallow one to enter numbers out of range
        if (value > this->Maximum || value < this->Minimum)
            return QValidator::Invalid;
    }
    catch(Base::Exception&) {
        // Actually invalid input but the newInput slot gives
        // some feedback
        return QValidator::Intermediate;
    }

    return QValidator::Acceptable;
}

// --------------------------------------------------------------------

InputValidator::InputValidator(InputField* parent) : QValidator(parent), dptr(parent)
{
}

InputValidator::~InputValidator() = default;

void InputValidator::fixup(QString& input) const
{
    dptr->fixup(input);
}

QValidator::State InputValidator::validate(QString& input, int& pos) const
{
    return dptr->validate(input, pos);
}


#include "moc_InputField.cpp"
