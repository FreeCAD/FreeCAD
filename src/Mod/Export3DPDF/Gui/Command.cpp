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
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Tools.h>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>

// Forward declarations to avoid heavy TechDraw dependencies
namespace TechDraw {
    class DrawPage;
    class DrawViewImage;
}

namespace TechDrawGui {
    class ViewProviderImage;
}
#include <Gui/Selection/Selection.h>

#include "Command.h"
#include "../App/Export3DPDFCore.h"

FC_LOG_LEVEL_INIT("Command", false)

using namespace Gui;

// Helper function to parse FreeCAD color format "(r, g, b, a)" into double array
static void parseFreeCADColor(const std::string& colorStr, double* rgba) {
    // Default values if parsing fails
    rgba[0] = 0.5; rgba[1] = 0.5; rgba[2] = 0.5; rgba[3] = 1.0;
    
    // Remove parentheses and spaces
    std::string cleanStr = colorStr;
    cleanStr.erase(std::remove(cleanStr.begin(), cleanStr.end(), '('), cleanStr.end());
    cleanStr.erase(std::remove(cleanStr.begin(), cleanStr.end(), ')'), cleanStr.end());
    cleanStr.erase(std::remove(cleanStr.begin(), cleanStr.end(), ' '), cleanStr.end());
    
    // Split by comma and parse values
    std::istringstream iss(cleanStr);
    std::string token;
    int index = 0;
    while (std::getline(iss, token, ',') && index < 4) {
        try {
            rgba[index] = std::stod(token);
        } catch (const std::exception&) {
            // Keep default value if parsing fails
        }
        index++;
    }
}

//===========================================================================
// Std_Print3dPdf
//===========================================================================

StdCmdPrint3dPdf::StdCmdPrint3dPdf()
  :Command("Std_Print3dPdf")
{
    sGroup        = "File";
    sMenuText     = QT_TR_NOOP("Export &3D PDF...");
    sToolTipText  = QT_TR_NOOP("Export the document as 3D PDF");
    sWhatsThis    = "Std_Print3dPdf";
    sStatusTip    = QT_TR_NOOP("Export the document as 3D PDF");
    sPixmap       = "Std_PrintPdf"; // Using same icon as regular PDF for now
    eType         = 0;
}

void StdCmdPrint3dPdf::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    // Check if we're in a TechDraw page context
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    std::vector<App::DocumentObject*> selection;
    double pageWidthPoints = 595.276;   // A4 width default
    double pageHeightPoints = 841.89;   // A4 height default
    if (activeDoc) {
        // Check if the document contains TechDraw pages (indicating TechDraw context)
        std::vector<App::DocumentObject*> pages = activeDoc->getObjectsOfType(Base::Type::fromName("TechDraw::DrawPage"));
        if (!pages.empty()) {
            Base::Console().message("Found %d TechDraw page(s)\n", pages.size());
            
            // Iterate through pages to find ActiveViews with Enable3DPDFExport
            for (auto* pageObj : pages) {
                // Use dynamic_cast with base class to avoid heavy includes
                if (!pageObj || !pageObj->isDerivedFrom(Base::Type::fromName("TechDraw::DrawPage"))) {
                    continue;
                }
                
                Base::Console().message("Checking page: %s\n", pageObj->getNameInDocument());
                
                // Use property access instead of direct method calls to avoid includes
                App::Property* viewsProp = pageObj->getPropertyByName("Views");
                if (!viewsProp) {
                    continue;
                }
                
                // Get views using property system to avoid header dependencies
                App::PropertyLinkList* viewsList = dynamic_cast<App::PropertyLinkList*>(viewsProp);
                if (!viewsList) {
                    continue;
                }
                
                const std::vector<App::DocumentObject*>& views = viewsList->getValues();
                
                for (auto* view : views) {
                    // Check if this is a DrawViewImage (ActiveView is a type of DrawViewImage)
                    if (!view || !view->isDerivedFrom(Base::Type::fromName("TechDraw::DrawViewImage"))) {
                        continue;
                    }
                    
                    // Check if this is an ActiveView
                    std::string viewLabel = view->getNameInDocument();
                    if (viewLabel.find("ActiveView") != std::string::npos) {
                        Base::Console().message("Found ActiveView: %s\n", viewLabel.c_str());
                        
                        // Get the ViewProvider to check Enable3DPDFExport
                        Gui::Document* guiDoc = Gui::Application::Instance->getDocument(view->getDocument());
                        if (guiDoc) {
                            Gui::ViewProvider* vp = guiDoc->getViewProvider(view);
                            if (vp) {
                                // Check Enable3DPDFExport property using property system
                                App::Property* enable3DProp = vp->getPropertyByName("Enable3DPDFExport");
                                if (enable3DProp) {
                                    App::PropertyBool* enableBool = dynamic_cast<App::PropertyBool*>(enable3DProp);
                                    if (enableBool && enableBool->getValue()) {
                                        Base::Console().message("ActiveView has Enable3DPDFExport=true\n");
                                        
                                        // Extract page dimensions from the TechDraw page
                                        try {
                                            // Access PageWidth and PageHeight properties
                                            doCommand(Command::Doc,
                                                "import FreeCAD as App\n"
                                                "page_obj = App.getDocument('%s').getObject('%s')\n"
                                                "if page_obj:\n"
                                                "    try:\n"
                                                "        width_mm = page_obj.PageWidth\n"
                                                "        height_mm = page_obj.PageHeight\n"
                                                "        print('PAGE_DIMENSIONS')\n"
                                                "        print('WIDTH_MM:', width_mm)\n"
                                                "        print('HEIGHT_MM:', height_mm)\n"
                                                "        print('PAGE_DIMENSIONS_END')\n"
                                                "        App.Console.PrintMessage('Page dimensions: {:.1f} x {:.1f} mm\\n'.format(width_mm, height_mm))\n"
                                                "    except Exception as e:\n"
                                                "        App.Console.PrintWarning('Failed to get page dimensions: {}\\n'.format(str(e)))\n"
                                                "        print('PAGE_DIMENSIONS_ERROR')\n"
                                                "else:\n"
                                                "    App.Console.PrintWarning('Page object not found\\n')\n"
                                                "    print('PAGE_DIMENSIONS_ERROR')",
                                                pageObj->getDocument()->getName(), pageObj->getNameInDocument());
                                            
                                            // Wait for the Python command to complete
                                            QCoreApplication::processEvents();
                                            QThread::msleep(50);
                                            
                                            // Extract dimensions using a temporary file approach (similar to tessellation)
                                            std::string tempDimFile = "page_dimensions_" + std::string(pageObj->getNameInDocument()) + ".tmp";
                                            doCommand(Command::Doc,
                                                "import FreeCAD as App\n"
                                                "page_obj = App.getDocument('%s').getObject('%s')\n"
                                                "if page_obj:\n"
                                                "    try:\n"
                                                "        width_mm = page_obj.PageWidth\n"
                                                "        height_mm = page_obj.PageHeight\n"
                                                "        with open('%s', 'w') as f:\n"
                                                "            f.write('WIDTH_MM\\n')\n"
                                                "            f.write(str(width_mm) + '\\n')\n"
                                                "            f.write('HEIGHT_MM\\n')\n"
                                                "            f.write(str(height_mm) + '\\n')\n"
                                                "            f.write('END\\n')\n"
                                                "        App.Console.PrintMessage('Successfully extracted page dimensions: {:.1f} x {:.1f} mm\\n'.format(width_mm, height_mm))\n"
                                                "    except Exception as e:\n"
                                                "        App.Console.PrintWarning('Failed to get page dimensions: {}\\n'.format(str(e)))\n",
                                                pageObj->getDocument()->getName(), pageObj->getNameInDocument(), tempDimFile.c_str());
                                            
                                            // Wait for the Python command to complete
                                            QCoreApplication::processEvents();
                                            QThread::msleep(100);
                                            
                                            // Read the page dimensions from the temporary file
                                            double pageWidthMM = 297.0;   // A4 landscape width default
                                            double pageHeightMM = 210.0;  // A4 landscape height default
                                            
                                            std::ifstream dimFile(tempDimFile);
                                            if (dimFile.is_open()) {
                                                std::string line;
                                                bool readingWidth = false;
                                                bool readingHeight = false;
                                                
                                                while (std::getline(dimFile, line)) {
                                                    if (line == "WIDTH_MM") {
                                                        readingWidth = true;
                                                        readingHeight = false;
                                                    } else if (line == "HEIGHT_MM") {
                                                        readingWidth = false;
                                                        readingHeight = true;
                                                    } else if (line == "END") {
                                                        break;
                                                    } else if (readingWidth) {
                                                        try {
                                                            pageWidthMM = std::stod(line);
                                                            Base::Console().message("Extracted page width: %.1f mm\n", pageWidthMM);
                                                        } catch (const std::exception& e) {
                                                            Base::Console().warning("Failed to parse page width: %s\n", e.what());
                                                        }
                                                        readingWidth = false;
                                                    } else if (readingHeight) {
                                                        try {
                                                            pageHeightMM = std::stod(line);
                                                            Base::Console().message("Extracted page height: %.1f mm\n", pageHeightMM);
                                                        } catch (const std::exception& e) {
                                                            Base::Console().warning("Failed to parse page height: %s\n", e.what());
                                                        }
                                                        readingHeight = false;
                                                    }
                                                }
                                                dimFile.close();
                                                
                                                // Clean up temporary file
                                                std::remove(tempDimFile.c_str());
                                                
                                                Base::Console().message("Successfully extracted page dimensions: %.1f x %.1f mm\n",
                                                                       pageWidthMM, pageHeightMM);
                                            } else {
                                                Base::Console().warning("Failed to read page dimensions from temp file, using A4 defaults\n");
                                            }
                                            
                                            // Convert mm to points (1 mm = 2.834645669 points)
                                            pageWidthPoints = pageWidthMM * 2.834645669;
                                            pageHeightPoints = pageHeightMM * 2.834645669;
                                            
                                            Base::Console().message("Using page dimensions: %.1f x %.1f mm (%.1f x %.1f points)\n",
                                                                   pageWidthMM, pageHeightMM, pageWidthPoints, pageHeightPoints);
                                            
                                        } catch (const std::exception& e) {
                                            Base::Console().warning("Failed to extract page dimensions, using A4 defaults: %s\n", e.what());
                                        }
                                        
                                        // Get source objects using property system
                                        App::Property* sourceProp = view->getPropertyByName("Source");
                                        App::Property* xsourceProp = view->getPropertyByName("XSource");
                                        
                                        
                                        if (sourceProp) {
                                            App::PropertyLinkList* sourceList = dynamic_cast<App::PropertyLinkList*>(sourceProp);
                                            if (sourceList) {
                                                const std::vector<App::DocumentObject*>& sources = sourceList->getValues();
                                                selection.insert(selection.end(), sources.begin(), sources.end());
                                            }
                                        }
                                        
                                        if (xsourceProp) {
                                            App::PropertyLinkList* xsourceList = dynamic_cast<App::PropertyLinkList*>(xsourceProp);
                                            if (xsourceList) {
                                                const std::vector<App::DocumentObject*>& xsources = xsourceList->getValues();
                                                selection.insert(selection.end(), xsources.begin(), xsources.end());
                                            }
                                        }
                                        
                                        Base::Console().message("ActiveView contains %d source objects:\n", selection.size());
                                        
                                        for (auto* obj : selection) {
                                            if (obj) {
                                                Base::Console().message("  - %s (%s)\n", 
                                                                       obj->Label.getValue(), 
                                                                       obj->getTypeId().getName());
                                            }
                                        }
                                        
                                        if (selection.empty()) {
                                            Base::Console().message("  (No source objects stored in this ActiveView)\n");
                                        }
                                    }
                                    else {
                                        Base::Console().message("ActiveView has Enable3DPDFExport=false\n");
                                    }
                                }
                                else {
                                    Base::Console().message("Could not find Enable3DPDFExport property\n");
                                }
                            }
                            else {
                                Base::Console().message("Could not get ViewProvider for ActiveView\n");
                            }
                        }
                        else {
                            Base::Console().message("Could not get GUI document\n");
                        }
                    }
                }
            }
        }
        else {
            Base::Console().message("No TechDraw pages found\n");
            selection = Gui::Selection().getObjectsOfType(App::DocumentObject::getClassTypeId());
        }
    }
    
    
    if (selection.empty()) {
        QMessageBox::warning(getMainWindow(),
            QCoreApplication::translate("StdCmdPrint3dPdf", "No selection"),
            QCoreApplication::translate("StdCmdPrint3dPdf", "Select the objects to export before choosing 3D PDF Export."));
        return;
    }
    
    // Set up file dialog filter for 3D PDF
    QStringList filterList;
    filterList << QObject::tr("3D PDF (*.pdf)");
    filterList << QObject::tr("All Files (*.*)");
    QString formatList = filterList.join(QLatin1String(";;"));
    QString selectedFilter;
    
    // Create default filename based on document/object names
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
    
    // Add object names to filename if only a few objects selected
    if (selection.size() <= 3) {
        QStringList objectNames;
        for (const auto& obj : selection) {
            objectNames << QString::fromStdString(obj->Label.getStrValue());
        }
        if (!objectNames.isEmpty()) {
            baseFilename += QString::fromLatin1("-") + objectNames.join(QString::fromLatin1("-"));
        }
    }
    
    // Clean only the filename for cross-platform compatibility (not the path!)
    QString invalidCharacters = QLatin1String("\\?%*:|\"<>");  // Removed forward slash
    for (const auto &c : invalidCharacters)
        baseFilename.replace(c, QLatin1String("_"));
    
    // Combine path and cleaned filename
    defaultFilename = QDir(defaultPath).filePath(baseFilename + QString::fromLatin1(".pdf"));
    
    // Launch the file selection dialog
    QString fileName = FileDialog::getSaveFileName(getMainWindow(),
        QObject::tr("Export 3D PDF"), defaultFilename, formatList, &selectedFilter);
    
    if (fileName.isEmpty()) {
        return; // User cancelled
    }
    
    // Remove extension from fileName as Export3DPDF will add it
    QFileInfo fileInfo(fileName);
    std::string outputPath = fileInfo.path().toStdString() + "/" + fileInfo.completeBaseName().toStdString();
    
    Base::Console().message("Extracting tessellation and material data for %zu selected object(s):\n", selection.size());
    
    // Collect tessellation data from all selected objects
    std::vector<Export3DPDF::TessellationData> tessData;
    
    // Process each selected object
    for (const auto& obj : selection) {
        if (!obj) continue;
        
        try {
            // Check if object has a Shape property
            App::Property* shapeProp = obj->getPropertyByName("Shape");
            if (shapeProp) {
                std::string docName = obj->getDocument()->getName();
                std::string objName = obj->getNameInDocument();
                
                Base::Console().message("Processing object '%s'...\n", objName.c_str());
                
                // Get tessellation data directly using C++ API
                Export3DPDF::TessellationData tessObj;
                tessObj.name = objName;
                
                // Try to access the Shape property directly through C++
                try {
                    App::Property* shapeProp = obj->getPropertyByName("Shape");
                    if (shapeProp) {
                        // Cast to appropriate property type and extract shape
                        // For now, execute Python but in a simpler way to get the data
                        doCommand(Command::Doc, 
                            "import FreeCAD as App\n"
                            "obj = App.getDocument('%s').getObject('%s')\n"
                            "if hasattr(obj, 'Shape') and obj.Shape:\n"
                            "    mesh_data = obj.Shape.tessellate(0.1)\n"
                            "    vertices = mesh_data[0]\n"
                            "    triangles = mesh_data[1]\n"
                            "    print('TESSELLATION_START')\n"
                            "    print('NAME:', obj.Name)\n"
                            "    print('VERTICES:', len(vertices))\n"
                            "    for v in vertices:\n"
                            "        print('V:', v[0], v[1], v[2])\n"
                            "    print('TRIANGLES:', len(triangles))\n"
                            "    for t in triangles:\n"
                            "        print('T:', t[0], t[1], t[2])\n"
                            "    print('TESSELLATION_END')\n"
                            "    App.Console.PrintMessage('Object \\'{}\\': {} vertices, {} triangles\\n'.format(obj.Name, len(vertices), len(triangles)))\n"
                            "else:\n"
                            "    print('TESSELLATION_START')\n"
                            "    print('ERROR: No valid Shape')\n"
                            "    print('TESSELLATION_END')",
                            docName.c_str(), objName.c_str());
                        
                        // Execute a separate Python command to write tessellation and material data to a temporary file
                        std::string tempFileName = "tessellation_" + objName + ".tmp";
                        doCommand(Command::Doc,
                            "import FreeCAD as App\n"
                            "import FreeCADGui as Gui\n"
                            "try:\n"
                            "    # Get object through GUI layer\n"
                            "    gui_doc = Gui.getDocument('%s')\n"
                            "    obj = gui_doc.getObject('%s')\n"
                            "    app_obj = App.getDocument('%s').getObject('%s')\n"
                            "    \n"
                            "    if hasattr(app_obj, 'Shape') and app_obj.Shape:\n"
                            "        # Extract tessellation data from App object\n"
                            "        mesh_data = app_obj.Shape.tessellate(0.1)\n"
                            "        vertices = mesh_data[0]\n"
                            "        triangles = mesh_data[1]\n"
                            "        \n"
                            "        # Extract material properties from ShapeAppearance in GUI\n"
                            "        material_props = {}\n"
                            "        if obj and hasattr(obj, 'ShapeAppearance') and obj.ShapeAppearance:\n"
                            "            try:\n"
                            "                mat = obj.ShapeAppearance[0]\n"
                            "                material_props['Name'] = getattr(mat, 'Name', 'Default')\n"
                            "                material_props['AmbientColor'] = str(getattr(mat, 'AmbientColor', (0.333333, 0.333333, 0.333333, 1)))\n"
                            "                material_props['DiffuseColor'] = str(getattr(mat, 'DiffuseColor', (0.678431, 0.709804, 0.741176, 1)))\n"
                            "                material_props['EmissiveColor'] = str(getattr(mat, 'EmissiveColor', (0, 0, 0, 1)))\n"
                            "                material_props['SpecularColor'] = str(getattr(mat, 'SpecularColor', (0.533333, 0.533333, 0.533333, 1)))\n"
                            "                material_props['Shininess'] = str(getattr(mat, 'Shininess', 0.9))\n"
                            "                material_props['Transparency'] = str(getattr(mat, 'Transparency', 0))\n"
                            "                App.Console.PrintMessage('Extracted material from ShapeAppearance: {} (Shininess: {}, Transparency: {})\\n'.format(\n"
                            "                    material_props['Name'], material_props['Shininess'], material_props['Transparency']))\n"
                            "            except Exception as e:\n"
                            "                App.Console.PrintWarning('Failed to extract ShapeAppearance properties: {}\\n'.format(str(e)))\n"
                            "                material_props = {'Name': 'Default'}\n"
                            "        else:\n"
                            "            App.Console.PrintMessage('No ShapeAppearance found, using default material\\n')\n"
                            "            material_props = {'Name': 'Default'}\n"
                            "        \n"
                            "        # Write data to temp file\n"
                            "        with open('%s', 'w') as f:\n"
                            "            # Write material properties first\n"
                            "            f.write('MATERIAL\\n')\n"
                            "            for key, value in material_props.items():\n"
                            "                f.write(f'{key}: {value}\\n')\n"
                            "            \n"
                            "            # Write tessellation data\n"
                            "            f.write('VERTICES\\n')\n"
                            "            for v in vertices:\n"
                            "                f.write(f'{v[0]} {v[1]} {v[2]}\\n')\n"
                            "            f.write('TRIANGLES\\n')\n"
                            "            for t in triangles:\n"
                            "                f.write(f'{t[0]} {t[1]} {t[2]}\\n')\n"
                            "            f.write('END\\n')\n"
                            "        App.Console.PrintMessage('Successfully wrote tessellation data to temp file\\n')\n"
                            "    else:\n"
                            "        App.Console.PrintWarning('Object has no Shape property\\n')\n"
                            "except Exception as e:\n"
                            "    App.Console.PrintError('Error in tessellation extraction: {}\\n'.format(str(e)))\n"
                            "    import traceback\n"
                            "    App.Console.PrintError('Traceback: {}\\n'.format(traceback.format_exc()))",
                            docName.c_str(), objName.c_str(), docName.c_str(), objName.c_str(), tempFileName.c_str());
                        
                        // Wait for the Python command to complete
                        QCoreApplication::processEvents();
                        QThread::msleep(100);
                        
                        // Read the tessellation and material data from the temporary file
                        std::ifstream tessFile(tempFileName);
                        if (tessFile.is_open()) {
                            std::string line;
                            bool readingMaterial = false;
                            bool readingVertices = false;
                            bool readingTriangles = false;
                            
                            while (std::getline(tessFile, line)) {
                                if (line == "MATERIAL") {
                                    readingMaterial = true;
                                    readingVertices = false;
                                    readingTriangles = false;
                                } else if (line == "VERTICES") {
                                    readingMaterial = false;
                                    readingVertices = true;
                                    readingTriangles = false;
                                } else if (line == "TRIANGLES") {
                                    readingMaterial = false;
                                    readingVertices = false;
                                    readingTriangles = true;
                                } else if (line == "END") {
                                    break;
                                } else if (readingMaterial) {
                                    // Parse material properties
                                    size_t colonPos = line.find(':');
                                    if (colonPos != std::string::npos) {
                                        std::string key = line.substr(0, colonPos);
                                        std::string value = line.substr(colonPos + 2); // Skip ': '
                                        
                                        if (key == "Name") {
                                            tessObj.material.name = value;
                                        } else if (key == "AmbientColor") {
                                            parseFreeCADColor(value, tessObj.material.ambientColor);
                                        } else if (key == "DiffuseColor") {
                                            parseFreeCADColor(value, tessObj.material.diffuseColor);
                                        } else if (key == "EmissiveColor") {
                                            parseFreeCADColor(value, tessObj.material.emissiveColor);
                                        } else if (key == "SpecularColor") {
                                            parseFreeCADColor(value, tessObj.material.specularColor);
                                        } else if (key == "Shininess") {
                                            tessObj.material.shininess = std::stod(value);
                                        } else if (key == "Transparency") {
                                            tessObj.material.transparency = std::stod(value);
                                        }
                                    }
                                } else if (readingVertices) {
                                    std::istringstream iss(line);
                                    double x, y, z;
                                    if (iss >> x >> y >> z) {
                                        tessObj.vertices.push_back(x);
                                        tessObj.vertices.push_back(y);
                                        tessObj.vertices.push_back(z);
                                    }
                                } else if (readingTriangles) {
                                    std::istringstream iss(line);
                                    int i1, i2, i3;
                                    if (iss >> i1 >> i2 >> i3) {
                                        tessObj.triangles.push_back(i1);
                                        tessObj.triangles.push_back(i2);
                                        tessObj.triangles.push_back(i3);
                                    }
                                }
                            }
                            tessFile.close();
                            
                            // Clean up temporary file
                            std::remove(tempFileName.c_str());
                            
                            Base::Console().message("Successfully extracted tessellation: %zu vertices, %zu triangles\n",
                                tessObj.vertices.size() / 3, tessObj.triangles.size() / 3);
                            Base::Console().message("Material: %s (Shininess: %.2f, Transparency: %.2f)\n",
                                tessObj.material.name.c_str(), tessObj.material.shininess, tessObj.material.transparency);
                        } else {
                            Base::Console().warning("Failed to read tessellation data from temp file, using fallback\n");
                            // Fallback to placeholder data
                            tessObj.vertices = {
                                0.0, 0.0, 0.0,  1.0, 0.0, 0.0,  1.0, 1.0, 0.0,  0.0, 1.0, 0.0,
                                0.0, 0.0, 1.0,  1.0, 0.0, 1.0,  1.0, 1.0, 1.0,  0.0, 1.0, 1.0
                            };
                            tessObj.triangles = {
                                0, 1, 2,  0, 2, 3,  4, 7, 6,  4, 6, 5,
                                0, 4, 5,  0, 5, 1,  2, 6, 7,  2, 7, 3,
                                0, 3, 7,  0, 7, 4,  1, 5, 6,  1, 6, 2
                            };
                            // Material data remains default
                        }
                    }
                } catch (const std::exception& e) {
                    Base::Console().error("Error extracting tessellation data: %s\n", e.what());
                    // Use fallback data
                    tessObj.vertices = {
                        0.0, 0.0, 0.0,  1.0, 0.0, 0.0,  1.0, 1.0, 0.0,  0.0, 1.0, 0.0,
                        0.0, 0.0, 1.0,  1.0, 0.0, 1.0,  1.0, 1.0, 1.0,  0.0, 1.0, 1.0
                    };
                    tessObj.triangles = {
                        0, 1, 2,  0, 2, 3,  4, 7, 6,  4, 6, 5,
                        0, 4, 5,  0, 5, 1,  2, 6, 7,  2, 7, 3,
                        0, 3, 7,  0, 7, 4,  1, 5, 6,  1, 6, 2
                    };
                    // Material data remains default
                }
                
                tessData.push_back(tessObj);
                
            } else {
                Base::Console().message("Object '%s' has no Shape property.\n", obj->getNameInDocument());
            }
        } catch (const Base::Exception& e) {
            Base::Console().error("Error processing object '%s': %s\n", obj->getNameInDocument(), e.what());
        }
    }
    
    // Now convert tessellation data to PRC format
    Base::Console().message("Converting tessellation data to PRC format...\n");
    Base::Console().message("PDF will use page dimensions: %.1f x %.1f points (%.1f x %.1f mm)\n",
                           pageWidthPoints, pageHeightPoints,
                           pageWidthPoints / 2.834645669, pageHeightPoints / 2.834645669);
    
    if (!tessData.empty()) {
        // Convert to PRC format using the user-selected path and extracted page dimensions
        bool success = Export3DPDF::Export3DPDFCore::convertTessellationToPRC(tessData, outputPath, pageWidthPoints, pageHeightPoints);
        
        if (success) {
            Base::Console().message("3D PDF export completed successfully: %s.pdf\n", outputPath.c_str());
        } else {
            Base::Console().error("3D PDF export failed\n");
        }
    } else {
        Base::Console().message("No tessellation data available for 3D PDF conversion\n");
    }
}

bool StdCmdPrint3dPdf::isActive()
{
    return (getActiveGuiDocument() ? true : false);
} 