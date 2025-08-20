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

static void parseFreeCADColor(const std::string& colorStr, double* rgba) {
    rgba[0] = 0.5; rgba[1] = 0.5; rgba[2] = 0.5; rgba[3] = 1.0;
    
    std::string cleanStr = colorStr;
    cleanStr.erase(std::remove(cleanStr.begin(), cleanStr.end(), '('), cleanStr.end());
    cleanStr.erase(std::remove(cleanStr.begin(), cleanStr.end(), ')'), cleanStr.end());
    cleanStr.erase(std::remove(cleanStr.begin(), cleanStr.end(), ' '), cleanStr.end());
    
    std::istringstream iss(cleanStr);
    std::string token;
    int index = 0;
    while (std::getline(iss, token, ',') && index < 4) {
        try {
            rgba[index] = std::stod(token);
        } catch (const std::exception&) {
        }
        index++;
    }
}



StdCmdPrint3dPdf::StdCmdPrint3dPdf()
  :Command("Std_Print3dPdf")
{
    sGroup        = "File";
    sMenuText     = QT_TR_NOOP("Export &3D PDF...");
    sToolTipText  = QT_TR_NOOP("Export the document as 3D PDF");
    sWhatsThis    = "Std_Print3dPdf";
    sStatusTip    = QT_TR_NOOP("Export the document as 3D PDF");
    sPixmap       = "Std_PrintPdf";
    eType         = 0;
}

void StdCmdPrint3dPdf::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    std::vector<App::DocumentObject*> selection;
    double pageWidthPoints = 595.276;
    double pageHeightPoints = 841.89;
    double backgroundR = 0.5, backgroundG = 0.5, backgroundB = 0.5;
    
    double activeViewX = 0.0, activeViewY = 0.0, activeViewScale = 1.0;
    double activeViewWidth = 100.0, activeViewHeight = 100.0;
    if (activeDoc) {
        std::vector<App::DocumentObject*> pages = activeDoc->getObjectsOfType(Base::Type::fromName("TechDraw::DrawPage"));
        if (!pages.empty()) {
            
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
                    
                    std::string viewLabel = view->getNameInDocument();
                    if (viewLabel.find("ActiveView") != std::string::npos) {
                        
                        Gui::Document* guiDoc = Gui::Application::Instance->getDocument(view->getDocument());
                        if (guiDoc) {
                            Gui::ViewProvider* vp = guiDoc->getViewProvider(view);
                            if (vp) {
                                App::Property* enable3DProp = vp->getPropertyByName("Enable3DPDFExport");
                                if (enable3DProp) {
                                    App::PropertyBool* enableBool = dynamic_cast<App::PropertyBool*>(enable3DProp);
                                    if (enableBool && enableBool->getValue()) {
                                        
                                        try {

                                            doCommand(Command::Doc,
                                                "import FreeCAD as App\n"
                                                "page_obj = App.getDocument('%s').getObject('%s')\n"
                                                "if page_obj:\n"
                                                "    try:\n"
                                                "        width_mm = page_obj.PageWidth\n"
                                                "        height_mm = page_obj.PageHeight\n"
                                                "    except Exception as e:\n"
                                                "        App.Console.PrintWarning('Failed to get page dimensions: {}\\n'.format(str(e)))\n"
                                                "else:\n"
                                                "    App.Console.PrintWarning('Page object not found\\n')\n",
                                                pageObj->getDocument()->getName(), pageObj->getNameInDocument());
                                            
                                            QCoreApplication::processEvents();
                                            QThread::msleep(50);
                                            
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
                                                "    except Exception as e:\n"
                                                "        App.Console.PrintWarning('Failed to get page dimensions: {}\\n'.format(str(e)))\n",
                                                pageObj->getDocument()->getName(), pageObj->getNameInDocument(), tempDimFile.c_str());
                                            
                                            QCoreApplication::processEvents();
                                            QThread::msleep(100);
                                            
                                            double pageWidthMM = 297.0;
                                            double pageHeightMM = 210.0;
                                            
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
                                                        } catch (const std::exception& e) {
                                                        }
                                                        readingWidth = false;
                                                    } else if (readingHeight) {
                                                        try {
                                                            pageHeightMM = std::stod(line);
                                                        } catch (const std::exception& e) {
                                                        }
                                                        readingHeight = false;
                                                    }
                                                }
                                                dimFile.close();
                                                
                                                std::remove(tempDimFile.c_str());
                                            }
                                            
                                            pageWidthPoints = pageWidthMM * 2.834645669;
                                            pageHeightPoints = pageHeightMM * 2.834645669;
                                            
                                        } catch (const std::exception& e) {
                                        }
                                        
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
                                                backgroundR = 1.0; backgroundG = 1.0; backgroundB = 1.0;
                                            } else if (solidBackground && backgroundColorProp) {
                                                App::PropertyColor* colorProp = dynamic_cast<App::PropertyColor*>(backgroundColorProp);
                                                if (colorProp) {
                                                    Base::Color bgColor = colorProp->getValue();
                                                    backgroundR = bgColor.r;
                                                    backgroundG = bgColor.g;
                                                    backgroundB = bgColor.b;
                                                }
                                            }
                                            
                                        } catch (const std::exception& e) {
                                        }
                                        
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
                                        

                                        
                                        try {
                                            App::Property* xProp = view->getPropertyByName("X");
                                            App::Property* yProp = view->getPropertyByName("Y");
                                            App::Property* scaleProp = view->getPropertyByName("Scale");
                                            App::Property* widthProp = view->getPropertyByName("Width");
                                            App::Property* heightProp = view->getPropertyByName("Height");
                                            
                                            if (xProp) {
                                                App::PropertyDistance* xDist = dynamic_cast<App::PropertyDistance*>(xProp);
                                                if (xDist) {
                                                    activeViewX = xDist->getValue();
                                                }
                                            }
                                            
                                            if (yProp) {
                                                App::PropertyDistance* yDist = dynamic_cast<App::PropertyDistance*>(yProp);
                                                if (yDist) {
                                                    activeViewY = yDist->getValue();
                                                }
                                            }
                                            
                                            if (scaleProp) {
                                                App::PropertyFloatConstraint* scaleFloat = dynamic_cast<App::PropertyFloatConstraint*>(scaleProp);
                                                if (scaleFloat) {
                                                    activeViewScale = scaleFloat->getValue();
                                                }
                                            }
                                            
                                            if (widthProp) {
                                                App::PropertyFloat* widthFloat = dynamic_cast<App::PropertyFloat*>(widthProp);
                                                if (widthFloat) {
                                                    activeViewWidth = widthFloat->getValue();
                                                }
                                            }
                                            
                                            if (heightProp) {
                                                App::PropertyFloat* heightFloat = dynamic_cast<App::PropertyFloat*>(heightProp);
                                                if (heightFloat) {
                                                    activeViewHeight = heightFloat->getValue();
                                                }
                                            }
                                            

                                            
                                        } catch (const std::exception& e) {
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            selection = Gui::Selection().getObjectsOfType(App::DocumentObject::getClassTypeId());
        }
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
    
    std::vector<Export3DPDF::TessellationData> tessData;
    for (const auto& obj : selection) {
        if (!obj) continue;
        
        try {
            App::Property* shapeProp = obj->getPropertyByName("Shape");
            if (shapeProp) {
                std::string docName = obj->getDocument()->getName();
                std::string objName = obj->getNameInDocument();
                
                Export3DPDF::TessellationData tessObj;
                tessObj.name = objName;
                
                try {
                    App::Property* shapeProp = obj->getPropertyByName("Shape");
                    if (shapeProp) {
                        doCommand(Command::Doc, 
                            "import FreeCAD as App\n"
                            "obj = App.getDocument('%s').getObject('%s')\n"
                            "if hasattr(obj, 'Shape') and obj.Shape:\n"
                            "    mesh_data = obj.Shape.tessellate(0.1)\n"
                            "    vertices = mesh_data[0]\n"
                            "    triangles = mesh_data[1]\n"
                            "else:\n"
                            "    App.Console.PrintWarning('Object \\'{}\\': No valid Shape\\n'.format(obj.Name))",
                            docName.c_str(), objName.c_str());
                        
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
                            "    else:\n"
                            "        App.Console.PrintWarning('Object has no Shape property\\n')\n"
                            "except Exception as e:\n"
                            "    App.Console.PrintError('Error in tessellation extraction: {}\\n'.format(str(e)))\n"
                            "    import traceback\n"
                            "    App.Console.PrintError('Traceback: {}\\n'.format(traceback.format_exc()))",
                            docName.c_str(), objName.c_str(), docName.c_str(), objName.c_str(), tempFileName.c_str());
                        
                        QCoreApplication::processEvents();
                        QThread::msleep(100);
                        

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
                                    size_t colonPos = line.find(':');
                                    if (colonPos != std::string::npos) {
                                        std::string key = line.substr(0, colonPos);
                                        std::string value = line.substr(colonPos + 2);
                                        
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
                            
                            std::remove(tempFileName.c_str());
                        } else {
                            tessObj.vertices = {
                                0.0, 0.0, 0.0,  1.0, 0.0, 0.0,  1.0, 1.0, 0.0,  0.0, 1.0, 0.0,
                                0.0, 0.0, 1.0,  1.0, 0.0, 1.0,  1.0, 1.0, 1.0,  0.0, 1.0, 1.0
                            };
                            tessObj.triangles = {
                                0, 1, 2,  0, 2, 3,  4, 7, 6,  4, 6, 5,
                                0, 4, 5,  0, 5, 1,  2, 6, 7,  2, 7, 3,
                                0, 3, 7,  0, 7, 4,  1, 5, 6,  1, 6, 2
                            };
                        }
                    }
                } catch (const std::exception& e) {
                    tessObj.vertices = {
                        0.0, 0.0, 0.0,  1.0, 0.0, 0.0,  1.0, 1.0, 0.0,  0.0, 1.0, 0.0,
                        0.0, 0.0, 1.0,  1.0, 0.0, 1.0,  1.0, 1.0, 1.0,  0.0, 1.0, 1.0
                    };
                    tessObj.triangles = {
                        0, 1, 2,  0, 2, 3,  4, 7, 6,  4, 6, 5,
                        0, 4, 5,  0, 5, 1,  2, 6, 7,  2, 7, 3,
                        0, 3, 7,  0, 7, 4,  1, 5, 6,  1, 6, 2
                    };
                }
                
                tessData.push_back(tessObj);
                
            }
        } catch (const Base::Exception& e) {
        }
    }
    
    App::DocumentObject* currentPage = nullptr;
    std::string backgroundImagePath;
    if (activeDoc) {
        std::vector<App::DocumentObject*> pages = activeDoc->getObjectsOfType(Base::Type::fromName("TechDraw::DrawPage"));
        for (auto* pageObj : pages) {
            if (pageObj && pageObj->isDerivedFrom(Base::Type::fromName("TechDraw::DrawPage"))) {
                // Check if this page contains our ActiveView
                App::Property* viewsProp = pageObj->getPropertyByName("Views");
                if (viewsProp) {
                    App::PropertyLinkList* viewsList = dynamic_cast<App::PropertyLinkList*>(viewsProp);
                    if (viewsList) {
                        const std::vector<App::DocumentObject*>& views = viewsList->getValues();
                        for (auto* view : views) {
                            if (view && view->isDerivedFrom(Base::Type::fromName("TechDraw::DrawViewImage"))) {
                                std::string viewLabel = view->getNameInDocument();
                                if (viewLabel.find("ActiveView") != std::string::npos) {
                                    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(view->getDocument());
                                    if (guiDoc) {
                                        Gui::ViewProvider* vp = guiDoc->getViewProvider(view);
                                        if (vp) {
                                            App::Property* enable3DProp = vp->getPropertyByName("Enable3DPDFExport");
                                            if (enable3DProp) {
                                                App::PropertyBool* enableBool = dynamic_cast<App::PropertyBool*>(enable3DProp);
                                                if (enableBool && enableBool->getValue()) {
                                                    currentPage = pageObj;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (currentPage) break;
                    }
                }
            }
        }
    }
    
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