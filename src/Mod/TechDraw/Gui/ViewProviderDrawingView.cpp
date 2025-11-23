/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#include <limits>
#include <boost/signals2.hpp>
#include <boost/signals2/connection.hpp>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Metadata.h>
#include <Base/Color.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "ViewProviderDrawingView.h"
#include "ViewProviderDrawingViewExtension.h"
#include "MDIViewPage.h"
#include "QGIView.h"
#include "QGSPage.h"
#include "ViewProviderPage.h"

using namespace TechDrawGui;
using namespace TechDraw;
namespace sp = std::placeholders;

PROPERTY_SOURCE(TechDrawGui::ViewProviderDrawingView, Gui::ViewProviderDocumentObject)

ViewProviderDrawingView::ViewProviderDrawingView() :
    m_myName(std::string())
{
//    Base::Console().message("VPDV::VPDV\n");
    initExtension(this);

    sPixmap = "TechDraw_TreeView";
    static const char *group = "Base";

    auto showLabel = Preferences::alwaysShowLabel();
    ADD_PROPERTY_TYPE(KeepLabel ,(showLabel), group, App::Prop_None, "Keep Label on Page even if toggled off");
    ADD_PROPERTY_TYPE(StackOrder,(0),group,App::Prop_None,"Over or under lap relative to other views");

    // Do not show in property editor   why? wf  WF: because DisplayMode applies only to coin and we
    // don't use coin.
    DisplayMode.setStatus(App::Property::Hidden, true);
}

ViewProviderDrawingView::~ViewProviderDrawingView()
{
}

void ViewProviderDrawingView::attach(App::DocumentObject *pcFeat)
{
    ViewProviderDocumentObject::attach(pcFeat);

    //NOLINTBEGIN
    auto bnd = std::bind(&ViewProviderDrawingView::onGuiRepaint, this, sp::_1);
    auto bndProgressMessage = std::bind(&ViewProviderDrawingView::onProgressMessage, this, sp::_1, sp::_2, sp::_3);
    //NOLINTEND
    auto feature = getViewObject();
    if (feature) {
        if (feature->isAttachedToDocument()) {
            // it could happen that feature is not completely in the document yet and getNameInDocument returns
            // nullptr, so we only update m_myName if we got a valid string.
            m_myName = feature->getNameInDocument();
        }
        connectGuiRepaint = feature->signalGuiPaint.connect(bnd);
        connectProgressMessage = feature->signalProgressMessage.connect(bndProgressMessage);
        //TODO: would be good to start the QGIV creation process here, but no guarantee we actually have
        //      MDIVP or QGVP yet.
        // but parent page might.  we may not be part of the document yet though!
        // :( we're not part of the page yet either!
    } else {
        Base::Console().warning("VPDV::attach has no Feature!\n");
    }
}

void ViewProviderDrawingView::onChanged(const App::Property *prop)
{
    App::DocumentObject* obj = getObject();
    if (!obj || obj->isRestoring()) {
            Gui::ViewProviderDocumentObject::onChanged(prop);
            return;
    }

    if (prop == &Visibility) {
        //handled by ViewProviderDocumentObject
    } else if (prop == &KeepLabel) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }

    if (prop == &StackOrder) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->setStack(StackOrder.getValue());
        }
    }

    Gui::ViewProviderDocumentObject::onChanged(prop);
}

void ViewProviderDrawingView::show()
{
    TechDraw::DrawView* obj = getViewObject();
    if (!obj || obj->isRestoring())
        return;

    if (obj->isDerivedFrom<TechDraw::DrawView>()) {
        QGIView* qView = getQView();
        if (qView) {
            qView->draw();
            qView->show();
        }
    }
    ViewProviderDocumentObject::show();
}

void ViewProviderDrawingView::hide()
{
    TechDraw::DrawView* obj = getViewObject();
    if (!obj || obj->isRestoring())
        return;

    if (obj->isDerivedFrom<TechDraw::DrawView>()) {
        QGIView* qView = getQView();
        if (qView) {
            //note: hiding an item in the scene clears its selection status
            //      this confuses Gui::Selection.
            //      So we block selection changes while we are hiding the qgiv
            //      in FC Tree hiding does not change selection state.
            //      block/unblock selection protects against crash in Gui::SelectionSingleton::setVisible
            MDIViewPage* mdi = getMDIViewPage();
            if (mdi) {                  //if there is no mdivp, there is nothing to hide!
                mdi->blockSceneSelection(true);
                qView->hide();
                ViewProviderDocumentObject::hide();
                mdi->blockSceneSelection(false);
            }
        }
    }
}
QGIView* ViewProviderDrawingView::getQView()
{
    TechDraw::DrawView* dv = getViewObject();
    if (!dv) {
        return nullptr;
    }

    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(dv->getDocument());
    if (!guiDoc) {
        return nullptr;
    }

    ViewProviderPage* vpp = getViewProviderPage();
    if (!vpp) {
        return nullptr;
    }

    QGSPage* page = vpp->getQGSPage();
    if (page) {
        return dynamic_cast<QGIView *>(page->findQViewForDocObj(getViewObject()));
    }

    return nullptr;
}

bool ViewProviderDrawingView::isShow() const
{
    return Visibility.getValue();
}

void ViewProviderDrawingView::dropObject(App::DocumentObject* docObj)
{
    getViewProviderPage()->dropObject(docObj);
}

void ViewProviderDrawingView::startRestoring()
{
    Gui::ViewProviderDocumentObject::startRestoring();
}

void ViewProviderDrawingView::finishRestoring()
{
    fixColorAlphaValues();

    Gui::ViewProviderDocumentObject::finishRestoring();
}

void ViewProviderDrawingView::updateData(const App::Property* prop)
{
    TechDraw::DrawView *obj = getViewObject();
    App::PropertyLink *ownerProp = obj->getOwnerProperty();

    //only move the view on X, Y change
    if (prop == &obj->X
        || prop == &obj->Y) {
        QGIView* qgiv = getQView();
        if (qgiv && !qgiv->isSnapping()) {
            qgiv->QGIView::updateView(true);

            // Update also the owner/parent view, if there is any
            if (ownerProp) {
                auto owner = dynamic_cast<TechDraw::DrawView *>(ownerProp->getValue());
                if (owner) {
                    auto page = dynamic_cast<QGSPage *>(qgiv->scene());
                    if (page) {
                        QGIView *ownerView = page->getQGIVByName(owner->getNameInDocument());
                        if (ownerView) {
                            ownerView->updateView();
                        }
                    }
                }
            }
        }
    }
    else if (ownerProp && prop == ownerProp) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            QGIView *ownerView = nullptr;
            auto owner = dynamic_cast<TechDraw::DrawView *>(ownerProp->getValue());
            if (owner) {
                auto page = dynamic_cast<QGSPage *>(qgiv->scene());
                if (page) {
                    ownerView = page->getQGIVByName(owner->getNameInDocument());
                }
            }

            qgiv->switchParentItem(ownerView);
            qgiv->updateView();
        }
    }

    Gui::ViewProviderDocumentObject::updateData(prop);
}

ViewProviderPage* ViewProviderDrawingView::getViewProviderPage() const
{
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(getViewObject()->getDocument());
    if (guiDoc) {
        Gui::ViewProvider* vp = guiDoc->getViewProvider(getViewObject()->findParentPage());
        return freecad_cast<ViewProviderPage*>(vp);
    }
    return nullptr;
}

MDIViewPage* ViewProviderDrawingView::getMDIViewPage() const
{
    ViewProviderPage* vpp = getViewProviderPage();
    if (vpp) {
        return vpp->getMDIViewPage();
    }
    return nullptr;
}

Gui::MDIView *ViewProviderDrawingView::getMDIView() const
{
    return getMDIViewPage();
}

void ViewProviderDrawingView::onGuiRepaint(const TechDraw::DrawView* dv)
{
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(getViewObject()->getDocument());
    if (!guiDoc) {
        return;
    }

    std::vector<TechDraw::DrawPage*> pages = getViewObject()->findAllParentPages();
    if (pages.size() > 1) {
        multiParentPaint(pages);
    } else if (dv == getViewObject()) {
        singleParentPaint(dv);
    }
}

void ViewProviderDrawingView::multiParentPaint(std::vector<TechDraw::DrawPage*>& pages)
{
    for (auto& p : pages) {
        std::vector<App::DocumentObject*> views = p->Views.getValues();
        for (auto& v: views) {
            if (v != getViewObject()) {  //should this be dv from onGuiRepaint?
                continue;
            }
            //view v belongs to this page p
            ViewProviderPage* vpPage = getViewProviderPage();
            if (!vpPage) {
                continue;
            }
            if (vpPage->getQGSPage()) {
                QGIView* qView = dynamic_cast<QGIView *>(vpPage->getQGSPage()->findQViewForDocObj(v));
                if (qView) {
                    qView->updateView(true);
                }
            }
        }
    }
}

void ViewProviderDrawingView::singleParentPaint(const TechDraw::DrawView* dv)
{
    //original logic for 1 view on 1 page
    if (dv->isRemoving() ||
        dv->isRestoring()) {
        return;
    }
    QGIView* qgiv = getQView();
    if (qgiv) {
        qgiv->updateView(true);
    } else {       //we are not part of the Gui page yet. ask page to add us.
        ViewProviderPage* vpPage = getViewProviderPage();
        if (vpPage) {
            if (vpPage->getQGSPage()) {
                vpPage->getQGSPage()->addView(dv);
            }
        }
    }
}

//handle status updates from App/DrawView
void ViewProviderDrawingView::onProgressMessage(const TechDraw::DrawView* dv,
                                              const std::string featureName,
                                              const std::string text)
{
    Q_UNUSED(dv)
    showProgressMessage(featureName, text);
}

void ViewProviderDrawingView::showProgressMessage(const std::string featureName, const std::string text) const
{
    QString msg = QStringLiteral("%1 %2")
            .arg(QString::fromStdString(featureName),
                 QString::fromStdString(text));
    if (Gui::getMainWindow()) {
        //neither of these work! Base::Console().message() output preempts these messages??
//        Gui::getMainWindow()->showMessage(msg, 3000);
//        Gui::getMainWindow()->showStatus(Gui::MainWindow::Msg, msg);
        //Temporary implementation. This works, but the messages are queued up and
        //not displayed in the report window in real time??
        Base::Console().message("%s\n", qPrintable(msg));
    }
}

void ViewProviderDrawingView::stackUp()
{
    QGIView* v = getQView();
    if (v) {
        int z = StackOrder.getValue();
        z++;
        StackOrder.setValue(z);
        v->setStack(z);
    }
}

void ViewProviderDrawingView::stackDown()
{
    QGIView* v = getQView();
    if (v) {
        int z = StackOrder.getValue();
        z--;
        StackOrder.setValue(z);
        v->setStack(z);
    }
}

void ViewProviderDrawingView::stackTop()
{
    QGIView* qView = getQView();
    if (!qView || !getViewProviderPage()) {
        //no view, nothing to stack
        return;
    }
    int maxZ = std::numeric_limits<int>::min();
    auto parent = qView->parentItem();
    if (parent) {
        //if we have a parentItem, we have to stack within the parentItem, not within the page
        auto siblings = parent->childItems();
        for (auto& child : siblings) {
            if (child->zValue() > maxZ) {
                maxZ = child->zValue();
            }
        }
    } else {
        //if we have no parentItem, we are a top level QGIView and we need to stack
        //with respect to the other top level views on this page
        std::vector<App::DocumentObject*> peerObjects = getViewProviderPage()->claimChildren();
        Gui::Document* gDoc = getDocument();
        for (auto& peer: peerObjects) {
            auto vpPeer = gDoc->getViewProvider(peer);
            ViewProviderDrawingView* vpdv = static_cast<ViewProviderDrawingView*>(vpPeer);
            int z = vpdv->StackOrder.getValue();
            if (z > maxZ) {
                maxZ = z;
            }
        }
    }
    StackOrder.setValue(maxZ + 1);
    qView->setStack(maxZ + 1);
}

void ViewProviderDrawingView::stackBottom()
{
    QGIView* qView = getQView();
    if (!qView || !getViewProviderPage()) {
        //no view, nothing to stack
        return;
    }
    int minZ = std::numeric_limits<int>::max();
    auto parent = qView->parentItem();
    if (parent) {
        //if we have a parentItem, we have to stack within the parentItem, not within the page
        auto siblings = parent->childItems();
        for (auto& child : siblings) {
            if (child->zValue() < minZ) {
                minZ = child->zValue();
            }
        }
    } else {
        //TODO: need to special case DPGI or any other member of a collection
        //if we have no parentItem, we are a top level QGIView and we need to stack
        //with respect to the other top level views on this page
        std::vector<App::DocumentObject*> peerObjects = getViewProviderPage()->claimChildren();
        Gui::Document* gDoc = getDocument();
        for (auto& peer: peerObjects) {
            auto vpPeer = gDoc->getViewProvider(peer);
            ViewProviderDrawingView* vpdv = static_cast<ViewProviderDrawingView*>(vpPeer);
            int z = vpdv->StackOrder.getValue();
            if (z < minZ) {
                minZ = z;
            }
        }
    }
    StackOrder.setValue(minZ - 1);
    qView->setStack(minZ - 1);
}

const char*  ViewProviderDrawingView::whoAmI() const
{
    return m_myName.c_str();
}

TechDraw::DrawView* ViewProviderDrawingView::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawView*>(pcObject);
}


//! it can happen that child graphic items can lose their parent item if the
//! the parent is deleted, then undo is invoked.  The linkages on the App side are
//! handled by the undo mechanism, but the QGraphicsScene parentage is not reset.
void ViewProviderDrawingView::fixSceneDependencies()
{
    auto page = getViewProviderPage();
    if (!page) {
        return;
    }

    auto scene = page->getQGSPage();
    auto ourQView = getQView();

    // this is the logic for items other than Dimensions and Balloons
    auto children = getViewObject()->getUniqueChildren();
    for (auto& child : children) {
        if (child->isDerivedFrom<DrawViewDimension>() ||
            child->isDerivedFrom<DrawViewBalloon>() ) {
            // these are handled by ViewProviderViewPart
            continue;
        }
        auto* childQView = scene->findQViewForDocObj(child);
        auto* childGraphicParent = scene->findParent(childQView);
        if (childGraphicParent != ourQView) {
            scene->addItemToParent(childQView, ourQView);
        }
    }
}


std::vector<App::DocumentObject*> ViewProviderDrawingView::claimChildren() const
{
    std::vector<App::DocumentObject*> temp;
    const std::vector<App::DocumentObject *> &potentialChildren = getViewObject()->getInList();
    try {
      for(auto& child : potentialChildren) {
          auto* view = freecad_cast<DrawView *>(child);
          if (view && view->claimParent() == getViewObject()) {
              temp.push_back(view);
              continue;
          }
      }
    }
    catch (...) {
        return {};
    }
    return temp;
}


//! convert old style transparency values in PropertyColor to new style alpha channel values
void ViewProviderDrawingView::fixColorAlphaValues()
{
    if (!Preferences::fixColorAlphaOnLoad() ||
        checkMiniumumDocumentVersion(1, 1)) {
        return;
    }

    // check every PropertyColor for transparency vs alpha
    std::vector<App::Property*> allProperties;
    getPropertyList(allProperties);

    constexpr double alphaNone{0.0};
    constexpr double alphaFull{1.0};

    for (auto& prop : allProperties) {
        auto* colorProp = freecad_cast<App::PropertyColor*>(prop);
        if (colorProp) {
            // Here we are assuming that transparent colors are not used/need not be converted.
            // To invert more generally, colorOut.a = 1 - colorIn.a;, but we would need a different
            // mechanism to determine when to do the conversion.
            Base::Color colorTemp = colorProp->getValue();
            if (colorTemp.a == alphaNone) {
                colorTemp.a = alphaFull;
                colorProp->setValue(colorTemp);
            }
        }
    }
}


//! true if document toBeChecked was written by a program with version >= minMajor.minMinor.
//! note that we can not check point releases as only the major and minor are recorded in the Document.xml
//! file.
//! (ex <Document SchemaVersion="4" ProgramVersion="1.2R44322 +1 (Git)" FileVersion="1" StringHasher="1">)
bool ViewProviderDrawingView::checkMiniumumDocumentVersion(App::Document* toBeChecked,
                                                           int minMajor,
                                                           int minMinor)
{
    const char* docVersionText = toBeChecked->getProgramVersion();
    int docMajor{0};
    int docMinor{0};
    // stole this bit from App::AttachExtension.
    // NOLINTNEXTLINE
    if (sscanf(docVersionText, "%d.%d", &docMajor, &docMinor) != 2) {
        Base::Console().warning("Failed to retrieve document version number for %s\n",
                    toBeChecked ? toBeChecked->getName() : "noname");
        return false;   // ?? should we fail here? the file appears broken.
    }

    return std::tie(docMajor, docMinor) >= std::tie(minMajor, minMinor);
}


