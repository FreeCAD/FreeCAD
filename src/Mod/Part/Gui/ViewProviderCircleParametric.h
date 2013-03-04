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


#ifndef PARTGUI_VIEWPROVIDERCIRCLEPARAMETRIC_H
#define PARTGUI_VIEWPROVIDERCIRCLEPARAMETRIC_H

#include "ViewProvider.h"


class TopoDS_Shape;
class TopoDS_Face;
class SoSeparator;
class SbVec3f;
class SoTransform;

namespace Gui {
  class View3DInventorViewer;
  class SoFCSelection;
}

namespace PartGui {


class PartGuiExport ViewProviderCircleParametric:public ViewProviderPart
{
    PROPERTY_HEADER(PartGui::ViewProviderCircleParametric);

public:
    /// constructor
    ViewProviderCircleParametric();
    /// destructor
    virtual ~ViewProviderCircleParametric();

    std::vector<std::string> getDisplayModes(void) const;

protected:

};

} // namespace PartGui


#endif // PARTGUI_VIEWPROVIDERCIRCLEPARAMETRIC_H

