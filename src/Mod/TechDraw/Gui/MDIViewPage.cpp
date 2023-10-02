/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPaintEngine>
#include <QPainter>
#include <QPdfWriter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <boost_signals2.hpp>
#include <cmath>
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
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Window.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawPagePy.h>
#include <Mod/TechDraw/App/DrawTemplate.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "MDIViewPage.h"
#include "QGIEdge.h"
#include "QGIFace.h"
#include "QGITemplate.h"
#include "QGIVertex.h"
#include "QGIView.h"
#include "QGIViewBalloon.h"
#include "QGIViewDimension.h"
#include "QGMText.h"
#include "QGSPage.h"
#include "QGVPage.h"
#include "Rez.h"
#include "ViewProviderPage.h"
#include "PagePrinter.h"

using namespace TechDrawGui;
using namespace TechDraw;
namespace sp = std::placeholders;

/* TRANSLATOR TechDrawGui::MDIViewPage */

TYPESYSTEM_SOURCE_ABSTRACT(TechDrawGui::MDIViewPage, Gui::MDIView)

MDIViewPage::MDIViewPage(ViewProviderPage* pageVp, Gui::Document* doc, QWidget* parent)
    : Gui::MDIView(doc, parent), m_vpPage(pageVp)
{
    setMouseTracking(true);

    m_toggleKeepUpdatedAction = new QAction(tr("Toggle &Keep Updated"), this);
    connect(m_toggleKeepUpdatedAction, &QAction::triggered, this, &MDIViewPage::toggleKeepUpdated);

    m_toggleFrameAction = new QAction(tr("Toggle &Frames"), this);
    connect(m_toggleFrameAction, &QAction::triggered, this, &MDIViewPage::toggleFrame);

    m_exportSVGAction = new QAction(tr("&Export SVG"), this);
    connect(m_exportSVGAction, &QAction::triggered, this, qOverload<>(&MDIViewPage::saveSVG));

    m_exportDXFAction = new QAction(tr("Export DXF"), this);
    connect(m_exportDXFAction, &QAction::triggered, this, qOverload<>(&MDIViewPage::saveDXF));

    m_exportPDFAction = new QAction(tr("Export PDF"), this);
    connect(m_exportPDFAction, &QAction::triggered, this, qOverload<>(&MDIViewPage::savePDF));

    m_printAllAction = new QAction(tr("Print All Pages"), this);
    connect(m_printAllAction, &QAction::triggered, this, qOverload<>(&MDIViewPage::printAllPages));

    isSelectionBlocked = false;

    QString tabText = QString::fromUtf8(pageVp->getDrawPage()->getNameInDocument());
    tabText += QString::fromUtf8("[*]");
    setWindowTitle(tabText);

    //NOLINTBEGIN
    //get informed by App side about deleted DocumentObjects
    App::Document* appDoc = m_vpPage->getDocument()->getDocument();
    auto bnd = std::bind(&MDIViewPage::onDeleteObject, this, sp::_1);
    connectDeletedObject = appDoc->signalDeletedObject.connect(bnd);
    //NOLINTEND

    m_pagePrinter = new PagePrinter(m_vpPage);
    m_pagePrinter->setOwner(this);
}

MDIViewPage::~MDIViewPage()
{
    connectDeletedObject.disconnect();
    delete m_pagePrinter;
}

void MDIViewPage::setScene(QGSPage* scene, QGVPage* viewWidget)
{
    m_scene = scene;
    setCentralWidget(viewWidget);//this makes viewWidget a Qt child of MDIViewPage
    QObject::connect(scene, &QGSPage::selectionChanged, this, &MDIViewPage::sceneSelectionChanged);
    if (m_pagePrinter) {
        m_pagePrinter->setScene(m_scene);
    }
}

void MDIViewPage::setDocumentObject(const std::string& name)
{
    m_objectName = name;
    setObjectName(Base::Tools::fromStdString(name));
}

void MDIViewPage::setDocumentName(const std::string& name) { m_documentName = name; }

void MDIViewPage::closeEvent(QCloseEvent* event)
{
    //    Base::Console().Message("MDIVP::closeEvent()\n");
    MDIView::closeEvent(event);
    if (!event->isAccepted()) {
        return;
    }
    detachSelection();

    blockSceneSelection(true);
    // when closing the view from GUI notify the view provider to mark it invisible
    if (_pcDocument && !m_objectName.empty()) {
        App::Document* doc = _pcDocument->getDocument();
        if (doc) {
            App::DocumentObject* obj = doc->getObject(m_objectName.c_str());
            Gui::ViewProvider* vp = _pcDocument->getViewProvider(obj);
            if (vp) {
                vp->hide();
            }
        }
    }
    blockSceneSelection(false);
}

void MDIViewPage::onDeleteObject(const App::DocumentObject& obj)
{
    //if this page has a QView for this obj, delete it.
    blockSceneSelection(true);
    if (obj.isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
        (void)m_scene->removeQViewByName(obj.getNameInDocument());
    }
    blockSceneSelection(false);
}

bool MDIViewPage::onMsg(const char* pMsg, const char**)
{
    Gui::Document* doc(getGuiDocument());

    if (!doc) {
        return false;
    }
    else if (strcmp("ViewFit", pMsg) == 0) {
        viewAll();
        return true;
    }
    else if (strcmp("Save", pMsg) == 0) {
        doc->save();
        return true;
    }
    else if (strcmp("SaveAs", pMsg) == 0) {
        doc->saveAs();
        return true;
    }
    else if (strcmp("Undo", pMsg) == 0) {
        doc->undo(1);
        Gui::Command::updateActive();
        fixSceneDependencies();    // check QGraphicsScene item parenting
        return true;
    }
    else if (strcmp("Redo", pMsg) == 0) {
        doc->redo(1);
        Gui::Command::updateActive();
        return true;
    }
    else if (strcmp("ZoomIn", pMsg) == 0) {
        zoomIn();
        return true;
    }
    else if (strcmp("ZoomOut", pMsg) == 0) {
        zoomOut();
        return true;
    }

    return false;
}

bool MDIViewPage::onHasMsg(const char* pMsg) const
{
    if (strcmp("ViewFit", pMsg) == 0) {
        return true;
    }
    else if (strcmp("Redo", pMsg) == 0 && getAppDocument()->getAvailableRedos() > 0) {
        return true;
    }
    else if (strcmp("Undo", pMsg) == 0 && getAppDocument()->getAvailableUndos() > 0) {
        return true;
    }
    else if (strcmp("Print", pMsg) == 0) {
        return true;
    }
    else if (strcmp("Save", pMsg) == 0) {
        return true;
    }
    else if (strcmp("SaveAs", pMsg) == 0) {
        return true;
    }
    else if (strcmp("PrintPreview", pMsg) == 0) {
        return true;
    }
    else if (strcmp("PrintPdf", pMsg) == 0) {
        return true;
    }
    else if (strcmp("PrintAll", pMsg) == 0) {
        return true;
    }
    else if (strcmp("ZoomIn", pMsg) == 0) {
        return true;
    }
    else if (strcmp("ZoomOut", pMsg) == 0) {
        return true;
    }
    return false;
}

// handle a zoomIn message from the menu
void MDIViewPage::zoomIn()
{
    m_vpPage->getQGVPage()->zoomIn();
}

// handle a zoomOut message from the menu
void MDIViewPage::zoomOut()
{
    m_vpPage->getQGVPage()->zoomOut();
}

// called by ViewProvider when Page feature Label changes
void MDIViewPage::setTabText(std::string tabText)
{
    if (!isPassive() && !tabText.empty()) {
        QString cap = QString::fromLatin1("%1 [*]").arg(QString::fromUtf8(tabText.c_str()));
        setWindowTitle(cap);
    }
}

// advise the page to check QGraphicsScene parent/child relationships after undo
void MDIViewPage::fixSceneDependencies()
{
    if (getViewProviderPage()) {
        getViewProviderPage()->fixSceneDependencies();
    }
}

//**** printing routines

/// MDIViewPage handles those aspects of printing that require user interaction, such
/// as file name selection and error messages
/// PagePrinter handles the actual printing mechanics.

/// overrides of MDIView print methods so that they print the QGraphicsScene instead
/// of the COIN3d scenegraph.
void MDIViewPage::printPdf()
{
//    Base::Console().Message("MDIVP::printPdf()\n");
    QStringList filter;
    filter << QObject::tr("PDF (*.pdf)");
    filter << QObject::tr("All Files (*.*)");
    QString fn =
        Gui::FileDialog::getSaveFileName(Gui::getMainWindow(), QObject::tr("Export Page As PDF"),
                                         QString(), filter.join(QLatin1String(";;")));
    if (fn.isEmpty()) {
        return;
    }

    Gui::WaitCursor wc;
    std::string utf8Content = fn.toUtf8().constData();
    if (m_pagePrinter) {
        m_pagePrinter->printPdf(utf8Content);
    }
}

void MDIViewPage::print()
{
//    Base::Console().Message("MDIVP::print()\n");

    if (!m_pagePrinter) {
        return;
    }
    m_pagePrinter->getPaperAttributes();

    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    if (m_pagePrinter->getPaperSize() == QPageSize::Custom) {
        printer.setPageSize(QPageSize(QSizeF(m_pagePrinter->getPageWidth(), m_pagePrinter->getPageHeight()), QPageSize::Millimeter));
    }
    else {
        printer.setPageSize(QPageSize(m_pagePrinter->getPaperSize()));
    }
    printer.setPageOrientation(m_pagePrinter->getOrientation());

    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) {
        print(&printer);
    }
}

void MDIViewPage::printPreview()
{
//    Base::Console().Message("MDIVP::printPreview()\n");

    if (!m_pagePrinter) {
        return;
    }
    m_pagePrinter->getPaperAttributes();

    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    if (m_pagePrinter->getPaperSize() == QPageSize::Custom) {
        printer.setPageSize(QPageSize(QSizeF(m_pagePrinter->getPageWidth(), m_pagePrinter->getPageHeight()), QPageSize::Millimeter));
    }
    else {
        printer.setPageSize(QPageSize(m_pagePrinter->getPaperSize()));
    }
    printer.setPageOrientation(m_pagePrinter->getOrientation());

    QPrintPreviewDialog dlg(&printer, this);
    connect(&dlg, &QPrintPreviewDialog::paintRequested, this, qOverload<QPrinter*>(&MDIViewPage::print));
    dlg.exec();
}


void MDIViewPage::print(QPrinter* printer)
{
//    Base::Console().Message("MDIVP::print(printer)\n");
    if (!m_pagePrinter) {
        return;
    }
    m_pagePrinter->getPaperAttributes();
    // As size of the render area paperRect() should be used. When performing a real
    // print pageRect() may also work but the output is cropped at the bottom part.
    // So, independent whether pageRect() or paperRect() is used there is no scaling effect.
    // However, when using a different paper size as set in the drawing template (e.g.
    // DIN A5 instead of DIN A4) then the output is scaled.
    //
    // When creating a PDF file there seems to be no difference between pageRect() and
    // paperRect().
    //
    // When showing the preview of a print paperRect() must be used because with pageRect()
    // a certain scaling effect can be observed and the content becomes smaller.
    QPaintEngine::Type paintType = printer->paintEngine()->type();
    if (printer->outputFormat() == QPrinter::NativeFormat) {
        QPageSize::PageSizeId psPrtSetting = printer->pageLayout().pageSize().id();

        // for the preview a 'Picture' paint engine is used which we don't
        // care if it uses wrong printer settings
        bool doPrint = paintType != QPaintEngine::Picture;

        if (doPrint && printer->pageLayout().orientation() != m_pagePrinter->getOrientation()) {
            int ret = QMessageBox::warning(
                this, tr("Different orientation"),
                tr("The printer uses a different orientation  than the drawing.\n"
                   "Do you want to continue?"),
                QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes) {
                return;
            }
        }
        if (doPrint && psPrtSetting != m_pagePrinter->getPaperSize()) {
            int ret = QMessageBox::warning(
                this, tr("Different paper size"),
                tr("The printer uses a different paper size than the drawing.\n"
                   "Do you want to continue?"),
                QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes) {
                return;
            }
        }
    }

    QPainter p(printer);
    // why use a QPainter to determine if we can write to a file?
    if (!p.isActive() && !printer->outputFileName().isEmpty()) {
        qApp->setOverrideCursor(Qt::ArrowCursor);
        QMessageBox::critical(
            this, tr("Opening file failed"),
            tr("Can not open file %1 for writing.").arg(printer->outputFileName()));
        qApp->restoreOverrideCursor();
        return;
    }
    // free the printer from the painter
    p.end();

    if (m_pagePrinter) {
        m_pagePrinter->print(printer);
    }

}

//static routine to print all pages in a document
void MDIViewPage::printAll(QPrinter* printer, App::Document* doc)
{
//    Base::Console().Message("MDIVP::printAll()\n");
    PagePrinter::printAll(printer, doc);
}

PyObject* MDIViewPage::getPyObject()
{
    if (!pythonObject) {
        pythonObject = new MDIViewPagePy(this);
    }

    Py_INCREF(pythonObject);
    return pythonObject;
}

void MDIViewPage::contextMenuEvent(QContextMenuEvent* event)
{
    //    Base::Console().Message("MDIVP::contextMenuEvent() - reason: %d\n", event->reason());
    QMenu menu;
    menu.addAction(m_toggleFrameAction);
    menu.addAction(m_toggleKeepUpdatedAction);
    menu.addAction(m_exportSVGAction);
    menu.addAction(m_exportDXFAction);
    menu.addAction(m_exportPDFAction);
    menu.addAction(m_printAllAction);
    menu.exec(event->globalPos());
}

void MDIViewPage::toggleFrame() { m_vpPage->toggleFrameState(); }

void MDIViewPage::toggleKeepUpdated()
{
    bool state = m_vpPage->getDrawPage()->KeepUpdated.getValue();
    m_vpPage->getDrawPage()->KeepUpdated.setValue(!state);
}

void MDIViewPage::viewAll()
{
    m_vpPage->getQGVPage()->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void MDIViewPage::saveSVG(std::string filename)
{
    if (m_pagePrinter) {
        m_pagePrinter->saveSVG(filename);
    }
}
void MDIViewPage::saveSVG()
{
    QStringList filter;
    filter << QObject::tr("SVG (*.svg)");
    filter << QObject::tr("All Files (*.*)");
    QString fn =
        Gui::FileDialog::getSaveFileName(Gui::getMainWindow(), QObject::tr("Export page as SVG"),
                                         QString(), filter.join(QLatin1String(";;")));
    if (fn.isEmpty()) {
        return;
    }
    static_cast<void>(blockSelection(true));// avoid to be notified by itself
    saveSVG(Base::Tools::toStdString(fn));
    static_cast<void>(blockSelection(false));
}

void MDIViewPage::saveDXF(std::string filename)
{
    if (m_pagePrinter) {
        m_pagePrinter->saveDXF(filename);
    }
}

void MDIViewPage::saveDXF()
{
    QString defaultDir;
    QString fileName = Gui::FileDialog::getSaveFileName(
        Gui::getMainWindow(), QString::fromUtf8(QT_TR_NOOP("Save DXF file")), defaultDir,
        QString::fromUtf8(QT_TR_NOOP("DXF (*.dxf)")));
    if (fileName.isEmpty()) {
        return;
    }

    std::string sFileName = fileName.toUtf8().constData();
    saveDXF(sFileName);
}

void MDIViewPage::savePDF(std::string filename)
{
    if (m_pagePrinter) {
        m_pagePrinter->savePDF(filename);
    }
}

void MDIViewPage::savePDF()
{
    QString defaultDir;
    QString fileName = Gui::FileDialog::getSaveFileName(
        Gui::getMainWindow(), QString::fromUtf8(QT_TR_NOOP("Save PDF file")), defaultDir,
        QString::fromUtf8(QT_TR_NOOP("PDF (*.pdf)")));
    if (fileName.isEmpty()) {
        return;
    }

    std::string sFileName = fileName.toUtf8().constData();
    savePDF(sFileName);
}

/// a slot for printing all the pages
void MDIViewPage::printAll()
{
    MDIViewPage::printAllPages();
}

//static routine for PrintAll command
/// prints all the pages in the active document
void MDIViewPage::printAllPages()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);

    QPrintDialog dlg(&printer, Gui::getMainWindow());
    if (dlg.exec() == QDialog::Accepted) {
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            return;
        }
        if (printer.outputFileName().isEmpty()) {
            PagePrinter::printAll(&printer, doc);
        }
        else {
            PagePrinter::printAllPdf(&printer, doc);
        }
    }
}

/////////////// Selection Routines ///////////////////
// wf: this is never executed???
// needs a signal from Scene? hoverEvent?  Scene does not emit signal for "preselect"
// there is no "preSelect" signal from Gui either.
void MDIViewPage::preSelectionChanged(const QPoint& pos)
{
    QObject* obj = QObject::sender();

    if (!obj) {
        return;
    }

    auto view(dynamic_cast<QGIView*>(obj));
    if (!view) {
        return;
    }

    QGraphicsItem* parent = view->parentItem();
    if (!parent) {
        return;
    }

    TechDraw::DrawView* viewObj = view->getViewObject();
    std::stringstream ss;

    QGIFace* face = dynamic_cast<QGIFace*>(obj);
    QGIEdge* edge = dynamic_cast<QGIEdge*>(obj);
    QGIVertex* vert = dynamic_cast<QGIVertex*>(obj);
    if (edge) {
        ss << "Edge" << edge->getProjIndex();
        //bool accepted =
        static_cast<void>(Gui::Selection().setPreselect(viewObj->getDocument()->getName(),
                                                        viewObj->getNameInDocument(),
                                                        ss.str().c_str(), pos.x(), pos.y(), 0));
    }
    else if (vert) {
        ss << "Vertex" << vert->getProjIndex();
        //bool accepted =
        static_cast<void>(Gui::Selection().setPreselect(viewObj->getDocument()->getName(),
                                                        viewObj->getNameInDocument(),
                                                        ss.str().c_str(), pos.x(), pos.y(), 0));
    }
    else if (face) {
        ss << "Face"
           << face->getProjIndex();//TODO: SectionFaces have ProjIndex = -1. (but aren't selectable?) Problem?
        //bool accepted =
        static_cast<void>(Gui::Selection().setPreselect(viewObj->getDocument()->getName(),
                                                        viewObj->getNameInDocument(),
                                                        ss.str().c_str(), pos.x(), pos.y(), 0));
    }
    else {
        ss << "";
        Gui::Selection().setPreselect(viewObj->getDocument()->getName(),
                                      viewObj->getNameInDocument(), ss.str().c_str(), pos.x(),
                                      pos.y(), 0);
    }
}

//flag to prevent selection activity within mdivp
void MDIViewPage::blockSceneSelection(const bool isBlocked) { isSelectionBlocked = isBlocked; }


//Set all QGIViews to unselected state
void MDIViewPage::clearSceneSelection()
{
    //    Base::Console().Message("MDIVP::clearSceneSelection()\n");
    blockSceneSelection(true);
    m_qgSceneSelected.clear();

    std::vector<QGIView*> views = m_scene->getViews();

    // Iterate through all views and unselect all
    for (std::vector<QGIView*>::iterator it = views.begin(); it != views.end(); ++it) {
        QGIView* item = *it;
        bool state = item->isSelected();

        //handle oddballs
        QGIViewDimension* dim = dynamic_cast<QGIViewDimension*>(*it);
        if (dim) {
            state = dim->getDatumLabel()->isSelected();
        }
        else {
            QGIViewBalloon* bal = dynamic_cast<QGIViewBalloon*>(*it);
            if (bal) {
                state = bal->getBalloonLabel()->isSelected();
            }
        }

        if (state) {
            item->setGroupSelection(false);
            item->updateView();
        }
    }

    blockSceneSelection(false);
}

//!Update QGIView's selection state based on Selection made outside Drawing Interface
void MDIViewPage::selectQGIView(App::DocumentObject* obj, const bool isSelected)
{
    QGIView* view = m_scene->findQViewForDocObj(obj);

    blockSceneSelection(true);
    if (view) {
        view->setGroupSelection(isSelected);
        view->updateView();
    }
    blockSceneSelection(false);
}

//! invoked by selection change made in Tree via father MDIView
//really "onTreeSelectionChanged"
void MDIViewPage::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    //    Base::Console().Message("MDIVP::onSelectionChanged()\n");
    std::vector<Gui::SelectionSingleton::SelObj> selObjs =
        Gui::Selection().getSelection(msg.pDocName);
    if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        clearSceneSelection();
    }
    else if (msg.Type == Gui::SelectionChanges::SetSelection) {//replace entire selection set
        clearSceneSelection();
        blockSceneSelection(true);
        for (auto& so : selObjs) {
            if (so.pObject->isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
                selectQGIView(so.pObject, true);
            }
        }
        blockSceneSelection(false);
    }
    else if (msg.Type == Gui::SelectionChanges::AddSelection) {
        blockSceneSelection(true);
        for (auto& so : selObjs) {
            if (so.pObject->isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
                selectQGIView(so.pObject, true);
            }
        }
        blockSceneSelection(false);
    }
}

//! maintain QGScene selected items in selection order
void MDIViewPage::sceneSelectionManager()
{
    //    Base::Console().Message("MDIVP::sceneSelectionManager()\n");
    QList<QGraphicsItem*> sceneSel = m_scene->selectedItems();

    if (sceneSel.isEmpty()) {
        m_qgSceneSelected.clear();//TODO: need to signal somebody?  Tree? handled elsewhere
        //clearSelection
        return;
    }

    if (m_qgSceneSelected.isEmpty() && !sceneSel.isEmpty()) {
        m_qgSceneSelected.push_back(sceneSel.front());
        return;
    }

    //add to m_qgSceneSelected anything that is in q_sceneSel
    for (auto qts : sceneSel) {
        bool found = false;
        for (auto ms : qAsConst(m_qgSceneSelected)) {
            if (qts == ms) {
                found = true;
                break;
            }
        }
        if (!found) {
            m_qgSceneSelected.push_back(qts);
            break;
        }
    }

    //remove items from m_qgSceneSelected that are not in q_sceneSel
    QList<QGraphicsItem*> m_new;
    for (auto m : qAsConst(m_qgSceneSelected)) {
        for (auto q : sceneSel) {
            if (m == q) {
                m_new.push_back(m);
                break;
            }
        }
    }
    m_qgSceneSelected = m_new;
}

//! update Tree Selection from QGraphicsScene selection
//triggered by m_scene signal
void MDIViewPage::sceneSelectionChanged()
{
    sceneSelectionManager();

    if (isSelectionBlocked) {
        return;
    }

    std::vector<Gui::SelectionObject> treeSel = Gui::Selection().getSelectionEx();
    QList<QGraphicsItem*> sceneSel = m_qgSceneSelected;

    //check if really need to change selection
    bool sameSel = compareSelections(treeSel, sceneSel);
    if (sameSel) {
        return;
    }

    setTreeToSceneSelect();
}

//Note: Qt says: "no guarantee of selection order"!!!
void MDIViewPage::setTreeToSceneSelect()
{
    //    Base::Console().Message("MDIVP::setTreeToSceneSelect()\n");
    bool saveBlock = blockSelection(true);// block selectionChanged signal from Tree/Observer
    blockSceneSelection(true);
    Gui::Selection().clearSelection();
    QList<QGraphicsItem*> sceneSel = m_qgSceneSelected;
    for (QList<QGraphicsItem*>::iterator it = sceneSel.begin(); it != sceneSel.end(); ++it) {
        QGIView* itemView = dynamic_cast<QGIView*>(*it);
        if (!itemView) {
            QGIEdge* edge = dynamic_cast<QGIEdge*>(*it);
            if (edge) {
                QGraphicsItem* parent = edge->parentItem();
                if (!parent) {
                    continue;
                }

                QGIView* viewItem = dynamic_cast<QGIView*>(parent);
                if (!viewItem) {
                    continue;
                }

                TechDraw::DrawView* viewObj = viewItem->getViewObject();

                std::stringstream ss;
                ss << "Edge" << edge->getProjIndex();
                //bool accepted =
                static_cast<void>(Gui::Selection().addSelection(viewObj->getDocument()->getName(),
                                                                viewObj->getNameInDocument(),
                                                                ss.str().c_str()));
                showStatusMsg(viewObj->getDocument()->getName(), viewObj->getNameInDocument(),
                              ss.str().c_str());
                continue;
            }

            QGIVertex* vert = dynamic_cast<QGIVertex*>(*it);
            if (vert) {
                QGraphicsItem* parent = vert->parentItem();
                if (!parent) {
                    continue;
                }

                QGIView* viewItem = dynamic_cast<QGIView*>(parent);
                if (!viewItem) {
                    continue;
                }

                TechDraw::DrawView* viewObj = viewItem->getViewObject();

                std::stringstream ss;
                ss << "Vertex" << vert->getProjIndex();
                //bool accepted =
                static_cast<void>(Gui::Selection().addSelection(viewObj->getDocument()->getName(),
                                                                viewObj->getNameInDocument(),
                                                                ss.str().c_str()));
                showStatusMsg(viewObj->getDocument()->getName(), viewObj->getNameInDocument(),
                              ss.str().c_str());
                continue;
            }

            QGIFace* face = dynamic_cast<QGIFace*>(*it);
            if (face) {
                QGraphicsItem* parent = face->parentItem();
                if (!parent) {
                    continue;
                }

                QGIView* viewItem = dynamic_cast<QGIView*>(parent);
                if (!viewItem) {
                    continue;
                }

                TechDraw::DrawView* viewObj = viewItem->getViewObject();

                std::stringstream ss;
                ss << "Face" << face->getProjIndex();
                //bool accepted =
                static_cast<void>(Gui::Selection().addSelection(viewObj->getDocument()->getName(),
                                                                viewObj->getNameInDocument(),
                                                                ss.str().c_str()));
                showStatusMsg(viewObj->getDocument()->getName(), viewObj->getNameInDocument(),
                              ss.str().c_str());
                continue;
            }

            QGIDatumLabel* dimLabel = dynamic_cast<QGIDatumLabel*>(*it);
            if (dimLabel) {
                QGraphicsItem* dimParent = dimLabel->QGraphicsItem::parentItem();
                if (!dimParent) {
                    continue;
                }

                QGIView* dimItem = dynamic_cast<QGIView*>(dimParent);

                if (!dimItem) {
                    continue;
                }

                TechDraw::DrawView* dimObj = dimItem->getViewObject();
                if (!dimObj) {
                    continue;
                }
                const char* name = dimObj->getNameInDocument();
                if (!name) {//can happen during undo/redo if Dim is selected???
                    //Base::Console().Log("INFO - MDIVP::sceneSelectionChanged - dimObj name is null!\n");
                    continue;
                }

                //bool accepted =
                static_cast<void>(Gui::Selection().addSelection(dimObj->getDocument()->getName(),
                                                                dimObj->getNameInDocument()));
            }

            QGMText* mText = dynamic_cast<QGMText*>(*it);
            if (mText) {
                QGraphicsItem* textParent = mText->QGraphicsItem::parentItem();
                if (!textParent) {
                    continue;
                }

                QGIView* parent = dynamic_cast<QGIView*>(textParent);

                if (!parent) {
                    continue;
                }

                TechDraw::DrawView* parentFeat = parent->getViewObject();
                if (!parentFeat) {
                    continue;
                }
                const char* name = parentFeat->getNameInDocument();
                if (!name) {//can happen during undo/redo if Dim is selected???
                    continue;
                }

                //bool accepted =
                static_cast<void>(Gui::Selection().addSelection(
                    parentFeat->getDocument()->getName(), parentFeat->getNameInDocument()));
            }
        }
        else {

            TechDraw::DrawView* viewObj = itemView->getViewObject();
            if (viewObj && !viewObj->isRemoving()) {
                std::string doc_name = viewObj->getDocument()->getName();
                std::string obj_name = viewObj->getNameInDocument();

                Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str());
                showStatusMsg(doc_name.c_str(), obj_name.c_str(), "");
            }
        }
    }

    blockSceneSelection(false);
    blockSelection(saveBlock);
}

bool MDIViewPage::compareSelections(std::vector<Gui::SelectionObject> treeSel,
                                    QList<QGraphicsItem*> sceneSel)
{
    bool result = true;

    if (treeSel.empty() && sceneSel.empty()) {
        return true;
    }
    else if (treeSel.empty() && !sceneSel.empty()) {
        return false;
    }
    else if (!treeSel.empty() && sceneSel.empty()) {
        return false;
    }

    int treeCount = 0;
    int sceneCount = 0;
    int ppCount = 0;
    std::vector<std::string> treeNames;
    std::vector<std::string> sceneNames;

    for (auto tn : treeSel) {
        if (tn.getObject()->isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
            std::string s = tn.getObject()->getNameInDocument();
            treeNames.push_back(s);
        }
    }
    std::sort(treeNames.begin(), treeNames.end());
    treeCount = treeNames.size();

    for (auto sn : sceneSel) {
        QGIView* itemView = dynamic_cast<QGIView*>(sn);
        if (!itemView) {
            QGIDatumLabel* dl = dynamic_cast<QGIDatumLabel*>(sn);
            QGIPrimPath* pp = dynamic_cast<QGIPrimPath*>(sn);//count Vertex/Edge/Face
            if (pp) {
                ppCount++;
            }
            else if (dl) {
                //get dim associated with this label
                QGraphicsItem* qgi = dl->parentItem();
                if (qgi) {
                    QGIViewDimension* vd = dynamic_cast<QGIViewDimension*>(qgi);
                    if (vd) {
                        std::string s = vd->getViewNameAsString();
                        sceneNames.push_back(s);
                    }
                }
            }
        }
        else {
            std::string s = itemView->getViewNameAsString();
            sceneNames.push_back(s);
        }
    }
    std::sort(sceneNames.begin(), sceneNames.end());
    sceneCount = sceneNames.size();

    //different # of DrawView* vs QGIV*
    if (sceneCount != treeCount) {
        return false;
    }

    // even of counts match, have to check that names in scene == names in tree
    auto treePtr = treeNames.begin();
    for (auto& s : sceneNames) {
        if (s == (*treePtr)) {
            treePtr++;
            continue;
        }
        else {
            return false;
        }
    }

    //Objects all match, check subs
    if (treeCount != ppCount) {
        return false;
    }

    return result;
}

///////////////////end Selection Routines //////////////////////

void MDIViewPage::showStatusMsg(const char* string1, const char* string2, const char* string3) const
{
    QString msg = QString::fromLatin1("%1 %2.%3.%4 ")
                      .arg(tr("Selected:"), QString::fromUtf8(string1), QString::fromUtf8(string2),
                           QString::fromUtf8(string3));
    if (Gui::getMainWindow()) {
        Gui::getMainWindow()->showMessage(msg, 6000);
    }
}

// ----------------------------------------------------------------------------

void MDIViewPagePy::init_type()
{
    behaviors().name("MDIViewPagePy");
    behaviors().doc("Python binding class for the MDI view page class");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();

    add_varargs_method("getPage", &MDIViewPagePy::getPage,
                       "getPage() returns the page being displayed");
    add_varargs_method("cast_to_base", &MDIViewPagePy::cast_to_base,
                       "cast_to_base() cast to MDIView class");
    behaviors().readyType();
}

MDIViewPagePy::MDIViewPagePy(MDIViewPage* mdi) : base(mdi) {}

MDIViewPagePy::~MDIViewPagePy() {}

Py::Object MDIViewPagePy::repr()
{
    std::ostringstream s_out;
    if (!getMDIViewPagePtr()) {
        throw Py::RuntimeError("Cannot print representation of deleted object");
    }
    s_out << "MDI view page";
    return Py::String(s_out.str());
}

// Since with PyCXX it's not possible to make a sub-class of MDIViewPy
// a trick is to use MDIViewPy as class member and override getattr() to
// join the attributes of both classes. This way all methods of MDIViewPy
// appear for SheetViewPy, too.
Py::Object MDIViewPagePy::getattr(const char* attrName)
{
    if (!getMDIViewPagePtr()) {
        std::ostringstream s_out;
        s_out << "Cannot access attribute '" << attrName << "' of deleted object";
        throw Py::RuntimeError(s_out.str());
    }
    std::string name(attrName);
    if (name == "__dict__" || name == "__class__") {
        Py::Dict dict_self(BaseType::getattr("__dict__"));
        Py::Dict dict_base(base.getattr("__dict__"));
        for (const auto& it : dict_base) {
            dict_self.setItem(it.first, it.second);
        }
        return dict_self;
    }

    try {
        return BaseType::getattr(attrName);
    }
    catch (Py::AttributeError& e) {
        e.clear();
        return base.getattr(attrName);
    }
}

MDIViewPage* MDIViewPagePy::getMDIViewPagePtr()
{
    return qobject_cast<MDIViewPage*>(base.getMDIViewPtr());
}

Py::Object MDIViewPagePy::getPage(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }
    return Py::asObject(new TechDraw::DrawPagePy(getMDIViewPagePtr()->getPage()));
}

Py::Object MDIViewPagePy::cast_to_base(const Py::Tuple&)
{
    return Gui::MDIViewPy::create(base.getMDIViewPtr());
}

#include <Mod/TechDraw/Gui/moc_MDIViewPage.cpp>
