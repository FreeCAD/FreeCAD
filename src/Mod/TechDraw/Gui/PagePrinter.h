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

#pragma once

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

class TechDrawGuiExport PaperAttributes
{
public:
    PaperAttributes();
    PaperAttributes(QPageLayout::Orientation orientation,
                    QPageSize::PageSizeId paperSizeId,
                    double pageWidth,
                    double pageHeight)
        : m_orientation(orientation)
        , m_paperSizeId(paperSizeId)
        , m_pagewidth(pageWidth)
        , m_pageheight(pageHeight)
    {}

    QPageLayout::Orientation orientation() const
    {
        return m_orientation;
    }
    QPageSize::PageSizeId pageSizeId() const
    {
        return m_paperSizeId;
    }
    double pageWidth() const
    {
        return m_pagewidth;
    }
    double pageHeight() const
    {
        return m_pageheight;
    }

private:
    QPageLayout::Orientation m_orientation;
    QPageSize::PageSizeId m_paperSizeId;
    double m_pagewidth;
    double m_pageheight;
};

class TechDrawGuiExport PagePrinter
{
public:
    // print banner page is no longer used
    static void printBannerPage(QPrinter* printer, QPainter& painter,
                                QPageLayout& pageLayout,
                                App::Document* doc,
                                std::vector<App::DocumentObject*>& docObjs);

    static void renderPage(ViewProviderPage* vpp,
                           QPainter& painter,
                           QRectF& sourceRect,
                           QRect& targetRect);
    static void makePageLayout(TechDraw::DrawPage* dPage,
                               QPageLayout& pageLayout,
                              double& width, double& height);

    static PaperAttributes getPaperAttributes(TechDraw::DrawPage* pageObject);
    static PaperAttributes getPaperAttributes(ViewProviderPage* vpPage);

    static void print(ViewProviderPage* vpPage, QPrinter* printer, bool isPreview = false);
    static void printPdf(ViewProviderPage* vpPage, const std::string& file);
    static void printAll(QPrinter* printer, App::Document* doc);
    static void printAllPdf(QPrinter* printer, App::Document* doc);

    static void saveSVG(ViewProviderPage* vpPage, const std::string& file);
    static void saveDXF(ViewProviderPage* vpPage, const std::string& file);
    static void savePDF(ViewProviderPage* vpPage, const std::string& file);
};

}  // namespace TechDrawGui