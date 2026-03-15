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

#ifndef _PreComp_
# include <QApplication>
# include <QFileInfo>
# include <QMessageBox>
# include <QDir>
# include <QCoreApplication>
# include <QThread>
# include <algorithm>
# include <cmath>
# include <fstream>
# include <sstream>
# include <set>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <Base/Console.h>
#include <Base/Color.h>
#include <Base/FileInfo.h>
#include <Base/Tools.h>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderGeometryObject.h>
#include <Gui/Selection/Selection.h>

// Part module includes for direct tessellation
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PropertyTopoShape.h>
#include <Mod/Part/App/TopoShape.h>

#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/MeshProperties.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Part/Gui/ViewProviderExt.h>
#include <App/ComplexGeoData.h>
#include <App/Material.h>

namespace TechDraw {
    class DrawPage;
    class DrawViewImage;
}

namespace TechDrawGui {
    class ViewProviderImage;
}

#include "Command.h"
#include "../App/Export3DPDFCore.h"

// OpenCASCADE includes
#include <Standard_Failure.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

FC_LOG_LEVEL_INIT("Command", false)

using namespace Gui;

// Helper struct to hold data for a single ActiveView with 3D PDF export enabled
struct ActiveView3DExportData {
    App::DocumentObject* view = nullptr;
    Gui::ViewProvider* viewProvider = nullptr;

    // Background settings for this view's 3D annotation
    double backgroundR = 1.0;
    double backgroundG = 1.0;
    double backgroundB = 1.0;

    // ActiveView position/scale properties
    double x = 0.0;
    double y = 0.0;
    double scale = 1.0;
    double width = 100.0;
    double height = 100.0;

    // Source objects for this view's 3D export
    std::vector<App::DocumentObject*> sourceObjects;
};

// Helper struct to hold TechDraw page and all ActiveViews with 3D PDF export
struct TechDraw3DPDFExportData {
    App::DocumentObject* page = nullptr;

    // Page dimensions
    double pageWidthPoints = 800.0;
    double pageHeightPoints = 800.0;

    // All ActiveViews with 3D PDF export enabled
    std::vector<ActiveView3DExportData> activeViews;

    bool found() const { return page != nullptr && !activeViews.empty(); }
};

// Helper function to find TechDraw page with ActiveViews that have Enable3DPDFExport=true
static TechDraw3DPDFExportData findTechDraw3DPDFExportData(App::Document* doc)
{
    TechDraw3DPDFExportData result;

    if (!doc) {
        return result;
    }

    std::vector<App::DocumentObject*> pages = doc->getObjectsOfType(Base::Type::fromName("TechDraw::DrawPage"));

    for (auto* pageObj : pages) {
        if (!pageObj || !pageObj->isDerivedFrom(Base::Type::fromName("TechDraw::DrawPage"))) {
            continue;
        }

        App::Property* viewsProp = pageObj->getPropertyByName("Views");
        if (!viewsProp) {
            continue;
        }

        App::PropertyLinkList* viewsList = dynamic_cast<App::PropertyLinkList*>(viewsProp);
        if (!viewsList) {
            continue;
        }

        const std::vector<App::DocumentObject*>& views = viewsList->getValues();

        // Collect all ActiveViews with 3D PDF export enabled on this page
        std::vector<ActiveView3DExportData> pageActiveViews;

        for (auto* view : views) {
            if (!view || !view->isDerivedFrom(Base::Type::fromName("TechDraw::DrawViewImage"))) {
                continue;
            }

            std::string viewName = view->getNameInDocument();
            if (viewName.find("ActiveView") == std::string::npos) {
                continue;
            }

            Gui::Document* guiDoc = Gui::Application::Instance->getDocument(view->getDocument());
            if (!guiDoc) {
                continue;
            }

            Gui::ViewProvider* vp = guiDoc->getViewProvider(view);
            if (!vp) {
                continue;
            }

            App::Property* enable3DProp = vp->getPropertyByName("Enable3DPDFExport");
            if (!enable3DProp) {
                continue;
            }

            App::PropertyBool* enableBool = dynamic_cast<App::PropertyBool*>(enable3DProp);
            if (!enableBool || !enableBool->getValue()) {
                continue;
            }

            // Found a valid ActiveView with 3D PDF export enabled
            ActiveView3DExportData avData;
            avData.view = view;
            avData.viewProvider = vp;

            // Extract background settings for this view
            try {
                App::Property* noBackgroundProp = vp->getPropertyByName("NoBackground");
                App::Property* solidBackgroundProp = vp->getPropertyByName("SolidBackground");
                App::Property* backgroundColorProp = vp->getPropertyByName("BackgroundColor");

                bool noBackground = false;
                bool solidBackground = false;

                if (noBackgroundProp) {
                    App::PropertyBool* noBgBool = dynamic_cast<App::PropertyBool*>(noBackgroundProp);
                    if (noBgBool) {
                        noBackground = noBgBool->getValue();
                    }
                }

                if (solidBackgroundProp) {
                    App::PropertyBool* solidBgBool = dynamic_cast<App::PropertyBool*>(solidBackgroundProp);
                    if (solidBgBool) {
                        solidBackground = solidBgBool->getValue();
                    }
                }

                if (noBackground) {
                    avData.backgroundR = 1.0;
                    avData.backgroundG = 1.0;
                    avData.backgroundB = 1.0;
                } else if (solidBackground && backgroundColorProp) {
                    App::PropertyColor* colorProp = dynamic_cast<App::PropertyColor*>(backgroundColorProp);
                    if (colorProp) {
                        Base::Color bgColor = colorProp->getValue();
                        avData.backgroundR = bgColor.r;
                        avData.backgroundG = bgColor.g;
                        avData.backgroundB = bgColor.b;
                    }
                }
            } catch (const std::exception& e) {
                Base::Console().warning("Failed to get background properties: %s\n", e.what());
            }

            // Extract source objects for this view
            App::Property* sourceProp = view->getPropertyByName("Source");
            App::Property* xsourceProp = view->getPropertyByName("XSource");

            if (sourceProp) {
                App::PropertyLinkList* sourceList = dynamic_cast<App::PropertyLinkList*>(sourceProp);
                if (sourceList) {
                    const std::vector<App::DocumentObject*>& sources = sourceList->getValues();
                    avData.sourceObjects.insert(avData.sourceObjects.end(), sources.begin(), sources.end());
                }
            }

            if (xsourceProp) {
                App::PropertyLinkList* xsourceList = dynamic_cast<App::PropertyLinkList*>(xsourceProp);
                if (xsourceList) {
                    const std::vector<App::DocumentObject*>& xsources = xsourceList->getValues();
                    avData.sourceObjects.insert(avData.sourceObjects.end(), xsources.begin(), xsources.end());
                }
            }

            // Extract ActiveView position/scale properties
            try {
                App::Property* xProp = view->getPropertyByName("X");
                App::Property* yProp = view->getPropertyByName("Y");
                App::Property* scaleProp = view->getPropertyByName("Scale");
                App::Property* widthProp = view->getPropertyByName("Width");
                App::Property* heightProp = view->getPropertyByName("Height");

                if (xProp) {
                    App::PropertyDistance* xDist = dynamic_cast<App::PropertyDistance*>(xProp);
                    if (xDist) {
                        avData.x = xDist->getValue();
                    }
                }

                if (yProp) {
                    App::PropertyDistance* yDist = dynamic_cast<App::PropertyDistance*>(yProp);
                    if (yDist) {
                        avData.y = yDist->getValue();
                    }
                }

                if (scaleProp) {
                    App::PropertyFloatConstraint* scaleFloat = dynamic_cast<App::PropertyFloatConstraint*>(scaleProp);
                    if (scaleFloat) {
                        avData.scale = scaleFloat->getValue();
                    }
                }

                if (widthProp) {
                    App::PropertyFloat* widthFloat = dynamic_cast<App::PropertyFloat*>(widthProp);
                    if (widthFloat) {
                        avData.width = widthFloat->getValue();
                    }
                }

                if (heightProp) {
                    App::PropertyFloat* heightFloat = dynamic_cast<App::PropertyFloat*>(heightProp);
                    if (heightFloat) {
                        avData.height = heightFloat->getValue();
                    }
                }
            } catch (const std::exception& e) {
                Base::Console().warning("Failed to get ActiveView properties: %s\n", e.what());
            }

            // Add this ActiveView to the collection (don't return early!)
            pageActiveViews.push_back(avData);
        }

        // If we found any ActiveViews with 3D export on this page, use this page
        if (!pageActiveViews.empty()) {
            result.page = pageObj;

            // Extract page dimensions
            double pageWidthMM = 297.0;  // Default A4
            double pageHeightMM = 210.0;

            App::Property* widthProp = pageObj->getPropertyByName("PageWidth");
            App::Property* heightProp = pageObj->getPropertyByName("PageHeight");

            if (widthProp) {
                auto* width = dynamic_cast<App::PropertyLength*>(widthProp);
                if (width) {
                    pageWidthMM = width->getValue();
                }
            }

            if (heightProp) {
                auto* height = dynamic_cast<App::PropertyLength*>(heightProp);
                if (height) {
                    pageHeightMM = height->getValue();
                }
            }

            result.pageWidthPoints = pageWidthMM * Export3DPDF::MM_TO_POINTS;
            result.pageHeightPoints = pageHeightMM * Export3DPDF::MM_TO_POINTS;

            result.activeViews = std::move(pageActiveViews);

            // Return after finding the first page with enabled ActiveViews
            return result;
        }
    }

    return result;
}

StdCmdPrint3dPdf::StdCmdPrint3dPdf()
  :Command("Std_Print3dPdf")
{
    sGroup        = "File";
    sMenuText     = QT_TR_NOOP("Export &3D PDF...");
    sToolTipText  = QT_TR_NOOP("Exports the document as 3D PDF");
    sWhatsThis    = "Std_Print3dPdf";
    sStatusTip    = sToolTipText;
    sPixmap       = "Std_PrintPdf";
    eType         = 0;
}

// Helper function to calculate adaptive tessellation tolerance based on bounding box
static double calculateTessellationTolerance(const Part::TopoShape& shape)
{
    // Get bounding box diagonal to determine appropriate tolerance
    Bnd_Box bounds;
    BRepBndLib::Add(shape.getShape(), bounds);

    if (bounds.IsVoid()) {
        return 0.01;  // Default for empty/invalid shapes
    }

    double xMin, yMin, zMin, xMax, yMax, zMax;
    bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);

    double diagonal = std::sqrt(
        (xMax - xMin) * (xMax - xMin) +
        (yMax - yMin) * (yMax - yMin) +
        (zMax - zMin) * (zMax - zMin)
    );

    // Use 0.1% of diagonal as tolerance, clamped between 0.001 and 0.1
    // This gives finer tessellation for smaller objects
    double tolerance = diagonal * 0.001;
    return std::max(0.001, std::min(0.1, tolerance));
}

// Helper function to filter objects - avoid exporting intermediate PartDesign features
// when their parent Body is also being exported (prevents overlapping geometry)
static std::vector<App::DocumentObject*> filterExportObjects(const std::vector<App::DocumentObject*>& objects)
{
    std::vector<App::DocumentObject*> filtered;
    std::set<App::DocumentObject*> objectSet(objects.begin(), objects.end());
    std::set<App::DocumentObject*> bodiesToExport;

    // First pass: identify all Bodies in the selection
    for (const auto& obj : objects) {
        if (obj && obj->isDerivedFrom(Base::Type::fromName("PartDesign::Body"))) {
            bodiesToExport.insert(obj);
        }
        // Also check for Part::BodyBase (parent class)
        if (obj && obj->isDerivedFrom(Base::Type::fromName("Part::BodyBase"))) {
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
        // (the Body's shape already includes this feature's result)
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

// Helper function to process objects - extract geometry from Part shapes or Mesh objects
static std::vector<Export3DPDF::TessellationData> tessellateObjects(const std::vector<App::DocumentObject*>& objects)
{
    std::vector<Export3DPDF::TessellationData> tessData;

    for (const auto& obj : objects) {
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

    return tessData;
}

void StdCmdPrint3dPdf::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    std::vector<App::DocumentObject*> selection;

    // Default values for direct export (no TechDraw)
    double pageWidthPoints = 800.0;
    double pageHeightPoints = 800.0;

    // Try to find TechDraw page with 3D PDF export enabled
    TechDraw3DPDFExportData exportData = findTechDraw3DPDFExportData(activeDoc);

    if (exportData.found()) {
        // Use page dimensions from TechDraw
        pageWidthPoints = exportData.pageWidthPoints;
        pageHeightPoints = exportData.pageHeightPoints;

        // Collect all source objects from all ActiveViews
        for (const auto& av : exportData.activeViews) {
            selection.insert(selection.end(), av.sourceObjects.begin(), av.sourceObjects.end());
        }
    } else if (activeDoc) {
        // No TechDraw page with 3D PDF export, use current selection
        selection = Gui::Selection().getObjectsOfType(App::DocumentObject::getClassTypeId());
    }

    if (selection.empty()) {
        QMessageBox::warning(getMainWindow(),
            QCoreApplication::translate("StdCmdPrint3dPdf", "No selection"),
            QCoreApplication::translate("StdCmdPrint3dPdf", "Select the objects to export before choosing 3D PDF Export."));
        return;
    }
    
    QStringList filterList;
    filterList << QObject::tr("3D PDF (*.pdf)");
    filterList << QObject::tr("All Files (*.*)");
    QString formatList = filterList.join(QLatin1String(";;"));
    QString selectedFilter;
    
    QString defaultFilename;
    QString docFilename = QString::fromUtf8(App::GetApplication().getActiveDocument()->getFileName());
    QString defaultPath;
    QString baseFilename;
    
    if (!docFilename.isEmpty()) {
        QFileInfo fi(docFilename);
        defaultPath = fi.path();
        baseFilename = fi.completeBaseName() + QString::fromLatin1("_3D");
    } else {
        defaultPath = Gui::FileDialog::getWorkingDirectory();
        QString docName = QString::fromStdString(App::GetApplication().getActiveDocument()->Label.getStrValue());
        baseFilename = docName + QString::fromLatin1("_3D");
    }
    
    if (selection.size() <= 3) {
        QStringList objectNames;
        for (const auto& obj : selection) {
            objectNames << QString::fromStdString(obj->Label.getStrValue());
        }
        if (!objectNames.isEmpty()) {
            baseFilename += QString::fromLatin1("-") + objectNames.join(QString::fromLatin1("-"));
        }
    }
    
    QString invalidCharacters = QLatin1String("\\?%*:|\"<>");
    for (const auto &c : invalidCharacters)
        baseFilename.replace(c, QLatin1String("_"));
    
    defaultFilename = QDir(defaultPath).filePath(baseFilename + QString::fromLatin1(".pdf"));
    
    QString fileName = FileDialog::getSaveFileName(getMainWindow(),
        QObject::tr("Export 3D PDF"), defaultFilename, formatList, &selectedFilter);
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFileInfo fileInfo(fileName);
    std::string outputPath = fileInfo.path().toStdString() + "/" + fileInfo.completeBaseName().toStdString();
    std::string pdfPath = outputPath + ".pdf";

    // Use the page found earlier by the helper function
    App::DocumentObject* currentPage = exportData.page;
    std::string backgroundImagePath;

    // Render TechDraw page background if we have a page
    if (currentPage) {
        backgroundImagePath = outputPath + "_background.png";

        try {
            std::string pythonCmd =
                "import FreeCAD as App\n"
                "import FreeCADGui as Gui\n"
                "try:\n"
                "    import TechDrawGui\n"
                "except ImportError as e:\n"
                "    App.Console.PrintError('Failed to import TechDrawGui: {}\\n'.format(str(e)))\n"
                "    raise\n"
                "try:\n"
                "    from PySide6 import QtCore, QtGui\n"
                "except ImportError:\n"
                "    try:\n"
                "        from PySide2 import QtCore, QtGui\n"
                "    except ImportError:\n"
                "        try:\n"
                "            from PyQt5 import QtCore, QtGui\n"
                "        except ImportError:\n"
                "            App.Console.PrintError('No Qt bindings found\\n')\n"
                "            raise ImportError('No Qt bindings available')\n"
                "try:\n"
                "    page_obj = App.getDocument('" + std::string(currentPage->getDocument()->getName()) + "').getObject('" + std::string(currentPage->getNameInDocument()) + "')\n"
                "    if page_obj:\n"
                "        qgs_page = TechDrawGui.getSceneForPage(page_obj)\n"
                "        if qgs_page:\n"
                "            dpi = 150.0\n"
                "            page_width_mm = " + std::to_string(pageWidthPoints / Export3DPDF::MM_TO_POINTS) + "\n"
                "            page_height_mm = " + std::to_string(pageHeightPoints / Export3DPDF::MM_TO_POINTS) + "\n"
                "            dpmm = dpi / 25.4\n"
                "            image_width = int(page_width_mm * dpmm)\n"
                "            image_height = int(page_height_mm * dpmm)\n"
                "            image = QtGui.QImage(image_width, image_height, QtGui.QImage.Format_RGB32)\n"
                "            image.fill(QtCore.Qt.white)\n"
                "            painter = QtGui.QPainter(image)\n"
                "            painter.setRenderHint(QtGui.QPainter.Antialiasing, True)\n"
                "            painter.setRenderHint(QtGui.QPainter.TextAntialiasing, True)\n"
                "            painter.setRenderHint(QtGui.QPainter.SmoothPixmapTransform, True)\n"
                "            rez_factor = 10.0\n"
                "            source_width = page_width_mm * rez_factor\n"
                "            source_height = page_height_mm * rez_factor\n"
                "            source_rect = QtCore.QRectF(0.0, -source_height, source_width, source_height)\n"
                "            target_rect = QtCore.QRectF(0, 0, image_width, image_height)\n"
                "            qgs_page.render(painter, target_rect, source_rect)\n"
                "            painter.end()\n"
                "            image.save('" + backgroundImagePath + "', 'PNG')\n"
                "except Exception as e:\n"
                "    App.Console.PrintError('Error rendering background: {}\\n'.format(str(e)))";

            doCommand(Command::Doc, pythonCmd.c_str());

            QCoreApplication::processEvents();
            QThread::msleep(500);

            QFileInfo bgFile(QString::fromStdString(backgroundImagePath));
            if (!bgFile.exists() || bgFile.size() == 0) {
                backgroundImagePath.clear();
            }

        } catch (const std::exception& e) {
            backgroundImagePath.clear();
        }
    }

    bool success = false;

    // Export based on whether we have TechDraw ActiveViews or direct selection
    if (exportData.found() && !exportData.activeViews.empty()) {
        // TechDraw hybrid export with multiple 3D regions
        std::vector<Export3DPDF::PDF3DRegion> regions;

        for (const auto& av : exportData.activeViews) {
            Export3DPDF::PDF3DRegion region;

            // Tessellate this view's source objects
            region.tessellationData = tessellateObjects(av.sourceObjects);

            if (region.tessellationData.empty()) {
                Base::Console().warning("ActiveView has no valid geometry, skipping\n");
                continue;
            }

            // Set view position and size
            region.viewSettings.x = av.x;
            region.viewSettings.y = av.y;
            region.viewSettings.scale = av.scale;
            region.viewSettings.width = av.width;
            region.viewSettings.height = av.height;

            // Set background color for 3D view
            region.background.r = av.backgroundR;
            region.background.g = av.backgroundG;
            region.background.b = av.backgroundB;

            // Use default camera settings
            // (could be extended to read per-view camera settings)

            regions.push_back(region);
        }

        if (regions.empty()) {
            QMessageBox::warning(getMainWindow(),
                QCoreApplication::translate("StdCmdPrint3dPdf", "No Data"),
                QCoreApplication::translate("StdCmdPrint3dPdf", "No valid geometry found in any ActiveView."));
            if (!backgroundImagePath.empty()) {
                Base::FileInfo(backgroundImagePath).deleteFile();
            }
            return;
        }

        Export3DPDF::PDFPageSettings pageSettings;
        pageSettings.widthPoints = pageWidthPoints;
        pageSettings.heightPoints = pageHeightPoints;

        Base::Console().message("Exporting %zu 3D regions to hybrid PDF\n", regions.size());

        success = Export3DPDF::Export3DPDFCore::exportToHybridPDFMultiRegion(
            regions, pdfPath, backgroundImagePath, pageSettings);

    } else {
        // Direct export (no TechDraw) - single 3D region
        // Filter objects to avoid exporting overlapping geometry from PartDesign features
        std::vector<App::DocumentObject*> filteredSelection = filterExportObjects(selection);
        std::vector<Export3DPDF::TessellationData> tessData = tessellateObjects(filteredSelection);

        if (tessData.empty()) {
            QMessageBox::warning(getMainWindow(),
                QCoreApplication::translate("StdCmdPrint3dPdf", "No Data"),
                QCoreApplication::translate("StdCmdPrint3dPdf", "No valid geometry found to export."));
            if (!backgroundImagePath.empty()) {
                Base::FileInfo(backgroundImagePath).deleteFile();
            }
            return;
        }

        Export3DPDF::PDFExportSettings settings;
        settings.page.widthPoints = pageWidthPoints;
        settings.page.heightPoints = pageHeightPoints;
        // Center the 3D view on the page for direct export
        settings.activeView.x = pageWidthPoints / Export3DPDF::MM_TO_POINTS / 2.0;
        settings.activeView.y = pageHeightPoints / Export3DPDF::MM_TO_POINTS / 2.0;
        settings.activeView.scale = 1.0;
        settings.activeView.width = pageWidthPoints / Export3DPDF::MM_TO_POINTS * 0.8;
        settings.activeView.height = pageHeightPoints / Export3DPDF::MM_TO_POINTS * 0.8;

        success = Export3DPDF::Export3DPDFCore::exportToPDF(tessData, pdfPath, settings);
    }

    // Clean up background image if it was created
    if (!backgroundImagePath.empty()) {
        Base::FileInfo(backgroundImagePath).deleteFile();
    }

    // Provide user feedback
    if (success) {
        Base::Console().message("3D PDF exported successfully: %s\n", pdfPath.c_str());
        QMessageBox::information(getMainWindow(),
            QCoreApplication::translate("StdCmdPrint3dPdf", "Export Successful"),
            QCoreApplication::translate("StdCmdPrint3dPdf", "3D PDF exported successfully to:\n%1")
                .arg(QString::fromStdString(pdfPath)));
    } else {
        Base::Console().error("Failed to export 3D PDF: %s\n", pdfPath.c_str());
        QMessageBox::critical(getMainWindow(),
            QCoreApplication::translate("StdCmdPrint3dPdf", "Export Failed"),
            QCoreApplication::translate("StdCmdPrint3dPdf", "Failed to export 3D PDF.\nCheck the console for details."));
    }
}

bool StdCmdPrint3dPdf::isActive()
{
    return (getActiveGuiDocument() ? true : false);
} 