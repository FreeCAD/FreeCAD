/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <juergen.riegel@web.de>             *
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


#ifndef PARTGUI_VIEWPROVIDERPARTEXT_H
#define PARTGUI_VIEWPROVIDERPARTEXT_H

#include <Standard_math.hxx>
#include <Standard_Boolean.hxx>
#include <TopoDS_Shape.hxx>
#include <Gui/ViewProviderGeometryObject.h>
#include <map>

class TopoDS_Shape;
class TopoDS_Edge;
class TopoDS_Wire;
class TopoDS_Face;
class SoSeparator;
class SoGroup;
class SoSwitch;
class SoVertexShape;
class SoPickedPoint;
class SoShapeHints;
class SoEventCallback;
class SbVec3f;
class SoSphere;
class SoScale;
class SoCoordinate3;
class SoIndexedFaceSet;
class SoNormal;
class SoNormalBinding;
class SoMaterialBinding;
class SoIndexedLineSet;

namespace PartGui {

class SoBrepFaceSet;
class SoBrepEdgeSet;
class SoBrepPointSet;

class PartGuiExport ViewProviderPartExt : public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER(PartGui::ViewProviderPartExt);

public:
    /// constructor
    ViewProviderPartExt();
    /// destructor
    virtual ~ViewProviderPartExt();

    // Display properties
    App::PropertyFloatConstraint LineWidth;
    App::PropertyFloatConstraint PointSize;
    App::PropertyFloatConstraint Deviation;
    App::PropertyColor LineColor;
    App::PropertyColor PointColor;
    App::PropertyMaterial LineMaterial;
    App::PropertyMaterial PointMaterial;
    App::PropertyEnumeration Lighting;
    App::PropertyEnumeration DrawStyle;

    App::PropertyColorList DiffuseColor;


    virtual void attach(App::DocumentObject *);
    virtual void setDisplayMode(const char* ModeName);
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    /// Update the view representation
    void reload();

    virtual void updateData(const App::Property*);

      /** @name Selection handling
      * This group of methodes do the selection handling.
      * Here you can define how the selection for your ViewProfider
      * works. 
     */
    //@{
    /// indicates if the ViewProvider use the new Selection model
    virtual bool useNewSelectionModel(void) const {return true;}
    /// return a hit element to the selection path or 0
    virtual std::string getElement(const SoDetail*) const;
    virtual SoDetail* getDetail(const char*) const;
    virtual std::vector<Base::Vector3d> getPickedPoints(const SoPickedPoint *) const;
    /// return the higlight lines for a given element or the whole shape
    virtual std::vector<Base::Vector3d> getSelectionShape(const char* Element) const;
    //@}

    /** @name Edit methods */
    //@{
    void setupContextMenu(QMenu*, QObject*, const char*);
protected:
    bool setEdit(int ModNum);
    void unsetEdit(int ModNum);
    //@}

protected:
    /// get called by the container whenever a property has been changed
    virtual void onChanged(const App::Property* prop);
    bool loadParameter();
    void updateVisual(const TopoDS_Shape &);

    // nodes for the data representation
    SoMaterialBinding * pcShapeBind;
    SoMaterial        * pcLineMaterial;
    SoMaterial        * pcPointMaterial;
    SoDrawStyle       * pcLineStyle;
    SoDrawStyle       * pcPointStyle;
    SoShapeHints      * pShapeHints;

    SoCoordinate3     * coords;
    SoBrepFaceSet     * faceset;
    SoNormal          * norm;
    SoNormalBinding   * normb;
    SoBrepEdgeSet     * lineset;
    SoBrepPointSet    * nodeset;

    bool VisualTouched;

private:
    // settings stuff
    bool noPerVertexNormals;
    bool qualityNormals;
    static App::PropertyFloatConstraint::Constraints sizeRange;
    static App::PropertyFloatConstraint::Constraints tessRange;
    static const char* LightingEnums[];
    static const char* DrawStyleEnums[];
};

}

#endif // PARTGUI_VIEWPROVIDERPARTEXT_H

