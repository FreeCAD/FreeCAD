/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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

#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPrinter>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Gui/PreferencePages/DlgSettingsPDF.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "PagePrinter.h"
#include "QGSPage.h"
#include "Rez.h"
#include "ViewProviderPage.h"

using namespace TechDrawGui;
using namespace TechDraw;
using DU = DrawUtil;

/* TRANSLATOR TechDrawGui::PagePrinter */

//! Retrieve default QPagelayout for the DrawPage. Matches the DrawPage layout.

QPageLayout PagePrinter::defaultLayout(TechDraw::DrawPage* pageObject)
{
    constexpr QPageSize::Unit mm = QPageSize::Millimeter;

    if (!pageObject) {
        return {};  // Returns an invalid QLayout.
    }
    double width = pageObject->getPageWidth();
    double height = pageObject->getPageHeight();

    // Qt figures out if this is a standard page size and uses it if so.
    // Name must be empty, so that Qt generates it.

    auto pageSize = QPageSize(QSizeF(width, height), mm, QString(), QPageSize::FuzzyOrientationMatch);

    // Orientation is bit tricky since Qt has different definition than TD.
    // TD orientation is the shape. Portrait is always higher than wide.
    // Qt orientation means that Landscape is rotated 90 deg, portrait is not. Shape does not matter.

    QPageLayout::Orientation orientation {};

    if (pageSize.id() == QPageSize::Custom) {
        // We could simply use QPageLayout::Portrait for all custom pages, but if we turn the
        // definition to portrait shape, the behaviour in printer settings is more natural.
        if (pageSize.size(mm).height() < pageSize.size(mm).width()) {
            pageSize = QPageSize(QSizeF(height, width), mm);
            orientation = QPageLayout::Landscape;
        }
        else {
            orientation = QPageLayout::Portrait;
        }
    }
    else {
        if (pageSize.size(mm).height() >= pageSize.size(mm).width()) {
            orientation = (height >= width) ? QPageLayout::Portrait : QPageLayout::Landscape;
        }
        else {
            orientation = (height > width) ? QPageLayout::Landscape : QPageLayout::Portrait;
        }
    }
    auto layout = QPageLayout(pageSize, orientation, QMarginsF(), QPageLayout::Millimeter, QMarginsF());
    layout.setMode(QPageLayout::FullPageMode);

    return layout;
}

// This renderer is private and only intended to be used to render prints/exports.

void PagePrinter::renderPage(
    const ViewProviderPage& vpp,
    QPainter& painter,
    const QPagedPaintDevice& printer,
    int resolution
)
{
    const TechDraw::DrawPage* dPage = vpp.getDrawPage();
    auto ourScene = vpp.getQGSPage();
    auto ourDoc = Gui::Application::Instance->getDocument(dPage->getDocument());

    // Clear selection to avoid it being rendered to the file
    ourScene->clearSelection();
    vpp.setTemplateMarkers(false);  // TBD: is this set somewhere?

    // exportingPdf flag is now on for all exports. It would not make sense to have different output
    // on printer and pdf. This currently just sets isExporting on lower layers, which is also set
    // for SVG. Fiddling modified flag is now here. Will be done several times for multipage but not
    // expected to be very expensive.

    auto docModifiedState = ourDoc->isModified();
    ourScene->setExportingPdf(true);
    {
        // scene might be drawn in light text.  we need to redraw in normal text.
        bool saveLightOnDark = Preferences::lightOnDark();
        if (Preferences::lightOnDark()) {
            Preferences::lightOnDark(false);
            vpp.getQGSPage()->redrawAllViews();
        }
        vpp.getQGSPage()->refreshViews();

        double height = Rez::guiX(dPage->getPageHeight());
        double width = Rez::guiX(dPage->getPageWidth());
        QRectF sourceRect {0.0, -height, width, height};
        QRect targetRect {printer.pageLayout().fullRectPixels(resolution)};

        ourScene->render(&painter, targetRect, sourceRect);

        // Reset
        Preferences::lightOnDark(saveLightOnDark);
        ourScene->refreshViews();  // TBD: What is the purpose of this? In or out of export?
    }
    ourScene->setExportingPdf(false);
    ourDoc->setModified(docModifiedState);
}

//! print all pages in a document
void PagePrinter::printAll(QPrinter* printer, App::Document* doc)
{
    std::vector<App::DocumentObject*> docObjs = doc->getObjectsOfType(
        TechDraw::DrawPage::getClassTypeId()
    );
    // we want to set the layout for the first page before we make the painter(&pdfWriter) or the
    // layout for the first page will not be correct.
    const auto dPage = static_cast<TechDraw::DrawPage*>(docObjs.front());
    printer->setPageLayout(defaultLayout(dPage));
    QPainter painter(printer);

    bool firstTime = true;
    for (auto& obj : docObjs) {
        auto vpp = dynamic_cast<TechDrawGui::ViewProviderPage*>(
            Gui::Application::Instance->getViewProvider(obj)
        );
        if (!vpp) {
            continue;  // can't print this one
        }
        auto dPage = static_cast<TechDraw::DrawPage*>(obj);
        printer->setPageLayout(defaultLayout(dPage));

        if (!firstTime) {
            printer->newPage();
        }
        firstTime = false;

        renderPage(*vpp, painter, *printer, printer->resolution());

        dPage->redrawCommand();  // TBD: Why is this needed after printing?
    }
}

//! print all pages in a document to pdf
void PagePrinter::printAllPdf(QPrinter* printer, App::Document* doc)
{
    // set up the pdfwriter
    QString outputFile = printer->outputFileName();
    QString documentName = QString::fromUtf8(doc->getName());
    QPdfWriter pdfWriter(outputFile);

    // setPdfVersion sets the printed PDF Version to what is chosen in Preferences/Import-Export/PDF
    // more details under: https://www.kdab.com/creating-pdfa-documents-qt/
    pdfWriter.setPdfVersion(Gui::Dialog::DlgSettingsPDF::evaluatePDFVersion());
    pdfWriter.setTitle(documentName);
    pdfWriter.setCreator(
        QString::fromStdString(App::Application::getNameWithVersion()) + QLatin1String(" TechDraw")
    );
    pdfWriter.setResolution(printer->resolution());
    QPageLayout pageLayout = printer->pageLayout();
    // we want to set the layout for the first page before we make the painter(&pdfWriter) or the
    // layout for the first page will not be correct.
    std::vector<App::DocumentObject*> docObjs = doc->getObjectsOfType(
        TechDraw::DrawPage::getClassTypeId()
    );
    auto firstPage = docObjs.front();
    auto dPage = static_cast<TechDraw::DrawPage*>(firstPage);

    pdfWriter.setPageLayout(defaultLayout(dPage));
    //
    // to get several pages into the same pdf, we must use the same painter for each page and not
    // have any start() or end() until all the pages are printed.
    QPainter painter(&pdfWriter);

    bool firstTime = true;
    for (auto& obj : docObjs) {
        auto* vpp = dynamic_cast<TechDrawGui::ViewProviderPage*>(
            Gui::Application::Instance->getViewProvider(obj)
        );
        if (!vpp) {
            continue;  // can't print this one
        }
        auto dPage = static_cast<TechDraw::DrawPage*>(obj);
        pdfWriter.setPageLayout(defaultLayout(dPage));

        if (!firstTime) {
            pdfWriter.newPage();
        }
        firstTime = false;

        // TBD: This one was exporting.
        renderPage(*vpp, painter, pdfWriter, pdfWriter.resolution());

        dPage->redrawCommand();  // TBD: Redraw command was while still exporting? Now moved out. ok?
    }
}

//! we don't need the banner page any more, but it might become useful again in the future.
void PagePrinter::printBannerPage(
    QPrinter* printer,
    QPainter& painter,
    QPageLayout& pageLayout,
    App::Document* doc,
    std::vector<App::DocumentObject*>& docObjs
)
{
    constexpr double mmPerInch = 25.4;
    QFont savePainterFont = painter.font();
    QFont painterFont;
    painterFont.setFamily(Preferences::labelFontQString());
    double fontSizeMM = Preferences::labelFontSizeMM();
    double dpmm = printer->resolution() / mmPerInch;
    int fontSizePx = fontSizeMM * dpmm;
    painterFont.setPixelSize(fontSizePx);
    painter.setFont(painterFont);

    // print a header
    QString docLine = QObject::tr("Document Name:") + QLatin1String(" ")
        + QString::fromUtf8(doc->getName());
    int leftMargin = pageLayout.margins().left() * dpmm + 5 * dpmm;   // layout margin + 5mm
    int verticalPos = pageLayout.margins().top() * dpmm + 20 * dpmm;  // layout margin + 20mm
    int verticalSpacing = 2;                                          // double space
    painter.drawText(leftMargin, verticalPos, docLine);

    // leave some blank space between document name and page entries
    verticalPos += 2 * verticalSpacing * fontSizePx;
    for (auto& obj : docObjs) {
        // print a line for each page
        QString pageLine = QString::fromUtf8(obj->getNameInDocument()) + QStringLiteral(" / ")
            + QString::fromUtf8(obj->Label.getValue());
        painter.drawText(leftMargin, verticalPos, pageLine);
        verticalPos += verticalSpacing * fontSizePx;
    }
    painter.setFont(savePainterFont);  // restore the original font
}

/// print the Page associated with the view provider using given printer.
//  This is a low level routine that should not modify settings.
//  TBD: Was parameter bool isPreview really needed. Don't see why. Now removed.
//   This one tried to guess from parameters if whether the output is pdf or not. Exported accordingly.

void PagePrinter::print(ViewProviderPage* vpPage, QPrinter* printer)
{

    TechDraw::DrawPage* dPage = vpPage->getDrawPage();
    QPainter painter(printer);
    renderPage(*vpPage, painter, *printer, printer->resolution());

    dPage->redrawCommand();
}

/// print the Page associated with the ViewProvider as a Pdf file
void PagePrinter::printPdf(ViewProviderPage* vpPage, const std::string& file)
{
    if (file.empty()) {
        Base::Console().warning("PagePrinter - no file specified\n");
        return;
    }
    auto filespec = Base::Tools::escapeEncodeFilename(file);
    filespec = DU::cleanFilespecBackslash(filespec);

    // set up the pdfwriter
    QString outputFile = QString::fromStdString(filespec);
    QString documentName = QString::fromUtf8(vpPage->getDrawPage()->getNameInDocument());
    QPdfWriter pdfWriter(outputFile);

    pdfWriter.setPdfVersion(Gui::Dialog::DlgSettingsPDF::evaluatePDFVersion());

    pdfWriter.setTitle(documentName);
    pdfWriter.setCreator(
        QString::fromStdString(App::Application::getNameWithVersion()) + QLatin1String(" TechDraw")
    );
    // default pdfWriter dpi is 1200.
    // set up the page layout; TBD check cleanup below.
    auto dPage = vpPage->getDrawPage();

    pdfWriter.setPageLayout(defaultLayout(dPage));
    QPainter painter(&pdfWriter);

    renderPage(*vpPage, painter, pdfWriter, pdfWriter.resolution());

    dPage->redrawCommand();
}

//! save the page associated with the view provider as an svg file
void PagePrinter::saveSVG(ViewProviderPage* vpPage, const std::string& file)
{
    if (file.empty()) {
        Base::Console().warning("PagePrinter - no file specified\n");
        return;
    }
    auto filespec = Base::Tools::escapeEncodeFilename(file);
    filespec = DU::cleanFilespecBackslash(file);
    QString filename = QString::fromStdString(filespec);

    auto ourScene = vpPage->getQGSPage();
    auto ourDoc = vpPage->getDocument();

    // setExportingSvg not needed here - saveSvg does it.
    auto docModifiedState = ourDoc->isModified();
    ourScene->saveSvg(filename);
    ourDoc->setModified(docModifiedState);
}

//! save the page associated with the view provider as an svg file
// Note: the dxf exporter does not modify the page, so we do not need to reset the modified flag
void PagePrinter::saveDXF(ViewProviderPage* vpPage, const std::string& inFileName)
{
    TechDraw::DrawPage* page = vpPage->getDrawPage();
    std::string PageName = page->getNameInDocument();

    auto filespec = Base::Tools::escapeEncodeFilename(inFileName);
    filespec = DU::cleanFilespecBackslash(filespec);
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Save page to DXF"));
    Gui::Command::doCommand(Gui::Command::Doc, "import TechDraw");
    Gui::Command::doCommand(
        Gui::Command::Doc,
        "TechDraw.writeDXFPage(App.activeDocument().%s, u\"%s\")",
        PageName.c_str(),
        filespec.c_str()
    );
    Gui::Command::commitCommand();
}

// this one is somewhat superfluous (just a redirect).
void PagePrinter::savePDF(ViewProviderPage* vpPage, const std::string& file)
{
    printPdf(vpPage, file);
}

// TBD: Check what happens if printed without template. Is that possible?
