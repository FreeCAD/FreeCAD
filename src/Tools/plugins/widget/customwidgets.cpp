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


#include <QtGui>
#include <QApplication>
#include <QColorDialog>
#include <QCursor>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QStyleOptionButton>
#include <QStylePainter>

#include "customwidgets.h"

using namespace Gui;


UrlLabel::UrlLabel ( QWidget * parent, Qt::WindowFlags f )
  : QLabel("TextLabel", parent, f)
{
    _url = "http://localhost";
    setToolTip(this->_url);
}

UrlLabel::~UrlLabel()
{
}

void UrlLabel::enterEvent ( QEvent * )
{
    setCursor(Qt::PointingHandCursor);
}

void UrlLabel::leaveEvent ( QEvent * )
{
    setCursor(Qt::ArrowCursor);
}

void UrlLabel::mouseReleaseEvent ( QMouseEvent * )
{
    QMessageBox::information(this, "Browser", 
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

LocationWidget::LocationWidget (QWidget * parent)
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
{
}

QSize LocationWidget::sizeHint() const
{
    return QSize(150,100);
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

FileChooser::FileChooser( QWidget *parent )
  : QWidget( parent ), md( File ), _filter( QString::null )
{
    QHBoxLayout *layout = new QHBoxLayout( this );
    layout->setMargin( 0 );
    layout->setSpacing( 6 );

    lineEdit = new QLineEdit( this );
    layout->addWidget( lineEdit );

    connect(lineEdit, SIGNAL(textChanged(const QString &)),
            this, SIGNAL(fileNameChanged(const QString &)));

    button = new QPushButton( "...", this );
    button->setFixedWidth(2*button->fontMetrics().width( " ... " ));
    layout->addWidget( button );

    connect(button, SIGNAL(clicked()), this, SLOT(chooseFile()));

    setFocusProxy( lineEdit );
}

FileChooser::~FileChooser()
{
}

QString FileChooser::fileName() const
{
    return lineEdit->text();
}

void FileChooser::setFileName( const QString &fn )
{
    lineEdit->setText( fn );
}

void FileChooser::chooseFile()
{
    QFileDialog::Options dlgOpt = QFileDialog::DontUseNativeDialog;
    QString fn;
    if ( mode() == File ) {
        fn = QFileDialog::getOpenFileName(this, tr("Select a file"),
        lineEdit->text(), _filter,0,dlgOpt);
    } else {
        QFileDialog::Options option = QFileDialog::ShowDirsOnly | dlgOpt;
        fn = QFileDialog::getExistingDirectory( this, tr( "Select a directory" ), lineEdit->text(),option );
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

void FileChooser::setMode( Mode m )
{
    md = m;
}

QString FileChooser::filter() const
{
    return _filter;
}

void FileChooser::setFilter ( const QString& filter )
{
    _filter = filter;
}

void FileChooser::setButtonText( const QString& txt )
{
    button->setText( txt );
    int w1 = 2*button->fontMetrics().width(txt);
    int w2 = 2*button->fontMetrics().width(" ... ");
    button->setFixedWidth((w1 > w2 ? w1 : w2));
}

QString FileChooser::buttonText() const
{
    return button->text();
}

// ------------------------------------------------------------------------------

PrefFileChooser::PrefFileChooser ( QWidget * parent )
  : FileChooser(parent)
{
}

PrefFileChooser::~PrefFileChooser()
{
}

QByteArray PrefFileChooser::entryName () const
{
    return m_sPrefName;
}

QByteArray PrefFileChooser::paramGrpPath () const
{
    return m_sPrefGrp;
}

void PrefFileChooser::setEntryName ( const QByteArray& name )
{
    m_sPrefName = name;
}

void PrefFileChooser::setParamGrpPath ( const QByteArray& name )
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

AccelLineEdit::AccelLineEdit ( QWidget * parent )
  : QLineEdit(parent)
{
    setText(tr("none"));
}

void AccelLineEdit::keyPressEvent ( QKeyEvent * e)
{
    QString txt;
    setText(tr("none"));

    int key = e->key();
    Qt::KeyboardModifiers state = e->modifiers();

    if ( key == Qt::Key_Control )
        return;
    else if ( key == Qt::Key_Shift )
        return;
    else if ( key == Qt::Key_Alt )
        return;
    else if ( state == Qt::NoModifier && key == Qt::Key_Backspace )
        return; // clears the edit field

    switch( state )
    {
    case Qt::ControlModifier:
        {
            QKeySequence keyseq(Qt::CTRL+key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        }   break;
    case Qt::AltModifier:
        {
            QKeySequence keyseq(Qt::ALT+key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        }   break;
    case Qt::ShiftModifier:
        {
            QKeySequence keyseq(Qt::SHIFT+key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        }   break;
    case Qt::ControlModifier+Qt::AltModifier:
        {
            QKeySequence keyseq(Qt::CTRL+Qt::ALT+key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        }   break;
    case Qt::ControlModifier+Qt::ShiftModifier:
        {
            QKeySequence keyseq(Qt::CTRL+Qt::SHIFT+key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        }   break;
    case Qt::ShiftModifier+Qt::AltModifier:
        {
            QKeySequence keyseq(Qt::SHIFT+Qt::ALT+key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        }   break;
    case Qt::ControlModifier+Qt::AltModifier+Qt::ShiftModifier:
        {
            QKeySequence keyseq(Qt::CTRL+Qt::ALT+Qt::SHIFT+key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        }   break;
    default:
        {
            QKeySequence keyseq(key);
            txt += keyseq.toString(QKeySequence::NativeText);
            setText(txt);
        }   break;
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
{
}

// --------------------------------------------------------------------

InputField::InputField (QWidget * parent)
  : QLineEdit(parent),
    Value(0),
    Maximum(INT_MAX),
    Minimum(-INT_MAX),
    StepSize(1.0),
    HistorySize(5)
{
}

InputField::~InputField()
{
}

/** Sets the preference path to \a path. */
void InputField::setParamGrpPath( const QByteArray& path )
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
int InputField::historySize(void)const
{
    return HistorySize;
}

// set the value of the minimum property 
void InputField::setHistorySize(int i)
{
    HistorySize = i;
}

// --------------------------------------------------------------------

QuantitySpinBox::QuantitySpinBox (QWidget * parent)
  : QAbstractSpinBox(parent),
    Value(0),
    Maximum(INT_MAX),
    Minimum(-INT_MAX),
    StepSize(1.0)
{
}

QuantitySpinBox::~QuantitySpinBox()
{
}

/// sets the field with a quantity
void QuantitySpinBox::setValue(double quant)
{
    Value = quant;
    lineEdit()->setText(QString("%1 %2").arg(Value).arg(UnitStr));
}

/// sets the field with a quantity
double QuantitySpinBox::value() const
{
    return Value;
}

/// get the value of the singleStep property
double QuantitySpinBox::singleStep(void)const
{
    return StepSize;
}

/// set the value of the singleStep property 
void QuantitySpinBox::setSingleStep(double s)
{
    StepSize = s;
}

/// get the value of the maximum property
double QuantitySpinBox::maximum(void)const
{
    return Maximum;
}

/// set the value of the maximum property 
void QuantitySpinBox::setMaximum(double m)
{
    Maximum = m;
}

/// get the value of the minimum property
double QuantitySpinBox::minimum(void)const
{
    return Minimum;
}

/// set the value of the minimum property 
void QuantitySpinBox::setMinimum(double m)
{
    Minimum = m;
}

QAbstractSpinBox::StepEnabled QuantitySpinBox::stepEnabled() const
{
    if (isReadOnly())
        return StepNone;
    if (wrapping())
        return StepEnabled(StepUpEnabled | StepDownEnabled);
    StepEnabled ret = StepNone;
    if (Value < Maximum) {
        ret |= StepUpEnabled;
    }
    if (Value > Minimum) {
        ret |= StepDownEnabled;
    }
    return ret;
}

void QuantitySpinBox::stepBy(int steps)
{
    double step = StepSize * steps;
    double val = Value + step;
    if (val > Maximum)
        val = Maximum;
    else if (val < Minimum)
        val = Minimum;

    lineEdit()->setText(QString::fromUtf8("%L1 %2").arg(val).arg(UnitStr));
    update();
}

void QuantitySpinBox::setUnitText(QString str)
{
    UnitStr = str;
    lineEdit()->setText(QString("%1 %2").arg(Value).arg(UnitStr));
}

QString QuantitySpinBox::unitText(void)
{
    return UnitStr;
}

// ------------------------------------------------------------------------------

PrefUnitSpinBox::PrefUnitSpinBox ( QWidget * parent )
  : QuantitySpinBox(parent)
{
}

PrefUnitSpinBox::~PrefUnitSpinBox()
{
}

QByteArray PrefUnitSpinBox::entryName () const
{
    return m_sPrefName;
}

QByteArray PrefUnitSpinBox::paramGrpPath () const
{
    return m_sPrefGrp;
}

void PrefUnitSpinBox::setEntryName ( const QByteArray& name )
{
    m_sPrefName = name;
}

void PrefUnitSpinBox::setParamGrpPath ( const QByteArray& name )
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

CommandIconView::CommandIconView ( QWidget * parent )
  : QListWidget(parent)
{
    connect(this, SIGNAL (currentItemChanged(QListWidgetItem *, QListWidgetItem *)), 
            this, SLOT (onSelectionChanged(QListWidgetItem *, QListWidgetItem *)) );
}

CommandIconView::~CommandIconView ()
{
}

void CommandIconView::startDrag ( Qt::DropActions /*supportedActions*/ )
{
    QList<QListWidgetItem*> items = selectedItems();
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);

    QPixmap pixmap;
    dataStream << items.count();
    for (QList<QListWidgetItem*>::ConstIterator it = items.begin(); it != items.end(); ++it) {
        if (it == items.begin())
            pixmap = ((*it)->data(Qt::UserRole)).value<QPixmap>();
        dataStream << (*it)->text();
    }

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("text/x-action-items", itemData);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));
    drag->setPixmap(pixmap);
    drag->start(Qt::MoveAction);
}

void CommandIconView::onSelectionChanged(QListWidgetItem * item, QListWidgetItem *)
{
    if (item)
        emitSelectionChanged(item->toolTip());
}

// ------------------------------------------------------------------------------

namespace Gui {

class UnsignedValidator : public QValidator
{
public:
    UnsignedValidator( QObject * parent );
    UnsignedValidator( uint minimum, uint maximum, QObject * parent );
    ~UnsignedValidator();

    QValidator::State validate( QString &, int & ) const;

    void setBottom( uint );
    void setTop( uint );
    virtual void setRange( uint bottom, uint top );

    uint bottom() const { return b; }
    uint top() const { return t; }

private:
    uint b, t;
};

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
    QString stripped;// = input.stripWhiteSpace();
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
        }
        else if ( v == INT_MAX ) {
            ui = UINT_MAX;
        }
        else if ( v < 0 ) {
            v -= INT_MIN; ui = (uint)v;
        }
        else {
            ui = (uint)v; ui -= INT_MIN;
        }
        return ui;
    }
    int mapToInt( uint v ) const
    {
        int in;
        if ( v == UINT_MAX ) {
            in = INT_MAX;
        }
        else if ( v == 0 ) {
            in = INT_MIN;
        }
        else if ( v > INT_MAX ) {
            v += INT_MIN; in = (int)v;
        }
        else {
            in = v; in += INT_MIN;
        }
        return in;
    }
};

} // namespace Gui

// -------------------------------------------------------------

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

// --------------------------------------------------------------------

PrefSpinBox::PrefSpinBox ( QWidget * parent )
  : QSpinBox(parent)
{
}

PrefSpinBox::~PrefSpinBox()
{
}

QByteArray PrefSpinBox::entryName () const
{
    return m_sPrefName;
}

QByteArray PrefSpinBox::paramGrpPath () const
{
    return m_sPrefGrp;
}

void PrefSpinBox::setEntryName ( const QByteArray& name )
{
    m_sPrefName = name;
}

void PrefSpinBox::setParamGrpPath ( const QByteArray& name )
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

PrefDoubleSpinBox::PrefDoubleSpinBox ( QWidget * parent )
  : QDoubleSpinBox(parent)
{
}

PrefDoubleSpinBox::~PrefDoubleSpinBox()
{
}

QByteArray PrefDoubleSpinBox::entryName () const
{
    return m_sPrefName;
}

QByteArray PrefDoubleSpinBox::paramGrpPath () const
{
    return m_sPrefGrp;
}

void PrefDoubleSpinBox::setEntryName ( const QByteArray& name )
{
    m_sPrefName = name;
}

void PrefDoubleSpinBox::setParamGrpPath ( const QByteArray& name )
{
    m_sPrefGrp = name;
}

// -------------------------------------------------------------

ColorButton::ColorButton(QWidget* parent)
    : QPushButton( parent ), _allowChange(true), _drawFrame(true)
{
    _col = palette().color(QPalette::Active,QPalette::Midlight);
    connect( this, SIGNAL( clicked() ), SLOT( onChooseColor() ));
}

ColorButton::~ColorButton()
{
}

void ColorButton::setColor( const QColor& c )
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

void ColorButton::setDrawFrame(bool ok)
{
    _drawFrame = ok;
}

bool ColorButton::drawFrame() const
{
    return _drawFrame;
}

void ColorButton::paintEvent ( QPaintEvent * e )
{
    // first paint the complete button
    QPushButton::paintEvent(e);

    // repaint the rectangle area
    QPalette::ColorGroup group = isEnabled() ? hasFocus() ? QPalette::Active : QPalette::Inactive : QPalette::Disabled;
    QColor pen = palette().color(group,QPalette::ButtonText);
    {
        QPainter paint(this);
        paint.setPen( pen );

        if (_drawFrame) {
            paint.setBrush(QBrush(_col));
            paint.drawRect(5, 5, width()-10, height()-10);
        } else {
            paint.fillRect(5, 5, width()-10, height()-10, QBrush(_col));
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
    if (!_allowChange)
        return;
    QColor c = QColorDialog::getColor( _col, this );
    if ( c.isValid() )
    {
        setColor( c );
        Q_EMIT changed();
    }
}

// ------------------------------------------------------------------------------

PrefColorButton::PrefColorButton ( QWidget * parent )
  : ColorButton(parent)
{
}

PrefColorButton::~PrefColorButton()
{
}

QByteArray PrefColorButton::entryName () const
{
    return m_sPrefName;
}

QByteArray PrefColorButton::paramGrpPath () const
{
    return m_sPrefGrp;
}

void PrefColorButton::setEntryName ( const QByteArray& name )
{
    m_sPrefName = name;
}

void PrefColorButton::setParamGrpPath ( const QByteArray& name )
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

PrefLineEdit::PrefLineEdit ( QWidget * parent )
  : QLineEdit(parent)
{
}

PrefLineEdit::~PrefLineEdit()
{
}

QByteArray PrefLineEdit::entryName () const
{
    return m_sPrefName;
}

QByteArray PrefLineEdit::paramGrpPath () const
{
    return m_sPrefGrp;
}

void PrefLineEdit::setEntryName ( const QByteArray& name )
{
    m_sPrefName = name;
}

void PrefLineEdit::setParamGrpPath ( const QByteArray& name )
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

PrefComboBox::PrefComboBox ( QWidget * parent )
  : QComboBox(parent)
{
    setEditable(false);
}

PrefComboBox::~PrefComboBox()
{
}

QByteArray PrefComboBox::entryName () const
{
    return m_sPrefName;
}

QByteArray PrefComboBox::paramGrpPath () const
{
    return m_sPrefGrp;
}

void PrefComboBox::setEntryName ( const QByteArray& name )
{
    m_sPrefName = name;
}

void PrefComboBox::setParamGrpPath ( const QByteArray& name )
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

PrefCheckBox::PrefCheckBox ( QWidget * parent )
  : QCheckBox(parent)
{
  setText("CheckBox");
}

PrefCheckBox::~PrefCheckBox()
{
}

QByteArray PrefCheckBox::entryName () const
{
    return m_sPrefName;
}

QByteArray PrefCheckBox::paramGrpPath () const
{
    return m_sPrefGrp;
}

void PrefCheckBox::setEntryName ( const QByteArray& name )
{
    m_sPrefName = name;
}

void PrefCheckBox::setParamGrpPath ( const QByteArray& name )
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

PrefRadioButton::PrefRadioButton ( QWidget * parent )
  : QRadioButton(parent)
{
    setText("RadioButton");
}

PrefRadioButton::~PrefRadioButton()
{
}

QByteArray PrefRadioButton::entryName () const
{
    return m_sPrefName;
}

QByteArray PrefRadioButton::paramGrpPath () const
{
    return m_sPrefGrp;
}

void PrefRadioButton::setEntryName ( const QByteArray& name )
{
    m_sPrefName = name;
}

void PrefRadioButton::setParamGrpPath ( const QByteArray& name )
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

PrefSlider::PrefSlider ( QWidget * parent )
  : QSlider(parent)
{
}

PrefSlider::~PrefSlider()
{
}

QByteArray PrefSlider::entryName () const
{
    return m_sPrefName;
}

QByteArray PrefSlider::paramGrpPath () const
{
    return m_sPrefGrp;
}

void PrefSlider::setEntryName ( const QByteArray& name )
{
    m_sPrefName = name;
}

void PrefSlider::setParamGrpPath ( const QByteArray& name )
{
    m_sPrefGrp = name;
}

// --------------------------------------------------------------------

PrefFontBox::PrefFontBox ( QWidget * parent )
  : QFontComboBox(parent)
{
}

PrefFontBox::~PrefFontBox()
{
}

QByteArray PrefFontBox::entryName () const
{
    return m_sPrefName;
}

QByteArray PrefFontBox::paramGrpPath () const
{
    return m_sPrefGrp;
}

void PrefFontBox::setEntryName ( const QByteArray& name )
{
    m_sPrefName = name;
}

void PrefFontBox::setParamGrpPath ( const QByteArray& name )
{
    m_sPrefGrp = name;
}

