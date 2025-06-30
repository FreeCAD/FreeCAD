#ifndef GUI_EXPORT3DPDF_H
#define GUI_EXPORT3DPDF_H

#include <vector>
#include <string>

namespace Gui {

/**
 * @brief Structure to hold tessellation data for a single object
 */
struct TessellationData {
    std::string name;
    std::vector<double> vertices;  // [x,y,z,x,y,z,...]
    std::vector<int> triangles;    // [v1,v2,v3,v1,v2,v3,...]
};

/**
 * @brief 3D PDF Export functionality using PRC format
 */
class Export3DPDF {
public:
    /**
     * @brief Convert tessellation data to PRC format and create 3D PDF
     * @param tessellationData Vector of tessellation data for multiple objects
     * @param outputPath Path where the 3D PDF should be saved
     * @return true if successful, false otherwise
     */
    static bool convertTessellationToPRC(const std::vector<TessellationData>& tessellationData, 
                                       const std::string& outputPath);

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
     * @return true if successful, false otherwise
     */
    static bool embedPRCInPDF(const std::string& prcPath, const std::string& pdfPath);

    // Note: getTessellationDataFromPython() removed - tessellation handled directly in CommandDoc.cpp

private:
    Export3DPDF() = default;  // Static class, no instances
};

} // namespace Gui

#endif // GUI_EXPORT3DPDF_H 