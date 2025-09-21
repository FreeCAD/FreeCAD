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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QPainter>
#include <QPdfWriter>
#include <QSvgGenerator>
#include <QSvgRenderer>
#include <QPageSize>
#include <QTextStream>
#endif

#include <algorithm>
#include <cmath>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>

#include "PageRenderer.h"
#include "DrawPage.h"
#include "DrawTemplate.h"
#include "DrawSVGTemplate.h"
#include "DrawParametricTemplate.h"

using namespace TechDraw;

namespace
{
constexpr const char* kPageRendererErrorPrefix = "PageRenderer: ";
}

//===========================================================================
// PageRenderer
//===========================================================================

PageRenderer::PageRenderer()
    : m_page(nullptr)
    , m_resolution(300.0)  // Default 300 DPI
    , m_templateRenderer(std::make_unique<TemplateRenderer>())
{
}

PageRenderer::PageRenderer(const DrawPage* page)
    : m_page(page)
    , m_resolution(300.0)
    , m_templateRenderer(std::make_unique<TemplateRenderer>())
{
    if (page && page->hasValidTemplate()) {
        auto* template_ = dynamic_cast<const DrawTemplate*>(page->Template.getValue());
        if (template_) {
            m_templateRenderer->setTemplate(template_);
        }
    }
}

PageRenderer::~PageRenderer() = default;

void PageRenderer::setPage(const DrawPage* page)
{
    m_page = page;
    clearError();

    if (page && page->hasValidTemplate()) {
        auto* template_ = dynamic_cast<const DrawTemplate*>(page->Template.getValue());
        if (template_) {
            m_templateRenderer->setTemplate(template_);
        }
    }
}

const DrawPage* PageRenderer::getPage() const
{
    return m_page;
}

void PageRenderer::setResolution(double dpi)
{
    if (dpi > 0.0) {
        m_resolution = dpi;
    }
}

double PageRenderer::getResolution() const
{
    return m_resolution;
}

bool PageRenderer::renderToPDF(const std::string& filePath) const
{
    clearError();

    if (!m_page) {
        setError("No page set for rendering");
        return false;
    }

    if (filePath.empty()) {
        setError("Empty file path provided");
        return false;
    }

    try {
        double width, height;
        calculatePageBounds(width, height);

        QPdfWriter pdfWriter(QString::fromStdString(filePath));
        pdfWriter.setResolution(static_cast<int>(m_resolution));

        // Convert from mm to points (1 mm = 2.83465 points)
        const double mmToPoints = 2.83465;
        pdfWriter.setPageSize(QPageSize(QSizeF(width * mmToPoints, height * mmToPoints),
                                       QPageSize::Point));
        pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));

        QPainter painter(&pdfWriter);
        if (!painter.isActive()) {
            setError("Failed to initialize PDF painter");
            return false;
        }

        setupPainter(painter);
        renderTemplate(painter);

        // Phase 2+: Add view rendering here
        // renderViews(painter);

        painter.end();

        Base::Console().message("PageRenderer: PDF exported to %s\n", filePath.c_str());
        return true;
    }
    catch (const std::exception& e) {
        setError(std::string("PDF rendering failed: ") + e.what());
        return false;
    }
}

bool PageRenderer::renderToSVG(const std::string& filePath) const
{
    clearError();

    if (!m_page) {
        setError("No page set for rendering");
        return false;
    }

    if (filePath.empty()) {
        setError("Empty file path provided");
        return false;
    }

    try {
        std::string svgContent = renderToSVGString();
        if (svgContent.empty()) {
            return false; // Error already set in renderToSVGString
        }

        QFile file(QString::fromStdString(filePath));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QString errorString = file.errorString();
            setError(std::string("Failed to open file for writing: ") + filePath +
                    " (" + errorString.toStdString() + ")");
            return false;
        }

        QTextStream stream(&file);
        stream << QString::fromStdString(svgContent);
        file.close();

        Base::Console().message("PageRenderer: SVG exported to %s\n", filePath.c_str());
        return true;
    }
    catch (const std::exception& e) {
        setError(std::string("SVG rendering failed: ") + e.what());
        return false;
    }
}

std::string PageRenderer::renderToSVGString() const
{
    clearError();

    if (!m_page) {
        setError("No page set for rendering");
        return std::string();
    }

    try {
        double width, height;
        calculatePageBounds(width, height);

        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);

        QSvgGenerator svgGenerator;
        svgGenerator.setOutputDevice(&buffer);

        const double dpi = m_resolution > 0.0 ? m_resolution : 72.0;
        const double mmToPixels = dpi / 25.4;

        // Prevent integer overflow by capping the maximum pixel dimensions
        const double maxPixelDimension = 1000000.0; // 1 megapixel per dimension
        const double rawPixelWidth = width * mmToPixels;
        const double rawPixelHeight = height * mmToPixels;

        if (rawPixelWidth > maxPixelDimension || rawPixelHeight > maxPixelDimension) {
            Base::Console().warning("PageRenderer: Calculated pixel dimensions too large (%.0f x %.0f), capping to prevent overflow\n",
                                  rawPixelWidth, rawPixelHeight);
        }

        const int pixelWidth = std::max(1, std::min(static_cast<int>(maxPixelDimension),
                                                   static_cast<int>(std::lround(rawPixelWidth))));
        const int pixelHeight = std::max(1, std::min(static_cast<int>(maxPixelDimension),
                                                    static_cast<int>(std::lround(rawPixelHeight))));

        svgGenerator.setResolution(static_cast<int>(dpi));
        svgGenerator.setSize(QSize(pixelWidth, pixelHeight));
        svgGenerator.setViewBox(QRect(0, 0, pixelWidth, pixelHeight));

        QPainter painter(&svgGenerator);
        if (!painter.isActive()) {
            setError("Failed to initialize SVG painter");
            return std::string();
        }

        setupPainter(painter);
        renderTemplate(painter);

        // Phase 2+: Add view rendering here
        // renderViews(painter);

        painter.end();

        buffer.close();
        QByteArray data = buffer.data();
        return std::string(data.constData(), data.size());
    }
    catch (const std::exception& e) {
        setError(std::string("SVG string rendering failed: ") + e.what());
        return std::string();
    }
}

std::string PageRenderer::renderTemplateToSVG() const
{
    clearError();

    if (!hasValidTemplate()) {
        setError("No valid template available");
        return std::string();
    }

    return m_templateRenderer->renderToSVG();
}

bool PageRenderer::hasValidTemplate() const
{
    return m_page && m_page->hasValidTemplate() && m_templateRenderer->isValid();
}

std::string PageRenderer::getLastError() const
{
    return m_lastError;
}

bool PageRenderer::hasError() const
{
    return !m_lastError.empty();
}

// Private methods

void PageRenderer::setupPainter(QPainter& painter) const
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
}

void PageRenderer::renderTemplate(QPainter& painter) const
{
    if (!hasValidTemplate()) {
        return;
    }

    // Phase 1: Basic template rendering
    // This is a stub implementation - template rendering will be improved
    // in subsequent phases when TemplateRenderer is fully implemented

    std::string templateSVG = m_templateRenderer->renderToSVG();
    if (templateSVG.empty()) {
        return;
    }

    QByteArray data(templateSVG.c_str(), static_cast<int>(templateSVG.size()));
    QSvgRenderer svgRenderer(data);
    if (!svgRenderer.isValid()) {
        Base::Console().warning("PageRenderer: Template SVG invalid, skipping render\n");
        return;
    }

    double widthMM = 0.0;
    double heightMM = 0.0;
    calculatePageBounds(widthMM, heightMM);

    painter.save();

    QSizeF viewBox = svgRenderer.viewBoxF().size();
    if (viewBox.isEmpty()) {
        const QSize defaultSize = svgRenderer.defaultSize();
        if (!defaultSize.isEmpty()) {
            viewBox = defaultSize;
        }
        else {
            viewBox = QSizeF(1.0, 1.0);
        }
    }

    const double deviceDpiX = painter.device()->logicalDpiX();
    const double deviceDpiY = painter.device()->logicalDpiY();
    const double mmToDeviceX = deviceDpiX > 0 ? deviceDpiX / 25.4 : 1.0;
    const double mmToDeviceY = deviceDpiY > 0 ? deviceDpiY / 25.4 : 1.0;
    const double targetWidth = widthMM * mmToDeviceX;
    const double targetHeight = heightMM * mmToDeviceY;

    const double scaleX = targetWidth > 0 ? targetWidth / viewBox.width() : 1.0;
    const double scaleY = targetHeight > 0 ? targetHeight / viewBox.height() : 1.0;
    painter.scale(scaleX, scaleY);

    svgRenderer.render(&painter);
    painter.restore();
}

void PageRenderer::calculatePageBounds(double& width, double& height) const
{
    // Default A4 size in mm
    width = 210.0;
    height = 297.0;

    if (m_page && m_page->hasValidTemplate()) {
        try {
            double templateWidth = m_page->getPageWidth();
            double templateHeight = m_page->getPageHeight();

            // Validate dimensions are reasonable (between 1mm and 10000mm)
            if (templateWidth > 0.0 && templateWidth <= 10000.0 &&
                templateHeight > 0.0 && templateHeight <= 10000.0) {
                width = templateWidth;
                height = templateHeight;
            }
            else {
                Base::Console().warning("PageRenderer: Invalid page dimensions (%.2f x %.2f mm), using A4 defaults\n",
                                      templateWidth, templateHeight);
            }
        }
        catch (const Base::Exception& e) {
            // Keep default values if template access fails
            Base::Console().warning("PageRenderer: Failed to get page dimensions (%s), using A4 defaults\n", e.what());
        }
    }
}

void PageRenderer::setError(const std::string& message) const
{
    m_lastError.assign(kPageRendererErrorPrefix);
    m_lastError.append(message);
}

//===========================================================================
// TemplateRenderer
//===========================================================================

TemplateRenderer::TemplateRenderer()
    : m_template(nullptr)
{
}

TemplateRenderer::TemplateRenderer(const DrawTemplate* template_)
    : m_template(template_)
{
}

TemplateRenderer::~TemplateRenderer() = default;

void TemplateRenderer::setTemplate(const DrawTemplate* template_)
{
    m_template = template_;
}

const DrawTemplate* TemplateRenderer::getTemplate() const
{
    return m_template;
}

std::string TemplateRenderer::renderToSVG() const
{
    if (!isValid()) {
        return std::string();
    }

    try {
        std::string svgContent = loadTemplateSVG();
        if (svgContent.empty()) {
            return std::string();
        }

        // Process template fields
        return processTemplateFields(svgContent);
    }
    catch (const std::exception& e) {
        Base::Console().error("TemplateRenderer: Failed to render template: %s\n", e.what());
        return std::string();
    }
}

bool TemplateRenderer::isValid() const
{
    return m_template != nullptr;
}

std::string TemplateRenderer::processTemplateFields(const std::string& svgContent) const
{
    // Phase 1: Basic field substitution stub
    // Phase 2+: Implement proper template field processing

    // For now, validate that the content is reasonable SVG
    if (svgContent.empty()) {
        Base::Console().warning("TemplateRenderer: Empty SVG content provided for field processing\n");
        return std::string();
    }

    // Basic validation that this looks like SVG content
    if (svgContent.find("<svg") == std::string::npos) {
        Base::Console().warning("TemplateRenderer: Content does not appear to contain SVG markup\n");
        // Return it anyway in case it's a fragment
    }

    return svgContent;
}

// Private methods

std::string TemplateRenderer::loadTemplateSVG() const
{
    if (!m_template) {
        return std::string();
    }

    // Phase 1: Basic template loading
    // Different template types need different handling

    if (const auto* svgTemplate = dynamic_cast<const DrawSVGTemplate*>(m_template)) {
        auto* mutableTemplate = const_cast<DrawSVGTemplate*>(svgTemplate);

        try {
            const QString processed = mutableTemplate->processTemplate();
            if (!processed.isEmpty()) {
                const QByteArray utf8 = processed.toUtf8();
                return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
            }
        }
        catch (const Base::Exception& e) {
            Base::Console().warning("TemplateRenderer: Failed to process SVG template: %s\n", e.what());
        }
        catch (const std::exception& e) {
            Base::Console().warning("TemplateRenderer: Failed to process SVG template: %s\n", e.what());
        }

        const char* embeddedPath = svgTemplate->PageResult.getValue();
        if (embeddedPath && *embeddedPath) {
            QFile file(QString::fromUtf8(embeddedPath));
            if (file.open(QIODevice::ReadOnly)) {
                const QByteArray data = file.readAll();
                return std::string(data.constData(), static_cast<std::size_t>(data.size()));
            }
            Base::Console().warning("TemplateRenderer: Unable to open embedded template file %s\n", embeddedPath);
        }

        const char* templatePath = svgTemplate->Template.getValue();
        if (templatePath && *templatePath) {
            QFile file(QString::fromUtf8(templatePath));
            if (file.open(QIODevice::ReadOnly)) {
                const QByteArray data = file.readAll();
                return std::string(data.constData(), static_cast<std::size_t>(data.size()));
            }
            Base::Console().warning("TemplateRenderer: Unable to open template source file %s\n", templatePath);
        }

        return std::string();
    }
    else if (dynamic_cast<const DrawParametricTemplate*>(m_template)) {
        Base::Console().warning("TemplateRenderer: Parametric templates are not yet supported for headless export\n");
        return std::string();
    }

    return std::string();
}

std::string TemplateRenderer::substituteFields(const std::string& content) const
{
    // Phase 1: No field substitution yet
    // Phase 2+: Implement template field replacement
    return content;
}
