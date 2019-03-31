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


#ifndef DRAWINGGUI_DRAWINGVIEW_H
#define DRAWINGGUI_DRAWINGVIEW_H

#include <Gui/MDIView.h>
#include <Gui/Selection.h>

#include <QPrinter>
#include <QGraphicsScene>
#include <QPointF>

QT_BEGIN_NAMESPACE
class QAction;
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

public:
    MDIViewPage(ViewProviderPage *page, Gui::Document* doc, QWidget* parent = 0);
    virtual ~MDIViewPage();

    /// Observer message from the Tree Selection mechanism
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void preSelectionChanged(const QPoint &pos);

    /// QGraphicsScene seletion routines
    void selectQGIView(App::DocumentObject *obj, bool state);
    void clearSceneSelection();
    void blockSelection(bool isBlocked);

    void attachTemplate(TechDraw::DrawTemplate *obj);
    void updateTemplate(bool force = false);
//    void updateDrawing(bool force = false);
    void updateDrawing(void);
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

    void setFrameState(bool state);
    bool getFrameState(void) {return m_frameState;};

    void setDocumentObject(const std::string&);
    void setDocumentName(const std::string&);
    PyObject* getPyObject();

    QGVPage* getQGVPage(void) {return m_view;};

    QGraphicsScene* m_scene;

    QPointF getTemplateCenter(TechDraw::DrawTemplate *obj);
    void centerOnPage(void);

    void redrawAllViews(void);
    void redraw1View(TechDraw::DrawView* dv);
    
    void setTabText(std::string t);

    bool addView(const App::DocumentObject *obj);

public Q_SLOTS:
    void viewAll();
    void saveSVG(void);
    void saveDXF(void);
    void savePDF(void);
    void toggleFrame(void);
    void toggleKeepUpdated(void);
//    void testAction(void);
    void sceneSelectionChanged();

protected:
    void findMissingViews( const std::vector<App::DocumentObject*> &list, std::vector<App::DocumentObject*> &missing);
    bool hasQView(App::DocumentObject *obj);
    bool orphanExists(const char *viewName, const std::vector<App::DocumentObject*> &list);

    /// Attaches view of obj to m_view.  Returns true on success, false otherwise
    bool attachView(App::DocumentObject *obj);

    void contextMenuEvent(QContextMenuEvent *event);
    void closeEvent(QCloseEvent*);
    QPrinter::PaperSize getPaperSize(int w, int h) const;
    void setDimensionGroups(void);
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

    QString m_currentPath;
    QPrinter::Orientation m_orientation;
    QPrinter::PaperSize m_paperSize;
    ViewProviderPage *m_vpPage;

    bool m_frameState;

    QList<QGraphicsItem*> m_sceneSelected;
    QList<QGIView *> deleteItems;
};


} // namespace MDIViewPageGui

#endif // DRAWINGGUI_DRAWINGVIEW_H
