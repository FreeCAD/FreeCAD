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
 * \author Jürgen Riegel
 */
class GuiExport InputField : public QLineEdit
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  InputField ( QWidget * parent = 0 );
  virtual ~InputField();

  /** @name history and default management */
  //@{
  /// the param group path where the widget write and read the dafault values
  QByteArray paramGrpPath () const;
  /// set the param group path where the widget write and read the dafault values
  void  setParamGrpPath  ( const QByteArray& name );
  /// push a new value to the history
  void pushToHistory(std::string value);
  /// get the history of the field, newest first
  std::vector<std::string> getHistory(void);
  //@}

protected Q_SLOTS:
    void newInput(const QString & text);

protected:
    virtual void 	contextMenuEvent ( QContextMenuEvent * event );

private:
  QByteArray m_sPrefGrp;
  std::string ErrorText;

  /// handle to the parameter group for defaults and history
  ParameterGrp::handle _handle;
  std::string sGroupString;
};



} // namespace Gui

#endif // GUI_INPUTFIELD_H
