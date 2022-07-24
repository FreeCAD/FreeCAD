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

#include <QPointF>
#include <QPrinter>

#include <Gui/MDIView.h>
#include <Gui/MDIViewPy.h>
#include <Gui/Selection.h>

#include "ViewProviderPage.h"


QT_BEGIN_NAMESPACE
class QAction;
class QGraphicsItem;
class QGraphicsScene;
QT_END_NAMESPACE

namespace TechDraw {
class DrawPage;
class DrawTemplate;
class DrawView;
}

namespace TechDrawGui
{

class ViewProviderPage;
class QGVPage;
class QGSPage;
class QGIView;

class TechDrawGuiExport MDIViewPage : public Gui::MDIView, public Gui::SelectionObserver
{
    Q_OBJECT
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MDIViewPage(ViewProviderPage *page, Gui::Document* doc, QWidget* parent = nullptr);
    ~MDIViewPage() override;

    void addChildrenToPage();


    /// Observer message from the Tree Selection mechanism
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void preSelectionChanged(const QPoint &pos);

    /// QGraphicsScene selection routines
    void selectQGIView(App::DocumentObject *obj, bool state);
    void clearSceneSelection();
    void blockSceneSelection(bool isBlocked);

    bool onMsg(const char* pMsg,const char** ppReturn) override;
    bool onHasMsg(const char* pMsg) const override;

    void print() override;
    void print(QPrinter* printer) override;
    void printPdf() override;
    void printPdf(std::string file);
    void printPreview() override;

    void saveSVG(std::string file);
    void saveDXF(std::string file);
    void savePDF(std::string file);

    void setDocumentObject(const std::string&);
    void setDocumentName(const std::string&);

    PyObject* getPyObject() override;
    TechDraw::DrawPage * getPage() { return m_vpPage->getDrawPage(); }

    ViewProviderPage* getViewProviderPage() {return m_vpPage;}

    void setTabText(std::string t);

    static MDIViewPage *getFromScene(const QGSPage *scene);
    void contextMenuEvent(QContextMenuEvent *event) override;

    void setScene(QGSPage* scene, QGVPage* view);

public Q_SLOTS:
    void viewAll() override;
    void saveSVG();
    void saveDXF();
    void savePDF();
    void toggleFrame();
    void toggleKeepUpdated();
//    void testAction(void);
    void sceneSelectionChanged();

protected:
    void closeEvent(QCloseEvent*) override;

    void showStatusMsg(const char* s1, const char* s2, const char* s3) const;
    
    void onDeleteObject(const App::DocumentObject& obj);

    typedef boost::signals2::connection Connection;
    Connection connectDeletedObject;

    bool compareSelections(std::vector<Gui::SelectionObject> treeSel,QList<QGraphicsItem*> sceneSel);
    void setTreeToSceneSelect();
    void sceneSelectionManager();

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
    QPointer<QGSPage> m_scene;
    QPointer<QGVPage> m_view;

    QString m_currentPath;
    ViewProviderPage* m_vpPage;

    QList<QGraphicsItem*> m_qgSceneSelected;        //items in selection order

    void getPaperAttributes(void);
    QPageLayout::Orientation m_orientation;
    QPageSize::PageSizeId m_paperSize;
    double m_pagewidth, m_pageheight;

};

class MDIViewPagePy : public Py::PythonExtension<MDIViewPagePy>
{
public:
    using BaseType = Py::PythonExtension<MDIViewPagePy>;
    static void init_type();

    MDIViewPagePy(MDIViewPage *mdi);
    ~MDIViewPagePy() override;

    Py::Object repr() override;
    Py::Object getattr(const char *) override;
    Py::Object getPage(const Py::Tuple&);
    Py::Object cast_to_base(const Py::Tuple&);

    MDIViewPage* getMDIViewPagePtr();

protected:
    Gui::MDIViewPy base;
};


} // namespace MDIViewPageGui

#endif // TECHDRAWGUI_MDIVIEWPAGE_H
