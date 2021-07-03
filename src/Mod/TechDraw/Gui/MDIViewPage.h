/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef TECHDRAWGUI_MDIVIEWPAGE_H
#define TECHDRAWGUI_MDIVIEWPAGE_H

#include "ViewProviderPage.h"

#include <Gui/MDIView.h>
#include <Gui/Selection.h>

#include <QPrinter>
#include <QGraphicsScene>
#include <QPointF>

#include <Mod/TechDraw/App/DrawPage.h>

QT_BEGIN_NAMESPACE
class QAction;
class QTimer;
QT_END_NAMESPACE

namespace TechDraw {
class DrawTemplate;
class DrawView;
}

namespace TechDrawGui
{

class ViewProviderPage;
class QGVPage;
class QGIView;

class TechDrawGuiExport MDIViewPage : public Gui::MDIView, public Gui::SelectionObserver
{
    Q_OBJECT
    TYPESYSTEM_HEADER();

public:
    MDIViewPage(ViewProviderPage *page, Gui::Document* doc, QWidget* parent = 0);
    virtual ~MDIViewPage();

    void addChildrenToPage(void);


    /// Observer message from the Tree Selection mechanism
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void preSelectionChanged(const QPoint &pos);

    /// QGraphicsScene selection routines
    void selectQGIView(App::DocumentObject *obj, bool state);
    void clearSceneSelection();
    void blockSelection(bool isBlocked);

    void attachTemplate(TechDraw::DrawTemplate *obj);
    void updateTemplate(bool force = false);
    void fixOrphans(bool force = false);
    void matchSceneRectToTemplate(void);
    
    bool onMsg(const char* pMsg,const char** ppReturn);
    bool onHasMsg(const char* pMsg) const;

    void print();
    void print(QPrinter* printer);
    void printPdf();
    void printPdf(std::string file);
    void printPreview();

    void saveSVG(std::string file);
    void saveDXF(std::string file);
    void savePDF(std::string file);

    void setDocumentObject(const std::string&);
    void setDocumentName(const std::string&);
    PyObject* getPyObject();
    TechDraw::DrawPage * getPage() { return m_vpPage->getDrawPage(); }

    QGVPage* getQGVPage(void) {return m_view;};

    QGraphicsScene* m_scene;

    QPointF getTemplateCenter(TechDraw::DrawTemplate *obj);
    void centerOnPage(void);

    void redrawAllViews(void);
    void redraw1View(TechDraw::DrawView* dv);
    
    void setTabText(std::string t);

    bool addView(const App::DocumentObject *obj);

    static MDIViewPage *getFromScene(const QGraphicsScene *scene);

public Q_SLOTS:
    void viewAll();
    void saveSVG(void);
    void saveDXF(void);
    void savePDF(void);
    void toggleFrame(void);
    void toggleKeepUpdated(void);
//    void testAction(void);
    void sceneSelectionChanged();
    void onTimer();

protected:
    void findMissingViews( const std::vector<App::DocumentObject*> &list, std::vector<App::DocumentObject*> &missing);
    bool hasQView(App::DocumentObject *obj);
    bool orphanExists(const char *viewName, const std::vector<App::DocumentObject*> &list);

    /// Attaches view of obj to m_view.  Returns true on success, false otherwise
    bool attachView(App::DocumentObject *obj);

    void contextMenuEvent(QContextMenuEvent *event);
    void closeEvent(QCloseEvent*);

    void setDimensionGroups(void);
    void setBalloonGroups(void);
    void setLeaderGroups(void);
    void showStatusMsg(const char* s1, const char* s2, const char* s3) const;
    
    void onDeleteObject(const App::DocumentObject& obj);

    typedef boost::signals2::connection Connection;
    Connection connectDeletedObject;

    bool compareSelections(std::vector<Gui::SelectionObject> treeSel,QList<QGraphicsItem*> sceneSel);
    void setTreeToSceneSelect(void);
    void sceneSelectionManager(void);


private:
    QAction *m_toggleFrameAction;
    QAction *m_toggleKeepUpdatedAction;
    QAction *m_exportSVGAction;
    QAction *m_exportDXFAction;
    QAction *m_exportPDFAction;
//    QAction* m_testAction;

    std::string m_objectName;
    std::string m_documentName;
    bool isSelectionBlocked;
    QGVPage *m_view;
    QTimer *m_timer;

    QString m_currentPath;
    QPageLayout::Orientation m_orientation;
    QPageSize::PageSizeId m_paperSize;
    qreal pagewidth, pageheight;
    ViewProviderPage *m_vpPage;

    QList<QGraphicsItem*> m_qgSceneSelected;        //items in selection order
    QList<QGIView *> deleteItems;
};


} // namespace MDIViewPageGui

#endif // TECHDRAWGUI_MDIVIEWPAGE_H
