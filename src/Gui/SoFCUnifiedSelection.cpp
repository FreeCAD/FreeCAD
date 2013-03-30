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
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoComplexityTypeElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoElements.h>
#include <Inventor/elements/SoFontNameElement.h>
#include <Inventor/elements/SoFontSizeElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoProfileCoordinateElement.h>
#include <Inventor/elements/SoProfileElement.h>
#include <Inventor/elements/SoSwitchElement.h>
#include <Inventor/elements/SoUnitsElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/SoPickedPoint.h>

#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Document.h>
#include <App/DocumentObject.h>

#include "SoFCUnifiedSelection.h"
#include "Application.h"
#include "MainWindow.h"
#include "Selection.h"
#include "ViewProvider.h"
#include "SoFCInteractiveElement.h"
#include "SoFCSelectionAction.h"
#include "ViewProviderDocumentObject.h"

using namespace Gui;

SoFullPath * Gui::SoFCUnifiedSelection::currenthighlight = NULL;


// *************************************************************************

SO_NODE_SOURCE(SoFCUnifiedSelection);

/*!
  Constructor.
*/
SoFCUnifiedSelection::SoFCUnifiedSelection() : pcDocument(0)
{
    SO_NODE_CONSTRUCTOR(SoFCUnifiedSelection);

    SO_NODE_ADD_FIELD(colorHighlight, (SbColor(1.0f, 0.6f, 0.0f)));
    SO_NODE_ADD_FIELD(colorSelection, (SbColor(0.1f, 0.8f, 0.1f)));
    SO_NODE_ADD_FIELD(highlightMode,  (AUTO));
    SO_NODE_ADD_FIELD(selectionMode,  (ON));
    SO_NODE_ADD_FIELD(selectionRole,  (TRUE));

    SO_NODE_DEFINE_ENUM_VALUE(HighlightModes, AUTO);
    SO_NODE_DEFINE_ENUM_VALUE(HighlightModes, ON);
    SO_NODE_DEFINE_ENUM_VALUE(HighlightModes, OFF);
    SO_NODE_SET_SF_ENUM_TYPE (highlightMode, HighlightModes);

    highlighted = FALSE;
}

/*!
  Destructor.
*/
SoFCUnifiedSelection::~SoFCUnifiedSelection()
{
    // If we're being deleted and we're the current highlight,
    // NULL out that variable
    if (currenthighlight != NULL) {
        currenthighlight->unref();
        currenthighlight = NULL;
    }
}

// doc from parent
void
SoFCUnifiedSelection::initClass(void)
{
    SO_NODE_INIT_CLASS(SoFCUnifiedSelection,SoSeparator,"Separator");
}

void SoFCUnifiedSelection::finish()
{
    atexit_cleanup();
}

void SoFCUnifiedSelection::applySettings()
{
    float transparency;
    ParameterGrp::handle hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("View");
    bool enablePre = hGrp->GetBool("EnablePreselection", true);
    bool enableSel = hGrp->GetBool("EnableSelection", true);
    if (!enablePre) {
        this->highlightMode = SoFCUnifiedSelection::OFF;
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
        this->selectionMode = SoFCUnifiedSelection::OFF;
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

const char* SoFCUnifiedSelection::getFileFormatName(void) const
{
    return "Separator";
}

void SoFCUnifiedSelection::write(SoWriteAction * action)
{
    SoOutput * out = action->getOutput();
    if (out->getStage() == SoOutput::WRITE) {
        // Do not write out the fields of this class
        if (this->writeHeader(out, TRUE, FALSE)) return;
        SoGroup::doAction((SoAction *)action);
        this->writeFooter(out);
    }
    else {
        inherited::write(action);
    }
}

int SoFCUnifiedSelection::getPriority(const SoPickedPoint* p)
{
    const SoDetail* detail = p->getDetail();
    if (!detail)                                           return 0;
    if (detail->isOfType(SoFaceDetail::getClassTypeId()))  return 1;
    if (detail->isOfType(SoLineDetail::getClassTypeId()))  return 2;
    if (detail->isOfType(SoPointDetail::getClassTypeId())) return 3;
    return 0;
}

const SoPickedPoint*
SoFCUnifiedSelection::getPickedPoint(SoHandleEventAction* action) const
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
    
    const SoPickedPoint* picked = points[0];
    int picked_prio = getPriority(picked);
    const SbVec3f& picked_pt = picked->getPoint();

    for (int i=1; i<points.getLength();i++) {
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

void SoFCUnifiedSelection::doAction(SoAction *action)
{
    if (action->getTypeId() == SoFCEnableHighlightAction::getClassTypeId()) {
        SoFCEnableHighlightAction *preaction = (SoFCEnableHighlightAction*)action;
        if (preaction->highlight) {
            this->highlightMode = SoFCUnifiedSelection::AUTO;
        }
        else {
            this->highlightMode = SoFCUnifiedSelection::OFF;
        }
    }

    if (action->getTypeId() == SoFCEnableSelectionAction::getClassTypeId()) {
        SoFCEnableSelectionAction *selaction = (SoFCEnableSelectionAction*)action;
        if (selaction->selection) {
            this->selectionMode = SoFCUnifiedSelection::ON;
        }
        else {
            this->selectionMode = SoFCUnifiedSelection::OFF;
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

    if (selectionMode.getValue() == ON && action->getTypeId() == SoFCSelectionAction::getClassTypeId()) {
        SoFCSelectionAction *selaction = static_cast<SoFCSelectionAction*>(action);
        if (selaction->SelChange.Type == SelectionChanges::AddSelection || 
            selaction->SelChange.Type == SelectionChanges::RmvSelection) {
            // selection changes inside the 3d view are handled in handleEvent()
            if (!currenthighlight) {
                App::Document* doc = App::GetApplication().getDocument(selaction->SelChange.pDocName);
                App::DocumentObject* obj = doc->getObject(selaction->SelChange.pObjectName);
                ViewProvider*vp = Application::Instance->getViewProvider(obj);
                if (vp && vp->useNewSelectionModel() && vp->isSelectable()) {
                    SoDetail* detail = vp->getDetail(selaction->SelChange.pSubName);
                    SoSelectionElementAction::Type type = SoSelectionElementAction::None;
                    if (selaction->SelChange.Type == SelectionChanges::AddSelection) {
                        if (detail)
                            type = SoSelectionElementAction::Append;
                        else
                            type = SoSelectionElementAction::All;
                    }
                    else {
                        if (detail)
                            type = SoSelectionElementAction::Remove;
                        else
                            type = SoSelectionElementAction::None;
                    }

                    SoSelectionElementAction action(type);
                    action.setColor(this->colorSelection.getValue());
                    action.setElement(detail);
                    action.apply(vp->getRoot());
                    delete detail;
                }
            }
        }
        else if (selaction->SelChange.Type == SelectionChanges::ClrSelection ||
                 selaction->SelChange.Type == SelectionChanges::SetSelection) {
            std::vector<ViewProvider*> vps;
            if (this->pcDocument)
                vps = this->pcDocument->getViewProvidersOfType(ViewProviderDocumentObject::getClassTypeId());
            for (std::vector<ViewProvider*>::iterator it = vps.begin(); it != vps.end(); ++it) {
                ViewProviderDocumentObject* vpd = static_cast<ViewProviderDocumentObject*>(*it);
                if (vpd->useNewSelectionModel()) {
                    if (Selection().isSelected(vpd->getObject()) && vpd->isSelectable()) {
                        SoSelectionElementAction action(SoSelectionElementAction::All);
                        action.setColor(this->colorSelection.getValue());
                        action.apply(vpd->getRoot());
                    }
                    else {
                        SoSelectionElementAction action(SoSelectionElementAction::None);
                        action.setColor(this->colorSelection.getValue());
                        action.apply(vpd->getRoot());
                    }
                }
            }
        }
    }

    inherited::doAction( action );
}

// doc from parent
void
SoFCUnifiedSelection::handleEvent(SoHandleEventAction * action)
{
    // If off then don't handle this event
    if (!selectionRole.getValue()) {
        inherited::handleEvent(action);
        return;
    }

    static char buf[513];
    HighlightModes mymode = (HighlightModes) this->highlightMode.getValue();
    const SoEvent * event = action->getEvent();

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
        // NOTE: If preselection is off then we do not check for a picked point because otherwise this search may slow
        // down extremely the system on really big data sets. In this case we just check for a picked point if the data
        // set has been selected.
        if (mymode == AUTO || mymode == ON) {
            // check to see if the mouse is over our geometry...
            const SoPickedPoint * pp = this->getPickedPoint(action);
            SoFullPath *pPath = (pp != NULL) ? (SoFullPath *) pp->getPath() : NULL;
            ViewProvider *vp = 0;
            ViewProviderDocumentObject* vpd = 0;
            if (this->pcDocument && pPath && pPath->containsPath(action->getCurPath()))
                vp = this->pcDocument->getViewProviderByPathFromTail(pPath);
            if (vp && vp->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
                vpd = static_cast<ViewProviderDocumentObject*>(vp);

            SbBool old_state = highlighted;
            highlighted = FALSE;
            if (vpd && vpd->useNewSelectionModel() && vpd->isSelectable()) {
                std::string documentName = vpd->getObject()->getDocument()->getName();
                std::string objectName = vpd->getObject()->getNameInDocument();
                std::string subElementName = vpd->getElement(pp ? pp->getDetail() : 0);

                static char buf[513];
                snprintf(buf,512,"Preselected: %s.%s.%s (%f,%f,%f)",documentName.c_str()
                                           ,objectName.c_str()
                                           ,subElementName.c_str()
                                           ,pp->getPoint()[0]
                                           ,pp->getPoint()[1]
                                           ,pp->getPoint()[2]);

                getMainWindow()->showMessage(QString::fromAscii(buf),3000);

                if (Gui::Selection().setPreselect(documentName.c_str()
                                       ,objectName.c_str()
                                       ,subElementName.c_str()
                                       ,pp->getPoint()[0]
                                       ,pp->getPoint()[1]
                                       ,pp->getPoint()[2])){

                    SoSearchAction sa;
                    sa.setNode(vp->getRoot());
                    sa.apply(vp->getRoot());
                    if (sa.getPath()) {
                        highlighted = TRUE;
                        if (currenthighlight && currenthighlight->getTail() != sa.getPath()->getTail()) {
                            SoHighlightElementAction action;
                            action.setHighlighted(FALSE);
                            action.apply(currenthighlight);
                            currenthighlight->unref();
                            currenthighlight = 0;
                            old_state = !highlighted;
                        }

                        currenthighlight = static_cast<SoFullPath*>(sa.getPath()->copy());
                        currenthighlight->ref();
                    }
                }
            }

            if (currenthighlight/* && old_state != highlighted*/) {
                SoHighlightElementAction action;
                action.setHighlighted(highlighted);
                action.setColor(this->colorHighlight.getValue());
                action.setElement(pp ? pp->getDetail() : 0);
                action.apply(currenthighlight);
                if (!highlighted) {
                    currenthighlight->unref();
                    currenthighlight = 0;
                }
                this->touch();
            }
        }
    }
    // mouse press events for (de)selection
    else if (event->isOfType(SoMouseButtonEvent::getClassTypeId()) && 
             selectionMode.getValue() == SoFCUnifiedSelection::ON) {
        const SoMouseButtonEvent* e = static_cast<const SoMouseButtonEvent *>(event);
        if (SoMouseButtonEvent::isButtonReleaseEvent(e,SoMouseButtonEvent::BUTTON1)) {
            // check to see if the mouse is over a geometry...
            const SoPickedPoint * pp = this->getPickedPoint(action);
            SoFullPath *pPath = (pp != NULL) ? (SoFullPath *) pp->getPath() : NULL;
            ViewProvider *vp = 0;
            ViewProviderDocumentObject* vpd = 0;
            if (this->pcDocument && pPath && pPath->containsPath(action->getCurPath()))
                vp = this->pcDocument->getViewProviderByPathFromTail(pPath);
            if (vp && vp->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
                vpd = static_cast<ViewProviderDocumentObject*>(vp);
            if (vpd && vpd->useNewSelectionModel() && vpd->isSelectable()) {
                SoSelectionElementAction::Type type = SoSelectionElementAction::None;
                std::string documentName = vpd->getObject()->getDocument()->getName();
                std::string objectName = vpd->getObject()->getNameInDocument();
                std::string subElementName = vpd->getElement(pp ? pp->getDetail() : 0);
                if (event->wasCtrlDown()) {
                    if (Gui::Selection().isSelected(documentName.c_str()
                                         ,objectName.c_str()
                                         ,subElementName.c_str())) {
                        Gui::Selection().rmvSelection(documentName.c_str()
                                          ,objectName.c_str()
                                          ,subElementName.c_str());
                        type = SoSelectionElementAction::Remove;
                    }
                    else {
                        bool ok = Gui::Selection().addSelection(documentName.c_str()
                                          ,objectName.c_str()
                                          ,subElementName.c_str()
                                          ,pp->getPoint()[0]
                                          ,pp->getPoint()[1]
                                          ,pp->getPoint()[2]);
                        if (ok)
                            type = SoSelectionElementAction::Append;
                        if (mymode == OFF) {
                            snprintf(buf,512,"Selected: %s.%s.%s (%f,%f,%f)",documentName.c_str()
                                                       ,objectName.c_str()
                                                       ,subElementName.c_str()
                                                       ,pp->getPoint()[0]
                                                       ,pp->getPoint()[1]
                                                       ,pp->getPoint()[2]);

                            getMainWindow()->showMessage(QString::fromAscii(buf),3000);
                        }
                    }
                }
                else { // Ctrl
                    if (!Gui::Selection().isSelected(documentName.c_str()
                                         ,objectName.c_str()
                                         ,subElementName.c_str())) {
                        Gui::Selection().clearSelection(documentName.c_str());
                        bool ok = Gui::Selection().addSelection(documentName.c_str()
                                              ,objectName.c_str()
                                              ,subElementName.c_str()
                                              ,pp->getPoint()[0]
                                              ,pp->getPoint()[1]
                                              ,pp->getPoint()[2]);
                        if (ok)
                            type = SoSelectionElementAction::Append;
                    }
                    else {
                        Gui::Selection().clearSelection(documentName.c_str());
                        bool ok = Gui::Selection().addSelection(documentName.c_str()
                                              ,objectName.c_str()
                                              ,0
                                              ,pp->getPoint()[0]
                                              ,pp->getPoint()[1]
                                              ,pp->getPoint()[2]);
                        if (ok)
                            type = SoSelectionElementAction::All;
                    }

                    if (mymode == OFF) {
                        snprintf(buf,512,"Selected: %s.%s.%s (%f,%f,%f)",documentName.c_str()
                                                   ,objectName.c_str()
                                                   ,subElementName.c_str()
                                                   ,pp->getPoint()[0]
                                                   ,pp->getPoint()[1]
                                                   ,pp->getPoint()[2]);

                        getMainWindow()->showMessage(QString::fromAscii(buf),3000);
                    }
                }

                action->setHandled(); 
                if (currenthighlight) {
                    SoSelectionElementAction action(type);
                    action.setColor(this->colorSelection.getValue());
                    action.setElement(pp ? pp->getDetail() : 0);
                    action.apply(currenthighlight);
                    this->touch();
                }
            } // picked point
        } // mouse release
    }

    inherited::handleEvent(action);
}

// ---------------------------------------------------------------

SO_ACTION_SOURCE(SoHighlightElementAction);

void SoHighlightElementAction::initClass()
{
    SO_ACTION_INIT_CLASS(SoHighlightElementAction,SoAction);

    SO_ENABLE(SoHighlightElementAction, SoSwitchElement);

    SO_ACTION_ADD_METHOD(SoNode,nullAction);

    SO_ENABLE(SoHighlightElementAction, SoCoordinateElement);

    SO_ACTION_ADD_METHOD(SoGroup,callDoAction);
    SO_ACTION_ADD_METHOD(SoIndexedLineSet,callDoAction);
    SO_ACTION_ADD_METHOD(SoIndexedFaceSet,callDoAction);
    SO_ACTION_ADD_METHOD(SoPointSet,callDoAction);
}

SoHighlightElementAction::SoHighlightElementAction () : _highlight(FALSE), _det(0)
{
    SO_ACTION_CONSTRUCTOR(SoHighlightElementAction);
}

SoHighlightElementAction::~SoHighlightElementAction()
{
}

void SoHighlightElementAction::beginTraversal(SoNode *node)
{
    traverse(node);
}

void SoHighlightElementAction::callDoAction(SoAction *action,SoNode *node)
{
    node->doAction(action);
}

void SoHighlightElementAction::setHighlighted(SbBool ok)
{
    this->_highlight = ok;
}

SbBool SoHighlightElementAction::isHighlighted() const
{
    return this->_highlight;
}

void SoHighlightElementAction::setColor(const SbColor& c)
{
    this->_color = c;
}

const SbColor& SoHighlightElementAction::getColor() const
{
    return this->_color;
}

void SoHighlightElementAction::setElement(const SoDetail* det)
{
    this->_det = det;
}

const SoDetail* SoHighlightElementAction::getElement() const
{
    return this->_det;
}

// ---------------------------------------------------------------

SO_ACTION_SOURCE(SoSelectionElementAction);

void SoSelectionElementAction::initClass()
{
    SO_ACTION_INIT_CLASS(SoSelectionElementAction,SoAction);

    SO_ENABLE(SoSelectionElementAction, SoSwitchElement);

    SO_ACTION_ADD_METHOD(SoNode,nullAction);

    SO_ENABLE(SoSelectionElementAction, SoCoordinateElement);

    SO_ACTION_ADD_METHOD(SoCoordinate3,callDoAction);
    SO_ACTION_ADD_METHOD(SoGroup,callDoAction);
    SO_ACTION_ADD_METHOD(SoIndexedLineSet,callDoAction);
    SO_ACTION_ADD_METHOD(SoIndexedFaceSet,callDoAction);
    SO_ACTION_ADD_METHOD(SoPointSet,callDoAction);
}

SoSelectionElementAction::SoSelectionElementAction (Type t) : _type(t), _select(FALSE), _det(0)
{
    SO_ACTION_CONSTRUCTOR(SoSelectionElementAction);
}

SoSelectionElementAction::~SoSelectionElementAction()
{
}

void SoSelectionElementAction::beginTraversal(SoNode *node)
{
    traverse(node);
}

void SoSelectionElementAction::callDoAction(SoAction *action,SoNode *node)
{
    node->doAction(action);
}

SoSelectionElementAction::Type
SoSelectionElementAction::getType() const
{
    return this->_type;
}

void SoSelectionElementAction::setColor(const SbColor& c)
{
    this->_color = c;
}

const SbColor& SoSelectionElementAction::getColor() const
{
    return this->_color;
}

void SoSelectionElementAction::setElement(const SoDetail* det)
{
    this->_det = det;
}

const SoDetail* SoSelectionElementAction::getElement() const
{
    return this->_det;
}
