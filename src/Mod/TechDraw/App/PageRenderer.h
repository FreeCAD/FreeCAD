/***************************************************************************
 *   Copyright (c) 2025 FreeCAD Developers                                 *
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

#ifndef TECHDRAW_PAGERENDERER_H
#define TECHDRAW_PAGERENDERER_H

#include <string>
#include <memory>

#include <Mod/TechDraw/TechDrawGlobal.h>

QT_BEGIN_NAMESPACE
class QPainter;
class QSvgGenerator;
class QPdfWriter;
QT_END_NAMESPACE

namespace TechDraw
{

class DrawPage;
class DrawTemplate;
class DrawView;

/**
 * @brief Headless renderer for TechDraw pages
 *
 * This class provides rendering capabilities for TechDraw pages without
 * requiring GUI components. It uses Qt's headless rendering capabilities
 * to generate PDF and SVG output directly from App layer objects.
 *
 * Phase 1 Implementation:
 * - Basic template rendering
 * - SVG string generation
 * - PDF/SVG file export stubs
 *
 * Future phases will add:
 * - View content rendering (dimensions, annotations, parts)
 * - Complete geometry processing
 * - Performance optimization
 */
class TechDrawExport PageRenderer
{
public:
    PageRenderer();
    explicit PageRenderer(const DrawPage* page);
    ~PageRenderer();

    // Phase 1: Core rendering methods
    bool renderToPDF(const std::string& filePath) const;
    bool renderToSVG(const std::string& filePath) const;
    std::string renderToSVGString() const;

    // Page association
    void setPage(const DrawPage* page);
    const DrawPage* getPage() const;

    // Rendering configuration
    void setResolution(double dpi);
    double getResolution() const;

    // Phase 1: Template-only rendering
    std::string renderTemplateToSVG() const;
    bool hasValidTemplate() const;

    // Phase 2+ (Future): View content rendering
    // std::string renderViewsToSVG() const;
    // void renderView(const DrawView* view, QPainter& painter) const;

    // Error handling
    std::string getLastError() const;
    bool hasError() const;

private:
    // Phase 1: Internal implementation
    void setupPainter(QPainter& painter) const;
    void renderTemplate(QPainter& painter) const;
    void calculatePageBounds(double& width, double& height) const;
    void setError(const std::string& message) const;
    void clearError() const { m_lastError.clear(); }

    // Phase 2+ (Future): View rendering internals
    // void renderViews(QPainter& painter) const;
    // void renderDimensions(QPainter& painter) const;
    // void renderAnnotations(QPainter& painter) const;

    // Member variables
    const DrawPage* m_page;
    double m_resolution;        // DPI for rendering
    mutable std::string m_lastError;

    // Phase 1: Template handling
    std::unique_ptr<class TemplateRenderer> m_templateRenderer;
};

/**
 * @brief Helper class for template processing without GUI dependencies
 *
 * Extracts template SVG processing from ViewProviderTemplate to App layer.
 * Handles template field substitution and SVG manipulation.
 */
class TechDrawExport TemplateRenderer
{
public:
    TemplateRenderer();
    explicit TemplateRenderer(const DrawTemplate* template_);
    ~TemplateRenderer();

    void setTemplate(const DrawTemplate* template_);
    const DrawTemplate* getTemplate() const;

    // Template rendering
    std::string renderToSVG() const;
    bool isValid() const;

    // Template field processing
    std::string processTemplateFields(const std::string& svgContent) const;

private:
    const DrawTemplate* m_template;

    // Internal template processing
    std::string loadTemplateSVG() const;
    std::string substituteFields(const std::string& content) const;
};

} // namespace TechDraw

#endif // TECHDRAW_PAGERENDERER_H
