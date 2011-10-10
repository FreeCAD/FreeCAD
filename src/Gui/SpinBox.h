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

#include <QValidator>
#include <QSpinBox>

namespace Gui {

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
class GuiExport UIntSpinBox : public QSpinBox
{
    Q_OBJECT
    Q_OVERRIDE( uint maximum READ maximum WRITE setMaximum )
    Q_OVERRIDE( uint minimum READ minimum WRITE setMinimum )
    Q_OVERRIDE( uint value READ value WRITE setValue )

public:
    UIntSpinBox ( QWidget* parent=0 );
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

} // namespace Gui

#endif // GUI_SPINBOX_H
