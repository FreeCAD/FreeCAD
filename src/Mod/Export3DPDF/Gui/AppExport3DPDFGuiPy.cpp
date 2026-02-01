/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Developers                                 *
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

#include "PreCompiled.h"

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <App/Material.h>
#include <App/ComplexGeoData.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Vector3D.h>

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderGeometryObject.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PropertyTopoShape.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/Gui/ViewProviderExt.h>

#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/MeshProperties.h>
#include <Mod/Mesh/App/MeshFeature.h>

#include "../App/Export3DPDFCore.h"

#include <Standard_Failure.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <cmath>
#include <algorithm>
#include <set>

#include <App/PropertyLinks.h>

namespace Export3DPDFGui
{

// Helper function to calculate adaptive tessellation tolerance based on bounding box
static double calculateTessellationTolerance(const Part::TopoShape& shape)
{
    Bnd_Box bounds;
    BRepBndLib::Add(shape.getShape(), bounds);

    if (bounds.IsVoid()) {
        return 0.01;
    }

    double xMin, yMin, zMin, xMax, yMax, zMax;
    bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);

    double diagonal = std::sqrt(
        (xMax - xMin) * (xMax - xMin) +
        (yMax - yMin) * (yMax - yMin) +
        (zMax - zMin) * (zMax - zMin)
    );

    // Use 0.1% of diagonal as tolerance, clamped between 0.001 and 0.1
    double tolerance = diagonal * 0.001;
    return std::max(0.001, std::min(0.1, tolerance));
}

// Helper function to filter objects - avoid exporting intermediate PartDesign features
// when their parent Body is also being exported (prevents overlapping geometry)
static std::vector<App::DocumentObject*> filterExportObjects(const std::vector<App::DocumentObject*>& objects)
{
    std::vector<App::DocumentObject*> filtered;
    std::set<App::DocumentObject*> bodiesToExport;

    // First pass: identify all Bodies in the selection
    for (const auto& obj : objects) {
        if (obj && (obj->isDerivedFrom(Base::Type::fromName("PartDesign::Body")) ||
                    obj->isDerivedFrom(Base::Type::fromName("Part::BodyBase")))) {
            bodiesToExport.insert(obj);
        }
    }

    // Second pass: filter out features that belong to a Body we're already exporting
    for (const auto& obj : objects) {
        if (!obj) continue;

        // Check if this object is a feature inside a Body
        App::DocumentObject* parentBody = nullptr;

        // Check for _Body property (PartDesign features have this)
        App::Property* bodyProp = obj->getPropertyByName("_Body");
        if (bodyProp) {
            auto* linkProp = dynamic_cast<App::PropertyLink*>(bodyProp);
            if (linkProp) {
                parentBody = linkProp->getValue();
            }
        }

        // Also check InList for parent Body
        if (!parentBody) {
            for (auto* parent : obj->getInList()) {
                if (parent && (parent->isDerivedFrom(Base::Type::fromName("PartDesign::Body")) ||
                               parent->isDerivedFrom(Base::Type::fromName("Part::BodyBase")))) {
                    parentBody = parent;
                    break;
                }
            }
        }

        // If this object's parent Body is in our export set, skip this object
        if (parentBody && bodiesToExport.find(parentBody) != bodiesToExport.end()) {
            Base::Console().log("Skipping '%s' (parent Body '%s' is being exported)\n",
                               obj->getNameInDocument(),
                               parentBody->getNameInDocument());
            continue;
        }

        filtered.push_back(obj);
    }

    return filtered;
}

class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Export3DPDFGui")
    {
        add_varargs_method("export", &Module::exporter,
            "export(objects, filename) -- Export objects to 3D PDF file.\n"
            "objects: list of document objects to export\n"
            "filename: path to the output PDF file");
        initialize("This module is the Export3DPDFGui module for 3D PDF export GUI functionality.");
    }

private:
    Py::Object exporter(const Py::Tuple& args)
    {
        PyObject* object = nullptr;
        char* filename = nullptr;

        if (!PyArg_ParseTuple(args.ptr(), "Oet", &object, "utf-8", &filename)) {
            throw Py::Exception();
        }

        std::string pdfPath(filename);
        PyMem_Free(filename);

        // Collect objects from the Python list
        std::vector<App::DocumentObject*> objects;
        Py::Sequence list(object);

        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                if (obj) {
                    objects.push_back(obj);
                }
            }
        }

        if (objects.empty()) {
            throw Py::RuntimeError("No valid objects to export");
        }

        // Filter objects to avoid exporting overlapping geometry from PartDesign features
        std::vector<App::DocumentObject*> filteredObjects = filterExportObjects(objects);

        if (filteredObjects.empty()) {
            throw Py::RuntimeError("No valid objects to export after filtering");
        }

        // Process objects - extract geometry from Part shapes or Mesh objects
        std::vector<Export3DPDF::TessellationData> tessData;

        for (const auto& obj : filteredObjects) {
            if (!obj) continue;

            try {
                Export3DPDF::TessellationData tessObj;
                tessObj.name = obj->getNameInDocument();

                std::vector<Base::Vector3d> points;
                std::vector<Data::ComplexGeoData::Facet> facets;
                bool geometryExtracted = false;

                // Try Part::PropertyPartShape first
                auto* shapeProp = dynamic_cast<Part::PropertyPartShape*>(obj->getPropertyByName("Shape"));
                if (shapeProp) {
                    const Part::TopoShape& topoShape = shapeProp->getShape();
                    if (!topoShape.isNull()) {
                        // Calculate adaptive tolerance based on object size
                        double tolerance = calculateTessellationTolerance(topoShape);
                        topoShape.getFaces(points, facets, tolerance);
                        geometryExtracted = true;
                    }
                }

                // If no Part shape, try Mesh::PropertyMeshKernel
                if (!geometryExtracted) {
                    auto* meshProp = dynamic_cast<Mesh::PropertyMeshKernel*>(obj->getPropertyByName("Mesh"));
                    if (meshProp) {
                        const Mesh::MeshObject& meshObject = meshProp->getValue();
                        if (meshObject.countFacets() > 0) {
                            // Mesh is already tessellated, just extract data
                            // Accuracy parameter is ignored for mesh objects
                            meshObject.getFaces(points, facets, 0.0);
                            geometryExtracted = true;
                        }
                    }
                }

                // Skip if no geometry could be extracted (non-geometric objects like Groups, Dimensions, etc.)
                if (!geometryExtracted) {
                    continue;
                }

                // Skip 2D objects that produce no faces (Lines, Sketches, Wires, etc.)
                if (points.empty() || facets.empty()) {
                    continue;
                }

                // Convert points to flat vertex array
                tessObj.vertices.reserve(points.size() * 3);
                for (const auto& pt : points) {
                    tessObj.vertices.push_back(pt.x);
                    tessObj.vertices.push_back(pt.y);
                    tessObj.vertices.push_back(pt.z);
                }

                // Convert facets to flat triangle index array
                tessObj.triangles.reserve(facets.size() * 3);
                for (const auto& f : facets) {
                    tessObj.triangles.push_back(static_cast<int>(f.I1));
                    tessObj.triangles.push_back(static_cast<int>(f.I2));
                    tessObj.triangles.push_back(static_cast<int>(f.I3));
                }

                // Get material properties from ViewProvider
                Gui::Document* guiDoc = Gui::Application::Instance->getDocument(obj->getDocument());
                if (guiDoc) {
                    Gui::ViewProvider* vp = guiDoc->getViewProvider(obj);
                    if (vp) {
                        auto* partVP = dynamic_cast<PartGui::ViewProviderPartExt*>(vp);
                        if (partVP) {
                            const auto& materials = partVP->ShapeAppearance.getValues();
                            if (!materials.empty()) {
                                const App::Material& mat = materials[0];
                                tessObj.material.diffuseColor[0] = mat.diffuseColor.r;
                                tessObj.material.diffuseColor[1] = mat.diffuseColor.g;
                                tessObj.material.diffuseColor[2] = mat.diffuseColor.b;
                                tessObj.material.diffuseColor[3] = 1.0 - mat.transparency;
                                tessObj.material.ambientColor[0] = mat.ambientColor.r;
                                tessObj.material.ambientColor[1] = mat.ambientColor.g;
                                tessObj.material.ambientColor[2] = mat.ambientColor.b;
                                tessObj.material.ambientColor[3] = 1.0;
                                tessObj.material.specularColor[0] = mat.specularColor.r;
                                tessObj.material.specularColor[1] = mat.specularColor.g;
                                tessObj.material.specularColor[2] = mat.specularColor.b;
                                tessObj.material.specularColor[3] = 1.0;
                                tessObj.material.emissiveColor[0] = mat.emissiveColor.r;
                                tessObj.material.emissiveColor[1] = mat.emissiveColor.g;
                                tessObj.material.emissiveColor[2] = mat.emissiveColor.b;
                                tessObj.material.emissiveColor[3] = 1.0;
                                tessObj.material.shininess = mat.shininess;
                                tessObj.material.transparency = mat.transparency;
                                tessObj.material.name = "Material";
                            }
                        } else {
                            // Fallback for other geometry view providers (including Mesh)
                            auto* geoVP = dynamic_cast<Gui::ViewProviderGeometryObject*>(vp);
                            if (geoVP) {
                                const auto& materials = geoVP->ShapeAppearance.getValues();
                                if (!materials.empty()) {
                                    const App::Material& mat = materials[0];
                                    tessObj.material.diffuseColor[0] = mat.diffuseColor.r;
                                    tessObj.material.diffuseColor[1] = mat.diffuseColor.g;
                                    tessObj.material.diffuseColor[2] = mat.diffuseColor.b;
                                    tessObj.material.diffuseColor[3] = 1.0 - mat.transparency;
                                    tessObj.material.shininess = mat.shininess;
                                    tessObj.material.transparency = mat.transparency;
                                }
                            }
                        }
                    }
                }

                tessData.push_back(tessObj);

            } catch (const Standard_Failure& e) {
                Base::Console().error("OpenCASCADE error processing '%s': %s\n",
                    obj->getNameInDocument(), e.GetMessageString());
            } catch (const Base::Exception& e) {
                Base::Console().error("FreeCAD error processing '%s': %s\n",
                    obj->getNameInDocument(), e.what());
            } catch (const std::exception& e) {
                Base::Console().error("Error processing '%s': %s\n",
                    obj->getNameInDocument(), e.what());
            }
        }

        if (tessData.empty()) {
            throw Py::RuntimeError("No valid geometry found to export");
        }

        // Set up export settings with reasonable defaults
        Export3DPDF::PDFExportSettings settings;
        settings.page.widthPoints = 800.0;
        settings.page.heightPoints = 800.0;
        // Center the 3D view on the page
        settings.activeView.x = 800.0 / Export3DPDF::MM_TO_POINTS / 2.0;
        settings.activeView.y = 800.0 / Export3DPDF::MM_TO_POINTS / 2.0;
        settings.activeView.scale = 1.0;
        settings.activeView.width = 800.0 / Export3DPDF::MM_TO_POINTS * 0.8;
        settings.activeView.height = 800.0 / Export3DPDF::MM_TO_POINTS * 0.8;

        // Export to PDF
        bool success = Export3DPDF::Export3DPDFCore::exportToPDF(tessData, pdfPath, settings);

        if (success) {
            Base::Console().message("3D PDF exported successfully: %s\n", pdfPath.c_str());
        } else {
            throw Py::RuntimeError("Failed to export 3D PDF. Check console for details.");
        }

        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace Export3DPDFGui
