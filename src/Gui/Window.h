/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_WINDOW_H
#define GUI_WINDOW_H


#include <Base/Parameter.h>
#include "View.h"

namespace Gui {

/** Adapter class to the parameter of FreeCAD for all windows
 * Retrieve the parameter group of the specific window by the windowname.
 * @author Jürgen Riegel
 */
class GuiExport WindowParameter : public ParameterGrp::ObserverType
{
public:
  WindowParameter(const char *name);
  virtual ~WindowParameter();

  bool setGroupName( const char* name );
  void OnChange(Base::Subject<const char*> &rCaller, const char * sReason);

  /// get the parameters
  static ParameterGrp::handle getDefaultParameter(void);
  /// return the parameter group of this window
  ParameterGrp::handle getWindowParameter(void);

private:
  ParameterGrp::handle _handle;
};

} // namespace Gui

#endif // GUI_WINDOW_H
