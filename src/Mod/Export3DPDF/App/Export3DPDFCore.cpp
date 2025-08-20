#include "PreCompiled.h"

#ifndef _PreComp_
# include <fstream>
# include <iostream>
# include <vector>
# include <string>
#endif

#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <oPRCFile.h>
#include <writePRC.h>
#include <PRC.h>
#include <hpdf.h>
#include <hpdf_u3d.h>
#include "Export3DPDFCore.h"

using namespace Export3DPDF;

bool Export3DPDFCore::convertTessellationToPRC(const std::vector<TessellationData>& tessellationData, const std::string& outputPath, double pageWidthPoints, double pageHeightPoints, double backgroundR, double backgroundG, double backgroundB, double activeViewX, double activeViewY, double activeViewScale, double activeViewWidth, double activeViewHeight)
{
    try {
        std::string prcPath = outputPath + ".prc";
        std::string result = createPRCFile(tessellationData, prcPath);
        if (result.empty()) {
            return false;
        }
        
        std::string pdfPath = outputPath + ".pdf";
        if (!embedPRCInPDF(prcPath, pdfPath, pageWidthPoints, pageHeightPoints, backgroundR, backgroundG, backgroundB, activeViewX, activeViewY, activeViewScale, activeViewWidth, activeViewHeight)) {
            return false;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

std::string Export3DPDFCore::createPRCFile(const std::vector<TessellationData>& tessellationData, const std::string& prcPath)
{
    try {
        std::ofstream prcStream(prcPath.c_str(), std::ios::binary);
        if (!prcStream.is_open()) {
            return "";
        }
        
        oPRCFile prcFile(prcStream, 1.0);
        
        for (const auto& objData : tessellationData) {
            if (!objData.vertices.empty() && !objData.triangles.empty()) {
                
                PRC3DTess* prc3DTess = new PRC3DTess();
                
                prc3DTess->has_faces = true;
                prc3DTess->has_loops = false;
                
                prc3DTess->coordinates = objData.vertices;
                
                PRCTessFace* tessFace = new PRCTessFace();
                tessFace->used_entities_flag = PRC_FACETESSDATA_Triangle;
                tessFace->start_triangulated = 0;
                tessFace->sizes_triangulated.push_back(0);
                tessFace->is_rgba = false;
                tessFace->behaviour = PRC_GRAPHICS_Show;
                tessFace->number_of_texture_coordinate_indexes = 0;
                
                uint32_t triangleCount = 0;
                for (size_t i = 0; i < objData.triangles.size(); i += 3) {
                    prc3DTess->triangulated_index.push_back(static_cast<uint32_t>(objData.triangles[i] * 3));
                    prc3DTess->triangulated_index.push_back(static_cast<uint32_t>(objData.triangles[i+1] * 3));
                    prc3DTess->triangulated_index.push_back(static_cast<uint32_t>(objData.triangles[i+2] * 3));
                    triangleCount++;
                }
                
                tessFace->sizes_triangulated[0] = triangleCount;
                
                prc3DTess->addTessFace(tessFace);
                
                uint32_t tessIndex = prcFile.add3DTess(prc3DTess);
                
                RGBAColour ambient(objData.material.ambientColor[0], 
                                 objData.material.ambientColor[1], 
                                 objData.material.ambientColor[2], 
                                 objData.material.ambientColor[3]);
                RGBAColour diffuse(objData.material.diffuseColor[0], 
                                 objData.material.diffuseColor[1], 
                                 objData.material.diffuseColor[2], 
                                 objData.material.diffuseColor[3]);
                RGBAColour emissive(objData.material.emissiveColor[0], 
                                  objData.material.emissiveColor[1], 
                                  objData.material.emissiveColor[2], 
                                  objData.material.emissiveColor[3]);
                RGBAColour specular(objData.material.specularColor[0], 
                                  objData.material.specularColor[1], 
                                  objData.material.specularColor[2], 
                                  objData.material.specularColor[3]);

                
                PRCmaterial material(ambient, diffuse, emissive, specular, 
                                   objData.material.getAlpha(),
                                   objData.material.shininess);
                
                uint32_t materialIndex = prcFile.addMaterial(material);
                
                prcFile.useMesh(tessIndex, materialIndex, NULL);
                
            }
        }
        
        if (!prcFile.finish()) {
            prcStream.close();
            return "";
        }
        
        prcStream.close();
        return prcPath;
    }
    catch (const std::exception& e) {
        return "";
    }
}

bool Export3DPDFCore::embedPRCInPDF(const std::string& prcPath, const std::string& pdfPath, double pageWidthPoints, double pageHeightPoints, double backgroundR, double backgroundG, double backgroundB, double activeViewX, double activeViewY, double activeViewScale, double activeViewWidth, double activeViewHeight)
{
    try {
        std::ifstream prcFile(prcPath, std::ios::binary);
        if (!prcFile.is_open()) {
            return false;
        }
        
        prcFile.seekg(0, std::ios::end);
        size_t prcSize = prcFile.tellg();
        prcFile.seekg(0, std::ios::beg);
        
        std::vector<HPDF_BYTE> prcBuffer(prcSize);
        prcFile.read(reinterpret_cast<char*>(prcBuffer.data()), prcSize);
        prcFile.close();
        
        HPDF_Doc pdf = HPDF_New(NULL, NULL);
        if (!pdf) {
            return false;
        }
        
        HPDF_SetInfoAttr(pdf, HPDF_INFO_PRODUCER, "FreeCAD 3D PDF Export");
        HPDF_SetInfoAttr(pdf, HPDF_INFO_TITLE, "FreeCAD 3D Model");
        
        HPDF_Page page = HPDF_AddPage(pdf);
        HPDF_Page_SetWidth(page, pageWidthPoints);
        HPDF_Page_SetHeight(page, pageHeightPoints);
        
        HPDF_Image u3d = HPDF_LoadU3DFromMem(pdf, prcBuffer.data(), static_cast<HPDF_UINT>(prcSize));
        if (!u3d) {
            HPDF_Free(pdf);
            return false;
        }
        
        double scaledViewWidth = activeViewWidth * activeViewScale;
        double scaledViewHeight = activeViewHeight * activeViewScale;
        double viewWidthPoints = scaledViewWidth * 2.834645669;
        double viewHeightPoints = scaledViewHeight * 2.834645669;
        double viewXPoints = activeViewX * 2.834645669;
        double viewYPoints = activeViewY * 2.834645669;
        
        double halfWidthPoints = viewWidthPoints / 2.0;
        double halfHeightPoints = viewHeightPoints / 2.0;
        
        double annotLeft = viewXPoints - halfWidthPoints;
        double annotRight = viewXPoints + halfWidthPoints;
        
        double annotBottom = viewYPoints - halfHeightPoints;
        double annotTop = viewYPoints + halfHeightPoints;
        
        
        
        HPDF_Rect rect = {static_cast<HPDF_REAL>(annotLeft), 
                         static_cast<HPDF_REAL>(annotBottom), 
                         static_cast<HPDF_REAL>(annotRight), 
                         static_cast<HPDF_REAL>(annotTop)}; // left, bottom, right, top
        HPDF_Annotation annot = HPDF_Page_Create3DAnnot(page, rect, HPDF_TRUE, HPDF_FALSE, u3d, NULL);
        if (!annot) {
            Base::Console().error("Failed to create 3D annotation\n");
            HPDF_Free(pdf);
            return false;
        }
        
        HPDF_Dict view = HPDF_Page_Create3DView(page, u3d, annot, "Default");
        if (!view) {
            Base::Console().error("Failed to create 3D view\n");
            HPDF_Free(pdf);
            return false;
        }
        
        HPDF_3DView_SetLighting(view, "CAD");
        HPDF_3DView_SetBackgroundColor(view, backgroundR, backgroundG, backgroundB);
        
        HPDF_3DView_SetCamera(view, 
            10.0, 10.0, 10.0,  // camera position
            0.0, 0.0, 0.0,     // target position  
            50.0,              // distance
            0.0);              // roll
        
        HPDF_U3D_SetDefault3DView(u3d, "Default");
        
        HPDF_STATUS result = HPDF_SaveToFile(pdf, pdfPath.c_str());
        if (result != HPDF_OK) {
            HPDF_Free(pdf);
            return false;
        }
        
        HPDF_Free(pdf);
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

bool Export3DPDFCore::createHybrid3DPDF(const std::vector<TessellationData>& tessellationData,
                                        const std::string& outputPath,
                                        const std::string& backgroundImagePath,
                                        double pageWidthPoints,
                                        double pageHeightPoints,
                                        double activeViewX,
                                        double activeViewY,
                                        double activeViewScale,
                                        double activeViewWidth,
                                        double activeViewHeight,
                                        double backgroundR,
                                        double backgroundG,
                                        double backgroundB)
{
    try {

        std::string prcPath = outputPath + ".prc";
        std::string result = createPRCFile(tessellationData, prcPath);
        if (result.empty()) {
            Base::Console().error("Failed to create PRC file for hybrid PDF\n");
            return false;
        }
        
        std::ifstream prcFile(prcPath, std::ios::binary);
        if (!prcFile.is_open()) {
            Base::Console().error("Failed to open PRC file: %s\n", prcPath.c_str());
            return false;
        }
        
        prcFile.seekg(0, std::ios::end);
        size_t prcSize = prcFile.tellg();
        prcFile.seekg(0, std::ios::beg);
        
        std::vector<HPDF_BYTE> prcBuffer(prcSize);
        prcFile.read(reinterpret_cast<char*>(prcBuffer.data()), prcSize);
        prcFile.close();
        
        HPDF_Doc pdf = HPDF_New(NULL, NULL);
        if (!pdf) {
            Base::Console().error("Failed to create PDF document for hybrid export\n");
            return false;
        }
        
        HPDF_SetInfoAttr(pdf, HPDF_INFO_PRODUCER, "FreeCAD 3D PDF Export");
        HPDF_SetInfoAttr(pdf, HPDF_INFO_TITLE, "FreeCAD Hybrid 2D+3D Technical Drawing");
        
        HPDF_Page page = HPDF_AddPage(pdf);
        HPDF_Page_SetWidth(page, pageWidthPoints);
        HPDF_Page_SetHeight(page, pageHeightPoints);
        
        HPDF_Image backgroundImg = nullptr;
        if (!backgroundImagePath.empty()) {
            try {
                backgroundImg = HPDF_LoadPngImageFromFile(pdf, backgroundImagePath.c_str());
            } catch (...) {
                try {
                    backgroundImg = HPDF_LoadJpegImageFromFile(pdf, backgroundImagePath.c_str());
                } catch (...) {
                    Base::Console().warning("Failed to load background image: %s\n", backgroundImagePath.c_str());
                }
            }
            
            if (backgroundImg) {
                HPDF_Page_DrawImage(page, backgroundImg, 0, 0, pageWidthPoints, pageHeightPoints);
            }
        }
        
        HPDF_Image u3d = HPDF_LoadU3DFromMem(pdf, prcBuffer.data(), static_cast<HPDF_UINT>(prcSize));
        if (!u3d) {
            Base::Console().error("Failed to load PRC data into hybrid PDF\n");
            HPDF_Free(pdf);
            return false;
        }

        double scaledViewWidth = activeViewWidth * activeViewScale;
        double scaledViewHeight = activeViewHeight * activeViewScale;
        double viewWidthPoints = scaledViewWidth * 2.834645669;
        double viewHeightPoints = scaledViewHeight * 2.834645669;
        double viewXPoints = activeViewX * 2.834645669;
        double viewYPoints = activeViewY * 2.834645669;
        
        double halfWidthPoints = viewWidthPoints / 2.0;
        double halfHeightPoints = viewHeightPoints / 2.0;
        
        double annotLeft = viewXPoints - halfWidthPoints;
        double annotRight = viewXPoints + halfWidthPoints;
        double annotBottom = viewYPoints - halfHeightPoints;
        double annotTop = viewYPoints + halfHeightPoints;
        
        
        HPDF_Rect rect = {static_cast<HPDF_REAL>(annotLeft), 
                         static_cast<HPDF_REAL>(annotBottom), 
                         static_cast<HPDF_REAL>(annotRight), 
                         static_cast<HPDF_REAL>(annotTop)};
        HPDF_Annotation annot = HPDF_Page_Create3DAnnot(page, rect, HPDF_TRUE, HPDF_FALSE, u3d, NULL);
        if (!annot) {
            Base::Console().error("Failed to create 3D annotation in hybrid PDF\n");
            HPDF_Free(pdf);
            return false;
        }
        
        HPDF_Dict view = HPDF_Page_Create3DView(page, u3d, annot, "Default");
        if (!view) {
            Base::Console().error("Failed to create 3D view in hybrid PDF\n");
            HPDF_Free(pdf);
            return false;
        }
        
        HPDF_3DView_SetLighting(view, "CAD");
        HPDF_3DView_SetBackgroundColor(view, backgroundR, backgroundG, backgroundB);
        
        HPDF_3DView_SetCamera(view, 
            10.0, 10.0, 10.0,  // camera position
            0.0, 0.0, 0.0,     // target position  
            50.0,              // distance
            0.0);              // roll
        
        HPDF_U3D_SetDefault3DView(u3d, "Default");
        
        std::string pdfPath = outputPath + ".pdf";
        HPDF_STATUS saveResult = HPDF_SaveToFile(pdf, pdfPath.c_str());
        if (saveResult != HPDF_OK) {
            Base::Console().error("Failed to save hybrid PDF file: error code %d\n", saveResult);
            HPDF_Free(pdf);
            return false;
        }
        
        HPDF_Free(pdf);
        
        
        return true;
    }
    catch (const std::exception& e) {
        Base::Console().error("Error creating hybrid 2D+3D PDF: %s\n", e.what());
        return false;
    }
} 