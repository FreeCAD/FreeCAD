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


#ifndef GUI_INPUTFIELD_H
#define GUI_INPUTFIELD_H

#include <QValidator>
#include <App/Property.h>
#include <App/Expression.h>
#include <Base/Parameter.h>
#include <Gui/MetaTypes.h>
#include "Widgets.h"
#include "Window.h"
#include "SpinBox.h"
#include "FileDialog.h"
#include "ExpressionBinding.h"
#include "ExpressionCompleter.h"

#ifdef Q_MOC_RUN
Q_DECLARE_METATYPE(Base::Quantity)
#endif

namespace App {
class DocumentObject;
class ObjectIdentifier;
class Expression;
}

namespace Gui {


/**
 * The InputField class
 * The input field widget handles all around user input of Quantities. That
 * includes parsing and checking input. Providing a context menu for common operations
 * and managing default and history values.
 * Although it's derived from a QLineEdit widget, it supports most of the properties and signals
 * of a spin box.
 * \author Jürgen Riegel
 */
class GuiExport InputField : public ExpressionLineEdit, public ExpressionBinding
{
    Q_OBJECT

    Q_PROPERTY(QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath )
    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep )
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum )
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum )
    Q_PROPERTY(double rawValue READ rawValue WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int historySize READ historySize WRITE setHistorySize )
    Q_PROPERTY(QString unit READ getUnitText WRITE setUnitText )
    Q_PROPERTY(int precision READ getPrecision WRITE setPrecision )
    Q_PROPERTY(QString format READ getFormat WRITE setFormat )
    Q_PROPERTY(Base::Quantity quantity READ getQuantity WRITE setValue )
    Q_PROPERTY(QString quantityString READ getQuantityString WRITE setQuantityString )
    Q_PROPERTY(QString rawText READ rawText WRITE setRawText )


public:
    InputField (QWidget * parent = 0);
    virtual ~InputField();

    /// set the field with a quantity
    void setValue(const Base::Quantity&);
    //set a numerical value which gets converted to a quantity with the currently set unit type
    void setValue(const double&);

    /// get the current value
    Base::Quantity getQuantity(void)const{return this->actQuantity;}
    /// Get the current quantity without unit
    double rawValue() const;

    /// get stored, valid quantity as a string (user string - avoid storing)
    QString getQuantityString(void) const;

    /// set, validate and display quantity from a string. Must match existing units.
    void setQuantityString(const QString& text);

    /// return the quantity in C locale, i.e. decimal separator is a dot.
    QString rawText(void) const;

    /// expects the string in C locale and internally converts it into the OS-specific locale
    void setRawText(const QString& text);

    /// gives the current state of the user input, gives true if it is a valid input with correct quantity
    /// (shown by the green pixmap), returns false if the input is a unparsable string or has a wrong unit
    /// (shown by the red pixmap in the gui)
    bool hasValidInput() { return validInput;}

    /** sets the Unit this field is working with. 
     *  After setting the Unit the field will only accept
     *  user input with this unit type. Or if the user input 
     *  a value without unit, this one will be added to the resulting
     *  Quantity. 
     */
    void setUnit(const Base::Unit&);
    const Base::Unit& getUnit() const;

    /// set the input field to the last used value (works only if the setParamGrpPath() was called)
    void setToLastUsedValue(void);
    /// get the value of the singleStep property
    double singleStep(void)const;
    /// set the value of the singleStep property 
    void setSingleStep(double);
    /// get the value of the maximum property
    double maximum(void)const;
    /// set the value of the maximum property 
    void setMaximum(double);
    /// get the value of the minimum property
    double minimum(void)const;
    /// set the value of the minimum property 
    void setMinimum(double);
    /// get the value of the minimum property
    int historySize(void)const;
    /// set the value of the minimum property 
    void setHistorySize(int);
    /// set the unit by a string (can be used in the *.ui file)  
    void setUnitText(const QString&);
    /// get the unit as a string (can be used in the *.ui file)  
    QString getUnitText(void); 
    /// get the value of the precision property
    int getPrecision(void) const; 
    /// set the value of the precision property (can be used in the *.ui file)  
    void setPrecision(const int);
    /// get the value of the format property: "f" (fixed), "e" (scientific), "g" (general)
    QString getFormat(void) const; 
    /// set the value of the format property (can be used in the *.ui file)  
    void setFormat(const QString&);
    /// set the number portion selected (use after setValue()) 
    void selectNumber(void);
    /// input validation
    void fixup(QString& input) const;
    /// input validation
    QValidator::State validate(QString& input, int& pos) const;

    /** @name history and default management */
    //@{
    /// the param group path where the widget writes and reads the default values
    QByteArray paramGrpPath () const;
    /// set the param group path where the widget writes and reads the default values
    void  setParamGrpPath  ( const QByteArray& name );
    /// push a new value to the history, if no string given the actual text of the input field is used. 
    void pushToHistory(const QString &valueq = QString());
    /// get the history of the field, newest first
    std::vector<QString> getHistory(void);
    /// push a new value to the history, if no string given the actual text of the input field is used. 
    void pushToSavedValues(const QString &valueq = QString());
    /// get the history of the field, newest first
    std::vector<QString> getSavedValues(void);
    //@}

    void bind(const App::ObjectIdentifier &_path);
    bool apply(const std::string &propName);
    bool apply();

Q_SIGNALS:
    /** gets emitted if the user has entered a VALID input
     *  Valid means the user inputted string obeys all restrictions
     *  like: minimum, maximum and/or the right Unit (if specified). 
     *  If you want the unfiltered/unvalidated input use textChanged(const QString&) 
     *  instead:
     */
    void valueChanged(const Base::Quantity&);
    /** gets emitted if the user has entered a VALID input
     *  Valid means the user inputted string obeys all restrictions
     *  like: minimum, maximum and/or the right Unit (if specified). 
     *  If you want the unfiltered/unvalidated input use textChanged(const QString&) 
     *  instead:
     */
    void valueChanged(double);

    /// signal for an invalid user input (signals a lot while typing!)
    void parseError(const QString& errorText);

protected Q_SLOTS:
    void newInput(const QString & text);
    void updateIconLabel(const QString& text);

protected:
    virtual void showEvent(QShowEvent * event);
    virtual void focusInEvent(QFocusEvent * event);
    virtual void focusOutEvent(QFocusEvent * event);
    virtual void keyPressEvent(QKeyEvent * event);
    virtual void wheelEvent(QWheelEvent * event);
    virtual void contextMenuEvent(QContextMenuEvent * event);
    virtual void resizeEvent(QResizeEvent*);

private:
    QPixmap getValidationIcon(const char* name, const QSize& size) const;
    void updateText(const Base::Quantity&);

private:
    QLabel* iconLabel;
    QByteArray m_sPrefGrp;
    bool validInput;

    /// handle to the parameter group for defaults and history
    ParameterGrp::handle _handle;
    std::string sGroupString;

    Base::Quantity actQuantity;
    Base::Unit     actUnit;
    double         actUnitValue;

    double Maximum;
    double Minimum;
    double StepSize;
    int HistorySize;
    int SaveSize;

    QPalette defaultPalette;
};

} // namespace Gui

#endif // GUI_INPUTFIELD_H
