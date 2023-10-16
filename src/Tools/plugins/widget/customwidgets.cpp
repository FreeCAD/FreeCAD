/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <werner.wm.mayer@gmx.de>              *
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


#include <QApplication>
#include <QColorDialog>
#include <QCursor>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QStyleOptionButton>
#include <QStylePainter>
#include <QToolTip>
#include <QtGui>
#include <cfloat>

#include "customwidgets.h"

using namespace Gui;


UrlLabel::UrlLabel(QWidget* parent, Qt::WindowFlags f)
    : QLabel("TextLabel", parent, f)
{
    _url = "http://localhost";
    setToolTip(this->_url);
    setCursor(Qt::PointingHandCursor);
}

UrlLabel::~UrlLabel()
{}

void UrlLabel::mouseReleaseEvent(QMouseEvent*)
{
    QMessageBox::information(this,
                             "Browser",
                             QString("This starts your browser with url %1").arg(_url));
}

QString UrlLabel::url() const
{
    return this->_url;
}

void UrlLabel::setUrl(const QString& u)
{
    this->_url = u;
    setToolTip(this->_url);
}

LocationWidget::LocationWidget(QWidget* parent)
    : QWidget(parent)
{
    box = new QGridLayout();

    xValue = new QDoubleSpinBox(this);
    xValue->setMinimum(-2.14748e+09);
    xValue->setMaximum(2.14748e+09);
    xLabel = new QLabel(this);
    box->addWidget(xLabel, 0, 0, 1, 1);
    box->addWidget(xValue, 0, 1, 1, 1);

    yValue = new QDoubleSpinBox(this);
    yValue->setMinimum(-2.14748e+09);
    yValue->setMaximum(2.14748e+09);
    yLabel = new QLabel(this);
    box->addWidget(yLabel, 1, 0, 1, 1);
    box->addWidget(yValue, 1, 1, 1, 1);

    zValue = new QDoubleSpinBox(this);
    zValue->setMinimum(-2.14748e+09);
    zValue->setMaximum(2.14748e+09);
    zLabel = new QLabel(this);
    box->addWidget(zLabel, 2, 0, 1, 1);
    box->addWidget(zValue, 2, 1, 1, 1);

    dLabel = new QLabel(this);
    dValue = new QComboBox(this);
    dValue->setCurrentIndex(-1);
    box->addWidget(dLabel, 3, 0, 1, 1);
    box->addWidget(dValue, 3, 1, 1, 1);

    QGridLayout* gridLayout = new QGridLayout(this);
    gridLayout->addLayout(box, 0, 0, 1, 2);

    retranslateUi();
}

LocationWidget::~LocationWidget()
{}

QSize LocationWidget::sizeHint() const
{
    return QSize(150, 100);
}

void LocationWidget::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        this->retranslateUi();
    }
    QWidget::changeEvent(e);
}

void LocationWidget::retranslateUi()
{
    xLabel->setText(QApplication::translate("Gui::LocationWidget", "X:"));
    yLabel->setText(QApplication::translate("Gui::LocationWidget", "Y:"));
    zLabel->setText(QApplication::translate("Gui::LocationWidget", "Z:"));
    dLabel->setText(QApplication::translate("Gui::LocationWidget", "Direction:"));
}

FileChooser::FileChooser(QWidget* parent)
    : QWidget(parent)
    , md(File)
    , _filter(QString())
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    lineEdit = new QLineEdit(this);
    layout->addWidget(lineEdit);

    connect(lineEdit, &QLineEdit::textChanged, this, &FileChooser::fileNameChanged);

    button = new QPushButton("...", this);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    button->setFixedWidth(2 * button->fontMetrics().horizontalAdvance(" ... "));
#else
    button->setFixedWidth(2 * button->fontMetrics().width(" ... "));
#endif
    layout->addWidget(button);

    connect(button, &QPushButton::clicked, this, &FileChooser::chooseFile);

    setFocusProxy(lineEdit);
}

FileChooser::~FileChooser()
{}

QString FileChooser::fileName() const
{
    return lineEdit->text();
}

void FileChooser::setFileName(const QString& fn)
{
    lineEdit->setText(fn);
}

void FileChooser::chooseFile()
{
    QFileDialog::Options dlgOpt = QFileDialog::DontUseNativeDialog;
    QString fn;
    if (mode() == File) {
        fn = QFileDialog::getOpenFileName(this,
                                          tr("Select a file"),
                                          lineEdit->text(),
                                          _filter,
                                          0,
                                          dlgOpt);
    }
    else {
        QFileDialog::Options option = QFileDialog::ShowDirsOnly | dlgOpt;
        fn = QFileDialog::getExistingDirectory(this,
                                               tr("Select a directory"),
                                               lineEdit->text(),
                                               option);
    }

    if (!fn.isEmpty()) {
        lineEdit->setText(fn);
        Q_EMIT fileNameSelected(fn);
    }
}

FileChooser::Mode FileChooser::mode() const
{
    return md;
}

void FileChooser::setMode(Mode m)
{
    md = m;
}

QString FileChooser::filter() const
{
    return _filter;
}

void FileChooser::setFilter(const QString& filter)
{
    _filter = filter;
}

void FileChooser::setButtonText(const QString& txt)
{
    button->setText(txt);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    int w1 = 2 * button->fontMetrics().horizontalAdvance(txt);
    int w2 = 2 * button->fontMetrics().horizontalAdvance(" ... ");
#else
    int w1 = 2 * button->fontMetrics().width(txt);
    int w2 = 2 * button->fontMetrics().width(" ... ");
#endif
    button->setFixedWidth((w1 > w2 ? w1 : w2));
}

QString FileChooser::buttonText() const
{
    return button->text();
}

// ------------------------------------------------------------------------------

PrefFileChooser::PrefFileChooser(QWidget* parent)
    : FileChooser(parent)
{}

PrefFileChooser::~PrefFileChooser()
{}

QByteArray PrefFileChooser::entryName() const
{
    return m_sPrefName;
}

QByteArray PrefFileChooser::paramGrpPath() const
{
    return m_sPrefGrp;
}

void PrefFileChooser::setEntryName(const QByteArray& name)
{
    m_sPrefName = name;
}

void PrefFileChooser::setParamGrpPath(const QByteArray& name)
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

AccelLineEdit::AccelLineEdit(QWidget* parent)
    : QLineEdit(parent)
{
    setText(tr("none"));
}

void AccelLineEdit::keyPressEvent(QKeyEvent* e)
{
    QString txt;
    setText(tr("none"));

    int key = e->key();
    Qt::KeyboardModifiers state = e->modifiers();

    if (key == Qt::Key_Control) {
        return;
    }
    else if (key == Qt::Key_Shift) {
        return;
    }
    else if (key == Qt::Key_Alt) {
        return;
    }
    else if (state == Qt::NoModifier && key == Qt::Key_Backspace) {
        return;  // clears the edit field
    }

    switch (state) {
        case Qt::ControlModifier: {
            QKeySequence keyseq(Qt::CTRL + key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        } break;
        case Qt::AltModifier: {
            QKeySequence keyseq(Qt::ALT + key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        } break;
        case Qt::ShiftModifier: {
            QKeySequence keyseq(Qt::SHIFT + key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        } break;
        case Qt::ControlModifier + Qt::AltModifier: {
            QKeySequence keyseq(Qt::CTRL + Qt::ALT + key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        } break;
        case Qt::ControlModifier + Qt::ShiftModifier: {
            QKeySequence keyseq(Qt::CTRL + Qt::SHIFT + key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        } break;
        case Qt::ShiftModifier + Qt::AltModifier: {
            QKeySequence keyseq(Qt::SHIFT + Qt::ALT + key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        } break;
        case Qt::ControlModifier + Qt::AltModifier + Qt::ShiftModifier: {
            QKeySequence keyseq(Qt::CTRL + Qt::ALT + Qt::SHIFT + key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        } break;
        default: {
            QKeySequence keyseq(key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        } break;
    }
}

// ------------------------------------------------------------------------------

ActionSelector::ActionSelector(QWidget* parent)
    : QWidget(parent)
{
    addButton = new QPushButton(this);
    addButton->setMinimumSize(QSize(30, 30));
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/icons/button_right.xpm"), QSize(), QIcon::Normal, QIcon::Off);
    addButton->setIcon(icon);
    gridLayout = new QGridLayout(this);
    gridLayout->addWidget(addButton, 1, 1, 1, 1);

    spacerItem = new QSpacerItem(33, 57, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout->addItem(spacerItem, 5, 1, 1, 1);
    spacerItem1 = new QSpacerItem(33, 58, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout->addItem(spacerItem1, 0, 1, 1, 1);

    removeButton = new QPushButton(this);
    removeButton->setMinimumSize(QSize(30, 30));
    QIcon icon1;
    icon1.addFile(QString::fromUtf8(":/icons/button_left.xpm"), QSize(), QIcon::Normal, QIcon::Off);
    removeButton->setIcon(icon1);
    removeButton->setAutoDefault(true);
    removeButton->setDefault(false);

    gridLayout->addWidget(removeButton, 2, 1, 1, 1);

    upButton = new QPushButton(this);
    upButton->setMinimumSize(QSize(30, 30));
    QIcon icon3;
    icon3.addFile(QString::fromUtf8(":/icons/button_up.xpm"), QSize(), QIcon::Normal, QIcon::Off);
    upButton->setIcon(icon3);

    gridLayout->addWidget(upButton, 3, 1, 1, 1);

    downButton = new QPushButton(this);
    downButton->setMinimumSize(QSize(30, 30));
    QIcon icon2;
    icon2.addFile(QString::fromUtf8(":/icons/button_down.xpm"), QSize(), QIcon::Normal, QIcon::Off);
    downButton->setIcon(icon2);
    downButton->setAutoDefault(true);

    gridLayout->addWidget(downButton, 4, 1, 1, 1);

    vboxLayout = new QVBoxLayout();
    vboxLayout->setContentsMargins(0, 0, 0, 0);
    labelAvailable = new QLabel(this);
    vboxLayout->addWidget(labelAvailable);

    availableWidget = new QTreeWidget(this);
    availableWidget->setRootIsDecorated(false);
    availableWidget->setHeaderLabels(QStringList() << QString());
    availableWidget->header()->hide();
    vboxLayout->addWidget(availableWidget);

    gridLayout->addLayout(vboxLayout, 0, 0, 6, 1);

    vboxLayout1 = new QVBoxLayout();
    vboxLayout1->setContentsMargins(0, 0, 0, 0);
    labelSelected = new QLabel(this);
    vboxLayout1->addWidget(labelSelected);

    selectedWidget = new QTreeWidget(this);
    selectedWidget->setRootIsDecorated(false);
    selectedWidget->setHeaderLabels(QStringList() << QString());
    selectedWidget->header()->hide();
    vboxLayout1->addWidget(selectedWidget);

    gridLayout->addLayout(vboxLayout1, 0, 2, 6, 1);

    addButton->setText(QString());
    removeButton->setText(QString());
    upButton->setText(QString());
    downButton->setText(QString());

    labelAvailable->setText(QApplication::translate("Gui::ActionSelector", "Available:"));
    labelSelected->setText(QApplication::translate("Gui::ActionSelector", "Selected:"));
    addButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Add"));
    removeButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Remove"));
    upButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Move up"));
    downButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Move down"));
}

ActionSelector::~ActionSelector()
{}

// --------------------------------------------------------------------

InputField::InputField(QWidget* parent)
    : QLineEdit(parent)
    , Value(0)
    , Maximum(INT_MAX)
    , Minimum(-INT_MAX)
    , StepSize(1.0)
    , HistorySize(5)
{}

InputField::~InputField()
{}

/** Sets the preference path to \a path. */
void InputField::setParamGrpPath(const QByteArray& path)
{
    m_sPrefGrp = path;
}

/** Returns the widget's preferences path. */
QByteArray InputField::paramGrpPath() const
{
    return m_sPrefGrp;
}

/// sets the field with a quantity
void InputField::setValue(double quant)
{
    Value = quant;
    setText(QString("%1 %2").arg(Value).arg(UnitStr));
}

/// sets the field with a quantity
double InputField::getQuantity() const
{
    return Value;
}

/// get the value of the singleStep property
double InputField::singleStep(void) const
{
    return StepSize;
}

/// set the value of the singleStep property
void InputField::setSingleStep(double s)
{
    StepSize = s;
}

/// get the value of the maximum property
double InputField::maximum(void) const
{
    return Maximum;
}

/// set the value of the maximum property
void InputField::setMaximum(double m)
{
    Maximum = m;
}

/// get the value of the minimum property
double InputField::minimum(void) const
{
    return Minimum;
}

/// set the value of the minimum property
void InputField::setMinimum(double m)
{
    Minimum = m;
}

void InputField::setUnitText(QString str)
{
    UnitStr = str;
    setText(QString("%1 %2").arg(Value).arg(UnitStr));
}

QString InputField::getUnitText(void)
{
    return UnitStr;
}

// get the value of the minimum property
int InputField::historySize(void) const
{
    return HistorySize;
}

// set the value of the minimum property
void InputField::setHistorySize(int i)
{
    HistorySize = i;
}

// --------------------------------------------------------------------

namespace Base
{

Unit::Unit()
{}

Unit::Unit(const QString& u)
    : unit(u)
{}

bool Unit::isEmpty() const
{
    return unit.isEmpty();
}

bool Unit::operator==(const Unit& that)
{
    return this->unit == that.unit;
}

bool Unit::operator!=(const Unit& that)
{
    return this->unit != that.unit;
}

const QString& Unit::getString() const
{
    return unit;
}

int QuantityFormat::defaultDenominator = 8;  // for 1/8"


QuantityFormat::QuantityFormat()
    : option(OmitGroupSeparator | RejectGroupSeparator)
    , format(Fixed)
    , precision(4)
    , denominator(defaultDenominator)
{}

Quantity::Quantity()
    : value(0)
    , unit()
{}

Quantity::Quantity(double v, const Unit& u)
    : value(v)
    , unit(u)
{}

Quantity Quantity::parse(const QString& str)
{
    bool ok;
    QString txt = str;
    QString unit;
    while (!txt.isEmpty() && txt[txt.length() - 1].isLetter()) {
        unit.prepend(txt[txt.length() - 1]);
        txt.chop(1);
    }

    double v = QLocale::system().toDouble(txt, &ok);
    // if (!ok && !txt.isEmpty())
    //     throw Base::Exception();
    return Quantity(v, Unit(unit));
}

void Quantity::setValue(double v)
{
    value = v;
}

double Quantity::getValue() const
{
    return value;
}

void Quantity::setUnit(const Unit& u)
{
    unit = u;
}

Unit Quantity::getUnit() const
{
    return unit;
}

QString Quantity::getUserString() const
{
    QLocale Lc;
    const QuantityFormat& format = getFormat();
    if (format.option != QuantityFormat::None) {
        uint opt = static_cast<uint>(format.option);
        Lc.setNumberOptions(static_cast<QLocale::NumberOptions>(opt));
    }

    QString Ln = Lc.toString(value, format.toFormat(), format.precision);
    return QString::fromUtf8("%1 %2").arg(Ln, unit.getString());
}

QString Quantity::getUserString(double& factor, QString& unitString) const
{
    factor = 1;
    unitString = unit.getString();

    QLocale Lc;
    const QuantityFormat& format = getFormat();
    if (format.option != QuantityFormat::None) {
        uint opt = static_cast<uint>(format.option);
        Lc.setNumberOptions(static_cast<QLocale::NumberOptions>(opt));
    }

    QString Ln = Lc.toString(value, format.toFormat(), format.precision);
    return QString::fromUtf8("%1 %2").arg(Ln, unit.getString());
}

}  // namespace Base

namespace Gui
{

class QuantitySpinBoxPrivate
{
public:
    QuantitySpinBoxPrivate()
        : validInput(true)
        , pendingEmit(false)
        , unitValue(0)
        , maximum(INT_MAX)
        , minimum(-INT_MAX)
        , singleStep(1.0)
    {}
    ~QuantitySpinBoxPrivate()
    {}

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

    bool validate(QString& input, Base::Quantity& result) const
    {
        bool success = false;
        QString tmp = input;
        int pos = 0;
        QValidator::State state;
        Base::Quantity res = validateAndInterpret(tmp, pos, state);
        res.setFormat(quantity.getFormat());
        if (state == QValidator::Acceptable) {
            success = true;
            result = res;
            input = tmp;
        }
        else if (state == QValidator::Intermediate) {
            tmp = tmp.trimmed();
            tmp += QLatin1Char(' ');
            tmp += unitStr;
            Base::Quantity res2 = validateAndInterpret(tmp, pos, state);
            res2.setFormat(quantity.getFormat());
            if (state == QValidator::Acceptable) {
                success = true;
                result = res2;
                input = tmp;
            }
        }

        return success;
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
                    if (minus) {
                        state = QValidator::Intermediate;
                    }
                    else {
                        state = QValidator::Invalid;
                    }
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
            default:
                break;
        }

        {
            if (copy.at(0) == locale.groupSeparator()) {
                state = QValidator::Invalid;
                goto end;
            }
            else if (len > 1) {
                bool decOccurred = false;
                for (int i = 0; i < copy.size(); i++) {
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

            bool ok = false;
            double value = min;

            if (locale.negativeSign() != QLatin1Char('-')) {
                copy.replace(locale.negativeSign(), QLatin1Char('-'));
            }
            if (locale.positiveSign() != QLatin1Char('+')) {
                copy.replace(locale.positiveSign(), QLatin1Char('+'));
            }

            try {
                QString copy2 = copy;
                copy2.remove(locale.groupSeparator());

                res = Base::Quantity::parse(copy2);
                value = res.getValue();
                ok = true;
            }
            catch (Base::Exception&) {
            }

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
                    state = QValidator::Invalid;
                }
                else {
                    state = QValidator::Acceptable;
                }
            }
            else if (max == min) {  // when max and min is the same the only non-Invalid input is
                                    // max (or min)
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
};
}  // namespace Gui

QuantitySpinBox::QuantitySpinBox(QWidget* parent)
    : QAbstractSpinBox(parent)
    , d_ptr(new QuantitySpinBoxPrivate())
{
    d_ptr->locale = locale();
    this->setContextMenuPolicy(Qt::DefaultContextMenu);
    connect(lineEdit(), &QLineEdit::textChanged, this, &QuantitySpinBox::userInput);
    connect(this, &QuantitySpinBox::editingFinished, this, &QuantitySpinBox::handlePendingEmit);
}

QuantitySpinBox::~QuantitySpinBox()
{}

void QuantitySpinBox::resizeEvent(QResizeEvent* event)
{
    QAbstractSpinBox::resizeEvent(event);
}

void Gui::QuantitySpinBox::keyPressEvent(QKeyEvent* event)
{
    QAbstractSpinBox::keyPressEvent(event);
}


void QuantitySpinBox::updateText(const Base::Quantity& quant)
{
    Q_D(QuantitySpinBox);

    double dFactor;
    QString txt = getUserString(quant, dFactor, d->unitStr);
    d->unitValue = quant.getValue() / dFactor;
    lineEdit()->setText(txt);
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
    setValue(Base::Quantity(value, d->unit));
}

bool QuantitySpinBox::hasValidInput() const
{
    Q_D(const QuantitySpinBox);
    return d->validInput;
}

// Gets called after call of 'validateAndInterpret'
void QuantitySpinBox::userInput(const QString& text)
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
        handlePendingEmit();
    }
    else {
        d->cached = res;
    }
}

void QuantitySpinBox::handlePendingEmit()
{
    updateFromCache(true);
}

void QuantitySpinBox::updateFromCache(bool notify)
{
    Q_D(QuantitySpinBox);
    if (d->pendingEmit) {
        double factor;
        const Base::Quantity& res = d->cached;
        QString text = getUserString(res, factor, d->unitStr);
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
        Base::Quantity quant = Base::Quantity::parse(str);
        setUnit(quant.getUnit());
    }
    catch (const Base::Exception&) {
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

void QuantitySpinBox::clearSchema()
{
    Q_D(QuantitySpinBox);
    updateText(d->quantity);
}

QString
QuantitySpinBox::getUserString(const Base::Quantity& val, double& factor, QString& unitString) const
{
    return val.getUserString(factor, unitString);
}

QString QuantitySpinBox::getUserString(const Base::Quantity& val) const
{
    return val.getUserString();
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

    lineEdit()->setText(QString::fromUtf8("%L1 %2").arg(val).arg(d->unitStr));
    updateFromCache(true);
    update();
    selectNumber();
}

QSize QuantitySpinBox::sizeHint() const
{
    ensurePolished();

    const QFontMetrics fm(fontMetrics());
    int frameWidth = lineEdit()->style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
    int iconHeight = fm.height() - frameWidth;
    int h = lineEdit()->sizeHint().height();
    int w = 0;

    QString s = QLatin1String("000000000000000000");
    QString fixedContent = QLatin1String(" ");
    s += fixedContent;

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    w = fm.horizontalAdvance(s);
#else
    w = fm.width(s);
#endif

    w += 2;  // cursor blinking space
    w += iconHeight;

    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    QSize hint(w, h);
    QSize size = style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this);
    return size;
}

QSize QuantitySpinBox::minimumSizeHint() const
{
    ensurePolished();

    const QFontMetrics fm(fontMetrics());
    int frameWidth = lineEdit()->style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
    int iconHeight = fm.height() - frameWidth;
    int h = lineEdit()->minimumSizeHint().height();
    int w = 0;

    QString s = QLatin1String("000000000000000000");
    QString fixedContent = QLatin1String(" ");
    s += fixedContent;

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    w = fm.horizontalAdvance(s);
#else
    w = fm.width(s);
#endif

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

void QuantitySpinBox::focusInEvent(QFocusEvent* event)
{
    bool hasSel = lineEdit()->hasSelectedText();
    QAbstractSpinBox::focusInEvent(event);

    if (event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason
        || event->reason() == Qt::ShortcutFocusReason) {

        if (!hasSel) {
            selectNumber();
        }
    }
}

void QuantitySpinBox::focusOutEvent(QFocusEvent* event)
{
    Q_D(QuantitySpinBox);

    int pos = 0;
    QString text = lineEdit()->text();
    QValidator::State state;
    d->validateAndInterpret(text, pos, state);
    if (state != QValidator::Acceptable) {
        lineEdit()->setText(d->validStr);
    }

    handlePendingEmit();

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

Base::Quantity QuantitySpinBox::valueFromText(const QString& text) const
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

QValidator::State QuantitySpinBox::validate(QString& text, int& pos) const
{
    Q_D(const QuantitySpinBox);

    QValidator::State state;
    d->validateAndInterpret(text, pos, state);
    return state;
}

void QuantitySpinBox::fixup(QString& input) const
{
    input.remove(locale().groupSeparator());
}

// ------------------------------------------------------------------------------

PrefUnitSpinBox::PrefUnitSpinBox(QWidget* parent)
    : QuantitySpinBox(parent)
{}

PrefUnitSpinBox::~PrefUnitSpinBox()
{}

QByteArray PrefUnitSpinBox::entryName() const
{
    return m_sPrefName;
}

QByteArray PrefUnitSpinBox::paramGrpPath() const
{
    return m_sPrefGrp;
}

void PrefUnitSpinBox::setEntryName(const QByteArray& name)
{
    m_sPrefName = name;
}

void PrefUnitSpinBox::setParamGrpPath(const QByteArray& name)
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

PrefQuantitySpinBox::PrefQuantitySpinBox(QWidget* parent)
    : QuantitySpinBox(parent)
{}

PrefQuantitySpinBox::~PrefQuantitySpinBox()
{}

QByteArray PrefQuantitySpinBox::entryName() const
{
    return m_sPrefName;
}

QByteArray PrefQuantitySpinBox::paramGrpPath() const
{
    return m_sPrefGrp;
}

void PrefQuantitySpinBox::setEntryName(const QByteArray& name)
{
    m_sPrefName = name;
}

void PrefQuantitySpinBox::setParamGrpPath(const QByteArray& name)
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

CommandIconView::CommandIconView(QWidget* parent)
    : QListWidget(parent)
{
    connect(this, &QListWidget::currentItemChanged, this, &CommandIconView::onSelectionChanged);
}

CommandIconView::~CommandIconView()
{}

void CommandIconView::startDrag(Qt::DropActions /*supportedActions*/)
{
    QList<QListWidgetItem*> items = selectedItems();
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);

    QPixmap pixmap;
    dataStream << items.count();
    for (QList<QListWidgetItem*>::ConstIterator it = items.begin(); it != items.end(); ++it) {
        if (it == items.begin()) {
            pixmap = ((*it)->data(Qt::UserRole)).value<QPixmap>();
        }
        dataStream << (*it)->text();
    }

    QMimeData* mimeData = new QMimeData;
    mimeData->setData("text/x-action-items", itemData);

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
    drag->setPixmap(pixmap);
    drag->exec(Qt::MoveAction);
}

void CommandIconView::onSelectionChanged(QListWidgetItem* item, QListWidgetItem*)
{
    if (item) {
        emitSelectionChanged(item->toolTip());
    }
}

// ------------------------------------------------------------------------------

namespace Gui
{

class UnsignedValidator: public QValidator
{
public:
    UnsignedValidator(QObject* parent);
    UnsignedValidator(uint minimum, uint maximum, QObject* parent);
    ~UnsignedValidator();

    QValidator::State validate(QString&, int&) const;

    void setBottom(uint);
    void setTop(uint);
    virtual void setRange(uint bottom, uint top);

    uint bottom() const
    {
        return b;
    }
    uint top() const
    {
        return t;
    }

private:
    uint b, t;
};

UnsignedValidator::UnsignedValidator(QObject* parent)
    : QValidator(parent)
{
    b = 0;
    t = UINT_MAX;
}

UnsignedValidator::UnsignedValidator(uint minimum, uint maximum, QObject* parent)
    : QValidator(parent)
{
    b = minimum;
    t = maximum;
}

UnsignedValidator::~UnsignedValidator()
{}

QValidator::State UnsignedValidator::validate(QString& input, int&) const
{
    QString stripped;  // = input.stripWhiteSpace();
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

class UIntSpinBoxPrivate
{
public:
    UnsignedValidator* mValidator;

    UIntSpinBoxPrivate()
        : mValidator(0)
    {}
    uint mapToUInt(int v) const
    {
        uint ui;
        if (v == INT_MIN) {
            ui = 0;
        }
        else if (v == INT_MAX) {
            ui = UINT_MAX;
        }
        else if (v < 0) {
            v -= INT_MIN;
            ui = (uint)v;
        }
        else {
            ui = (uint)v;
            ui -= INT_MIN;
        }
        return ui;
    }
    int mapToInt(uint v) const
    {
        int in;
        if (v == UINT_MAX) {
            in = INT_MAX;
        }
        else if (v == 0) {
            in = INT_MIN;
        }
        else if (v > INT_MAX) {
            v += INT_MIN;
            in = (int)v;
        }
        else {
            in = v;
            in += INT_MIN;
        }
        return in;
    }
};

}  // namespace Gui

// -------------------------------------------------------------

UIntSpinBox::UIntSpinBox(QWidget* parent)
    : QSpinBox(parent)
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
    d = 0;
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
    unsignedChanged(d->mapToUInt(value));
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

// --------------------------------------------------------------------

IntSpinBox::IntSpinBox(QWidget* parent)
    : QSpinBox(parent)
{}

IntSpinBox::~IntSpinBox()
{}

// --------------------------------------------------------------------

PrefSpinBox::PrefSpinBox(QWidget* parent)
    : QSpinBox(parent)
{}

PrefSpinBox::~PrefSpinBox()
{}

QByteArray PrefSpinBox::entryName() const
{
    return m_sPrefName;
}

QByteArray PrefSpinBox::paramGrpPath() const
{
    return m_sPrefGrp;
}

void PrefSpinBox::setEntryName(const QByteArray& name)
{
    m_sPrefName = name;
}

void PrefSpinBox::setParamGrpPath(const QByteArray& name)
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

DoubleSpinBox::DoubleSpinBox(QWidget* parent)
    : QDoubleSpinBox(parent)
{}

DoubleSpinBox::~DoubleSpinBox()
{}

// --------------------------------------------------------------------

PrefDoubleSpinBox::PrefDoubleSpinBox(QWidget* parent)
    : QDoubleSpinBox(parent)
{}

PrefDoubleSpinBox::~PrefDoubleSpinBox()
{}

QByteArray PrefDoubleSpinBox::entryName() const
{
    return m_sPrefName;
}

QByteArray PrefDoubleSpinBox::paramGrpPath() const
{
    return m_sPrefGrp;
}

void PrefDoubleSpinBox::setEntryName(const QByteArray& name)
{
    m_sPrefName = name;
}

void PrefDoubleSpinBox::setParamGrpPath(const QByteArray& name)
{
    m_sPrefGrp = name;
}

// -------------------------------------------------------------

ColorButton::ColorButton(QWidget* parent)
    : QPushButton(parent)
    , _allowChange(true)
    , _allowTransparency(false)
    , _drawFrame(true)
{
    _col = palette().color(QPalette::Active, QPalette::Midlight);
    connect(this, &ColorButton::clicked, this, &ColorButton::onChooseColor);
}

ColorButton::~ColorButton()
{}

void ColorButton::setColor(const QColor& c)
{
    _col = c;
    update();
}

QColor ColorButton::color() const
{
    return _col;
}

void ColorButton::setAllowChangeColor(bool ok)
{
    _allowChange = ok;
}

bool ColorButton::allowChangeColor() const
{
    return _allowChange;
}

void ColorButton::setAllowTransparency(bool ok)
{
    _allowTransparency = ok;
}

bool ColorButton::allowTransparency() const
{
    return _allowTransparency;
}

void ColorButton::setDrawFrame(bool ok)
{
    _drawFrame = ok;
}

bool ColorButton::drawFrame() const
{
    return _drawFrame;
}

void ColorButton::paintEvent(QPaintEvent* e)
{
    // first paint the complete button
    QPushButton::paintEvent(e);

    // repaint the rectangle area
    QPalette::ColorGroup group =
        isEnabled() ? hasFocus() ? QPalette::Active : QPalette::Inactive : QPalette::Disabled;
    QColor pen = palette().color(group, QPalette::ButtonText);
    {
        QPainter paint(this);
        paint.setPen(pen);

        if (_drawFrame) {
            paint.setBrush(QBrush(_col));
            paint.drawRect(5, 5, width() - 10, height() - 10);
        }
        else {
            paint.fillRect(5, 5, width() - 10, height() - 10, QBrush(_col));
        }
    }

    // overpaint the rectangle to paint icon and text
    QStyleOptionButton opt;
    opt.init(this);
    opt.text = text();
    opt.icon = icon();
    opt.iconSize = iconSize();

    QStylePainter p(this);
    p.drawControl(QStyle::CE_PushButtonLabel, opt);
}

void ColorButton::onChooseColor()
{
    if (!_allowChange) {
        return;
    }
    QColor c = QColorDialog::getColor(_col, this);
    if (c.isValid()) {
        setColor(c);
        Q_EMIT changed();
    }
}

// ------------------------------------------------------------------------------

PrefColorButton::PrefColorButton(QWidget* parent)
    : ColorButton(parent)
{}

PrefColorButton::~PrefColorButton()
{}

QByteArray PrefColorButton::entryName() const
{
    return m_sPrefName;
}

QByteArray PrefColorButton::paramGrpPath() const
{
    return m_sPrefGrp;
}

void PrefColorButton::setEntryName(const QByteArray& name)
{
    m_sPrefName = name;
}

void PrefColorButton::setParamGrpPath(const QByteArray& name)
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

PrefLineEdit::PrefLineEdit(QWidget* parent)
    : QLineEdit(parent)
{}

PrefLineEdit::~PrefLineEdit()
{}

QByteArray PrefLineEdit::entryName() const
{
    return m_sPrefName;
}

QByteArray PrefLineEdit::paramGrpPath() const
{
    return m_sPrefGrp;
}

void PrefLineEdit::setEntryName(const QByteArray& name)
{
    m_sPrefName = name;
}

void PrefLineEdit::setParamGrpPath(const QByteArray& name)
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

PrefComboBox::PrefComboBox(QWidget* parent)
    : QComboBox(parent)
{
    setEditable(false);
}

PrefComboBox::~PrefComboBox()
{}

QByteArray PrefComboBox::entryName() const
{
    return m_sPrefName;
}

QByteArray PrefComboBox::paramGrpPath() const
{
    return m_sPrefGrp;
}

void PrefComboBox::setEntryName(const QByteArray& name)
{
    m_sPrefName = name;
}

void PrefComboBox::setParamGrpPath(const QByteArray& name)
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

PrefCheckBox::PrefCheckBox(QWidget* parent)
    : QCheckBox(parent)
{
    setText("CheckBox");
}

PrefCheckBox::~PrefCheckBox()
{}

QByteArray PrefCheckBox::entryName() const
{
    return m_sPrefName;
}

QByteArray PrefCheckBox::paramGrpPath() const
{
    return m_sPrefGrp;
}

void PrefCheckBox::setEntryName(const QByteArray& name)
{
    m_sPrefName = name;
}

void PrefCheckBox::setParamGrpPath(const QByteArray& name)
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

PrefRadioButton::PrefRadioButton(QWidget* parent)
    : QRadioButton(parent)
{
    setText("RadioButton");
}

PrefRadioButton::~PrefRadioButton()
{}

QByteArray PrefRadioButton::entryName() const
{
    return m_sPrefName;
}

QByteArray PrefRadioButton::paramGrpPath() const
{
    return m_sPrefGrp;
}

void PrefRadioButton::setEntryName(const QByteArray& name)
{
    m_sPrefName = name;
}

void PrefRadioButton::setParamGrpPath(const QByteArray& name)
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

PrefSlider::PrefSlider(QWidget* parent)
    : QSlider(parent)
{}

PrefSlider::~PrefSlider()
{}

QByteArray PrefSlider::entryName() const
{
    return m_sPrefName;
}

QByteArray PrefSlider::paramGrpPath() const
{
    return m_sPrefGrp;
}

void PrefSlider::setEntryName(const QByteArray& name)
{
    m_sPrefName = name;
}

void PrefSlider::setParamGrpPath(const QByteArray& name)
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

PrefFontBox::PrefFontBox(QWidget* parent)
    : QFontComboBox(parent)
{}

PrefFontBox::~PrefFontBox()
{}

QByteArray PrefFontBox::entryName() const
{
    return m_sPrefName;
}

QByteArray PrefFontBox::paramGrpPath() const
{
    return m_sPrefGrp;
}

void PrefFontBox::setEntryName(const QByteArray& name)
{
    m_sPrefName = name;
}

void PrefFontBox::setParamGrpPath(const QByteArray& name)
{
    m_sPrefGrp = name;
}
