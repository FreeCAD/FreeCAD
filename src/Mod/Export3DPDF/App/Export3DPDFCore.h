#ifndef EXPORT3DPDFCORE_H
#define EXPORT3DPDFCORE_H

#include <vector>
#include <string>
#include "../Export3DPDFGlobal.h"

namespace Export3DPDF {

/**
 * @brief PDF page dimensions in points (1 point = 1/72 inch)
 */
struct Export3DPDFExport PDFPageSettings {
    double widthPoints = 595.276;   // A4 width in points
    double heightPoints = 841.89;   // A4 height in points
};

/**
 * @brief RGB color for 3D view background (values 0.0-1.0)
 */
struct Export3DPDFExport BackgroundColor {
    double r = 0.5;
    double g = 0.5;
    double b = 0.5;
};

/**
 * @brief ActiveView positioning and scaling settings
 */
struct Export3DPDFExport ActiveViewSettings {
    double x = 0.0;           // X position in mm
    double y = 0.0;           // Y position in mm
    double scale = 1.0;       // Scale factor
    double width = 100.0;     // Width in mm
    double height = 100.0;    // Height in mm
};

/**
 * @brief Combined export settings for 3D PDF generation
 */
struct Export3DPDFExport PDFExportSettings {
    PDFPageSettings page;
    BackgroundColor background;
    ActiveViewSettings activeView;
};

/**
 * @brief Structure to hold material properties for an object
 */
struct Export3DPDFExport MaterialData {
    std::string name = "Default";
    // Color properties (RGBA format: r,g,b,a values between 0.0 and 1.0)
    double ambientColor[4] = {0.333333, 0.333333, 0.333333, 1.0};   // AmbientColor
    double diffuseColor[4] = {0.978431, 0.709804, 0.741176, 1.0};   // DiffuseColor
    double emissiveColor[4] = {0.0, 0.0, 0.0, 1.0};                 // EmissiveColor
    double specularColor[4] = {0.533333, 0.533333, 0.533333, 1.0};  // SpecularColor
    double shininess = 0.9;                                          // Shininess
    double transparency = 0.0;                                       // Transparency
    
    // Helper method to get alpha (opacity) from transparency
    double getAlpha() const {
        return 1.0 - transparency;  // FreeCAD transparency: 0=opaque, 1=transparent
    }
};

/**
 * @brief Structure to hold tessellation data for a single object
 */
struct Export3DPDFExport TessellationData {
    std::string name;
    std::vector<double> vertices;  // [x,y,z,x,y,z,...]
    std::vector<int> triangles;    // [v1,v2,v3,v1,v2,v3,...]
    MaterialData material;         // Material properties for this object
};

/**
 * @brief 3D PDF Export functionality using PRC format
 */
class Export3DPDFExport Export3DPDFCore {
public:
    /**
     * @brief Convert tessellation data to PRC format and create 3D PDF
     * @param tessellationData Vector of tessellation data for multiple objects
     * @param outputPath Path where the 3D PDF should be saved (without extension)
     * @param settings Export settings (page dimensions, background color, view settings)
     * @return true if successful, false otherwise
     */
    static bool convertTessellationToPRC(const std::vector<TessellationData>& tessellationData,
                                         const std::string& outputPath,
                                         const PDFExportSettings& settings = PDFExportSettings());

    /**
     * @brief Create a PRC file from tessellation data
     * @param tessellationData Vector of tessellation data for multiple objects
     * @param prcPath Path where the PRC file should be saved
     * @return Path to created file on success, empty string on failure
     */
    static std::string createPRCFile(const std::vector<TessellationData>& tessellationData,
                                     const std::string& prcPath);

    /**
     * @brief Create a 3D PDF from PRC file data
     * @param prcPath Path to the PRC file
     * @param pdfPath Path where the 3D PDF should be saved
     * @param settings Export settings (page dimensions, background color, view settings)
     * @return true if successful, false otherwise
     */
    static bool embedPRCInPDF(const std::string& prcPath,
                              const std::string& pdfPath,
                              const PDFExportSettings& settings = PDFExportSettings());

    /**
     * @brief Create hybrid 2D+3D PDF with complete TechDraw page and 3D content
     * @param tessellationData Vector of tessellation data for 3D objects
     * @param outputPath Path where the hybrid PDF should be saved (without extension)
     * @param backgroundImagePath Path to background image (rendered TechDraw page)
     * @param settings Export settings (page dimensions, background color, view settings)
     * @return true if successful, false otherwise
     */
    static bool createHybrid3DPDF(const std::vector<TessellationData>& tessellationData,
                                  const std::string& outputPath,
                                  const std::string& backgroundImagePath,
                                  const PDFExportSettings& settings);

private:
    Export3DPDFCore() = default;  // Static class, no instances
};

} // namespace Export3DPDF

#endif // EXPORT3DPDFCORE_H 