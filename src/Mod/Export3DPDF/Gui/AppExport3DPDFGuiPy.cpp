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

#include "../App/Export3DPDFCore.h"

#include <Standard_Failure.hxx>

namespace Export3DPDFGui
{

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

        // Tessellate objects
        std::vector<Export3DPDF::TessellationData> tessData;

        for (const auto& obj : objects) {
            if (!obj) continue;

            try {
                auto* shapeProp = dynamic_cast<Part::PropertyPartShape*>(obj->getPropertyByName("Shape"));
                if (!shapeProp) {
                    Base::Console().warning("Object '%s' has no Shape property, skipping\n",
                                           obj->getNameInDocument());
                    continue;
                }

                const Part::TopoShape& topoShape = shapeProp->getShape();
                if (topoShape.isNull()) {
                    Base::Console().warning("Object '%s' has null shape, skipping\n",
                                           obj->getNameInDocument());
                    continue;
                }

                Export3DPDF::TessellationData tessObj;
                tessObj.name = obj->getNameInDocument();

                std::vector<Base::Vector3d> points;
                std::vector<Data::ComplexGeoData::Facet> facets;
                topoShape.getFaces(points, facets, 0.1);

                if (points.empty() || facets.empty()) {
                    Base::Console().warning("Object '%s' produced empty tessellation, skipping\n",
                                           obj->getNameInDocument());
                    continue;
                }

                tessObj.vertices.reserve(points.size() * 3);
                for (const auto& pt : points) {
                    tessObj.vertices.push_back(pt.x);
                    tessObj.vertices.push_back(pt.y);
                    tessObj.vertices.push_back(pt.z);
                }

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
