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

namespace TechDraw
{
class DrawPage;
}

namespace TechDrawGui
{
class MDIViewPage;
class QGVPage;
class QGSPage;
class QGIView;

class TechDrawGuiExport PagePrinter
{
    // print banner page is no longer used
    static void printBannerPage(
        QPrinter* printer,
        QPainter& painter,
        QPageLayout& pageLayout,
        App::Document* doc,
        std::vector<App::DocumentObject*>& docObjs
    );

    static void renderPage(
        const ViewProviderPage& vpp,
        QPainter& painter,
        const QPagedPaintDevice& printer,
        int resolution
    );

public:
    static QPageLayout defaultLayout(TechDraw::DrawPage* pageObject);

    static void print(ViewProviderPage* vpPage, QPrinter* printer);
    static void printPdf(ViewProviderPage* vpPage, const std::string& file);
    static void printAll(QPrinter* printer, App::Document* doc);
    static void printAllPdf(QPrinter* printer, App::Document* doc);

    static void saveSVG(ViewProviderPage* vpPage, const std::string& file);
    static void saveDXF(ViewProviderPage* vpPage, const std::string& file);
    static void savePDF(ViewProviderPage* vpPage, const std::string& file);
};

}  // namespace TechDrawGui

#endif  // TECHDRAWGUI_PAGEPRINTER_H
