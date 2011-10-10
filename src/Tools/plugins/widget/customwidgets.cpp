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
#include <QCursor>
#include <QFileDialog>
#include <QMessageBox>
#include <QStyleOptionButton>
#include <QStylePainter>

#include "customwidgets.h"

using namespace Gui;


UrlLabel::UrlLabel ( QWidget * parent, Qt::WFlags f )
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
    QString fn;
    if ( mode() == File )
        fn = QFileDialog::getOpenFileName(this, tr("Select a file"),
        lineEdit->text(), _filter);
    else
        fn = QFileDialog::getExistingDirectory(this, tr("Select a directory"),
        lineEdit->text() );

    if (!fn.isEmpty()) {
        lineEdit->setText(fn);
        emit fileNameSelected(fn);
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
            QKeySequence key(Qt::CTRL+key);
            txt += (QString)(key);
            setText(txt);
        }   break;
    case Qt::AltModifier:
        {
            QKeySequence key(Qt::ALT+key);
            txt += (QString)(key);
            setText(txt);
        }   break;
    case Qt::ShiftModifier:
        {
            QKeySequence key(Qt::SHIFT+key);
            txt += (QString)(key);
            setText(txt);
        }   break;
    case Qt::ControlModifier+Qt::AltModifier:
        {
            QKeySequence key(Qt::CTRL+Qt::ALT+key);
            txt += (QString)(key);
            setText(txt);
        }   break;
    case Qt::ControlModifier+Qt::ShiftModifier:
        {
            QKeySequence key(Qt::CTRL+Qt::SHIFT+key);
            txt += (QString)(key);
            setText(txt);
        }   break;
    case Qt::ShiftModifier+Qt::AltModifier:
        {
            QKeySequence key(Qt::SHIFT+Qt::ALT+key);
            txt += (QString)(key);
            setText(txt);
        }   break;
    case Qt::ControlModifier+Qt::AltModifier+Qt::ShiftModifier:
        {
            QKeySequence key(Qt::CTRL+Qt::ALT+Qt::SHIFT+key);
            txt += (QString)(key);
            setText(txt);
        }   break;
    default:
        {
            QKeySequence key(key);
            txt += (QString)(key);
            setText(txt);
        }   break;
    }
}

// ------------------------------------------------------------------------------

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
            pixmap = qVariantValue<QPixmap>((*it)->data(Qt::UserRole));
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
        emit changed();
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
