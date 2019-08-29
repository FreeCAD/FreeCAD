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
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef GUI_QUANTITYSPINBOX_H
#define GUI_QUANTITYSPINBOX_H

#include <QAbstractSpinBox>
#include <Gui/MetaTypes.h>
#include "ExpressionBinding.h"

#ifdef Q_MOC_RUN
Q_DECLARE_METATYPE(Base::Quantity)
#endif

namespace Gui {

class QuantitySpinBoxPrivate;
class GuiExport QuantitySpinBox : public QAbstractSpinBox, public ExpressionBinding
{
    Q_OBJECT

    Q_PROPERTY(QString unit READ unitText WRITE setUnitText)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(double rawValue READ rawValue WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(Base::Quantity value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(QString binding READ boundToName WRITE setBoundToByName)
    Q_PROPERTY(QString expression READ expressionText)

public:
    using ExpressionBinding::apply;

    explicit QuantitySpinBox(QWidget *parent = 0);
    virtual ~QuantitySpinBox();

    /// Get the current quantity
    Base::Quantity value() const;
    /// Get the current quantity without unit
    double rawValue() const;

    /// Gives the current state of the user input, gives true if it is a valid input with correct quantity
    /// or returns false if the input is a unparsable string or has a wrong unit.
    bool hasValidInput() const;

    /** Sets the Unit this widget is working with.
     *  After setting the Unit the widget will only accept
     *  user input with this unit type. Or if the user input 
     *  a value without unit, this one will be added to the resulting
     *  Quantity.
     */
    Base::Unit unit() const;
    void setUnit(const Base::Unit &unit);
    /// Set the unit property
    void setUnitText(const QString&);
    /// Get the unit property
    QString unitText(void);

    /// Get the value of the singleStep property
    double singleStep() const;
    /// Set the value of the singleStep property 
    void setSingleStep(double val);

    /// Gets the value of the minimum property
    double minimum() const;
    /// Sets the value of the minimum property 
    void setMinimum(double min);

    /// Gets the value of the maximum property
    double maximum() const;
    /// Sets the value of the maximum property 
    void setMaximum(double max);

    /// Gets the path of the bound property
    QString boundToName() const;
    /// Sets the path of the bound property
    void setBoundToByName(const QString &path);

    /// Gets the expression as a string
    QString expressionText() const;

    /// Set the number portion selected
    void selectNumber();

    void setRange(double min, double max);

    Base::Quantity valueFromText(const QString &text) const;
    QString textFromValue(const Base::Quantity& val) const;
    virtual void stepBy(int steps);
    virtual void clear();
    virtual QValidator::State validate(QString &input, int &pos) const;
    virtual void fixup(QString &str) const;

    bool event(QEvent *event);

    void setExpression(boost::shared_ptr<App::Expression> expr);
    void bind(const App::ObjectIdentifier &_path);
    bool apply(const std::string &propName);

public Q_SLOTS:
    /// Sets the field with a quantity
    void setValue(const Base::Quantity& val);
    /// Set a numerical value which gets converted to a quantity with the currently set unit type
    void setValue(double);

protected Q_SLOTS:
    void userInput(const QString & text);
    void openFormulaDialog();
    void finishFormulaDialog();
    
    //get notified on expression change
    virtual void onChange();

protected:
    virtual StepEnabled stepEnabled() const;
    virtual void showEvent(QShowEvent * event);
    virtual void focusInEvent(QFocusEvent * event);
    virtual void focusOutEvent(QFocusEvent * event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void resizeEvent(QResizeEvent *event);

private:
    void updateText(const Base::Quantity&);

Q_SIGNALS:
    /** Gets emitted if the user has entered a VALID input
     *  Valid means the user inputted string obeys all restrictions
     *  like: minimum, maximum and/or the right Unit (if specified).
     */
    void valueChanged(const Base::Quantity&);
    /** Gets emitted if the user has entered a VALID input
     *  Valid means the user inputted string obeys all restrictions
     *  like: minimum, maximum and/or the right Unit (if specified).
     */
    void valueChanged(double);
    /** Gets emitted if formula dialog is about to be opened (true)
     *  or finished (false).
     */
    void showFormulaDialog(bool);

private:
    QScopedPointer<QuantitySpinBoxPrivate> d_ptr;
    Q_DISABLE_COPY(QuantitySpinBox)
    Q_DECLARE_PRIVATE(QuantitySpinBox)
};

} // namespace Gui

#endif // GUI_QUANTITYSPINBOX_H
