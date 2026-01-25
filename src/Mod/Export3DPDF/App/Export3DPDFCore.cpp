#include "PreCompiled.h"

#ifndef _PreComp_
# include <cstdio>
# include <fstream>
# include <sstream>
# include <iostream>
# include <vector>
# include <string>
# include <memory>
# include <filesystem>
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

namespace {

// Libharu error handler - logs errors to FreeCAD console
void hpdfErrorHandler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void* /*user_data*/)
{
    Base::Console().error("libharu error: error_no=0x%04X, detail_no=%d\n",
                         static_cast<unsigned int>(error_no),
                         static_cast<int>(detail_no));
}

// Cross-platform file reading that handles Unicode paths on Windows
bool readFileToBuffer(const std::string& filePath, std::vector<HPDF_BYTE>& buffer)
{
    try {
        std::filesystem::path fsPath = Base::FileInfo::stringToPath(filePath);
        std::ifstream file(fsPath, std::ios::binary);
        if (!file.is_open()) {
            Base::Console().error("Failed to open file for reading: %s\n", filePath.c_str());
            return false;
        }

        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size <= 0) {
            Base::Console().error("File is empty or error getting size: %s\n", filePath.c_str());
            return false;
        }

        buffer.resize(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            Base::Console().error("Failed to read file contents: %s\n", filePath.c_str());
            return false;
        }

        return true;
    }
    catch (const std::exception& e) {
        Base::Console().error("Exception reading file '%s': %s\n", filePath.c_str(), e.what());
        return false;
    }
}

// Cross-platform file writing that handles Unicode paths on Windows
bool writeBufferToFile(const std::string& filePath, const void* data, size_t size)
{
    try {
        std::filesystem::path fsPath = Base::FileInfo::stringToPath(filePath);
        std::ofstream file(fsPath, std::ios::binary);
        if (!file.is_open()) {
            Base::Console().error("Failed to open file for writing: %s\n", filePath.c_str());
            return false;
        }

        file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
        if (!file.good()) {
            Base::Console().error("Failed to write file contents: %s\n", filePath.c_str());
            return false;
        }

        return true;
    }
    catch (const std::exception& e) {
        Base::Console().error("Exception writing file '%s': %s\n", filePath.c_str(), e.what());
        return false;
    }
}

// Save libharu PDF to file using cross-platform file I/O
bool savePdfToFile(HPDF_Doc pdf, const std::string& pdfPath)
{
    // Save PDF to internal memory stream
    HPDF_STATUS status = HPDF_SaveToStream(pdf);
    if (status != HPDF_OK) {
        Base::Console().error("Failed to save PDF to stream: error code 0x%04X\n", status);
        return false;
    }

    // Get the size of the PDF data
    HPDF_UINT32 streamSize = HPDF_GetStreamSize(pdf);
    if (streamSize == 0) {
        Base::Console().error("PDF stream is empty\n");
        return false;
    }

    // Rewind the stream to the beginning
    status = HPDF_ResetStream(pdf);
    if (status != HPDF_OK) {
        Base::Console().error("Failed to reset PDF stream: error code 0x%04X\n", status);
        return false;
    }

    // Allocate buffer and read PDF data
    std::vector<HPDF_BYTE> pdfBuffer(streamSize);
    HPDF_UINT32 readSize = streamSize;
    status = HPDF_ReadFromStream(pdf, pdfBuffer.data(), &readSize);
    if (status != HPDF_OK) {
        Base::Console().error("Failed to read PDF from stream: error code 0x%04X\n", status);
        return false;
    }

    // Write to file using cross-platform file I/O
    return writeBufferToFile(pdfPath, pdfBuffer.data(), readSize);
}

} // anonymous namespace


// Public API

bool Export3DPDFCore::exportToPDF(const std::vector<TessellationData>& tessellationData,
                                   const std::string& pdfPath,
                                   const PDFExportSettings& settings)
{
    // Create PRC data in memory
    std::vector<uint8_t> prcBuffer = createPRCBuffer(tessellationData);
    if (prcBuffer.empty()) {
        Base::Console().error("Failed to create PRC data\n");
        return false;
    }

    // Create PDF directly from buffer (no background image for direct export)
    return createPDFFromBuffer(prcBuffer, pdfPath, "", settings);
}

bool Export3DPDFCore::exportToHybridPDF(const std::vector<TessellationData>& tessellationData,
                                         const std::string& pdfPath,
                                         const std::string& backgroundImagePath,
                                         const PDFExportSettings& settings)
{
    // Create PRC data in memory
    std::vector<uint8_t> prcBuffer = createPRCBuffer(tessellationData);
    if (prcBuffer.empty()) {
        Base::Console().error("Failed to create PRC data for hybrid PDF\n");
        return false;
    }

    // Create PDF with background image
    return createPDFFromBuffer(prcBuffer, pdfPath, backgroundImagePath, settings);
}


// Private implementation

std::vector<uint8_t> Export3DPDFCore::createPRCBuffer(const std::vector<TessellationData>& tessellationData)
{
    try {
        // Use ostringstream as in-memory output stream
        std::ostringstream prcStream(std::ios::binary);

        oPRCFile prcFile(prcStream, 1.0);

        for (const auto& objData : tessellationData) {
            if (!objData.vertices.empty() && !objData.triangles.empty()) {

                std::unique_ptr<PRC3DTess> prc3DTess = std::make_unique<PRC3DTess>();

                prc3DTess->has_faces = true;
                prc3DTess->has_loops = false;

                prc3DTess->coordinates = objData.vertices;

                std::unique_ptr<PRCTessFace> tessFace = std::make_unique<PRCTessFace>();
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

                PRCTessFace* tessFaceRaw = tessFace.release();
                prc3DTess->addTessFace(tessFaceRaw);

                PRC3DTess* prc3DTessRaw = prc3DTess.release();
                uint32_t tessIndex = prcFile.add3DTess(prc3DTessRaw);

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
            Base::Console().error("Failed to finalize PRC data\n");
            return {};
        }

        // Get the string from the stream and convert to vector
        std::string prcData = prcStream.str();
        return std::vector<uint8_t>(prcData.begin(), prcData.end());
    }
    catch (const std::exception& e) {
        Base::Console().error("Failed to create PRC buffer: %s\n", e.what());
        return {};
    }
}

bool Export3DPDFCore::createPDFFromBuffer(const std::vector<uint8_t>& prcBuffer,
                                           const std::string& pdfPath,
                                           const std::string& backgroundImagePath,
                                           const PDFExportSettings& settings)
{
    try {
        HPDF_Doc pdf = HPDF_New(hpdfErrorHandler, nullptr);
        if (!pdf) {
            Base::Console().error("Failed to create PDF document\n");
            return false;
        }

        // Set PDF metadata
        if (backgroundImagePath.empty()) {
            HPDF_SetInfoAttr(pdf, HPDF_INFO_PRODUCER, "FreeCAD 3D PDF Export");
            HPDF_SetInfoAttr(pdf, HPDF_INFO_TITLE, "FreeCAD 3D Model");
        } else {
            HPDF_SetInfoAttr(pdf, HPDF_INFO_PRODUCER, "FreeCAD 3D PDF Export");
            HPDF_SetInfoAttr(pdf, HPDF_INFO_TITLE, "FreeCAD Hybrid 2D+3D Technical Drawing");
        }

        // Create page with specified dimensions
        HPDF_Page page = HPDF_AddPage(pdf);
        HPDF_Page_SetWidth(page, settings.page.widthPoints);
        HPDF_Page_SetHeight(page, settings.page.heightPoints);

        // Add background image if provided (for hybrid PDF)
        if (!backgroundImagePath.empty()) {
            std::vector<HPDF_BYTE> imageBuffer;
            if (readFileToBuffer(backgroundImagePath, imageBuffer)) {
                // Try PNG first, then JPEG
                HPDF_Image backgroundImg = HPDF_LoadPngImageFromMem(pdf, imageBuffer.data(),
                                                                     static_cast<HPDF_UINT>(imageBuffer.size()));
                if (!backgroundImg) {
                    HPDF_ResetError(pdf);
                    backgroundImg = HPDF_LoadJpegImageFromMem(pdf, imageBuffer.data(),
                                                               static_cast<HPDF_UINT>(imageBuffer.size()));
                }

                if (backgroundImg) {
                    HPDF_Page_DrawImage(page, backgroundImg, 0, 0,
                                        settings.page.widthPoints, settings.page.heightPoints);
                } else {
                    Base::Console().warning("Failed to decode background image: %s\n", backgroundImagePath.c_str());
                    HPDF_ResetError(pdf);
                }
            } else {
                Base::Console().warning("Failed to read background image: %s\n", backgroundImagePath.c_str());
            }
        }

        // Load PRC data as U3D
        HPDF_Image u3d = HPDF_LoadU3DFromMem(pdf, prcBuffer.data(), static_cast<HPDF_UINT>(prcBuffer.size()));
        if (!u3d) {
            Base::Console().error("Failed to load PRC data into PDF\n");
            HPDF_Free(pdf);
            return false;
        }

        // Calculate annotation rectangle from ActiveView settings
        const auto& av = settings.activeView;
        double scaledViewWidth = av.width * av.scale;
        double scaledViewHeight = av.height * av.scale;
        double viewWidthPoints = scaledViewWidth * MM_TO_POINTS;
        double viewHeightPoints = scaledViewHeight * MM_TO_POINTS;
        double viewXPoints = av.x * MM_TO_POINTS;
        double viewYPoints = av.y * MM_TO_POINTS;

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

        // Create 3D annotation
        HPDF_Annotation annot = HPDF_Page_Create3DAnnot(page, rect, HPDF_TRUE, HPDF_FALSE, u3d, NULL);
        if (!annot) {
            Base::Console().error("Failed to create 3D annotation\n");
            HPDF_Free(pdf);
            return false;
        }

        // Create 3D view
        HPDF_Dict view = HPDF_Page_Create3DView(page, u3d, annot, "Default");
        if (!view) {
            Base::Console().error("Failed to create 3D view\n");
            HPDF_Free(pdf);
            return false;
        }

        // Configure view settings
        HPDF_3DView_SetLighting(view, "CAD");
        HPDF_3DView_SetBackgroundColor(view, settings.background.r, settings.background.g, settings.background.b);

        const auto& cam = settings.camera;
        HPDF_3DView_SetCamera(view,
            cam.posX, cam.posY, cam.posZ,
            cam.targetX, cam.targetY, cam.targetZ,
            cam.distance,
            cam.roll);

        HPDF_U3D_SetDefault3DView(u3d, "Default");

        // Save PDF to file
        bool saveSuccess = savePdfToFile(pdf, pdfPath);
        HPDF_Free(pdf);

        if (!saveSuccess) {
            Base::Console().error("Failed to save PDF file: %s\n", pdfPath.c_str());
            return false;
        }

        return true;
    }
    catch (const std::exception& e) {
        Base::Console().error("Error creating PDF: %s\n", e.what());
        return false;
    }
}
