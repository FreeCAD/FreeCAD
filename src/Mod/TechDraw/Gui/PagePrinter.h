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

#ifndef TECHDRAWGUI_PAGEPRINTER_H
#define TECHDRAWGUI_PAGEPRINTER_H

#include <QPrinter>


#include <Mod/TechDraw/TechDrawGlobal.h>

#include "ViewProviderPage.h"

QT_BEGIN_NAMESPACE
class QGraphicsScene;
QT_END_NAMESPACE

namespace TechDraw {
class DrawPage;
class DrawTemplate;
class DrawView;
}

namespace TechDrawGui
{
class MDIViewPage;
class QGVPage;
class QGSPage;
class QGIView;

class TechDrawGuiExport PagePrinter
{
public:
    PagePrinter(ViewProviderPage *page);
    ~PagePrinter() = default;

    void print(QPrinter* printer);
    void printPdf();
    void printPdf(std::string file);
    void printPreview();
    static void printAllPages();
    static void printAll(QPrinter* printer,
                         App::Document* doc);
    static void printAllPdf(QPrinter* printer,
                            App::Document* doc);
    static void printBannerPage(QPrinter* printer, QPainter& painter,
                                QPageLayout& pageLayout,
                                App::Document* doc,
                                std::vector<App::DocumentObject*>& docObjs);
    static void renderPage(ViewProviderPage* vpp,
                           QPainter& painter,
                           QRectF& sourceRect,
                           QRect& targetRect);
    static void setPageLayout(QPageLayout& pageLayout,
                              TechDraw::DrawPage* dPage,
                              double& width, double& height);

    void saveSVG(std::string file);
    void saveDXF(std::string file);
    void savePDF(std::string file);

    void setDocumentName(const std::string&);
    void setScene(QGSPage* scene, QGVPage* view);
    void setOwner(MDIViewPage* owner) { m_owner = owner; }
    void setScene(QGSPage* scene);


    TechDraw::DrawPage * getPage() { return m_vpPage->getDrawPage(); }

    ViewProviderPage* getViewProviderPage() {return m_vpPage;}

    void getPaperAttributes();
    QPageLayout::Orientation getOrientation() const { return m_orientation; }
    QPageSize::PageSizeId getPaperSize() const { return m_paperSize; }
    double getPageWidth() const { return m_pagewidth; }
    double getPageHeight() const { return m_pageheight; }

private:
    std::string m_objectName;
    std::string m_documentName;
    QPointer<QGSPage> m_scene;

    QString m_currentPath;
    ViewProviderPage* m_vpPage;

    QPageLayout::Orientation m_orientation;
    QPageSize::PageSizeId m_paperSize;
    double m_pagewidth, m_pageheight;

    MDIViewPage* m_owner;

};


} // namespace PagePrinterGui

#endif // TECHDRAWGUI_PAGEPRINTER_H

