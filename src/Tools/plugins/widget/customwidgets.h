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


#ifndef GUI_CUSTOMWIDGETS_H
#define GUI_CUSTOMWIDGETS_H

#include <QButtonGroup>
#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>

namespace Gui
{

class UrlLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY( QString  url    READ url   WRITE setUrl)

public:
    UrlLabel ( QWidget * parent = 0, Qt::WFlags f = 0 );
    virtual ~UrlLabel();

    QString url() const;

public Q_SLOTS:
    void setUrl( const QString &u );

protected:
    void enterEvent ( QEvent * );
    void leaveEvent ( QEvent * );
    void mouseReleaseEvent ( QMouseEvent * );

private:
    QString _url;
};

/**
 * There is a bug in QtDesigner of Qt version 4.0, 4.1 and 4.2. If a class declaration
 * is inside a namespace and it uses the Q_ENUMS macro then QtDesigner doesn't handle
 * the enum(s) correctly in its property editor. This bug is fixed since Qt 4.3.0.
 */
class FileChooser : public QWidget
{
    Q_OBJECT

    Q_ENUMS( Mode )
    Q_PROPERTY( Mode mode READ mode WRITE setMode )
    Q_PROPERTY( QString  fileName  READ fileName      WRITE setFileName      )
    Q_PROPERTY( QString  filter    READ filter        WRITE setFilter        )
    Q_PROPERTY( QString  buttonText  READ buttonText  WRITE setButtonText    )

public:
    enum Mode { File, Directory };

    FileChooser (QWidget *parent = 0);
    virtual ~FileChooser();


    QString filter() const;
    QString fileName() const;
    Mode mode() const;
    QString buttonText() const;

public Q_SLOTS:
    void setFileName( const QString &fn );
    void setMode( Mode m );
    void setFilter ( const QString & );
    void setButtonText ( const QString & );

Q_SIGNALS:
    void fileNameChanged( const QString & );
    void fileNameSelected( const QString & );

private Q_SLOTS:
    void chooseFile();

private:
    QLineEdit *lineEdit;
    QPushButton *button;
    Mode md;
    QString _filter;
};

// ------------------------------------------------------------------------------

class PrefFileChooser : public FileChooser
{
    Q_OBJECT

    Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
    Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
    PrefFileChooser ( QWidget * parent = 0 );
    virtual ~PrefFileChooser();

    QByteArray entryName    () const;
    QByteArray paramGrpPath () const;
    void  setEntryName     ( const QByteArray& name );
    void  setParamGrpPath  ( const QByteArray& name );

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class AccelLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    AccelLineEdit ( QWidget * parent=0 );

protected:
    void keyPressEvent ( QKeyEvent * e);
};

// ------------------------------------------------------------------------------

class CommandIconView : public QListWidget
{
    Q_OBJECT

public:
    CommandIconView ( QWidget * parent = 0 );
    virtual ~CommandIconView ();

protected:
    void startDrag ( Qt::DropActions supportedActions );

protected Q_SLOTS:
    void onSelectionChanged( QListWidgetItem * item, QListWidgetItem * );

Q_SIGNALS:
    void emitSelectionChanged( const QString& );
};

// -------------------------------------------------------------

class UIntSpinBoxPrivate;
class UIntSpinBox : public QSpinBox
{
    Q_OBJECT
    Q_OVERRIDE( uint maximum READ maximum WRITE setMaximum )
    Q_OVERRIDE( uint minimum READ minimum WRITE setMinimum )
    Q_OVERRIDE( uint value READ value WRITE setValue )

public:
    UIntSpinBox ( QWidget* parent );
    virtual ~UIntSpinBox();

    void setRange( uint minVal, uint maxVal );
    uint value() const;
    virtual QValidator::State validate ( QString & input, int & pos ) const;
    uint minimum() const;
    void setMinimum( uint value );
    uint maximum() const;
    void setMaximum( uint value );

Q_SIGNALS:
    void valueChanged( uint value );

public Q_SLOTS:
    void setValue( uint value );

private Q_SLOTS:
    void valueChange( int value );

protected:
    virtual QString textFromValue ( int v ) const;
    virtual int valueFromText ( const QString & text ) const;

private:
    void updateValidator();
    UIntSpinBoxPrivate * d;
};

// -------------------------------------------------------------

class PrefSpinBox : public QSpinBox
{
    Q_OBJECT

    Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
    Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
    PrefSpinBox ( QWidget * parent = 0 );
    virtual ~PrefSpinBox();

    QByteArray entryName    () const;
    QByteArray paramGrpPath () const;
    void  setEntryName     ( const QByteArray& name );
    void  setParamGrpPath  ( const QByteArray& name );

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class ColorButton : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY( QColor color READ color WRITE setColor )
    Q_PROPERTY( bool allowChangeColor READ allowChangeColor WRITE setAllowChangeColor )
    Q_PROPERTY( bool drawFrame READ drawFrame WRITE setDrawFrame )

public:
    ColorButton( QWidget* parent = 0 );
    ~ColorButton();

    void setColor( const QColor& );
    QColor color() const;

    void setAllowChangeColor(bool);
    bool allowChangeColor() const;

    void setDrawFrame(bool);
    bool drawFrame() const;

public Q_SLOTS:
    virtual void onChooseColor();

Q_SIGNALS:
    void changed();

protected:
    void paintEvent ( QPaintEvent* );

private:
    QColor _col;
    bool _allowChange;
    bool _drawFrame;
};

// ------------------------------------------------------------------------------

class PrefColorButton : public ColorButton
{
    Q_OBJECT

    Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
    Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
    PrefColorButton ( QWidget * parent = 0 );
    virtual ~PrefColorButton();

    QByteArray entryName    () const;
    QByteArray paramGrpPath () const;
    void  setEntryName     ( const QByteArray& name );
    void  setParamGrpPath  ( const QByteArray& name );

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class PrefDoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT

    Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
    Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
    PrefDoubleSpinBox ( QWidget * parent = 0 );
    virtual ~PrefDoubleSpinBox();

    QByteArray entryName    () const;
    QByteArray paramGrpPath () const;
    void  setEntryName     ( const QByteArray& name );
    void  setParamGrpPath  ( const QByteArray& name );

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class PrefLineEdit : public QLineEdit
{
    Q_OBJECT

    Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
    Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
    PrefLineEdit ( QWidget * parent = 0 );
    virtual ~PrefLineEdit();

    QByteArray entryName    () const;
    QByteArray paramGrpPath () const;
    void  setEntryName     ( const QByteArray& name );
    void  setParamGrpPath  ( const QByteArray& name );

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class PrefComboBox : public QComboBox
{
    Q_OBJECT

    Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
    Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
    PrefComboBox ( QWidget * parent = 0 );
    virtual ~PrefComboBox();

    QByteArray entryName    () const;
    QByteArray paramGrpPath () const;
    void  setEntryName     ( const QByteArray& name );
    void  setParamGrpPath  ( const QByteArray& name );

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class PrefCheckBox : public QCheckBox
{
    Q_OBJECT

    Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
    Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
    PrefCheckBox ( QWidget * parent = 0 );
    virtual ~PrefCheckBox();

    QByteArray entryName    () const;
    QByteArray paramGrpPath () const;
    void  setEntryName     ( const QByteArray& name );
    void  setParamGrpPath  ( const QByteArray& name );

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class PrefRadioButton : public QRadioButton
{
    Q_OBJECT

    Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
    Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
    PrefRadioButton ( QWidget * parent = 0 );
    virtual ~PrefRadioButton();

    QByteArray entryName    () const;
    QByteArray paramGrpPath () const;
    void  setEntryName     ( const QByteArray& name );
    void  setParamGrpPath  ( const QByteArray& name );

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class PrefSlider : public QSlider
{
    Q_OBJECT

    Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
    Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
    PrefSlider ( QWidget * parent = 0 );
    virtual ~PrefSlider();

    QByteArray entryName    () const;
    QByteArray paramGrpPath () const;
    void  setEntryName     ( const QByteArray& name );
    void  setParamGrpPath  ( const QByteArray& name );

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

} // namespace Gui

#endif // GUI_CUSTOMWIDGETS_H
