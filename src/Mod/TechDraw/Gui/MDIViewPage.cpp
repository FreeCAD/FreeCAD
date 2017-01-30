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
#endif  // #ifndef _PreComp_

#include <math.h>

#include "MDIViewPage.h"

#include <Base/Stream.h>
#include <Base/gzstream.h>
#include <Base/PyObjectBase.h>
#include <Base/Console.h>

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
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSection.h>
#include <Mod/TechDraw/App/DrawViewSpreadsheet.h>
#include <Mod/TechDraw/App/DrawViewSymbol.h>
#include <Mod/TechDraw/App/DrawViewImage.h>

#include "Rez.h"
#include "QGIDrawingTemplate.h"
#include "QGIView.h"
#include "QGIViewPart.h"
#include "QGIViewDimension.h"
#include "QGIViewClip.h"
#include "QGIVertex.h"
#include "QGIEdge.h"
#include "QGIFace.h"
#include "ViewProviderPage.h"
#include "QGVPage.h"


using namespace TechDrawGui;

/* TRANSLATOR TechDrawGui::MDIViewPage */

MDIViewPage::MDIViewPage(ViewProviderPage *pageVp, Gui::Document* doc, QWidget* parent)
  : Gui::MDIView(doc, parent),
    m_orientation(QPrinter::Landscape),
    m_paperSize(QPrinter::A4),
    m_vpPage(pageVp),
    m_frameState(true)
{

    m_scene = new QGraphicsScene(this);
    m_view = new QGVPage(pageVp,m_scene,this);

    m_exportSVGAction = new QAction(tr("&Export SVG"), this);
    connect(m_exportSVGAction, SIGNAL(triggered()), this, SLOT(saveSVG()));

    m_nativeAction = new QAction(tr("&Native"), this);
    m_nativeAction->setCheckable(true);
    m_nativeAction->setChecked(false);
#ifndef QT_NO_OPENGL
    m_glAction = new QAction(tr("&OpenGL"), this);
    m_glAction->setCheckable(true);
#endif
    m_imageAction = new QAction(tr("&Image"), this);
    m_imageAction->setCheckable(true);

#ifndef QT_NO_OPENGL
    m_highQualityAntialiasingAction = new QAction(tr("&High Quality Antialiasing"), this);
    m_highQualityAntialiasingAction->setEnabled(false);
    m_highQualityAntialiasingAction->setCheckable(true);
    m_highQualityAntialiasingAction->setChecked(false);
    connect(m_highQualityAntialiasingAction, SIGNAL(toggled(bool)),
            m_view, SLOT(setHighQualityAntialiasing(bool)));
#endif

    isSelectionBlocked = false;

    QActionGroup *rendererGroup = new QActionGroup(this);
    rendererGroup->addAction(m_nativeAction);
#ifndef QT_NO_OPENGL
    rendererGroup->addAction(m_glAction);
#endif
    rendererGroup->addAction(m_imageAction);
    connect(rendererGroup, SIGNAL(triggered(QAction *)),
            this, SLOT(setRenderer(QAction *)));

    setWindowTitle(tr("dummy[*]"));      //Yuck. prevents "QWidget::setWindowModified: The window title does not contain a '[*]' placeholder"
    setCentralWidget(m_view);            //this makes m_view a Qt child of MDIViewPage

    // Connect Signals and Slots
    QObject::connect(
        m_view->scene(), SIGNAL(selectionChanged()),
        this           , SLOT  (selectionChanged())
       );

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

    App::DocumentObject *obj = m_vpPage->getDrawPage()->Template.getValue();
    auto pageTemplate( dynamic_cast<TechDraw::DrawTemplate *>(obj) );
    if( pageTemplate ) {
        //make sceneRect 1 pagesize bigger in every direction
        double width  =  Rez::guiX(pageTemplate->Width.getValue());
        double height =  Rez::guiX(pageTemplate->Height.getValue());
        m_view->scene()->setSceneRect(QRectF(-width,-2.0 * height,3.0*width,3.0*height));
        attachTemplate(pageTemplate);
        viewAll();
    }
}


MDIViewPage::~MDIViewPage()
{
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

void MDIViewPage::setDocumentObject(const std::string& name)
{
    m_objectName = name;
}


void MDIViewPage::closeEvent(QCloseEvent* ev)
{
    MDIView::closeEvent(ev);
    if (!ev->isAccepted())
        return;

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
}


void MDIViewPage::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    menu.addAction(m_exportSVGAction);
    QMenu* submenu = menu.addMenu(tr("&Renderer"));
    submenu->addAction(m_nativeAction);
    submenu->addAction(m_glAction);
    submenu->addAction(m_imageAction);
    submenu->addSeparator();
    submenu->addAction(m_highQualityAntialiasingAction);
    menu.exec(event->globalPos());
}


void MDIViewPage::attachTemplate(TechDraw::DrawTemplate *obj)
{
    m_view->setPageTemplate(obj);
    double width  =  obj->Width.getValue();
    double height =  obj->Height.getValue();
    m_paperSize = getPaperSize(int(round(width)),int(round(height)));
    if (width > height) {
        m_orientation = QPrinter::Landscape;
    } else {
        m_orientation = QPrinter::Portrait;
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

    } else if (typeId.isDerivedFrom(TechDraw::DrawHatch::getClassTypeId()) ) {
        //Hatch is not attached like other Views (since it isn't really a View)
        return true;

    } else {
        Base::Console().Log("Logic Error - Unknown view type in MDIViewPage::attachView\n");
    }

    return (qview != nullptr);
}

void MDIViewPage::removeView(QGIView *view)
{
    (void) m_view->removeView(view);
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

void MDIViewPage::updateDrawing(bool forceUpdate)
{
   // We cannot guarantee if the number of graphical representations (QGIVxxxx) have changed so check the number (MLP)
    const std::vector<QGIView *> &graphicsList = m_view->getViews();
    const std::vector<App::DocumentObject*> &pageChildren  = m_vpPage->getDrawPage()->Views.getValues();

    // Count total # DocumentObjects in Page
    unsigned int docObjCount = 0;
    for(std::vector<App::DocumentObject*>::const_iterator it = pageChildren.begin(); it != pageChildren.end(); ++it) {
        App::DocumentObject *docObj = *it;
        if(docObj->getTypeId().isDerivedFrom(TechDraw::DrawViewCollection::getClassTypeId())) {
            TechDraw::DrawViewCollection *collection = dynamic_cast<TechDraw::DrawViewCollection *>(docObj);
            docObjCount += collection->countChildren(); // Include self
        }
        docObjCount += 1;
    }

    if(graphicsList.size() < docObjCount) {
        // there are more DocumentObjects than graphical representations (QGIVxxxx's)
        // Find which DocumentObjects have no graphical representation (QGIVxxxx)
        // Iterate over DocumentObjects without graphical representations and create the QGIVxxxx
        // TODO think of a better algorithm to deal with any changes to views list
        std::vector<App::DocumentObject*> notFnd;
        findMissingViews(pageChildren, notFnd);
        for(std::vector<App::DocumentObject*>::const_iterator it = notFnd.begin(); it != notFnd.end(); ++it) {
            attachView(*it);
        }
    } else if(graphicsList.size() > docObjCount) {
        // There are more graphical representations (QGIVxxxx) than DocumentObjects
        // Remove the orphans
        std::vector<QGIView *>::const_iterator itGraphics = graphicsList.begin();
        std::vector<QGIView *> newGraphicsList;
        bool fnd = false;
        while(itGraphics != graphicsList.end()) {
            fnd = orphanExists((*itGraphics)->getViewName(), pageChildren);
            if(fnd) {
                newGraphicsList.push_back(*itGraphics);
            } else {
                if (m_view->scene() == (*itGraphics)->scene()) {
                    (*itGraphics)->hide();
                    m_view->scene()->removeItem(*itGraphics);
                } else {   // this "shouldn't" happen, but it does
                    Base::Console().Log("ERROR - MDIViewPage::updateDrawing - %s already removed from QGraphicsScene\n",
                                        (*itGraphics)->getViewName());
                }
            }
            itGraphics++;
        }

        // Update the QGVPage (QGraphicsView) list of QGIVxxxx
        m_view->setViews(newGraphicsList);
    }

    // Update all the QGIVxxxx
    const std::vector<QGIView *> &upviews = m_view->getViews();
    for(std::vector<QGIView *>::const_iterator it = upviews.begin(); it != upviews.end(); ++it) {
        if((*it)->getViewObject()->isTouched() ||
           forceUpdate) {
            (*it)->updateView(forceUpdate);
        }
    }
}

void MDIViewPage::redrawAllViews()
{
    const std::vector<QGIView *> &upviews = m_view->getViews();
    for(std::vector<QGIView *>::const_iterator it = upviews.begin(); it != upviews.end(); ++it) {
            (*it)->updateView(true);
    }
}

void MDIViewPage::redraw1View(TechDraw::DrawView* dv)
{
    std::string dvName = dv->getNameInDocument();
    const std::vector<QGIView *> &upviews = m_view->getViews();
    for(std::vector<QGIView *>::const_iterator it = upviews.begin(); it != upviews.end(); ++it) {
        std::string qgivName = (*it)->getViewObject()->getNameInDocument();
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
        if(strcmp((*qview)->getViewObject()->getNameInDocument(), obj->getNameInDocument()) == 0) {
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
        Gui::Command::updateActive();
        return true;
    } else if (strcmp("SaveAs", pMsg) == 0 ) {
        doc->saveAs();
        Gui::Command::updateActive();
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


void MDIViewPage::onRelabel(Gui::Document *pDoc)
{
    if (!bIsPassive && pDoc) {
        QString cap = QString::fromLatin1("%1 : %2[*]")
            .arg(QString::fromUtf8(pDoc->getDocument()->Label.getValue()))
            .arg(objectName());
        setWindowTitle(cap);
    }
}

void MDIViewPage::printPdf()
{
    Gui::FileOptionsDialog dlg(this, 0);
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setWindowTitle(tr("Export PDF"));
    dlg.setNameFilters(QStringList() << QString::fromLatin1("%1 (*.pdf)").arg(tr("PDF file")));

    QGridLayout *gridLayout;
    QGridLayout *formLayout;
    QGroupBox *groupBox;
    QListWidget *listWidget;
    QListWidgetItem* item;
    QWidget *form = new QWidget(&dlg);
    form->resize(40, 300);
    formLayout = new QGridLayout(form);
    groupBox = new QGroupBox(form);
    gridLayout = new QGridLayout(groupBox);
    listWidget = new QListWidget(groupBox);
    gridLayout->addWidget(listWidget, 0, 0, 1, 1);
    formLayout->addWidget(groupBox, 0, 0, 1, 1);

    //the "Extended" part of the dialog
    //doesn't have any impact on result?
    groupBox->setTitle(tr("Page sizes"));
    item = new QListWidgetItem(tr("A0"), listWidget);
    item->setData(Qt::UserRole, QVariant(QPrinter::A0));
    item = new QListWidgetItem(tr("A1"), listWidget);
    item->setData(Qt::UserRole, QVariant(QPrinter::A1));
    item = new QListWidgetItem(tr("A2"), listWidget);
    item->setData(Qt::UserRole, QVariant(QPrinter::A2));
    item = new QListWidgetItem(tr("A3"), listWidget);
    item->setData(Qt::UserRole, QVariant(QPrinter::A3));
    item = new QListWidgetItem(tr("A4"), listWidget);
    item->setData(Qt::UserRole, QVariant(QPrinter::A4));
    item = new QListWidgetItem(tr("A5"), listWidget);
    item->setData(Qt::UserRole, QVariant(QPrinter::A5));
    item = new QListWidgetItem(tr("Letter"), listWidget);
    item->setData(Qt::UserRole, QVariant(QPrinter::Letter));
    item = new QListWidgetItem(tr("Legal"), listWidget);
    item->setData(Qt::UserRole, QVariant(QPrinter::Legal));
    //listWidget->item(4)->setSelected(true); // by default A4
    int index = 4; // by default A4
    for (int i=0; i<listWidget->count(); i++) {
        if (listWidget->item(i)->data(Qt::UserRole).toInt() == m_paperSize) {
            index = i;
            break;
        }
    }
    listWidget->item(index)->setSelected(true);
    dlg.setOptionsWidget(Gui::FileOptionsDialog::ExtensionRight, form, false);

    if (dlg.exec() == QDialog::Accepted) {
        Gui::WaitCursor wc;
        QString filename = dlg.selectedFiles().front();
        QPrinter printer(QPrinter::HighResolution);
        printer.setFullPage(true);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filename);
        printer.setOrientation(m_orientation);
        printer.setPaperSize(m_paperSize);
        QList<QListWidgetItem*> items = listWidget->selectedItems();

//        if (items.size() == 1) {
//            int AX = items.front()->data(Qt::UserRole).toInt();
//            printer.setPaperSize(QPrinter::PaperSize(AX));
//        }

        print(&printer);
    }
}


void MDIViewPage::print()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    printer.setPaperSize(m_paperSize);
    printer.setOrientation(m_orientation);
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) {
        print(&printer);
    }
}

void MDIViewPage::printPreview()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    printer.setPaperSize(m_paperSize);
    printer.setOrientation(m_orientation);

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
        int w = printer->widthMM();
        int h = printer->heightMM();
        QPrinter::PaperSize psPrtCalcd = getPaperSize(w, h);
        QPrinter::PaperSize psPrtSetting = printer->paperSize();

        // for the preview a 'Picture' paint engine is used which we don't
        // care if it uses wrong printer settings
        bool doPrint = paintType != QPaintEngine::Picture;

        if (doPrint && printer->orientation() != m_orientation) {
            int ret = QMessageBox::warning(this, tr("Different orientation"),
                tr("The printer uses a different orientation  than the drawing.\n"
                   "Do you want to continue?"),
                   QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes)
                return;
        }
        else if (doPrint && psPrtCalcd != m_paperSize) {
            int ret = QMessageBox::warning(this, tr("Different paper size"),
                tr("The printer uses a different paper size than the drawing.\n"
                   "Do you want to continue?"),
                   QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes)
                return;
        }
        else if (doPrint && psPrtSetting != m_paperSize) {
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
            tr("Can't open file '%1' for writing.").arg(printer->outputFileName()));
        qApp->restoreOverrideCursor();
        return;
    }

    QRect targetRect = printer->paperRect();
#ifdef Q_OS_WIN32
    // On Windows the preview looks broken when using paperRect as render area.
    // Although the picture is scaled when using pageRect, it looks just fine.
    if (paintType == QPaintEngine::Picture)
        targetRect = printer->pageRect();
#endif

    //bool block =
    static_cast<void> (blockConnection(true)); // avoid to be notified by itself
    Gui::Selection().clearSelection();

    m_view->toggleMarkers(false);
    m_view->scene()->update();

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
    m_view->toggleMarkers(true);
}


QPrinter::PaperSize MDIViewPage::getPaperSize(int w, int h) const
{
    static const float paperSizes[][2] = {
        {210, 297}, // A4
        {176, 250}, // B5
        {215.9f, 279.4f}, // Letter
        {215.9f, 355.6f}, // Legal
        {190.5f, 254}, // Executive
        {841, 1189}, // A0
        {594, 841}, // A1
        {420, 594}, // A2
        {297, 420}, // A3
        {148, 210}, // A5
        {105, 148}, // A6
        {74, 105}, // A7
        {52, 74}, // A8
        {37, 52}, // A8
        {1000, 1414}, // B0
        {707, 1000}, // B1
        {31, 44}, // B10
        {500, 707}, // B2
        {353, 500}, // B3
        {250, 353}, // B4
        {125, 176}, // B6
        {88, 125}, // B7
        {62, 88}, // B8
        {33, 62}, // B9
        {163, 229}, // C5E
        {105, 241}, // US Common
        {110, 220}, // DLE
        {210, 330}, // Folio
        {431.8f, 279.4f}, // Ledger
        {279.4f, 431.8f} // Tabloid
    };

    QPrinter::PaperSize ps = QPrinter::Custom;
    for (int i=0; i<30; i++) {
        if (std::abs(paperSizes[i][0]-w) <= 1 &&
            std::abs(paperSizes[i][1]-h) <= 1) {
            ps = static_cast<QPrinter::PaperSize>(i);
            break;
        }
        else
        if (std::abs(paperSizes[i][0]-h) <= 1 &&
            std::abs(paperSizes[i][1]-w) <= 1) {
            ps = static_cast<QPrinter::PaperSize>(i);
            break;
        }
    }
    return ps;
}


void MDIViewPage::setFrameState(bool state)
{
    m_frameState = state;
    m_view->toggleMarkers(state);
    m_view->scene()->update();
}


PyObject* MDIViewPage::getPyObject()
{
    Py_Return;
}

void MDIViewPage::setRenderer(QAction *action)
{
#ifndef QT_NO_OPENGL
    m_highQualityAntialiasingAction->setEnabled(false);
#endif

    if (action == m_nativeAction)
        m_view->setRenderer(QGVPage::Native);
#ifndef QT_NO_OPENGL
    else if (action == m_glAction) {
        m_highQualityAntialiasingAction->setEnabled(true);
        m_view->setRenderer(QGVPage::OpenGL);
    }
#endif
    else if (action == m_imageAction) {
        m_view->setRenderer(QGVPage::Image);
    }
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


/////////////// Selection Routines ///////////////////
// wf: this is never executed???
// needs a signal from Scene? hoverEvent?  Scene does not emit signal for "preselect"
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

void MDIViewPage::blockSelection(const bool state)
{
  isSelectionBlocked = state;
}


void MDIViewPage::clearSelection()
{
  blockSelection(true);
  std::vector<QGIView *> views = m_view->getViews();

  // Iterate through all views and unselect all
  for (std::vector<QGIView *>::iterator it = views.begin(); it != views.end(); ++it) {
      QGIView *item = *it;
      item->setSelected(false);
      item->updateView();
  }

  blockSelection(false);
}

//!Update QGVPage's selection based on Selection made outside Drawing Interace
//invoked from VPP
void MDIViewPage::selectFeature(App::DocumentObject *obj, const bool isSelected)
{
    App::DocumentObject* objCopy = obj;
    TechDraw::DrawHatch* hatchObj = dynamic_cast<TechDraw::DrawHatch*>(objCopy);
    if (hatchObj) {                                                    //Hatch does not have a QGIV of it's own. mark parent as selected.
        objCopy = hatchObj->getSourceView();                           //possible to highlight subObject?
    }
    QGIView *view = m_view->findView(objCopy);

    blockSelection(true);
    if(view) {
        view->setSelected(isSelected);
        view->updateView();
    }
    blockSelection(false);
}

//! invoked by selection change made in Tree?
// wf: seems redundant? executed, but no real logic.
void MDIViewPage::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::ClrSelection) {

    }
    else if (msg.Type == Gui::SelectionChanges::AddSelection ||
             msg.Type == Gui::SelectionChanges::RmvSelection) {
        //bool add = (msg.Type == Gui::SelectionChanges::AddSelection);
        // Check if it is a view object
        std::string feat = msg.pObjectName;
        std::string sub  = msg.pSubName;
    }
    else if (msg.Type == Gui::SelectionChanges::SetSelection) {
        // do nothing here  wf: handled by VPP::onSelectionChanged?
    }
}

//! update FC Selection from QGraphicsScene selection
//trigged by m_view->scene() signal
void MDIViewPage::selectionChanged()
{
    if(isSelectionBlocked)  {
      return;
    }

    QList<QGraphicsItem*> selection = m_view->scene()->selectedItems();
    bool saveBlock = blockConnection(true); // avoid to be notified by itself
    blockSelection(true);

    Gui::Selection().clearSelection();
    for (QList<QGraphicsItem*>::iterator it = selection.begin(); it != selection.end(); ++it) {
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
                QGraphicsItem*dimParent = dimLabel->parentItem();
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
                    //Base::Console().Log("INFO - MDIVP::selectionChanged - dimObj name is null!\n");
                    continue;
                }

                //bool accepted =
                static_cast<void> (Gui::Selection().addSelection(dimObj->getDocument()->getName(),dimObj->getNameInDocument()));
            }
        } else {

            TechDraw::DrawView *viewObj = itemView->getViewObject();

            std::string doc_name = viewObj->getDocument()->getName();
            std::string obj_name = viewObj->getNameInDocument();

            Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str());
            showStatusMsg(doc_name.c_str(),
                          obj_name.c_str(),
                          "");

        }

    }

    blockConnection(saveBlock);
    blockSelection(false);
} // end MDIViewPage::selectionChanged()

///////////////////end Selection Routines //////////////////////

void MDIViewPage::showStatusMsg(const char* s1, const char* s2, const char* s3) const
{
    QString msg = QString::fromUtf8("Selected: ");
    msg.append(QObject::tr(" %1.%2.%3 ")
               .arg(QString::fromUtf8(s1))
               .arg(QString::fromUtf8(s2))
               .arg(QString::fromUtf8(s3))
               );
    if (Gui::getMainWindow()) {
        Gui::getMainWindow()->showMessage(msg,3000);
    }
}

#include <Mod/TechDraw/Gui/moc_MDIViewPage.cpp>
