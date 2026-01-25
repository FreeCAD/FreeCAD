/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QGraphicsScene>


class QTemporaryFile;
class QLabel;

namespace App
{
class DocumentObject;
}

namespace TechDraw
{
class DrawView;
class DrawViewPart;
class DrawViewSection;
class DrawViewDetail;
class DrawProjGroup;
class DrawViewDimension;
class DrawPage;
class DrawTemplate;
class DrawViewAnnotation;
class DrawViewSymbol;
class DrawViewClip;
class DrawViewCollection;
class DrawViewSpreadsheet;
class DrawViewImage;
class DrawLeaderLine;
class DrawViewBalloon;
class DrawRichAnno;
class DrawWeldSymbol;
}// namespace TechDraw

namespace TechDrawGui
{
class QGIView;
class QGIViewDimension;
class QGITemplate;
class ViewProviderPage;
class QGIViewBalloon;
class QGITile;
class QGILeaderLine;
class QGIRichAnno;

class TechDrawGuiExport QGSPage: public QGraphicsScene
{
    Q_OBJECT

public:
    explicit QGSPage(ViewProviderPage* vpPage, QWidget* parent = nullptr);
    ~QGSPage() override = default;

    bool addView(const App::DocumentObject* obj);
    bool attachView(App::DocumentObject* obj);
    QGIView* addViewDimension(TechDraw::DrawViewDimension* dimFeat);
    QGIView* addViewBalloon(TechDraw::DrawViewBalloon* balloonFeat);
    QGIView* addProjectionGroup(TechDraw::DrawProjGroup* projGroupFeat);
    QGIView* addViewPart(TechDraw::DrawViewPart* partFeat);
    QGIView* addViewSection(TechDraw::DrawViewSection* sectionFeat);
    QGIView* addDrawView(TechDraw::DrawView* viewFeat);
    QGIView* addDrawViewCollection(TechDraw::DrawViewCollection* collectionFeat);
    QGIView* addAnnotation(TechDraw::DrawViewAnnotation* annoFeat);
    QGIView* addDrawViewSymbol(TechDraw::DrawViewSymbol* symbolFeat);
    QGIView* addDrawViewClip(TechDraw::DrawViewClip* clipFeat);
    QGIView* addDrawViewSpreadsheet(TechDraw::DrawViewSpreadsheet* sheetFeat);
    QGIView* addDrawViewImage(TechDraw::DrawViewImage* imageFeat);
    QGIView* addViewLeader(TechDraw::DrawLeaderLine* leaderFeat);
    QGIView* addRichAnno(TechDraw::DrawRichAnno* richFeat);
    QGIView* addWeldSymbol(TechDraw::DrawWeldSymbol* weldFeat);

    void addChildrenToPage();
    void fixOrphans(bool force = false);

    void redrawAllViews();
    void redraw1View(TechDraw::DrawView* dView);

    QGIView* findQViewForDocObj(App::DocumentObject* obj) const;
    QGIView* getQGIVByName(std::string name) const;
    QGIView* findParent(QGIView*) const;
    void findMissingViews(const std::vector<App::DocumentObject*>& list,
                          std::vector<App::DocumentObject*>& missing);
    bool hasQView(App::DocumentObject* obj);

    void addBalloonToParent(QGIViewBalloon* balloon, QGIView* parent);
    void createBalloon(QPointF origin, TechDraw::DrawView* parent);

    void addDimToParent(QGIViewDimension* dim, QGIView* parent);
    void addLeaderToParent(QGILeaderLine* leader, QGIView* parent);
    void addRichAnnoToParent(QGIRichAnno* anno, QGIView* parent);

    void addItemToScene(QGIView* item);
    void addItemToParent(QGIView* item, QGIView* parent);

    std::vector<QGIView*> getViews() const;

    int addQView(QGIView* view);
    int removeQView(QGIView* view);
    int removeQViewByName(const char* name);
    void removeQViewFromScene(QGIView* view);

    void setPageTemplate(TechDraw::DrawTemplate* templateFeat);
    QGITemplate* getTemplate() const;
    void removeTemplate();
    void matchSceneRectToTemplate();
    void attachTemplate(TechDraw::DrawTemplate* obj);
    void updateTemplate(bool force = false);
    QPointF getTemplateCenter();

    TechDraw::DrawPage* getDrawPage();

    void setExportingSvg(bool enable);
    bool getExportingSvg() const { return m_exportingSvg; }

    void setExportingPdf(bool enable) { m_exportingPdf = enable; };
    bool getExportingPdf() const { return m_exportingPdf; }
    bool getExportingAny() const { return getExportingPdf() || getExportingSvg(); }

    virtual void refreshViews();

    /// Renders the page to SVG with filename.
    void saveSvg(QString filename);
    void postProcessXml(QTemporaryFile& temporaryFile, QString filename, QString pagename);

    // scene parentage fixups
    void setViewParents();

    static bool itemClearsSelection(int itemTypeIn);
    static Qt::KeyboardModifiers cleanModifierList(Qt::KeyboardModifiers mods);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    QColor getBackgroundColor();
    bool orphanExists(const char* viewName, const std::vector<App::DocumentObject*>& list);

private:
    QGITemplate* pageTemplate;
    ViewProviderPage* m_vpPage;

    bool m_exportingSvg{false};
    bool m_exportingPdf{false};
};

}// namespace TechDrawGui