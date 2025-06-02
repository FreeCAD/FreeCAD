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
#include <QApplication>
#include <QDomDocument>
#include <QFile>
#include <QGraphicsSceneEvent>
#include <QPainter>
#include <QSvgGenerator>
#include <QTemporaryFile>
#include <QTextStream>
#endif

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>

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
#include "QGIUserTypes.h"
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
#include "PreferencesGui.h"


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
}


void QGSPage::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    Qt::KeyboardModifiers originalModifiers = event->modifiers();
    auto itemUnderMouse = itemAt(event->scenePos().x(), event->scenePos().y(), QTransform());

    if (!itemUnderMouse ||
        itemClearsSelection(itemUnderMouse->type()) ) {
        Gui::Selection().clearSelection();
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton &&
        PreferencesGui::multiSelection() &&
        (cleanModifierList(QApplication::keyboardModifiers()) == Preferences::multiselectModifiers()) ) {
        event->setModifiers(originalModifiers | Preferences::multiselectModifiers());
    }

    QGraphicsScene::mousePressEvent(event);
}


//! returns true if clicking on the item should clear the selection
bool QGSPage::itemClearsSelection(int itemTypeIn)
{
    // type 13 is the itemUnderMouse on a page outside of any views. It is not
    // the template or background or foreground.  QGraphicsItem type = 13 is not
    // documented and not found in QGraphicsItem.h.
    const std::vector<int> ClearingTypes {
        13,  // MysteryType
        UserType::QGITemplate,
        UserType::QGIDrawingTemplate,
        UserType::QGISVGTemplate
    };

    for (auto& type : ClearingTypes) {
        if (itemTypeIn == type) {
            return true;
        }
    }
    return false;
}


//! return only the modifiers that are relevant to snapping/balloon drag.
//! this is a substitute for !modifiers (ie no modifiers in use) since keypad or group modifiers
//! (which don't apply to snapping/dragging) would give a misleading result.
Qt::KeyboardModifiers QGSPage::cleanModifierList(Qt::KeyboardModifiers mods)
{
    if (!mods) {
        return mods;
    }

    // remove misleading modifiers if present
    auto newMods = mods;
    if (newMods & Qt::KeypadModifier) {
        newMods = newMods & ~Qt::KeypadModifier;
    }
    if (newMods & Qt::GroupSwitchModifier) {
        newMods = newMods & ~Qt::GroupSwitchModifier;
    }

    return newMods;
}


void QGSPage::addChildrenToPage()
{
    // A fresh page is added and we iterate through its collected children and add these to Canvas View  -MLP
    // if docobj is a featureviewcollection (ex orthogroup), add its child views. if there are ever children that have children,
    // we'll have to make this recursive. -WF
    for (auto* view : m_vpPage->getDrawPage()->getViews()) {
        attachView(view);
        auto* collect = dynamic_cast<TechDraw::DrawViewCollection*>(view);
        if (collect) {
            for (auto* childView : collect->getViews()) {
                attachView(childView);
            }
        }
    }
    // when restoring, it is possible for an item (ex a Dimension) to be loaded before the ViewPart
    // it applies to therefore we need to make sure parentage of the graphics representation is set
    // properly. bit of a kludge.
    //
    setViewParents();

    App::DocumentObject* obj = m_vpPage->getDrawPage()->Template.getValue();
    auto pageTemplate(dynamic_cast<TechDraw::DrawTemplate*>(obj));
    if (pageTemplate) {
        attachTemplate(pageTemplate);
        matchSceneRectToTemplate();
    }

}

//********** template related routines *********

void QGSPage::attachTemplate(TechDraw::DrawTemplate* obj)
{
    //    Base::Console().message("QGSP::attachTemplate()\n");
    setPageTemplate(obj);
}

void QGSPage::updateTemplate(bool forceUpdate)
{
    App::DocumentObject* templObj = m_vpPage->getDrawPage()->Template.getValue();
    // TODO: what if template has been deleted? templObj will be NULL. segfault?
    if (!templObj) {
        return;
    }

    if (m_vpPage->getDrawPage()->Template.isTouched() || templObj->isTouched()) {
        // Template is touched so update

        if (forceUpdate
            || (templObj && templObj->isTouched()
                && templObj->isDerivedFrom<TechDraw::DrawTemplate>())) {

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

void QGSPage::matchSceneRectToTemplate()
{
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
    removeTemplate();

    if (templateFeat->isDerivedFrom<TechDraw::DrawParametricTemplate>()) {
        pageTemplate = new QGIDrawingTemplate(this);
    }
    else if (templateFeat->isDerivedFrom<TechDraw::DrawSVGTemplate>()) {
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
    QGIView* existing = getQGIVByName(view->getViewName());
    if (!existing) { //don't add twice!
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

        auto viewProvider = dynamic_cast<ViewProviderDrawingView *>(QGIView::getViewProvider(viewObj));
        if (viewProvider) {
            view->setZValue(viewProvider->StackOrder.getValue());
        }

        view->updateView(true);
    } else {
        Base::Console().message("QGSP::addQView - qview already exists\n");
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
        if (ourItem->type() == UserType::QGIViewBalloon) {
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
    if (findQViewForDocObj(obj)) {
        return true;
    }

    QGIView* qview(nullptr);

    if (auto o = freecad_cast<TechDraw::DrawViewSection*>(obj)) {
        qview = addViewSection(o);
    }
    else if (auto o = freecad_cast<TechDraw::DrawViewPart*>(obj)) {
        qview = addViewPart(o);
    }
    else if (auto o = freecad_cast<TechDraw::DrawProjGroup*>(obj)) {
        qview = addProjectionGroup(o);
    }
    else if (auto o = freecad_cast<TechDraw::DrawViewCollection*>(obj)) {
        qview = addDrawView(o);
    }
    else if (auto o = freecad_cast<TechDraw::DrawViewDimension*>(obj)) {
        qview = addViewDimension(o);
    }
    else if (auto o = freecad_cast<TechDraw::DrawViewBalloon*>(obj)) {
        qview = addViewBalloon(o);
    }
    else if (auto o = freecad_cast<TechDraw::DrawViewAnnotation*>(obj)) {
        qview = addAnnotation(o);
    }
    else if (auto o = freecad_cast<TechDraw::DrawViewSpreadsheet*>(obj)) {
        // has to be before DrawViewSymbol since it's a subclass of it.
        qview = addDrawViewSpreadsheet(o);
    }
    else if (auto o = freecad_cast<TechDraw::DrawViewSymbol*>(obj)) {
        qview = addDrawViewSymbol(o);
    }
    else if (auto o = freecad_cast<TechDraw::DrawViewClip*>(obj)) {
        qview = addDrawViewClip(o);
    }
    else if (auto o = freecad_cast<TechDraw::DrawViewImage*>(obj)) {
        qview = addDrawViewImage(o);
    }
    else if (auto o = freecad_cast<TechDraw::DrawLeaderLine*>(obj)) {
        qview = addViewLeader(o);
    }
    else if (auto o = freecad_cast<TechDraw::DrawRichAnno*>(obj)) {
        qview = addRichAnno(o);
    }
    else if (auto o = freecad_cast<TechDraw::DrawWeldSymbol*>(obj)) {
        qview = addWeldSymbol(o);
    }
    else if (freecad_cast<TechDraw::DrawHatch*>(obj)) {
        //Hatch is not attached like other Views (since it isn't really a View)
        return true;
    }

    return (qview != nullptr);
}


void QGSPage::addItemToScene(QGIView* item)
{
    addItem(item);

    // Does item belong to a parent?
    QGIView* parent = nullptr;
    parent = findParent(item);

    if (parent) {
        addItemToParent(item, parent);
    }
}

//! adds item to parent's group with position adjustments if required.
void QGSPage::addItemToParent(QGIView* item, QGIView* parent)
{
    // not every view uses the same remapping?? spreadsheets, image, RTA, anno?
    // anything that was originally designed to have its position
    // defined relative to the Page should not use the dimension/balloon mapping.
    assert(item);
    assert(parent);

    if (item->type() == UserType::QGIWeldSymbol) {
        // don't touch these
        return;
    }

    // original parenting logic here
    QPointF posRef(0., 0.);
    if (item->type() == UserType::QGIViewDimension ||
        item->type() == UserType::QGIViewBalloon) {
        QPointF mapPos = item->mapToItem(parent, posRef);
        item->moveBy(-mapPos.x(), -mapPos.y());
        parent->addToGroup(item);
        return;
    }

    // positioning logic for objects (leader/rta/etc) that normally draw relative to the page goes
    // here
    //
    QPointF itemPosition {item->getViewObject()->X.getValue(),  // millimetres on page
                          -item->getViewObject()->Y.getValue()};
    parent->addToGroup(item);
    item->setPos(Rez::guiX(itemPosition));

    item->setZValue(ZVALUE::DIMENSION);
}

QGIView* QGSPage::addViewPart(TechDraw::DrawViewPart* partFeat)
{
    auto viewPart(new QGIViewPart);
    addItem(viewPart);
    viewPart->setViewPartFeature(partFeat);
    // we need to install an event filter for any views derived from DrawViewPart
    viewPart->installSceneEventFilter(viewPart);

    return viewPart;
}

QGIView* QGSPage::addViewSection(DrawViewSection* sectionFeat)
{
    auto viewSection(new QGIViewSection);
    addItem(viewSection);
    viewSection->setViewPartFeature(sectionFeat);
    viewSection->installSceneEventFilter(viewSection);

    return viewSection;
}

QGIView* QGSPage::addProjectionGroup(TechDraw::DrawProjGroup* projGroupFeat)
{
    auto qview(new QGIProjGroup);
    addItem(qview);
    qview->setViewFeature(projGroupFeat);
    qview->installSceneEventFilter(qview);

    return qview;
}

QGIView* QGSPage::addDrawView(TechDraw::DrawView* view)
{
    auto qview(new QGIView);
    addItem(qview);
    qview->setViewFeature(view);
    return qview;
}

QGIView* QGSPage::addDrawViewCollection(TechDraw::DrawViewCollection* collectionFeat)
{
    auto qview(new QGIViewCollection);
    addItem(qview);
    qview->setViewFeature(collectionFeat);

    return qview;
}

QGIView* QGSPage::addAnnotation(TechDraw::DrawViewAnnotation* annoFeat)
{
    auto annoView{new QGIViewAnnotation};
    annoView->setViewFeature(annoFeat);
    annoView->setZValue(ZVALUE::ANNOTATION);
    addItemToScene(annoView);

    return annoView;
}

QGIView* QGSPage::addDrawViewSymbol(TechDraw::DrawViewSymbol* symbolFeat)
{
    QGIViewSymbol *symbolView = new QGIViewSymbol;
    symbolView->setViewFeature(symbolFeat);
    addItemToScene(symbolView);

    return symbolView;
}

QGIView* QGSPage::addDrawViewClip(TechDraw::DrawViewClip* view)
{
    auto qview(new QGIViewClip);
    qview->setViewFeature(view);
    addItemToScene(qview);
    qview->setPosition(Rez::guiX(view->X.getValue()), Rez::guiX(view->Y.getValue()));

    return qview;
}

QGIView* QGSPage::addDrawViewSpreadsheet(TechDraw::DrawViewSpreadsheet* sheetFeat)
{
    auto qview(new QGIViewSpreadsheet);
    qview->setViewFeature(sheetFeat);
    addItemToScene(qview);

    return qview;
}

QGIView* QGSPage::addDrawViewImage(TechDraw::DrawViewImage* imageFeat)
{
    auto qview(new QGIViewImage);
    qview->setViewFeature(imageFeat);
    addItemToScene(qview);

    return qview;
}

QGIView* QGSPage::addViewBalloon(TechDraw::DrawViewBalloon* balloonFeat)
{
    auto vBalloon(new QGIViewBalloon);
    vBalloon->setViewPartFeature(balloonFeat);
    addItemToScene(vBalloon);

    return vBalloon;
}

void QGSPage::addBalloonToParent(QGIViewBalloon* balloon, QGIView* parent)
{
    assert(balloon);
    assert(parent);//blow up if we don't have Dimension or Parent
    QPointF posRef(0., 0.);
    QPointF mapPos = balloon->mapToItem(parent, posRef);
    balloon->moveBy(-mapPos.x(), -mapPos.y());
    parent->addToGroup(balloon);
    balloon->setZValue(ZVALUE::DIMENSION);
}

// origin is in scene coordinates from QGVPage widget
void QGSPage::createBalloon(QPointF origin, DrawView* parent)
{
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

    // convert from scene coords to parent DrawView's coords, unscaled, unrotated
    QGIView* qgParent = getQGIVByName(parent->getNameInDocument());
    auto parentOrigin = DU::toVector3d(qgParent->mapFromScene(origin));     // from scene to view
    parentOrigin = Rez::appX(parentOrigin) / parent->getScale();            // unrez & unscale
    parentOrigin = DrawUtil::invertY(parentOrigin);                         // +Y up
    auto parentRotationDeg = parent->Rotation.getValue();
    parentOrigin.RotateZ(Base::toRadians(-parentRotationDeg));               // unrotated

    balloon->setOrigin(parentOrigin);

    double textOffset = 20.0 / parent->getScale();
    balloon->setPosition(parentOrigin.x + textOffset,
                         parentOrigin.y + textOffset);

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
    dimGroup->setViewPartFeature(dimFeat);
    addItemToScene(dimGroup);

    return dimGroup;
}

void QGSPage::addDimToParent(QGIViewDimension* dim, QGIView* parent)
{
    //    Base::Console().message("QGSP::addDimToParent()\n");
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
    QGILeaderLine *leaderItem = new QGILeaderLine;
    leaderItem->setViewFeature(leaderFeat);
    addItemToScene(leaderItem);

    return leaderItem;
}

// TODO: can this be generalized?  addViewToParent(childItem, parentItem, positionInParent)?
void QGSPage::addLeaderToParent(QGILeaderLine* leader, QGIView* parent)
{
    assert(leader);
    assert(parent);//blow up if we don't have Dimension or Parent
    QPointF posRef(0., 0.);
    QPointF parentOrigin = leader->mapToItem(parent, posRef);
    QPointF leaderPositionInParent{ leader->getViewObject()->X.getValue(),
                                    leader->getViewObject()->Y.getValue()};
    QPointF moveToPosition = parentOrigin + leaderPositionInParent;
    leader->moveBy(-moveToPosition.x(), -moveToPosition.y());
    parent->addToGroup(leader);
    leader->setZValue(ZVALUE::DIMENSION);
}

QGIView* QGSPage::addRichAnno(TechDraw::DrawRichAnno* richFeat)
{
    auto richView = new QGIRichAnno;
    richView->setViewFeature(richFeat);
    addItemToScene(richView);

    // Find if it belongs to a parent
    QGIView* parent = nullptr;
    parent = findParent(richView);

    if (parent) {
        addRichAnnoToParent(richView, parent);
    }

    return richView;
}


void QGSPage::addRichAnnoToParent(QGIRichAnno* anno, QGIView* parent)
{
    assert(anno);
    assert(parent);//blow up if we don't have Anno or Parent
    QPointF posRef(0., 0.);
    QPointF parentOrigin = anno->mapToItem(parent, posRef);
    // this is not right for a DPGI?  Needs the usual calculation??
    QPointF annoPositionInParent{ anno->getViewObject()->X.getValue(),
                                    anno->getViewObject()->Y.getValue()};
    QPointF moveToPosition = parentOrigin + annoPositionInParent;
    anno->moveBy(-moveToPosition.x(), -moveToPosition.y());
    parent->addToGroup(anno);
    anno->setZValue(ZVALUE::DIMENSION);
}

// ?? why does this not get parented to its leader here??
// the taskdialog sets the Leader property in the weld feature.
// the weld symbol draws itself based on the leader's geometry, but is not added to the leader's
// group(?why?)
QGIView* QGSPage::addWeldSymbol(TechDraw::DrawWeldSymbol* weldFeat)
{
    QGIWeldSymbol *weldView = new QGIWeldSymbol;
    weldView->setViewFeature(weldFeat);
    addItem(weldView);

    return weldView;
}


//! ensure that all QGIViews are parented correctly in the scene
void QGSPage::setViewParents()
{
    const std::vector<QGIView*>& allItems = getViews();

    for (auto& item : allItems) {
        if (item->group()) {
            // this item already has a parent in the scene.  probably should check if it is the
            // correct parent
            continue;
        }

        QGIView* parent = findParent(item);
        if (parent) {
            // item has a parent, so make sure it belongs to parent's group
            addItemToParent(item, parent);
        }
    }
}

//! find the graphic for a DocumentObject
QGIView* QGSPage::findQViewForDocObj(App::DocumentObject* obj) const
{
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
    auto* dim = dynamic_cast<TechDraw::DrawViewDimension*>(myFeat);
    if (dim) {
        std::vector<App::DocumentObject*> objs = dim->References2D.getValues();

        if (!objs.empty()) {
            // Attach the dimension to the first object's group
            for (auto* qview : qviews) {
                if (strcmp(qview->getViewName(), objs.at(0)->getNameInDocument()) == 0) {
                    return qview;
                }
            }
        }
    }

    //If type is balloon we check references first
    auto* balloon = dynamic_cast<TechDraw::DrawViewBalloon*>(myFeat);
    if (balloon) {
        App::DocumentObject* obj = balloon->SourceView.getValue();

        if (obj) {
            // Attach the Balloon to the first object's group
            for (auto* qview : qviews) {
                if (strcmp(qview->getViewName(), obj->getNameInDocument()) == 0) {
                    return qview;
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
    QList<QGraphicsItem*> list = items();
    QList<QGraphicsItem*> qgiv;
    //find only QGIV's
    for (auto q : list) {
        QString viewFamily = QStringLiteral("QGIV");
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
    for (auto* obj : list) {

        if (!hasQView(obj))
            missing.push_back(obj);

        if (obj->isDerivedFrom<TechDraw::DrawViewCollection>()) {
            std::vector<App::DocumentObject*> missingChildViews;
            auto* collection = static_cast<TechDraw::DrawViewCollection*>(obj);
            // Find Child Views recursively
            findMissingViews(collection->getViews(), missingChildViews);

            // Append the views to current missing list
            for (auto* missingChild : missingChildViews) {
                missing.push_back(missingChild);
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
            TechDraw::DrawView* viewObj = qv->getViewObject();
            int numParentPages = viewObj->countParentPages();
            if (numParentPages == 0) {
                //DrawView does not belong to any DrawPage
                //remove QGItem from QGScene
                removeQView(qv);
            }
            else if (numParentPages == 1) {
                //Does DrawView belong to this DrawPage?
                TechDraw::DrawPage* parentPage = viewObj->findParentPage();
                if (thisPage != parentPage) {
                    // The view could be a child of a link (ea Dimension).
                    TechDraw::DrawView* parentView = viewObj->claimParent();
                    for (auto* v : thisPage->getViews()) {
                        if (v == parentView) {
                            continue;
                        }
                    }

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
    for (auto* obj : list) {

        //Check child objects too recursively
        if (obj->isDerivedFrom<TechDraw::DrawViewCollection>()) {
            auto* collection = static_cast<TechDraw::DrawViewCollection*>(obj);
            if (orphanExists(viewName, collection->getViews()))
                return true;
        }

        // Unsure if we can compare pointers so rely on name
        if (strcmp(viewName, obj->getNameInDocument()) == 0) {
            return true;
        }
    }
    return false;
}

//NOTE: this doesn't add missing views.  see fixOrphans()
void QGSPage::redrawAllViews()
{
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
// Also true of QGraphicsSvgItems.
void QGSPage::setExportingSvg(bool enable)
{
    m_exportingSvg = enable;
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
    QString svgDescription = QStringLiteral("Drawing page: ") + pageName
        + QStringLiteral(" exported from FreeCAD document: ") + docName;

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

    svgGen.setTitle(QStringLiteral("FreeCAD SVG Export"));
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
    QDomDocument exportDoc(QStringLiteral("SvgDoc"));
    QFile file(temporaryFile.fileName());
    if (!file.open(QIODevice::ReadOnly)) {
        Base::Console().error("QGSPage::ppsvg - tempfile open error\n");
        return;
    }
    if (!exportDoc.setContent(&file)) {
        Base::Console().error("QGSPage::ppsvg - xml error\n");
        file.close();
        return;
    }
    file.close();

    QDomElement exportDocElem = exportDoc.documentElement();//root <svg>

    // Insert Freecad SVG namespace into namespace declarations
    exportDocElem.setAttribute(QStringLiteral("xmlns:freecad"),
                               QString::fromUtf8(FREECAD_SVG_NS_URI));
    // Insert all namespaces used by TechDraw's page template SVGs
    exportDocElem.setAttribute(QStringLiteral("xmlns:svg"), QString::fromUtf8(SVG_NS_URI));
    exportDocElem.setAttribute(QStringLiteral("xmlns:cc"), QString::fromUtf8(CC_NS_URI));
    exportDocElem.setAttribute(QStringLiteral("xmlns:dc"), QString::fromUtf8(DC_NS_URI));
    exportDocElem.setAttribute(QStringLiteral("xmlns:rdf"), QString::fromUtf8(RDF_NS_URI));
    exportDocElem.setAttribute(QStringLiteral("xmlns:inkscape"),
                               QString::fromUtf8(INKSCAPE_NS_URI));
    exportDocElem.setAttribute(QStringLiteral("xmlns:sodipodi"),
                               QString::fromUtf8(SODIPODI_NS_URI));

    // Create the root group which will host the drawing group and the template group
    QDomElement rootGroup = exportDoc.createElement(QStringLiteral("g"));
    rootGroup.setAttribute(QStringLiteral("id"), pageName);
    rootGroup.setAttribute(QStringLiteral("inkscape:groupmode"), QStringLiteral("layer"));
    rootGroup.setAttribute(QStringLiteral("inkscape:label"), QStringLiteral("TechDraw"));

    // Now insert our template
    QGISVGTemplate* svgTemplate = dynamic_cast<QGISVGTemplate*>(pageTemplate);
    if (svgTemplate) {
        DrawSVGTemplate* drawTemplate = svgTemplate->getSVGTemplate();
        if (drawTemplate) {
            QString templateSvg = drawTemplate->processTemplate();
            QDomDocument templateResultDoc(QStringLiteral("SvgDoc"));
            if (templateResultDoc.setContent(templateSvg)) {
                QDomElement templateDocElem = templateResultDoc.documentElement();

                // Insert the template into a new group with id set to template name
                QDomElement templateGroup = exportDoc.createElement(QStringLiteral("g"));
                Base::FileInfo fi(drawTemplate->PageResult.getValue());
                templateGroup.setAttribute(QStringLiteral("id"),
                                           QString::fromUtf8(fi.fileName().c_str()));
                templateGroup.setAttribute(QStringLiteral("style"),
                                           QStringLiteral("stroke: none;"));

                // Scale the template group correctly
                templateGroup.setAttribute(QStringLiteral("transform"),
                                           QStringLiteral("scale(%1, %2)")
                                               .arg(Rez::guiX(1.0), 0, 'f')
                                               .arg(Rez::guiX(1.0), 0, 'f'));

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
        drawingGroup.setAttribute(QStringLiteral("id"), QStringLiteral("DrawingContent"));
        rootGroup.appendChild(drawingGroup);
    }
    exportDocElem.appendChild(rootGroup);

    // As icing on the cake, get rid of the empty <g>'s Qt SVG generator painting inserts.
    removeEmptyGroups(exportDocElem);

    // Time to save our product
    QFile outFile(fileName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Base::Console().error("QGSP::ppxml - failed to open file for writing: %s\n",
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
    Base::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Colors")->GetUnsigned("Background", 0x70707000));
    return fcColor.asValue<QColor>();
}


#include <Mod/TechDraw/Gui/moc_QGSPage.cpp>
