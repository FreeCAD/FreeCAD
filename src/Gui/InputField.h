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
 * The InputField class.
 * \author Jürgen Riegel
 */
class GuiExport InputField : public QLineEdit
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  InputField ( QWidget * parent = 0 );
  virtual ~InputField();

  // PROPERTIES
  // getters
  QByteArray paramGrpPath () const;
  // setters
  void  setParamGrpPath  ( const QByteArray& name );

private:
  QByteArray m_sPrefGrp;

};



} // namespace Gui

#endif // GUI_INPUTFIELD_H
