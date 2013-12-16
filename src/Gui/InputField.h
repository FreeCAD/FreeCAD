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

#include <Base/Parameter.h>
#include <Base/Quantity.h>
#include "Widgets.h"
#include "Window.h"
#include "SpinBox.h"
#include "FileDialog.h"

namespace Gui {


/**
 * The InputField class
 * The input field widget handles all around user input of Quantities. Thats
 * include parsing and checking input. Providing a context menu for common operations
 * and managing default and history values.
 * Although its derived from a QLineEdit widget, its supports most of the properties and signals
 * of a  
 * \author Jürgen Riegel
 */
class GuiExport InputField : public QLineEdit
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath )
  Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep )
  Q_PROPERTY(double maximum READ maximum WRITE setMaximum )
  Q_PROPERTY(double minimum READ minimum WRITE setMinimum )
  Q_PROPERTY(int historySize READ historySize WRITE setHistorySize )


public:
  InputField ( QWidget * parent = 0 );
  virtual ~InputField();

  /// sets the field with a quantity
  void setValue(const Base::Quantity&);
  /// get the actual value
  Base::Quantity getQuantity(void)const{return this->actQuantity;}
  /** sets the Unit this field working with. 
  *   After seting the Unit the field will only acceppt
  *   user input with this unit type. Or if the user input 
  *   a value without unit, this one will be added to the resulting
  *   Quantity. 
  */
  void setUnit(const Base::Unit&);

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

  /// set the number portion selected (use after setValue()) 
  void selectNumber(void);

  /** @name history and default management */
  //@{
  /// the param group path where the widget write and read the dafault values
  QByteArray paramGrpPath () const;
  /// set the param group path where the widget write and read the dafault values
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


Q_SIGNALS:
    /** gets emited if the user has entered a VALID input
    *   Valid means the user inputed string obays all restrictions
    *   like: minimum, maximum and/or the right Unit (if specified). 
    *   If you want the unfiltered/unvalidated input use valueChanged(const QString&) 
    *   instead:
    */
    void valueChanged(const Base::Quantity&);
        /** gets emited if the user has entered a VALID input
    *   Valid means the user inputed string obays all restrictions
    *   like: minimum, maximum and/or the right Unit (if specified). 
    *   If you want the unfiltered/unvalidated input use valueChanged(const QString&) 
    *   instead:
    */ 
    void valueChanged(double);

    /// signal for an invalid user input (signals a lot while typing!)
    void parseError(const QString& errorText);

protected Q_SLOTS:
    void newInput(const QString & text);

    void wheelEvent ( QWheelEvent * event ) ;
protected:
    virtual void 	contextMenuEvent ( QContextMenuEvent * event );

private:
  QByteArray m_sPrefGrp;
  std::string ErrorText;

  /// handle to the parameter group for defaults and history
  ParameterGrp::handle _handle;
  std::string sGroupString;

  Base::Quantity actQuantity;
  Base::Unit     actUnit;
  double         actUnitValue;
  QString        actUnitStr;

  double Maximum;
  double Minimum;
  double StepSize;
  int HistorySize;
  int SaveSize;
};



} // namespace Gui

#endif // GUI_INPUTFIELD_H
