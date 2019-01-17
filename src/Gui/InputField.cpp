/***************************************************************************
 *   Copyright (c) 2013 Juergen Riegel                                     *
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
# include <QContextMenuEvent>
# include <QMenu>
# include <QPixmapCache>
#endif

#include <Base/Console.h>
#include <Base/Quantity.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/PropertyUnits.h>
#include <App/DocumentObject.h>
#include "ExpressionCompleter.h"
#include "Command.h"
#include "InputField.h"
#include "BitmapFactory.h"
#include "propertyeditor/PropertyItem.h"

using namespace Gui;
using namespace App;
using namespace Base;

// --------------------------------------------------------------------

namespace Gui {
class InputValidator : public QValidator
{
public:
    InputValidator(InputField* parent);
    ~InputValidator();

    void fixup(QString& input) const;
    State validate(QString& input, int& pos) const;

private:
    InputField* dptr;
};
}

// --------------------------------------------------------------------

InputField::InputField(QWidget * parent)
  : ExpressionLineEdit(parent),
    ExpressionBinding(),
    validInput(true),
    actUnitValue(0),
    Maximum(DOUBLE_MAX),
    Minimum(-DOUBLE_MAX),
    StepSize(1.0),
    HistorySize(5),
    SaveSize(5)
{
    setValidator(new InputValidator(this));
    iconLabel = new QLabel(this);
    iconLabel->setCursor(Qt::ArrowCursor);
    QPixmap pixmap = getValidationIcon(":/icons/button_valid.svg", QSize(sizeHint().height(),sizeHint().height()));
    iconLabel->setPixmap(pixmap);
    iconLabel->setStyleSheet(QString::fromLatin1("QLabel { border: none; padding: 0px; }"));
    iconLabel->hide();
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateIconLabel(const QString&)));
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    setStyleSheet(QString::fromLatin1("QLineEdit { padding-right: %1px } ").arg(iconLabel->sizeHint().width() + frameWidth + 1));
    QSize msz = minimumSizeHint();
    setMinimumSize(qMax(msz.width(), iconLabel->sizeHint().height() + frameWidth * 2 + 2),
                   qMax(msz.height(), iconLabel->sizeHint().height() + frameWidth * 2 + 2));

    this->setContextMenuPolicy(Qt::DefaultContextMenu);

    QObject::connect(this, SIGNAL(textChanged(QString)),
                     this, SLOT(newInput(QString)));
}

InputField::~InputField()
{
}

void InputField::bind(const App::ObjectIdentifier &_path)
{
    ExpressionBinding::bind(_path);

    PropertyQuantity * prop = freecad_dynamic_cast<PropertyQuantity>(getPath().getProperty());

    if (prop)
        actQuantity = Base::Quantity(prop->getValue());

    DocumentObject * docObj = getPath().getDocumentObject();

    if (docObj) {
        boost::shared_ptr<const Expression> expr(docObj->getExpression(getPath()).expression);

        if (expr)
            newInput(Tools::fromStdString(expr->toString()));
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
    QString key = QString::fromLatin1("%1_%2x%3")
        .arg(QString::fromLatin1(name))
        .arg(size.width())
        .arg(size.height());
    QPixmap icon;
    if (QPixmapCache::find(key, icon))
        return icon;

    icon = BitmapFactory().pixmapFromSvg(name, size);
    if (!icon.isNull())
        QPixmapCache::insert(key, icon);
    return icon;
}

void InputField::updateText(const Base::Quantity& quant)
{
    if (isBound()) {
        boost::shared_ptr<const Expression> e(getPath().getDocumentObject()->getExpression(getPath()).expression);

        if (e) {
            setText(Tools::fromStdString(e->toString()));
            return;
        }
    }

    double dFactor;
    QString unitStr;
    QString txt = quant.getUserString(dFactor, unitStr);
    actUnitValue = quant.getValue()/dFactor;
    setText(txt);
}

void InputField::resizeEvent(QResizeEvent *)
{
    QSize sz = iconLabel->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    iconLabel->move(rect().right() - frameWidth - sz.width(),
                    (rect().bottom() + 1 - sz.height())/2);
}

void InputField::updateIconLabel(const QString& text)
{
    iconLabel->setVisible(!text.isEmpty());
}

void InputField::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *editMenu = createStandardContextMenu();
    editMenu->setTitle(tr("Edit"));
    QMenu* menu = new QMenu(QString::fromLatin1("InputFieldContextmenu"));

    menu->addMenu(editMenu);
    menu->addSeparator();

    // datastructure to remember actions for values
    std::vector<QString> values;
    std::vector<QAction *> actions;

    // add the history menu part...
    std::vector<QString> history = getHistory();

    for(std::vector<QString>::const_iterator it = history.begin();it!= history.end();++it){
        actions.push_back(menu->addAction(*it));
        values.push_back(*it);
    }

    // add the save value portion of the menu
    menu->addSeparator();
    QAction *SaveValueAction = menu->addAction(tr("Save value"));
    std::vector<QString> savedValues = getSavedValues();

    for(std::vector<QString>::const_iterator it = savedValues.begin();it!= savedValues.end();++it){
        actions.push_back(menu->addAction(*it));
        values.push_back(*it);
    }

    // call the menu and wait until its back
    QAction *saveAction = menu->exec(event->globalPos());

    // look what the user has chosen
    if(saveAction == SaveValueAction)
        pushToSavedValues();
    else{
        int i=0;
        for(std::vector<QAction *>::const_iterator it = actions.begin();it!=actions.end();++it,i++)
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
            boost::shared_ptr<Expression> e(ExpressionParser::parse(getPath().getDocumentObject(), input.toUtf8()));

            setExpression(e);

            std::unique_ptr<Expression> evalRes(getExpression()->eval());

            NumberExpression * value = freecad_dynamic_cast<NumberExpression>(evalRes.get());
            if (value) {
                res.setValue(value->getValue());
                res.setUnit(value->getUnit());
            }
        }
        else
            res = Quantity::parse(input);
    }
    catch(Base::Exception &e){
        QString errorText = QString::fromLatin1(e.what());
        QPixmap pixmap = getValidationIcon(":/icons/button_invalid.svg", QSize(sizeHint().height(),sizeHint().height()));
        iconLabel->setPixmap(pixmap);
        parseError(errorText);
        validInput = false;
        return;
    }

    if (res.getUnit().isEmpty())
        res.setUnit(this->actUnit);

    // check if unit fits!
    if(!actUnit.isEmpty() && !res.getUnit().isEmpty() && actUnit != res.getUnit()){
        QPixmap pixmap = getValidationIcon(":/icons/button_invalid.svg", QSize(sizeHint().height(),sizeHint().height()));
        iconLabel->setPixmap(pixmap);
        parseError(QString::fromLatin1("Wrong unit"));
        validInput = false;
        return;
    }


    QPixmap pixmap = getValidationIcon(":/icons/button_valid.svg", QSize(sizeHint().height(),sizeHint().height()));
    iconLabel->setPixmap(pixmap);
    validInput = true;

    if (res.getValue() > Maximum){
        res.setValue(Maximum);
    }
    if (res.getValue() < Minimum){
        res.setValue(Minimum);
    }

    double dFactor;
    QString unitStr;
    res.getUserString(dFactor, unitStr);
    actUnitValue = res.getValue()/dFactor;
    // Preserve previous format
    res.setFormat(this->actQuantity.getFormat());
    actQuantity = res;

    // signaling
    valueChanged(res);
    valueChanged(res.getValue());
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
    for(std::vector<QString>::const_iterator it = hist.begin();it!=hist.end();++it)
        if( *it == val)
            return;
    
    std::string value(val.toUtf8());
    if(_handle.isValid()){
        char hist1[21];
        char hist0[21];
        for(int i = HistorySize -1 ; i>=0 ;i--){
            snprintf(hist1,20,"Hist%i",i+1);
            snprintf(hist0,20,"Hist%i",i);
            std::string tHist = _handle->GetASCII(hist0,"");
            if(tHist != "")
                _handle->SetASCII(hist1,tHist.c_str());
        }
        _handle->SetASCII("Hist0",value.c_str());
    }
}

std::vector<QString> InputField::getHistory(void)
{
    std::vector<QString> res;

    if(_handle.isValid()){
        std::string tmp;
        char hist[21];
        for(int i = 0 ; i< HistorySize ;i++){
            snprintf(hist,20,"Hist%i",i);
            tmp = _handle->GetASCII(hist,"");
            if( tmp != "")
                res.push_back(QString::fromUtf8(tmp.c_str()));
            else
                break; // end of history reached
        }
    }
    return res;
}

void InputField::setToLastUsedValue(void)
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
            if(tHist != "")
                _handle->SetASCII(hist1,tHist.c_str());
        }
        _handle->SetASCII("Save0",value.c_str());
    }
}

std::vector<QString> InputField::getSavedValues(void)
{
    std::vector<QString> res;

    if(_handle.isValid()){
        std::string tmp;
        char hist[21];
        for(int i = 0 ; i< SaveSize ;i++){
            snprintf(hist,20,"Save%i",i);
            tmp = _handle->GetASCII(hist,"");
            if( tmp != "")
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
    return QByteArray();
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
QString InputField::getQuantityString(void) const
{
    return actQuantity.getUserString();
}

/// set, validate and display quantity from a string. Must match existing units.
void InputField::setQuantityString(const QString& text)
{
    // Input and then format the quantity
    newInput(text);
    updateText(actQuantity);
}

/// return the quantity in C locale, i.e. decimal separator is a dot.
QString InputField::rawText(void) const
{
    double  factor;
    QString unit;
    double value = actQuantity.getValue();
    actQuantity.getUserString(factor, unit);
    return QString::fromLatin1("%1 %2").arg(value / factor).arg(unit);
}

/// expects the string in C locale and internally converts it into the OS-specific locale
void InputField::setRawText(const QString& text)
{
    Base::Quantity quant = Base::Quantity::parse(text);
    // Input and then format the quantity
    newInput(quant.getUserString());
    updateText(actQuantity);
}

/// get the value of the singleStep property
double InputField::singleStep(void)const
{
    return StepSize;
}

/// set the value of the singleStep property 
void InputField::setSingleStep(double s)
{
    StepSize = s;
}

/// get the value of the maximum property
double InputField::maximum(void)const
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
double InputField::minimum(void)const
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
        Base::Quantity quant = Base::Quantity::parse(str);
        setUnit(quant.getUnit());
    }
    catch (...) {
        // ignore exceptions
    }
}

QString InputField::getUnitText(void)
{
    double dFactor;
    QString unitStr;
    actQuantity.getUserString(dFactor, unitStr);
    return unitStr;
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
    return QString(QChar::fromLatin1(actQuantity.getFormat().toFormat()));
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
int InputField::historySize(void)const
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

void InputField::selectNumber(void)
{
    QString str = text();
    unsigned int i = 0;

    QChar d = locale().decimalPoint();
    QChar g = locale().groupSeparator();
    QChar n = locale().negativeSign();
    QChar e = locale().exponential();

    for (QString::iterator it = str.begin(); it != str.end(); ++it) {
        if (it->isDigit())
            i++;
        else if (*it == d)
            i++;
        else if (*it == g)
            i++;
        else if (*it == n)
            i++;
        else if (*it == e && actQuantity.getFormat().format != Base::QuantityFormat::Fixed)
            i++;
        else // any non-number character
            break;
    }

    setSelection(0, i);
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
    this->setText(actQuantity.getUserString());
    QLineEdit::focusOutEvent(event);
}

void InputField::keyPressEvent(QKeyEvent *event)
{
    if (isReadOnly()) {
        QLineEdit::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Up:
        {
            double val = actUnitValue + StepSize;
            Base::Quantity quant = actQuantity;
            quant.setValue(val);
            this->setText(quant.getUserString());
            event->accept();
        }
        break;
    case Qt::Key_Down:
        {
            double val = actUnitValue - StepSize;
            Base::Quantity quant = actQuantity;
            quant.setValue(val);
            this->setText(quant.getUserString());
            event->accept();
        }
        break;
    default:
        QLineEdit::keyPressEvent(event);
        break;
    }
}

void InputField::wheelEvent (QWheelEvent * event)
{
    if (isReadOnly()) {
        QLineEdit::wheelEvent(event);
        return;
    }

    double step = event->delta() > 0 ? StepSize : -StepSize;
    double val = actUnitValue + step;
    if (val > Maximum)
        val = Maximum;
    else if (val < Minimum)
        val = Minimum;

    Base::Quantity quant = actQuantity;
    quant.setValue(val);
    this->setText(quant.getUserString());
    selectNumber();
    event->accept();
}

void InputField::fixup(QString& input) const
{
    input.remove(locale().groupSeparator());
    if (locale().negativeSign() != QLatin1Char('-'))
        input.replace(locale().negativeSign(), QLatin1Char('-'));
    if (locale().positiveSign() != QLatin1Char('+'))
        input.replace(locale().positiveSign(), QLatin1Char('+'));
}

QValidator::State InputField::validate(QString& input, int& pos) const
{
    Q_UNUSED(pos);
    try {
        Quantity res;
        QString text = input;
        fixup(text);
        res = Quantity::parse(text);

        double factor;
        QString unitStr;
        res.getUserString(factor, unitStr);
        double value = res.getValue()/factor;
        // disallow to enter numbers out of range
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

InputValidator::~InputValidator()
{
}

void InputValidator::fixup(QString& input) const
{
    dptr->fixup(input);
}

QValidator::State InputValidator::validate(QString& input, int& pos) const
{
    return dptr->validate(input, pos);
}


#include "moc_InputField.cpp"
