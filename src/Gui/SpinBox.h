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

#ifndef GUI_SPINBOX_H
#define GUI_SPINBOX_H

#include <QSpinBox>
#include <QValidator>
#include "ExpressionBinding.h"


class QStyleOptionSpinBox;

namespace App {
class NumberExpression;
}

namespace Gui {

class GuiExport ExpressionSpinBox : public ExpressionBinding
{
public:
    ExpressionSpinBox(QAbstractSpinBox*);
    virtual ~ExpressionSpinBox();

    void bind(const App::ObjectIdentifier &_path);
    void setExpression(std::shared_ptr<App::Expression> expr);

protected:
    /*! Expression handling */
    //@{
    enum class Number {
        KeepCurrent = 0,
        SetIfNumber = 1
    };
    void showInvalidExpression(const QString&);
    void showValidExpression(Number number);
    void clearExpression();
    void updateExpression();
    //@}

    void onChange();
    virtual void setNumberExpression(App::NumberExpression*) = 0;
    void resizeWidget();

    bool handleKeyEvent(const QString&);
    virtual void openFormulaDialog();

    void drawControl(QStyleOptionSpinBox&);

protected:
    QLineEdit* lineedit;
    QAbstractSpinBox* spinbox;
};

/**
 * A validator that allows only input of unsigned int values in the range
 * from 0 to UINT_MAX.
 */
class GuiExport UnsignedValidator : public QValidator
{
    Q_OBJECT
    Q_PROPERTY( uint bottom READ bottom WRITE setBottom )
    Q_PROPERTY( uint top READ top WRITE setTop )

public:
    UnsignedValidator( QObject * parent );
    UnsignedValidator( uint bottom, uint top, QObject * parent );
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

class UIntSpinBoxPrivate;
/**
 * The UIntSpinBox class does basically the same as Qt's QSpinBox unless
 * that it works with unsigned int's instead.
 * This allows to use numbers in the range of [0, UINT_MAX]
 * @author Werner Mayer
 */
class GuiExport UIntSpinBox : public QSpinBox, public ExpressionSpinBox
{
    Q_OBJECT
    Q_OVERRIDE( uint maximum READ maximum WRITE setMaximum )
    Q_OVERRIDE( uint minimum READ minimum WRITE setMinimum )
    Q_OVERRIDE( uint value READ value WRITE setValue )

public:
    UIntSpinBox ( QWidget* parent=nullptr );
    virtual ~UIntSpinBox();

    void setRange( uint minVal, uint maxVal );
    uint value() const;
    virtual QValidator::State validate ( QString & input, int & pos ) const;
    uint minimum() const;
    void setMinimum( uint value );
    uint maximum() const;
    void setMaximum( uint value );

    bool apply(const std::string &propName);
    using ExpressionSpinBox::apply;

    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);

Q_SIGNALS:
    void valueChanged( uint value ); // clazy:exclude=overloaded-signal

public Q_SLOTS:
    void setValue( uint value );

private Q_SLOTS:
    void valueChange( int value );

protected:
    virtual QString textFromValue ( int v ) const;
    virtual int valueFromText ( const QString & text ) const;
    virtual void setNumberExpression(App::NumberExpression*);

private:
    void updateValidator();
    UIntSpinBoxPrivate * d;
};


/**
 * The IntSpinBox class does exactly the same as Qt's QSpinBox but has expression support
 * @author Stefan Tröger
 */
class GuiExport IntSpinBox : public QSpinBox, public ExpressionSpinBox
{
    Q_OBJECT

public:
    IntSpinBox ( QWidget* parent=nullptr );
    virtual ~IntSpinBox();

    bool apply(const std::string &propName);
    using ExpressionSpinBox::apply;
    void setNumberExpression(App::NumberExpression*);

    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
};

/**
 * The DoubleSpinBox class does exactly the same as Qt's QDoubleSpinBox but has expression
 * support
 * @author Stefan Tröger
 */
class GuiExport DoubleSpinBox : public QDoubleSpinBox, public ExpressionSpinBox
{
    Q_OBJECT

public:
    DoubleSpinBox ( QWidget* parent=nullptr );
    virtual ~DoubleSpinBox();

    bool apply(const std::string &propName);
    using ExpressionSpinBox::apply;
    void setNumberExpression(App::NumberExpression*);

    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
};

} // namespace Gui

#endif // GUI_SPINBOX_H
