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
 * - Template rendering via QSvgRenderer (metadata + template-id tagging)
 * - DPI-aware PDF/SVG emission that matches template mm sizing
 * - Qt bootstrap through App::QtApp::ensureGuiApplication (fails fast on core-only)
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

    // Phase 1: Template-only rendering (Qt-backed, offscreen-capable)
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
    bool renderTemplate(QPainter& painter) const;
    void calculatePageBounds(double& width, double& height) const;
    void setError(const std::string& message) const;
    void clearError() const { m_lastError.clear(); }
    std::string postProcessSvg(std::string&& svg) const;
    void updateTemplateRenderer() const;

    // Member variables
    const DrawPage* m_page;
    double m_resolution;        // DPI for rendering
    mutable std::string m_lastError;

    // Phase 1: Template handling
    mutable std::unique_ptr<class TemplateRenderer> m_templateRenderer;
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
    const std::string& getTemplateIdentifier() const;

    // Template rendering
    std::string renderToSVG() const;
    bool isValid() const;
    bool canRender() const;
    bool isParametric() const;
    std::string getUnsupportedReason() const;

    // Template field processing
    std::string processTemplateFields(const std::string& svgContent) const;

private:
    const DrawTemplate* m_template;
    mutable std::string m_lastTemplateIdentifier;

    // Internal template processing
    std::string loadTemplateSVG() const;
    std::string substituteFields(const std::string& content) const;
    void cacheTemplateIdentifier(const std::string& svgContent) const;
};

} // namespace TechDraw

#endif // TECHDRAW_PAGERENDERER_H
