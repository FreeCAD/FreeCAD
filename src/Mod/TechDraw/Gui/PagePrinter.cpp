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
#include <Mod/TechDraw/App/Preferences.h>

#include "PagePrinter.h"
#include "QGSPage.h"
#include "Rez.h"
#include "ViewProviderPage.h"

using namespace TechDrawGui;
using namespace TechDraw;

constexpr double A4Heightmm = 297.0;
constexpr double A4Widthmm = 210.0;
constexpr double mmPerInch = 25.4;


/* TRANSLATOR TechDrawGui::PagePrinter */

//TYPESYSTEM_SOURCE_ABSTRACT(TechDrawGui::PagePrinter)

PagePrinter::PagePrinter(ViewProviderPage* pageVp)
    : m_vpPage(pageVp), m_orientation(QPageLayout::Landscape),
      m_paperSize(QPageSize::A4), m_pagewidth(0.0), m_pageheight(0.0)
{
}

void PagePrinter::setScene(QGSPage* scene)
{
    m_scene = scene;
}

void PagePrinter::setDocumentName(const std::string& name) { m_documentName = name; }


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
    result.pagewidth = width;
    result.pageheight = height;

    //Qt's page size determination assumes Portrait orientation. To get the right paper size
    //we need to ask in the proper form.
    QPageSize::PageSizeId paperSizeID =
        QPageSize::id(QSizeF(std::min(width, height), std::max(width, height)),
                      QPageSize::Millimeter, QPageSize::FuzzyOrientationMatch);
    result.paperSize =  paperSizeID;

    result.orientation = (QPageLayout::Orientation)dPage->getOrientation();
    if (result.paperSize == QPageSize::Ledger) {
        // Ledger size paper orientation is reversed inside Qt
        result.orientation =(QPageLayout::Orientation)(1 - result.orientation);
    }

    return result;
}

void PagePrinter::getPaperAttributes()
{
    PaperAttributes attr = getPaperAttributes(m_vpPage->getDrawPage());
    m_pagewidth = attr.pagewidth;
    m_pageheight = attr.pageheight;
    m_paperSize = attr.paperSize;
    m_orientation = attr.orientation;
}

//! construct a page layout object that reflects the characteristics of a DrawPage
//static
void PagePrinter::makePageLayout(TechDraw::DrawPage* dPage, QPageLayout& pageLayout, double& width,
                                double& height)
{
    PaperAttributes attr = getPaperAttributes(dPage);
    width = attr.pagewidth;
    height = attr.pageheight;
    pageLayout.setPageSize(QPageSize(attr.paperSize));
    pageLayout.setOrientation(attr.orientation);
}

/// print the Page associated with the parent MDIViewPage as a Pdf file
void PagePrinter::printPdf(std::string file)
{
//    Base::Console().Message("PP::printPdf(%s)\n", file.c_str());
    if (file.empty()) {
        Base::Console().Warning("PagePrinter - no file specified\n");
        return;
    }

    // set up the pdfwriter
    QString outputFile = QString::fromUtf8(file.data(), file.size());
    QPdfWriter pdfWriter(outputFile);
    QPageLayout pageLayout = pdfWriter.pageLayout();
    QString documentName = QString::fromUtf8(m_vpPage->getDrawPage()->getNameInDocument());
    pdfWriter.setTitle(documentName);
    // default pdfWriter dpi is 1200.

    // set up the page layout
    auto dPage = m_vpPage->getDrawPage();
    double width = A4Heightmm;//default to A4 Landscape 297 x 210
    double height = A4Widthmm;
    makePageLayout(dPage, pageLayout, width, height);
    pdfWriter.setPageLayout(pageLayout);

    // first page does not respect page layout unless painter is created after
    // pdfWriter layout is established.
    QPainter painter(&pdfWriter);

    // render the page
    QRectF sourceRect(0.0, Rez::guiX(-height), Rez::guiX(width), Rez::guiX(height));
    double dpmm = pdfWriter.resolution() / mmPerInch;
    int twide = int(std::round(width * dpmm));
    int thigh = int(std::round(height * dpmm));
    QRect targetRect(0, 0, twide, thigh);
    renderPage(m_vpPage, painter, sourceRect, targetRect);
}


/// print the Page associated with the parent MDIViewPage
void PagePrinter::print(QPrinter* printer)
{
//    Base::Console().Message("PP::print(printer)\n");
    QPageLayout pageLayout = printer->pageLayout();

    TechDraw::DrawPage* dp = m_vpPage->getDrawPage();
    double width = A4Heightmm;//default to A4 Landscape 297 x 210
    double height = A4Widthmm;
    makePageLayout(dp, pageLayout, width, height);
    printer->setPageLayout(pageLayout);

    QPainter painter(printer);

    QRect targetRect = printer->pageLayout().fullRectPixels(printer->resolution());
    QRectF sourceRect(0.0, Rez::guiX(-height), Rez::guiX(width), Rez::guiX(height));
    renderPage(m_vpPage, painter, sourceRect, targetRect);
}

//static routine to print all pages in a document
void PagePrinter::printAll(QPrinter* printer, App::Document* doc)
{
//    Base::Console().Message("PP::printAll()\n");

    QPageLayout pageLayout = printer->pageLayout();
    std::vector<App::DocumentObject*> docObjs =
        doc->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    auto firstPage = docObjs.front();

    auto dPage = static_cast<TechDraw::DrawPage*>(firstPage);
    double width = A4Heightmm;//default to A4 Landscape 297 x 210
    double height = A4Widthmm;
    makePageLayout(dPage, pageLayout, width, height);
    printer->setPageLayout(pageLayout);
    QPainter painter(printer);

    bool firstTime = true;
    for (auto& obj : docObjs) {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(obj);
        if (!vp) {
            continue;// can't print this one
        }
        auto* vpp = dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
        if (!vpp) {
            continue;// can't print this one
        }

        auto dPage = static_cast<TechDraw::DrawPage*>(obj);
        double width = A4Heightmm;//default to A4 Landscape 297 x 210
        double height = A4Widthmm;
        makePageLayout(dPage, pageLayout, width, height);
        printer->setPageLayout(pageLayout);

        //for some reason the first page doesn't obey the pageLayout, so we have to print
        //a sacrificial blank page, but we make it a feature instead of a bug by printing a
        //table of contents on the sacrificial page.
        // Note: if the painter(printer) occurs after the printer->setPageLayout, then the
        // first page will obey the layout.  This would mean creating the painter inside the
        // loop.
        // if (firstTime) {
        //     firstTime = false;
        //     printBannerPage(printer, painter, pageLayout, doc, docObjs);
        // }
        if (!firstTime) {
            printer->newPage();
        }
        firstTime = false;
        QRectF sourceRect(0.0, Rez::guiX(-height), Rez::guiX(width), Rez::guiX(height));
        QRect targetRect = printer->pageLayout().fullRectPixels(printer->resolution());

        renderPage(vpp, painter, sourceRect, targetRect);

    }
}

//static routine to print all pages in a document to pdf
void PagePrinter::printAllPdf(QPrinter* printer, App::Document* doc)
{
//    Base::Console().Message("PP::printAllPdf()\n");
    double dpmm = printer->resolution() / mmPerInch;

    QString outputFile = printer->outputFileName();
    QString documentName = QString::fromUtf8(doc->getName());
    QPdfWriter pdfWriter(outputFile);
    // setPdfVersion sets the printed PDF Version to comply with PDF/A-1b, more details under: https://www.kdab.com/creating-pdfa-documents-qt/
    // but this is not working as of Qt 5.12
    //printer->setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);
    //pdfWriter.setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);
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
    }
}

//static
//! we don't need the banner page any more
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

//static
void PagePrinter::renderPage(ViewProviderPage* vpp, QPainter& painter, QRectF& sourceRect,
                             QRect& targetRect)
{
//    Base::Console().Message("PP::renderPage()\n");
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

void PagePrinter::saveSVG(std::string file)
{
    if (file.empty()) {
        Base::Console().Warning("PagePrinter - no file specified\n");
        return;
    }
    QString filename = QString::fromUtf8(file.data(), file.size());
    if (m_scene) {
        m_scene->saveSvg(filename);
    }
}

void PagePrinter::saveDXF(std::string fileName)
{
    TechDraw::DrawPage* page = m_vpPage->getDrawPage();
    std::string PageName = page->getNameInDocument();
    fileName = Base::Tools::escapeEncodeFilename(fileName);
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Save page to dxf"));
    Gui::Command::doCommand(Gui::Command::Doc, "import TechDraw");
    Gui::Command::doCommand(Gui::Command::Doc,
                            "TechDraw.writeDXFPage(App.activeDocument().%s, u\"%s\")",
                            PageName.c_str(), (const char*)fileName.c_str());
    Gui::Command::commitCommand();
}

void PagePrinter::savePDF(std::string file)
{
//    Base::Console().Message("PP::savePDF(%s)\n", file.c_str());
    printPdf(file);
}

PaperAttributes::PaperAttributes()
{
    // set default values to A4 Landscape
    orientation = QPageLayout::Orientation::Landscape;
    paperSize = QPageSize::A4;
    pagewidth = A4Heightmm;
    pageheight = A4Widthmm;
}
