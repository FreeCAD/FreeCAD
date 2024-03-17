/***************************************************************************
 *   Copyright (c) 2020 Wanderer Fan <wandererfan@gmail.com>               *
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
#include <cmath>

#include <QDomDocument>
#include <QFile>
#include <QPainter>
#include <QSvgGenerator>
#include <QTemporaryFile>
#include <QTextStream>
#endif

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>

#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawParametricTemplate.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawRichAnno.h>
#include <Mod/TechDraw/App/DrawSVGTemplate.h>
#include <Mod/TechDraw/App/DrawTemplate.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewAnnotation.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewClip.h>
#include <Mod/TechDraw/App/DrawViewCollection.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewImage.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSection.h>
#include <Mod/TechDraw/App/DrawViewSpreadsheet.h>
#include <Mod/TechDraw/App/DrawViewSymbol.h>
#include <Mod/TechDraw/App/DrawWeldSymbol.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "QGIDrawingTemplate.h"
#include "QGILeaderLine.h"
#include "QGIProjGroup.h"
#include "QGIRichAnno.h"
#include "QGISVGTemplate.h"
#include "QGITemplate.h"
#include "QGIViewAnnotation.h"
#include "QGIViewBalloon.h"
#include "QGIViewClip.h"
#include "QGIViewCollection.h"
#include "QGIViewDimension.h"
#include "QGIViewImage.h"
#include "QGIViewPart.h"
#include "QGIViewSection.h"
#include "QGIViewSpreadsheet.h"
#include "QGIViewSymbol.h"
#include "QGIWeldSymbol.h"
#include "QGSPage.h"
#include "Rez.h"
#include "ViewProviderDrawingView.h"
#include "ViewProviderPage.h"
#include "ZVALUE.h"


// used SVG namespaces
#define CC_NS_URI "http://creativecommons.org/ns#"
#define DC_NS_URI "http://purl.org/dc/elements/1.1/"
#define RDF_NS_URI "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define INKSCAPE_NS_URI "http://www.inkscape.org/namespaces/inkscape"
#define SODIPODI_NS_URI "http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;
using DU = DrawUtil;

QGSPage::QGSPage(ViewProviderPage* vpPage, QWidget* parent)
    : QGraphicsScene(parent), pageTemplate(nullptr), m_vpPage(nullptr)
{
    assert(vpPage);
    m_vpPage = vpPage;
    setItemIndexMethod(QGraphicsScene::BspTreeIndex);//the default
    //    setItemIndexMethod(QGraphicsScene::NoIndex);    //sometimes faster
}

void QGSPage::addChildrenToPage()
{
    //    Base::Console().Message("QGSP::addChildrenToPage()\n");
    // A fresh page is added and we iterate through its collected children and add these to Canvas View  -MLP
    // if docobj is a featureviewcollection (ex orthogroup), add its child views. if there are ever children that have children,
    // we'll have to make this recursive. -WF
    const std::vector<App::DocumentObject*>& grp = m_vpPage->getDrawPage()->Views.getValues();
    std::vector<App::DocumentObject*> childViews;
    for (std::vector<App::DocumentObject*>::const_iterator it = grp.begin(); it != grp.end();
         ++it) {
        attachView(*it);
        TechDraw::DrawViewCollection* collect = dynamic_cast<TechDraw::DrawViewCollection*>(*it);
        if (collect) {
            childViews = collect->Views.getValues();
            for (std::vector<App::DocumentObject*>::iterator itChild = childViews.begin();
                 itChild != childViews.end(); ++itChild) {
                attachView(*itChild);
            }
        }
    }
    //when restoring, it is possible for a Dimension to be loaded before the ViewPart it applies to
    //therefore we need to make sure parentage of the graphics representation is set properly. bit of a kludge.
    setDimensionGroups();
    setBalloonGroups();

    App::DocumentObject* obj = m_vpPage->getDrawPage()->Template.getValue();
    auto pageTemplate(dynamic_cast<TechDraw::DrawTemplate*>(obj));
    if (pageTemplate) {
        attachTemplate(pageTemplate);
        matchSceneRectToTemplate();
    }

    //    viewAll();
}

//********** template related routines *********

void QGSPage::attachTemplate(TechDraw::DrawTemplate* obj)
{
    //    Base::Console().Message("QGSP::attachTemplate()\n");
    setPageTemplate(obj);
}

void QGSPage::updateTemplate(bool forceUpdate)
{
    //    Base::Console().Message("QGSP::updateTemplate()\n");
    App::DocumentObject* templObj = m_vpPage->getDrawPage()->Template.getValue();
    // TODO: what if template has been deleted? templObj will be NULL. segfault?
    if (!templObj) {
        return;
    }

    if (m_vpPage->getDrawPage()->Template.isTouched() || templObj->isTouched()) {
        // Template is touched so update

        if (forceUpdate
            || (templObj && templObj->isTouched()
                && templObj->isDerivedFrom(TechDraw::DrawTemplate::getClassTypeId()))) {

            QGITemplate* qItemTemplate = getTemplate();

            if (qItemTemplate) {
                TechDraw::DrawTemplate* pageTemplate =
                    dynamic_cast<TechDraw::DrawTemplate*>(templObj);
                qItemTemplate->setTemplate(pageTemplate);
                qItemTemplate->updateView();
            }
        }
    }
}

QPointF QGSPage::getTemplateCenter()
{
    App::DocumentObject* obj = m_vpPage->getDrawPage()->Template.getValue();
    auto pageTemplate(dynamic_cast<TechDraw::DrawTemplate*>(obj));
    if (pageTemplate) {
        double cx = Rez::guiX(pageTemplate->Width.getValue()) / 2.0;
        double cy = -Rez::guiX(pageTemplate->Height.getValue()) / 2.0;
        return QPointF(cx, cy);
    }
    return QPointF(0.0, 0.0);
}

void QGSPage::matchSceneRectToTemplate(void)
{
    //    Base::Console().Message("QGSP::matchSceneRectToTemplate()\n");
    App::DocumentObject* obj = m_vpPage->getDrawPage()->Template.getValue();
    auto pageTemplate(dynamic_cast<TechDraw::DrawTemplate*>(obj));
    if (pageTemplate) {
        //make sceneRect 1 pagesize bigger in every direction
        double width = Rez::guiX(pageTemplate->Width.getValue());
        double height = Rez::guiX(pageTemplate->Height.getValue());
        setSceneRect(QRectF(-width, -2.0 * height, 3.0 * width, 3.0 * height));
    }
}

void QGSPage::setPageTemplate(TechDraw::DrawTemplate* templateFeat)
{
    //    Base::Console().Message("QGSP::setPageTemplate()\n");
    removeTemplate();

    if (templateFeat->isDerivedFrom(TechDraw::DrawParametricTemplate::getClassTypeId())) {
        pageTemplate = new QGIDrawingTemplate(this);
    }
    else if (templateFeat->isDerivedFrom(TechDraw::DrawSVGTemplate::getClassTypeId())) {
        pageTemplate = new QGISVGTemplate(this);
    }
    pageTemplate->setTemplate(templateFeat);
    pageTemplate->updateView();
}

QGITemplate* QGSPage::getTemplate() const { return pageTemplate; }

void QGSPage::removeTemplate()
{
    if (pageTemplate) {
        removeItem(pageTemplate);
        pageTemplate->deleteLater();
        pageTemplate = nullptr;
    }
}

//******* QGIView related routines

//! retrieve the QGIView objects currently in the scene
std::vector<QGIView*> QGSPage::getViews() const
{
    std::vector<QGIView*> result;
    QList<QGraphicsItem*> items = this->items();
    for (auto& v : items) {
        QGIView* qv = dynamic_cast<QGIView*>(v);
        if (qv) {
            result.push_back(qv);
        }
    }
    return result;
}

int QGSPage::addQView(QGIView* view)
{
    //don't add twice!
    QGIView* existing = getQGIVByName(view->getViewName());
    if (!existing) {
        addItem(view);

        TechDraw::DrawView *viewObj = view->getViewObject();
        // Preserve the desired position, as addToGroup() adjusts the child view's position
        QPointF viewPos(Rez::guiX(viewObj->X.getValue()), -Rez::guiX(viewObj->Y.getValue()));
        // Find if it belongs to a parent
        QGIView *parent = findParent(view);
        if (parent) {
            parent->addToGroup(view);
        }
        view->setPos(viewPos);

        auto viewProvider = dynamic_cast<ViewProviderDrawingView *>(QGIView::getViewProvider(view->getViewObject()));
        if (viewProvider) {
            view->setZValue(viewProvider->StackOrder.getValue());
        }

        view->updateView(true);
    }
    return 0;
}

int QGSPage::removeQView(QGIView* view)
{
    if (view) {
        removeQViewFromScene(view);
        delete view;
    }
    return 0;
}

int QGSPage::removeQViewByName(const char* name)
{
    std::vector<QGIView*> items = getViews();
    QString qsName = QString::fromUtf8(name);
    bool found = false;
    QGIView* ourItem = nullptr;
    for (auto& i : items) {
        if (qsName == i->data(1).toString()) {//is there a QGIV with this name in scene?
            found = true;
            ourItem = i;
            break;
        }
    }

    if (found) {
        int balloonItemType = QGraphicsItem::UserType + 140;
        if (ourItem->type() == balloonItemType) {
            QGIViewBalloon* balloon = dynamic_cast<QGIViewBalloon*>(ourItem);
            balloon->disconnect();
        }
        removeQViewFromScene(ourItem);
        delete ourItem;//commenting this prevents crash but means a small memory waste.
                       //alternate fix(?) is to change indexing/caching option in scene/view
    }

    return 0;
}

void QGSPage::removeQViewFromScene(QGIView* view)
{
    QGIView* qgParent = dynamic_cast<QGIView*>(view->parentItem());
    if (qgParent) {
        qgParent->removeChild(view);
    }
    else {
        removeItem(view);
    }
}

bool QGSPage::addView(const App::DocumentObject* obj)
{
    return attachView(const_cast<App::DocumentObject*>(obj));
}

bool QGSPage::attachView(App::DocumentObject* obj)
{
    //    Base::Console().Message("QGSP::attachView(%s)\n", obj->getNameInDocument());
    QGIView* existing = findQViewForDocObj(obj);
    if (existing)
        return true;

    auto typeId(obj->getTypeId());

    QGIView* qview(nullptr);

    if (typeId.isDerivedFrom(TechDraw::DrawViewSection::getClassTypeId())) {
        qview = addViewSection(static_cast<TechDraw::DrawViewSection*>(obj));
    }
    else if (typeId.isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        qview = addViewPart(static_cast<TechDraw::DrawViewPart*>(obj));
    }
    else if (typeId.isDerivedFrom(TechDraw::DrawProjGroup::getClassTypeId())) {
        qview = addProjectionGroup(static_cast<TechDraw::DrawProjGroup*>(obj));
    }
    else if (typeId.isDerivedFrom(TechDraw::DrawViewCollection::getClassTypeId())) {
        qview = addDrawView(static_cast<TechDraw::DrawViewCollection*>(obj));
    }
    else if (typeId.isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())) {
        qview = addViewDimension(static_cast<TechDraw::DrawViewDimension*>(obj));
    }
    else if (typeId.isDerivedFrom(TechDraw::DrawViewBalloon::getClassTypeId())) {
        qview = addViewBalloon(static_cast<TechDraw::DrawViewBalloon*>(obj));
    }
    else if (typeId.isDerivedFrom(TechDraw::DrawViewAnnotation::getClassTypeId())) {
        qview = addDrawViewAnnotation(static_cast<TechDraw::DrawViewAnnotation*>(obj));
    }
    else if (typeId.isDerivedFrom(TechDraw::DrawViewSymbol::getClassTypeId())) {
        qview = addDrawViewSymbol(static_cast<TechDraw::DrawViewSymbol*>(obj));
    }
    else if (typeId.isDerivedFrom(TechDraw::DrawViewClip::getClassTypeId())) {
        qview = addDrawViewClip(static_cast<TechDraw::DrawViewClip*>(obj));
    }
    else if (typeId.isDerivedFrom(TechDraw::DrawViewSpreadsheet::getClassTypeId())) {
        qview = addDrawViewSpreadsheet(static_cast<TechDraw::DrawViewSpreadsheet*>(obj));
    }
    else if (typeId.isDerivedFrom(TechDraw::DrawViewImage::getClassTypeId())) {
        qview = addDrawViewImage(static_cast<TechDraw::DrawViewImage*>(obj));
    }
    else if (typeId.isDerivedFrom(TechDraw::DrawLeaderLine::getClassTypeId())) {
        qview = addViewLeader(static_cast<TechDraw::DrawLeaderLine*>(obj));
    }
    else if (typeId.isDerivedFrom(TechDraw::DrawRichAnno::getClassTypeId())) {
        qview = addRichAnno(static_cast<TechDraw::DrawRichAnno*>(obj));
    }
    else if (typeId.isDerivedFrom(TechDraw::DrawWeldSymbol::getClassTypeId())) {
        qview = addWeldSymbol(static_cast<TechDraw::DrawWeldSymbol*>(obj));
    }
    else if (typeId.isDerivedFrom(TechDraw::DrawHatch::getClassTypeId())) {
        //Hatch is not attached like other Views (since it isn't really a View)
        return true;
    }

    return (qview != nullptr);
}

QGIView* QGSPage::addViewPart(TechDraw::DrawViewPart* partFeat)
{
    // Base::Console().Message("QGSP::addViewPart(%s)\n", partFeat->Label.getValue());
    auto viewPart(new QGIViewPart);

    viewPart->setViewPartFeature(partFeat);

    addQView(viewPart);
    return viewPart;
}

QGIView* QGSPage::addViewSection(DrawViewSection* sectionFeat)
{
    auto viewSection(new QGIViewSection);

    viewSection->setViewPartFeature(sectionFeat);

    addQView(viewSection);
    return viewSection;
}

QGIView* QGSPage::addProjectionGroup(TechDraw::DrawProjGroup* projGroupFeat)
{
    // Base::Console().Message("QGSP::addprojectionGroup(%s)\n", projGroupFeat->Label.getValue());
    auto qview(new QGIProjGroup);

    qview->setViewFeature(projGroupFeat);
    addQView(qview);
    return qview;
}

QGIView* QGSPage::addDrawView(TechDraw::DrawView* view)
{
    auto qview(new QGIView);

    qview->setViewFeature(view);
    addQView(qview);
    return qview;
}

QGIView* QGSPage::addDrawViewCollection(TechDraw::DrawViewCollection* collectionFeat)
{
    auto qview(new QGIViewCollection);

    qview->setViewFeature(collectionFeat);
    addQView(qview);
    return qview;
}

QGIView* QGSPage::addDrawViewAnnotation(TechDraw::DrawViewAnnotation* annoFeat)
{
    auto qview(new QGIViewAnnotation);

    qview->setViewAnnoFeature(annoFeat);

    addQView(qview);
    return qview;
}

QGIView* QGSPage::addDrawViewSymbol(TechDraw::DrawViewSymbol* symbolFeat)
{
    QGIViewSymbol *symbolView = new QGIViewSymbol;
    symbolView->setViewFeature(symbolFeat);

    addQView(symbolView);
    return symbolView;
}

QGIView* QGSPage::addDrawViewClip(TechDraw::DrawViewClip* view)
{
    auto qview(new QGIViewClip);

    qview->setPosition(Rez::guiX(view->X.getValue()), Rez::guiX(view->Y.getValue()));
    qview->setViewFeature(view);

    addQView(qview);
    return qview;
}

QGIView* QGSPage::addDrawViewSpreadsheet(TechDraw::DrawViewSpreadsheet* sheetFeat)
{
    auto qview(new QGIViewSpreadsheet);

    qview->setViewFeature(sheetFeat);

    addQView(qview);
    return qview;
}

QGIView* QGSPage::addDrawViewImage(TechDraw::DrawViewImage* imageFeat)
{
    auto qview(new QGIViewImage);

    qview->setViewFeature(imageFeat);

    addQView(qview);
    return qview;
}

QGIView* QGSPage::addViewBalloon(TechDraw::DrawViewBalloon* balloonFeat)
{
    //    Base::Console().Message("QGSP::addViewBalloon(%s)\n", balloonFeat->getNameInDocument());
    auto vBalloon(new QGIViewBalloon);

    addItem(vBalloon);

    vBalloon->setViewPartFeature(balloonFeat);

    QGIView* parent = nullptr;
    parent = findParent(vBalloon);

    if (parent) {
        addBalloonToParent(vBalloon, parent);
    }

    return vBalloon;
}

void QGSPage::addBalloonToParent(QGIViewBalloon* balloon, QGIView* parent)
{
    //    Base::Console().Message("QGSP::addBalloonToParent()\n");
    assert(balloon);
    assert(parent);//blow up if we don't have Dimension or Parent
    QPointF posRef(0., 0.);
    QPointF mapPos = balloon->mapToItem(parent, posRef);
    balloon->moveBy(-mapPos.x(), -mapPos.y());
    parent->addToGroup(balloon);
    balloon->setZValue(ZVALUE::DIMENSION);
}

//origin is in scene coordinates from QGViewPage
void QGSPage::createBalloon(QPointF origin, DrawView* parent)
{
    //    Base::Console().Message("QGSP::createBalloon(%s)\n", DrawUtil::formatVector(origin).c_str());
    std::string featName = getDrawPage()->getDocument()->getUniqueObjectName("Balloon");
    std::string pageName = getDrawPage()->getNameInDocument();

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Balloon"));
    Command::doCommand(Command::Doc,
                       "App.activeDocument().addObject('TechDraw::DrawViewBalloon', '%s')",
                       featName.c_str());
    Command::doCommand(Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewBalloon', 'Balloon', '%s')",
              featName.c_str(), featName.c_str());

    TechDraw::DrawViewBalloon* balloon = dynamic_cast<TechDraw::DrawViewBalloon*>(
        getDrawPage()->getDocument()->getObject(featName.c_str()));
    if (!balloon) {
        throw Base::TypeError("QGSP::createBalloon - balloon not found\n");
    }
    Command::doCommand(Command::Doc,
                       "App.activeDocument().%s.SourceView = (App.activeDocument().%s)",
                       featName.c_str(), parent->getNameInDocument());
    QGIView* qgParent = getQGIVByName(parent->getNameInDocument());
    //convert from scene coords to qgParent coords and unscale
    QPointF parentOrigin = qgParent->mapFromScene(origin) / parent->getScale();
    balloon->setOrigin(parentOrigin);
    //convert origin to App side coords
    QPointF appOrigin = Rez::appPt(parentOrigin);
    appOrigin = DrawUtil::invertY(appOrigin);
    balloon->OriginX.setValue(appOrigin.x());
    balloon->OriginY.setValue(appOrigin.y());
    double textOffset = 20.0 / parent->getScale();
    balloon->X.setValue(appOrigin.x() + textOffset);
    balloon->Y.setValue(appOrigin.y() + textOffset);

    int idx = getDrawPage()->getNextBalloonIndex();
    balloon->Text.setValue(std::to_string(idx).c_str());

    Command::doCommand(Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)",
                       pageName.c_str(), featName.c_str());

    Gui::Command::commitCommand();

    // Touch the parent feature so the balloon in tree view appears as a child
    parent->touch(true);
}

QGIView* QGSPage::addViewDimension(TechDraw::DrawViewDimension* dimFeat)
{
    auto dimGroup(new QGIViewDimension);

    addItem(dimGroup);

    dimGroup->setViewPartFeature(dimFeat);

    // Find if it belongs to a parent
    QGIView* parent = nullptr;
    parent = findParent(dimGroup);

    if (parent) {
        addDimToParent(dimGroup, parent);
    }

    return dimGroup;
}

void QGSPage::addDimToParent(QGIViewDimension* dim, QGIView* parent)
{
    //    Base::Console().Message("QGSP::addDimToParent()\n");
    assert(dim);
    assert(parent);//blow up if we don't have Dimension or Parent
    QPointF posRef(0., 0.);
    QPointF mapPos = dim->mapToItem(parent, posRef);
    dim->moveBy(-mapPos.x(), -mapPos.y());
    parent->addToGroup(dim);
    dim->setZValue(ZVALUE::DIMENSION);
}

QGIView* QGSPage::addViewLeader(TechDraw::DrawLeaderLine* leaderFeat)
{
    QGILeaderLine *leaderView = new QGILeaderLine;
    leaderView->setViewFeature(leaderFeat);

    addQView(leaderView);
    return leaderView;
}

QGIView* QGSPage::addRichAnno(TechDraw::DrawRichAnno* richFeat)
{
    QGIRichAnno *richView = new QGIRichAnno;
    richView->setViewFeature(richFeat);

    addQView(richView);
    return richView;
}

QGIView* QGSPage::addWeldSymbol(TechDraw::DrawWeldSymbol* weldFeat)
{
    QGIWeldSymbol *weldView = new QGIWeldSymbol;
    weldView->setViewFeature(weldFeat);

    addQView(weldView);
    return weldView;
}

void QGSPage::setDimensionGroups(void)
{
    const std::vector<QGIView*>& allItems = getViews();
    int dimItemType = QGraphicsItem::UserType + 106;

    for (auto& item : allItems) {
        if (item->type() == dimItemType && !item->group()) {
            QGIView* parent = findParent(item);
            if (parent) {
                QGIViewDimension* dim = dynamic_cast<QGIViewDimension*>(item);
                addDimToParent(dim, parent);
            }
        }
    }
}

void QGSPage::setBalloonGroups(void)
{
    const std::vector<QGIView*>& allItems = getViews();
    int balloonItemType = QGraphicsItem::UserType + 140;

    for (auto& item : allItems) {
        if (item->type() == balloonItemType && !item->group()) {
            QGIView* parent = findParent(item);
            if (parent) {
                QGIViewBalloon* balloon = dynamic_cast<QGIViewBalloon*>(item);
                addBalloonToParent(balloon, parent);
            }
        }
    }
}

//! find the graphic for a DocumentObject
QGIView* QGSPage::findQViewForDocObj(App::DocumentObject* obj) const
{
    //    Base::Console().Message("QGSP::findQViewForDocObj(%s)\n", obj->getNameInDocument());
    if (obj) {
        const std::vector<QGIView*> qviews = getViews();
        for (std::vector<QGIView*>::const_iterator it = qviews.begin(); it != qviews.end(); ++it) {
            if (strcmp(obj->getNameInDocument(), (*it)->getViewName()) == 0)
                return *it;
        }
    }
    return nullptr;
}

//! find the graphic for DocumentObject with name
QGIView* QGSPage::getQGIVByName(std::string name) const
{
    QList<QGraphicsItem*> qgItems = items();
    QList<QGraphicsItem*>::iterator it = qgItems.begin();
    for (; it != qgItems.end(); it++) {
        QGIView* qv = dynamic_cast<QGIView*>((*it));
        if (qv) {
            const char* qvName = qv->getViewName();
            if (name.compare(qvName) == 0) {
                return (qv);
            }
        }
    }
    return nullptr;
}

//find the parent of a QGIV based on the corresponding feature's parentage
QGIView* QGSPage::findParent(QGIView* view) const
{
    //    Base::Console().Message("QGSP::findParent(%s)\n", view->getViewName());
    const std::vector<QGIView*> qviews = getViews();
    TechDraw::DrawView* myFeat = view->getViewObject();

    TechDraw::DrawView *ownerFeat = myFeat->claimParent();
    if (ownerFeat) {
        QGIView *ownerView = getQGIVByName(ownerFeat->getNameInDocument());
        if (ownerView) {
            return ownerView;
        }
    }

    //If type is dimension we check references first
    TechDraw::DrawViewDimension* dim = nullptr;
    dim = dynamic_cast<TechDraw::DrawViewDimension*>(myFeat);
    if (dim) {
        std::vector<App::DocumentObject*> objs = dim->References2D.getValues();

        if (!objs.empty()) {
            std::vector<App::DocumentObject*> objs = dim->References2D.getValues();
            // Attach the dimension to the first object's group
            for (std::vector<QGIView*>::const_iterator it = qviews.begin(); it != qviews.end();
                 ++it) {
                if (strcmp((*it)->getViewName(), objs.at(0)->getNameInDocument()) == 0) {
                    return *it;
                }
            }
        }
    }

    //If type is balloon we check references first
    TechDraw::DrawViewBalloon* balloon = nullptr;
    balloon = dynamic_cast<TechDraw::DrawViewBalloon*>(myFeat);

    if (balloon) {
        App::DocumentObject* obj = balloon->SourceView.getValue();

        if (obj) {
            // Attach the Balloon to the first object's group
            for (std::vector<QGIView*>::const_iterator it = qviews.begin(); it != qviews.end();
                 ++it) {
                if (strcmp((*it)->getViewName(), obj->getNameInDocument()) == 0) {
                    return *it;
                }
            }
        }
    }

    // Not found a parent
    return nullptr;
}

bool QGSPage::hasQView(App::DocumentObject* obj)
{
    const std::vector<QGIView*>& views = getViews();
    std::vector<QGIView*>::const_iterator qview = views.begin();

    while (qview != views.end()) {
        // Unsure if we can compare pointers so rely on name
        if (strcmp((*qview)->getViewName(), obj->getNameInDocument()) == 0) {
            return true;
        }
        qview++;
    }

    return false;
}

void QGSPage::refreshViews()
{
    //    Base::Console().Message("QGSP::refreshViews()\n");
    QList<QGraphicsItem*> list = items();
    QList<QGraphicsItem*> qgiv;
    //find only QGIV's
    for (auto q : list) {
        QString viewFamily = QString::fromUtf8("QGIV");
        if (viewFamily == q->data(0).toString()) {
            qgiv.push_back(q);
        }
    }
    for (auto q : qgiv) {
        QGIView* itemView = dynamic_cast<QGIView*>(q);
        if (itemView) {
            itemView->updateView(true);
        }
    }
}

void QGSPage::findMissingViews(const std::vector<App::DocumentObject*>& list,
                               std::vector<App::DocumentObject*>& missing)
{
    for (std::vector<App::DocumentObject*>::const_iterator it = list.begin(); it != list.end();
         ++it) {

        if (!hasQView(*it))
            missing.push_back(*it);

        if ((*it)->isDerivedFrom<TechDraw::DrawViewCollection>()) {
            std::vector<App::DocumentObject*> missingChildViews;
            TechDraw::DrawViewCollection* collection =
                dynamic_cast<TechDraw::DrawViewCollection*>(*it);
            // Find Child Views recursively
            findMissingViews(collection->Views.getValues(), missingChildViews);

            // Append the views to current missing list
            for (std::vector<App::DocumentObject*>::const_iterator it = missingChildViews.begin();
                 it != missingChildViews.end(); ++it) {
                missing.push_back(*it);
            }
        }
    }
}

//this is time consuming. should only be used when there is a problem.
//Solve the situation where a DrawView belonging to this DrawPage has no QGraphicsItem in
//the QGScene for the DrawPage -or-
//a QGraphics item exists in the DrawPage's QGScene, but there is no corresponding DrawView
//in the DrawPage.
void QGSPage::fixOrphans(bool force)
{
    Q_UNUSED(force)

    // get all the DrawViews for this page, including the second level ones
    // if we ever have collections of collections, we'll need to revisit this
    TechDraw::DrawPage* thisPage = m_vpPage->getDrawPage();

    if (!thisPage->isAttachedToDocument())
        return;

    std::vector<App::DocumentObject*> pChildren = thisPage->getAllViews();

    // if dv doesn't have a graphic, make one
    for (auto& dv : pChildren) {
        if (dv->isRemoving())
            continue;
        QGIView* qv = findQViewForDocObj(dv);
        if (!qv)
            attachView(dv);
    }
    // if qView doesn't have a Feature on this Page, delete it
    std::vector<QGIView*> qvss = getViews();
    // qvss may contain an item and its child item(s) and to avoid to access a deleted item a QPointer is needed
    std::vector<QPointer<QGIView>> qvs;
    std::for_each(qvss.begin(), qvss.end(), [&qvs](QGIView* v) { qvs.emplace_back(v); });
    App::Document* doc = m_vpPage->getDrawPage()->getDocument();
    for (auto& qv : qvs) {
        if (!qv)
            continue;// already deleted?

        App::DocumentObject* obj = doc->getObject(qv->getViewName());
        if (!obj) {
            //no DrawView anywhere in Document
            removeQView(qv);
        }
        else {
            //DrawView exists in Document.  Does it belong to this DrawPage?
            int numParentPages = qv->getViewObject()->countParentPages();
            if (numParentPages == 0) {
                //DrawView does not belong to any DrawPage
                //remove QGItem from QGScene
                removeQView(qv);
            }
            else if (numParentPages == 1) {
                //Does DrawView belong to this DrawPage?
                TechDraw::DrawPage* parentPage = qv->getViewObject()->findParentPage();
                if (thisPage != parentPage) {
                    //DrawView does not belong to this DrawPage
                    //remove QGItem from QGScene
                    removeQView(qv);
                }
            }
            else if (numParentPages > 1) {
                //DrawView belongs to multiple DrawPages
                //check if this MDIViewPage corresponds to any parent DrawPage
                //if not, delete the QGItem
                std::vector<TechDraw::DrawPage*> potentialParentPages =
                    qv->getViewObject()->findAllParentPages();
                bool found = false;
                for (auto p : potentialParentPages) {
                    if (thisPage == p) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    //none of the parent Pages for View correspond to this Page
                    removeQView(qv);
                }
            }
        }
    }
}

bool QGSPage::orphanExists(const char* viewName, const std::vector<App::DocumentObject*>& list)
{
    for (std::vector<App::DocumentObject*>::const_iterator it = list.begin(); it != list.end();
         ++it) {

        //Check child objects too recursively
        if ((*it)->isDerivedFrom(TechDraw::DrawViewCollection::getClassTypeId())) {
            TechDraw::DrawViewCollection* collection =
                dynamic_cast<TechDraw::DrawViewCollection*>(*it);
            if (orphanExists(viewName, collection->Views.getValues()))
                return true;
        }

        // Unsure if we can compare pointers so rely on name
        if (strcmp(viewName, (*it)->getNameInDocument()) == 0) {
            return true;
        }
    }
    return false;
}

//NOTE: this doesn't add missing views.  see fixOrphans()
void QGSPage::redrawAllViews()
{
    //    Base::Console().Message("QGSP::redrawAllViews() - views: %d\n", getViews().size());
    const std::vector<QGIView*>& upviews = getViews();
    for (std::vector<QGIView*>::const_iterator it = upviews.begin(); it != upviews.end(); ++it) {
        (*it)->updateView(true);
    }
}

//NOTE: this doesn't add missing views.   see fixOrphans()
void QGSPage::redraw1View(TechDraw::DrawView* dView)
{
    std::string dvName = dView->getNameInDocument();
    const std::vector<QGIView*>& upviews = getViews();
    for (std::vector<QGIView*>::const_iterator it = upviews.begin(); it != upviews.end(); ++it) {
        std::string qgivName = (*it)->getViewName();
        if (dvName == qgivName) {
            (*it)->updateView(true);
        }
    }
}

// RichTextAnno needs to know when it is rendering an Svg as the font size
// is handled differently in Svg compared to the screen or Pdf.
void QGSPage::setExportingSvg(bool enable)
{
    QList<QGraphicsItem*> sceneItems = items();
    for (auto& qgi : sceneItems) {
        QGIRichAnno* qgiRTA = dynamic_cast<QGIRichAnno*>(qgi);
        if (qgiRTA) {
            qgiRTA->setExportingSvg(enable);
        }
    }
}

void QGSPage::saveSvg(QString filename)
{
    // TODO: We only have m_vpPage because constructor gets passed a view provider...
    //NOTE: this makes wrong size pages in low-Rez
    TechDraw::DrawPage* page(m_vpPage->getDrawPage());

    const QString docName(QString::fromUtf8(page->getDocument()->getName()));
    const QString pageName(QString::fromUtf8(page->getNameInDocument()));
    QString svgDescription = QString::fromUtf8("Drawing page: ") + pageName
        + QString::fromUtf8(" exported from FreeCAD document: ") + docName;

    QSvgGenerator svgGen;
    QTemporaryFile temporaryFile;
    svgGen.setOutputDevice(&temporaryFile);

    // Set resolution in DPI. Use the actual one, i.e. Rez::guiX(inch)
    svgGen.setResolution(Rez::guiX(25.4));

    // Set size in pixels, which Qt recomputes using DPI to mm.
    int pixelWidth = Rez::guiX(page->getPageWidth());
    int pixelHeight = Rez::guiX(page->getPageHeight());
    svgGen.setSize(QSize(pixelWidth, pixelHeight));

    //"By default this property is set to QSize(-1, -1), which indicates that the generator should not output
    // the width and height attributes of the <svg> element."  >> but Inkscape won't read it without size info??
    svgGen.setViewBox(QRect(0, 0, pixelWidth, pixelHeight));

    svgGen.setTitle(QString::fromUtf8("FreeCAD SVG Export"));
    svgGen.setDescription(svgDescription);

    Gui::Selection().clearSelection();

    bool saveState = m_vpPage->getFrameState();
    m_vpPage->setFrameState(false);
    m_vpPage->setTemplateMarkers(false);
    setExportingSvg(true);

    // Here we temporarily hide the page template, because Qt would otherwise convert the SVG template
    // texts into series of paths, making the later document edits practically unfeasible.
    // We will insert the SVG template ourselves in the final XML postprocessing operation.
    QGISVGTemplate* svgTemplate = dynamic_cast<QGISVGTemplate*>(pageTemplate);
    bool templateVisible = false;
    if (svgTemplate) {
        templateVisible = svgTemplate->isVisible();
        svgTemplate->hide();
    }

    refreshViews();

    double width = Rez::guiX(page->getPageWidth());
    double height = Rez::guiX(page->getPageHeight());
    QRectF sourceRect(0.0, -height, width, height);
    QRectF targetRect(0.0, 0.0, width, height);

    Gui::Selection().clearSelection();
    QPainter p;

    p.begin(&svgGen);
    render(&p, targetRect, sourceRect);//note: scene render, not item render!
    p.end();

    m_vpPage->setFrameState(saveState);
    m_vpPage->setTemplateMarkers(saveState);
    setExportingSvg(false);
    if (templateVisible && svgTemplate) {
        svgTemplate->show();
    }

    refreshViews();

    temporaryFile.close();
    postProcessXml(temporaryFile, filename, pageName);
}

static void removeEmptyGroups(QDomElement e)
{
    while (!e.isNull()) {
        QDomElement next = e.nextSiblingElement();
        if (e.hasChildNodes()) {
            removeEmptyGroups(e.firstChildElement());
        }
        else if (e.tagName() == QLatin1String("g")) {
            e.parentNode().removeChild(e);
        }
        e = next;
    }
}

void QGSPage::postProcessXml(QTemporaryFile& temporaryFile, QString fileName, QString pageName)
{
    QDomDocument exportDoc(QString::fromUtf8("SvgDoc"));
    QFile file(temporaryFile.fileName());
    if (!file.open(QIODevice::ReadOnly)) {
        Base::Console().Error("QGSPage::ppsvg - tempfile open error\n");
        return;
    }
    if (!exportDoc.setContent(&file)) {
        Base::Console().Error("QGSPage::ppsvg - xml error\n");
        file.close();
        return;
    }
    file.close();

    QDomElement exportDocElem = exportDoc.documentElement();//root <svg>

    // Insert Freecad SVG namespace into namespace declarations
    exportDocElem.setAttribute(QString::fromUtf8("xmlns:freecad"),
                               QString::fromUtf8(FREECAD_SVG_NS_URI));
    // Insert all namespaces used by TechDraw's page template SVGs
    exportDocElem.setAttribute(QString::fromUtf8("xmlns:svg"), QString::fromUtf8(SVG_NS_URI));
    exportDocElem.setAttribute(QString::fromUtf8("xmlns:cc"), QString::fromUtf8(CC_NS_URI));
    exportDocElem.setAttribute(QString::fromUtf8("xmlns:dc"), QString::fromUtf8(DC_NS_URI));
    exportDocElem.setAttribute(QString::fromUtf8("xmlns:rdf"), QString::fromUtf8(RDF_NS_URI));
    exportDocElem.setAttribute(QString::fromUtf8("xmlns:inkscape"),
                               QString::fromUtf8(INKSCAPE_NS_URI));
    exportDocElem.setAttribute(QString::fromUtf8("xmlns:sodipodi"),
                               QString::fromUtf8(SODIPODI_NS_URI));

    // Create the root group which will host the drawing group and the template group
    QDomElement rootGroup = exportDoc.createElement(QString::fromUtf8("g"));
    rootGroup.setAttribute(QString::fromUtf8("id"), pageName);
    rootGroup.setAttribute(QString::fromUtf8("inkscape:groupmode"), QString::fromUtf8("layer"));
    rootGroup.setAttribute(QString::fromUtf8("inkscape:label"), QString::fromUtf8("TechDraw"));

    // Now insert our template
    QGISVGTemplate* svgTemplate = dynamic_cast<QGISVGTemplate*>(pageTemplate);
    if (svgTemplate) {
        DrawSVGTemplate* drawTemplate = svgTemplate->getSVGTemplate();
        if (drawTemplate) {
            QString templateSvg = drawTemplate->processTemplate();
            QDomDocument templateResultDoc(QString::fromUtf8("SvgDoc"));
            if (templateResultDoc.setContent(templateSvg)) {
                QDomElement templateDocElem = templateResultDoc.documentElement();

                // Insert the template into a new group with id set to template name
                QDomElement templateGroup = exportDoc.createElement(QString::fromUtf8("g"));
                Base::FileInfo fi(drawTemplate->PageResult.getValue());
                templateGroup.setAttribute(QString::fromUtf8("id"),
                                           QString::fromUtf8(fi.fileName().c_str()));
                templateGroup.setAttribute(QString::fromUtf8("style"),
                                           QString::fromUtf8("stroke: none;"));

                // Scale the template group correctly
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
                templateGroup.setAttribute(
                    QString::fromUtf8("transform"),
                    QString().sprintf("scale(%f, %f)", Rez::guiX(1.0), Rez::guiX(1.0)));
#else
                templateGroup.setAttribute(QString::fromUtf8("transform"),
                                           QString::fromLatin1("scale(%1, %2)")
                                               .arg(Rez::guiX(1.0), 0, 'f')
                                               .arg(Rez::guiX(1.0), 0, 'f'));
#endif

                // Finally, transfer all template document child nodes under the template group
                while (!templateDocElem.firstChild().isNull()) {
                    templateGroup.appendChild(templateDocElem.firstChild());
                }

                rootGroup.appendChild(templateGroup);
            }
        }
    }

    // Obtain the drawing group element, move it under root node and set its id to "DrawingContent"
    QDomElement drawingGroup = exportDocElem.firstChildElement(QLatin1String("g"));
    if (!drawingGroup.isNull()) {
        drawingGroup.setAttribute(QString::fromUtf8("id"), QString::fromUtf8("DrawingContent"));
        rootGroup.appendChild(drawingGroup);
    }
    exportDocElem.appendChild(rootGroup);

    // As icing on the cake, get rid of the empty <g>'s Qt SVG generator painting inserts.
    removeEmptyGroups(exportDocElem);

    // Time to save our product
    QFile outFile(fileName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Base::Console().Error("QGSP::ppxml - failed to open file for writing: %s\n",
                              qPrintable(fileName));
    }

    QTextStream stream(&outFile);
    stream.setGenerateByteOrderMark(false);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    stream.setCodec("UTF-8");
#endif

    stream << exportDoc.toByteArray();
    outFile.close();
}

TechDraw::DrawPage* QGSPage::getDrawPage() { return m_vpPage->getDrawPage(); }

QColor QGSPage::getBackgroundColor()
{
    App::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Colors")->GetUnsigned("Background", 0x70707000));
    return fcColor.asValue<QColor>();
}


#include <Mod/TechDraw/Gui/moc_QGSPage.cpp>
