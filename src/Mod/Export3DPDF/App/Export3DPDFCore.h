#ifndef EXPORT3DPDFCORE_H
#define EXPORT3DPDFCORE_H

#include <vector>
#include <string>
#include "../Export3DPDFGlobal.h"

namespace Export3DPDF {

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
     * @param outputPath Path where the 3D PDF should be saved
     * @param pageWidthPoints Page width in points (default: A4 width)
     * @param pageHeightPoints Page height in points (default: A4 height)
     * @param backgroundR Background red component (0.0-1.0, default: gray)
     * @param backgroundG Background green component (0.0-1.0, default: gray)
     * @param backgroundB Background blue component (0.0-1.0, default: gray)
     * @return true if successful, false otherwise
     */
        static bool convertTessellationToPRC(const std::vector<TessellationData>& tessellationData, 
                                        const std::string& outputPath,
                                        double pageWidthPoints = 595.276,
                                        double pageHeightPoints = 841.89,
                                        double backgroundR = 0.5,
                                        double backgroundG = 0.5,
                                        double backgroundB = 0.5,
                                        double activeViewX = 0.0,
                                        double activeViewY = 0.0,
                                        double activeViewScale = 1.0,
                                        double activeViewWidth = 100.0,
                                        double activeViewHeight = 100.0);

    /**
     * @brief Create a PRC file from tessellation data
     * @param tessellationData Vector of tessellation data for multiple objects
     * @param prcPath Path where the PRC file should be saved
     * @return true if successful, false otherwise
     */
    static std::string createPRCFile(const std::vector<TessellationData>& tessellationData,
                            const std::string& prcPath);

    /**
     * @brief Create a 3D PDF from PRC file data
     * @param prcPath Path to the PRC file
     * @param pdfPath Path where the 3D PDF should be saved
     * @param pageWidthPoints Page width in points (default: A4 width)
     * @param pageHeightPoints Page height in points (default: A4 height)
     * @param backgroundR Background red component (0.0-1.0, default: gray)
     * @param backgroundG Background green component (0.0-1.0, default: gray)
     * @param backgroundB Background blue component (0.0-1.0, default: gray)
     * @return true if successful, false otherwise
     */
    static bool embedPRCInPDF(const std::string& prcPath, const std::string& pdfPath,
                             double pageWidthPoints = 595.276,
                             double pageHeightPoints = 841.89,
                             double backgroundR = 0.5,
                             double backgroundG = 0.5,
                             double backgroundB = 0.5,
                             double activeViewX = 0.0,
                             double activeViewY = 0.0,
                             double activeViewScale = 1.0,
                             double activeViewWidth = 100.0,
                             double activeViewHeight = 100.0);

private:
    Export3DPDFCore() = default;  // Static class, no instances
};

} // namespace Export3DPDF

#endif // EXPORT3DPDFCORE_H 