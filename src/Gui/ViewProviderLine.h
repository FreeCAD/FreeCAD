/***************************************************************************
 *   Copyright (c) Juergen Riegel          (juergen.riegel@web.de) 2012    *
 *   Copyright (c) Alexander Golubev (Fat-Zer) <fatzer2@gmail.com> 2015    *
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


#ifndef GUI_ViewProviderLine_H
#define GUI_ViewProviderLine_H

#include "ViewProviderOriginFeature.h"

namespace Gui
{

class GuiExport ViewProviderLine : public ViewProviderOriginFeature {
    PROPERTY_HEADER(Gui::ViewProviderLine);
public:
    /// Constructor
    ViewProviderLine(void);
    virtual ~ViewProviderLine();

    virtual void attach ( App::DocumentObject * );
};

} //namespace Gui


#endif // GUI_ViewProviderLine_H
