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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QApplication>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPaintEngine>
#include <QPainter>
#include <QPdfWriter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Stream.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawPagePy.h>
#include <Mod/TechDraw/App/DrawTemplate.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "PagePrinter.h"
#include "QGSPage.h"
#include "Rez.h"
#include "ViewProviderPage.h"

using namespace TechDrawGui;
using namespace TechDraw;
using DU = DrawUtil;

constexpr double A4Heightmm = 297.0;
constexpr double A4Widthmm = 210.0;
constexpr double mmPerInch = 25.4;


/* TRANSLATOR TechDrawGui::PagePrinter */

//TYPESYSTEM_SOURCE_ABSTRACT(TechDrawGui::PagePrinter)


//! retrieve the attributes of a DrawPage and its Template
PaperAttributes PagePrinter::getPaperAttributes(TechDraw::DrawPage* dPage)
{
    PaperAttributes result;
    if (!dPage) {
        return result;
    }
    double width = A4Widthmm;
    double height = A4Heightmm;
    auto pageTemplate(dynamic_cast<TechDraw::DrawTemplate*>(dPage->Template.getValue()));
    if (pageTemplate) {
        width = pageTemplate->Width.getValue();
        height = pageTemplate->Height.getValue();
    }
    // result.m_pagewidth = width;
    // result.m_pageheight = height;

    //Qt's page size determination assumes Portrait orientation. To get the right paper size
    //we need to ask in the proper form.
    QPageSize::PageSizeId paperSizeID =
        QPageSize::id(QSizeF(std::min(width, height), std::max(width, height)),
                      QPageSize::Millimeter, QPageSize::FuzzyOrientationMatch);
    auto paperSize = paperSizeID;

    auto orientation = (QPageLayout::Orientation)dPage->getOrientation();
    if (paperSize == QPageSize::Ledger) {
        // Ledger size paper orientation is reversed inside Qt
        orientation = (QPageLayout::Orientation)(1 - orientation);
    }

    return {orientation, paperSize, width, height};
}

//! retrieve the attributes of a DrawPage by its viewProvider
PaperAttributes PagePrinter::getPaperAttributes(ViewProviderPage* vpPage)
{
    auto page = vpPage->getDrawPage();
    return getPaperAttributes(page);
}


//! construct a page layout object that reflects the characteristics of a DrawPage
void PagePrinter::makePageLayout(TechDraw::DrawPage* dPage, QPageLayout& pageLayout, double& width,
                                double& height)
{
    PaperAttributes attr = getPaperAttributes(dPage);
    width = attr.pageWidth();
    height = attr.pageHeight();
    pageLayout.setPageSize(QPageSize(attr.pageSize()));
    pageLayout.setOrientation(attr.orientation());
    pageLayout.setMode(QPageLayout::FullPageMode);
    pageLayout.setMargins(QMarginsF());
}


//! print all pages in a document
void PagePrinter::printAll(QPrinter* printer, App::Document* doc)
{
    QPageLayout pageLayout = printer->pageLayout();
    std::vector<App::DocumentObject*> docObjs =
        doc->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    auto firstPage = docObjs.front();

    auto dPage = static_cast<TechDraw::DrawPage*>(firstPage);
    double width = A4Heightmm;  // default to A4 Landscape 297 x 210
    double height = A4Widthmm;
    makePageLayout(dPage, pageLayout, width, height);
    printer->setPageLayout(pageLayout);
    QPainter painter(printer);

    auto ourDoc = Gui::Application::Instance->getDocument(doc);
    auto docModifiedState = ourDoc->isModified();

    bool firstTime = true;
    for (auto& obj : docObjs) {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(obj);
        if (!vp) {
            continue;  // can't print this one
        }
        auto* vpp = dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
        if (!vpp) {
            continue;  // can't print this one
        }

        auto dPage = static_cast<TechDraw::DrawPage*>(obj);
        double width = A4Heightmm;  // default to A4 Landscape 297 x 210
        double height = A4Widthmm;
        makePageLayout(dPage, pageLayout, width, height);
        printer->setPageLayout(pageLayout);

        if (!firstTime) {
            printer->newPage();
        }
        firstTime = false;
        QRectF sourceRect(0.0, Rez::guiX(-height), Rez::guiX(width), Rez::guiX(height));
        QRect targetRect = printer->pageLayout().fullRectPixels(printer->resolution());
        renderPage(vpp, painter, sourceRect, targetRect);
        dPage->redrawCommand();
    }

    ourDoc->setModified(docModifiedState);
}

//! print all pages in a document to pdf
void PagePrinter::printAllPdf(QPrinter* printer, App::Document* doc)
{
    double dpmm = printer->resolution() / mmPerInch;

    QString outputFile = printer->outputFileName();
    QString documentName = QString::fromUtf8(doc->getName());
    QPdfWriter pdfWriter(outputFile);

    // set the printed PDF Version to comply with PDF/A-1b, more details under:
    // https://www.kdab.com/creating-pdfa-documents-qt/
    pdfWriter.setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);

    pdfWriter.setTitle(documentName);
    pdfWriter.setResolution(printer->resolution());
    QPageLayout pageLayout = printer->pageLayout();
    // we want to set the layout for the first page before we make the painter(&pdfWriter) or the layout for the first page will
    // not be correct.
    std::vector<App::DocumentObject*> docObjs =
        doc->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    auto firstPage = docObjs.front();

    auto dPage = static_cast<TechDraw::DrawPage*>(firstPage);
    double width = A4Heightmm;//default to A4 Landscape 297 x 210
    double height = A4Widthmm;
    makePageLayout(dPage, pageLayout, width, height);

    pdfWriter.setPageLayout(pageLayout);
    // to get several pages into the same pdf, we must use the same painter for each page and not have any
    // start() or end() until all the pages are printed.
    QPainter painter(&pdfWriter);

    auto ourDoc = Gui::Application::Instance->getDocument(doc);
    auto docModifiedState = ourDoc->isModified();

    bool firstTime = true;
    for (auto& obj : docObjs) {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(obj);
        if (!vp) {
            continue;// can't print this one
        }
        auto vpp = dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
        if (!vpp) {
            continue;// can't print this one
        }

        auto ourScene = vpp->getQGSPage();
        ourScene->setExportingPdf(true);

        auto dPage = static_cast<TechDraw::DrawPage*>(obj);
        double width{0};
        double height{0};
        makePageLayout(dPage, pageLayout, width, height);
        pdfWriter.setPageLayout(pageLayout);
        if (!firstTime) {
            pdfWriter.newPage();
        }
        firstTime = false;

        QRectF sourceRect(0.0, Rez::guiX(-height), Rez::guiX(width), Rez::guiX(height));
        QRect targetRect(0, 0, width * dpmm, height * dpmm);
        renderPage(vpp, painter, sourceRect, targetRect);
        dPage->redrawCommand();

        ourScene->setExportingPdf(true);
    }

    ourDoc->setModified(docModifiedState);
}


//! we don't need the banner page any more, but it might become useful again in the future.
void PagePrinter::printBannerPage(QPrinter* printer, QPainter& painter, QPageLayout& pageLayout,
                                  App::Document* doc, std::vector<App::DocumentObject*>& docObjs)
{
    QFont savePainterFont = painter.font();
    QFont painterFont;
    painterFont.setFamily(Preferences::labelFontQString());
    int fontSizeMM = Preferences::labelFontSizeMM();
    double dpmm = printer->resolution() / mmPerInch;
    int fontSizePx = fontSizeMM * dpmm;
    painterFont.setPixelSize(fontSizePx);
    painter.setFont(painterFont);

    //print a header
    QString docLine = QObject::tr("Document Name:") + QLatin1String(" ") + QString::fromUtf8(doc->getName());
    int leftMargin = pageLayout.margins().left() * dpmm + 5 * dpmm; //layout margin + 5mm
    int verticalPos = pageLayout.margins().top() * dpmm + 20 * dpmm;//layout margin + 20mm
    int verticalSpacing = 2;                                        //double space
    painter.drawText(leftMargin, verticalPos, docLine);

    //leave some blank space between document name and page entries
    verticalPos += 2 * verticalSpacing * fontSizePx;
    for (auto& obj : docObjs) {
        //print a line for each page
        QString pageLine = QString::fromUtf8(obj->getNameInDocument()) + QString::fromUtf8(" / ")
            + QString::fromUtf8(obj->Label.getValue());
        painter.drawText(leftMargin, verticalPos, pageLine);
        verticalPos += verticalSpacing * fontSizePx;
    }
    painter.setFont(savePainterFont);//restore the original font
}


void PagePrinter::renderPage(ViewProviderPage* vpp, QPainter& painter, QRectF& sourceRect,
                             QRect& targetRect)
{
    //turn off view frames for print
    bool saveState = vpp->getFrameState();
    vpp->setFrameState(false);
    vpp->setTemplateMarkers(false);

    //scene might be drawn in light text.  we need to redraw in normal text.
    bool saveLightOnDark = Preferences::lightOnDark();
    if (Preferences::lightOnDark()) {
        Preferences::lightOnDark(false);
        vpp->getQGSPage()->redrawAllViews();
    }

    vpp->getQGSPage()->refreshViews();
    vpp->getQGSPage()->render(&painter, targetRect, sourceRect);

    // Reset
    vpp->setFrameState(saveState);
    vpp->setTemplateMarkers(saveState);
    Preferences::lightOnDark(saveLightOnDark);

    vpp->getQGSPage()->refreshViews();
}


/// print the Page associated with the view provider
void PagePrinter::print(ViewProviderPage* vpPage, QPrinter* printer)
{
    QPageLayout pageLayout = printer->pageLayout();

    TechDraw::DrawPage* dPage = vpPage->getDrawPage();
    double width = A4Heightmm;  // default to A4 Landscape 297 x 210
    double height = A4Widthmm;
    makePageLayout(dPage, pageLayout, width, height);
    printer->setPageLayout(pageLayout);

    QPainter painter(printer);

    auto ourScene = vpPage->getQGSPage();
    if (!printer->outputFileName().isEmpty()) {
        ourScene->setExportingPdf(true);
    }
    auto ourDoc = Gui::Application::Instance->getDocument(dPage->getDocument());
    auto docModifiedState = ourDoc->isModified();

    QRect targetRect = printer->pageLayout().fullRectPixels(printer->resolution());
    QRectF sourceRect(0.0, Rez::guiX(-height), Rez::guiX(width), Rez::guiX(height));
    renderPage(vpPage, painter, sourceRect, targetRect);

    ourScene->setExportingPdf(false);  // doesn't hurt if not pdf
    ourDoc->setModified(docModifiedState);
    dPage->redrawCommand();
}


/// print the Page associated with the ViewProvider as a Pdf file
void PagePrinter::printPdf(ViewProviderPage* vpPage, const std::string& file)
{
    if (file.empty()) {
        Base::Console().Warning("PagePrinter - no file specified\n");
        return;
    }

    auto filespec = Base::Tools::escapeEncodeFilename(file);
    filespec = DU::cleanFilespecBackslash(filespec);

    // set up the pdfwriter
    QString outputFile = QString::fromStdString(filespec);
    QPdfWriter pdfWriter(outputFile);
    pdfWriter.setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);
    QPageLayout pageLayout = pdfWriter.pageLayout();
    auto marginsdb = pageLayout.margins(QPageLayout::Millimeter);
    QString documentName = QString::fromUtf8(vpPage->getDrawPage()->getNameInDocument());
    pdfWriter.setTitle(documentName);
    // default pdfWriter dpi is 1200.

    // set up the page layout
    auto dPage = vpPage->getDrawPage();
    double width = A4Heightmm;  // default to A4 Landscape 297 x 210
    double height = A4Widthmm;
    makePageLayout(dPage, pageLayout, width, height);
    pdfWriter.setPageLayout(pageLayout);
    marginsdb = pageLayout.margins(QPageLayout::Millimeter);

    // first page does not respect page layout unless painter is created after
    // pdfWriter layout is established.
    QPainter painter(&pdfWriter);

    auto ourScene = vpPage->getQGSPage();
    ourScene->setExportingPdf(true);
    auto ourDoc = Gui::Application::Instance->getDocument(dPage->getDocument());
    auto docModifiedState = ourDoc->isModified();

    // render the page
    QRectF sourceRect(0.0, Rez::guiX(-height), Rez::guiX(width), Rez::guiX(height));
    double dpmm = pdfWriter.resolution() / mmPerInch;
    int twide = int(std::round(width * dpmm));
    int thigh = int(std::round(height * dpmm));
    QRect targetRect(0, 0, twide, thigh);
    renderPage(vpPage, painter, sourceRect, targetRect);

    ourScene->setExportingPdf(false);
    ourDoc->setModified(docModifiedState);
    dPage->redrawCommand();
}


//! save the page associated with the view provider as an svg file
void PagePrinter::saveSVG(ViewProviderPage* vpPage, const std::string& file)
{
    if (file.empty()) {
        Base::Console().Warning("PagePrinter - no file specified\n");
        return;
    }
    auto filespec = Base::Tools::escapeEncodeFilename(file);
    filespec = DU::cleanFilespecBackslash(file);
    QString filename = QString::fromStdString(filespec);

    auto ourScene = vpPage->getQGSPage();
    ourScene->setExportingSvg(true);
    auto ourDoc = vpPage->getDocument();
    auto docModifiedState = ourDoc->isModified();

    ourScene->saveSvg(filename);

    ourScene->setExportingSvg(false);
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
    Gui::Command::doCommand(Gui::Command::Doc,
                            "TechDraw.writeDXFPage(App.activeDocument().%s, u\"%s\")",
                            PageName.c_str(),
                            filespec.c_str());
    Gui::Command::commitCommand();
}

// this one is somewhat superfluous (just a redirect).
void PagePrinter::savePDF(ViewProviderPage* vpPage, const std::string& file)
{
    printPdf(vpPage, file);
}


PaperAttributes::PaperAttributes()
{
    // set default values to A4 Landscape
    m_orientation = QPageLayout::Orientation::Landscape;
    m_paperSize = QPageSize::A4;
    m_pagewidth = A4Heightmm;
    m_pageheight = A4Widthmm;
}
