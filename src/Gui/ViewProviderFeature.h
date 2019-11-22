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


#ifndef GUI_VIEWPROVIDER_FEATURE_H
#define GUI_VIEWPROVIDER_FEATURE_H

#include <Inventor/lists/SoPickedPointList.h> 
#include <App/PropertyStandard.h>
#include "ViewProviderDocumentObject.h"

class SoPickedPointList;
class SbVec2s;

namespace Gui {

class View3DInventorViewer;

class GuiExport ViewProviderFeature:public ViewProviderDocumentObject
{
    PROPERTY_HEADER(Gui::ViewProviderFeature);

public:
    /// constructor.
    ViewProviderFeature();

    /// destructor.
    virtual ~ViewProviderFeature();

    App::PropertyColorList    ColourList;

    /**
     * Attaches the document object to this view provider.
     */
    virtual void attach(App::DocumentObject *pcObj);
};

} // namespace Gui

#endif // GUI_VIEWPROVIDER_FEATURE_H

