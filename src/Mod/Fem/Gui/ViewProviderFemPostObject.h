/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#ifndef FEM_VIEWPROVIDERFEMPOSTOBJECT_H
#define FEM_VIEWPROVIDERFEMPOSTOBJECT_H

#include <Base/Observer.h>
#include <Gui/ViewProviderGeometryObject.h>
#include <Mod/Fem/FemGlobal.h>

#include <vtkAppendPolyData.h>
#include <vtkExtractEdges.h>
#include <vtkGeometryFilter.h>
#include <vtkSmartPointer.h>
#include <vtkOutlineCornerFilter.h>
#include <vtkVertexGlyphFilter.h>

class SoIndexedPointSet;
class vtkUnsignedCharArray;
class vtkDataArray;
class vtkPoints;
class SoSeparator;
class SoNormal;
class SoNormalBinding;
class SoMaterial;
class SoShapeHints;
class SoMaterialBinding;
class SoIndexedFaceSet;
class SoIndexedLineSet;
class SoIndexedMarkerSet;
class SoCoordinate3;
class SoDrawStyle;
class SoIndexedFaceSet;
class SoIndexedLineSet;
class SoIndexedTriangleStripSet;

namespace Gui {
  class SoFCColorBar;
}

namespace FemGui
{

class TaskDlgPost;

class FemGuiExport ViewProviderFemPostObject : public Gui::ViewProviderDocumentObject,
                                               public Base::Observer<int>
{
    PROPERTY_HEADER(FemGui::ViewProviderFemPostObject);

public:
    /// constructor.
    ViewProviderFemPostObject();

    /// destructor.
    ~ViewProviderFemPostObject();

    App::PropertyEnumeration            Field;
    App::PropertyEnumeration            VectorMode;
    App::PropertyPercent                Transparency;

    void attach(App::DocumentObject *pcObject);
    void setDisplayMode(const char* ModeName);
    std::vector<std::string> getDisplayModes() const;
    void updateData(const App::Property*);
    void onChanged(const App::Property* prop);

    //edit handling
    virtual bool doubleClicked(void);
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);

    virtual void hide(void);
    virtual void show(void);

    virtual SoSeparator* getFrontRoot(void) const;

    //observer for the color bar
    virtual void OnChange(Base::Subject< int >& rCaller, int rcReason);

    // handling when object is deleted
    virtual bool onDelete(const std::vector<std::string>&);
    virtual bool canDelete(App::DocumentObject* obj) const;

      /** @name Selection handling
      * This group of methods do the selection handling.
      * Here you can define how the selection for your ViewProvider
      * works.
     */
    //@{
//     /// indicates if the ViewProvider use the new Selection model
//     virtual bool useNewSelectionModel(void) const {return true;}
//     /// return a hit element to the selection path or 0
//     virtual std::string getElement(const SoDetail*) const;
//     virtual SoDetail* getDetail(const char*) const;
//     /// return the highlight lines for a given element or the whole shape
//     virtual std::vector<Base::Vector3d> getSelectionShape(const char* Element) const;
//     //@}

protected:
    virtual void setupTaskDialog(TaskDlgPost* dlg);
    bool setupPipeline();
    void updateVtk();
    void setRangeOfColorBar(double min, double max);

    SoCoordinate3*              m_coordinates;
    SoIndexedPointSet*          m_markers;
    SoIndexedLineSet*           m_lines;
    SoIndexedFaceSet*           m_faces;
    SoIndexedTriangleStripSet*  m_triangleStrips;
    SoMaterial*                 m_material;
    SoMaterialBinding*          m_materialBinding;
    SoShapeHints*               m_shapeHints;
    SoNormalBinding*            m_normalBinding;
    SoNormal*                   m_normals;
    SoDrawStyle*                m_drawStyle;
    SoSeparator*                m_seperator;
    Gui::SoFCColorBar*          m_colorBar;
    SoSeparator*                m_colorRoot;
    SoDrawStyle*                m_colorStyle;

    vtkSmartPointer<vtkPolyDataAlgorithm>       m_currentAlgorithm;
    vtkSmartPointer<vtkGeometryFilter>          m_surface;
    vtkSmartPointer<vtkAppendPolyData>          m_surfaceEdges;
    vtkSmartPointer<vtkOutlineCornerFilter>     m_outline;
    vtkSmartPointer<vtkExtractEdges>            m_wireframe, m_wireframeSurface;
    vtkSmartPointer<vtkVertexGlyphFilter>       m_points, m_pointsSurface;

private:
    void updateProperties();
    void update3D();
    void WritePointData(vtkPoints *points, vtkDataArray *normals,
                        vtkDataArray *tcoords);
    void WriteColorData(bool ResetColorBarRange);
    void WriteTransparency();

    App::Enumeration m_coloringEnum, m_vectorEnum;
    bool m_blockPropertyChanges;
};

} //namespace FemGui


#endif // FEM_VIEWPROVIDERFEMPOSTOBJECT_H
