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
        if (!embedPRCInPDF(prcPath, pdfPath, pageWidthPoints, pageHeightPoints, backgroundR, backgroundG, backgroundB, activeViewX, activeViewY, activeViewScale, activeViewWidth, activeViewHeight)) {
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

std::string Export3DPDFCore::createPRCFile(const std::vector<TessellationData>& tessellationData, const std::string& prcPath)
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

bool Export3DPDFCore::embedPRCInPDF(const std::string& prcPath, const std::string& pdfPath, double pageWidthPoints, double pageHeightPoints, double backgroundR, double backgroundG, double backgroundB, double activeViewX, double activeViewY, double activeViewScale, double activeViewWidth, double activeViewHeight)
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
        
        // Create a page with dimensions from TechDraw page
        HPDF_Page page = HPDF_AddPage(pdf);
        HPDF_Page_SetWidth(page, pageWidthPoints);
        HPDF_Page_SetHeight(page, pageHeightPoints);
        
        Base::Console().message("PDF page size: %.1f x %.1f points (%.1f x %.1f mm)\n",
                               pageWidthPoints, pageHeightPoints,
                               pageWidthPoints / 2.834645669, pageHeightPoints / 2.834645669);
        Base::Console().message("PDF background color: (%.3f, %.3f, %.3f)\n",
                               backgroundR, backgroundG, backgroundB);
        
        // Load PRC/U3D data into PDF
        HPDF_Image u3d = HPDF_LoadU3DFromMem(pdf, prcBuffer.data(), static_cast<HPDF_UINT>(prcSize));
        if (!u3d) {
            Base::Console().error("Failed to load PRC data into PDF\n");
            HPDF_Free(pdf);
            return false;
        }
        
        // Calculate 3D annotation position based on ActiveView position and size
        // Convert ActiveView position and dimensions from TechDraw coordinates to PDF coordinates
        // TechDraw: Y-axis points up, origin at bottom-left, units in mm
        // PDF: Y-axis points up, origin at bottom-left, units in points
        
        double viewWidthPoints = activeViewWidth * 2.834645669;  // Convert mm to points
        double viewHeightPoints = activeViewHeight * 2.834645669;
        double viewXPoints = activeViewX * 2.834645669;
        double viewYPoints = activeViewY * 2.834645669;
        
        // Calculate the annotation rectangle in PDF coordinate system
        // In TechDraw: (X,Y) represents the CENTER of the ActiveView, not bottom-left corner
        // So we need to calculate the actual bounds by offsetting by half width/height
        double halfWidthPoints = viewWidthPoints / 2.0;
        double halfHeightPoints = viewHeightPoints / 2.0;
        
        // Calculate bounds around the center point
        double annotLeft = viewXPoints - halfWidthPoints;
        double annotRight = viewXPoints + halfWidthPoints;
        
        // For Y coordinate: TechDraw Y increases upward, PDF Y also increases upward
        // Both have origin at bottom-left, so NO coordinate flipping needed
        double annotBottom = viewYPoints - halfHeightPoints;
        double annotTop = viewYPoints + halfHeightPoints;
        
        Base::Console().message("ActiveView positioning calculation:\n");
        Base::Console().message("  - TechDraw center position: (%.2f, %.2f) mm, size: %.2f x %.2f mm\n", 
                               activeViewX, activeViewY, activeViewWidth, activeViewHeight);
        Base::Console().message("  - PDF center position: (%.2f, %.2f) points, size: %.2f x %.2f points\n", 
                               viewXPoints, viewYPoints, viewWidthPoints, viewHeightPoints);
        Base::Console().message("  - 3D annotation bounds: left=%.2f, bottom=%.2f, right=%.2f, top=%.2f\n", 
                               annotLeft, annotBottom, annotRight, annotTop);
        Base::Console().message("  - ActiveView bounds in TechDraw: X(%.2f to %.2f), Y(%.2f to %.2f) mm\n",
                               activeViewX - activeViewWidth/2, activeViewX + activeViewWidth/2,
                               activeViewY - activeViewHeight/2, activeViewY + activeViewHeight/2);
        
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
        
        // Create 3D view configuration
        HPDF_Dict view = HPDF_Page_Create3DView(page, u3d, annot, "Default");
        if (!view) {
            Base::Console().error("Failed to create 3D view\n");
            HPDF_Free(pdf);
            return false;
        }
        
        // Configure 3D view settings
        HPDF_3DView_SetLighting(view, "CAD");
        HPDF_3DView_SetBackgroundColor(view, backgroundR, backgroundG, backgroundB);
        
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