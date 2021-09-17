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
    #include <QTimer>
    #include <QApplication>
    #include <QContextMenuEvent>
    #include <QFileDialog>
    #include <QGridLayout>
    #include <QGroupBox>
    #include <QListWidget>
    #include <QMenu>
    #include <QMessageBox>
    #include <QPaintEngine>
    #include <QPainter>
    #include <QPrinter>
    #include <QPrintDialog>
    #include <QPrintPreviewDialog>
    #include <boost_signals2.hpp>
    #include <boost_bind_bind.hpp>

#endif  // #ifndef _PreComp_

#include <math.h>

#include "MDIViewPage.h"
#include "MDIViewPagePy.h"

#include <Base/Stream.h>
#include <Base/Tools.h>
#include <Base/gzstream.h>
#include <Base/PyObjectBase.h>
#include <Base/Console.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Gui/FileDialog.h>
#include <Gui/WaitCursor.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Window.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>

#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawTemplate.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewAnnotation.h>
#include <Mod/TechDraw/App/DrawViewClip.h>
#include <Mod/TechDraw/App/DrawViewCollection.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSection.h>
#include <Mod/TechDraw/App/DrawViewSpreadsheet.h>
#include <Mod/TechDraw/App/DrawViewSymbol.h>
#include <Mod/TechDraw/App/DrawViewImage.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawRichAnno.h>
#include <Mod/TechDraw/App/DrawWeldSymbol.h>
#include <Mod/TechDraw/App/DrawTile.h>
#include <Mod/TechDraw/App/DrawTileWeld.h>

#include "Rez.h"
#include "QGIDrawingTemplate.h"
#include "QGIView.h"
#include "QGIViewPart.h"
#include "QGIViewDimension.h"
#include "QGIViewBalloon.h"
#include "QGIViewClip.h"
#include "QGIVertex.h"
#include "QGIEdge.h"
#include "QGIFace.h"
#include "ViewProviderPage.h"
#include "QGVPage.h"
#include "QGILeaderLine.h"
#include "QGIRichAnno.h"
#include "QGMText.h"
#include "QGIWeldSymbol.h"
#include "QGITile.h"


using namespace TechDrawGui;
namespace bp = boost::placeholders;

/* TRANSLATOR TechDrawGui::MDIViewPage */

TYPESYSTEM_SOURCE_ABSTRACT(TechDrawGui::MDIViewPage, Gui::MDIView)

MDIViewPage::MDIViewPage(ViewProviderPage *pageVp, Gui::Document* doc, QWidget* parent)
  : Gui::MDIView(doc, parent),
    m_orientation(QPageLayout::Landscape),
    m_paperSize(QPageSize::A4),
    pagewidth(0.0),
    pageheight(0.0),
    m_vpPage(pageVp)
{

    setMouseTracking(true);
    m_scene = new QGraphicsScene(this);
    m_scene->setItemIndexMethod(QGraphicsScene::NoIndex); //this prevents crash when deleting dims.
                                                          //scene(view?) indices of dirty regions gets
                                                          //out of sync.  missing prepareGeometryChange
                                                          //somewhere???? QTBUG-18021???
    m_view = new QGVPage(pageVp,m_scene,this);

    m_toggleKeepUpdatedAction = new QAction(tr("Toggle &Keep Updated"), this);
    connect(m_toggleKeepUpdatedAction, SIGNAL(triggered()), this, SLOT(toggleKeepUpdated()));

    m_toggleFrameAction = new QAction(tr("Toggle &Frames"), this);
    connect(m_toggleFrameAction, SIGNAL(triggered()), this, SLOT(toggleFrame()));

    m_exportSVGAction = new QAction(tr("&Export SVG"), this);
    connect(m_exportSVGAction, SIGNAL(triggered()), this, SLOT(saveSVG()));

    m_exportDXFAction = new QAction(tr("Export DXF"), this);
    connect(m_exportDXFAction, SIGNAL(triggered()), this, SLOT(saveDXF()));

    m_exportPDFAction = new QAction(tr("Export PDF"), this);
    connect(m_exportPDFAction, SIGNAL(triggered()), this, SLOT(savePDF()));

    isSelectionBlocked = false;

    QString tabText = QString::fromUtf8(pageVp->getDrawPage()->getNameInDocument());
    tabText += QString::fromUtf8("[*]");
    setWindowTitle(tabText);
    setCentralWidget(m_view);            //this makes m_view a Qt child of MDIViewPage

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    QObject::connect(m_timer,SIGNAL(timeout()),this,SLOT(onTimer()));

    // Connect Signals and Slots
    QObject::connect(
        m_view->scene(), SIGNAL(selectionChanged()),
        this           , SLOT  (sceneSelectionChanged())
       );

    //get informed by App side about deleted DocumentObjects
    App::Document* appDoc = m_vpPage->getDocument()->getDocument();
    auto bnd = boost::bind(&MDIViewPage::onDeleteObject, this, bp::_1);
    connectDeletedObject = appDoc->signalDeletedObject.connect(bnd);
}


MDIViewPage::~MDIViewPage()
{
    connectDeletedObject.disconnect();
}

void MDIViewPage::addChildrenToPage(void)
{
     // A fresh page is added and we iterate through its collected children and add these to Canvas View  -MLP
     // if docobj is a featureviewcollection (ex orthogroup), add its child views. if there are ever children that have children,
     // we'll have to make this recursive. -WF
    const std::vector<App::DocumentObject*> &grp = m_vpPage->getDrawPage()->Views.getValues();
    std::vector<App::DocumentObject*> childViews;
    for (std::vector<App::DocumentObject*>::const_iterator it = grp.begin();it != grp.end(); ++it) {
        attachView(*it);
        TechDraw::DrawViewCollection* collect = dynamic_cast<TechDraw::DrawViewCollection *>(*it);
        if (collect) {
            childViews = collect->Views.getValues();
            for (std::vector<App::DocumentObject*>::iterator itChild = childViews.begin();itChild != childViews.end(); ++itChild) {
                attachView(*itChild);
            }
        }
    }
    //when restoring, it is possible for a Dimension to be loaded before the ViewPart it applies to
    //therefore we need to make sure parentage of the graphics representation is set properly. bit of a kludge.
    setDimensionGroups();
    setBalloonGroups();
    setLeaderGroups();

    App::DocumentObject *obj = m_vpPage->getDrawPage()->Template.getValue();
    auto pageTemplate( dynamic_cast<TechDraw::DrawTemplate *>(obj) );
    if( pageTemplate ) {
        attachTemplate(pageTemplate);
        matchSceneRectToTemplate();
    }

    viewAll();
}

void MDIViewPage::matchSceneRectToTemplate(void)
{
    App::DocumentObject *obj = m_vpPage->getDrawPage()->Template.getValue();
    auto pageTemplate( dynamic_cast<TechDraw::DrawTemplate *>(obj) );
    if( pageTemplate ) {
        //make sceneRect 1 pagesize bigger in every direction
        double width  =  Rez::guiX(pageTemplate->Width.getValue());
        double height =  Rez::guiX(pageTemplate->Height.getValue());
        m_view->scene()->setSceneRect(QRectF(-width,-2.0 * height,3.0*width,3.0*height));
    }
}

void MDIViewPage::setDimensionGroups(void)
{
    const std::vector<QGIView *> &allItems = m_view->getViews();
    std::vector<QGIView *>::const_iterator itInspect;
    int dimItemType = QGraphicsItem::UserType + 106;

    for (itInspect = allItems.begin(); itInspect != allItems.end(); itInspect++) {
        if (((*itInspect)->type() == dimItemType) && (!(*itInspect)->group())) {
            QGIView* parent = m_view->findParent((*itInspect));
            if (parent) {
                QGIViewDimension* dim = dynamic_cast<QGIViewDimension*>((*itInspect));
                m_view->addDimToParent(dim,parent);
            }
        }
    }
}

void MDIViewPage::setBalloonGroups(void)
{
    const std::vector<QGIView *> &allItems = m_view->getViews();
    std::vector<QGIView *>::const_iterator itInspect;
    int balloonItemType = QGraphicsItem::UserType + 140;

    for (itInspect = allItems.begin(); itInspect != allItems.end(); itInspect++) {
        if (((*itInspect)->type() == balloonItemType) && (!(*itInspect)->group())) {
            QGIView* parent = m_view->findParent((*itInspect));
            if (parent) {
                QGIViewBalloon* balloon = dynamic_cast<QGIViewBalloon*>((*itInspect));
                m_view->addBalloonToParent(balloon,parent);
            }
        }
    }
}

void MDIViewPage::setLeaderGroups(void)
{
//    Base::Console().Message("MDIVP::setLeaderGroups()\n");
    const std::vector<QGIView *> &allItems = m_view->getViews();
    std::vector<QGIView *>::const_iterator itInspect;
    int leadItemType = QGraphicsItem::UserType + 232;

    //make sure that qgileader belongs to correct parent. 
    //quite possibly redundant
    for (itInspect = allItems.begin(); itInspect != allItems.end(); itInspect++) {
        if (((*itInspect)->type() == leadItemType) && (!(*itInspect)->group())) {
            QGIView* parent = m_view->findParent((*itInspect));
            if (parent) {
                QGILeaderLine* lead = dynamic_cast<QGILeaderLine*>((*itInspect));
                m_view->addLeaderToParent(lead,parent);
            }
        }
    }
}

void MDIViewPage::setDocumentObject(const std::string& name)
{
    m_objectName = name;
}

void MDIViewPage::setDocumentName(const std::string& name)
{
    m_documentName = name;
}

void MDIViewPage::closeEvent(QCloseEvent* ev)
{
    MDIView::closeEvent(ev);
    if (!ev->isAccepted())
        return;
    detachSelection();

    blockSelection(true);
    // when closing the view from GUI notify the view provider to mark it invisible
    if (_pcDocument && !m_objectName.empty()) {
        App::Document* doc = _pcDocument->getDocument();
        if (doc) {
            App::DocumentObject* obj = doc->getObject(m_objectName.c_str());
            Gui::ViewProvider* vp = _pcDocument->getViewProvider(obj);
            if (vp)
                vp->hide();
        }
    }
    blockSelection(false);
}

void MDIViewPage::attachTemplate(TechDraw::DrawTemplate *obj)
{
    m_view->setPageTemplate(obj);
    pagewidth  =  obj->Width.getValue();
    pageheight =  obj->Height.getValue();
    m_paperSize = QPageSize::id(QSizeF(pagewidth, pageheight), QPageSize::Millimeter, QPageSize::FuzzyOrientationMatch);
    if (pagewidth > pageheight) {
        m_orientation = QPageLayout::Landscape;
    } else {
        m_orientation = QPageLayout::Portrait;
    }
}

QPointF MDIViewPage::getTemplateCenter(TechDraw::DrawTemplate *obj)
{
    double cx  =  Rez::guiX(obj->Width.getValue())/2.0;
    double cy =  -Rez::guiX(obj->Height.getValue())/2.0;
    QPointF result(cx,cy);
    return result;
}

void MDIViewPage::centerOnPage(void)
{
    App::DocumentObject *obj = m_vpPage->getDrawPage()->Template.getValue();
    auto pageTemplate( dynamic_cast<TechDraw::DrawTemplate *>(obj) );
    if( pageTemplate ) {
        QPointF viewCenter = getTemplateCenter(pageTemplate);
        m_view->centerOn(viewCenter);
    }
}

bool MDIViewPage::addView(const App::DocumentObject *obj)
{
    return attachView(const_cast<App::DocumentObject*>(obj));
}

bool MDIViewPage::attachView(App::DocumentObject *obj)
{
    auto typeId(obj->getTypeId());

    QGIView *qview(nullptr);

    if (typeId.isDerivedFrom(TechDraw::DrawViewSection::getClassTypeId()) ) {
        qview = m_view->addViewSection( static_cast<TechDraw::DrawViewSection *>(obj) );

    } else if (typeId.isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) ) {
        qview = m_view->addViewPart( static_cast<TechDraw::DrawViewPart *>(obj) );

    } else if (typeId.isDerivedFrom(TechDraw::DrawProjGroup::getClassTypeId()) ) {
        qview = m_view->addProjectionGroup( static_cast<TechDraw::DrawProjGroup *>(obj) );

    } else if (typeId.isDerivedFrom(TechDraw::DrawViewCollection::getClassTypeId()) ) {
        qview = m_view->addDrawView( static_cast<TechDraw::DrawViewCollection *>(obj) );

    } else if (typeId.isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId()) ) {
        qview = m_view->addViewDimension( static_cast<TechDraw::DrawViewDimension *>(obj) );

    } else if (typeId.isDerivedFrom(TechDraw::DrawViewBalloon::getClassTypeId()) ) {
        qview = m_view->addViewBalloon( static_cast<TechDraw::DrawViewBalloon *>(obj) );

    } else if (typeId.isDerivedFrom(TechDraw::DrawViewAnnotation::getClassTypeId()) ) {
        qview = m_view->addDrawViewAnnotation( static_cast<TechDraw::DrawViewAnnotation *>(obj) );

    } else if (typeId.isDerivedFrom(TechDraw::DrawViewSymbol::getClassTypeId()) ) {
        qview = m_view->addDrawViewSymbol( static_cast<TechDraw::DrawViewSymbol *>(obj) );

    } else if (typeId.isDerivedFrom(TechDraw::DrawViewClip::getClassTypeId()) ) {
        qview = m_view->addDrawViewClip( static_cast<TechDraw::DrawViewClip *>(obj) );

    } else if (typeId.isDerivedFrom(TechDraw::DrawViewSpreadsheet::getClassTypeId()) ) {
        qview = m_view->addDrawViewSpreadsheet( static_cast<TechDraw::DrawViewSpreadsheet *>(obj) );

    } else if (typeId.isDerivedFrom(TechDraw::DrawViewImage::getClassTypeId()) ) {
        qview = m_view->addDrawViewImage( static_cast<TechDraw::DrawViewImage *>(obj) );

    } else if (typeId.isDerivedFrom(TechDraw::DrawLeaderLine::getClassTypeId()) ) {
        qview = m_view->addViewLeader( static_cast<TechDraw::DrawLeaderLine *>(obj) );

    } else if (typeId.isDerivedFrom(TechDraw::DrawRichAnno::getClassTypeId()) ) {
        qview = m_view->addRichAnno( static_cast<TechDraw::DrawRichAnno*>(obj) );

    } else if (typeId.isDerivedFrom(TechDraw::DrawWeldSymbol::getClassTypeId()) ) {
        qview = m_view->addWeldSymbol( static_cast<TechDraw::DrawWeldSymbol*>(obj) );

    } else if (typeId.isDerivedFrom(TechDraw::DrawHatch::getClassTypeId()) ) {
        //Hatch is not attached like other Views (since it isn't really a View)
        return true;
    //DrawGeomHatch??

    } else {
        Base::Console().Log("Logic Error - Unknown view type in MDIViewPage::attachView\n");
    }

    return (qview != nullptr);
}

void MDIViewPage::onDeleteObject(const App::DocumentObject& obj)
{
    //if this page has a QView for this obj, delete it.
    blockSelection(true);
    if (obj.isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
        (void) m_view->removeQViewByName(obj.getNameInDocument());
    }
    blockSelection(false);
}

void MDIViewPage::onTimer() {
    fixOrphans(true);
}

void MDIViewPage::updateTemplate(bool forceUpdate)
{
    App::DocumentObject *templObj = m_vpPage->getDrawPage()->Template.getValue();
    // TODO: what if template has been deleted? templObj will be NULL. segfault?
    if (!templObj) {
        Base::Console().Log("INFO - MDIViewPage::updateTemplate - Page: %s has NO template!!\n",m_vpPage->getDrawPage()->getNameInDocument());
        return;
    }

    if(m_vpPage->getDrawPage()->Template.isTouched() || templObj->isTouched()) {
        // Template is touched so update

        if(forceUpdate ||
           (templObj && templObj->isTouched() && templObj->isDerivedFrom(TechDraw::DrawTemplate::getClassTypeId())) ) {

            QGITemplate *qItemTemplate = m_view->getTemplate();

            if(qItemTemplate) {
                TechDraw::DrawTemplate *pageTemplate = dynamic_cast<TechDraw::DrawTemplate *>(templObj);
                qItemTemplate->setTemplate(pageTemplate);
                qItemTemplate->updateView();
            }
        }
    }
}

//this is time consuming. should only be used when there is a problem.
void MDIViewPage::fixOrphans(bool force)
{
    if(!force) {
        m_timer->start(100);
        return;
    }
    m_timer->stop();

    // get all the DrawViews for this page, including the second level ones
    // if we ever have collections of collections, we'll need to revisit this
    TechDraw::DrawPage* thisPage = m_vpPage->getDrawPage();

    if(!thisPage->getNameInDocument())
        return;

    std::vector<App::DocumentObject*> pChildren  = thisPage->getAllViews();

    // if dv doesn't have a graphic, make one
    for (auto& dv: pChildren) {
        if (dv->isRemoving()) {
            continue;
        }
        QGIView* qv = m_view->findQViewForDocObj(dv);
        if (qv == nullptr) {
            attachView(dv);
        }
    }

    // if qView doesn't have a Feature on this Page, delete it
    std::vector<QGIView*> qvs = m_view->getViews();
    App::Document* doc = getAppDocument();
    for (auto& qv: qvs) {
        App::DocumentObject* obj = doc->getObject(qv->getViewName());
        if (obj == nullptr) {
            m_view->removeQView(qv);
        } else {
            TechDraw::DrawPage* pp = qv->getViewObject()->findParentPage();
            /** avoid crash where a view might have more than one parent page
             * if the user duplicated the page without duplicating dependencies
             */
            int numParentPages = qv->getViewObject()->countParentPages();
            if (thisPage != pp && numParentPages == 0) {
               m_view->removeQView(qv);
            }
        }
    }
}

//NOTE: this doesn't add missing views.  see fixOrphans()
void MDIViewPage::redrawAllViews()
{
    const std::vector<QGIView *> &upviews = m_view->getViews();
    for(std::vector<QGIView *>::const_iterator it = upviews.begin(); it != upviews.end(); ++it) {
            (*it)->updateView(true);
    }
}

//NOTE: this doesn't add missing views.   see fixOrphans()
void MDIViewPage::redraw1View(TechDraw::DrawView* dv)
{
    std::string dvName = dv->getNameInDocument();
    const std::vector<QGIView *> &upviews = m_view->getViews();
    for(std::vector<QGIView *>::const_iterator it = upviews.begin(); it != upviews.end(); ++it) {
        std::string qgivName = (*it)->getViewName();
        if(dvName == qgivName) {
            (*it)->updateView(true);
        }
    }
}

void MDIViewPage::findMissingViews(const std::vector<App::DocumentObject*> &list, std::vector<App::DocumentObject*> &missing)
{
    for(std::vector<App::DocumentObject*>::const_iterator it = list.begin(); it != list.end(); ++it) {

        if(!hasQView(*it))
             missing.push_back(*it);

        if((*it)->getTypeId().isDerivedFrom(TechDraw::DrawViewCollection::getClassTypeId())) {
            std::vector<App::DocumentObject*> missingChildViews;
            TechDraw::DrawViewCollection *collection = dynamic_cast<TechDraw::DrawViewCollection *>(*it);
            // Find Child Views recursively
            findMissingViews(collection->Views.getValues(), missingChildViews);

            // Append the views to current missing list
            for(std::vector<App::DocumentObject*>::const_iterator it = missingChildViews.begin(); it != missingChildViews.end(); ++it) {
                missing.push_back(*it);
            }
        }
    }
}

/// Helper function
bool MDIViewPage::hasQView(App::DocumentObject *obj)
{
    const std::vector<QGIView *> &views = m_view->getViews();
    std::vector<QGIView *>::const_iterator qview = views.begin();

    while(qview != views.end()) {
        // Unsure if we can compare pointers so rely on name
        if(strcmp((*qview)->getViewName(), obj->getNameInDocument()) == 0) {
            return true;
        }
        qview++;
    }

    return false;
}


/// Helper function
bool MDIViewPage::orphanExists(const char *viewName, const std::vector<App::DocumentObject*> &list)
{
    for(std::vector<App::DocumentObject*>::const_iterator it = list.begin(); it != list.end(); ++it) {

        //Check child objects too recursively
        if((*it)->isDerivedFrom(TechDraw::DrawViewCollection::getClassTypeId())) {
            TechDraw::DrawViewCollection *collection = dynamic_cast<TechDraw::DrawViewCollection *>(*it);
            if(orphanExists(viewName, collection->Views.getValues()))
                return true;
        }

        // Unsure if we can compare pointers so rely on name
        if(strcmp(viewName, (*it)->getNameInDocument()) == 0) {
            return true;
        }
    }
    return false;
}


bool MDIViewPage::onMsg(const char *pMsg, const char **)
{
    Gui::Document *doc(getGuiDocument());

    if (!doc) {
        return false;
    } else if (strcmp("ViewFit", pMsg) == 0) {
        viewAll();
        return true;
    } else if (strcmp("Save", pMsg) == 0 ) {
        doc->save();
        return true;
    } else if (strcmp("SaveAs", pMsg) == 0 ) {
        doc->saveAs();
        return true;
    } else if (strcmp("Undo", pMsg) == 0 ) {
        doc->undo(1);
        Gui::Command::updateActive();
        return true;
    } else if (strcmp("Redo", pMsg) == 0 ) {
        doc->redo(1);
        Gui::Command::updateActive();
        return true;
    }

    return false;
}


bool MDIViewPage::onHasMsg(const char* pMsg) const
{
    if (strcmp("ViewFit",pMsg) == 0)
        return true;
    else if(strcmp("Redo", pMsg) == 0 && getAppDocument()->getAvailableRedos() > 0)
        return true;
    else if(strcmp("Undo", pMsg) == 0 && getAppDocument()->getAvailableUndos() > 0)
        return true;
    else if (strcmp("Print",pMsg) == 0)
        return true;
    else if (strcmp("Save",pMsg) == 0)
        return true;
    else if (strcmp("SaveAs",pMsg) == 0)
        return true;
    else if (strcmp("PrintPreview",pMsg) == 0)
        return true;
    else if (strcmp("PrintPdf",pMsg) == 0)
        return true;
    return false;
}

//called by ViewProvider when Page feature Label changes
void MDIViewPage::setTabText(std::string t)
{
    if (!isPassive() && !t.empty()) {
        QString cap = QString::fromLatin1("%1 [*]")
            .arg(QString::fromUtf8(t.c_str()));
        setWindowTitle(cap);
    }
}

void MDIViewPage::printPdf()
{
    QStringList filter;
    filter << QObject::tr("PDF (*.pdf)");
    filter << QObject::tr("All Files (*.*)");
    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(), QObject::tr("Export Page As PDF"),
                                                  QString(), filter.join(QLatin1String(";;")));
    if (fn.isEmpty()) {
      return;
    }

    Gui::WaitCursor wc;
    std::string utf8Content = fn.toUtf8().constData();
    printPdf(utf8Content);
}

void MDIViewPage::printPdf(std::string file)
{
    if (file.empty()) {
        Base::Console().Warning("MDIViewPage - no file specified\n");
        return;
    }
    QString filename = QString::fromUtf8(file.data(),file.size());
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    printer.setOutputFileName(filename);

    if (m_paperSize == QPageSize::Ledger)  {
        printer.setPageOrientation((QPageLayout::Orientation) (1 - m_orientation));  //reverse 0/1
    } else {
        printer.setPageOrientation(m_orientation);
    }
    if (m_paperSize == QPageSize::Custom) {
        printer.setPageSize(QPageSize(QSizeF(pagewidth, pageheight), QPageSize::Millimeter));
    } else {
        printer.setPageSize(QPageSize(m_paperSize));
    }
    print(&printer);
}

void MDIViewPage::print()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    if (m_paperSize == QPageSize::Custom) {
        printer.setPageSize(QPageSize(QSizeF(pagewidth, pageheight), QPageSize::Millimeter));
    } else {
        printer.setPageSize(QPageSize(m_paperSize));
    }
    printer.setPageOrientation(m_orientation);

    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) {
        print(&printer);
    }
}

void MDIViewPage::printPreview()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    if (m_paperSize == QPageSize::Custom) {
        printer.setPageSize(QPageSize(QSizeF(pagewidth, pageheight), QPageSize::Millimeter));
    } else {
        printer.setPageSize(QPageSize(m_paperSize));
    }
    printer.setPageOrientation(m_orientation);

    QPrintPreviewDialog dlg(&printer, this);
    connect(&dlg, SIGNAL(paintRequested (QPrinter *)),
            this, SLOT(print(QPrinter *)));
    dlg.exec();
}


void MDIViewPage::print(QPrinter* printer)
{
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

        if (doPrint && printer->pageLayout().orientation() != m_orientation) {
            int ret = QMessageBox::warning(this, tr("Different orientation"),
                tr("The printer uses a different orientation  than the drawing.\n"
                   "Do you want to continue?"),
                   QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes)
                return;
        }
        if (doPrint && psPrtSetting != m_paperSize) {
            int ret = QMessageBox::warning(this, tr("Different paper size"),
                tr("The printer uses a different paper size than the drawing.\n"
                   "Do you want to continue?"),
                   QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes)
                return;
        }
    }

    QPainter p(printer);
    if (!p.isActive() && !printer->outputFileName().isEmpty()) {
        qApp->setOverrideCursor(Qt::ArrowCursor);
        QMessageBox::critical(this, tr("Opening file failed"),
            tr("Can not open file %1 for writing.").arg(printer->outputFileName()));
        qApp->restoreOverrideCursor();
        return;
    }

    QRect targetRect = printer->pageLayout().fullRectPixels(printer->resolution());

#ifdef Q_OS_WIN32
    // On Windows the preview looks broken when using paperRect as render area.
    // Although the picture is scaled when using pageRect, it looks just fine.
    if (paintType == QPaintEngine::Picture)
        targetRect = printer->pageLayout().paintRectPixels(printer->resolution());
#endif

    //bool block =
    static_cast<void> (blockConnection(true)); // avoid to be notified by itself
    Gui::Selection().clearSelection();

    bool saveState = m_vpPage->getFrameState();
    m_vpPage->setFrameState(false);
    m_vpPage->setTemplateMarkers(false);
    m_view->refreshViews();

    Gui::Selection().clearSelection();

    App::DocumentObject *obj = m_vpPage->getDrawPage()->Template.getValue();
    auto pageTemplate( dynamic_cast<TechDraw::DrawTemplate *>(obj) );
    double width  =  0.0;
    double height =  0.0;
    if( pageTemplate ) {
      width  =  Rez::guiX(pageTemplate->Width.getValue());
      height =  Rez::guiX(pageTemplate->Height.getValue());
    }
    QRectF sourceRect(0.0,-height,width,height);

    m_view->scene()->render(&p, targetRect,sourceRect);

    // Reset
    m_vpPage->setFrameState(saveState);
    m_vpPage->setTemplateMarkers(saveState);
    m_view->refreshViews();
    //bool block =
    static_cast<void> (blockConnection(false));
}

PyObject* MDIViewPage::getPyObject()
{
    if (!pythonObject)
        pythonObject = new MDIViewPagePy(this);

    Py_INCREF(pythonObject);
    return pythonObject;
}

void MDIViewPage::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    menu.addAction(m_toggleFrameAction);
    menu.addAction(m_toggleKeepUpdatedAction);
    menu.addAction(m_exportSVGAction);
    menu.addAction(m_exportDXFAction);
    menu.addAction(m_exportPDFAction);
    menu.exec(event->globalPos());
}

void MDIViewPage::toggleFrame(void)
{
    m_vpPage->toggleFrameState();
}

void MDIViewPage::toggleKeepUpdated(void)
{
    bool state = m_vpPage->getDrawPage()->KeepUpdated.getValue();
    m_vpPage->getDrawPage()->KeepUpdated.setValue(!state);
}

void MDIViewPage::viewAll()
{
    //m_view->fitInView(m_view->scene()->sceneRect(), Qt::KeepAspectRatio);
    m_view->fitInView(m_view->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void MDIViewPage::saveSVG()
{
    QStringList filter;
    filter << QObject::tr("SVG (*.svg)");
    filter << QObject::tr("All Files (*.*)");
    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(), QObject::tr("Export page as SVG"),
                                                  QString(), filter.join(QLatin1String(";;")));
    if (fn.isEmpty()) {
      return;
    }
    static_cast<void> (blockConnection(true)); // avoid to be notified by itself

    m_view->saveSvg(fn);
}

void MDIViewPage::saveSVG(std::string file)
{
    if (file.empty()) {
        Base::Console().Warning("MDIViewPage - no file specified\n");
        return;
    }
    QString filename = QString::fromUtf8(file.data(),file.size());
    m_view->saveSvg(filename);
}

void MDIViewPage::saveDXF()
{
    QString defaultDir;
    QString fileName = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(),
                                                   QString::fromUtf8(QT_TR_NOOP("Save Dxf File")),
                                                   defaultDir,
                                                   QString::fromUtf8(QT_TR_NOOP("Dxf (*.dxf)")));
    if (fileName.isEmpty()) {
        return;
    }

    std::string sFileName = fileName.toUtf8().constData();
    saveDXF(sFileName);
}

void MDIViewPage::saveDXF(std::string fileName)
{
    TechDraw::DrawPage* page = m_vpPage->getDrawPage();
    std::string PageName = page->getNameInDocument();
    fileName = Base::Tools::escapeEncodeFilename(fileName);
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Save page to dxf"));
    Gui::Command::doCommand(Gui::Command::Doc,"import TechDraw");
    Gui::Command::doCommand(Gui::Command::Doc,"TechDraw.writeDXFPage(App.activeDocument().%s,u\"%s\")",
                            PageName.c_str(),(const char*)fileName.c_str());
    Gui::Command::commitCommand();
}

void MDIViewPage::savePDF()
{
    printPdf();
}

void MDIViewPage::savePDF(std::string file)
{
    printPdf(file);
}

/////////////// Selection Routines ///////////////////
// wf: this is never executed???
// needs a signal from Scene? hoverEvent?  Scene does not emit signal for "preselect"
// there is no "preSelect" signal from Gui either.
void MDIViewPage::preSelectionChanged(const QPoint &pos)
{
    QObject *obj = QObject::sender();

    if(!obj)
        return;

    auto view( dynamic_cast<QGIView *>(obj) );
    if(!view)
            return;

    QGraphicsItem* parent = view->parentItem();
    if(!parent)
        return;

    TechDraw::DrawView *viewObj = view->getViewObject();
    std::stringstream ss;

    QGIFace *face   = dynamic_cast<QGIFace *>(obj);
    QGIEdge *edge   = dynamic_cast<QGIEdge *>(obj);
    QGIVertex *vert = dynamic_cast<QGIVertex *>(obj);
    if(edge) {
        ss << "Edge" << edge->getProjIndex();
        //bool accepted =
        static_cast<void> (Gui::Selection().setPreselect(viewObj->getDocument()->getName()
                                     ,viewObj->getNameInDocument()
                                     ,ss.str().c_str()
                                     ,pos.x()
                                     ,pos.y()
                                     ,0));
    } else if(vert) {
        ss << "Vertex" << vert->getProjIndex();
        //bool accepted =
        static_cast<void> (Gui::Selection().setPreselect(viewObj->getDocument()->getName()
                                     ,viewObj->getNameInDocument()
                                     ,ss.str().c_str()
                                     ,pos.x()
                                     ,pos.y()
                                     ,0));
    } else if(face) {
        ss << "Face" << face->getProjIndex();      //TODO: SectionFaces have ProjIndex = -1. (but aren't selectable?) Problem?
        //bool accepted =
        static_cast<void> (Gui::Selection().setPreselect(viewObj->getDocument()->getName()
                                     ,viewObj->getNameInDocument()
                                     ,ss.str().c_str()
                                     ,pos.x()
                                     ,pos.y()
                                     ,0));
    } else {
        ss << "";
        Gui::Selection().setPreselect(viewObj->getDocument()->getName()
                                     ,viewObj->getNameInDocument()
                                     ,ss.str().c_str()
                                     ,pos.x()
                                     ,pos.y()
                                     ,0);
    }
}

//flag to prevent selection activity within mdivp
void MDIViewPage::blockSelection(const bool state)
{
    isSelectionBlocked = state;
}


//Set all QGIViews to unselected state
void MDIViewPage::clearSceneSelection()
{
//    Base::Console().Message("MDIVP::clearSceneSelection()\n");
    blockSelection(true);
    m_qgSceneSelected.clear();

    std::vector<QGIView *> views = m_view->getViews();

    // Iterate through all views and unselect all
    for (std::vector<QGIView *>::iterator it = views.begin(); it != views.end(); ++it) {
        QGIView *item = *it;
        bool state = item->isSelected();

        //handle oddballs
        QGIViewDimension* dim = dynamic_cast<QGIViewDimension*>(*it);
        if (dim != nullptr) {
            state = dim->getDatumLabel()->isSelected();
        } else {
            QGIViewBalloon* bal = dynamic_cast<QGIViewBalloon*>(*it);
                if (bal != nullptr) {
                    state = bal->getBalloonLabel()->isSelected();
                }
        }

        if (state) {
            item->setGroupSelection(false);
            item->updateView();
        }
    }

    blockSelection(false);
}

//!Update QGIView's selection state based on Selection made outside Drawing Interface
void MDIViewPage::selectQGIView(App::DocumentObject *obj, const bool isSelected)
{
    QGIView *view = m_view->findQViewForDocObj(obj);

    blockSelection(true);
    if(view) {
        view->setGroupSelection(isSelected);
        view->updateView();
    }
    blockSelection(false);
}

//! invoked by selection change made in Tree via father MDIView
//really "onTreeSelectionChanged"
void MDIViewPage::onSelectionChanged(const Gui::SelectionChanges& msg)
{
//    Base::Console().Message("MDIVP::onSelectionChanged()\n");
    std::vector<Gui::SelectionSingleton::SelObj> selObjs = Gui::Selection().getSelection(msg.pDocName);
    if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        clearSceneSelection();
    } else if(msg.Type == Gui::SelectionChanges::SetSelection) {                     //replace entire selection set
        clearSceneSelection();
        blockSelection(true);
        for (auto& so: selObjs){
            if (so.pObject->isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
                selectQGIView(so.pObject, true);
            }
        }
        blockSelection(false);
    } else if(msg.Type == Gui::SelectionChanges::AddSelection) {
        blockSelection(true);
        for (auto& so: selObjs){
            if (so.pObject->isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
                selectQGIView(so.pObject, true);
            }
        }
        blockSelection(false);
    } else {
        Base::Console().Log("MDIVP::onSelectionChanged - unhandled: %d\n", msg.Type);
    }
}

//! maintain QGScene selected items in selection order
void MDIViewPage::sceneSelectionManager()
{
//    Base::Console().Message("MDIVP::sceneSelectionManager()\n");
    QList<QGraphicsItem*> sceneSel = m_view->scene()->selectedItems();

    if (sceneSel.isEmpty()) {
        m_qgSceneSelected.clear(); //TODO: need to signal somebody?  Tree? handled elsewhere
        //clearSelection
        return;
    }

    if (m_qgSceneSelected.isEmpty() &&
        !sceneSel.isEmpty()) {
        m_qgSceneSelected.push_back(sceneSel.front());
        return;
    }

    //add to m_qgSceneSelected anything that is in q_sceneSel
    for (auto qts: sceneSel) {
        bool found = false;
        for (auto ms: m_qgSceneSelected) {
            if ( qts == ms ) {
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
    for (auto m: m_qgSceneSelected) {
        for (auto q: sceneSel)  {
            if (m == q) {
                m_new.push_back(m);
                break;
            }
        }
    }
    m_qgSceneSelected = m_new;
}

//! update Tree Selection from QGraphicsScene selection
//triggered by m_view->scene() signal
void MDIViewPage::sceneSelectionChanged()
{
    sceneSelectionManager();

//    QList<QGraphicsItem*> dbsceneSel = m_view->scene()->selectedItems();

    if(isSelectionBlocked)  {
        return;
    }

    std::vector<Gui::SelectionObject> treeSel = Gui::Selection().getSelectionEx();
    QList<QGraphicsItem*> sceneSel = m_qgSceneSelected;

    //check if really need to change selection
    bool sameSel = compareSelections(treeSel,sceneSel);
    if (sameSel) {
        return;
    }

    setTreeToSceneSelect();
}

//Note: Qt says: "no guarantee of selection order"!!!
void MDIViewPage::setTreeToSceneSelect(void)
{
    bool saveBlock = blockConnection(true); // block selectionChanged signal from Tree/Observer
    blockSelection(true);
    Gui::Selection().clearSelection();
    QList<QGraphicsItem*> sceneSel = m_qgSceneSelected;
    for (QList<QGraphicsItem*>::iterator it = sceneSel.begin(); it != sceneSel.end(); ++it) {
        QGIView *itemView = dynamic_cast<QGIView *>(*it);
        if(itemView == 0) {
            QGIEdge *edge = dynamic_cast<QGIEdge *>(*it);
            if(edge) {
                QGraphicsItem*parent = edge->parentItem();
                if(!parent)
                    continue;

                QGIView *viewItem = dynamic_cast<QGIView *>(parent);
                if(!viewItem)
                  continue;

                TechDraw::DrawView *viewObj = viewItem->getViewObject();

                std::stringstream ss;
                ss << "Edge" << edge->getProjIndex();
                //bool accepted =
                static_cast<void> (Gui::Selection().addSelection(viewObj->getDocument()->getName(),
                                              viewObj->getNameInDocument(),
                                              ss.str().c_str()));
                showStatusMsg(viewObj->getDocument()->getName(),
                                              viewObj->getNameInDocument(),
                                              ss.str().c_str());
                continue;
            }

            QGIVertex *vert = dynamic_cast<QGIVertex *>(*it);
            if(vert) {
                QGraphicsItem*parent = vert->parentItem();
                if(!parent)
                    continue;

                QGIView *viewItem = dynamic_cast<QGIView *>(parent);
                if(!viewItem)
                  continue;

                TechDraw::DrawView *viewObj = viewItem->getViewObject();

                std::stringstream ss;
                ss << "Vertex" << vert->getProjIndex();
                //bool accepted =
                static_cast<void> (Gui::Selection().addSelection(viewObj->getDocument()->getName(),
                                              viewObj->getNameInDocument(),
                                              ss.str().c_str()));
                showStatusMsg(viewObj->getDocument()->getName(),
                                              viewObj->getNameInDocument(),
                                              ss.str().c_str());
                continue;
            }

            QGIFace *face = dynamic_cast<QGIFace *>(*it);
            if(face) {
                QGraphicsItem*parent = face->parentItem();
                if(!parent)
                    continue;

                QGIView *viewItem = dynamic_cast<QGIView *>(parent);
                if(!viewItem)
                  continue;

                TechDraw::DrawView *viewObj = viewItem->getViewObject();

                std::stringstream ss;
                ss << "Face" << face->getProjIndex();
                //bool accepted =
                static_cast<void> (Gui::Selection().addSelection(viewObj->getDocument()->getName(),
                                              viewObj->getNameInDocument(),
                                              ss.str().c_str()));
                showStatusMsg(viewObj->getDocument()->getName(),
                                              viewObj->getNameInDocument(),
                                              ss.str().c_str());
                continue;
            }

            QGIDatumLabel *dimLabel = dynamic_cast<QGIDatumLabel*>(*it);
            if(dimLabel) {
                QGraphicsItem*dimParent = dimLabel->QGraphicsItem::parentItem();
                if(!dimParent)
                    continue;

                QGIView *dimItem = dynamic_cast<QGIView *>(dimParent);

                if(!dimItem)
                  continue;

                TechDraw::DrawView *dimObj = dimItem->getViewObject();
                if (!dimObj) {
                    continue;
                }
                const char* name = dimObj->getNameInDocument();
                if (!name) {                                   //can happen during undo/redo if Dim is selected???
                    //Base::Console().Log("INFO - MDIVP::sceneSelectionChanged - dimObj name is null!\n");
                    continue;
                }

                //bool accepted =
                static_cast<void> (Gui::Selection().addSelection(dimObj->getDocument()->getName(),dimObj->getNameInDocument()));
            }
            
            QGMText *mText = dynamic_cast<QGMText*>(*it);
            if(mText) {
                QGraphicsItem* textParent = mText->QGraphicsItem::parentItem();
                if(!textParent) {
                    continue;
                }

                QGIView *parent = dynamic_cast<QGIView *>(textParent);

                if(!parent) {
                  continue;
                  }

                TechDraw::DrawView *parentFeat = parent->getViewObject();
                if (!parentFeat) {
                    continue;
                }
                const char* name = parentFeat->getNameInDocument();
                if (!name) {                                   //can happen during undo/redo if Dim is selected???
                    continue;
                }

                //bool accepted =
                static_cast<void> (Gui::Selection().addSelection(parentFeat->getDocument()->getName(),parentFeat->getNameInDocument()));
            }

        } else {

            TechDraw::DrawView *viewObj = itemView->getViewObject();
            if (viewObj && !viewObj->isRemoving()) {
                std::string doc_name = viewObj->getDocument()->getName();
                std::string obj_name = viewObj->getNameInDocument();

                Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str());
                showStatusMsg(doc_name.c_str(),
                              obj_name.c_str(),
                              "");
            }
        }
    }

    blockSelection(false);
    blockConnection(saveBlock);
}

bool MDIViewPage::compareSelections(std::vector<Gui::SelectionObject> treeSel, QList<QGraphicsItem*> sceneSel)
{
    bool result = true;

    if (treeSel.empty() && sceneSel.empty()) {
        return true;
    } else if (treeSel.empty() && !sceneSel.empty()) {
        return false;
    } else if (!treeSel.empty() && sceneSel.empty()) {
        return false;
    }

    int treeCount = 0;
    int sceneCount = 0;
    int subCount = 0;
    int ppCount = 0;
    std::vector<std::string> treeNames;
    std::vector<std::string> sceneNames;

    for (auto tn: treeSel) {
        if (tn.getObject()->isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
            int treeSubs = tn.getSubNames().size();
            subCount += treeSubs;
            std::string s = tn.getObject()->getNameInDocument();
            treeNames.push_back(s);
        }
    }
    std::sort(treeNames.begin(),treeNames.end());
    treeCount = treeNames.size();

    for (auto sn:sceneSel){
        QGIView *itemView = dynamic_cast<QGIView *>(sn);
        if(itemView == 0) {
            QGIDatumLabel* dl = dynamic_cast<QGIDatumLabel*>(sn);
            QGIPrimPath* pp = dynamic_cast<QGIPrimPath*>(sn);   //count Vertex/Edge/Face
            if (pp != nullptr) {
                ppCount++;
            } else if (dl != nullptr) {
                //get dim associated with this label
                QGraphicsItem* qgi = dl->parentItem();
                if (qgi != nullptr) {
                    QGIViewDimension* vd = dynamic_cast<QGIViewDimension*>(qgi);
                    if (vd != nullptr) {
                        std::string s = vd->getViewNameAsString();
                        sceneNames.push_back(s);
                    }
                }
            }
        } else {
            std::string s = itemView->getViewNameAsString();
            sceneNames.push_back(s);
        }
    }
    std::sort(sceneNames.begin(),sceneNames.end());
    sceneCount = sceneNames.size();

    //different # of DrawView* vs QGIV*
    if (sceneCount != treeCount) {
        return false;
    }

// even of counts match, have to check that names in scene == names in tree
    auto treePtr = treeNames.begin();
    for (auto& s: sceneNames){
        if (s == (*treePtr)) {
            treePtr++;
            continue;
        } else {
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

void MDIViewPage::showStatusMsg(const char* s1, const char* s2, const char* s3) const
{
    QString msg = QString::fromLatin1("%1 %2.%3.%4 ")
            .arg(tr("Selected:"),
                 QString::fromUtf8(s1),
                 QString::fromUtf8(s2),
                 QString::fromUtf8(s3));
    if (Gui::getMainWindow()) {
        Gui::getMainWindow()->showMessage(msg,3000);
    }
}

MDIViewPage *MDIViewPage::getFromScene(const QGraphicsScene *scene)
{
    if (scene != nullptr && scene->parent() != nullptr) {
        return dynamic_cast<MDIViewPage *>(scene->parent());
    }

    return nullptr;
}

#include <Mod/TechDraw/Gui/moc_MDIViewPage.cpp>
