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
#include <vtkDataObject.h>
#include <vtkExtractEdges.h>
#include <vtkGeometryFilter.h>
#include <vtkOutlineCornerFilter.h>
#include <vtkSmartPointer.h>
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

namespace Gui
{
class SelectionChanges;
class SoFCColorBar;
}  // namespace Gui

namespace FemGui
{

class TaskDlgPost;

class FemGuiExport ViewProviderFemPostObject: public Gui::ViewProviderDocumentObject,
                                              public Base::Observer<int>
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostObject);

public:
    /// constructor.
    ViewProviderFemPostObject();

    /// destructor.
    ~ViewProviderFemPostObject() override;

    App::PropertyEnumeration Field;
    App::PropertyEnumeration VectorMode;
    App::PropertyPercent Transparency;

    void attach(App::DocumentObject* pcObject) override;
    void setDisplayMode(const char* ModeName) override;
    std::vector<std::string> getDisplayModes() const override;
    void updateData(const App::Property*) override;
    void onChanged(const App::Property* prop) override;

    // edit handling
    bool doubleClicked() override;
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;

    void hide() override;
    void show() override;

    SoSeparator* getFrontRoot() const override;

    // observer for the color bar
    void OnChange(Base::Subject<int>& rCaller, int rcReason) override;
    // update color bar
    void updateMaterial();

    // handling when object is deleted
    bool onDelete(const std::vector<std::string>&) override;
    bool canDelete(App::DocumentObject* obj) const override;
    virtual void onSelectionChanged(const Gui::SelectionChanges& sel);

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

    SoCoordinate3* m_coordinates;
    SoIndexedPointSet* m_markers;
    SoIndexedLineSet* m_lines;
    SoIndexedFaceSet* m_faces;
    SoIndexedTriangleStripSet* m_triangleStrips;
    SoMaterial* m_material;
    SoMaterialBinding* m_materialBinding;
    SoShapeHints* m_shapeHints;
    SoNormalBinding* m_normalBinding;
    SoNormal* m_normals;
    SoDrawStyle* m_drawStyle;
    SoSeparator* m_separator;
    Gui::SoFCColorBar* m_colorBar;
    SoSeparator* m_colorRoot;
    SoDrawStyle* m_colorStyle;

    vtkSmartPointer<vtkPolyDataAlgorithm> m_currentAlgorithm;
    vtkSmartPointer<vtkGeometryFilter> m_surface;
    vtkSmartPointer<vtkAppendPolyData> m_surfaceEdges;
    vtkSmartPointer<vtkOutlineCornerFilter> m_outline;
    vtkSmartPointer<vtkExtractEdges> m_wireframe, m_wireframeSurface;
    vtkSmartPointer<vtkVertexGlyphFilter> m_points, m_pointsSurface;

private:
    void filterArtifacts(vtkDataSet* data);
    void updateProperties();
    void update3D();
    void WritePointData(vtkPoints* points, vtkDataArray* normals, vtkDataArray* tcoords);
    void WriteColorData(bool ResetColorBarRange);
    void WriteTransparency();
    void addAbsoluteField(vtkDataSet* dset, std::string FieldName);

    App::Enumeration m_coloringEnum, m_vectorEnum;
    bool m_blockPropertyChanges {false};
};

}  // namespace FemGui


#endif  // FEM_VIEWPROVIDERFEMPOSTOBJECT_H
