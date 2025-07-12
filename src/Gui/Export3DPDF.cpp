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
#include "../3rdParty/libPRC/src/asymptote/oPRCFile.h"
#include "../3rdParty/libPRC/src/asymptote/writePRC.h"
#include "../3rdParty/libPRC/src/asymptote/PRC.h"
#include "../3rdParty/libharu/include/hpdf.h"
#include "../3rdParty/libharu/include/hpdf_u3d.h"
#include "Export3DPDF.h"

using namespace Gui;

bool Export3DPDF::convertTessellationToPRC(const std::vector<TessellationData>& tessellationData, const std::string& outputPath)
{
    try {
        Base::Console().message("Converting %zu objects to PRC format...\n", tessellationData.size());
        
        // Create PRC file
        std::string prcPath = outputPath + ".prc";
        std::string result = createPRCFile(tessellationData, prcPath);
        if (result.empty()) {
            Base::Console().error("Failed to create PRC file\n");
            return false;
        }
        
        Base::Console().message("Successfully created PRC file: %s\n", prcPath.c_str());
        
        // Create 3D PDF from PRC
        std::string pdfPath = outputPath + ".pdf";
        if (!embedPRCInPDF(prcPath, pdfPath)) {
            Base::Console().error("Failed to create 3D PDF\n");
            return false;
        }
        
        Base::Console().message("Successfully created 3D PDF: %s\n", pdfPath.c_str());
        return true;
    }
    catch (const std::exception& e) {
        Base::Console().error("Error in convertTessellationToPRC: %s\n", e.what());
        return false;
    }
}

std::string Export3DPDF::createPRCFile(const std::vector<TessellationData>& tessellationData, const std::string& prcPath)
{
    try {
        Base::Console().message("Creating PRC file: %s\n", prcPath.c_str());
        
        // Create output stream for PRC file
        std::ofstream prcStream(prcPath.c_str(), std::ios::binary);
        if (!prcStream.is_open()) {
            Base::Console().error("Failed to open output stream for PRC file\n");
            return "";
        }
        
        // Create libPRC file object
        oPRCFile prcFile(prcStream, 1.0); // 1.0 = unit scale
        
        // Add tessellation data to PRC file
        for (const auto& objData : tessellationData) {
            if (!objData.vertices.empty() && !objData.triangles.empty()) {
                Base::Console().message("Processing object '%s': %zu vertices, %zu triangles\n", 
                    objData.name.c_str(), objData.vertices.size() / 3, objData.triangles.size() / 3);
                
                // Create PRC3DTess object
                PRC3DTess* prc3DTess = new PRC3DTess();
                
                // Set up tessellation flags
                prc3DTess->has_faces = true;
                prc3DTess->has_loops = false;
                
                // Populate coordinates (x1,y1,z1, x2,y2,z2, ...)
                prc3DTess->coordinates = objData.vertices;
                
                // Create face definition for proper surface representation
                PRCTessFace* tessFace = new PRCTessFace();
                tessFace->used_entities_flag = PRC_FACETESSDATA_Triangle; // Basic triangles
                tessFace->start_triangulated = 0; // Start from first triangle
                tessFace->sizes_triangulated.push_back(0); // Initialize with 0, will be updated
                tessFace->is_rgba = false; // No per-vertex colors for now
                tessFace->behaviour = PRC_GRAPHICS_Show; // Make face visible
                tessFace->number_of_texture_coordinate_indexes = 0; // No texture coordinates
                
                // Add triangle indices in the pattern expected by PRC
                uint32_t triangleCount = 0;
                for (size_t i = 0; i < objData.triangles.size(); i += 3) {
                    // For each triangle, add the 3 vertex indices
                    prc3DTess->triangulated_index.push_back(static_cast<uint32_t>(objData.triangles[i] * 3));     // vertex index * 3
                    prc3DTess->triangulated_index.push_back(static_cast<uint32_t>(objData.triangles[i+1] * 3));   // vertex index * 3
                    prc3DTess->triangulated_index.push_back(static_cast<uint32_t>(objData.triangles[i+2] * 3));   // vertex index * 3
                    triangleCount++;
                }
                
                // Update the face with the actual triangle count
                tessFace->sizes_triangulated[0] = triangleCount;
                
                // Add the tessellated face to the 3D tessellation
                prc3DTess->addTessFace(tessFace);
                
                // Add tessellation to PRC file
                uint32_t tessIndex = prcFile.add3DTess(prc3DTess);
                Base::Console().message("Added tessellation for '%s' at index %u\n", 
                    objData.name.c_str(), tessIndex);
                
                // Create material using actual object properties
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
                // Print the material properties
                Base::Console().message("Material properties: %s\n", objData.material.name.c_str());
                Base::Console().message("  Ambient: (%.3f, %.3f, %.3f, %.3f)\n", 
                    ambient.R, ambient.G, ambient.B, ambient.A);
                Base::Console().message("  Diffuse: (%.3f, %.3f, %.3f, %.3f)\n", 
                    diffuse.R, diffuse.G, diffuse.B, diffuse.A);
                Base::Console().message("  Emissive: (%.3f, %.3f, %.3f, %.3f)\n", 
                    emissive.R, emissive.G, emissive.B, emissive.A);
                Base::Console().message("  Specular: (%.3f, %.3f, %.3f, %.3f)\n", 
                    specular.R, specular.G, specular.B, specular.A);
                Base::Console().message("  Shininess: %.3f\n", objData.material.shininess);
                Base::Console().message("  Alpha: %.3f\n", objData.material.getAlpha());
                
                PRCmaterial material(ambient, diffuse, emissive, specular, 
                                   objData.material.getAlpha(),    // alpha (opacity)
                                   objData.material.shininess);   // shininess
                
                // Add material and apply it to the tessellation
                uint32_t materialIndex = prcFile.addMaterial(material);
                Base::Console().message("Added material '%s' for object '%s' at index %u\n", 
                    objData.material.name.c_str(), objData.name.c_str(), materialIndex);
                Base::Console().message("  Diffuse: (%.3f, %.3f, %.3f, %.3f), Shininess: %.3f, Alpha: %.3f\n",
                    diffuse.R, diffuse.G, diffuse.B, diffuse.A, 
                    objData.material.shininess, objData.material.getAlpha());
                
                // Apply material to tessellation (no transformation)
                prcFile.useMesh(tessIndex, materialIndex, NULL);
                Base::Console().message("Applied material to tessellation for '%s'\n", 
                    objData.name.c_str());
                
            } else {
                Base::Console().warning("Skipping object '%s' - no geometry data\n", objData.name.c_str());
            }
        }
        
        // Finalize the PRC file
        if (!prcFile.finish()) {
            Base::Console().error("Failed to finalize PRC file\n");
            prcStream.close();
            return "";
        }
        
        prcStream.close();
        Base::Console().message("PRC file created successfully using libPRC\n");
        return prcPath;
    }
    catch (const std::exception& e) {
        Base::Console().error("Error creating PRC file: %s\n", e.what());
        return "";
    }
}

bool Export3DPDF::embedPRCInPDF(const std::string& prcPath, const std::string& pdfPath)
{
    try {
        Base::Console().message("Creating 3D PDF from PRC file: %s\n", prcPath.c_str());
        
        // Read PRC file into memory
        std::ifstream prcFile(prcPath, std::ios::binary);
        if (!prcFile.is_open()) {
            Base::Console().error("Failed to open PRC file: %s\n", prcPath.c_str());
            return false;
        }
        
        // Get file size
        prcFile.seekg(0, std::ios::end);
        size_t prcSize = prcFile.tellg();
        prcFile.seekg(0, std::ios::beg);
        
        // Read PRC data into buffer
        std::vector<HPDF_BYTE> prcBuffer(prcSize);
        prcFile.read(reinterpret_cast<char*>(prcBuffer.data()), prcSize);
        prcFile.close();
        
        Base::Console().message("Loaded PRC data: %zu bytes\n", prcSize);
        
        // Create PDF document
        HPDF_Doc pdf = HPDF_New(NULL, NULL);
        if (!pdf) {
            Base::Console().error("Failed to create PDF document\n");
            return false;
        }
        
        // Set PDF metadata
        HPDF_SetInfoAttr(pdf, HPDF_INFO_PRODUCER, "FreeCAD 3D PDF Export");
        HPDF_SetInfoAttr(pdf, HPDF_INFO_TITLE, "FreeCAD 3D Model");
        
        // Create a page (A4 landscape)
        HPDF_Page page = HPDF_AddPage(pdf);
        HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_LANDSCAPE);
        
        // Load PRC/U3D data into PDF
        HPDF_Image u3d = HPDF_LoadU3DFromMem(pdf, prcBuffer.data(), static_cast<HPDF_UINT>(prcSize));
        if (!u3d) {
            Base::Console().error("Failed to load PRC data into PDF\n");
            HPDF_Free(pdf);
            return false;
        }
        
        // Create 3D annotation (covers most of the page)
        HPDF_Rect rect = {50, 50, 750, 550}; // left, bottom, right, top
        HPDF_Annotation annot = HPDF_Page_Create3DAnnot(page, rect, HPDF_TRUE, HPDF_FALSE, u3d, NULL);
        if (!annot) {
            Base::Console().error("Failed to create 3D annotation\n");
            HPDF_Free(pdf);
            return false;
        }
        
        // Create 3D view configuration
        HPDF_Dict view = HPDF_Page_Create3DView(page, u3d, annot, "Default");
        if (!view) {
            Base::Console().error("Failed to create 3D view\n");
            HPDF_Free(pdf);
            return false;
        }
        
        // Configure 3D view settings
        HPDF_3DView_SetLighting(view, "CAD");
        HPDF_3DView_SetBackgroundColor(view, 0.5, 0.5, 0.5); // Gray background
        
        // Set camera position (isometric-like view)
        HPDF_3DView_SetCamera(view, 
            10.0, 10.0, 10.0,  // camera position
            0.0, 0.0, 0.0,     // target position  
            50.0,              // distance
            0.0);              // roll
        
        // Set this as the default view
        HPDF_U3D_SetDefault3DView(u3d, "Default");
        
        // Save PDF to file
        HPDF_STATUS result = HPDF_SaveToFile(pdf, pdfPath.c_str());
        if (result != HPDF_OK) {
            Base::Console().error("Failed to save PDF file: error code %d\n", result);
            HPDF_Free(pdf);
            return false;
        }
        
        // Clean up
        HPDF_Free(pdf);
        
        Base::Console().message("3D PDF created successfully using libHaru\n");
        return true;
    }
    catch (const std::exception& e) {
        Base::Console().error("Error creating 3D PDF: %s\n", e.what());
        return false;
    }
}

