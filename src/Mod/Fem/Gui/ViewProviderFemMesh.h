/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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

#ifndef FEM_VIEWPROVIDERFEMMESH_H
#define FEM_VIEWPROVIDERFEMMESH_H

#include <Gui/ViewProviderBuilder.h>
#include <Gui/ViewProviderGeometryObject.h>
#include <Gui/ViewProviderPythonFeature.h>
#include <Mod/Fem/FemGlobal.h>

class SoCoordinate3;
class SoDrawStyle;
class SoIndexedFaceSet;
class SoIndexedLineSet;
class SoShapeHints;
class SoMaterialBinding;

namespace FemGui
{

class ViewProviderFEMMeshBuilder : public Gui::ViewProviderBuilder
{
public:
    ViewProviderFEMMeshBuilder(){}
    virtual ~ViewProviderFEMMeshBuilder(){}
    virtual void buildNodes(const App::Property*, std::vector<SoNode*>&) const;
    void createMesh(const App::Property*,
                    SoCoordinate3*,
                    SoIndexedFaceSet*,
                    SoIndexedLineSet*,
                    std::vector<unsigned long>&,
                    std::vector<unsigned long>&,
                    bool &edgeOnly,
                    bool ShowInner,
                    int MaxFacesShowInner
                   ) const;
};

class FemGuiExport ViewProviderFemMesh : public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER(FemGui::ViewProviderFemMesh);

public:
    /// constructor.
    ViewProviderFemMesh();

    /// destructor.
    virtual ~ViewProviderFemMesh();

    // Display properties
    App::PropertyColor PointColor;
    App::PropertyFloatConstraint PointSize;
    App::PropertyFloatConstraint LineWidth;
    App::PropertyBool     BackfaceCulling;
    App::PropertyBool     ShowInner;
    App::PropertyInteger  MaxFacesShowInner;

    void attach(App::DocumentObject *pcObject);
    void setDisplayMode(const char* ModeName);
    std::vector<std::string> getDisplayModes() const;
    void updateData(const App::Property*);

      /** @name Selection handling
      * This group of methods do the selection handling.
      * Here you can define how the selection for your ViewProvider
      * works.
     */
    //@{
    /// indicates if the ViewProvider use the new Selection model
    virtual bool useNewSelectionModel(void) const {return true;}
    /// return a hit element to the selection path or 0
    virtual std::string getElement(const SoDetail*) const;
    virtual SoDetail* getDetail(const char*) const;
    /// return the highlight lines for a given element or the whole shape
    virtual std::vector<Base::Vector3d> getSelectionShape(const char* Element) const;
    //@}

    // interface methods
    void setHighlightNodes(const std::set<long>&);
    std::set<long> getHighlightNodes() const;
    void resetHighlightNodes(void);

    /** @name Postprocessing
      * this interfaces apply post processing stuff to the View-
      * Provider. They can override the positioning and the color
      * color or certain elements.
     */
    //@{

    /// set the color for each node
    void setColorByNodeId(const std::map<long,App::Color> &NodeColorMap);
    void setColorByNodeId(const std::vector<long> &NodeIds,const std::vector<App::Color>  &NodeColors);

    /// reset the view of the node colors
    void resetColorByNodeId(void);
    /// set the displacement for each node
    void setDisplacementByNodeId(const std::map<long,Base::Vector3d> &NodeDispMap);
    void setDisplacementByNodeId(const std::vector<long> &NodeIds,const std::vector<Base::Vector3d> &NodeDisps);
    /// reset the view of the node displacement
    void resetDisplacementByNodeId(void);
    /// reaply the node displacement with a certain factor and do a redraw
    void applyDisplacementToNodes(double factor);
    /// set the color for each element
    void setColorByElementId(const std::map<long,App::Color> &ElementColorMap);
    /// reset the view of the element colors
    void resetColorByElementId(void);
    //@}

    const std::vector<unsigned long> &getVisibleElementFaces(void)const{return vFaceElementIdx;}

    PyObject *getPyObject();

private:
    static App::PropertyFloatConstraint::Constraints floatRange;

    Py::Object PythonObject;

protected:
    /// get called by the container whenever a property has been changed
    virtual void onChanged(const App::Property* prop);

    void setColorByNodeIdHelper(const std::vector<App::Color> &);
    void setDisplacementByNodeIdHelper(const std::vector<Base::Vector3d>& DispVector,long startId);
    /// index of elements to their triangles
    std::vector<unsigned long> vFaceElementIdx;
    std::vector<unsigned long> vNodeElementIdx;
    std::vector<unsigned long> vHighlightedIdx;

    std::vector<Base::Vector3d> DisplacementVector;
    double                      DisplacementFactor;

    SoMaterial            * pcPointMaterial;
    SoDrawStyle           * pcPointStyle;

    SoDrawStyle           * pcDrawStyle;
    SoShapeHints          * pShapeHints;
    SoMaterialBinding     * pcMatBinding;
    SoCoordinate3         * pcCoords;
    SoCoordinate3         * pcAnoCoords;
    SoIndexedFaceSet      * pcFaces;
    SoIndexedLineSet      * pcLines;

    bool onlyEdges;

private:
    class Private;
};

typedef Gui::ViewProviderPythonFeatureT<ViewProviderFemMesh> ViewProviderFemMeshPython;


} //namespace FemGui


#endif // FEM_VIEWPROVIDERFEMMESH_H
