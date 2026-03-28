#ifndef EXPORT3DPDFGUI_H
#define EXPORT3DPDFGUI_H

#include "../Export3DPDFGlobal.h"

namespace Export3DPDFGui {

/**
 * @brief GUI utilities for the Export3DPDF module
 */
class Export3DPDFGuiExport Export3DPDFGuiUtils {
public:
    /**
     * @brief Initialize GUI components for Export3DPDF
     */
    static void initialize();

private:
    Export3DPDFGuiUtils() = default;  // Static class, no instances
};

} // namespace Export3DPDFGui

#endif // EXPORT3DPDFGUI_H 