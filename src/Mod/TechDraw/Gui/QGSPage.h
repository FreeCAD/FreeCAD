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

#ifndef TECHDRAWGUI_QGSCENE_H
#define TECHDRAWGUI_QGSCENE_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QGraphicsScene>
#include <QPageLayout>

class QTemporaryFile;
class QLabel;

namespace App {
class DocumentObject;
}

namespace TechDraw {
class DrawView;
class DrawViewPart;
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
}

namespace TechDrawGui
{
class QGIView;
class QGIViewDimension;
class QGITemplate;
class ViewProviderPage;
class QGIViewBalloon;
class QGILeaderLine;
class QGIRichAnno;
class QGITile;

class TechDrawGuiExport QGSPage : public QGraphicsScene
{
    Q_OBJECT

public:
    QGSPage(ViewProviderPage *vp, QWidget *parent = nullptr);
    ~QGSPage() override;

    bool addView(const App::DocumentObject *obj);
    bool attachView(App::DocumentObject *obj);
    QGIView * addViewDimension(TechDraw::DrawViewDimension *dim);
    QGIView * addViewBalloon(TechDraw::DrawViewBalloon *balloon);
    QGIView * addProjectionGroup(TechDraw::DrawProjGroup *view);
    QGIView * addViewPart(TechDraw::DrawViewPart *part);
    QGIView * addViewSection(TechDraw::DrawViewPart *part);
    QGIView * addDrawView(TechDraw::DrawView *view);
    QGIView * addDrawViewCollection(TechDraw::DrawViewCollection *view);
    QGIView * addDrawViewAnnotation(TechDraw::DrawViewAnnotation *view);
    QGIView * addDrawViewSymbol(TechDraw::DrawViewSymbol *view);
    QGIView * addDrawViewClip(TechDraw::DrawViewClip *view);
    QGIView * addDrawViewSpreadsheet(TechDraw::DrawViewSpreadsheet *view);
    QGIView * addDrawViewImage(TechDraw::DrawViewImage *view);
    QGIView * addViewLeader(TechDraw::DrawLeaderLine* view);
    QGIView * addRichAnno(TechDraw::DrawRichAnno* anno);
    QGIView * addWeldSymbol(TechDraw::DrawWeldSymbol* weld);

    void addChildrenToPage(void);
    void fixOrphans(bool force = false);

    void redrawAllViews(void);
    void redraw1View(TechDraw::DrawView* dv);

    QGIView* findQViewForDocObj(App::DocumentObject *obj) const;
    QGIView* getQGIVByName(std::string name);
    QGIView* findParent(QGIView *) const;
    void findMissingViews( const std::vector<App::DocumentObject*> &list, std::vector<App::DocumentObject*> &missing);
    bool hasQView(App::DocumentObject *obj);

    void addBalloonToParent(QGIViewBalloon* balloon, QGIView* parent);
    void createBalloon(QPointF origin, TechDraw::DrawViewPart *parent);

    void addDimToParent(QGIViewDimension* dim, QGIView* parent);
    void addLeaderToParent(QGILeaderLine* lead, QGIView* parent);
    void addAnnoToParent(QGIRichAnno* anno, QGIView* parent);

    std::vector<QGIView *> getViews() const;

    int addQView(QGIView * view);
    int removeQView(QGIView *view);
    int removeQViewByName(const char* name);
    void removeQViewFromScene(QGIView *view);

    void setPageTemplate(TechDraw::DrawTemplate *pageTemplate);
    QGITemplate * getTemplate() const;
    void removeTemplate();
    void matchSceneRectToTemplate(void);
    void attachTemplate(TechDraw::DrawTemplate *obj);
    void updateTemplate(bool force = false);
    QPointF getTemplateCenter();

    TechDraw::DrawPage * getDrawPage();

    void setExporting(bool enable);
    virtual void refreshViews();

    /// Renders the page to SVG with filename.
    void saveSvg(QString filename);
    void postProcessXml(QTemporaryFile& tempFile, QString filename, QString pagename);

    void setDimensionGroups(void);
    void setBalloonGroups(void);
    void setLeaderGroups(void);
    void setRichAnnoGroups(void);

public Q_SLOTS:

protected:
    static QColor SelectColor;
    static QColor PreselectColor;
    QColor getBackgroundColor();
    

    QGITemplate *pageTemplate;

    bool orphanExists(const char *viewName, const std::vector<App::DocumentObject*> &list);

private:
    ViewProviderPage *m_vpPage;

    QPageLayout::Orientation m_orientation;
    QPageSize::PageSizeId m_paperSize;
    qreal pagewidth, pageheight;

};

} // namespace 

#endif // TECHDRAWGUI_QGSCENE_H
