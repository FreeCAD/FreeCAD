/******************************************************************************
 *   Copyright (c)2012 Konstantinos Poulios <logari81@gmail.com>              *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#ifndef GUI_ReferenceSelection_H
#define GUI_ReferenceSelection_H

#include <Gui/SelectionFilter.h>

namespace PartDesignGui {

class ReferenceSelection : public Gui::SelectionFilterGate
{
    const App::DocumentObject* support;
    bool edge, plane;
    bool planar;
public:
    ReferenceSelection(const App::DocumentObject* support_,
                       const bool edge_, const bool plane_, const bool planar_)
        : Gui::SelectionFilterGate((Gui::SelectionFilter*)0),
          support(support_), edge(edge_), plane(plane_), planar(planar_)
    {
    }
    /**
      * Allow the user to pick only edges or faces (or both) from the defined support
      * Optionally restrict the selection to planar edges/faces
      */
    bool allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName);
};

} //namespace PartDesignGui

#endif // GUI_ReferenceSelection_H
