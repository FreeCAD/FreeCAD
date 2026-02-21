#ifndef EXPORT3DPDFCORE_H
#define EXPORT3DPDFCORE_H

#include <cstdint>
#include <vector>
#include <string>
#include "../Export3DPDFGlobal.h"

namespace Export3DPDF {

/**
 * @brief Conversion factor from millimeters to PDF points
 * 1 point = 1/72 inch, 1 inch = 25.4 mm
 * So 1 mm = 72/25.4 = 2.834645669... points
 */
constexpr double MM_TO_POINTS = 72.0 / 25.4;  // ~2.834645669

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
 * @brief 3D camera settings for the PDF view
 */
struct Export3DPDFExport CameraSettings {
    // Camera position (where the camera is located)
    double posX = 10.0;
    double posY = 10.0;
    double posZ = 10.0;
    // Target position (where the camera looks at)
    double targetX = 0.0;
    double targetY = 0.0;
    double targetZ = 0.0;
    // View parameters
    double distance = 50.0;   // Distance from target
    double roll = 0.0;        // Camera roll angle
};

/**
 * @brief Combined export settings for 3D PDF generation
 */
struct Export3DPDFExport PDFExportSettings {
    PDFPageSettings page;
    BackgroundColor background;
    ActiveViewSettings activeView;
    CameraSettings camera;
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
 * @brief Data for a single 3D region in the PDF (for multiple ActiveViews)
 */
struct Export3DPDFExport PDF3DRegion {
    std::vector<TessellationData> tessellationData;  // Geometry for this region
    ActiveViewSettings viewSettings;                  // Position and size
    BackgroundColor background;                       // Background color for 3D view
    CameraSettings camera;                            // Camera settings
};

/**
 * @brief 3D PDF Export functionality using PRC format
 *
 * This class uses in-memory buffers for PRC data to avoid intermediate files,
 * improving performance and eliminating file path encoding issues.
 */
class Export3DPDFExport Export3DPDFCore {
public:
    /**
     * @brief Create a 3D PDF from tessellation data (direct export)
     * @param tessellationData Vector of tessellation data for multiple objects
     * @param pdfPath Full path where the 3D PDF should be saved (including .pdf extension)
     * @param settings Export settings (page dimensions, background color, view settings)
     * @return true if successful, false otherwise
     */
    static bool exportToPDF(const std::vector<TessellationData>& tessellationData,
                            const std::string& pdfPath,
                            const PDFExportSettings& settings = PDFExportSettings());

    /**
     * @brief Create hybrid 2D+3D PDF with TechDraw page background and single 3D region
     * @param tessellationData Vector of tessellation data for 3D objects
     * @param pdfPath Full path where the hybrid PDF should be saved (including .pdf extension)
     * @param backgroundImagePath Path to background image (rendered TechDraw page)
     * @param settings Export settings (page dimensions, background color, view settings)
     * @return true if successful, false otherwise
     */
    static bool exportToHybridPDF(const std::vector<TessellationData>& tessellationData,
                                  const std::string& pdfPath,
                                  const std::string& backgroundImagePath,
                                  const PDFExportSettings& settings);

    /**
     * @brief Create hybrid 2D+3D PDF with TechDraw page background and multiple 3D regions
     * @param regions Vector of 3D regions, each with its own geometry, position and settings
     * @param pdfPath Full path where the hybrid PDF should be saved (including .pdf extension)
     * @param backgroundImagePath Path to background image (rendered TechDraw page)
     * @param pageSettings Page dimensions
     * @return true if successful, false otherwise
     */
    static bool exportToHybridPDFMultiRegion(const std::vector<PDF3DRegion>& regions,
                                              const std::string& pdfPath,
                                              const std::string& backgroundImagePath,
                                              const PDFPageSettings& pageSettings);

private:
    Export3DPDFCore() = default;  // Static class, no instances

    /**
     * @brief Create PRC data in memory from tessellation data
     * @param tessellationData Vector of tessellation data for multiple objects
     * @return Vector containing PRC binary data, empty on failure
     */
    static std::vector<uint8_t> createPRCBuffer(const std::vector<TessellationData>& tessellationData);

    /**
     * @brief Create 3D PDF from PRC buffer data (supports single or multiple 3D regions)
     * @param prcBuffers Vector of PRC buffers (one per 3D region)
     * @param pdfPath Path where the PDF should be saved
     * @param backgroundImagePath Optional path to background image (empty for no background)
     * @param regions Region settings (position, size, background, camera) for each annotation
     * @param pageSettings Page dimensions
     * @return true if successful, false otherwise
     */
    static bool createPDFFromBuffer(const std::vector<std::vector<uint8_t>>& prcBuffers,
                                    const std::string& pdfPath,
                                    const std::string& backgroundImagePath,
                                    const std::vector<PDF3DRegion>& regions,
                                    const PDFPageSettings& pageSettings);
};

} // namespace Export3DPDF

#endif // EXPORT3DPDFCORE_H 