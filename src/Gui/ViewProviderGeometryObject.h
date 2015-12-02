/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_VIEWPROVIDER_GEOMETRYOBJECT_H
#define GUI_VIEWPROVIDER_GEOMETRYOBJECT_H

#include <Inventor/lists/SoPickedPointList.h> 
#include "ViewProviderDocumentObject.h"

class SoPickedPointList;
class SoSwitch;
class SoSensor;
class SoDragger;
class SbVec2s;

namespace Gui {

class SoFCSelection;
class SoFCBoundingBox;
class View3DInventorViewer;

/**
 * The base class for all view providers that display geometric data, like mesh, point cloudes and shapes.
 * @author Werner Mayer
 */
class GuiExport ViewProviderGeometryObject : public ViewProviderDocumentObject
{
    PROPERTY_HEADER(Gui::ViewProviderGeometryObject);

public:
    /// constructor.
    ViewProviderGeometryObject();

    /// destructor.
    virtual ~ViewProviderGeometryObject();

    // Display properties
    App::PropertyColor ShapeColor;
    App::PropertyPercent Transparency;
    App::PropertyMaterial ShapeMaterial;
    App::PropertyBool BoundingBox;
    App::PropertyBool Selectable;

    /**
     * Attaches the document object to this view provider.
     */
    void attach(App::DocumentObject *pcObject);
    void updateData(const App::Property*);

    bool isSelectable(void) const {return Selectable.getValue();}

    /**
     * Returns a list of picked points from the geometry under \a getRoot().
     * If \a pickAll is false (the default) only the intersection point closest to the camera will be picked, otherwise
     * all intersection points will be picked. 
     */
    SoPickedPointList getPickedPoints(const SbVec2s& pos, const View3DInventorViewer& viewer,bool pickAll=false) const;
    /**
     * This method is provided for convenience and does basically the same as getPickedPoints() unless that only the closest
     * point to the camera will be picked.
     * \note It is in the response of the client programmer to delete the returned SoPickedPoint object.
     */
    SoPickedPoint* getPickedPoint(const SbVec2s& pos, const View3DInventorViewer& viewer) const;

    /** @name Edit methods */
    //@{
    bool doubleClicked(void);
    void setupContextMenu(QMenu*, QObject*, const char*);
protected:
    bool setEdit(int ModNum);
    void unsetEdit(int ModNum);
    void setEditViewer(View3DInventorViewer*, int ModNum);
    void unsetEditViewer(View3DInventorViewer*);
    //@}

protected:
    void showBoundingBox(bool);
    /// get called by the container whenever a property has been changed
    void onChanged(const App::Property* prop);
    void setSelectable(bool Selectable=true);

private:
    static void sensorCallback(void * data, SoSensor * sensor);
    static void dragStartCallback(void * data, SoDragger * d);
    static void dragFinishCallback(void * data, SoDragger * d);
    static void dragMotionCallback(void * data, SoDragger * d);
    bool m_dragStart;

protected:
    SoMaterial       * pcShapeMaterial;
    SoFCBoundingBox  * pcBoundingBox;
    SoSwitch         * pcBoundSwitch;
};

} // namespace Gui


#endif // GUI_VIEWPROVIDER_GEOMETRYOBJECT_H

