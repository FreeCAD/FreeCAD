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
# include <fstream>
# include <sstream>
#endif

#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>

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

// OpenCASCADE exception handling
#include <Standard_Failure.hxx>

FC_LOG_LEVEL_INIT("Command", false)

using namespace Gui;

// Helper struct to hold TechDraw page and ActiveView data
struct TechDraw3DPDFExportData {
    App::DocumentObject* page = nullptr;
    App::DocumentObject* activeView = nullptr;
    Gui::ViewProvider* activeViewVP = nullptr;

    // Page dimensions
    double pageWidthPoints = 800.0;
    double pageHeightPoints = 800.0;

    // Background settings
    double backgroundR = 1.0;
    double backgroundG = 1.0;
    double backgroundB = 1.0;

    // ActiveView properties
    double activeViewX = 0.0;
    double activeViewY = 0.0;
    double activeViewScale = 1.0;
    double activeViewWidth = 100.0;
    double activeViewHeight = 100.0;

    // Source objects for 3D export
    std::vector<App::DocumentObject*> sourceObjects;

    bool found() const { return page != nullptr && activeView != nullptr; }
};

// Helper function to find TechDraw page with ActiveView that has Enable3DPDFExport=true
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
            result.page = pageObj;
            result.activeView = view;
            result.activeViewVP = vp;

            // Extract page dimensions
            {
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

                result.pageWidthPoints = pageWidthMM * 2.834645669;
                result.pageHeightPoints = pageHeightMM * 2.834645669;
            }

            // Extract background settings
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
                    result.backgroundR = 1.0;
                    result.backgroundG = 1.0;
                    result.backgroundB = 1.0;
                } else if (solidBackground && backgroundColorProp) {
                    App::PropertyColor* colorProp = dynamic_cast<App::PropertyColor*>(backgroundColorProp);
                    if (colorProp) {
                        Base::Color bgColor = colorProp->getValue();
                        result.backgroundR = bgColor.r;
                        result.backgroundG = bgColor.g;
                        result.backgroundB = bgColor.b;
                    }
                }
            } catch (const std::exception& e) {
                Base::Console().warning("Failed to get background properties: %s\n", e.what());
            }

            // Extract source objects
            App::Property* sourceProp = view->getPropertyByName("Source");
            App::Property* xsourceProp = view->getPropertyByName("XSource");

            if (sourceProp) {
                App::PropertyLinkList* sourceList = dynamic_cast<App::PropertyLinkList*>(sourceProp);
                if (sourceList) {
                    const std::vector<App::DocumentObject*>& sources = sourceList->getValues();
                    result.sourceObjects.insert(result.sourceObjects.end(), sources.begin(), sources.end());
                }
            }

            if (xsourceProp) {
                App::PropertyLinkList* xsourceList = dynamic_cast<App::PropertyLinkList*>(xsourceProp);
                if (xsourceList) {
                    const std::vector<App::DocumentObject*>& xsources = xsourceList->getValues();
                    result.sourceObjects.insert(result.sourceObjects.end(), xsources.begin(), xsources.end());
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
                        result.activeViewX = xDist->getValue();
                    }
                }

                if (yProp) {
                    App::PropertyDistance* yDist = dynamic_cast<App::PropertyDistance*>(yProp);
                    if (yDist) {
                        result.activeViewY = yDist->getValue();
                    }
                }

                if (scaleProp) {
                    App::PropertyFloatConstraint* scaleFloat = dynamic_cast<App::PropertyFloatConstraint*>(scaleProp);
                    if (scaleFloat) {
                        result.activeViewScale = scaleFloat->getValue();
                    }
                }

                if (widthProp) {
                    App::PropertyFloat* widthFloat = dynamic_cast<App::PropertyFloat*>(widthProp);
                    if (widthFloat) {
                        result.activeViewWidth = widthFloat->getValue();
                    }
                }

                if (heightProp) {
                    App::PropertyFloat* heightFloat = dynamic_cast<App::PropertyFloat*>(heightProp);
                    if (heightFloat) {
                        result.activeViewHeight = heightFloat->getValue();
                    }
                }
            } catch (const std::exception& e) {
                Base::Console().warning("Failed to get ActiveView properties: %s\n", e.what());
            }

            // Found what we need, return
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

void StdCmdPrint3dPdf::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    std::vector<App::DocumentObject*> selection;

    // Default values
    double pageWidthPoints = 800.0;
    double pageHeightPoints = 800.0;
    double backgroundR = 1.0, backgroundG = 1.0, backgroundB = 1.0;
    double activeViewX = pageWidthPoints / 5.0;
    double activeViewY = pageHeightPoints / 5.0;
    double activeViewScale = 2.0;
    double activeViewWidth = 100.0, activeViewHeight = 100.0;

    // Try to find TechDraw page with 3D PDF export enabled
    TechDraw3DPDFExportData exportData = findTechDraw3DPDFExportData(activeDoc);

    if (exportData.found()) {
        // Use data from TechDraw ActiveView
        pageWidthPoints = exportData.pageWidthPoints;
        pageHeightPoints = exportData.pageHeightPoints;
        backgroundR = exportData.backgroundR;
        backgroundG = exportData.backgroundG;
        backgroundB = exportData.backgroundB;
        activeViewX = exportData.activeViewX;
        activeViewY = exportData.activeViewY;
        activeViewScale = exportData.activeViewScale;
        activeViewWidth = exportData.activeViewWidth;
        activeViewHeight = exportData.activeViewHeight;
        selection = exportData.sourceObjects;
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
    
    // Tessellate objects using direct C++ API - no Python, no temp files
    std::vector<Export3DPDF::TessellationData> tessData;
    for (const auto& obj : selection) {
        if (!obj) continue;

        try {
            // Get Shape property using Part module's PropertyPartShape
            auto* shapeProp = dynamic_cast<Part::PropertyPartShape*>(obj->getPropertyByName("Shape"));
            if (!shapeProp) {
                Base::Console().warning("Object '%s' has no Shape property, skipping\n", obj->getNameInDocument());
                continue;
            }

            const Part::TopoShape& topoShape = shapeProp->getShape();
            if (topoShape.isNull()) {
                Base::Console().warning("Object '%s' has null shape, skipping\n", obj->getNameInDocument());
                continue;
            }

            Export3DPDF::TessellationData tessObj;
            tessObj.name = obj->getNameInDocument();

            // Tessellate directly using C++ API
            std::vector<Base::Vector3d> points;
            std::vector<Data::ComplexGeoData::Facet> facets;
            topoShape.getFaces(points, facets, 0.1);  // 0.1mm tolerance

            if (points.empty() || facets.empty()) {
                Base::Console().warning("Object '%s' produced empty tessellation, skipping\n", obj->getNameInDocument());
                continue;
            }

            // Convert vertices to our format
            tessObj.vertices.reserve(points.size() * 3);
            for (const auto& pt : points) {
                tessObj.vertices.push_back(pt.x);
                tessObj.vertices.push_back(pt.y);
                tessObj.vertices.push_back(pt.z);
            }

            // Convert triangles to our format
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
                    // Try to cast to ViewProviderPartExt for full material access
                    auto* partVP = dynamic_cast<PartGui::ViewProviderPartExt*>(vp);
                    if (partVP) {
                        const auto& materials = partVP->ShapeAppearance.getValues();
                        if (!materials.empty()) {
                            const App::Material& mat = materials[0];

                            // Diffuse color
                            tessObj.material.diffuseColor[0] = mat.diffuseColor.r;
                            tessObj.material.diffuseColor[1] = mat.diffuseColor.g;
                            tessObj.material.diffuseColor[2] = mat.diffuseColor.b;
                            tessObj.material.diffuseColor[3] = 1.0 - mat.transparency;

                            // Ambient color
                            tessObj.material.ambientColor[0] = mat.ambientColor.r;
                            tessObj.material.ambientColor[1] = mat.ambientColor.g;
                            tessObj.material.ambientColor[2] = mat.ambientColor.b;
                            tessObj.material.ambientColor[3] = 1.0;

                            // Specular color
                            tessObj.material.specularColor[0] = mat.specularColor.r;
                            tessObj.material.specularColor[1] = mat.specularColor.g;
                            tessObj.material.specularColor[2] = mat.specularColor.b;
                            tessObj.material.specularColor[3] = 1.0;

                            // Emissive color
                            tessObj.material.emissiveColor[0] = mat.emissiveColor.r;
                            tessObj.material.emissiveColor[1] = mat.emissiveColor.g;
                            tessObj.material.emissiveColor[2] = mat.emissiveColor.b;
                            tessObj.material.emissiveColor[3] = 1.0;

                            tessObj.material.shininess = mat.shininess;
                            tessObj.material.transparency = mat.transparency;
                            tessObj.material.name = "Material";
                        }
                    } else {
                        // Fallback: try ViewProviderGeometryObject for basic appearance
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
    
    // Use the page found earlier by the helper function
    App::DocumentObject* currentPage = exportData.page;
    std::string backgroundImagePath;

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
                "# Import Qt bindings with fallbacks\n"
                "try:\n"
                "    from PySide6 import QtCore, QtGui\n"
                "except ImportError:\n"
                "    try:\n"
                "        from PySide2 import QtCore, QtGui\n"
                "    except ImportError:\n"
                "        try:\n"
                "            from PyQt5 import QtCore, QtGui\n"
                "        except ImportError:\n"
                "            App.Console.PrintError('No Qt bindings found (PySide6/PySide2/PyQt5)\\n')\n"
                "            raise ImportError('No Qt bindings available')\n"
                "# Main rendering code\n"
                "try:\n"
                "    page_obj = App.getDocument('" + std::string(currentPage->getDocument()->getName()) + "').getObject('" + std::string(currentPage->getNameInDocument()) + "')\n"
                "    if page_obj:\n"
                "        \n"
                "        # Use TechDrawGui.getSceneForPage - this is the proper way to get QGSPage\n"
                "        qgs_page = TechDrawGui.getSceneForPage(page_obj)\n"
                "        # Proceed with rendering if we have a valid QGSPage\n"
                "        if qgs_page:\n"
                "            # Calculate page dimensions in mm (TechDraw's native unit)\n"
                "            dpi = 300.0\n"
                "            mm_to_points = 2.834645669\n"
                "            page_width_mm = " + std::to_string(pageWidthPoints / 2.834645669) + "\n"
                "            page_height_mm = " + std::to_string(pageHeightPoints / 2.834645669) + "\n"
                "            \n"
                "            # Calculate target image size\n"
                "            dpmm = dpi / 25.4  # dots per mm\n"
                "            image_width = int(page_width_mm * dpmm)\n"
                "            image_height = int(page_height_mm * dpmm)\n"
                "            \n"
                "            # Create high-resolution image\n"
                "            image = QtGui.QImage(image_width, image_height, QtGui.QImage.Format_RGB32)\n"
                "            image.fill(QtCore.Qt.white)\n"
                "            \n"
                "            # Set up painter\n"
                "            painter = QtGui.QPainter(image)\n"
                "            painter.setRenderHint(QtGui.QPainter.Antialiasing, True)\n"
                "            painter.setRenderHint(QtGui.QPainter.TextAntialiasing, True)\n"
                "            painter.setRenderHint(QtGui.QPainter.SmoothPixmapTransform, True)\n"
                "            \n"
                "            # Get TechDraw's resolution factor for scene coordinates\n"
                "            rez_factor = 10.0  # Default TechDraw resolution\n"
                "            \n"
                "            # Calculate source rectangle in scene coordinates (like TechDraw does)\n"
                "            # This matches PagePrinter.cpp line 240: QRectF sourceRect(0.0, Rez::guiX(-height), Rez::guiX(width), Rez::guiX(height))\n"
                "            source_width = page_width_mm * rez_factor\n"
                "            source_height = page_height_mm * rez_factor\n"
                "            source_rect = QtCore.QRectF(0.0, -source_height, source_width, source_height)\n"
                "            \n"
                "            # Target rectangle is the full image\n"
                "            target_rect = QtCore.QRectF(0, 0, image_width, image_height)\n"
                "            \n"
                "            # Render the page\n"
                "            qgs_page.render(painter, target_rect, source_rect)\n"
                "            painter.end()\n"
                "            \n"
                "            # Save the image\n"
                "            success = image.save('" + backgroundImagePath + "', 'PNG')\n"
                "        else:\n"
                "            App.Console.PrintWarning('Could not get GUI document\\n')\n"
                "    else:\n"
                "        App.Console.PrintWarning('Could not get page object\\n')\n"
                "except Exception as e:\n"
                "    App.Console.PrintError('Error rendering background: {}\\n'.format(str(e)))\n"
                "    import traceback\n"
                "    App.Console.PrintError('Traceback: {}\\n'.format(traceback.format_exc()))";
            
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
    

    
    if (!tessData.empty()) {
        bool success = false;
        
        if (!backgroundImagePath.empty() && currentPage) {
            success = Export3DPDF::Export3DPDFCore::createHybrid3DPDF(
                tessData, outputPath, backgroundImagePath,
                pageWidthPoints, pageHeightPoints,
                activeViewX, activeViewY, activeViewScale,
                activeViewWidth, activeViewHeight,
                backgroundR, backgroundG, backgroundB
            );
        } else {
            success = Export3DPDF::Export3DPDFCore::convertTessellationToPRC(
                tessData, outputPath, pageWidthPoints, pageHeightPoints,
                backgroundR, backgroundG, backgroundB, activeViewX, activeViewY,
                activeViewScale, activeViewWidth, activeViewHeight
            );
        }
        
        if (success && !backgroundImagePath.empty()) {
            std::remove(backgroundImagePath.c_str());
        }
    }
}

bool StdCmdPrint3dPdf::isActive()
{
    return (getActiveGuiDocument() ? true : false);
} 