// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2014 WandererFan <wandererfan@gmail.com>                *
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

# include <algorithm>    // std::find
# include <QGraphicsScene>
# include <QKeyEvent>

#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>

#include <Base/Console.h>
#include <Mod/TechDraw/App/DrawViewClip.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include "QGIViewClip.h"
#include "QGCustomClip.h"
#include "QGCustomRect.h"
#include "Rez.h"
#include "ViewProviderViewClip.h"


using namespace TechDrawGui;
using namespace TechDraw;

QGIViewClip::QGIViewClip() :
    m_frame(new QGCustomRect()),
    m_cliparea(new QGCustomClip())
{
    setHandlesChildEvents(false);
    // setHandlesChildEvents(true);
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);

    addToGroup(m_cliparea);

    constexpr double DefaultSize{5};
    m_cliparea->setPos(0., 0.);
    m_cliparea->setRect(0., 0., Rez::guiX(DefaultSize), Rez::guiX(DefaultSize));

    addToGroup(m_frame);
    m_frame->setPos(0., 0.);
    m_frame->setRect(0., 0., Rez::guiX(DefaultSize), Rez::guiX(DefaultSize));
}

void QGIViewClip::updateView(bool update)
{
    auto viewClip( dynamic_cast<TechDraw::DrawViewClip *>(getViewObject()) );
    if (!viewClip) {
        return;
    }

    if (update ||
        viewClip->isTouched() ||
        viewClip->Height.isTouched() ||
        viewClip->Width.isTouched() ||
        viewClip->ShowFrame.isTouched() ||
        viewClip->Views.isTouched() ) {
        draw();
    }

    QGIView::updateView(update);
}

void QGIViewClip::draw()
{
    if (!isVisible()) {
        return;
    }

    drawClip();
    drawBorder();
}

void QGIViewClip::drawClip()
{
    auto viewClip( dynamic_cast<TechDraw::DrawViewClip *>(getViewObject()) );

    if (!viewClip) {
        return;
    }

    prepareGeometryChange();
    double h = viewClip->Height.getValue();
    double w = viewClip->Width.getValue();
    QRectF r = QRectF(-Rez::guiX(w)/2, -Rez::guiX(h)/2, Rez::guiX(w), Rez::guiX(h));
    m_frame->setRect(r);                    // (-50, -50) -> (50, 50)
    m_frame->setPos(0., 0.);
    if (viewClip->ShowFrame.getValue()) {
        m_frame->show();
    } else {
        m_frame->hide();
    }

    //probably a slicker way to do this?
    QPointF midFrame   = m_frame->boundingRect().center();
    QPointF midMapped  = mapFromItem(m_frame, midFrame);
    QPointF clipOrigin = mapToItem(m_cliparea, midMapped);

    m_cliparea->setRect(r.adjusted(-1, -1, 1,1));

    std::vector<std::string> childNames = viewClip->getChildViewNames();
    //for all child Views in Clip, add the graphics representation of the View to the Clip group
    for (auto& name : childNames) {
        QGIView* qgiv = getQGIVByName((name));
        if (qgiv) {
            //TODO: why is qgiv never already in a group?
            if (qgiv->group() != m_cliparea) {
                qgiv->hide();
                scene()->removeItem(qgiv);
                m_cliparea->addToGroup(qgiv);
                qgiv->isInnerView(true);
                double x = Rez::guiX(qgiv->getViewObject()->X.getValue());
                double y = Rez::guiX(qgiv->getViewObject()->Y.getValue());
                qgiv->setPosition(clipOrigin.x() + x, clipOrigin.y() + y);
                qgiv->show();
            }
        } else {
            Base::Console().warning("Logic error? - drawClip() - qgiv for %s not found\n", name.c_str());   //gview for feature !exist
        }
    }

    //for all graphic views in qgigroup, remove from qgigroup the ones that aren't in ViewClip
    QList<QGraphicsItem*> qgItems = m_cliparea->childItems();
    for (auto& item :  qgItems) {
        auto* qv = dynamic_cast<QGIView*>(item);
        if (qv) {
            if (auto qvName = std::string(qv->getViewName());
                std::ranges::find(childNames, qvName) == childNames.end()) {
                m_cliparea->removeFromGroup(qv);
                removeFromGroup(qv);
                qv->isInnerView(false);
            }
        }
    }

    auto* vpClip = freecad_cast<ViewProviderViewClip*>(getViewProvider(viewClip));
    if (!vpClip) {
        return;
    }

    m_cliparea->setFlag(QGraphicsItem::ItemClipsChildrenToShape, vpClip->ClipChildren.getValue());
}


bool QGIViewClip::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        // if we accept this event, we should get a regular keystroke event next
        // which will be processed by QGVPage/QGVNavStyle keypress logic, but not forwarded to
        // Std_Delete
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->matches(QKeySequence::Delete))  {
            DrawView* selectedView = selectionIsInGroup();
            if (selectedView) {
                return forwardEventToSelection(getQGIVByName(selectedView->getNameInDocument()),
                                               event);
            }
        }
    }

    return QGraphicsItem::sceneEventFilter(watched, event);
}


//! returns first view in the selection that is a member of this clip group with subelements selected.
DrawView* QGIViewClip::selectionIsInGroup() const
{
    bool single = false;
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(nullptr, DrawView::getClassTypeId(),
                                           Gui::ResolveMode::OldStyleElement, single);
    if (selection.empty()) {
        return {};
    }

    auto* clipGroup = freecad_cast<DrawViewClip*>(getViewObject());
    for (auto& selItem : selection) {
        auto view = freecad_cast<DrawView*>(selItem.getObject());
        if (view &&
            clipGroup->isViewInClip(view) &&
            selItem.hasSubNames()) {
            return view;
        }
    }

    return {};
}


bool QGIViewClip::forwardEventToSelection(QGIView* qview, QEvent* event) const
{
    if (!qview) {
        return false;
    }

    return qview->pseudoEventFilter(qview, event);
}


