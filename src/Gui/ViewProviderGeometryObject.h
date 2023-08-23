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

#include "ViewProviderDragger.h"
#include <Inventor/lists/SoPickedPointList.h>

class SoPickedPointList;
class SoSwitch;
class SoSensor;
class SbVec2s;
class SoBaseColor;

namespace Gui {

class SoFCSelection;
class SoFCBoundingBox;
class View3DInventorViewer;

/**
 * The base class for all view providers that display geometric data, like mesh, point clouds and shapes.
 * @author Werner Mayer
 */
class GuiExport ViewProviderGeometryObject : public ViewProviderDragger
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderGeometryObject);

public:
    /// constructor.
    ViewProviderGeometryObject();

    /// destructor.
    ~ViewProviderGeometryObject() override;

    // Display properties
    App::PropertyColor ShapeColor;
    App::PropertyPercent Transparency;
    App::PropertyMaterial ShapeMaterial;
    App::PropertyBool BoundingBox;
    App::PropertyBool Selectable;

    /**
     * Attaches the document object to this view provider.
     */
    void attach(App::DocumentObject *pcObject) override;
    void updateData(const App::Property*) override;

    bool isSelectable() const override {return Selectable.getValue();}

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
    virtual void showBoundingBox(bool);
    //@}

protected:
    /// get called by the container whenever a property has been changed
    void onChanged(const App::Property* prop) override;
    void setSelectable(bool Selectable=true);

    virtual unsigned long getBoundColor() const;

protected:
    SoMaterial       * pcShapeMaterial{nullptr};
    SoFCBoundingBox  * pcBoundingBox{nullptr};
    SoSwitch         * pcBoundSwitch{nullptr};
    SoBaseColor      * pcBoundColor{nullptr};
};

} // namespace Gui


#endif // GUI_VIEWPROVIDER_GEOMETRYOBJECT_H
