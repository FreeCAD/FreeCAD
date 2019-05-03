/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <qstatusbar.h>
# include <qstring.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/details/SoLineDetail.h>
#endif

#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoWindowElement.h>

#include <Inventor/SoFullPath.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/SoPickedPoint.h>

#include "View3DInventor.h"
#include "View3DInventorViewer.h"

#include <Base/Console.h>
#include "SoFCSelection.h"
#include "MainWindow.h"
#include "Selection.h"
#include "SoFCSelectionAction.h"
#include "SoFCInteractiveElement.h"
#include "SoFCUnifiedSelection.h"
#include "ViewParams.h"

// For 64-bit system the method using the front buffer doesn't work at all for lines.
// Thus, use the method which forces a redraw every time. This is a bit slower but at
// least it works.
//
// Disable front buffer in all cases, in order to spare the repeating logic of
// handling selection contextn. SoFCSelection is not really used that much
// anyway. 
//
// #if defined(_OCC64) // is set by configure or cmake
# define NO_FRONTBUFFER
// #endif

using namespace Gui;

SoFullPath * Gui::SoFCSelection::currenthighlight = NULL;


// *************************************************************************

SO_NODE_SOURCE(SoFCSelection);

/*!
  Constructor.
*/
SoFCSelection::SoFCSelection()
{
    SO_NODE_CONSTRUCTOR(SoFCSelection);

    SO_NODE_ADD_FIELD(colorHighlight, (SbColor(0.8f, 0.1f, 0.1f)));
    SO_NODE_ADD_FIELD(colorSelection, (SbColor(0.1f, 0.8f, 0.1f)));
    SO_NODE_ADD_FIELD(style,          (EMISSIVE));
    SO_NODE_ADD_FIELD(highlightMode,  (AUTO));
    SO_NODE_ADD_FIELD(selectionMode,  (SEL_ON));
    SO_NODE_ADD_FIELD(selected,       (NOTSELECTED));
    SO_NODE_ADD_FIELD(documentName,   (""));
    SO_NODE_ADD_FIELD(objectName,     (""));
    SO_NODE_ADD_FIELD(subElementName, (""));
    SO_NODE_ADD_FIELD(useNewSelection, (true));

    SO_NODE_DEFINE_ENUM_VALUE(Styles, EMISSIVE);
    SO_NODE_DEFINE_ENUM_VALUE(Styles, EMISSIVE_DIFFUSE);
    SO_NODE_DEFINE_ENUM_VALUE(Styles, BOX);
    SO_NODE_SET_SF_ENUM_TYPE(style,   Styles);

    SO_NODE_DEFINE_ENUM_VALUE(HighlightModes, AUTO);
    SO_NODE_DEFINE_ENUM_VALUE(HighlightModes, ON);
    SO_NODE_DEFINE_ENUM_VALUE(HighlightModes, OFF);
    SO_NODE_SET_SF_ENUM_TYPE (highlightMode, HighlightModes);

    SO_NODE_DEFINE_ENUM_VALUE(SelectionModes, SEL_ON);
    SO_NODE_DEFINE_ENUM_VALUE(SelectionModes, SEL_OFF);
    SO_NODE_SET_SF_ENUM_TYPE (selectionMode,  SelectionModes);

    SO_NODE_DEFINE_ENUM_VALUE(Selected, NOTSELECTED);
    SO_NODE_DEFINE_ENUM_VALUE(Selected, SELECTED);
    SO_NODE_SET_SF_ENUM_TYPE(selected,  Selected);

    highlighted = false;
    bShift      = false;
    bCtrl       = false;

    selected = NOTSELECTED;

    useNewSelection = ViewParams::instance()->getUseNewSelection();
    selContext = std::make_shared<SelContext>();
    selContext2 = std::make_shared<SelContext>();
}

/*!
  Destructor.
*/
SoFCSelection::~SoFCSelection()
{
    // If we're being deleted and we're the current highlight,
    // NULL out that variable
    if (currenthighlight != NULL &&
        (!currenthighlight->getTail()->isOfType(SoFCSelection::getClassTypeId()))) {
        currenthighlight->unref();
        currenthighlight = NULL;
    }
    //delete THIS;
}

// doc from parent
void
SoFCSelection::initClass(void)
{
    SO_NODE_INIT_CLASS(SoFCSelection,SoGroup,"Group");
}

void SoFCSelection::finish()
{
    atexit_cleanup();
}

/*!
  Static method that can be used to turn off the current highlight.
*/
void
SoFCSelection::turnOffCurrentHighlight(SoGLRenderAction * action)
{
    SoFCSelection::turnoffcurrent(action);
}

void SoFCSelection::doAction(SoAction *action)
{
    if(useNewSelection.getValue()) {
        if (action->getTypeId() == Gui::SoHighlightElementAction::getClassTypeId()) {
            Gui::SoHighlightElementAction* hlaction = static_cast<Gui::SoHighlightElementAction*>(action);
            if(!hlaction->isHighlighted()) {
                auto ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext,false);
                if(ctx->isHighlighted()) {
                    ctx->highlightIndex = -1;
                    touch();
                }
            }else{
                auto ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext);
                ctx->highlightColor = hlaction->getColor();
                if(!ctx->isHighlighted()) {
                    ctx->highlightIndex = 0;
                    touch();
                }
            }
            return;
        } else if (action->getTypeId() == Gui::SoSelectionElementAction::getClassTypeId()) {
            Gui::SoSelectionElementAction* selaction = static_cast<Gui::SoSelectionElementAction*>(action);
            if (selaction->getType() == Gui::SoSelectionElementAction::All ||
                selaction->getType() == Gui::SoSelectionElementAction::Append) {
                SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext);
                ctx->selectionColor = selaction->getColor();
                if(!ctx->isSelectAll()) {
                    ctx->selectAll();
                    this->touch();
                }
            } else if (selaction->getType() == Gui::SoSelectionElementAction::None ||
                       selaction->getType() == Gui::SoSelectionElementAction::Remove) {
                SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext,false);
                if(ctx && ctx->isSelected()) {
                    ctx->selectionIndex.clear();
                    this->touch();
                }
            }
            return;
        }
    }

    if (action->getTypeId() == SoFCDocumentAction::getClassTypeId()) {
        SoFCDocumentAction *docaction = (SoFCDocumentAction*)action;
        this->documentName = docaction->documentName;
    }

    if (action->getTypeId() == SoFCDocumentObjectAction::getClassTypeId()) {
        SoFCDocumentObjectAction* objaction = static_cast<SoFCDocumentObjectAction*>(action);
        objaction->documentName  = this->documentName.getValue();
        objaction->objectName    = this->objectName.getValue();
        objaction->componentName = this->subElementName.getValue();
        objaction->setHandled();
    }

    if(!useNewSelection.getValue()) {

        if (action->getTypeId() == SoFCEnableHighlightAction::getClassTypeId()) {
            SoFCEnableHighlightAction *preaction = (SoFCEnableHighlightAction*)action;
            if (preaction->highlight) {
                this->highlightMode = SoFCSelection::AUTO;
            }
            else {
                this->highlightMode = SoFCSelection::OFF;
            }
        }

        if (action->getTypeId() == SoFCEnableSelectionAction::getClassTypeId()) {
            SoFCEnableSelectionAction *selaction = (SoFCEnableSelectionAction*)action;
            if (selaction->selection) {
                this->selectionMode = SoFCSelection::SEL_ON;
            }
            else {
                this->selectionMode = SoFCSelection::SEL_OFF;
                if (selected.getValue() == SELECTED) {
                    this->selected = NOTSELECTED;
                }
            }
        }

        if (action->getTypeId() == SoFCSelectionColorAction::getClassTypeId()) {
            SoFCSelectionColorAction *colaction = (SoFCSelectionColorAction*)action;
            this->colorSelection = colaction->selectionColor;
        }

        if (action->getTypeId() == SoFCHighlightColorAction::getClassTypeId()) {
            SoFCHighlightColorAction *colaction = (SoFCHighlightColorAction*)action;
            this->colorHighlight = colaction->highlightColor;
        }

        if (selectionMode.getValue() == SEL_ON && action->getTypeId() == SoFCSelectionAction::getClassTypeId()) {
            SoFCSelectionAction *selaction = static_cast<SoFCSelectionAction*>(action);

            if (selaction->SelChange.Type == SelectionChanges::AddSelection || 
                selaction->SelChange.Type == SelectionChanges::RmvSelection) {
                if (documentName.getValue() == selaction->SelChange.pDocName &&
                    objectName.getValue() == selaction->SelChange.pObjectName &&
                    (subElementName.getValue() == selaction->SelChange.pSubName || 
                    *(selaction->SelChange.pSubName) == '\0') ) {
                    if (selaction->SelChange.Type == SelectionChanges::AddSelection) {
                        if(selected.getValue() == NOTSELECTED){
                            selected = SELECTED;
                        }
                    }
                    else {
                        if(selected.getValue() == SELECTED){
                            selected = NOTSELECTED;
                        }
                    }
                    return;
                }
            }
            else if (selaction->SelChange.Type == SelectionChanges::ClrSelection) {
                if (documentName.getValue() == selaction->SelChange.pDocName ||
                    strcmp(selaction->SelChange.pDocName,"") == 0){
                    if(selected.getValue() == SELECTED){
                        selected = NOTSELECTED;
                    }
                
                }
            }
            else if (selaction->SelChange.Type == SelectionChanges::SetSelection) {
                bool sel = Selection().isSelected(
                        documentName.getValue().getString(),
                        objectName.getValue().getString()/*,
                        subElementName.getValue().getString()*/);
                if (sel) {
                    if (selected.getValue() == NOTSELECTED) {
                        selected = SELECTED;
                    }
                }
                else {
                    if (selected.getValue() == SELECTED) {
                        selected = NOTSELECTED;
                    }
                }
            }
        }
    }

    inherited::doAction( action );
}

int SoFCSelection::getPriority(const SoPickedPoint* p)
{
    const SoDetail* detail = p->getDetail();
    if(!detail) return 0;
    if(detail->isOfType(SoFaceDetail::getClassTypeId())) return 1;
    if(detail->isOfType(SoLineDetail::getClassTypeId())) return 2;
    if(detail->isOfType(SoPointDetail::getClassTypeId())) return 3;
    return 0;
}

const SoPickedPoint*
SoFCSelection::getPickedPoint(SoHandleEventAction* action) const
{
    // To identify the picking of lines in a concave area we have to 
    // get all intersection points. If we have two or more intersection
    // points where the first is of a face and the second of a line with
    // almost similar coordinates we use the second point, instead.
    const SoPickedPointList & points = action->getPickedPointList();
    if (points.getLength() == 0)
        return 0;
    else if (points.getLength() == 1)
        return points[0];
    //const SoPickedPoint* pp0 = points[0];
    //const SoPickedPoint* pp1 = points[1];
    //const SoDetail* det0 = pp0->getDetail();
    //const SoDetail* det1 = pp1->getDetail();
    //if (det0 && det0->isOfType(SoFaceDetail::getClassTypeId()) &&
    //    det1 && det1->isOfType(SoLineDetail::getClassTypeId())) {
    //    const SbVec3f& pt0 = pp0->getPoint();
    //    const SbVec3f& pt1 = pp1->getPoint();
    //    if (pt0.equals(pt1, 0.01f))
    //        return pp1;
    //}

    //return pp0;
    
    const SoPickedPoint* picked = points[0];

    int picked_prio = getPriority(picked);
    const SbVec3f& picked_pt = picked->getPoint();

   
    for(int i=1; i<points.getLength();i++) {
        const SoPickedPoint* cur = points[i];
        int cur_prio = getPriority(cur);
        const SbVec3f& cur_pt = cur->getPoint();

        if ((cur_prio > picked_prio) && picked_pt.equals(cur_pt, 0.01f)) {
            picked = cur;
            picked_prio = cur_prio;
        }
    }
    return picked;

}

// doc from parent
void
SoFCSelection::handleEvent(SoHandleEventAction * action)
{
    if(useNewSelection.getValue()) {
       inherited::handleEvent( action );
       return;
    }

    static char buf[513];
    HighlightModes mymode = (HighlightModes) this->highlightMode.getValue();
    const SoEvent * event = action->getEvent();
#ifdef NO_FRONTBUFFER
    // mouse move events for preselection
    if (event->isOfType(SoLocation2Event::getClassTypeId())) {
        // NOTE: If preselection is off then we do not check for a picked point because otherwise this search may slow
        // down extremely the system on really big data sets. In this case we just check for a picked point if the data
        // set has been selected.
        if (mymode == AUTO || mymode == ON) {
            const SoPickedPoint * pp = this->getPickedPoint(action);
            if (pp && pp->getPath()->containsPath(action->getCurPath())) {
                if (!highlighted) {
                    if (Gui::Selection().setPreselect(documentName.getValue().getString()
                                           ,objectName.getValue().getString()
                                           ,subElementName.getValue().getString()
                                           ,pp->getPoint()[0]
                                           ,pp->getPoint()[1]
                                           ,pp->getPoint()[2])){
                        SoFCSelection::turnoffcurrent(action);
                        SoFCSelection::currenthighlight = (SoFullPath*)action->getCurPath()->copy();
                        SoFCSelection::currenthighlight->ref();
                        highlighted = true;
                        this->touch(); // force scene redraw
                        this->redrawHighlighted(action, true);
                    }
                }
                
                const auto &pt = pp->getPoint();
                snprintf(buf,512,"Preselected: %s.%s.%s (%g, %g, %g)",documentName.getValue().getString()
                                           ,objectName.getValue().getString()
                                           ,subElementName.getValue().getString()
                                           ,fabs(pt[0])>1e-7?pt[0]:0.0
                                           ,fabs(pt[1])>1e-7?pt[1]:0.0
                                           ,fabs(pt[2])>1e-7?pt[2]:0.0);

                getMainWindow()->showMessage(QString::fromLatin1(buf));
            }
            else { // picked point
                if (highlighted) {
                    if (mymode == AUTO)
                        SoFCSelection::turnoffcurrent(action);
                    //FIXME: I think we should set 'highlighted' to false whenever no point is picked
                    //else
                    highlighted = false;
                    Gui::Selection().rmvPreselect();
                }
            }
        }
    } // key press events
    else if (event->isOfType(SoKeyboardEvent ::getClassTypeId())) {
        SoKeyboardEvent  * const e = (SoKeyboardEvent  *) event;
        if (SoKeyboardEvent::isKeyPressEvent(e,SoKeyboardEvent::LEFT_SHIFT)     ||
            SoKeyboardEvent::isKeyPressEvent(e,SoKeyboardEvent::RIGHT_SHIFT)     )
            bShift = true;
        if (SoKeyboardEvent::isKeyReleaseEvent(e,SoKeyboardEvent::LEFT_SHIFT)   ||
            SoKeyboardEvent::isKeyReleaseEvent(e,SoKeyboardEvent::RIGHT_SHIFT)   )
            bShift = false;
        if (SoKeyboardEvent::isKeyPressEvent(e,SoKeyboardEvent::LEFT_CONTROL)   ||
            SoKeyboardEvent::isKeyPressEvent(e,SoKeyboardEvent::RIGHT_CONTROL)   )
            bCtrl = true;
        if (SoKeyboardEvent::isKeyReleaseEvent(e,SoKeyboardEvent::LEFT_CONTROL) ||
            SoKeyboardEvent::isKeyReleaseEvent(e,SoKeyboardEvent::RIGHT_CONTROL) )
            bCtrl = false;
    } // mouse press events for (de)selection
    else if (event->isOfType(SoMouseButtonEvent::getClassTypeId())) {
        SoMouseButtonEvent * const e = (SoMouseButtonEvent *) event;
        if (SoMouseButtonEvent::isButtonReleaseEvent(e,SoMouseButtonEvent::BUTTON1)) {
            //FIXME: Shouldn't we remove the preselection for newly selected objects?
            //       Otherwise the tree signals that an object is preselected even though it is hidden. (Werner)
            const SoPickedPoint * pp = this->getPickedPoint(action);
            if (pp && pp->getPath()->containsPath(action->getCurPath())) {
                const auto &pt = pp->getPoint();
                if (bCtrl) {
                    if (Gui::Selection().isSelected(documentName.getValue().getString()
                                         ,objectName.getValue().getString()
                                         ,subElementName.getValue().getString())) {
                        Gui::Selection().rmvSelection(documentName.getValue().getString()
                                          ,objectName.getValue().getString()
                                          ,subElementName.getValue().getString());
                    } else {
                        Gui::Selection().addSelection(documentName.getValue().getString()
                                          ,objectName.getValue().getString()
                                          ,subElementName.getValue().getString()
                                          ,pt[0] ,pt[1] ,pt[2]);

                        if (mymode == OFF) {
                            snprintf(buf,512,"Selected: %s.%s.%s (%g, %g, %g)",documentName.getValue().getString()
                                                       ,objectName.getValue().getString()
                                                       ,subElementName.getValue().getString()
                                                       ,fabs(pt[0])>1e-7?pt[0]:0.0
                                                       ,fabs(pt[1])>1e-7?pt[1]:0.0
                                                       ,fabs(pt[2])>1e-7?pt[2]:0.0);

                            getMainWindow()->showMessage(QString::fromLatin1(buf));
                        }
                    }
                }
                else { // Ctrl
                    if (!Gui::Selection().isSelected(documentName.getValue().getString()
                                         ,objectName.getValue().getString()
                                         ,subElementName.getValue().getString())) {
                        Gui::Selection().clearSelection(documentName.getValue().getString());
                        Gui::Selection().addSelection(documentName.getValue().getString()
                                              ,objectName.getValue().getString()
                                              ,subElementName.getValue().getString()
                                              ,pt[0] ,pt[1] ,pt[2]);
                    }
                    else {
                        Gui::Selection().clearSelection(documentName.getValue().getString());
                        Gui::Selection().addSelection(documentName.getValue().getString()
                                              ,objectName.getValue().getString()
                                              ,0 ,pt[0] ,pt[1] ,pt[2]);
                    }

                    if (mymode == OFF) {
                        snprintf(buf,512,"Selected: %s.%s.%s (%g, %g, %g)",documentName.getValue().getString()
                                                   ,objectName.getValue().getString()
                                                   ,subElementName.getValue().getString()
                                                   ,fabs(pt[0])>1e-7?pt[0]:0.0
                                                   ,fabs(pt[1])>1e-7?pt[1]:0.0
                                                   ,fabs(pt[2])>1e-7?pt[2]:0.0);

                        getMainWindow()->showMessage(QString::fromLatin1(buf));
                    }
                }

                action->setHandled(); 
            } // picked point
        } // mouse release
    }

    inherited::handleEvent(action);
#else
    // If we don't need to pick for locate highlighting,
    // then just behave as separator and return.
    // NOTE: we still have to pick for ON even though we don't have
    // to re-render, because the app needs to be notified as the mouse
    // goes over locate highlight nodes.
    //if (highlightMode.getValue() == OFF) {
    //    inherited::handleEvent( action );
    //    return;
    //}

    
    //
    // If this is a mouseMotion event, then check for locate highlighting
    //
    if (event->isOfType(SoLocation2Event::getClassTypeId())) {
        // check to see if the mouse is over our geometry...
        SbBool underTheMouse = false;
        const SoPickedPoint * pp = this->getPickedPoint(action);
        SoFullPath *pPath = (pp != NULL) ? (SoFullPath *) pp->getPath() : NULL;
        if (pPath && pPath->containsPath(action->getCurPath())) {
            // Make sure I'm the lowest LocHL in the pick path!
            underTheMouse = true;
            for (int i = 0; i < pPath->getLength(); i++) {
                SoNode *node = pPath->getNodeFromTail(i);
                if (node->isOfType(SoFCSelection::getClassTypeId())) {
                    if (node != this)
                    underTheMouse = false;
                    break; // found the lowest LocHL - look no further
                }
            }
        }
        // Am I currently highlighted?
        if (isHighlighted(action)) {
            if (! underTheMouse) {
                // re-draw the object with it's normal color
                //if(mymode != OFF)
                redrawHighlighted(action, false);
                Gui::Selection().rmvPreselect();
            }
            else {
                action->setHandled();
                //const SoPickedPoint * pp = action->getPickedPoint();
                Gui::Selection().setPreselectCoord(pp->getPoint()[0]
                                                 ,pp->getPoint()[1]
                                                 ,pp->getPoint()[2]);
            }
        }
        // Else I am not currently highlighted
        else {
            // If under the mouse, then highlight!
            if (underTheMouse) {
                // draw this object highlighted
                if (mymode != OFF)
                    redrawHighlighted(action, true);
                //const SoPickedPoint * pp = action->getPickedPoint();
                Gui::Selection().setPreselect(documentName.getValue().getString()
                                                 ,objectName.getValue().getString()
                                                 ,subElementName.getValue().getString()
                                                 ,pp->getPoint()[0]
                                                 ,pp->getPoint()[1]
                                                 ,pp->getPoint()[2]);
            }
        }
        //if(selected == SELECTED){
        //    redrawHighlighted(action, true);
        //}
        //if(selectionCleared ){
        //    redrawHighlighted(action, false);
        //    selectionCleared = false;
        //}
    }
    // key press events
    else if (event->isOfType(SoKeyboardEvent ::getClassTypeId())) {
        SoKeyboardEvent  * const e = (SoKeyboardEvent  *) event;
        if (SoKeyboardEvent::isKeyPressEvent(e,SoKeyboardEvent::LEFT_SHIFT)     ||
            SoKeyboardEvent::isKeyPressEvent(e,SoKeyboardEvent::RIGHT_SHIFT)     )
            bShift = true;
        if (SoKeyboardEvent::isKeyReleaseEvent(e,SoKeyboardEvent::LEFT_SHIFT)   ||
            SoKeyboardEvent::isKeyReleaseEvent(e,SoKeyboardEvent::RIGHT_SHIFT)   )
            bShift = false;
        if (SoKeyboardEvent::isKeyPressEvent(e,SoKeyboardEvent::LEFT_CONTROL)   ||
            SoKeyboardEvent::isKeyPressEvent(e,SoKeyboardEvent::RIGHT_CONTROL)   )
            bCtrl = true;
        if (SoKeyboardEvent::isKeyReleaseEvent(e,SoKeyboardEvent::LEFT_CONTROL) ||
            SoKeyboardEvent::isKeyReleaseEvent(e,SoKeyboardEvent::RIGHT_CONTROL) )
            bCtrl = false;
    }
    // mouse press events for (de)selection (only if selection is enabled on this node)
    else if (event->isOfType(SoMouseButtonEvent::getClassTypeId()) && 
             selectionMode.getValue() == SoFCSelection::SEL_ON) {
        SoMouseButtonEvent * const e = (SoMouseButtonEvent *) event;
        if (SoMouseButtonEvent::isButtonReleaseEvent(e,SoMouseButtonEvent::BUTTON1)) {
            //FIXME: Shouldn't we remove the preselection for newly selected objects?
            //       Otherwise the tree signals that an object is preselected even though it is hidden. (Werner)
            const SoPickedPoint * pp = this->getPickedPoint(action);
            if (pp && pp->getPath()->containsPath(action->getCurPath())) {
                const auto &pt = pp->getPoint();
                if (bCtrl) {
                    if (Gui::Selection().isSelected(documentName.getValue().getString()
                                         ,objectName.getValue().getString()
                                         ,subElementName.getValue().getString())) {
                        Gui::Selection().rmvSelection(documentName.getValue().getString()
                                          ,objectName.getValue().getString()
                                          ,subElementName.getValue().getString());
                    }
                    else {
                        Gui::Selection().addSelection(documentName.getValue().getString()
                                          ,objectName.getValue().getString()
                                          ,subElementName.getValue().getString()
                                          ,pt[0] ,pt[1] ,pt[2]);

                        if (mymode == OFF) {
                            snprintf(buf,512,"Selected: %s.%s.%s (%g, %g, %g)",documentName.getValue().getString()
                                                       ,objectName.getValue().getString()
                                                       ,subElementName.getValue().getString()
                                                       ,fabs(pt[0])>1e-7?pt[0]:0.0
                                                       ,fabs(pt[1])>1e-7?pt[1]:0.0
                                                       ,fabs(pt[2])>1e-7?pt[2]:0.0);

                            getMainWindow()->showMessage(QString::fromLatin1(buf));
                        }
                    }
                }
                else { // Ctrl
                    if (!Gui::Selection().isSelected(documentName.getValue().getString()
                                         ,objectName.getValue().getString()
                                         ,subElementName.getValue().getString())) {
                        Gui::Selection().clearSelection(documentName.getValue().getString());
                        Gui::Selection().addSelection(documentName.getValue().getString()
                                              ,objectName.getValue().getString()
                                              ,subElementName.getValue().getString()
                                              ,pt[0] ,pt[1] ,pt[2]);
                    }
                    else {
                        Gui::Selection().clearSelection(documentName.getValue().getString());
                        Gui::Selection().addSelection(documentName.getValue().getString()
                                              ,objectName.getValue().getString()
                                              ,0 ,pt[0] ,pt[1] ,pt[2]);
                    }
 
                    if (mymode == OFF) {
                        snprintf(buf,512,"Selected: %s.%s.%s (%g, %g, %g)",documentName.getValue().getString()
                                                   ,objectName.getValue().getString()
                                                   ,subElementName.getValue().getString()
                                                   ,fabs(pt[0])>1e-7?pt[0]:0.0
                                                   ,fabs(pt[1])>1e-7?pt[1]:0.0
                                                   ,fabs(pt[2])>1e-7?pt[2]:0.0);

                        getMainWindow()->showMessage(QString::fromLatin1(buf));
                    }
                }

                action->setHandled(); 
            } // picked point
        } // mouse release
    }

    // Let the base class traverse the children.
    if (action->getGrabber() != this)
        inherited::handleEvent(action);
#endif
}

// doc from parent
void
SoFCSelection::GLRenderBelowPath(SoGLRenderAction * action)
{
    SoState * state = action->getState();
    SelContextPtr ctx = Gui::SoFCSelectionRoot::getRenderContext<SelContext>(this,selContext);
    if(selContext2->checkGlobal(ctx))
        ctx = selContext2;
    if(!useNewSelection.getValue() && selContext == ctx) {
        ctx->selectionColor = this->colorSelection.getValue();
        ctx->highlightColor = this->colorHighlight.getValue();
        if(this->selected.getValue()==SELECTED)
            ctx->selectAll();
        else
            ctx->selectionIndex.clear();
        ctx->highlightIndex = this->highlighted?0:-1;
    }

#ifdef NO_FRONTBUFFER
    // check if preselection is active
    state->push();
    this->setOverride(action,ctx);
    inherited::GLRenderBelowPath(action);
    state->pop();
#else
    // Set up state for locate highlighting (if necessary)
    GLint oldDepthFunc;
    SbBool drawHighlighted = preRender(action, oldDepthFunc);

    // now invoke the parent method
    inherited::GLRenderBelowPath(action);

    // Restore old depth buffer model if needed
    if (drawHighlighted || highlighted)
        glDepthFunc((GLenum)oldDepthFunc);

    // Clean up state if needed
    if (drawHighlighted)
        action->getState()->pop();
#endif
}

void SoFCSelection::GLRender(SoGLRenderAction * action)
{
    SoState * state = action->getState();
    SelContextPtr ctx = Gui::SoFCSelectionRoot::getRenderContext<SelContext>(this,selContext);
    if(selContext2->checkGlobal(ctx))
        ctx = selContext2;
    if(!useNewSelection.getValue() && selContext == ctx) {
        ctx->selectionColor = this->colorSelection.getValue();
        ctx->highlightColor = this->colorHighlight.getValue();
        if(this->selected.getValue()==SELECTED)
            ctx->selectAll();
        else
            ctx->selectionIndex.clear();
        ctx->highlightIndex = this->highlighted?0:-1;
    }

#ifdef NO_FRONTBUFFER
    // check if preselection is active
    state->push();
    this->setOverride(action,ctx);
    inherited::GLRender(action);
    state->pop();
#else
    // Set up state for locate highlighting (if necessary)
    GLint oldDepthFunc;
    SbBool drawHighlighted = preRender(action, oldDepthFunc);

    // now invoke the parent method
    inherited::GLRender(action);

    // Restore old depth buffer model if needed
    if (drawHighlighted || highlighted)
        glDepthFunc((GLenum)oldDepthFunc);

    // Clean up state if needed
    if (drawHighlighted)
        action->getState()->pop();
#endif
}

// doc from parent
void
SoFCSelection::GLRenderInPath(SoGLRenderAction * action)
{
    SelContextPtr ctx = Gui::SoFCSelectionRoot::getRenderContext<SelContext>(this,selContext);
    if(selContext2->checkGlobal(ctx))
        ctx = selContext2;
    if(!useNewSelection.getValue() && selContext == ctx) {
        ctx->selectionColor = this->colorSelection.getValue();
        ctx->highlightColor = this->colorHighlight.getValue();
        if(this->selected.getValue()==SELECTED)
            ctx->selectAll();
        else
            ctx->selectionIndex.clear();
        ctx->highlightIndex = this->highlighted?0:-1;
    }
#ifdef NO_FRONTBUFFER
    // check if preselection is active
    SoState * state = action->getState();
    state->push();
    this->setOverride(action,ctx);
    inherited::GLRenderInPath(action);
    state->pop();
#else
    // Set up state for locate highlighting (if necessary)
    GLint oldDepthFunc;
    SbBool drawHighlighted = preRender(action, oldDepthFunc);

    // now invoke the parent method
    inherited::GLRenderInPath(action);

    // Restore old depth buffer model if needed
    if (drawHighlighted || highlighted)
        glDepthFunc((GLenum)oldDepthFunc);

    // Clean up state if needed
    if (drawHighlighted)
        action->getState()->pop();
#endif
}

SbBool
SoFCSelection::preRender(SoGLRenderAction *action, GLint &oldDepthFunc)
//
////////////////////////////////////////////////////////////////////////
{
    // If not performing locate highlighting, just return.
    if (highlightMode.getValue() == OFF)
        return false;

    SoState *state = action->getState();

    // ??? prevent caching at this level - for some reason the
    // ??? SoWindowElement::copyMatchInfo() method get called, which should
    // ??? never be called. We are not caching this node correctly yet....
    //SoCacheElement::invalidate(state);

    SbBool drawHighlighted = (highlightMode.getValue() == ON || isHighlighted(action) || selected.getValue() == SELECTED);

    if (drawHighlighted) {
        // prevent diffuse & emissive color from leaking out...
        state->push();
        SbColor col;
        if (selected.getValue() == SELECTED)
            col = colorSelection.getValue();
        else
            col = colorHighlight.getValue();

        // Emissive Color
        SoLazyElement::setEmissive(state, &col);
        SoOverrideElement::setEmissiveColorOverride(state, this, true);

        // Diffuse Color
        if (style.getValue() == EMISSIVE_DIFFUSE) {
            SoLazyElement::setDiffuse(state, this, 1, &col, &colorpacker);
            SoOverrideElement::setDiffuseColorOverride(state, this, true);
        }
    }

    // Draw on top of other things at same z-buffer depth if:
    // [a] we're highlighted
    // [b] this is the highlighting pass. This occurs when changing from
    //     non-hilit to lit OR VICE VERSA.
    // Otherwise, leave it alone...
    if (drawHighlighted || highlighted) {
        glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
        if (oldDepthFunc != GL_LEQUAL)
            glDepthFunc(GL_LEQUAL);
    }

    return drawHighlighted;
}

/*!
  Empty method in Coin. Can be used by subclasses to be told
  when status change.
*/
void
SoFCSelection::redrawHighlighted(SoAction *  action , SbBool  doHighlight )
{
    Q_UNUSED(action); 
    Q_UNUSED(doHighlight); 
    //Base::Console().Log("SoFCSelection::redrawHighlighted() (%p) doHigh=%d \n",this,doHighlight?1:0);

#ifdef NO_FRONTBUFFER
#else
    // If we are about to highlight, and there is something else highlighted,
    // that something else needs to unhighlight.
    if (doHighlight && currenthighlight != NULL &&
        !(*((SoFullPath *)action->getCurPath()) == *currenthighlight)) {

        SoNode *tail = currenthighlight->getTail();
        if (tail->isOfType( SoFCSelection::getClassTypeId()))
            ((SoFCSelection *)tail)->redrawHighlighted(action, false);
        else {
            // Just get rid of the path. It's no longer valid for redraw.
            currenthighlight->unref();
            currenthighlight = NULL;
        }
    }

    SoPath *pathToRender;
    // save the path to ourself for later de-highlight
    if (doHighlight) {
        if (currenthighlight != NULL)
            currenthighlight->unref();
        currenthighlight = (SoFullPath *) action->getCurPath()->copy();
        currenthighlight->ref();

        // We will be rendering this new path to highlight it
        pathToRender = currenthighlight;
        pathToRender->ref();
    }
    // delete our path if we are no longer highlighted
    else {
        if (currenthighlight) {
            // We will be rendering this old path to unhighlight it
            pathToRender = currenthighlight;
            pathToRender->ref();

            currenthighlight->unref();
            currenthighlight = NULL;
        }
    }

    // If highlighting is forced on for this node, we don't need this special render.
    if (highlightMode.getValue() != AUTO) {
        pathToRender->unref();
        return;
    }

    SoState *state = action->getState();

    //void* window;
    //void* context;
    //void *display;
    QtGLWidget* window;
    SoGLRenderAction *glAction;
    //SoWindowElement::get(state, window, context, display, glAction);
    SoGLWidgetElement::get(state, window);
    SoGLRenderActionElement::get(state, glAction);

    // If we don't have a current window, then simply return...
    if (window == 0 /*|| context == NULL || display == NULL*/ || glAction == NULL)
        return;

    window->makeCurrent();
#ifndef WIN32
    // set the current window
    //glXMakeCurrent(display, window, context);
#endif
    // render into the front buffer (save the current buffering type)
    GLint whichBuffer;
    glGetIntegerv(GL_DRAW_BUFFER, &whichBuffer);
    if (whichBuffer != GL_FRONT)
        glDrawBuffer(GL_FRONT);

    highlighted = true;
    glAction->apply(pathToRender);
    highlighted = false;

    // restore the buffering type
    if (whichBuffer != GL_FRONT)
        glDrawBuffer((GLenum)whichBuffer);
    glFlush();

    pathToRender->unref();
#endif
}

SbBool 
SoFCSelection::readInstance  (  SoInput *  in, unsigned short  flags )
{
    // Note: The read in document name can be false, so the caller must ensure pointing to the correct document
    SbBool ret = inherited::readInstance(in, flags);
    return ret;
}
//
// update override state before rendering
//
void
SoFCSelection::setOverride(SoGLRenderAction * action, SelContextPtr ctx)
{
    HighlightModes mymode = (HighlightModes) this->highlightMode.getValue();
    bool preselected = ctx && ctx->isHighlighted() && (useNewSelection.getValue()||mymode == AUTO);
    if (!preselected && mymode!=ON && (!ctx || !ctx->isSelected()))
        return;

    // uniqueId is returned by SoNode::getNodeId(). It is used to node change
    // and for render cache update. In order to update cache on selection state
    // change, We manually change the id here by using a combined hash of the
    // original id and context pointer.
    auto oldId = this->uniqueId;
    this->uniqueId ^= std::hash<void*>()(ctx.get()) + 0x9e3779b9 + (oldId << 6) + (oldId >> 2);
    
    //Base::Console().Log("SoFCSelection::setOverride() (%p)\n",this);
    SoState * state = action->getState();
    if(!preselected)
        SoLazyElement::setEmissive(state, &ctx->selectionColor);
    else
        SoLazyElement::setEmissive(state, &ctx->highlightColor);
    SoOverrideElement::setEmissiveColorOverride(state, this, true);

    Styles mystyle = (Styles) this->style.getValue();
    if (mystyle == SoFCSelection::EMISSIVE_DIFFUSE) {
        if(!preselected)
            SoLazyElement::setDiffuse(state, this,1, &ctx->selectionColor,&colorpacker);
        else
            SoLazyElement::setDiffuse(state, this,1, &ctx->highlightColor,&colorpacker);
        SoOverrideElement::setDiffuseColorOverride(state, this, true);
    }

    this->uniqueId = oldId;
}

// private convenience method
void
SoFCSelection::turnoffcurrent(SoAction * action)
{
#ifdef NO_FRONTBUFFER
    if (SoFCSelection::currenthighlight &&
        SoFCSelection::currenthighlight->getLength()) {
        SoNode * tail = SoFCSelection::currenthighlight->getTail();
        if (tail->isOfType(SoFCSelection::getClassTypeId())) {
            ((SoFCSelection*)tail)->highlighted = false;
            ((SoFCSelection*)tail)->touch(); // force scene redraw
            if (action) ((SoFCSelection*)tail)->redrawHighlighted(action, false);
        }
    }
    if (SoFCSelection::currenthighlight) {
        SoFCSelection::currenthighlight->unref();
        SoFCSelection::currenthighlight = NULL;
    }
#else
    if (currenthighlight == NULL)
        return;

    SoNode *tail = currenthighlight->getTail();
    if (tail->isOfType(SoFCSelection::getClassTypeId())) {

        // don't redraw if we already are in the middle of rendering
        // (processing events during render abort might cause this)
        SoState *state = action->getState();
        if (state && state->getDepth() == 1)
            ((SoFCSelection *)tail)->redrawHighlighted(action, false);
    }
    else {
        // Just get rid of the path. It's no longer valid for redraw.
        currenthighlight->unref();
        currenthighlight = NULL;
    }
#endif
}

SbBool
SoFCSelection::isHighlighted(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    SoFullPath *actionPath = (SoFullPath *) action->getCurPath();
    return (currenthighlight != NULL &&
        currenthighlight->getTail() == actionPath->getTail() && // nested SoHL!
        *currenthighlight == *actionPath);
}

void SoFCSelection::applySettings ()
{
    // TODO Some view providers got copy of this code: make them use this (2015-09-03, Fat-Zer)
    // Note: SoFCUnifiedSelection got the same code, keep in sync or think about a way to share it
    float transparency;
    ParameterGrp::handle hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("View");
    bool enablePre = hGrp->GetBool("EnablePreselection", true);
    bool enableSel = hGrp->GetBool("EnableSelection", true);
    if (!enablePre) {
        this->highlightMode = Gui::SoFCSelection::OFF;
    }
    else {
        // Search for a user defined value with the current color as default
        SbColor highlightColor = this->colorHighlight.getValue();
        unsigned long highlight = (unsigned long)(highlightColor.getPackedValue());
        highlight = hGrp->GetUnsigned("HighlightColor", highlight);
        highlightColor.setPackedValue((uint32_t)highlight, transparency);
        this->colorHighlight.setValue(highlightColor);
    }
    if (!enableSel) {
        this->selectionMode = Gui::SoFCSelection::SEL_OFF;
    }
    else {
        // Do the same with the selection color
        SbColor selectionColor = this->colorSelection.getValue();
        unsigned long selection = (unsigned long)(selectionColor.getPackedValue());
        selection = hGrp->GetUnsigned("SelectionColor", selection);
        selectionColor.setPackedValue((uint32_t)selection, transparency);
        this->colorSelection.setValue(selectionColor);
    }
}

//#undef THIS
