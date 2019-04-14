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
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoProfileCoordinateElement.h>
#include <Inventor/elements/SoProfileElement.h>
#include <Inventor/elements/SoSwitchElement.h>
#include <Inventor/elements/SoUnitsElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/SoPickedPoint.h>

#ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
#else
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# include <GL/gl.h>
#endif

#include <QtOpenGL.h>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Document.h>
#include <App/DocumentObject.h>
#include <App/ComplexGeoData.h>

#include "SoFCUnifiedSelection.h"
#include "Application.h"
#include "MainWindow.h"
#include "Selection.h"
#include "ViewProvider.h"
#include "SoFCInteractiveElement.h"
#include "SoFCSelectionAction.h"
#include "ViewProviderDocumentObject.h"
#include "ViewProviderGeometryObject.h"
#include "ViewParams.h"

FC_LOG_LEVEL_INIT("SoFCUnifiedSelection",false,true,true);

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
    SO_NODE_ADD_FIELD(selectionRole,  (true));

    SO_NODE_DEFINE_ENUM_VALUE(HighlightModes, AUTO);
    SO_NODE_DEFINE_ENUM_VALUE(HighlightModes, ON);
    SO_NODE_DEFINE_ENUM_VALUE(HighlightModes, OFF);
    SO_NODE_SET_SF_ENUM_TYPE (highlightMode, HighlightModes);

    detailPath = static_cast<SoFullPath*>(new SoPath(20));
    detailPath->ref();

    setPreSelection = false;
    preSelection = -1;
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
    if (detailPath) {
        detailPath->unref();
        detailPath = NULL;
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

bool SoFCUnifiedSelection::hasHighlight() {
    return currenthighlight != NULL;
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
        if (this->writeHeader(out, true, false)) return;
        SoGroup::doAction((SoAction *)action);
        this->writeFooter(out);
    }
    else {
        inherited::write(action);
    }
}

int SoFCUnifiedSelection::getNumSelected(void) const
{
    return this->selectionList.getLength();
}

const SoPathList* SoFCUnifiedSelection::getList(void) const
{
    return &this->selectionList;
}

void SoFCUnifiedSelection::addPath(SoPath * path)
{
    this->selectionList.append(path);
}

void SoFCUnifiedSelection::removePath(const int which)
{
    SoPath * path = this->selectionList[which];
    path->ref();
    this->selectionList.remove(which);
    path->unref();
}

SoPath * SoFCUnifiedSelection::copyFromThis(const SoPath * path) const
{
    SoPath * newpath = NULL;
    path->ref();
    int i = path->findNode(this);
    if (i >= 0) {
        newpath = path->copy(i);
    }
    path->unrefNoDelete();
    return newpath;
}

int SoFCUnifiedSelection::findPath(const SoPath * path) const
{
    int idx = -1;

    // make copy only if necessary
    if (path->getHead() != this) {
        SoPath * newpath = this->copyFromThis(path);
        if (newpath) {
            newpath->ref();
            idx = this->selectionList.findPath(*newpath);
            newpath->unref();
        }
        else {
            idx = -1;
        }
    }
    else {
        idx = this->selectionList.findPath(*path);
    }
    return idx;
}

SoPath * SoFCUnifiedSelection::searchNode(SoNode * node) const
{
    SoSearchAction sa;
    sa.setNode(node);
    sa.apply(const_cast<SoFCUnifiedSelection*>(this));
    SoPath * path = sa.getPath();
    if (path)
        path->ref();
    return path;
}

void SoFCUnifiedSelection::select(SoNode * node)
{
    SoPath * path = this->searchNode(node);
    if (path) {
        // don't ref() the path. searchNode() will ref it before returning
        if (this->findPath(path) < 0)
            this->addPath(path);
        path->unref();
    }
}

void SoFCUnifiedSelection::deselect(const SoPath * path)
{
    int idx = this->findPath(path);
    if (idx >= 0) this->removePath(idx);
}

void SoFCUnifiedSelection::deselect(SoNode * node)
{
    SoPath * path = this->searchNode(node);
    if (path) {
        // don't ref() the path. searchNode() will ref it before returning
        this->deselect(path);
        path->unref();
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

std::vector<SoFCUnifiedSelection::PickedInfo> 
SoFCUnifiedSelection::getPickedList(SoHandleEventAction* action, bool singlePick) const
{
    ViewProvider *last_vp = 0;
    std::vector<PickedInfo> ret;
    const SoPickedPointList & points = action->getPickedPointList();
    for(int i=0,count=points.getLength();i<count;++i) {
        PickedInfo info;
        info.pp = points[i];
        info.vpd = 0;
        ViewProvider *vp = 0;
        SoFullPath *path = static_cast<SoFullPath *>(info.pp->getPath());
        if (this->pcDocument && path && path->containsPath(action->getCurPath())) {
            vp = this->pcDocument->getViewProviderByPathFromHead(path);
            if(singlePick && last_vp && last_vp!=vp)
                return ret;
        }
        if(!vp || !vp->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) {
            if(!singlePick) continue;
            if(ret.empty())
                ret.push_back(info);
            break;
        }
        info.vpd = static_cast<ViewProviderDocumentObject*>(vp);
        if(!info.vpd->useNewSelectionModel() || !info.vpd->isSelectable()) {
            if(!singlePick) continue;
            if(ret.empty()) {
                info.vpd = 0;
                ret.push_back(info);
            }
            break;
        }
        if(!info.vpd->getElementPicked(info.pp,info.element))
            continue;

        if(singlePick) 
            last_vp = vp;
        ret.push_back(info);
    }

    if(ret.size()<=1) return ret;

    // To identify the picking of lines in a concave area we have to 
    // get all intersection points. If we have two or more intersection
    // points where the first is of a face and the second of a line with
    // almost similar coordinates we use the second point, instead.

    int picked_prio = getPriority(ret[0].pp);
    auto last_vpd = ret[0].vpd;
    const SbVec3f& picked_pt = ret.front().pp->getPoint();
    auto itPicked = ret.begin();
    for(auto it=ret.begin()+1;it!=ret.end();++it) {
        auto &info = *it;
        if(last_vpd != info.vpd) 
            break;

        int cur_prio = getPriority(info.pp);
        const SbVec3f& cur_pt = info.pp->getPoint();

        if ((cur_prio > picked_prio) && picked_pt.equals(cur_pt, 0.01f)) {
            itPicked = it;
            picked_prio = cur_prio;
        }
    }

    if(singlePick) {
        std::vector<PickedInfo> sret(itPicked,itPicked+1);
        return sret;
    }
    if(itPicked != ret.begin())
        std::swap(*itPicked, *ret.begin());
    return ret;
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

    if (highlightMode.getValue() != OFF && action->getTypeId() == SoFCHighlightAction::getClassTypeId()) {
        SoFCHighlightAction *hilaction = static_cast<SoFCHighlightAction*>(action);
        // Do not clear currently highlighted object when setting new pre-selection
        if (!setPreSelection && hilaction->SelChange.Type == SelectionChanges::RmvPreselect) {
            if (currenthighlight) {
                SoHighlightElementAction action;
                action.apply(currenthighlight);
                currenthighlight->unref();
                currenthighlight = 0;
            }
        }
    }

    if (selectionMode.getValue() == ON && action->getTypeId() == SoFCSelectionAction::getClassTypeId()) {
        SoFCSelectionAction *selaction = static_cast<SoFCSelectionAction*>(action);
        if (selaction->SelChange.Type == SelectionChanges::AddSelection || 
            selaction->SelChange.Type == SelectionChanges::RmvSelection) {
            // selection changes inside the 3d view are handled in handleEvent()
            App::Document* doc = App::GetApplication().getDocument(selaction->SelChange.pDocName);
            App::DocumentObject* obj = doc->getObject(selaction->SelChange.pObjectName);
            ViewProvider*vp = Application::Instance->getViewProvider(obj);
            if (vp && vp->useNewSelectionModel() && vp->isSelectable()) {
                SoDetail *detail = nullptr;
                detailPath->truncate(0);
                if(!selaction->SelChange.pSubName || !selaction->SelChange.pSubName[0] ||
                    vp->getDetailPath(selaction->SelChange.pSubName,detailPath,true,detail))
                {
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

                    if(checkSelectionStyle(type,vp)) {
                        SoSelectionElementAction action(type);
                        action.setColor(this->colorSelection.getValue());
                        action.setElement(detail);
                        if(detailPath->getLength())
                            action.apply(detailPath);
                        else
                            action.apply(vp->getRoot());
                    }
                }
                detailPath->truncate(0);
                delete detail;
            }
        }else if (selaction->SelChange.Type == SelectionChanges::ClrSelection) {
            SoSelectionElementAction action(SoSelectionElementAction::None);
            for(int i=0;i<this->getNumChildren();++i)
                action.apply(this->getChild(i));
        }else if(selaction->SelChange.Type == SelectionChanges::SetSelection) {
            std::vector<ViewProvider*> vps;
            if (this->pcDocument)
                vps = this->pcDocument->getViewProvidersOfType(ViewProviderDocumentObject::getClassTypeId());
            for (std::vector<ViewProvider*>::iterator it = vps.begin(); it != vps.end(); ++it) {
                ViewProviderDocumentObject* vpd = static_cast<ViewProviderDocumentObject*>(*it);
                if (vpd->useNewSelectionModel()) {
                    SoSelectionElementAction::Type type;
                    if(Selection().isSelected(vpd->getObject()) && vpd->isSelectable())
                        type = SoSelectionElementAction::All;
                    else
                        type = SoSelectionElementAction::None;
                    if (checkSelectionStyle(type, vpd)) {
                        SoSelectionElementAction action(type);
                        action.setColor(this->colorSelection.getValue());
                        action.apply(vpd->getRoot());
                    }
                }
            }
        } else if (selaction->SelChange.Type == SelectionChanges::SetPreselectSignal) {
            // selection changes inside the 3d view are handled in handleEvent()
            App::Document* doc = App::GetApplication().getDocument(selaction->SelChange.pDocName);
            App::DocumentObject* obj = doc->getObject(selaction->SelChange.pObjectName);
            ViewProvider*vp = Application::Instance->getViewProvider(obj);
            if (vp && vp->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()) &&
                vp->useNewSelectionModel() && vp->isSelectable()) 
            {
                detailPath->truncate(0);
                SoDetail *det = 0;
                if(vp->getDetailPath(selaction->SelChange.pSubName,detailPath,true,det)) {
                    setHighlight(detailPath,det,static_cast<ViewProviderDocumentObject*>(vp),
                            selaction->SelChange.pSubName, 
                            selaction->SelChange.x,
                            selaction->SelChange.y,
                            selaction->SelChange.z);
                }
                delete det;
            }
        }
    }

    inherited::doAction( action );
}

bool SoFCUnifiedSelection::setHighlight(const PickedInfo &info) {
    if(!info.pp)
        return setHighlight(0,0,0,0,0.0,0.0,0.0);
    const auto &pt = info.pp->getPoint();
    return setHighlight(static_cast<SoFullPath*>(info.pp->getPath()), 
            info.pp->getDetail(), info.vpd, info.element.c_str(), pt[0],pt[1],pt[2]);
}

bool SoFCUnifiedSelection::setHighlight(SoFullPath *path, const SoDetail *det, 
        ViewProviderDocumentObject *vpd, const char *element, float x, float y, float z) 
{
    Base::FlagToggler<SbBool> flag(setPreSelection);

    bool highlighted = false;
    if(path && path->getLength() && 
       vpd && vpd->getObject() && vpd->getObject()->getNameInDocument()) 
    {
        const char *docname = vpd->getObject()->getDocument()->getName();
        const char *objname = vpd->getObject()->getNameInDocument();

        this->preSelection = 1;
        static char buf[513];
        snprintf(buf,512,"Preselected: %s.%s.%s (%g, %g, %g)"
                ,docname,objname,element
                ,fabs(x)>1e-7?x:0.0
                ,fabs(y)>1e-7?y:0.0
                ,fabs(z)>1e-7?z:0.0);

        getMainWindow()->showMessage(QString::fromLatin1(buf));

        int ret = Gui::Selection().setPreselect(docname,objname,element,x,y,z);
        if(ret<0 && currenthighlight)
            return true;

        if(ret) {
            if (currenthighlight) {
                SoHighlightElementAction action;
                action.setHighlighted(false);
                action.apply(currenthighlight);
                currenthighlight->unref();
                currenthighlight = 0;
            }
            currenthighlight = static_cast<SoFullPath*>(path->copy());
            currenthighlight->ref();
            highlighted = true;
        }
    }

    if(currenthighlight) {
        SoHighlightElementAction action;
        action.setHighlighted(highlighted);
        action.setColor(this->colorHighlight.getValue());
        action.setElement(det);
        action.apply(currenthighlight);
        if(!highlighted) {
            currenthighlight->unref();
            currenthighlight = 0;
            Selection().rmvPreselect();
        }
        this->touch();
    }
    return highlighted;
}

bool SoFCUnifiedSelection::setSelection(const std::vector<PickedInfo> &infos, bool ctrlDown) {
    if(infos.empty() || !infos[0].vpd) return false;

    std::vector<SelectionSingleton::SelObj> sels;
    if(infos.size()>1) {
        for(auto &info : infos) {
            if(!info.vpd) continue;
            SelectionSingleton::SelObj sel;
            sel.pObject = info.vpd->getObject();
            sel.pDoc = sel.pObject->getDocument();
            sel.DocName = sel.pDoc->getName();
            sel.FeatName = sel.pObject->getNameInDocument();
            sel.TypeName = sel.pObject->getTypeId().getName();
            sel.SubName = info.element.c_str();
            const auto &pt = info.pp->getPoint();
            sel.x = pt[0];
            sel.y = pt[1];
            sel.z = pt[2];
            sels.push_back(sel);
        }
    }

    const auto &info = infos[0];
    auto vpd = info.vpd;
    if(!vpd) return false;
    const char *objname = vpd->getObject()->getNameInDocument();
    if(!objname) return false;
    const char *docname = vpd->getObject()->getDocument()->getName();

    bool hasNext = false;
    const SoPickedPoint * pp = info.pp;
    const SoDetail *det = pp->getDetail();
    SoDetail *detNext = 0;
    SoFullPath *pPath = static_cast<SoFullPath*>(pp->getPath());
    const auto &pt = pp->getPoint();
    SoSelectionElementAction::Type type = SoSelectionElementAction::None;
    HighlightModes mymode = (HighlightModes) this->highlightMode.getValue();
    static char buf[513];

    if (ctrlDown) {
        if(Gui::Selection().isSelected(docname,objname,info.element.c_str(),0))
            Gui::Selection().rmvSelection(docname,objname,info.element.c_str(),&sels);
        else {
            bool ok = Gui::Selection().addSelection(docname,objname,
                    info.element.c_str(), pt[0] ,pt[1] ,pt[2], &sels);
            if (ok && mymode == OFF) {
                snprintf(buf,512,"Selected: %s.%s.%s (%g, %g, %g)",
                        docname,objname,info.element.c_str()
                        ,fabs(pt[0])>1e-7?pt[0]:0.0
                        ,fabs(pt[1])>1e-7?pt[1]:0.0
                        ,fabs(pt[2])>1e-7?pt[2]:0.0);

                getMainWindow()->showMessage(QString::fromLatin1(buf));
            }
        }
        return true;
    }

    // Hierarchy acending
    //
    // If the clicked subelement is already selected, check if there is an
    // upper hierarchy, and select that hierarchy instead. 
    //
    // For example, let's suppose PickedInfo above reports
    // 'link.link2.box.Face1', and below Selection().getSelectedElement returns
    // 'link.link2.box.', meaning that 'box' is the current selected hierarchy,
    // and the user is clicking the box again.  So we shall go up one level,
    // and select 'link.link2.'
    //

    std::string subName = info.element;
    std::string objectName = objname;

    const char *subSelected = Gui::Selection().getSelectedElement(
                                vpd->getObject(),subName.c_str());

    FC_TRACE("select " << (subSelected?subSelected:"'null'") << ", " << 
            objectName << ", " << subName);
    std::string newElement;
    if(subSelected) {
        newElement = Data::ComplexGeoData::newElementName(subSelected);
        subSelected = newElement.c_str();
        std::string nextsub;
        const char *next = strrchr(subSelected,'.');
        if(next && next!=subSelected) {
            if(next[1]==0) {
                // The convention of dot separated SubName demands a mandatory
                // ending dot for every object name reference inside SubName.
                // The non-object sub-element, however, must not end with a dot.
                // So, next[1]==0 here means current selection is a whole object
                // selection (because no sub-element), so we shall search
                // upwards for the second last dot, which is the end of the
                // parent name of the current selected object
                for(--next;next!=subSelected;--next) {
                    if(*next == '.') break;
                }
            }
            if(*next == '.')
                nextsub = std::string(subSelected,next-subSelected+1);
        }
        if(nextsub.length() || *subSelected!=0) {
            hasNext = true;
            subName = nextsub;
            detailPath->truncate(0);
            if(vpd->getDetailPath(subName.c_str(),detailPath,true,detNext) && 
               detailPath->getLength()) 
            {
                pPath = detailPath;
                det = detNext;
                FC_TRACE("select next " << objectName << ", " << subName);
            }
        }
    }

#if 0 // ViewProviderDocumentObject now has default implementation of getElementPicked

    // If no next hierarchy is found, do another try on view provider hierarchies, 
    // which is used by geo feature group.
    if(!hasNext) {
        bool found = false;
        auto vps = this->pcDocument->getViewProvidersByPath(pPath);
        for(auto it=vps.begin();it!=vps.end();++it) {
            auto vpdNext = it->first;
            if(Gui::Selection().isSelected(vpdNext->getObject(),"")) {
                found = true;
                continue;
            }
            if(!found || !vpdNext->useNewSelectionModel() || !vpdNext->isSelectable())
                continue;
            hasNext = true;
            vpd = vpdNext;
            det = 0;
            pPath->truncate(it->second+1);
            objectName = vpd->getObject()->getNameInDocument();
            subName = "";
            break;
        }
    }
#endif

    FC_TRACE("clearing selection");
    Gui::Selection().clearSelection();
    FC_TRACE("add selection");
    bool ok = Gui::Selection().addSelection(docname, objectName.c_str() ,subName.c_str(), 
            pt[0] ,pt[1] ,pt[2], &sels);
    if (ok)
        type = hasNext?SoSelectionElementAction::All:SoSelectionElementAction::Append;

    if (mymode == OFF) {
        snprintf(buf,512,"Selected: %s.%s.%s (%g, %g, %g)",
                docname, objectName.c_str() ,subName.c_str()
                ,fabs(pt[0])>1e-7?pt[0]:0.0
                ,fabs(pt[1])>1e-7?pt[1]:0.0
                ,fabs(pt[2])>1e-7?pt[2]:0.0);

        getMainWindow()->showMessage(QString::fromLatin1(buf));
    }

    if (pPath && checkSelectionStyle(type,vpd)) {
        FC_TRACE("applying action");
        SoSelectionElementAction action(type);
        action.setColor(this->colorSelection.getValue());
        action.setElement(det);
        action.apply(pPath);
        FC_TRACE("applied action");
        this->touch();
    }

    if(detNext) delete detNext;
    return true;
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
            auto infos = this->getPickedList(action,true);
            if(infos.size()) 
                setHighlight(infos[0]);
            else {
                setHighlight(PickedInfo());
                if (this->preSelection > 0) {
                    this->preSelection = 0;
                    // touch() makes sure to call GLRenderBelowPath so that the cursor can be updated
                    // because only from there the SoGLWidgetElement delivers the OpenGL window
                    this->touch();
                }
            }
        }
    }
    // mouse press events for (de)selection
    else if (event->isOfType(SoMouseButtonEvent::getClassTypeId()) && 
             selectionMode.getValue() == SoFCUnifiedSelection::ON) {
        const SoMouseButtonEvent* e = static_cast<const SoMouseButtonEvent *>(event);
        if (SoMouseButtonEvent::isButtonReleaseEvent(e,SoMouseButtonEvent::BUTTON1)) {
            // check to see if the mouse is over a geometry...
            auto infos = this->getPickedList(action,!Selection().needPickedList());
            if(setSelection(infos,event->wasCtrlDown()))
                action->setHandled();
        } // mouse release
    }

    inherited::handleEvent(action);
}

bool SoFCUnifiedSelection::checkSelectionStyle(int type, ViewProvider *vp)
{
    if ((type == SoSelectionElementAction::All || type == SoSelectionElementAction::None) &&
        vp->isDerivedFrom(ViewProviderGeometryObject::getClassTypeId()) &&
        static_cast<ViewProviderGeometryObject*>(vp)->SelectionStyle.getValue() == 1)
    {
        bool selected = (type == SoSelectionElementAction::All);
        int numSelected = getNumSelected();
        if (selected) {
            select(vp->getRoot());
        }
        else {
            deselect(vp->getRoot());
        }

        if (numSelected != getNumSelected())
            this->touch();

        if (selected) {
            return false;
        }
    }
    return true;
}

void SoFCUnifiedSelection::GLRenderBelowPath(SoGLRenderAction * action)
{
    inherited::GLRenderBelowPath(action);

    // nothing picked, so restore the arrow cursor if needed
    if (this->preSelection == 0) {
        // this is called when a selection gate forbade to select an object
        // and the user moved the mouse to an empty area
        this->preSelection = -1;
        QtGLWidget* window;
        SoState *state = action->getState();
        SoGLWidgetElement::get(state, window);
        QWidget* parent = window ? window->parentWidget() : 0;
        if (parent) {
            QCursor c = parent->cursor();
            if (c.shape() == Qt::ForbiddenCursor) {
                c.setShape(Qt::ArrowCursor);
                parent->setCursor(c);
            }
        }
    }
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

SoHighlightElementAction::SoHighlightElementAction () : _highlight(false), _det(0)
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

SoSelectionElementAction::SoSelectionElementAction (Type t, bool secondary)
    : _type(t), _det(0), _secondary(secondary)
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

// ---------------------------------------------------------------

SO_ACTION_SOURCE(SoVRMLAction);

void SoVRMLAction::initClass()
{
    SO_ACTION_INIT_CLASS(SoVRMLAction,SoAction);

    SO_ENABLE(SoVRMLAction, SoSwitchElement);

    SO_ACTION_ADD_METHOD(SoNode,nullAction);

    SO_ENABLE(SoVRMLAction, SoCoordinateElement);
    SO_ENABLE(SoVRMLAction, SoMaterialBindingElement);
    SO_ENABLE(SoVRMLAction, SoLazyElement);
    SO_ENABLE(SoVRMLAction, SoShapeStyleElement);

    SO_ACTION_ADD_METHOD(SoCoordinate3,callDoAction);
    SO_ACTION_ADD_METHOD(SoMaterialBinding,callDoAction);
    SO_ACTION_ADD_METHOD(SoMaterial,callDoAction);
    SO_ACTION_ADD_METHOD(SoNormalBinding,callDoAction);
    SO_ACTION_ADD_METHOD(SoGroup,callDoAction);
    SO_ACTION_ADD_METHOD(SoIndexedLineSet,callDoAction);
    SO_ACTION_ADD_METHOD(SoIndexedFaceSet,callDoAction);
    SO_ACTION_ADD_METHOD(SoPointSet,callDoAction);
}

SoVRMLAction::SoVRMLAction() : overrideMode(true)
{
    SO_ACTION_CONSTRUCTOR(SoVRMLAction);
}

SoVRMLAction::~SoVRMLAction()
{
}

void SoVRMLAction::setOverrideMode(SbBool on)
{
    overrideMode = on;
}

SbBool SoVRMLAction::isOverrideMode() const
{
    return overrideMode;
}

void SoVRMLAction::callDoAction(SoAction *action, SoNode *node)
{
    if (node->getTypeId().isDerivedFrom(SoNormalBinding::getClassTypeId()) && action->isOfType(SoVRMLAction::getClassTypeId())) {
        SoVRMLAction* vrmlAction = static_cast<SoVRMLAction*>(action);
        if (vrmlAction->overrideMode) {
            SoNormalBinding* bind = static_cast<SoNormalBinding*>(node);
            vrmlAction->bindList.push_back(bind->value.getValue());
            // this normal binding causes some problems for the part view provider
            // See also #0002222: Number of normals in exported VRML is wrong
            if (bind->value.getValue() == static_cast<int>(SoNormalBinding::PER_VERTEX_INDEXED))
                bind->value = SoNormalBinding::OVERALL;
        }
        else if (!vrmlAction->bindList.empty()) {
            static_cast<SoNormalBinding*>(node)->value = static_cast<SoNormalBinding::Binding>(vrmlAction->bindList.front());
            vrmlAction->bindList.pop_front();
        }
    }

    node->doAction(action);
}

// ---------------------------------------------------------------------------------
bool SoFCSelectionRoot::StackComp::operator()(const Stack &a, const Stack &b) const {
    if(a.size()-a.offset < b.size()-b.offset) 
        return true;
    if(a.size()-a.offset > b.size()-b.offset) 
        return false;
    auto it1=a.rbegin(), end1=a.rend()-a.offset; 
    auto it2=b.rbegin();
    for(;it1!=end1;++it1,++it2) {
        if(*it1 < *it2) 
            return true;
        if(*it1 > *it2) 
            return false;
    }
    return false;
}
// ---------------------------------------------------------------------------------
SoSeparator::CacheEnabled SoFCSeparator::CacheMode = SoSeparator::AUTO;
SO_NODE_SOURCE(SoFCSeparator);

SoFCSeparator::SoFCSeparator(bool trackCacheMode)
    :trackCacheMode(trackCacheMode)
{
    SO_NODE_CONSTRUCTOR(SoFCSeparator);
    if(!trackCacheMode)
        renderCaching = SoSeparator::OFF;
}

void SoFCSeparator::GLRenderBelowPath(SoGLRenderAction * action) {
    if(trackCacheMode && renderCaching.getValue()!=CacheMode)
        renderCaching = CacheMode;
    inherited::GLRenderBelowPath(action);
}

void SoFCSeparator::initClass(void)
{
    SO_NODE_INIT_CLASS(SoFCSeparator,SoSeparator,"FCSeparator");
}

void SoFCSeparator::finish()
{
    atexit_cleanup();
}


// ---------------------------------------------------------------------------------

SoFCSelectionRoot::Stack SoFCSelectionRoot::SelStack;
std::unordered_map<SoAction*,SoFCSelectionRoot::Stack> SoFCSelectionRoot::ActionStacks;
SoFCSelectionRoot::ColorStack SoFCSelectionRoot::SelColorStack;
SoFCSelectionRoot::ColorStack SoFCSelectionRoot::HlColorStack;
SoFCSelectionRoot* SoFCSelectionRoot::ShapeColorNode;

SO_NODE_SOURCE(SoFCSelectionRoot);

SoFCSelectionRoot::SoFCSelectionRoot(bool trackCacheMode)
    :SoFCSeparator(trackCacheMode)
{
    SO_NODE_CONSTRUCTOR(SoFCSelectionRoot);
}

SoFCSelectionRoot::~SoFCSelectionRoot()
{
}

void SoFCSelectionRoot::initClass(void)
{
    SO_NODE_INIT_CLASS(SoFCSelectionRoot,SoFCSeparator,"FCSelectionRoot");
}

void SoFCSelectionRoot::finish()
{
    atexit_cleanup();
}

SoNode *SoFCSelectionRoot::getCurrentRoot(bool front, SoNode *def) {
    if(SelStack.size()) 
        return front?SelStack.front():SelStack.back();
    return def;
}

SoFCSelectionContextBasePtr SoFCSelectionRoot::getNodeContext(
        Stack &stack, SoNode *node, SoFCSelectionContextBasePtr def)
{
    if(stack.empty())
        return def;

    SoFCSelectionRoot *front = stack.front();

    // NOTE: _node is not necssary of type SoFCSelectionRoot, but it is safe
    // here since we only use it as searching key, although it is probably not
    // a best practice.
    stack.front() = static_cast<SoFCSelectionRoot*>(node);

    auto it = front->contextMap.find(stack);
    stack.front() = front;
    if(it!=front->contextMap.end()) 
        return it->second;
    return SoFCSelectionContextBasePtr();
}

SoFCSelectionContextBasePtr 
SoFCSelectionRoot::getNodeContext2(Stack &stack, SoNode *node, SoFCSelectionContextBase::MergeFunc *merge) 
{
    SoFCSelectionContextBasePtr ret;
    if(stack.empty() || stack.back()->contextMap2.empty())
        return ret;

    int status = 0;
    auto *back = stack.back();
    auto &map = back->contextMap2;
    stack.back() = static_cast<SoFCSelectionRoot*>(node);
    for(stack.offset=0;stack.offset<stack.size();++stack.offset) {
        auto it = map.find(stack);
        auto ctx = it!=map.end()?it->second:SoFCSelectionContextBasePtr();
        status = merge(status,ret,ctx,stack.offset==stack.size()-1?0:stack[stack.offset]);
        if(status<0)
            break;
    }
    stack.offset = 0;
    stack.back() = back;
    return ret;
}

std::pair<bool,SoFCSelectionContextBasePtr*> SoFCSelectionRoot::findActionContext(
        SoAction *action, SoNode *_node, bool create, bool erase)
{
    std::pair<bool,SoFCSelectionContextBasePtr*> res(false,0);
    if(action->isOfType(SoSelectionElementAction::getClassTypeId()))
        res.first = static_cast<SoSelectionElementAction*>(action)->isSecondary();

    auto it = ActionStacks.find(action);
    if(it==ActionStacks.end() || it->second.empty())
        return res;

    auto &stack = it->second;
    
    auto node = static_cast<SoFCSelectionRoot*>(_node);

    if(res.first) {
        auto back = stack.back();
        stack.back() = node;
        if(create)
            res.second = &back->contextMap2[stack];
        else {
            auto it = back->contextMap2.find(stack);
            if(it!=back->contextMap2.end()) {
                res.second = &it->second;
                if(erase)
                    back->contextMap2.erase(it);
            }
        }
        stack.back() = back;
    }else{
        auto front = stack.front();
        stack.front() = node;
        if(create) 
            res.second = &front->contextMap[stack];
        else {
            auto it = front->contextMap.find(stack);
            if(it!=front->contextMap.end()) {
                res.second = &it->second;
                if(erase)
                    front->contextMap.erase(it);
            }
        }
        stack.front() = front;
    }
    return res;
}

static std::time_t _CyclicLastReported;

void SoFCSelectionRoot::renderPrivate(SoGLRenderAction * action, bool inPath) {
    if(ViewParams::instance()->getCoinCycleCheck()
            && !SelStack.nodeSet.insert(this).second) 
    {
        std::time_t t = std::time(0);
        if(_CyclicLastReported < t) {
            _CyclicLastReported = t+5;
            FC_ERR("Cyclic scene graph: " << getName());
        }
        return;
    }
    SelStack.push_back(this);
    auto ctx2 = std::static_pointer_cast<SelContext>(getNodeContext2(SelStack,this,SelContext::merge));
    if(!ctx2 || !ctx2->hideAll) {
        auto state = action->getState();
        SelContextPtr ctx = getRenderContext<SelContext>(this);
        bool colorPushed = false;
        if(!ShapeColorNode && overrideColor && 
            !SoOverrideElement::getDiffuseColorOverride(state) &&
            (!ctx || (!ctx->selAll && !ctx->hideAll))) 
        {
            ShapeColorNode = this;
            colorPushed = true;
            state->push();
            auto &packer = ShapeColorNode->shapeColorPacker;
            auto &trans = ShapeColorNode->transOverride;
            auto &color = ShapeColorNode->colorOverride;
            if(!SoOverrideElement::getTransparencyOverride(state) && trans) {
                SoLazyElement::setTransparency(state, ShapeColorNode, 1, &trans, &packer);
                SoOverrideElement::setTransparencyOverride(state,ShapeColorNode,true);
            }
            SoLazyElement::setDiffuse(state, ShapeColorNode, 1, &color, &packer);
            SoOverrideElement::setDiffuseColorOverride(state,ShapeColorNode,true);
            SoMaterialBindingElement::set(state, ShapeColorNode, SoMaterialBindingElement::OVERALL);
            SoOverrideElement::setMaterialBindingOverride(state,ShapeColorNode,true);

            SoTextureEnabledElement::set(state,ShapeColorNode,false);
        }
        if(!ctx) {
            if(inPath)
                SoSeparator::GLRenderInPath(action);
            else
                SoSeparator::GLRenderBelowPath(action);
        } else {
            bool selPushed;
            bool hlPushed;
            if((selPushed = ctx->selAll)) {
                SelColorStack.push_back(ctx->selColor);
                state->push();
                auto &color = SelColorStack.back();
                SoLazyElement::setEmissive(state, &color);
                SoOverrideElement::setEmissiveColorOverride(state,this,true);
                if (SoLazyElement::getLightModel(state) == SoLazyElement::BASE_COLOR) {
                    auto &packer = shapeColorPacker;
                    SoLazyElement::setDiffuse(state, this, 1, &color, &packer);
                    SoOverrideElement::setDiffuseColorOverride(state,this,true);
                    SoMaterialBindingElement::set(state, this, SoMaterialBindingElement::OVERALL);
                    SoOverrideElement::setMaterialBindingOverride(state,this,true);
                }
            }
            if((hlPushed = ctx->hlAll)) 
                HlColorStack.push_back(ctx->hlColor);
            if(inPath)
                SoSeparator::GLRenderInPath(action);
            else
                SoSeparator::GLRenderBelowPath(action);
            if(selPushed) {
                SelColorStack.pop_back();
                state->pop();
            }
            if(hlPushed)
                HlColorStack.pop_back();
        }
        if(colorPushed) {
            ShapeColorNode = 0;
            state->pop();
        }
    }
    SelStack.pop_back();
    SelStack.nodeSet.erase(this);
}

void SoFCSelectionRoot::GLRenderBelowPath(SoGLRenderAction * action) {
    renderPrivate(action,false);
}

void SoFCSelectionRoot::GLRenderInPath(SoGLRenderAction * action) {
    if(action->getCurPathCode() == SoAction::BELOW_PATH)
        return GLRenderBelowPath(action);
    renderPrivate(action,true);
}

bool SoFCSelectionRoot::checkColorOverride(SoState *state) {
    if(ShapeColorNode) {
        if(!SoOverrideElement::getDiffuseColorOverride(state)) {
            state->push();
            auto &packer = ShapeColorNode->shapeColorPacker;
            auto &trans = ShapeColorNode->transOverride;
            auto &color = ShapeColorNode->colorOverride;
            if(!SoOverrideElement::getTransparencyOverride(state) && trans) {
                SoLazyElement::setTransparency(state, ShapeColorNode, 1, &trans, &packer);
                SoOverrideElement::setTransparencyOverride(state,ShapeColorNode,true);
            }
            SoLazyElement::setDiffuse(state, ShapeColorNode, 1, &color, &packer);
            SoOverrideElement::setDiffuseColorOverride(state,ShapeColorNode,true);
            SoMaterialBindingElement::set(state, ShapeColorNode, SoMaterialBindingElement::OVERALL);
            SoOverrideElement::setMaterialBindingOverride(state,ShapeColorNode,true);

            SoTextureEnabledElement::set(state,ShapeColorNode,false);
            return true;
        }
    }
    return false;
}

void SoFCSelectionRoot::checkSelection(bool &sel, SbColor &selColor, bool &hl, SbColor &hlColor) {
    sel = false;
    hl = false;
    if((sel = !SelColorStack.empty()))
        selColor = SelColorStack.back();
    if((hl = !HlColorStack.empty()))
        hlColor = HlColorStack.back();
}

void SoFCSelectionRoot::resetContext() {
    contextMap.clear();
}


#define BEGIN_ACTION \
    auto &stack = ActionStacks[action];\
    if(ViewParams::instance()->getCoinCycleCheck() \
        && !stack.nodeSet.insert(this).second) \
    {\
        std::time_t t = std::time(0);\
        if(_CyclicLastReported < t) {\
            _CyclicLastReported = t+5;\
            FC_ERR("Cyclic scene graph: " << getName());\
        }\
        return;\
    }\
    stack.push_back(this);\
    auto size = stack.size();

#define END_ACTION \
    if(stack.size()!=size || stack.back()!=this)\
        FC_ERR("action stack fault");\
    else {\
        stack.nodeSet.erase(this);\
        stack.pop_back();\
        if(stack.empty())\
            ActionStacks.erase(action);\
    }

void SoFCSelectionRoot::pick(SoPickAction * action) {
    BEGIN_ACTION;
    inherited::pick(action);
    END_ACTION;
}

void SoFCSelectionRoot::rayPick(SoRayPickAction * action) {
    BEGIN_ACTION;
    inherited::rayPick(action);
    END_ACTION;
}

void SoFCSelectionRoot::handleEvent(SoHandleEventAction * action) {
    BEGIN_ACTION;
    inherited::handleEvent(action);
    END_ACTION;
}

void SoFCSelectionRoot::search(SoSearchAction * action) {
    BEGIN_ACTION;
    inherited::search(action);
    END_ACTION;
}

void SoFCSelectionRoot::getPrimitiveCount(SoGetPrimitiveCountAction * action) {
    BEGIN_ACTION;
    inherited::getPrimitiveCount(action);
    END_ACTION;
}

void SoFCSelectionRoot::getBoundingBox(SoGetBoundingBoxAction * action)
{
    BEGIN_ACTION;
    inherited::getBoundingBox(action);
    END_ACTION;
}

void SoFCSelectionRoot::getMatrix(SoGetMatrixAction * action) {
    BEGIN_ACTION;
    inherited::getMatrix(action);
    END_ACTION;
}

void SoFCSelectionRoot::callback(SoCallbackAction *action) {
    BEGIN_ACTION;
    inherited::callback(action);
    END_ACTION;
}

void SoFCSelectionRoot::doAction(SoAction *action) {
    BEGIN_ACTION
    if(doActionPrivate(stack,action))
        inherited::doAction(action);
    END_ACTION
}

bool SoFCSelectionRoot::doActionPrivate(Stack &stack, SoAction *action) {
    // Selection action short-circuit optimization. In case of whole object
    // selection/pre-selection, we shall store a SelContext keyed by ourself.
    // And the action traversal can be short-curcuited once the first targeted
    // SoFCSelectionRoot is found here. New fuction checkSelection() is exposed
    // to check for whole object selection. This greatly imporve performance on
    // large group.

    SelContextPtr ctx2;
    bool ctx2Searched = false;
    bool isTail = false;
    if(action->getCurPathCode()==SoAction::IN_PATH) {
        auto path = action->getPathAppliedTo();
        if(path) {
            isTail = path->getTail()==this || 
                     (path->getLength()>1 
                      && path->getNodeFromTail(1)==this
                      && path->getTail()->isOfType(SoSwitch::getClassTypeId()));
        }

        if(!action->isOfType(SoSelectionElementAction::getClassTypeId())) {
            ctx2Searched = true;
            ctx2 = std::static_pointer_cast<SelContext>(getNodeContext2(stack,this,SelContext::merge));
            if(ctx2 && ctx2->hideAll)
                return false;
        }
        if(!isTail)
            return true;
    }else if(action->getWhatAppliedTo()!=SoAction::NODE && action->getCurPathCode()!=SoAction::BELOW_PATH)
        return true;

    if(action->isOfType(SoSelectionElementAction::getClassTypeId())) {
        auto selAction = static_cast<SoSelectionElementAction*>(action);
        if(selAction->isSecondary()) {
            if(selAction->getType() == SoSelectionElementAction::Show ||
               (selAction->getType() == SoSelectionElementAction::Color && 
                selAction->getColors().empty() &&
                action->getWhatAppliedTo()==SoAction::NODE))
            {
                auto ctx = getActionContext(action,this,SelContextPtr(),false);
                if(ctx && ctx->hideAll) {
                    ctx->hideAll = false;
                    if(!ctx->hlAll && !ctx->selAll)
                        removeActionContext(action,this);
                    touch();
                }
                // applied to a node means clear all visibility setting, so
                // return true to propgate the action
                return selAction->getType()==SoSelectionElementAction::Color ||
                       action->getWhatAppliedTo()==SoAction::NODE;

            }else if(selAction->getType() == SoSelectionElementAction::Hide) {
                if(action->getCurPathCode()==SoAction::BELOW_PATH || isTail) {
                    auto ctx = getActionContext(action,this,SelContextPtr());
                    if(ctx && !ctx->hideAll) {
                        ctx->hideAll = true;
                        touch();
                    }
                    return false;
                }
            }
            return true;
        } 
        
        if(selAction->getType() == SoSelectionElementAction::None) {
            if(action->getWhatAppliedTo() == SoAction::NODE) {
                // Here the 'select none' action is applied to a node, and we
                // are the first SoFCSelectionRoot encounted (which means all
                // children stores selection context here, both whole object
                // and element selection), then we can simply perform the
                // action by clearing the selection context here, and save the
                // time for traversing a potentially large amount of children
                // nodes.
                resetContext();
                touch();
                return false;
            }

            auto ctx = getActionContext(action,this,SelContextPtr(),false);
            if(ctx && ctx->selAll) {
                ctx->selAll = false;
                touch();
                return false;
            }
        } else if(selAction->getType() == SoSelectionElementAction::All) {
            auto ctx = getActionContext(action,this,SelContextPtr());
            assert(ctx);
            ctx->selAll = true;
            ctx->selColor = selAction->getColor();
            touch();
            return false;
        }
        return true;
    }

    if(action->isOfType(SoHighlightElementAction::getClassTypeId())) {
        auto hlAction = static_cast<SoHighlightElementAction*>(action);
        if(hlAction->isHighlighted()) {
            if(hlAction->getElement()) {
                auto ctx = getActionContext(action,this,SelContextPtr(),false);
                if(ctx && ctx->hlAll) {
                    ctx->hlAll = false;
                    touch();
                }
            } else {
                auto ctx = getActionContext(action,this,SelContextPtr());
                assert(ctx);
                ctx->hlAll = true;
                ctx->hlColor = hlAction->getColor();
                touch();
                return false;
            }
        } else {
            auto ctx = getActionContext(action,this,SelContextPtr(),false);
            if(ctx && ctx->hlAll) {
                ctx->hlAll = false;
                touch();
                return false;
            }
        }
        return true;
    }

    if(!ctx2Searched) {
        ctx2 = std::static_pointer_cast<SelContext>(getNodeContext2(stack,this,SelContext::merge));
        if(ctx2 && ctx2->hideAll)
            return false;
    }
    return true;
}

int SoFCSelectionRoot::SelContext::merge(int status, SoFCSelectionContextBasePtr &output, 
        SoFCSelectionContextBasePtr input, SoFCSelectionRoot *)
{
    auto ctx = std::dynamic_pointer_cast<SelContext>(input);
    if(ctx && ctx->hideAll) {
        output = ctx;
        return -1;
    }
    return status;
}

/////////////////////////////////////////////////////////////////////////////

SO_NODE_SOURCE(SoFCPathAnnotation);

SoFCPathAnnotation::SoFCPathAnnotation()
{
    SO_NODE_CONSTRUCTOR(SoFCPathAnnotation);
    path = 0;
    tmpPath = 0;
    det = 0;
}

SoFCPathAnnotation::~SoFCPathAnnotation()
{
    if(path) path->unref();
    if(tmpPath) tmpPath->unref();
    delete det;
}

void SoFCPathAnnotation::finish() 
{
    atexit_cleanup();
}

void SoFCPathAnnotation::initClass(void)
{
    SO_NODE_INIT_CLASS(SoFCPathAnnotation,SoSeparator,"Separator");
}

void SoFCPathAnnotation::GLRender(SoGLRenderAction * action)
{
    switch (action->getCurPathCode()) {
    case SoAction::NO_PATH:
    case SoAction::BELOW_PATH:
        this->GLRenderBelowPath(action);
        break;
    case SoAction::OFF_PATH:
        break;
    case SoAction::IN_PATH:
        this->GLRenderInPath(action);
        break;
    }
}

void SoFCPathAnnotation::GLRenderBelowPath(SoGLRenderAction * action)
{
    if(!path || !path->getLength() || !tmpPath || !tmpPath->getLength())
        return;

    if(path->getLength() != tmpPath->getLength()) {
        // The auditing SoPath may be truncated due to harmless things such as
        // fliping a SoSwitch sibling node. So we keep an unauditing SoTempPath
        // around to try to restore the path.
        for(int i=path->getLength()-1;i<tmpPath->getLength()-1;++i) {
            auto children = path->getNode(i)->getChildren();
            if(children) {
                int idx = children->find(tmpPath->getNode(i+1));
                if(idx >= 0) {
                    path->append(idx);
                    continue;
                }
            }
            tmpPath->unref();
            tmpPath = 0;
            return;
        }
    }

    SoState * state = action->getState();
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DONT_AUTO_CACHE);

    if (action->isRenderingDelayedPaths()) {
        SbBool zbenabled = glIsEnabled(GL_DEPTH_TEST);
        if (zbenabled) glDisable(GL_DEPTH_TEST);
        inherited::GLRenderInPath(action);
        if (zbenabled) glEnable(GL_DEPTH_TEST);
    }
    else {
        SoCacheElement::invalidate(action->getState());
        auto curPath = action->getCurPath();
        SoPath *newPath = new SoPath(curPath->getLength()+path->getLength());
        newPath->append(curPath);
        newPath->append(path);
        action->addDelayedPath(newPath);
    }
}

void SoFCPathAnnotation::GLRenderInPath(SoGLRenderAction * action)
{
    GLRenderBelowPath(action);
}

void SoFCPathAnnotation::setDetail(SoDetail *d) {
    if(d!=det) {
        delete det;
        det = d;
    }
}

void SoFCPathAnnotation::setPath(SoPath *newPath) {
    if(path) {
        path->unref();
        removeAllChildren();
        path = 0;
        if(tmpPath) {
            tmpPath->unref();
            tmpPath = 0;
        }
    }
    if(!newPath || !newPath->getLength())
        return;

    tmpPath = new SoTempPath(newPath->getLength());
    tmpPath->ref();
    for(int i=0;i<newPath->getLength();++i)
        tmpPath->append(newPath->getNode(i));
    path = newPath->copy();
    path->ref();
    addChild(path->getNode(0));
}

