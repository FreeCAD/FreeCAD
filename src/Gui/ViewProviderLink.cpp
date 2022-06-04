/****************************************************************************
 *   Copyright (c) 2017 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <atomic>
# include <cctype>
# include <boost/algorithm/string/predicate.hpp>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/details/SoDetail.h>
# include <Inventor/draggers/SoCenterballDragger.h>
# include <Inventor/misc/SoChildList.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoCube.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoSurroundScale.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/sensors/SoNodeSensor.h>
# include <Inventor/SoPickedPoint.h>
# include <QApplication>
# include <QMenu>
# include <QCheckBox>
#endif

#include <boost/range.hpp>
#include <App/ComplexGeoData.h>
#include <App/Document.h>
#include <Base/BoundBoxPy.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/Tools.h>

#include "MainWindow.h"
#include "ViewProviderLink.h"
#include "ViewProviderLinkPy.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Control.h"
#include "LinkViewPy.h"
#include "Selection.h"
#include "SoFCCSysDragger.h"
#include "SoFCUnifiedSelection.h"
#include "TaskCSysDragger.h"
#include "TaskElementColors.h"
#include "View3DInventor.h"
#include "ViewParams.h"
#include "ViewProviderGeometryObject.h"

#include "ActionFunction.h"
#include "Command.h"
#include "DlgObjectSelection.h"

FC_LOG_LEVEL_INIT("App::Link", true, true)

using namespace Gui;
using namespace Base;

typedef boost::iterator_range<const char*> CharRange;
////////////////////////////////////////////////////////////////////////////

static inline bool appendPathSafe(SoPath *path, SoNode *node) {
    if(path->getLength()) {
        SoNode * tail = path->getTail();
        const SoChildList * children = tail->getChildren();
        if(!children || children->find((void *)node)<0)
            return false;
    }
    path->append(node);
    return true;
}

#ifdef FC_DEBUG
#define appendPath(_path,_node)  \
do{\
    if(!appendPathSafe(_path,_node))\
        FC_ERR("LinkView: coin path error");\
}while(0)
#else
#define appendPath(_path, _node) (_path)->append(_node)
#endif

////////////////////////////////////////////////////////////////////////////
class Gui::LinkInfo {

public:
    std::atomic<int> ref;

    typedef boost::signals2::scoped_connection Connection;
    Connection connChangeIcon;

    ViewProviderDocumentObject *pcLinked;
    std::unordered_set<Gui::LinkOwner*> links;

    typedef LinkInfoPtr Pointer;

    SoNodeSensor sensor;
    SoNodeSensor switchSensor;
    SoNodeSensor childSensor;
    SoNodeSensor transformSensor;

    std::array<CoinPtr<SoSeparator>,LinkView::SnapshotMax> pcSnapshots;
    std::array<CoinPtr<SoSwitch>,LinkView::SnapshotMax> pcSwitches;
    CoinPtr<SoSwitch> pcLinkedSwitch;

    // for group type view providers
    CoinPtr<SoGroup> pcChildGroup;
    typedef std::unordered_map<SoNode *, Pointer> NodeMap;
    NodeMap nodeMap;

    std::map<qint64, QIcon> iconMap;

    static ViewProviderDocumentObject *getView(App::DocumentObject *obj) {
        if(obj && obj->getNameInDocument()) {
            Document *pDoc = Application::Instance->getDocument(obj->getDocument());
            if(pDoc) {
                ViewProvider *vp = pDoc->getViewProvider(obj);
                if(vp && vp->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
                    return static_cast<ViewProviderDocumentObject*>(vp);
            }
        }
        return nullptr;
    }

    static Pointer get(App::DocumentObject *obj, Gui::LinkOwner *owner) {
        return get(getView(obj),owner);
    }

    static Pointer get(ViewProviderDocumentObject *vp, LinkOwner *owner) {
        if(!vp)
            return Pointer();

        auto ext = vp->getExtensionByType<ViewProviderLinkObserver>(true);
        if(!ext) {
            ext = new ViewProviderLinkObserver();
            ext->initExtension(vp);
        }
        if(!ext->linkInfo) {
            // extension can be created automatically when restored from document,
            // with an empty linkInfo. So we need to check here.
            ext->linkInfo = Pointer(new LinkInfo(vp));
            ext->linkInfo->update();
        }
        if(owner)
            ext->linkInfo->links.insert(owner);
        return ext->linkInfo;
    }

    static void sensorCB(void *data, SoSensor *) {
        static_cast<LinkInfo*>(data)->update();
    }

    static void switchSensorCB(void *data, SoSensor *) {
        static_cast<LinkInfo*>(data)->updateSwitch();
    }

    static void childSensorCB(void *data, SoSensor *) {
        static_cast<LinkInfo*>(data)->updateChildren();
    }

    static void transformSensorCB(void *data, SoSensor *) {
        auto self = static_cast<LinkInfo*>(data);
        for(size_t i=0;i<self->pcSnapshots.size();++i)  {
            if(self->pcSnapshots[i] && i!=LinkView::SnapshotTransform)
                self->getSnapshot(i,true);
        }
    }

    LinkInfo(ViewProviderDocumentObject *vp)
        :ref(0),pcLinked(vp)
    {
        FC_LOG("new link to " << pcLinked->getObject()->getFullName());
        connChangeIcon = vp->signalChangeIcon.connect(
                boost::bind(&LinkInfo::slotChangeIcon,this));
        vp->forceUpdate(true);
        sensor.setFunction(sensorCB);
        sensor.setData(this);
        switchSensor.setFunction(switchSensorCB);
        switchSensor.setData(this);
        childSensor.setFunction(childSensorCB);
        childSensor.setData(this);
        transformSensor.setFunction(transformSensorCB);
        transformSensor.setData(this);
    }

    ~LinkInfo() {
    }

    bool checkName(const char *name) const {
        return isLinked() && strcmp(name,getLinkedName())==0;
    }

    void remove(LinkOwner *owner) {
        links.erase(owner);
    }

    bool isLinked() const {
        return pcLinked && pcLinked->getObject() &&
           pcLinked->getObject()->getNameInDocument();
    }

    const char *getLinkedName() const {
        return pcLinked->getObject()->getNameInDocument();
    }

    const char *getLinkedLabel() const {
        return pcLinked->getObject()->Label.getValue();
    }

    const char *getLinkedNameSafe() const {
        if(isLinked())
            return getLinkedName();
        return "<nil>";
    }

    const char *getDocName() const {
        return pcLinked->getDocument()->getDocument()->getName();
    }

    void detach(bool unlink) {
        FC_LOG("link detach " << getLinkedNameSafe());
        auto me = LinkInfoPtr(this);
        if(unlink) {
            while(links.size()) {
                auto link = *links.begin();
                links.erase(links.begin());
                link->unlink(me);
            }
        }
        sensor.detach();
        switchSensor.detach();
        childSensor.detach();
        transformSensor.detach();
        for(const auto &node : pcSnapshots) {
            if(node)
                coinRemoveAllChildren(node);
        }
        for(const auto &node : pcSwitches) {
            if(node)
                coinRemoveAllChildren(node);
        }
        pcLinkedSwitch.reset();
        if(pcChildGroup) {
            coinRemoveAllChildren(pcChildGroup);
            pcChildGroup.reset();
        }
        pcLinked = nullptr;
        connChangeIcon.disconnect();
    }

    void updateSwitch(SoSwitch *node=nullptr) {
        if(!isLinked() || !pcLinkedSwitch)
            return;
        int index = pcLinkedSwitch->whichChild.getValue();
        for(size_t i=0;i<pcSwitches.size();++i) {
            if(!pcSwitches[i] || (node && node!=pcSwitches[i]))
                continue;
            int count = pcSwitches[i]->getNumChildren();
            if((index<0 && i==LinkView::SnapshotChild) || !count)
                pcSwitches[i]->whichChild = -1;
            else if(count>pcLinked->getDefaultMode())
                pcSwitches[i]->whichChild = pcLinked->getDefaultMode();
            else
                pcSwitches[i]->whichChild = 0;
        }
    }

    inline void addref() {
        ++ref;
    }

    inline void release(){
        int r = --ref;
        assert(r>=0);
        if(r==0)
            delete this;
        else if(r==1) {
            if(pcLinked) {
                FC_LOG("link release " << getLinkedNameSafe());
                auto ext = pcLinked->getExtensionByType<ViewProviderLinkObserver>(true);
                if(ext && ext->linkInfo == this) {
                    pcLinked->forceUpdate(false);
                    detach(true);
                    ext->linkInfo.reset();
                }
            }
        }
    }

    // VC2013 has trouble with template argument dependent lookup in
    // namespace. Have to put the below functions in global namespace.
    //
    // However, gcc seems to behave the oppsite, hence the conditional
    // compilation  here.
    //
#ifdef _MSC_VER
    friend void ::intrusive_ptr_add_ref(LinkInfo *px);
    friend void ::intrusive_ptr_release(LinkInfo *px);
#else
    friend inline void intrusive_ptr_add_ref(LinkInfo *px) { px->addref(); }
    friend inline void intrusive_ptr_release(LinkInfo *px) { px->release(); }
#endif

    bool isVisible() const {
        if(!isLinked())
            return true;
        int indices[] = {LinkView::SnapshotTransform, LinkView::SnapshotVisible};
        for(int idx : indices) {
            if(!pcSwitches[idx])
                continue;
            if(pcSwitches[idx]->whichChild.getValue()==-1)
                return false;
        }
        return true;
    }

    void setVisible(bool visible) {
        if(!isLinked())
            return;
        int indices[] = {LinkView::SnapshotTransform, LinkView::SnapshotVisible};
        for(int idx : indices) {
            if(!pcSwitches[idx])
                continue;
            if(!visible)
                pcSwitches[idx]->whichChild = -1;
            else if(pcSwitches[idx]->getNumChildren()>pcLinked->getDefaultMode())
                pcSwitches[idx]->whichChild = pcLinked->getDefaultMode();
        }
    }

    SoSeparator *getSnapshot(int type, bool update=false) {
        if(type<0 || type>=LinkView::SnapshotMax)
            return nullptr;

        SoSeparator *root;
        if(!isLinked() || !(root=pcLinked->getRoot()))
            return nullptr;

        if(sensor.getAttachedNode()!=root) {
            sensor.detach();
            sensor.attach(root);
        }

        auto &pcSnapshot = pcSnapshots[type];
        auto &pcModeSwitch = pcSwitches[type];
        if(pcSnapshot) {
            if(!update)
                return pcSnapshot;
        }else{
            if(ViewParams::instance()->getUseSelectionRoot())
                pcSnapshot = new SoFCSelectionRoot;
            else
                pcSnapshot = new SoSeparator;
            pcSnapshot->boundingBoxCaching = SoSeparator::OFF;
            pcSnapshot->renderCaching = SoSeparator::OFF;
            std::ostringstream ss;
            ss << pcLinked->getObject()->getNameInDocument()
                << "(" << type << ')';
            pcSnapshot->setName(ss.str().c_str());
            pcModeSwitch = new SoSwitch;
        }

        pcLinkedSwitch.reset();

        coinRemoveAllChildren(pcSnapshot);
        pcModeSwitch->whichChild = -1;
        coinRemoveAllChildren(pcModeSwitch);

        SoSwitch *pcUpdateSwitch = pcModeSwitch;

        auto childRoot = pcLinked->getChildRoot();

        for(int i=0,count=root->getNumChildren();i<count;++i) {
            SoNode *node = root->getChild(i);
            if(node==pcLinked->getTransformNode()) {
                if(type!=LinkView::SnapshotTransform)
                    pcSnapshot->addChild(node);
                else {
                    auto transform = pcLinked->getTransformNode();
                    const auto &scale = transform->scaleFactor.getValue();
                    if(scale[0]!=1.0 || scale[1]!=1.0 || scale[2]!=1.0) {
                        SoTransform *trans = new SoTransform;
                        pcSnapshot->addChild(trans);
                        trans->scaleFactor.setValue(scale);
                        trans->scaleOrientation = transform->scaleOrientation;
                        if(transformSensor.getAttachedNode()!=transform) {
                            transformSensor.detach();
                            transformSensor.attach(transform);
                        }
                    }
                }
                continue;
            } else if(node!=pcLinked->getModeSwitch()) {
                pcSnapshot->addChild(node);
                continue;
            }

            pcLinkedSwitch = static_cast<SoSwitch*>(node);
            if(switchSensor.getAttachedNode() != pcLinkedSwitch) {
                switchSensor.detach();
                switchSensor.attach(pcLinkedSwitch);
                pcUpdateSwitch = nullptr;
            }

            pcSnapshot->addChild(pcModeSwitch);
            for(int i=0,count=pcLinkedSwitch->getNumChildren();i<count;++i) {
                auto child = pcLinkedSwitch->getChild(i);
                if(pcChildGroup && child==childRoot)
                    pcModeSwitch->addChild(pcChildGroup);
                else
                    pcModeSwitch->addChild(child);
            }
        }
        updateSwitch(pcUpdateSwitch);
        return pcSnapshot;
    }

    void updateData(const App::Property *prop) {
        LinkInfoPtr me(this);
        for(auto link : links)
            link->onLinkedUpdateData(me,prop);
        // update();
    }

    void update() {
        if(!isLinked() || pcLinked->isRestoring())
            return;

        updateChildren();

        for(size_t i=0;i<pcSnapshots.size();++i)
            if(pcSnapshots[i])
                getSnapshot(i,true);
    }

    void updateChildren() {
        if(!isLinked())
            return;

        if(!pcLinked->getChildRoot()) {
            childSensor.detach();
            if(pcChildGroup)
                coinRemoveAllChildren(pcChildGroup);
            return;
        }

        if(childSensor.getAttachedNode() != pcLinked->getChildRoot()) {
            childSensor.detach();
            childSensor.attach(pcLinked->getChildRoot());
        }
        if(!pcChildGroup)
            pcChildGroup = new SoGroup;
        else
            coinRemoveAllChildren(pcChildGroup);

        NodeMap nodeMap;

        for(auto child : pcLinked->claimChildren3D()) {
            Pointer info = get(child,nullptr);
            if(!info) continue;
            SoNode *node = info->getSnapshot(LinkView::SnapshotChild);
            if(!node) continue;
            nodeMap[node] = info;
            pcChildGroup->addChild(node);
        }

        // Use swap instead of clear() here to avoid potential link
        // destruction
        this->nodeMap.swap(nodeMap);
    }

    bool getElementPicked(bool addname, int type,
            const SoPickedPoint *pp, std::ostream &str) const
    {
        if(!pp || !isLinked() || !pcLinked->isSelectable())
            return false;

        if(addname)
            str << getLinkedName() <<'.';

        auto pcSwitch = pcSwitches[type];
        if(pcChildGroup && pcSwitch && pcSwitch->whichChild.getValue()>=0 &&
            pcSwitch->getChild(pcSwitch->whichChild.getValue())==pcChildGroup)
        {
            SoPath *path = pp->getPath();
            int index = path->findNode(pcChildGroup);
            if(index<=0)
                return false;
            auto it = nodeMap.find(path->getNode(index+1));
            if(it==nodeMap.end())
                return false;
            return it->second->getElementPicked(true,LinkView::SnapshotChild,pp,str);
        }else{
            std::string subname;
            if(!pcLinked->getElementPicked(pp,subname))
                return false;
            str<<subname;
        }
        return true;
    }

    static const char *checkSubname(App::DocumentObject *obj, const char *subname) {
#define CHECK_NAME(_name,_end) do{\
            if(!_name) return 0;\
            const char *_n = _name;\
            for(;*subname && *_n; ++subname,++_n)\
                if(*subname != *_n) break;\
            if(*_n || (*subname!=0 && *subname!=_end))\
                    return 0;\
            if(*subname == _end) ++subname;\
        }while(0)

        // if(subname[0] == '*') {
        //     ++subname;
        //     CHECK_NAME(obj->getDocument()->getName(),'*');
        // }
        CHECK_NAME(obj->getNameInDocument(),'.');
        return subname;
    }

    bool getDetail(bool checkname, int type, const char* subname,
            SoDetail *&det, SoFullPath *path) const
    {
        if(!isLinked())
            return false;

        if(checkname) {
            subname = checkSubname(pcLinked->getObject(),subname);
            if(!subname)
                return false;
        }

        if(pcSnapshots[type]->findChild(pcSwitches[type]) < 0) {
            if(path) {
                if(!appendPathSafe(path,pcSnapshots[type]))
                    return false;
            }
            // this is possible in case of editing, where the switch node
            // of the linked view object is temparaly removed from its root
            return true;
        }
        int len = 0;
        if(path) {
            len = path->getLength();
            if(!appendPathSafe(path,pcSnapshots[type]))
                return false;
            appendPath(path,pcSwitches[type]);
        }
        if(*subname == 0)
            return true;

        auto pcSwitch = pcSwitches[type];
        if(!pcChildGroup || !pcSwitch || pcSwitch->whichChild.getValue()<0 ||
            pcSwitch->getChild(pcSwitch->whichChild.getValue())!=pcChildGroup)
        {
            return pcLinked->getDetailPath(subname,path,false,det);
        }
        if(path){
            appendPath(path,pcChildGroup);
            if(pcLinked->getChildRoot())
                type = LinkView::SnapshotChild;
            else
                type = LinkView::SnapshotVisible;
        }

        // Special handling of nodes with childRoot, especially geo feature
        // group. It's object hierarchy in the tree view (i.e. in subname) is
        // difference from its coin hierarchy. All objects under a geo feature
        // group is visually grouped directly under the group's childRoot,
        // even though some object has secondary hierarchy in subname. E.g.
        //
        // Body
        //   |--Pad
        //       |--Sketch
        //
        //  Both Sketch and Pad's coin nodes are grouped directly under Body as,
        //
        // Body
        //   |--Pad
        //   |--Sketch

        const char *dot = strchr(subname,'.');
        const char *nextsub = subname;
        if(!dot)
            return false;
        auto geoGroup = pcLinked->getObject();
        auto sobj = geoGroup;
        while(1) {
            std::string objname = std::string(nextsub,dot-nextsub+1);
            if(!geoGroup->getSubObject(objname.c_str())) {
                // this object is not found under the geo group, abort.
                break;
            }
            // Object found under geo group, remember this subname
            subname = nextsub;

            auto ssobj = sobj->getSubObject(objname.c_str());
            if(!ssobj) {
                FC_ERR("invalid sub name " << nextsub << " of object " << sobj->getFullName());
                return false;
            }
            sobj = ssobj;
            auto vp = Application::Instance->getViewProvider(sobj);
            if(!vp) {
                FC_ERR("cannot find view provider of " << sobj->getFullName());
                return false;
            }
            if(vp->getChildRoot()) {
                // In case the children is also a geo group, it will visually
                // hold all of its own children, so stop going further down.
                break;
            }
            // new style mapped sub-element
            if(Data::ComplexGeoData::isMappedElement(dot+1))
                break;
            auto next = strchr(dot+1,'.');
            if(!next) {
                // no dot any more, the following must be a sub-element
                break;
            }
            nextsub = dot+1;
            dot = next;
        }

        for(auto v : nodeMap) {
            if(v.second->getDetail(true,type,subname,det,path))
                return true;
        }
        if(path)
            path->truncate(len);
        return false;
    }

    void slotChangeIcon() {
        iconMap.clear();
        if(!isLinked())
            return;
        LinkInfoPtr me(this);
        for(auto link : links)
            link->onLinkedIconChange(me);
    }

    QIcon getIcon(QPixmap px) {
        static int iconSize = -1;
        if(iconSize < 0)
            iconSize = QApplication::style()->standardPixmap(QStyle::SP_DirClosedIcon).width();

        if(!isLinked())
            return QIcon();

        if(px.isNull())
            return pcLinked->getIcon();
        QIcon &iconLink = iconMap[px.cacheKey()];
        if(iconLink.isNull()) {
            QIcon icon = pcLinked->getIcon();
            iconLink = QIcon();
            iconLink.addPixmap(BitmapFactory().merge(icon.pixmap(iconSize, iconSize, QIcon::Normal, QIcon::Off),
                px,BitmapFactoryInst::BottomLeft), QIcon::Normal, QIcon::Off);
            iconLink.addPixmap(BitmapFactory().merge(icon.pixmap(iconSize, iconSize, QIcon::Normal, QIcon::On ),
                px,BitmapFactoryInst::BottomLeft), QIcon::Normal, QIcon::On);
        }
        return iconLink;
    }
};

#ifdef _MSC_VER
void intrusive_ptr_add_ref(Gui::LinkInfo *px){
    px->addref();
}

void intrusive_ptr_release(Gui::LinkInfo *px){
    px->release();
}
#endif

////////////////////////////////////////////////////////////////////////////////////

EXTENSION_TYPESYSTEM_SOURCE(Gui::ViewProviderLinkObserver,Gui::ViewProviderExtension)

ViewProviderLinkObserver::ViewProviderLinkObserver() {
    // TODO: any better way to get deleted automatically?
    m_isPythonExtension = true;
    initExtensionType(ViewProviderLinkObserver::getExtensionClassTypeId());
}

ViewProviderLinkObserver::~ViewProviderLinkObserver() {
    if(linkInfo) {
        linkInfo->detach(true);
        linkInfo.reset();
    }
}

bool ViewProviderLinkObserver::isLinkVisible() const {
    if(linkInfo)
        return linkInfo->isVisible();
    return true;
}

void ViewProviderLinkObserver::setLinkVisible(bool visible) {
    if(linkInfo)
        linkInfo->setVisible(visible);
}

void ViewProviderLinkObserver::extensionBeforeDelete() {
    if(linkInfo)
        linkInfo->detach(false);
}

void ViewProviderLinkObserver::extensionReattach(App::DocumentObject *) {
    if(linkInfo) {
        linkInfo->pcLinked =
            Base::freecad_dynamic_cast<ViewProviderDocumentObject>(getExtendedContainer());
        linkInfo->update();
    }
}

void ViewProviderLinkObserver::extensionOnChanged(const App::Property *prop) {
#if 0
    auto owner = freecad_dynamic_cast<ViewProviderDocumentObject>(getExtendedContainer());
    if(!owner || !linkInfo)
        return;
    if(prop != &owner->Visibility && prop != &owner->DisplayMode)
        linkInfo->update();
#else
    (void)prop;
#endif
}

void ViewProviderLinkObserver::extensionModeSwitchChange() {
    auto owner = freecad_dynamic_cast<ViewProviderDocumentObject>(getExtendedContainer());
    if(owner && linkInfo)
        linkInfo->updateSwitch();
}

void ViewProviderLinkObserver::extensionUpdateData(const App::Property *prop) {
    if(linkInfo && linkInfo->pcLinked && linkInfo->pcLinked->getObject() &&
       prop != &linkInfo->pcLinked->getObject()->Visibility)
        linkInfo->updateData(prop);
}

void ViewProviderLinkObserver::extensionFinishRestoring() {
    if(linkInfo) {
        FC_TRACE("linked finish restoing");
        linkInfo->update();
    }
}

class LinkView::SubInfo : public LinkOwner {
public:
    LinkInfoPtr linkInfo;
    LinkView &handle;
    CoinPtr<SoSeparator> pcNode;
    CoinPtr<SoTransform> pcTransform;
    std::set<std::string> subElements;

    friend LinkView;

    SubInfo(LinkView &handle):handle(handle) {
        pcNode = new SoFCSelectionRoot(true);
        pcTransform = new SoTransform;
        pcNode->addChild(pcTransform);
    }

    ~SubInfo() {
        unlink();
        auto root = handle.getLinkRoot();
        if(root) {
            int idx = root->findChild(pcNode);
            if(idx>=0)
                root->removeChild(idx);
        }
    }

    virtual void onLinkedIconChange(LinkInfoPtr) override {
        if(handle.autoSubLink && handle.subInfo.size()==1)
            handle.onLinkedIconChange(handle.linkInfo);
    }

    virtual void unlink(LinkInfoPtr info=LinkInfoPtr()) override {
        (void)info;
        if(linkInfo) {
            linkInfo->remove(this);
            linkInfo.reset();
        }
        coinRemoveAllChildren(pcNode);
        pcNode->addChild(pcTransform);
    }

    void link(App::DocumentObject *obj) {
        if(isLinked() && linkInfo->pcLinked->getObject()==obj)
            return;
        unlink();
        linkInfo = LinkInfo::get(obj,this);
        if(linkInfo)
            pcNode->addChild(linkInfo->getSnapshot(LinkView::SnapshotTransform));
    }

    bool isLinked() const{
        return linkInfo && linkInfo->isLinked();
    }
};

//////////////////////////////////////////////////////////////////////////////////

class LinkView::Element : public LinkOwner {
public:
    LinkInfoPtr linkInfo;
    LinkView &handle;
    CoinPtr<SoSwitch> pcSwitch;
    CoinPtr<SoFCSelectionRoot> pcRoot;
    CoinPtr<SoTransform> pcTransform;
    int groupIndex = -1;
    bool isGroup = false;

    friend LinkView;

    Element(LinkView &handle):handle(handle) {
        pcTransform = new SoTransform;
        pcRoot = new SoFCSelectionRoot(true);
        pcSwitch = new SoSwitch;
        pcSwitch->addChild(pcRoot);
        pcSwitch->whichChild = 0;
    }

    ~Element() {
        unlink();
        auto root = handle.getLinkRoot();
        if(root) {
            int idx = root->findChild(pcRoot);
            if(idx>=0)
                root->removeChild(idx);
        }
    }

    virtual void unlink(LinkInfoPtr info=LinkInfoPtr()) override{
        (void)info;
        if(linkInfo) {
            linkInfo->remove(this);
            linkInfo.reset();
        }
        coinRemoveAllChildren(pcRoot);
    }

    void link(App::DocumentObject *obj) {
        if(isLinked() && linkInfo->pcLinked->getObject()==obj)
            return;
        unlink();
        linkInfo = LinkInfo::get(obj,this);
        if(isLinked())
            pcRoot->addChild(linkInfo->getSnapshot(handle.childType));
    }

    bool isLinked() const{
        return linkInfo && linkInfo->isLinked();
    }
};

///////////////////////////////////////////////////////////////////////////////////

TYPESYSTEM_SOURCE(Gui::LinkView,Base::BaseClass)

LinkView::LinkView()
    :nodeType(SnapshotTransform)
    ,childType((SnapshotType)-1),autoSubLink(true)
{
    pcLinkRoot = new SoFCSelectionRoot;
}

LinkView::~LinkView() {
    unlink(linkInfo);
    unlink(linkOwner);
}

PyObject *LinkView::getPyObject(void)
{
    if (PythonObject.is(Py::_None()))
        PythonObject = Py::Object(new LinkViewPy(this),true);
    return Py::new_reference_to(PythonObject);
}

void LinkView::setInvalid(void) {
    if (!PythonObject.is(Py::_None())){
        Base::PyObjectBase* obj = (Base::PyObjectBase*)PythonObject.ptr();
        obj->setInvalid();
        obj->DecRef();
    }else
        delete this;
}

Base::BoundBox3d _getBoundBox(ViewProviderDocumentObject *vpd, SoNode *rootNode) {
    auto doc = vpd->getDocument();
    if(!doc)
        LINK_THROW(Base::RuntimeError,"no document");
    Gui::MDIView* view = doc->getViewOfViewProvider(vpd);
    if(!view)
        LINK_THROW(Base::RuntimeError,"no view");

    Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
    SoGetBoundingBoxAction bboxAction(viewer->getSoRenderManager()->getViewportRegion());
    bboxAction.apply(rootNode);
    auto bbox = bboxAction.getBoundingBox();
    float minX,minY,minZ,maxX,maxY,maxZ;
    bbox.getMax().getValue(maxX,maxY,maxZ);
    bbox.getMin().getValue(minX,minY,minZ);
    return Base::BoundBox3d(minX,minY,minZ,maxX,maxY,maxZ);
}

Base::BoundBox3d LinkView::getBoundBox(ViewProviderDocumentObject *vpd) const {
    if(!vpd) {
        if(!linkOwner || !linkOwner->isLinked())
            LINK_THROW(Base::ValueError,"no ViewProvider");
        vpd = linkOwner->pcLinked;
    }
    return _getBoundBox(vpd,pcLinkRoot);
}

ViewProviderDocumentObject *LinkView::getOwner() const {
    if(linkOwner && linkOwner->isLinked())
        return linkOwner->pcLinked;
    return nullptr;
}

void LinkView::setOwner(ViewProviderDocumentObject *vpd) {
    unlink(linkOwner);
    linkOwner = LinkInfo::get(vpd,this);
}

bool LinkView::isLinked() const{
    return linkInfo && linkInfo->isLinked();
}

void LinkView::setDrawStyle(int style, double lineWidth, double pointSize) {
    if(!pcDrawStyle) {
        if(!style)
            return;
        pcDrawStyle = new SoDrawStyle;
        pcDrawStyle->style = SoDrawStyle::FILLED;
        pcLinkRoot->insertChild(pcDrawStyle,0);
    }
    if(!style) {
        pcDrawStyle->setOverride(false);
        return;
    }
    pcDrawStyle->lineWidth = lineWidth;
    pcDrawStyle->pointSize = pointSize;
    switch(style) {
    case 2:
        pcDrawStyle->linePattern = 0xf00f;
        break;
    case 3:
        pcDrawStyle->linePattern = 0x0f0f;
        break;
    case 4:
        pcDrawStyle->linePattern = 0xff88;
        break;
    default:
        pcDrawStyle->linePattern = 0xffff;
    }
    pcDrawStyle->setOverride(true);
}

void LinkView::renderDoubleSide(bool enable) {
    if(enable) {
        if(!pcShapeHints) {
            pcShapeHints = new SoShapeHints;
            pcShapeHints->vertexOrdering = SoShapeHints::CLOCKWISE;
            pcShapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
            pcLinkRoot->insertChild(pcShapeHints,0);
        }
        pcShapeHints->setOverride(true);
    }else if(pcShapeHints)
        pcShapeHints->setOverride(false);
}

void LinkView::setMaterial(int index, const App::Material *material) {
    if(index < 0) {
        if(!material) {
            pcLinkRoot->removeColorOverride();
            return;
        }
        App::Color c = material->diffuseColor;
        c.a = material->transparency;
        pcLinkRoot->setColorOverride(c);
        for(int i=0;i<getSize();++i)
            setMaterial(i,nullptr);
    }else if(index >= (int)nodeArray.size())
        LINK_THROW(Base::ValueError,"LinkView: material index out of range");
    else {
        auto &info = *nodeArray[index];
        if(!material) {
            info.pcRoot->removeColorOverride();
            return;
        }
        App::Color c = material->diffuseColor;
        c.a = material->transparency;
        info.pcRoot->setColorOverride(c);
    }
}

void LinkView::setLink(App::DocumentObject *obj, const std::vector<std::string> &subs) {
    setLinkViewObject(Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
            Application::Instance->getViewProvider(obj)),subs);

}

void LinkView::setLinkViewObject(ViewProviderDocumentObject *vpd,
        const std::vector<std::string> &subs)
{
    if(!isLinked() || linkInfo->pcLinked != vpd) {
        unlink(linkInfo);
        linkInfo = LinkInfo::get(vpd,this);
        if(!linkInfo)
            return;
    }
    subInfo.clear();
    for(const auto &sub : subs) {
        if(sub.empty()) continue;
        const char *subelement = Data::ComplexGeoData::findElementName(sub.c_str());
        std::string subname = sub.substr(0,subelement-sub.c_str());
        auto it = subInfo.find(subname);
        if(it == subInfo.end()) {
            it = subInfo.insert(std::make_pair(subname,std::unique_ptr<SubInfo>())).first;
            it->second.reset(new SubInfo(*this));
        }
        if(subelement[0])
            it->second->subElements.insert(subelement);
    }
    updateLink();
}

void LinkView::setTransform(SoTransform *pcTransform, const Base::Matrix4D &mat) {
#if 1
    double dMtrx[16];
    mat.getGLMatrix(dMtrx);
    pcTransform->setMatrix(SbMatrix(dMtrx[0], dMtrx[1], dMtrx[2],  dMtrx[3],
                                    dMtrx[4], dMtrx[5], dMtrx[6],  dMtrx[7],
                                    dMtrx[8], dMtrx[9], dMtrx[10], dMtrx[11],
                                    dMtrx[12],dMtrx[13],dMtrx[14], dMtrx[15]));
#else
    // extract scale factor from column vector length
    double sx = Base::Vector3d(mat[0][0],mat[1][0],mat[2][0]).Sqr();
    double sy = Base::Vector3d(mat[0][1],mat[1][1],mat[2][1]).Sqr();
    double sz = Base::Vector3d(mat[0][2],mat[1][2],mat[2][2]).Sqr();
    bool bx,by,bz;
    if((bx=fabs(sx-1.0)>=1e-10))
        sx = sqrt(sx);
    else
        sx = 1.0;
    if((by=fabs(sy-1.0)>=1e-10))
        sy = sqrt(sy);
    else
        sy = 1.0;
    if((bz=fabs(sz-1.0)>=1e-10))
        sz = sqrt(sz);
    else
        sz = 1.0;
    // TODO: how to deal with negative scale?
    pcTransform->scaleFactor.setValue(sx,sy,sz);

    Base::Matrix4D matRotate;
    if(bx) {
        matRotate[0][0] = mat[0][0]/sx;
        matRotate[1][0] = mat[1][0]/sx;
        matRotate[2][0] = mat[2][0]/sx;
    }else{
        matRotate[0][0] = mat[0][0];
        matRotate[1][0] = mat[1][0];
        matRotate[2][0] = mat[2][0];
    }
    if(by) {
        matRotate[0][1] = mat[0][1]/sy;
        matRotate[1][1] = mat[1][1]/sy;
        matRotate[2][1] = mat[2][1]/sy;
    }else{
        matRotate[0][1] = mat[0][1];
        matRotate[1][1] = mat[1][1];
        matRotate[2][1] = mat[2][1];
    }
    if(bz) {
        matRotate[0][2] = mat[0][2]/sz;
        matRotate[1][2] = mat[1][2]/sz;
        matRotate[2][2] = mat[2][2]/sz;
    }else{
        matRotate[0][2] = mat[0][2];
        matRotate[1][2] = mat[1][2];
        matRotate[2][2] = mat[2][2];
    }

    Base::Rotation rot(matRotate);
    pcTransform->rotation.setValue(rot[0],rot[1],rot[2],rot[3]);
    pcTransform->translation.setValue(mat[0][3],mat[1][3],mat[2][3]);
    pcTransform->center.setValue(0.0f,0.0f,0.0f);
#endif
}

void LinkView::setSize(int _size) {
    size_t size = _size<0?0:(size_t)_size;
    if(childType<0 && size==nodeArray.size())
        return;
    resetRoot();
    if(!size || childType>=0) {
        nodeArray.clear();
        nodeMap.clear();
        if(!size && childType<0) {
            if(pcLinkedRoot)
                pcLinkRoot->addChild(pcLinkedRoot);
            return;
        }
        childType = SnapshotContainer;
    }
    if(size<nodeArray.size()) {
        for(size_t i=size;i<nodeArray.size();++i)
            nodeMap.erase(nodeArray[i]->pcSwitch);
        nodeArray.resize(size);
    }
    for(const auto &info : nodeArray)
        pcLinkRoot->addChild(info->pcSwitch);

    while(nodeArray.size()<size) {
        nodeArray.push_back(std::unique_ptr<Element>(new Element(*this)));
        auto &info = *nodeArray.back();
        info.pcRoot->addChild(info.pcTransform);
        if(pcLinkedRoot)
            info.pcRoot->addChild(pcLinkedRoot);
        pcLinkRoot->addChild(info.pcSwitch);
        nodeMap.emplace(info.pcSwitch,(int)nodeArray.size()-1);
    }
}

void LinkView::resetRoot() {
    coinRemoveAllChildren(pcLinkRoot);
    if(pcTransform)
        pcLinkRoot->addChild(pcTransform);
    if(pcShapeHints)
        pcLinkRoot->addChild(pcShapeHints);
    if(pcDrawStyle)
        pcLinkRoot->addChild(pcDrawStyle);
}

void LinkView::setChildren(const std::vector<App::DocumentObject*> &children,
        const boost::dynamic_bitset<> &vis, SnapshotType type)
{
    if(children.empty()) {
        if(nodeArray.size()) {
            nodeArray.clear();
            nodeMap.clear();
            childType = SnapshotContainer;
            resetRoot();
            if(pcLinkedRoot)
                pcLinkRoot->addChild(pcLinkedRoot);
        }
        return;
    }

    if(type<0 || type>=SnapshotMax)
        LINK_THROW(Base::ValueError,"invalid children type");

    resetRoot();

    if(childType<0)
        nodeArray.clear();
    childType = type;

    if(nodeArray.size() > children.size())
        nodeArray.resize(children.size());
    else
        nodeArray.reserve(children.size());

    std::map<App::DocumentObject*, size_t> groups;
    for(size_t i=0;i<children.size();++i) {
        auto obj = children[i];
        if(nodeArray.size()<=i)
            nodeArray.push_back(std::unique_ptr<Element>(new Element(*this)));
        auto &info = *nodeArray[i];
        info.isGroup = false;
        info.groupIndex = -1;
        info.pcSwitch->whichChild = (vis.size()<=i||vis[i])?0:-1;
        info.link(obj);
        if(obj->hasExtension(App::GroupExtension::getExtensionClassTypeId(),false)) {
            info.isGroup = true;
            coinRemoveAllChildren(info.pcRoot);
            groups.emplace(obj,i);
        }
    }
    nodeMap.clear();
    for(size_t i=0;i<nodeArray.size();++i) {
        auto &info = *nodeArray[i];
        nodeMap.emplace(info.pcSwitch,i);
        if(info.isLinked() && groups.size()) {
            auto iter = groups.find(App::GroupExtension::getGroupOfObject(
                            info.linkInfo->pcLinked->getObject()));
            if(iter != groups.end()) {
                info.groupIndex = iter->second;
                auto &groupInfo = *nodeArray[iter->second];
                groupInfo.pcRoot->addChild(info.pcSwitch);
                continue;
            }
        }
        pcLinkRoot->addChild(info.pcSwitch);
    }
}

std::vector<ViewProviderDocumentObject*> LinkView::getChildren() const {
    std::vector<ViewProviderDocumentObject*> ret;
    for(const auto &info : nodeArray) {
        if(info->isLinked())
            ret.push_back(info->linkInfo->pcLinked);
    }
    return ret;
}

void LinkView::setTransform(int index, const Base::Matrix4D &mat) {
    if(index<0) {
        if(!pcTransform) {
            pcTransform = new SoTransform;
            pcLinkRoot->insertChild(pcTransform,0);
        }
        setTransform(pcTransform,mat);
        return;
    }
    if(index<0 || index>=(int)nodeArray.size())
        LINK_THROW(Base::ValueError,"LinkView: index out of range");
    setTransform(nodeArray[index]->pcTransform,mat);
}

void LinkView::setElementVisible(int idx, bool visible) {
    if(idx>=0 && idx<(int)nodeArray.size())
        nodeArray[idx]->pcSwitch->whichChild = visible?0:-1;
}

bool LinkView::isElementVisible(int idx) const {
    if(idx>=0 && idx<(int)nodeArray.size())
        return nodeArray[idx]->pcSwitch->whichChild.getValue()>=0;
    return false;
}

ViewProviderDocumentObject *LinkView::getLinkedView() const {
    auto link = linkInfo;
    if(autoSubLink && subInfo.size()==1)
        link = subInfo.begin()->second->linkInfo;
    return link?link->pcLinked:nullptr;
}

std::vector<std::string> LinkView::getSubNames() const {
    std::vector<std::string> ret;
    for(const auto &v : subInfo) {
        if(v.second->subElements.empty()) {
            ret.push_back(v.first);
            continue;
        }
        for(const auto &s : v.second->subElements)
            ret.push_back(v.first+s);
    }
    return ret;
}

void LinkView::setNodeType(SnapshotType type, bool sublink) {
    autoSubLink = sublink;
    if(nodeType==type)
        return;
    if(type>=SnapshotMax ||
       (type<0 && type!=SnapshotContainer && type!=SnapshotContainerTransform))
        LINK_THROW(Base::ValueError,"LinkView: invalid node type");

    if(nodeType>=0 && type<0) {
        if(pcLinkedRoot) {
            SoSelectionElementAction action(SoSelectionElementAction::None,true);
            action.apply(pcLinkedRoot);
        }
        replaceLinkedRoot(CoinPtr<SoSeparator>(new SoFCSelectionRoot));
    }else if(nodeType<0 && type>=0) {
        if(isLinked())
            replaceLinkedRoot(linkInfo->getSnapshot(type));
        else
            replaceLinkedRoot(nullptr);
    }
    nodeType = type;
    updateLink();
}

void LinkView::replaceLinkedRoot(SoSeparator *root) {
    if(root==pcLinkedRoot)
        return;
    if(nodeArray.empty()) {
        if(pcLinkedRoot && root)
            pcLinkRoot->replaceChild(pcLinkedRoot,root);
        else if(root)
            pcLinkRoot->addChild(root);
        else
            resetRoot();
    }else if(childType<0) {
        if(pcLinkedRoot && root) {
            for(const auto &info : nodeArray)
                info->pcRoot->replaceChild(pcLinkedRoot,root);
        }else if(root) {
            for(const auto &info : nodeArray)
                info->pcRoot->addChild(root);
        }else{
            for(const auto &info : nodeArray)
                info->pcRoot->removeChild(pcLinkedRoot);
        }
    }
    pcLinkedRoot = root;
}

void LinkView::onLinkedIconChange(LinkInfoPtr info) {
    if(info==linkInfo && info!=linkOwner && linkOwner && linkOwner->isLinked())
        linkOwner->pcLinked->signalChangeIcon();
}

void LinkView::onLinkedUpdateData(LinkInfoPtr info, const App::Property *prop) {
    if(info!=linkInfo || !linkOwner || !linkOwner->isLinked() || info==linkOwner)
        return;
    auto ext = linkOwner->pcLinked->getObject()->getExtensionByType<App::LinkBaseExtension>(true);
    if (ext && !(prop->getType() & App::Prop_Output) &&
            !prop->testStatus(App::Property::Output))
    {
        // propagate the signalChangedObject to potentially multiple levels
        // of links, to inform tree view of children change, and other
        // parent objects about the change. But we need to be careful to not
        // touch the object if the property of change is marked as output.
        ext->_LinkTouched.touch();
    }else{
        // In case the owner object does not have link extension, here is a
        // trick to link the signalChangedObject from linked object to the
        // owner
        linkOwner->pcLinked->getDocument()->signalChangedObject(
                *linkOwner->pcLinked,linkOwner->pcLinked->getObject()->Label);
    }
}

void LinkView::updateLink() {
    if(!isLinked())
        return;

    if(linkOwner && linkOwner->isLinked() && linkOwner->pcLinked->isRestoring()) {
        FC_TRACE("restoring '" << linkOwner->pcLinked->getObject()->getFullName() << "'");
        return;
    }

    // TODO: is it a good idea to clear any selection here?
    pcLinkRoot->resetContext();

    if(nodeType >= 0) {
        replaceLinkedRoot(linkInfo->getSnapshot(nodeType));
        return;
    }

    // rebuild link sub objects tree
    CoinPtr<SoSeparator> linkedRoot = pcLinkedRoot;
    if(!linkedRoot)
        linkedRoot = new SoFCSelectionRoot;
    else{
        SoSelectionElementAction action(SoSelectionElementAction::None,true);
        action.apply(linkedRoot);
        coinRemoveAllChildren(linkedRoot);
    }

    SoTempPath path(10);
    path.ref();
    appendPath(&path,linkedRoot);
    auto obj = linkInfo->pcLinked->getObject();
    for(const auto &v : subInfo) {
        auto &sub = *v.second;
        Base::Matrix4D mat;
        App::DocumentObject *sobj = obj->getSubObject(
                v.first.c_str(), nullptr, &mat, nodeType==SnapshotContainer);
        if(!sobj) {
            sub.unlink();
            continue;
        }
        sub.link(sobj);
        linkedRoot->addChild(sub.pcNode);
        setTransform(sub.pcTransform,mat);

        if(sub.subElements.size()) {
            path.truncate(1);
            appendPath(&path,sub.pcNode);
            SoSelectionElementAction action(SoSelectionElementAction::Append,true);
            for(const auto &subelement : sub.subElements) {
                path.truncate(2);
                SoDetail *det = nullptr;
                if(!sub.linkInfo->getDetail(false,SnapshotTransform,subelement.c_str(),det,&path))
                    continue;
                action.setElement(det);
                action.apply(&path);
                delete det;
            }
        }
    }
    path.unrefNoDelete();
    replaceLinkedRoot(linkedRoot);
}

bool LinkView::linkGetElementPicked(const SoPickedPoint *pp, std::string &subname) const
{
    std::ostringstream ss;
    CoinPtr<SoPath> path = pp->getPath();
    if(nodeArray.size()) {
        auto idx = path->findNode(pcLinkRoot);
        if(idx<0 || idx+2>=path->getLength())
            return false;
        auto node = path->getNode(idx+1);
        auto it = nodeMap.find(node);
        if(it == nodeMap.end() || !isElementVisible(it->second))
            return false;
        int nodeIdx = it->second;
        ++idx;
        while(nodeArray[nodeIdx]->isGroup) {
            auto &info = *nodeArray[nodeIdx];
            if(!info.isLinked())
                return false;
            ss << info.linkInfo->getLinkedName() << '.';
            idx += 2;
            if(idx>=path->getLength())
                return false;
            auto iter = nodeMap.find(path->getNode(idx));
            if(iter == nodeMap.end() || !isElementVisible(iter->second))
                return false;
            nodeIdx = iter->second;
        }
        auto &info = *nodeArray[nodeIdx];
        if(nodeIdx == it->second)
            ss << it->second << '.';
        else
            ss << info.linkInfo->getLinkedName() << '.';
        if(info.isLinked()) {
            if(!info.linkInfo->getElementPicked(false,childType,pp,ss))
                return false;
            subname = ss.str();
            return true;
        }
    }

    if(!isLinked())
        return false;

    if(nodeType >= 0) {
        if(linkInfo->getElementPicked(false,nodeType,pp,ss)) {
            subname = ss.str();
            return true;
        }
        return false;
    }
    auto idx = path->findNode(pcLinkedRoot);
    if(idx<0 || idx+1>=path->getLength())
        return false;
    auto node = path->getNode(idx+1);
    for(const auto &v : subInfo) {
        auto &sub = *v.second;
        if(node != sub.pcNode) continue;
        std::ostringstream ss2;
        if(!sub.linkInfo->getElementPicked(false,SnapshotTransform,pp,ss2))
            return false;
        const std::string &element = ss2.str();
        if(sub.subElements.size()) {
            if(sub.subElements.find(element)==sub.subElements.end()) {
                auto pos = element.find('.');
                if(pos==std::string::npos ||
                   sub.subElements.find(element.c_str()+pos+1)==sub.subElements.end())
                    return false;
            }
        }
        if(!autoSubLink || subInfo.size()>1)
            ss << v.first;
        ss << element;
        subname = ss.str();
        return true;
    }
    return false;
}

bool LinkView::getGroupHierarchy(int index, SoFullPath *path) const {
    if(index > (int)nodeArray.size())
        return false;
    auto &info = *nodeArray[index];
    if(info.groupIndex>=0 && !getGroupHierarchy(info.groupIndex,path))
        return false;
    appendPath(path,info.pcSwitch);
    appendPath(path,info.pcRoot);
    return true;
}

bool LinkView::linkGetDetailPath(const char *subname, SoFullPath *path, SoDetail *&det) const
{
    if(!subname || *subname==0)
        return true;
    auto len = path->getLength();
    if(nodeArray.empty()) {
        if(!appendPathSafe(path,pcLinkRoot))
            return false;
    } else {
        int idx = -1;
        if (subname[0]>='0' && subname[0]<='9') {
            idx = App::LinkBaseExtension::getArrayIndex(subname,&subname);
        } else {
            while(1) {
                const char *dot = strchr(subname,'.');
                if(!dot)
                    break;
                int i = 0;
                if (subname[0] == '$') {
                    CharRange name(subname+1,dot);
                    for(const auto &info : nodeArray) {
                        if(info->isLinked() && boost::equals(name,info->linkInfo->getLinkedLabel())) {
                            idx = i;
                            break;
                        }
                        ++i;
                    }
                } else {
                    CharRange name(subname,dot);
                    for(const auto &info : nodeArray) {
                        if(info->isLinked() && boost::equals(name,info->linkInfo->getLinkedName())) {
                            idx = i;
                            break;
                        }
                        ++i;
                    }
                }

                if(idx<0)
                    return false;

                subname = dot+1;
                if(!subname[0] || nodeArray[idx]->isGroup==0)
                    break;
                idx = -1;
            }
        }

        if(idx<0 || idx>=(int)nodeArray.size())
            return false;
        auto &info = *nodeArray[idx];
        if(!appendPathSafe(path,pcLinkRoot))
            return false;
        if(info.groupIndex>=0 && !getGroupHierarchy(info.groupIndex,path))
            return false;
        appendPath(path,info.pcSwitch);
        appendPath(path,info.pcRoot);

        if(*subname == 0)
            return true;

        if(info.isLinked()) {
            info.linkInfo->getDetail(false,childType,subname,det,path);
            return true;
        }
    }
    if(isLinked()) {
        if(nodeType >= 0) {
            if(linkInfo->getDetail(false,nodeType,subname,det,path))
                return true;
        }else {
            appendPath(path,pcLinkedRoot);
            for(const auto &v : subInfo) {
                auto &sub = *v.second;
                if(!sub.isLinked())
                    continue;
                const char *nextsub;
                if(autoSubLink && subInfo.size()==1)
                    nextsub = subname;
                else{
                    if(!boost::algorithm::starts_with(subname,v.first))
                        continue;
                    nextsub = subname+v.first.size();
                    if(*nextsub != '.')
                        continue;
                    ++nextsub;
                }
                if(*nextsub && sub.subElements.size() &&
                   sub.subElements.find(nextsub)==sub.subElements.end())
                    break;
                appendPath(path,sub.pcNode);
                len = path->getLength();
                if(sub.linkInfo->getDetail(false,SnapshotTransform,nextsub,det,path))
                    return true;
                break;
            }
        }
    }
    path->truncate(len);
    return false;
}

void LinkView::unlink(LinkInfoPtr info) {
    if(!info)
        return;
    if(info == linkOwner) {
        linkOwner->remove(this);
        linkOwner.reset();
    }
    if(info != linkInfo)
        return;
    if(linkInfo) {
        linkInfo->remove(this);
        linkInfo.reset();
    }
    pcLinkRoot->resetContext();
    if(pcLinkedRoot) {
        if(nodeArray.empty())
            resetRoot();
        else {
            for(const auto &info : nodeArray) {
                int idx;
                if(info->isLinked() &&
                   (idx=info->pcRoot->findChild(pcLinkedRoot))>=0)
                    info->pcRoot->removeChild(idx);
            }
        }
        pcLinkedRoot.reset();
    }
    subInfo.clear();
    return;
}

QIcon LinkView::getLinkedIcon(QPixmap px) const {
    auto link = linkInfo;
    if(autoSubLink && subInfo.size()==1)
        link = subInfo.begin()->second->linkInfo;
    if(!link || !link->isLinked())
        return QIcon();
    return link->getIcon(px);
}

bool LinkView::hasSubs() const {
    return isLinked() && subInfo.size();
}

///////////////////////////////////////////////////////////////////////////////////

PROPERTY_SOURCE(Gui::ViewProviderLink, Gui::ViewProviderDocumentObject)

static const char *_LinkIcon = "Link";
// static const char *_LinkArrayIcon = "LinkArray";
static const char *_LinkGroupIcon = "LinkGroup";
static const char *_LinkElementIcon = "LinkElement";

ViewProviderLink::ViewProviderLink()
    :linkType(LinkTypeNone),hasSubName(false),hasSubElement(false)
    ,useCenterballDragger(false),childVp(nullptr),overlayCacheKey(0)
{
    sPixmap = _LinkIcon;

    ADD_PROPERTY_TYPE(Selectable, (true), " Link", App::Prop_None, 0);

    ADD_PROPERTY_TYPE(OverrideMaterial, (false), " Link", App::Prop_None, "Override linked object's material");

    App::Material mat(App::Material::DEFAULT);
    mat.diffuseColor.setPackedValue(ViewParams::instance()->getDefaultLinkColor());
    ADD_PROPERTY_TYPE(ShapeMaterial, (mat), " Link", App::Prop_None, 0);
    ShapeMaterial.setStatus(App::Property::MaterialEdit, true);

    ADD_PROPERTY_TYPE(DrawStyle,((long int)0), " Link", App::Prop_None, "");
    static const char* DrawStyleEnums[]= {"None","Solid","Dashed","Dotted","Dashdot",nullptr};
    DrawStyle.setEnums(DrawStyleEnums);

    int lwidth = ViewParams::instance()->getDefaultShapeLineWidth();
    ADD_PROPERTY_TYPE(LineWidth,(lwidth), " Link", App::Prop_None, "");

    static App::PropertyFloatConstraint::Constraints sizeRange = {1.0,64.0,1.0};
    LineWidth.setConstraints(&sizeRange);

    ADD_PROPERTY_TYPE(PointSize,(lwidth), " Link", App::Prop_None, "");
    PointSize.setConstraints(&sizeRange);

    ADD_PROPERTY(MaterialList,());
    MaterialList.setStatus(App::Property::NoMaterialListEdit, true);

    ADD_PROPERTY(OverrideMaterialList,());
    ADD_PROPERTY(OverrideColorList,());

    ADD_PROPERTY(ChildViewProvider, (""));
    ChildViewProvider.setStatus(App::Property::Hidden,true);

    DisplayMode.setStatus(App::Property::Status::Hidden, true);

    linkView = new LinkView;
}

ViewProviderLink::~ViewProviderLink()
{
    linkView->setInvalid();
}

bool ViewProviderLink::isSelectable() const {
    return !pcDragger && Selectable.getValue();
}

void ViewProviderLink::attach(App::DocumentObject *pcObj) {
    SoNode *node = linkView->getLinkRoot();
    node->setName(pcObj->getFullName().c_str());
    addDisplayMaskMode(node,"Link");
    if(childVp) {
        childVpLink = LinkInfo::get(childVp,nullptr);
        node = childVpLink->getSnapshot(LinkView::SnapshotTransform);
    }
    addDisplayMaskMode(node,"ChildView");
    setDisplayMaskMode("Link");
    inherited::attach(pcObj);
    checkIcon();
    if(pcObj->isDerivedFrom(App::LinkElement::getClassTypeId()))
        hide();
    linkView->setOwner(this);

}

void ViewProviderLink::reattach(App::DocumentObject *obj) {
    linkView->setOwner(this);
    if(childVp)
        childVp->reattach(obj);
    ViewProviderDocumentObject::reattach(obj);
}

std::vector<std::string> ViewProviderLink::getDisplayModes(void) const
{
    std::vector<std::string> StrList = inherited::getDisplayModes();
    StrList.push_back("Link");
    StrList.push_back("ChildView");
    return StrList;
}

QIcon ViewProviderLink::getIcon() const {
    auto ext = getLinkExtension();
    if(ext) {
        auto link = ext->getLinkedObjectValue();
        if(link && link!=getObject()) {
            QPixmap overlay = getOverlayPixmap();
            overlayCacheKey = overlay.cacheKey();
            QIcon icon = linkView->getLinkedIcon(overlay);
            if(!icon.isNull())
                return icon;
        }
    }
    overlayCacheKey = 0;
    return Gui::BitmapFactory().pixmap(sPixmap);
}

QPixmap ViewProviderLink::getOverlayPixmap() const {
    auto ext = getLinkExtension();
    int px = 12 * getMainWindow()->devicePixelRatioF();
    if(ext && ext->getLinkedObjectProperty() && ext->_getElementCountValue())
        return BitmapFactory().pixmapFromSvg("LinkArrayOverlay", QSizeF(px,px));
    else if(hasSubElement)
        return BitmapFactory().pixmapFromSvg("LinkSubElement", QSizeF(px,px));
    else if(hasSubName)
        return BitmapFactory().pixmapFromSvg("LinkSubOverlay", QSizeF(px,px));
    else
        return BitmapFactory().pixmapFromSvg("LinkOverlay", QSizeF(px,px));
}

void ViewProviderLink::onChanged(const App::Property* prop) {
    if(prop==&ChildViewProvider) {
        childVp = freecad_dynamic_cast<ViewProviderDocumentObject>(ChildViewProvider.getObject().get());
        if(childVp && getObject()) {
            if(strcmp(childVp->getTypeId().getName(),getObject()->getViewProviderName())!=0
                    && !childVp->allowOverride(*getObject()))
            {
                FC_ERR("Child view provider type '" << childVp->getTypeId().getName()
                        << "' does not support " << getObject()->getFullName());
            } else {
                childVp->setPropertyPrefix("ChildViewProvider.");
                childVp->Visibility.setValue(getObject()->Visibility.getValue());
                childVp->attach(getObject());
                childVp->updateView();
                childVp->setActiveMode();
                if(pcModeSwitch->getNumChildren()>1){
                    childVpLink = LinkInfo::get(childVp,nullptr);
                    pcModeSwitch->replaceChild(1,childVpLink->getSnapshot(LinkView::SnapshotTransform));
                }
            }
        }
    }else if(!isRestoring()) {
        if (prop == &OverrideMaterial || prop == &ShapeMaterial ||
            prop == &MaterialList || prop == &OverrideMaterialList)
        {
            applyMaterial();
        }else if(prop == &OverrideColorList) {
            applyColors();
        }else if(prop==&DrawStyle || prop==&PointSize || prop==&LineWidth) {
            if(!DrawStyle.getValue())
                linkView->setDrawStyle(0);
            else
                linkView->setDrawStyle(DrawStyle.getValue(),LineWidth.getValue(),PointSize.getValue());
        }
    }

    inherited::onChanged(prop);
}

bool ViewProviderLink::setLinkType(App::LinkBaseExtension *ext) {
    auto propLink = ext->getLinkedObjectProperty();
    if(!propLink)
        return false;
    LinkType type;
    if(hasSubName)
        type = LinkTypeSubs;
    else
        type = LinkTypeNormal;
    if(linkType != type)
        linkType = type;
    switch(type) {
    case LinkTypeSubs:
        linkView->setNodeType(ext->linkTransform()?LinkView::SnapshotContainer:
                LinkView::SnapshotContainerTransform);
        break;
    case LinkTypeNormal:
        linkView->setNodeType(ext->linkTransform()?LinkView::SnapshotVisible:
                LinkView::SnapshotTransform);
        break;
    default:
        break;
    }
    return true;
}

App::LinkBaseExtension *ViewProviderLink::getLinkExtension() {
    if(!pcObject || !pcObject->getNameInDocument())
        return nullptr;
    return pcObject->getExtensionByType<App::LinkBaseExtension>(true);
}

const App::LinkBaseExtension *ViewProviderLink::getLinkExtension() const{
    if(!pcObject || !pcObject->getNameInDocument())
        return nullptr;
    return const_cast<App::DocumentObject*>(pcObject)->getExtensionByType<App::LinkBaseExtension>(true);
}

void ViewProviderLink::updateData(const App::Property *prop) {
    if(childVp)
        childVp->updateData(prop);
    if(!isRestoring() && !pcObject->isRestoring()) {
        auto ext = getLinkExtension();
        if(ext) updateDataPrivate(getLinkExtension(),prop);
    }
    return inherited::updateData(prop);
}

static inline bool canScale(const Base::Vector3d &v) {
    return fabs(v.x)>1e-7 && fabs(v.y)>1e-7 && fabs(v.z)>1e-7;
}

void ViewProviderLink::updateDataPrivate(App::LinkBaseExtension *ext, const App::Property *prop) {
    if(!prop)
        return;
    if(prop == &ext->_ChildCache) {
        updateElementList(ext);
    } else if(prop == &ext->_LinkTouched) {
        if(linkView->hasSubs())
            linkView->updateLink();
        applyColors();
        checkIcon(ext);
    }else if(prop==ext->getColoredElementsProperty()) {
        if(!prop->testStatus(App::Property::User3))
            applyColors();
    }else if(prop==ext->getScaleProperty() || prop==ext->getScaleVectorProperty()) {
        if(!prop->testStatus(App::Property::User3)) {
            const auto &v = ext->getScaleVector();
            if(canScale(v))
                pcTransform->scaleFactor.setValue(v.x,v.y,v.z);
            SbMatrix matrix = convert(ext->getTransform(false));
            linkView->renderDoubleSide(matrix.det3() < 0);
        }
    }else if(prop == ext->getPlacementProperty() || prop == ext->getLinkPlacementProperty()) {
        auto propLinkPlacement = ext->getLinkPlacementProperty();
        if(!propLinkPlacement || propLinkPlacement == prop) {
            const auto &pla = static_cast<const App::PropertyPlacement*>(prop)->getValue();
            ViewProviderGeometryObject::updateTransform(pla, pcTransform);
            const auto &v = ext->getScaleVector();
            if(canScale(v))
                pcTransform->scaleFactor.setValue(v.x,v.y,v.z);
            SbMatrix matrix = convert(ext->getTransform(false));
            linkView->renderDoubleSide(matrix.det3() < 0);
        }
    }else if(prop == ext->getLinkCopyOnChangeGroupProperty()) {
        if (auto group = ext->getLinkCopyOnChangeGroupValue()) {
            auto vp = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                    Application::Instance->getViewProvider(group));
            if (vp) {
                vp->hide();
                vp->ShowInTree.setValue(false);
            }
        }
    }else if(prop == ext->getLinkedObjectProperty()) {

        if(!prop->testStatus(App::Property::User3)) {
            std::vector<std::string> subs;
            const char *subname = ext->getSubName();
            std::string sub;
            if(subname)
                sub = subname;
            hasSubElement = false;
            for(const auto &s : ext->getSubElements()) {
                if(s.empty()) continue;
                hasSubElement = true;
                subs.push_back(sub+s);
            }

            if(subs.empty() && sub.size())
                subs.push_back(sub);

            hasSubName = !subs.empty();
            setLinkType(ext);

            auto obj = ext->getLinkedObjectValue();
            linkView->setLink(obj,subs);

            if(ext->_getShowElementValue())
                updateElementList(ext);
            else
                updateDataPrivate(ext,ext->_getElementCountProperty());

            // applyColors();
            signalChangeIcon();
        }
    }else if(prop == ext->getLinkTransformProperty()) {
        setLinkType(ext);
        applyColors();
    }else if(prop==ext->_getElementCountProperty()) {
        if(!ext->_getShowElementValue()) {
            linkView->setSize(ext->_getElementCountValue());
            updateDataPrivate(ext,ext->getVisibilityListProperty());
            updateDataPrivate(ext,ext->getPlacementListProperty());
        }
    }else if(prop == ext->_getShowElementProperty()) {
        if(!ext->_getShowElementValue()) {

            auto linked = freecad_dynamic_cast<ViewProviderDocumentObject>(getLinkedView(true,ext));
            if(linked && linked->getDocument()==getDocument())
                linked->hide();

            const auto &elements = ext->_getElementListValue();
            // elements is about to be collapsed, preserve the materials
            if(elements.size()) {
                std::vector<App::Material> materials;
                boost::dynamic_bitset<> overrideMaterials;
                overrideMaterials.resize(elements.size(),false);
                bool overrideMaterial = false;
                bool hasMaterial = false;
                materials.reserve(elements.size());
                for(size_t i=0;i<elements.size();++i) {
                    auto element = freecad_dynamic_cast<App::LinkElement>(elements[i]);
                    if(!element) continue;
                    auto vp = freecad_dynamic_cast<ViewProviderLink>(
                            Application::Instance->getViewProvider(element));
                    if(!vp) continue;
                    overrideMaterial = overrideMaterial || vp->OverrideMaterial.getValue();
                    hasMaterial = overrideMaterial || hasMaterial
                        || vp->ShapeMaterial.getValue()!=ShapeMaterial.getValue();
                    materials.push_back(vp->ShapeMaterial.getValue());
                    overrideMaterials[i] = vp->OverrideMaterial.getValue();
                }
                if(!overrideMaterial)
                    overrideMaterials.clear();
                OverrideMaterialList.setStatus(App::Property::User3,true);
                OverrideMaterialList.setValue(overrideMaterials);
                OverrideMaterialList.setStatus(App::Property::User3,false);
                if(!hasMaterial)
                    materials.clear();
                MaterialList.setStatus(App::Property::User3,true);
                MaterialList.setValue(materials);
                MaterialList.setStatus(App::Property::User3,false);

                linkView->setSize(ext->_getElementCountValue());
                updateDataPrivate(ext,ext->getVisibilityListProperty());
                applyMaterial();
                applyColors();
            }
        }
    }else if(prop==ext->getScaleListProperty() || prop==ext->getPlacementListProperty()) {
        if(!prop->testStatus(App::Property::User3) &&
            linkView->getSize() &&
            !ext->_getShowElementValue())
        {
            auto propPlacements = ext->getPlacementListProperty();
            auto propScales = ext->getScaleListProperty();
            if(propPlacements && linkView->getSize()) {
                const auto &touched =
                    prop==propScales?propScales->getTouchList():propPlacements->getTouchList();
                if(touched.empty()) {
                    for(int i=0;i<linkView->getSize();++i) {
                        Base::Matrix4D mat;
                        if(propPlacements && propPlacements->getSize()>i)
                            mat = (*propPlacements)[i].toMatrix();
                        if(propScales && propScales->getSize()>i && canScale((*propScales)[i])) {
                            Base::Matrix4D s;
                            s.scale((*propScales)[i]);
                            mat *= s;
                        }
                        linkView->setTransform(i,mat);
                    }
                }else{
                    for(int i : touched) {
                        if(i<0 || i>=linkView->getSize())
                            continue;
                        Base::Matrix4D mat;
                        if(propPlacements && propPlacements->getSize()>i)
                            mat = (*propPlacements)[i].toMatrix();
                        if(propScales && propScales->getSize()>i && canScale((*propScales)[i])) {
                            Base::Matrix4D s;
                            s.scale((*propScales)[i]);
                            mat *= s;
                        }
                        linkView->setTransform(i,mat);
                    }
                }
            }
        }
    }else if(prop == ext->getVisibilityListProperty()) {
        const auto &vis = ext->getVisibilityListValue();
        for(size_t i=0;i<(size_t)linkView->getSize();++i) {
            if(vis.size()>i)
                linkView->setElementVisible(i,vis[i]);
            else
                linkView->setElementVisible(i,true);
        }
    }else if(prop == ext->_getElementListProperty()) {
        if(ext->_getShowElementValue())
            updateElementList(ext);
    }
}

void ViewProviderLink::updateElementList(App::LinkBaseExtension *ext) {
    const auto &elements = ext->_getElementListValue();
    if(OverrideMaterialList.getSize() || MaterialList.getSize()) {
        int i=-1;
        for(auto obj : elements) {
            ++i;
            auto vp = freecad_dynamic_cast<ViewProviderLink>(
                    Application::Instance->getViewProvider(obj));
            if(!vp) continue;
            if(OverrideMaterialList.getSize()>i)
                vp->OverrideMaterial.setValue(OverrideMaterialList[i]);
            if(MaterialList.getSize()>i)
                vp->ShapeMaterial.setValue(MaterialList[i]);
        }
        OverrideMaterialList.setSize(0);
        MaterialList.setSize(0);
    }
    linkView->setChildren(elements, ext->getVisibilityListValue());
    applyColors();
}

void ViewProviderLink::checkIcon(const App::LinkBaseExtension *ext) {
    if(!ext) {
        ext = getLinkExtension();
        if(!ext)
            return;
    }
    const char *icon;
    auto element = freecad_dynamic_cast<App::LinkElement>(getObject());
    if(element)
        icon = _LinkElementIcon;
    else if(!ext->getLinkedObjectProperty() && ext->getElementListProperty())
        icon = _LinkGroupIcon;
    // else if(ext->_getElementCountValue())
    //     icon = _LinkArrayIcon;
    else
        icon = _LinkIcon;
    qint64 cacheKey = 0;
    if(getObject()->getLinkedObject(false)!=getObject())
        cacheKey = getOverlayPixmap().cacheKey();
    if(icon!=sPixmap || cacheKey!=overlayCacheKey) {
        sPixmap = icon;
        signalChangeIcon();
    }
}

void ViewProviderLink::applyMaterial() {
    if(OverrideMaterial.getValue())
        linkView->setMaterial(-1,&ShapeMaterial.getValue());
    else {
        for(int i=0;i<linkView->getSize();++i) {
            if(MaterialList.getSize()>i &&
               OverrideMaterialList.getSize()>i && OverrideMaterialList[i])
                linkView->setMaterial(i,&MaterialList[i]);
            else
                linkView->setMaterial(i,nullptr);
        }
        linkView->setMaterial(-1,nullptr);
    }
}

void ViewProviderLink::finishRestoring() {
    FC_TRACE("finish restoring");
    auto ext = getLinkExtension();
    if(!ext)
        return;
    linkView->setDrawStyle(DrawStyle.getValue(),LineWidth.getValue(),PointSize.getValue());
    updateDataPrivate(ext,ext->getLinkedObjectProperty());
    if(ext->getLinkPlacementProperty())
        updateDataPrivate(ext,ext->getLinkPlacementProperty());
    else
        updateDataPrivate(ext,ext->getPlacementProperty());
    updateDataPrivate(ext,ext->_getElementCountProperty());
    if(ext->getPlacementListProperty())
        updateDataPrivate(ext,ext->getPlacementListProperty());
    else
        updateDataPrivate(ext,ext->getScaleListProperty());
    updateDataPrivate(ext,ext->_getElementListProperty());
    applyMaterial();
    applyColors();

    // TODO: notify the tree. This is ugly, any other way?
    getDocument()->signalChangedObject(*this,ext->_LinkTouched);

    if(childVp)
        childVp->finishRestoring();
}

bool ViewProviderLink::hasElements(const App::LinkBaseExtension *ext) const {
    if(!ext) {
        ext = getLinkExtension();
        if(!ext)
            return false;
    }
    const auto &elements = ext->getElementListValue();
    return elements.size() && (int)elements.size()==ext->_getElementCountValue();
}

bool ViewProviderLink::isGroup(const App::LinkBaseExtension *ext, bool plainGroup) const {
    if(!ext) {
        ext = getLinkExtension();
        if(!ext)
            return false;
    }
    return (plainGroup && ext->linkedPlainGroup())
        || (ext->getElementListProperty() && !ext->getLinkedObjectProperty());
}

ViewProvider *ViewProviderLink::getLinkedView(
        bool real,const App::LinkBaseExtension *ext) const
{
    if(!ext)
        ext = getLinkExtension();
    auto obj = ext&&real?ext->getTrueLinkedObject(true):
        getObject()->getLinkedObject(true);
    if(obj && obj!=getObject())
        return Application::Instance->getViewProvider(obj);
    return nullptr;
}

std::vector<App::DocumentObject*> ViewProviderLink::claimChildren(void) const {
    auto ext = getLinkExtension();
    std::vector<App::DocumentObject*> ret;

    if(ext && !ext->_getShowElementValue() && ext->_getElementCountValue()) {
        // in array mode without element objects, we'd better not show the
        // linked object's children to avoid inconsistent behavior on selection.
        // We claim the linked object instead
        if(ext) {
            auto obj = ext->getLinkedObjectValue();
            if(obj) ret.push_back(obj);
        }
    } else if(hasElements(ext) || isGroup(ext)) {
        ret = ext->getElementListValue();
        if (ext->_getElementCountValue() 
                && ext->getLinkClaimChildValue()
                && ext->getLinkedObjectValue())
            ret.insert(ret.begin(), ext->getLinkedObjectValue());
    } else if(!hasSubName) {
        auto linked = getLinkedView(true);
        if(linked) {
            ret = linked->claimChildren();
            if (ext->getLinkClaimChildValue() && ext->getLinkedObjectValue())
                ret.insert(ret.begin(), ext->getLinkedObjectValue());
        }
    }
    if (ext && ext->getLinkCopyOnChangeGroupValue())
        ret.insert(ret.begin(), ext->getLinkCopyOnChangeGroupValue());
    return ret;
}

bool ViewProviderLink::canDragObject(App::DocumentObject* obj) const {
    auto ext = getLinkExtension();
    if(isGroup(ext))
        return true;
    if(hasElements(ext))
        return false;
    auto linked = getLinkedView(false,ext);
    if(linked)
        return linked->canDragObject(obj);
    return false;
}

bool ViewProviderLink::canDragObjects() const {
    auto ext = getLinkExtension();
    if(isGroup(ext))
        return true;
    if(hasElements(ext))
        return false;
    auto linked = getLinkedView(false,ext);
    if(linked)
        return linked->canDragObjects();
    return false;
}

void ViewProviderLink::dragObject(App::DocumentObject* obj) {
    auto ext = getLinkExtension();
    if(isGroup(ext)) {
        const auto &objs = ext->getElementListValue();
        for(size_t i=0;i<objs.size();++i) {
            if(obj==objs[i]) {
                ext->setLink(i,nullptr);
                break;
            }
        }
        return;
    }
    if(hasElements(ext))
        return;
    auto linked = getLinkedView(false);
    if(linked)
        linked->dragObject(obj);
}

bool ViewProviderLink::canDropObjects() const {
    auto ext = getLinkExtension();
    if(isGroup(ext))
        return true;
    if(hasElements(ext))
        return false;
    if(hasSubElement)
        return true;
    else if(hasSubName)
        return false;
    auto linked = getLinkedView(false,ext);
    if(linked)
        return linked->canDropObjects();
    return true;
}

bool ViewProviderLink::canDropObjectEx(App::DocumentObject *obj,
        App::DocumentObject *owner, const char *subname,
        const std::vector<std::string> &subElements) const
{
    if(pcObject == obj || pcObject == owner)
        return false;
    auto ext = getLinkExtension();
    if(isGroup(ext))
        return true;
    if(!ext || !ext->getLinkedObjectProperty() || hasElements(ext))
        return false;
    if(!hasSubName && linkView->isLinked()) {
        auto linked = getLinkedView(false,ext);
        if(linked) {
            auto linkedVdp = freecad_dynamic_cast<ViewProviderDocumentObject>(linked);
            if(linkedVdp) {
                if(linkedVdp->getObject()==obj || linkedVdp->getObject()==owner)
                    return false;
            }
            return linked->canDropObjectEx(obj,owner,subname,subElements);
        }
    }
    if(obj->getDocument() != getObject()->getDocument() &&
       !freecad_dynamic_cast<App::PropertyXLink>(ext->getLinkedObjectProperty()))
        return false;

    return true;
}

std::string ViewProviderLink::dropObjectEx(App::DocumentObject* obj,
    App::DocumentObject *owner, const char *subname,
    const std::vector<std::string> &subElements)
{
    auto ext = getLinkExtension();
    if (!ext)
        return std::string();

    if(isGroup(ext)) {
        size_t size = ext->getElementListValue().size();
        ext->setLink(size,obj);
        return std::to_string(size)+".";
    }

    if(!ext->getLinkedObjectProperty() || hasElements(ext))
        return std::string();

    if(!hasSubName) {
        auto linked = getLinkedView(false,ext);
        if(linked)
            return linked->dropObjectEx(obj,owner,subname,subElements);
    }
    if(owner) {
        if(ext->getSubElements().size())
            ext->setLink(-1,owner,subname,subElements);
        else
            ext->setLink(-1,owner,subname);
    } else if(ext->getSubElements().size())
        ext->setLink(-1,obj,nullptr,subElements);
    else
        ext->setLink(-1,obj,nullptr);
    return std::string();
}

bool ViewProviderLink::canDragAndDropObject(App::DocumentObject* obj) const {
    auto ext = getLinkExtension();
    if(!ext)
        return true;
    if(isGroup(ext)) {
        return ext->getLinkModeValue()<App::LinkBaseExtension::LinkModeAutoLink &&
               obj->getDocument()==getObject()->getDocument();
    }
    if(!ext->getLinkedObjectProperty() || hasElements(ext))
        return false;
    if(!hasSubName) {
        auto linked = getLinkedView(false,ext);
        if(linked)
            return linked->canDragAndDropObject(obj);
    }
    return false;
}

bool ViewProviderLink::getElementPicked(const SoPickedPoint *pp, std::string &subname) const {
    if(!isSelectable())
        return false;
    auto ext = getLinkExtension();
    if(!ext)
        return false;
    if(childVpLink && childVp) {
        auto path = pp->getPath();
        int idx = path->findNode(childVpLink->getSnapshot(LinkView::SnapshotTransform));
        if(idx>=0)
            return childVp->getElementPicked(pp,subname);
    }
    bool ret = linkView->linkGetElementPicked(pp,subname);
    if(!ret)
        return ret;
    if(isGroup(ext,true)) {
        const char *sub = nullptr;
        int idx = App::LinkBaseExtension::getArrayIndex(subname.c_str(),&sub);
        if(idx>=0 ) {
            --sub;
            assert(*sub == '.');
            const auto &elements = ext->_getElementListValue();
            subname.replace(0,sub-subname.c_str(),elements[idx]->getNameInDocument());
        }
    }
    return ret;
}

bool ViewProviderLink::getDetailPath(
        const char *subname, SoFullPath *pPath, bool append, SoDetail *&det) const
{
    auto ext = getLinkExtension();
    if(!ext)
        return false;

    auto len = pPath->getLength();
    if(append) {
        appendPath(pPath,pcRoot);
        appendPath(pPath,pcModeSwitch);
    }
    if(childVpLink && getDefaultMode()==1) {
        if(childVpLink->getDetail(false,LinkView::SnapshotTransform,subname,det,pPath))
            return true;
        pPath->truncate(len);
        return false;
    }
    std::string _subname;
    if(subname && subname[0]) {
        if (auto linked = ext->getLinkedObjectValue()) {
            if (const char *dot = strchr(subname,'.')) {
                if(subname[0]=='$') {
                    CharRange sub(subname+1, dot);
                    if (!boost::equals(sub, linked->Label.getValue()))
                        dot = nullptr;
                } else {
                    CharRange sub(subname, dot);
                    if (!boost::equals(sub, linked->getNameInDocument()))
                        dot = nullptr;
                }
                if (dot && linked->getSubObject(dot+1))
                    subname = dot+1;
            }
        }

        if (isGroup(ext,true) || hasElements(ext) || ext->getElementCountValue()) {
            int index = ext->getElementIndex(subname,&subname);
            if(index>=0) {
                _subname = std::to_string(index)+'.'+subname;
                subname = _subname.c_str();
            }
        }
    }
    if(linkView->linkGetDetailPath(subname,pPath,det))
        return true;
    pPath->truncate(len);
    return false;
}

bool ViewProviderLink::onDelete(const std::vector<std::string> &) {
    auto element = freecad_dynamic_cast<App::LinkElement>(getObject());
    if (element && !element->canDelete())
        return false;
    auto ext = getLinkExtension();
    if (ext->isLinkMutated()) {
        auto linked = ext->getLinkedObjectValue();
        auto doc = ext->getContainer()->getDocument();
        if (linked->getDocument() == doc) {
            std::deque<std::string> objs;
            for (auto obj : ext->getOnChangeCopyObjects(nullptr, linked)) {
                if (obj->getDocument() == doc) {
                    // getOnChangeCopyObjects() returns object in depending
                    // order. So we delete it in reverse to avoid error
                    // reported by some parent object failing to find child
                    objs.emplace_front(obj->getNameInDocument());
                }
            }
            for (const auto &name : objs)
                doc->removeObject(name.c_str());
        }
    }
    return true;
}

bool ViewProviderLink::canDelete(App::DocumentObject *obj) const {
    auto ext = getLinkExtension();
    if(isGroup(ext) || hasElements(ext) || hasSubElement)
        return true;
    auto linked = getLinkedView(false,ext);
    if(linked)
        return linked->canDelete(obj);
    return false;
}

bool ViewProviderLink::linkEdit(const App::LinkBaseExtension *ext) const {
    if(!ext)
        ext = getLinkExtension();
    if(!ext ||
       (!ext->_getShowElementValue() && ext->_getElementCountValue()) ||
       hasElements(ext) ||
       isGroup(ext) ||
       hasSubName)
    {
        return false;
    }
    return linkView->isLinked();
}

bool ViewProviderLink::doubleClicked() {
    if(linkEdit())
        return linkView->getLinkedView()->doubleClicked();
    return getDocument()->setEdit(this,ViewProvider::Transform);
}

void ViewProviderLink::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    auto ext = getLinkExtension();
    if (!ext)
        return;

    _setupContextMenu(ext, menu, receiver, member);
    Gui::ActionFunction* func = nullptr;

    if (ext->isLinkedToConfigurableObject()) {
        auto src = ext->getLinkCopyOnChangeSourceValue();
        if (!src) src = ext->getLinkedObjectValue();
        if (src && !ext->getOnChangeCopyObjects(nullptr, src).empty()) {
            QAction *act = menu->addAction(
                    QObject::tr("Setup configurable object"));
            act->setToolTip(QObject::tr(
                        "Select which object to copy or exclude when configuration changes. "
                        "All external linked objects are excluded by default."));
            act->setData(-1);
            if (!func) func = new Gui::ActionFunction(menu);
            func->trigger(act, [ext](){
                try {
                    std::vector<App::DocumentObject*> excludes;
                    auto src = ext->getLinkCopyOnChangeSourceValue();
                    if (!src)
                        src = ext->getLinkedObjectValue();
                    auto objs = ext->getOnChangeCopyObjects(&excludes, src);
                    if (objs.empty())
                        return;
                    DlgObjectSelection dlg({src}, excludes, getMainWindow());
                    dlg.setMessage(QObject::tr(
                                "Please select which objects to copy when the configuration is changed"));
                    QCheckBox *box = new QCheckBox(QObject::tr("Apply to all"), &dlg);
                    box->setToolTip(QObject::tr("Apply the setting to all links. Or, uncheck this\n"
                                                "option to apply only to this link."));
                    box->setChecked(App::LinkParams::getCopyOnChangeApplyToAll());
                    dlg.addCheckBox(box);
                    if(dlg.exec()!=QDialog::Accepted)
                        return;

                    bool applyAll = box->isChecked();
                    App::LinkParams::setCopyOnChangeApplyToAll(applyAll);

                    App::Link::OnChangeCopyOptions options;
                    if (applyAll)
                        options |= App::Link::OnChangeCopyOptions::ApplyAll;

                    App::AutoTransaction guard("Setup configurable object");
                    auto sels = dlg.getSelections(DlgObjectSelection::SelectionOptions::InvertSort);
                    for (auto it=excludes.begin(); it!=excludes.end(); ++it) {
                        auto iter = std::lower_bound(sels.begin(), sels.end(), *it);
                        if (iter == sels.end() || *iter != *it) {
                            ext->setOnChangeCopyObject(*it, options);
                        } else
                            sels.erase(iter);
                    }
                    options |= App::Link::OnChangeCopyOptions::Exclude;
                    for (auto obj : sels)
                        ext->setOnChangeCopyObject(obj, options);
                    if (!applyAll)
                        ext->monitorOnChangeCopyObjects(ext->getOnChangeCopyObjects());
                    else {
                        std::set<App::LinkBaseExtension*> exts;
                        for (auto o : App::Document::getDependencyList(objs)) {
                            if (auto ext = o->getExtensionByType<App::LinkBaseExtension>(true))
                                exts.insert(ext);
                        }
                        for (auto ext : exts)
                            ext->monitorOnChangeCopyObjects(ext->getOnChangeCopyObjects());
                    }
                    Command::updateActive();
                } catch (Base::Exception &e) {
                    e.ReportException();
                }
            });
        }

        if (ext->getLinkCopyOnChangeValue() == 0) {
            auto submenu = menu->addMenu(QObject::tr("Copy on change"));
            auto act = submenu->addAction(QObject::tr("Enable"));
            act->setToolTip(QObject::tr(
                        "Enable auto copy of linked object when its configuration is changed"));
            act->setData(-1);
            if (!func) func = new Gui::ActionFunction(menu);
            func->trigger(act, [ext](){
                try {
                    App::AutoTransaction guard("Enable Link copy on change");
                    ext->getLinkCopyOnChangeProperty()->setValue(1);
                    Command::updateActive();
                } catch (Base::Exception &e) {
                    e.ReportException();
                }
            });
            act = submenu->addAction(QObject::tr("Tracking"));
            act->setToolTip(QObject::tr(
                        "Copy the linked object when its configuration is changed.\n"
                        "Also auto redo the copy if the original linked object is changed.\n"));
            act->setData(-1);
            func->trigger(act, [ext](){
                try {
                    App::AutoTransaction guard("Enable Link tracking");
                    ext->getLinkCopyOnChangeProperty()->setValue(3);
                    Command::updateActive();
                } catch (Base::Exception &e) {
                    e.ReportException();
                }
            });
        }
    }

    if (ext->getLinkCopyOnChangeValue() != 2
            && ext->getLinkCopyOnChangeValue() != 0) {
        QAction *act = menu->addAction(
                QObject::tr("Disable copy on change"));
        act->setData(-1);
        if (!func) func = new Gui::ActionFunction(menu);
        func->trigger(act, [ext](){
            try {
                App::AutoTransaction guard("Disable copy on change");
                ext->getLinkCopyOnChangeProperty()->setValue((long)0);
                Command::updateActive();
            } catch (Base::Exception &e) {
                e.ReportException();
            }
        });
    }

    if (ext->isLinkMutated()) {
        QAction* act = menu->addAction(QObject::tr("Refresh configurable object"));
        act->setToolTip(QObject::tr(
                    "Synchronize the original configurable source object by\n"
                    "creating a new deep copy. Note that any changes made to\n"
                    "the current copy will be lost.\n"));
        act->setData(-1);
        if (!func) func = new Gui::ActionFunction(menu);
        func->trigger(act, [ext](){
            try {
                App::AutoTransaction guard("Link refresh");
                ext->syncCopyOnChange();
                Command::updateActive();
            } catch (Base::Exception &e) {
                e.ReportException();
            }
        });
    }
}

void ViewProviderLink::_setupContextMenu(
        App::LinkBaseExtension *ext, QMenu* menu, QObject* receiver, const char* member)
{
    if(linkEdit(ext)) {
        if (auto linkvp = Base::freecad_dynamic_cast<ViewProviderLink>(linkView->getLinkedView()))
            linkvp->_setupContextMenu(ext, menu, receiver, member);
        else
            linkView->getLinkedView()->setupContextMenu(menu,receiver,member);
    }

    if(ext->getLinkedObjectProperty()
            && ext->_getShowElementProperty()
            && ext->_getElementCountValue() > 1)
    {
        auto action = menu->addAction(QObject::tr("Toggle array elements"), [ext] {
            try {
                App::AutoTransaction guard(QT_TRANSLATE_NOOP("Command", "Toggle array elements"));
                ext->getShowElementProperty()->setValue(!ext->getShowElementValue());
                Command::updateActive();
            } catch (Base::Exception &e) {
                e.ReportException();
            }
        });
        action->setToolTip(QObject::tr(
                    "Change whether show each link array element as individual objects"));
    }

    if((ext->getPlacementProperty() && !ext->getPlacementProperty()->isReadOnly())
            || (ext->getLinkPlacementProperty() && !ext->getLinkPlacementProperty()->isReadOnly()))
    {
        bool found = false;
        for(auto action : menu->actions()) {
            if(action->data().toInt() == ViewProvider::Transform) {
                found = true;
                break;
            }
        }
        if (!found) {
            QIcon iconObject = mergeGreyableOverlayIcons(Gui::BitmapFactory().pixmap("Std_TransformManip.svg"));
            QAction* act = menu->addAction(iconObject, QObject::tr("Transform"), receiver, member);
            act->setToolTip(QObject::tr("Transform at the origin of the placement"));
            act->setData(QVariant((int)ViewProvider::Transform));
        }
    }

    if(ext->getColoredElementsProperty()) {
        bool found = false;
        for(auto action : menu->actions()) {
            if(action->data().toInt() == ViewProvider::Color) {
                action->setText(QObject::tr("Override colors..."));
                found = true;
                break;
            }
        }
        if(!found) {
            QAction* act = menu->addAction(QObject::tr("Override colors..."), receiver, member);
            act->setData(QVariant((int)ViewProvider::Color));
        }
    }
}

bool ViewProviderLink::initDraggingPlacement() {
    Base::PyGILStateLocker lock;
    try {
        auto* proxy = getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            const char *fname = "initDraggingPlacement";
            if (feature.hasAttr(fname)) {
                Py::Callable method(feature.getAttr(fname));
                Py::Tuple arg;
                Py::Object ret(method.apply(arg));
                if(ret.isTuple()) {
                    PyObject *pymat,*pypla,*pybbox;
                    if(!PyArg_ParseTuple(ret.ptr(),"O!O!O!",&Base::MatrixPy::Type, &pymat,
                                &Base::PlacementPy::Type, &pypla,
                                &Base::BoundBoxPy::Type, &pybbox)) {
                        FC_ERR("initDraggingPlacement() expects return of type tuple(matrix,placement,boundbox)");
                        return false;
                    }
                    dragCtx.reset(new DraggerContext);
                    dragCtx->initialPlacement = *static_cast<Base::PlacementPy*>(pypla)->getPlacementPtr();
                    dragCtx->preTransform = *static_cast<Base::MatrixPy*>(pymat)->getMatrixPtr();
                    dragCtx->bbox = *static_cast<Base::BoundBoxPy*>(pybbox)->getBoundBoxPtr();
                    return true;
                }else if(!ret.isTrue())
                    return false;
            }
        }
    } catch (Py::Exception&) {
        Base::PyException e;
        e.ReportException();
        return false;
    }

    auto ext = getLinkExtension();
    if(!ext) {
        FC_ERR("no link extension");
        return false;
    }
    if(!ext->hasPlacement()) {
        FC_ERR("no placement");
        return false;
    }
    auto doc = Application::Instance->editDocument();
    if(!doc) {
        FC_ERR("no editing document");
        return false;
    }

    dragCtx.reset(new DraggerContext);

    dragCtx->preTransform = doc->getEditingTransform();
    doc->setEditingTransform(dragCtx->preTransform);

    const auto &pla = ext->getPlacementProperty()?
        ext->getPlacementValue():ext->getLinkPlacementValue();

    // Cancel out our own transformation from the editing transform, because
    // the dragger is meant to change our transformation.
    dragCtx->preTransform *= pla.inverse().toMatrix();

    dragCtx->bbox = getBoundingBox(nullptr,false);
    // The returned bounding box is before our own transform, but we still need
    // to scale it to get the correct center.
    auto scale = ext->getScaleVector();
    dragCtx->bbox.ScaleX(scale.x);
    dragCtx->bbox.ScaleY(scale.y);
    dragCtx->bbox.ScaleZ(scale.z);

    auto modifier = QApplication::queryKeyboardModifiers();
    // Determine the dragger base position
    // if CTRL key is down, force to use bound box center,
    // if SHIFT key is down, force to use origine,
    // if not a sub link, use origine,
    // else (e.g. group, array, sub link), use bound box center
    if(modifier != Qt::ShiftModifier
            && ((ext->getLinkedObjectValue() && !linkView->hasSubs())
                || modifier == Qt::ControlModifier))
    {
        App::PropertyPlacement *propPla = nullptr;
        if(ext->getLinkTransformValue() && ext->getLinkedObjectValue()) {
            propPla = Base::freecad_dynamic_cast<App::PropertyPlacement>(
                    ext->getLinkedObjectValue()->getPropertyByName("Placement"));
        }
        if(propPla) {
            dragCtx->initialPlacement = pla * propPla->getValue();
            dragCtx->mat *= propPla->getValue().inverse().toMatrix();
        } else
            dragCtx->initialPlacement = pla;

    } else {
        auto offset = dragCtx->bbox.GetCenter();

        // This determines the initial placement of the dragger. We place it at the
        // center of our bounding box.
        dragCtx->initialPlacement = pla * Base::Placement(offset, Base::Rotation());

        // dragCtx->mat is to transform the dragger placement to our own placement.
        // Since the dragger is placed at the center, we set the transformation by
        // moving the same amount in reverse direction.
        dragCtx->mat.move(Vector3d() - offset);
    }

    return true;
}

ViewProvider *ViewProviderLink::startEditing(int mode) {
    if(mode==ViewProvider::Color) {
        auto ext = getLinkExtension();
        if(!ext || !ext->getColoredElementsProperty()) {
            if(linkEdit(ext))
                return linkView->getLinkedView()->startEditing(mode);
        }
        return inherited::startEditing(mode);
    }

    static thread_local bool _pendingTransform;
    static thread_local Base::Matrix4D  _editingTransform;

    auto doc = Application::Instance->editDocument();

    if(mode==ViewProvider::Transform) {
        if(_pendingTransform && doc)
            doc->setEditingTransform(_editingTransform);

        if(!initDraggingPlacement())
            return nullptr;
        if(useCenterballDragger)
            pcDragger = CoinPtr<SoCenterballDragger>(new SoCenterballDragger);
        else
            pcDragger = CoinPtr<SoFCCSysDragger>(new SoFCCSysDragger);
        updateDraggingPlacement(dragCtx->initialPlacement,true);
        pcDragger->addStartCallback(dragStartCallback, this);
        pcDragger->addFinishCallback(dragFinishCallback, this);
        pcDragger->addMotionCallback(dragMotionCallback, this);
        return inherited::startEditing(mode);
    }

    if(!linkEdit()) {
        FC_ERR("unsupported edit mode " << mode);
        return nullptr;
    }

    // TODO: the 0x8000 mask here is for caller to disambiguate the intention
    // here, whether they want to, say transform the link itself or the linked
    // object. Use of a mask here will allow forwarding those editing modes that
    // are supported by both the link and the linked object, such as transform
    // and set color. We need to find a better place to declare this constant.
    mode &= ~0x8000;

    if(!doc) {
        FC_ERR("no editing document");
        return nullptr;
    }

    // We are forwarding the editing request to linked object. We need to
    // adjust the editing transformation.
    Base::Matrix4D mat;
    auto linked = getObject()->getLinkedObject(true,&mat,false);
    if(!linked || linked==getObject()) {
        FC_ERR("no linked object");
        return nullptr;
    }
    auto vpd = freecad_dynamic_cast<ViewProviderDocumentObject>(
                Application::Instance->getViewProvider(linked));
    if(!vpd) {
        FC_ERR("no linked viewprovider");
        return nullptr;
    }
    // Amend the editing transformation with the link transformation.
    // But save it first in case the linked object reroute the editing request
    // back to us.
    _editingTransform = doc->getEditingTransform();
    doc->setEditingTransform(doc->getEditingTransform()*mat);
    Base::FlagToggler<> guard(_pendingTransform);
    return vpd->startEditing(mode);
}

bool ViewProviderLink::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Color) {
        auto ext = getLinkExtension();
        if(!ext || !ext->getColoredElementsProperty())
            return false;
        TaskView::TaskDialog *dlg = Control().activeDialog();
        if (dlg) {
            Control().showDialog(dlg);
            return false;
        }
        Selection().clearSelection();
        return true;
    }
    return inherited::setEdit(ModNum);
}

void ViewProviderLink::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    if (ModNum == ViewProvider::Color) {
        Gui::Control().showDialog(new TaskElementColors(this));
        return;
    }

    if (pcDragger && viewer)
    {
        SoPickStyle *rootPickStyle = new SoPickStyle();
        rootPickStyle->style = SoPickStyle::UNPICKABLE;
        static_cast<SoFCUnifiedSelection*>(
                viewer->getSceneGraph())->insertChild(rootPickStyle, 0);

        if(useCenterballDragger) {
            auto dragger = static_cast<SoCenterballDragger*>(pcDragger.get());
            SoSeparator *group = new SoAnnotation;
            SoPickStyle *pickStyle = new SoPickStyle;
            pickStyle->setOverride(true);
            group->addChild(pickStyle);
            group->addChild(pcDragger);

            // Because the dragger is not grouped with the actual geometry,
            // we use an invisible cube sized by the bounding box obtained from
            // initDraggingPlacement() to scale the centerball dragger properly

            auto * ss = (SoSurroundScale*)dragger->getPart("surroundScale", TRUE);
            ss->numNodesUpToContainer = 3;
            ss->numNodesUpToReset = 2;

            auto *geoGroup = new SoGroup;
            group->addChild(geoGroup);
            auto *style = new SoDrawStyle;
            style->style.setValue(SoDrawStyle::INVISIBLE);
            style->setOverride(TRUE);
            geoGroup->addChild(style);
            auto *cube = new SoCube;
            geoGroup->addChild(cube);
            auto length = std::max(std::max(dragCtx->bbox.LengthX(),
                        dragCtx->bbox.LengthY()), dragCtx->bbox.LengthZ());
            cube->width = length;
            cube->height = length;
            cube->depth = length;

            viewer->setupEditingRoot(group,&dragCtx->preTransform);
        }else{
            SoFCCSysDragger* dragger = static_cast<SoFCCSysDragger*>(pcDragger.get());
            dragger->draggerSize.setValue(0.05f);
            dragger->setUpAutoScale(viewer->getSoRenderManager()->getCamera());
            viewer->setupEditingRoot(pcDragger,&dragCtx->preTransform);

            TaskCSysDragger *task = new TaskCSysDragger(this, dragger);
            Gui::Control().showDialog(task);
        }
    }
}

void ViewProviderLink::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
    SoNode *child = static_cast<SoFCUnifiedSelection*>(viewer->getSceneGraph())->getChild(0);
    if (child && child->isOfType(SoPickStyle::getClassTypeId()))
        static_cast<SoFCUnifiedSelection*>(viewer->getSceneGraph())->removeChild(child);
    pcDragger.reset();
    dragCtx.reset();
    Gui::Control().closeDialog();
}

Base::Placement ViewProviderLink::currentDraggingPlacement() const{
    assert(pcDragger);
    SbVec3f v;
    SbRotation r;
    if(useCenterballDragger) {
        SoCenterballDragger *dragger = static_cast<SoCenterballDragger*>(pcDragger.get());
        v = dragger->center.getValue();
        r = dragger->rotation.getValue();
    }else{
        SoFCCSysDragger *dragger = static_cast<SoFCCSysDragger*>(pcDragger.get());
        v = dragger->translation.getValue();
        r = dragger->rotation.getValue();
    }
    float q1,q2,q3,q4;
    r.getValue(q1,q2,q3,q4);
    return Base::Placement(Base::Vector3d(v[0],v[1],v[2]),Base::Rotation(q1,q2,q3,q4));
}

void ViewProviderLink::enableCenterballDragger(bool enable) {
    if(enable == useCenterballDragger)
        return;
    if(pcDragger)
        LINK_THROW(Base::RuntimeError,"Cannot change dragger during dragging");
    useCenterballDragger = enable;
}

void ViewProviderLink::updateDraggingPlacement(const Base::Placement &pla, bool force) {
    if(pcDragger && (force || currentDraggingPlacement()!=pla)) {
        const auto &pos = pla.getPosition();
        const auto &rot = pla.getRotation();
        FC_LOG("updating dragger placement (" << pos.x << ", " << pos.y << ", " << pos.z << ')');
        if(useCenterballDragger) {
            SoCenterballDragger *dragger = static_cast<SoCenterballDragger*>(pcDragger.get());
            SbBool wasenabled = dragger->enableValueChangedCallbacks(FALSE);
            SbMatrix matrix;
            matrix = convert(pla.toMatrix());
            dragger->center.setValue(SbVec3f(0,0,0));
            dragger->setMotionMatrix(matrix);
            if (wasenabled) {
                dragger->enableValueChangedCallbacks(TRUE);
                dragger->valueChanged();
            }
        }else{
            SoFCCSysDragger *dragger = static_cast<SoFCCSysDragger*>(pcDragger.get());
            dragger->translation.setValue(SbVec3f(pos.x,pos.y,pos.z));
            dragger->rotation.setValue(rot[0],rot[1],rot[2],rot[3]);
        }
    }
}

bool ViewProviderLink::callDraggerProxy(const char *fname, bool update) {
    if(!pcDragger)
        return false;
    Base::PyGILStateLocker lock;
    try {
        auto* proxy = getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(fname)) {
                Py::Callable method(feature.getAttr(fname));
                Py::Tuple args;
                if(method.apply(args).isTrue())
                    return true;
            }
        }
    } catch (Py::Exception&) {
        Base::PyException e;
        e.ReportException();
        return true;
    }

    if(update) {
        auto ext = getLinkExtension();
        if(ext) {
            const auto &pla = currentDraggingPlacement();
            auto prop = ext->getLinkPlacementProperty();
            if(!prop)
                prop = ext->getPlacementProperty();
            if(prop) {
                auto plaNew = pla * Base::Placement(dragCtx->mat);
                if(prop->getValue()!=plaNew)
                    prop->setValue(plaNew);
            }
            updateDraggingPlacement(pla);
        }
    }
    return false;
}

void ViewProviderLink::dragStartCallback(void *data, SoDragger *) {
    auto me = static_cast<ViewProviderLink*>(data);
    me->dragCtx->initialPlacement = me->currentDraggingPlacement();
    if(!me->callDraggerProxy("onDragStart",false)) {
        me->dragCtx->cmdPending = true;
        me->getDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Link Transform"));
    }else
        me->dragCtx->cmdPending = false;
}

void ViewProviderLink::dragFinishCallback(void *data, SoDragger *) {
    auto me = static_cast<ViewProviderLink*>(data);
    me->callDraggerProxy("onDragEnd",true);
    if(me->dragCtx->cmdPending) {
        if(me->currentDraggingPlacement() == me->dragCtx->initialPlacement)
            me->getDocument()->abortCommand();
        else
            me->getDocument()->commitCommand();
    }
}

void ViewProviderLink::dragMotionCallback(void *data, SoDragger *) {
    auto me = static_cast<ViewProviderLink*>(data);
    me->callDraggerProxy("onDragMotion",true);
}

void ViewProviderLink::updateLinks(ViewProvider *vp) {
    try {
        auto ext = vp->getExtensionByType<ViewProviderLinkObserver>(true);
        if (ext && ext->linkInfo)
            ext->linkInfo->update();
    }
    catch (const Base::TypeError &e) {
        e.ReportException();
    }
    catch (const Base::ValueError &e) {
        e.ReportException();
    }
}

PyObject *ViewProviderLink::getPyObject() {
    if (!pyViewObject)
        pyViewObject = new ViewProviderLinkPy(this);
    pyViewObject->IncRef();
    return pyViewObject;
}

PyObject *ViewProviderLink::getPyLinkView() {
    return linkView->getPyObject();
}

std::map<std::string, App::Color> ViewProviderLink::getElementColors(const char *subname) const {
    bool isPrefix = true;
    if(!subname)
        subname = "";
    else {
        auto len = strlen(subname);
        isPrefix = !len || subname[len-1]=='.';
    }
    std::map<std::string, App::Color> colors;
    auto ext = getLinkExtension();
    if(!ext || ! ext->getColoredElementsProperty())
        return colors;
    const auto &subs = ext->getColoredElementsProperty()->getShadowSubs();
    int size = OverrideColorList.getSize();

    std::string wildcard(subname);
    if(wildcard == "Face" || wildcard == "Face*" || wildcard.empty()) {
        if(wildcard.size()==4 || OverrideMaterial.getValue()) {
            App::Color c = ShapeMaterial.getValue().diffuseColor;
            c.a = ShapeMaterial.getValue().transparency;
            colors["Face"] = c;
            if(wildcard.size()==4)
                return colors;
        }
        if(wildcard.size())
            wildcard.resize(4);
    }else if(wildcard == "Edge*")
        wildcard.resize(4);
    else if(wildcard == "Vertex*")
        wildcard.resize(5);
    else if(wildcard == ViewProvider::hiddenMarker()+"*")
        wildcard.resize(ViewProvider::hiddenMarker().size());
    else
        wildcard.clear();

    int i=-1;
    if(wildcard.size()) {
        for(const auto &sub : subs) {
            if(++i >= size)
                break;
            auto pos = sub.second.rfind('.');
            if(pos == std::string::npos)
                pos = 0;
            else
                ++pos;
            const char *element = sub.second.c_str()+pos;
            if(boost::starts_with(element,wildcard))
                colors[sub.second] = OverrideColorList[i];
            else if(!element[0] && wildcard=="Face")
                colors[sub.second.substr(0,element-sub.second.c_str())+wildcard] = OverrideColorList[i];
        }

        // In case of multi-level linking, we recursively call into each level,
        // and merge the colors
        auto vp = this;
        while(1) {
            if(wildcard!=ViewProvider::hiddenMarker() && vp->OverrideMaterial.getValue()) {
                auto color = ShapeMaterial.getValue().diffuseColor;
                color.a = ShapeMaterial.getValue().transparency;
                colors.emplace(wildcard,color);
            }
            auto link = vp->getObject()->getLinkedObject(false);
            if(!link || link==vp->getObject())
                break;
            auto next = freecad_dynamic_cast<ViewProviderLink>(
                    Application::Instance->getViewProvider(link));
            if(!next)
                break;
            for(const auto &v : next->getElementColors(subname))
                colors.insert(v);
            vp = next;
        }
        if(wildcard!=ViewProvider::hiddenMarker()) {
            // Get collapsed array color override.
            auto ext = vp->getLinkExtension();
            if(ext->_getElementCountValue() && !ext->_getShowElementValue()) {
                const auto &overrides = vp->OverrideMaterialList.getValues();
                int i=-1;
                for(const auto &mat : vp->MaterialList.getValues()) {
                    if(++i>=(int)overrides.size())
                        break;
                    if(!overrides[i])
                        continue;
                    auto color = mat.diffuseColor;
                    color.a = mat.transparency;
                    colors.emplace(std::to_string(i)+"."+wildcard,color);
                }
            }
        }
        return colors;
    }

    int element_count = ext->getElementCountValue();

    for(const auto &sub : subs) {
        if(++i >= size)
            break;

        int offset = 0;

        if(sub.second.size() && element_count && !std::isdigit(sub.second[0])) {
            // For checking and expanding color override of array base
            if(!subname[0]) {
                std::ostringstream ss;
                ss << "0." << sub.second;
                if(getObject()->getSubObject(ss.str().c_str())) {
                    for(int j=0;j<element_count;++j) {
                        ss.str("");
                        ss << j << '.' << sub.second;
                        colors.emplace(ss.str(),OverrideColorList[i]);
                    }
                    continue;
                }
            } else if (std::isdigit(subname[0])) {
                const char *dot = strchr(subname,'.');
                if(dot)
                    offset = dot-subname+1;
            }
        }

        if(isPrefix) {
            if(!boost::starts_with(sub.first,subname+offset)
                    && !boost::starts_with(sub.second,subname+offset))
                continue;
        }else if(sub.first!=subname+offset && sub.second!=subname+offset)
            continue;

        if(offset)
            colors.emplace(std::string(subname,offset)+sub.second, OverrideColorList[i]);
        else
            colors[sub.second] = OverrideColorList[i];
    }

    if(!subname[0])
        return colors;

    bool found = true;
    if(colors.empty()) {
        found = false;
        colors.emplace(subname,App::Color());
    }
    std::map<std::string, App::Color> ret;
    for(const auto &v : colors) {
        const char *pos = 0;
        auto sobj = getObject()->resolve(v.first.c_str(),nullptr,nullptr,&pos);
        if(!sobj || !pos)
            continue;
        auto link = sobj->getLinkedObject(true);
        if(!link || link==getObject())
            continue;
        auto vp = Application::Instance->getViewProvider(sobj->getLinkedObject(true));
        if(!vp)
            continue;
        for(const auto &v2 : vp->getElementColors(!pos[0]?"Face":pos)) {
            std::string name;
            if(pos[0])
                name = v.first.substr(0,pos-v.first.c_str())+v2.first;
            else
                name = v.first;
            ret[name] = found?v.second:v2.second;
        }
    }
    return ret;
}

void ViewProviderLink::setElementColors(const std::map<std::string, App::Color> &colorMap) {
    auto ext = getLinkExtension();
    if(!ext || ! ext->getColoredElementsProperty())
        return;

    // For checking and collapsing array element color
    std::map<std::string,std::map<int,App::Color> > subMap;
    int element_count = ext->getElementCountValue();

    std::vector<std::string> subs;
    std::vector<App::Color> colors;
    App::Color faceColor;
    bool hasFaceColor = false;
    for(const auto &v : colorMap) {
        if(!hasFaceColor && v.first == "Face") {
            hasFaceColor = true;
            faceColor = v.second;
            continue;
        }

        if(element_count && v.first.size() && std::isdigit(v.first[0])) {
            // In case of array, check if there are override of the same
            // sub-element for every array element. And collapse those overrides
            // into one without the index.
            const char *dot = strchr(v.first.c_str(),'.');
            if(dot) {
                subMap[dot+1][std::atoi(v.first.c_str())] = v.second;
                continue;
            }
        }
        subs.push_back(v.first);
        colors.push_back(v.second);
    }
    for(auto &v : subMap) {
        if(element_count == (int)v.second.size()) {
            App::Color firstColor = v.second.begin()->second;
            subs.push_back(v.first);
            colors.push_back(firstColor);
            for(auto it=v.second.begin();it!=v.second.end();) {
                if(it->second==firstColor)
                    it = v.second.erase(it);
                else
                    ++it;
            }
        }
        std::ostringstream ss;
        for(const auto &colorInfo : v.second) {
            ss.str("");
            ss << colorInfo.first << '.' << v.first;
            subs.push_back(ss.str());
            colors.push_back(colorInfo.second);
        }
    }

    auto prop = ext->getColoredElementsProperty();
    if(subs!=prop->getSubValues() || colors!=OverrideColorList.getValues()) {
        prop->setStatus(App::Property::User3,true);
        prop->setValue(getObject(),subs);
        prop->setStatus(App::Property::User3,false);
        OverrideColorList.setValues(colors);
    }
    if(hasFaceColor) {
        auto mat = ShapeMaterial.getValue();
        mat.diffuseColor = faceColor;
        mat.transparency = faceColor.a;
        ShapeMaterial.setStatus(App::Property::User3,true);
        ShapeMaterial.setValue(mat);
        ShapeMaterial.setStatus(App::Property::User3,false);
    }
    OverrideMaterial.setValue(hasFaceColor);
}

void ViewProviderLink::applyColors() {
    auto ext = getLinkExtension();
    if(!ext || ! ext->getColoredElementsProperty())
        return;

    SoSelectionElementAction action(SoSelectionElementAction::Color,true);
    // reset color and visibility first
    action.apply(linkView->getLinkRoot());

    std::map<std::string, std::map<std::string,App::Color> > colorMap;
    std::set<std::string> hideList;
    auto colors = getElementColors();
    colors.erase("Face");
    for(const auto &v : colors) {
        const char *subname = v.first.c_str();
        const char *element = nullptr;
        auto sobj = getObject()->resolve(subname,nullptr,nullptr,&element);
        if(!sobj || !element)
            continue;
        if(ViewProvider::hiddenMarker() == element)
            hideList.emplace(subname,element-subname);
        else
            colorMap[std::string(subname,element-subname)][element] = v.second;
    }

    SoTempPath path(10);
    path.ref();
    for(auto &v : colorMap) {
        action.swapColors(v.second);
        if(v.first.empty()) {
            action.apply(linkView->getLinkRoot());
            continue;
        }
        SoDetail *det=nullptr;
        path.truncate(0);
        if(getDetailPath(v.first.c_str(), &path, false, det))
            action.apply(&path);
        delete det;
    }

    action.setType(SoSelectionElementAction::Hide);
    for(const auto &sub : hideList) {
        SoDetail *det=nullptr;
        path.truncate(0);
        if(sub.size() && getDetailPath(sub.c_str(), &path, false, det))
            action.apply(&path);
        delete det;
    }
    path.unrefNoDelete();
}

void ViewProviderLink::setOverrideMode(const std::string &mode) {
    auto ext = getLinkExtension();
    if(!ext)
        return;
    auto obj = ext->getTrueLinkedObject(false);
    if(obj && obj!=getObject()) {
        auto vp = Application::Instance->getViewProvider(obj);
        vp->setOverrideMode(mode);
    }
    if(childVp)
        childVp->setOverrideMode(mode);
}

void ViewProviderLink::onBeforeChange(const App::Property *prop) {
    if(prop == &ChildViewProvider) {
        if(childVp) {
            childVp->beforeDelete();
            pcModeSwitch->replaceChild(1,linkView->getLinkRoot());
            childVpLink.reset();
            childVp = nullptr;
        }
    }
    inherited::onBeforeChange(prop);
}

static bool isExcludedProperties(const char *name) {
#define CHECK_EXCLUDE_PROP(_name) if(strcmp(name,#_name)==0) return true;
    CHECK_EXCLUDE_PROP(Proxy);
    return false;
}

App::Property *ViewProviderLink::getPropertyByName(const char *name) const {
    auto prop = inherited::getPropertyByName(name);
    if(prop || isExcludedProperties(name))
        return prop;
    if(childVp) {
        prop = childVp->getPropertyByName(name);
        if(prop && !prop->testStatus(App::Property::Hidden))
            return prop;
        prop = nullptr;
    }
    if(pcObject && pcObject->canLinkProperties()) {
        auto linked = getLinkedViewProvider(nullptr,true);
        if(linked && linked!=this)
            prop = linked->getPropertyByName(name);
    }
    return prop;
}

void ViewProviderLink::getPropertyMap(std::map<std::string,App::Property*> &Map) const {
    inherited::getPropertyMap(Map);
    if(!childVp)
        return;
    std::map<std::string,App::Property*> childMap;
    childVp->getPropertyMap(childMap);
    for(const auto &v : childMap) {
        auto ret = Map.insert(v);
        if(!ret.second) {
            auto myProp = ret.first->second;
            if(myProp->testStatus(App::Property::Hidden))
                ret.first->second = v.second;
        }
    }
}

void ViewProviderLink::getPropertyList(std::vector<App::Property*> &List) const {
    std::map<std::string,App::Property*> Map;
    getPropertyMap(Map);
    List.reserve(List.size()+Map.size());
    for(const auto &v:Map)
        List.push_back(v.second);
}

ViewProviderDocumentObject *ViewProviderLink::getLinkedViewProvider(
        std::string *subname, bool recursive) const
{
    auto self = const_cast<ViewProviderLink*>(this);
    auto ext = getLinkExtension();
    if(!ext)
        return self;
    App::DocumentObject *linked = nullptr;
    if(!recursive) {
        linked = ext->getLink();
        const char *s = ext->getSubName();
        if(subname && s)
            *subname = s;
    } else
        linked = ext->getTrueLinkedObject(recursive);
    if(!linked)
        return self;
    auto res = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
            Application::Instance->getViewProvider(linked));
    if(res)
        return res;
    return self;
}

////////////////////////////////////////////////////////////////////////////////////////

namespace Gui {
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderLinkPython, Gui::ViewProviderLink)
template class GuiExport ViewProviderPythonFeatureT<ViewProviderLink>;
}
