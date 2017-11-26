/***************************************************************************
 *   Copyright (c) 2017 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_VIEWPROVIDER_DRAGGER_H
#define GUI_VIEWPROVIDER_DRAGGER_H

#include "ViewProviderDocumentObject.h"

class SoDragger;
class SoTransform;

namespace Base { class Placement;}

namespace Gui {

class View3DInventorViewer;
class SoFCCSysDragger;

/**
 * The base class for all view providers modifiying the placement
 * of a geometric feature.
 * @author Werner Mayer
 */
class GuiExport ViewProviderDragger : public ViewProviderDocumentObject
{
    PROPERTY_HEADER(Gui::ViewProviderDragger);

public:
    /// constructor.
    ViewProviderDragger();

    /// destructor.
    virtual ~ViewProviderDragger();

    /** @name Edit methods */
    //@{
    bool doubleClicked(void);
    void setupContextMenu(QMenu*, QObject*, const char*);
    void updateData(const App::Property*);

    /*! synchronize From FC placement to Coin placement*/
    static void updateTransform(const Base::Placement &from, SoTransform *to);

protected:
    bool setEdit(int ModNum);
    void unsetEdit(int ModNum);
    void setEditViewer(View3DInventorViewer*, int ModNum);
    void unsetEditViewer(View3DInventorViewer*);
    //@}
    SoFCCSysDragger *csysDragger = nullptr;

private:
    static void dragStartCallback(void * data, SoDragger * d);
    static void dragFinishCallback(void * data, SoDragger * d);
    
    static void updatePlacementFromDragger(ViewProviderDragger *sudoThis, SoFCCSysDragger *draggerIn);
};

} // namespace Gui


#endif // GUI_VIEWPROVIDER_DRAGGER_H

