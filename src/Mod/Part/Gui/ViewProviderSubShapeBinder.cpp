/***************************************************************************
 *   Copyright (c) 2021 Zheng Lei (realthunder) <realthunder.dev@gmail.com>*
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
# include <QApplication>
# include <QMessageBox>
# include <QMenu>
# include <QPainter>
# include <QMouseEvent>
# include <Inventor/nodes/SoSeparator.h>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <set>
#include <unordered_set>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/ViewParams.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ActionFunction.h>
#include <Gui/Command.h>

#include <Mod/Part/App/SubShapeBinder.h>
#include <Mod/Part/Gui/PartParams.h>
#include "ViewProviderSubShapeBinder.h"

FC_LOG_LEVEL_INIT("Part",true,true)

using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderSubShapeBinder,PartGui::ViewProviderPart)

ViewProviderSubShapeBinder::ViewProviderSubShapeBinder() {
    sPixmap = "Part_SubShapeBinder.svg";

    ADD_PROPERTY_TYPE(UseBinderStyle, (false), "",(App::PropertyType)(App::Prop_None), "");
    ForceMapColors.setValue(true);
}

void ViewProviderSubShapeBinder::attach(App::DocumentObject *obj) {
    ViewProviderPart::attach(obj);
    if (boost::starts_with(obj->getNameInDocument(), "Import")) {
        UseBinderStyle.setValue(true);
        auto binder = Base::freecad_dynamic_cast<Part::SubShapeBinder>(obj);
        if (binder)
            binder->MakeFace.setValue(false);
    } else
        UseBinderStyle.setValue(
            boost::istarts_with(obj->getNameInDocument(),"binder"));
}

template<class P, class V>
void setProperty(P &prop, const V &v) {
    Base::ObjectStatusLocker<App::Property::Status, App::Property> lock(App::Property::User3, &prop);
    prop.setValue(v);
}

void ViewProviderSubShapeBinder::onChanged(const App::Property *prop) {
    if(prop == &UseBinderStyle
            && (!getObject() || !getObject()->isRestoring()))
    {
        App::Color shapeColor,lineColor,pointColor;
        int transparency, linewidth;
        bool mapFace,mapLine,mapPoint,mapTrans;
        if(UseBinderStyle.getValue()) {
            //get the datum coloring scheme
            // set default color for datums (golden yellow with 60% transparency)
            shapeColor.setPackedValue(PartParams::DefaultDatumColor());
            lineColor = shapeColor;
            pointColor = shapeColor;
            transparency = 60;
            linewidth = 1;
            mapLine = mapPoint = mapTrans = false;
            mapFace = true;
        } else {
            shapeColor.setPackedValue(Gui::ViewParams::instance()->getDefaultShapeColor());
            lineColor.setPackedValue(Gui::ViewParams::instance()->getDefaultShapeLineColor());
            pointColor = lineColor;
            transparency = 0;
            linewidth = Gui::ViewParams::instance()->getDefaultShapeLineWidth();
#if 0
            static ParameterGrp::handle hPart = App::GetApplication().GetParameterGroupByPath
                ("User parameter:BaseApp/Preferences/Mod/Part");
            mapFace = hPart->GetBool("MapFaceColor");
            mapLine = hPart->GetBool("MapLineColor");
            mapPoint = hPart->GetBool("MapPointColor");
            mapTrans = hPart->GetBool("MapTransparency");
#else
            mapFace = true;
            mapLine = true;
            mapPoint = true;
            mapTrans = true;
#endif
        }

        setProperty(LineColor, lineColor);
        setProperty(PointColor, pointColor);
        setProperty(ShapeColor, shapeColor);
        setProperty(Transparency, transparency);
        setProperty(LineWidth, linewidth);
        setProperty(MapFaceColor, mapFace);
        setProperty(MapLineColor, mapLine);
        setProperty(MapPointColor, mapPoint);
        setProperty(MapTransparency, mapTrans);
        updateColors();
    }
    else if (prop == &ShapeColor) {
        if (!prop->testStatus(App::Property::User3))
            setProperty(MapFaceColor, false);
    }
    else if (prop == &LineColor) {
        if (!prop->testStatus(App::Property::User3))
            setProperty(MapLineColor, false);
    }
    else if (prop == &PointColor) {
        if (!prop->testStatus(App::Property::User3))
            setProperty(MapPointColor, false);
    }
    else if (prop == &Transparency) {
        if (!prop->testStatus(App::Property::User3))
            setProperty(MapTransparency, false);
    }

    ViewProviderPart::onChanged(prop);
}

void ViewProviderSubShapeBinder::updateData(const App::Property *prop)
{
    auto binder = Base::freecad_dynamic_cast<Part::SubShapeBinder>(getObject());
    if (binder) {
        if (prop == &binder->Support) {
            iconChangeConns.clear();
            std::set<App::SubObjectT> objs;
            for(auto &l : binder->Support.getSubListValues()) {
                for (auto & sub : l.getSubValues()) {
                    auto res = objs.emplace(l.getValue(), sub.c_str());
                    if (!res.second)
                        continue;
                    auto vp = Gui::Application::Instance->getViewProvider(res.first->getSubObject());
                    if (vp) {
                        iconChangeConns.push_back(vp->signalChangeIcon.connect(
                            [this]() { this->iconMap.clear(); this->signalChangeIcon(); }));
                    }
                }
            }
            this->iconMap.clear();
            signalChangeIcon();

        } else if (prop == &binder->BindMode) {
            switch(binder->BindMode.getValue()) {
            case 1:
                sPixmap = "Part_SubShapeBinder_Frozen.svg";
                break;
            case 2:
                sPixmap = "Part_SubShapeBinder_Detached.svg";
                break;
            default:
                sPixmap = "Part_SubShapeBinder.svg";
            }
            signalChangeIcon();
        }
    }
    ViewProviderPart::updateData(prop);
}

bool ViewProviderSubShapeBinder::canDropObjectEx(App::DocumentObject *, 
        App::DocumentObject *, const char *, const std::vector<std::string> &) const
{
    return true;
}

std::string ViewProviderSubShapeBinder::dropObjectEx(App::DocumentObject *obj, App::DocumentObject *owner,
        const char *subname, const std::vector<std::string> &elements)
{
    auto self = dynamic_cast<Part::SubShapeBinder*>(getObject());
    if(!self) return std::string(".");
    std::map<App::DocumentObject *, std::vector<std::string> > values;
    if(!subname) subname = "";
    std::string sub(subname);
    if(sub.empty()) 
        values[owner?owner:obj] = elements;
    else {
        std::vector<std::string> subs;
        if(elements.size()) {
            subs.reserve(elements.size());
            for(auto &element : elements)
                subs.push_back(sub+element);
        }else
            subs.push_back(sub);
        values[owner?owner:obj] = std::move(subs);
    }

    int dropid = Gui::isTreeViewDropping();
    self->setLinks(std::move(values),
            QApplication::keyboardModifiers()==Qt::ControlModifier && _dropID != dropid);
    _dropID = dropid;
    if(self->Relative.getValue())
        updatePlacement(false);
    return std::string(".");
}

bool ViewProviderSubShapeBinder::doubleClicked() {
    updatePlacement(true);
    return true;
}

void ViewProviderSubShapeBinder::setupContextMenu(QMenu* menu, QObject* receiver, const char* member) 
{
    auto self = Base::freecad_dynamic_cast<Part::SubShapeBinder>(getObject());
    if (!self)
        return;

    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    QAction* act;
    act = menu->addAction(QObject::tr("Synchronize"), receiver, member);
    func->trigger(act, [this]() {updatePlacement(false);});

    if(self->Support.getValue()) {
        if (self->BindMode.getValue() <= 1) {
            act = menu->addAction(QObject::tr("Toggle freeze"), receiver, member);
            func->trigger(act, [self]() {
                App::AutoTransaction committer(
                    self->BindMode.getValue() == 1 ? 
                        "Unfreeze shape binder" : "Freeze shape binder");
                try {
                    if (self->BindMode.getValue() == 0)
                        self->BindMode.setValue((long)1);
                    else
                        self->BindMode.setValue((long)0);
                    Gui::Command::updateActive();
                } catch (Base::Exception &e) {
                    e.ReportException();
                }
            });

            act = menu->addAction(QObject::tr("Detach"), receiver, member);
            func->trigger(act, [self]() {
                App::AutoTransaction committer("Detach shape binder");
                try {
                    self->BindMode.setValue((long)2);
                    Gui::Command::updateActive();
                } catch (Base::Exception &e) {
                    e.ReportException();
                }
            });
        } else {
            act = menu->addAction(QObject::tr("Reset bind mode"), receiver, member);
            func->trigger(act, [self]() {
                App::AutoTransaction committer("Reset bind mode");
                try {
                    self->BindMode.setValue((long)0);
                    Gui::Command::updateActive();
                } catch (Base::Exception &e) {
                    e.ReportException();
                }
            });
        }

        act = menu->addAction(QObject::tr("Select bound object"), receiver, member);
        func->trigger(act, [self]() {
            Gui::Selection().selStackPush();
            Gui::Selection().clearSelection();
            for(auto &link : self->Support.getSubListValues()) {
                auto obj = link.getValue();
                if(!obj || !obj->getNameInDocument())
                    continue;
                const auto &subs = link.getSubValues();
                if(subs.size())
                    Gui::Selection().addSelections(obj->getDocument()->getName(),
                            obj->getNameInDocument(),subs);
                else
                    Gui::Selection().addSelection(obj->getDocument()->getName(),
                            obj->getNameInDocument());
            }
            Gui::Selection().selStackPush();
        });
    }

    ViewProviderPart::setupContextMenu(menu,receiver,member);
}

bool ViewProviderSubShapeBinder::setEdit(int ModNum) {
    
    switch(ModNum) {
    case 0:
        updatePlacement(true);
        break;
    case 0x81: {
        auto self = dynamic_cast<Part::SubShapeBinder*>(getObject());
        if(!self || !self->Support.getValue())
            break;

        Gui::Selection().selStackPush();
        Gui::Selection().clearSelection();
        for(auto &link : self->Support.getSubListValues()) {
            auto obj = link.getValue();
            if(!obj || !obj->getNameInDocument())
                continue;
            const auto &subs = link.getSubValues();
            if(subs.size())
                Gui::Selection().addSelections(obj->getDocument()->getName(),
                        obj->getNameInDocument(),subs);
            else
                Gui::Selection().addSelection(obj->getDocument()->getName(),
                        obj->getNameInDocument());
        }
        Gui::Selection().selStackPush();
        break;
    }
    default:
        return ViewProviderPart::setEdit(ModNum);
    }
    return false;
}

void ViewProviderSubShapeBinder::updatePlacement(bool transaction) {
    auto self = dynamic_cast<Part::SubShapeBinder*>(getObject());
    if(!self || !self->Support.getValue())
        return;

    std::vector<Base::Matrix4D> mats;
    bool relative = self->Relative.getValue();
    App::DocumentObject *parent = 0;
    std::string parentSub;
    if(relative && self->getParents().size()) {
        const auto &sel = Gui::Selection().getSelection("",0);
        if(sel.size()!=1 || !sel[0].pObject ||
            sel[0].pObject->getSubObject(sel[0].SubName)!=self) 
        {
            FC_WARN("invalid selection");
        } else {
            parent = sel[0].pObject;
            parentSub = sel[0].SubName;
        }
    }

    if(!transaction) {
        if(relative)
            self->Context.setValue(parent,parentSub.c_str());
        try {
            self->update(Part::SubShapeBinder::UpdateForced);
        } catch (Base::Exception &e) {
            e.ReportException();
        }
        return;
    }

    App::AutoTransaction commiter("Sync binder");
    try{
        if(relative)
            self->Context.setValue(parent,parentSub.c_str());
        self->update(Part::SubShapeBinder::UpdateForced);
        Gui::Command::updateActive();
    }catch(Base::Exception &e) {
        e.ReportException();
    }catch(Standard_Failure &e) {
        std::ostringstream str;
        Standard_CString msg = e.GetMessageString();
        str << typeid(e).name() << " ";
        if (msg) {str << msg;}
        else     {str << "No OCCT Exception Message";}
        FC_ERR(str.str());
    }
}

std::vector<App::DocumentObject*> ViewProviderSubShapeBinder::claimChildren(void) const {
    std::vector<App::DocumentObject *> ret;
    auto self = Base::freecad_dynamic_cast<Part::SubShapeBinder>(getObject());
    if(self && self->ClaimChildren.getValue() && self->Support.getValue()) {
        std::set<App::DocumentObject *> objSet;
        for(auto &l : self->Support.getSubListValues()) {
            auto obj = l.getValue();
            if(!obj)
                continue;
            const auto &subs = l.getSubValues();
            if(subs.empty()) {
                if(objSet.insert(obj).second)
                    ret.push_back(obj);
                continue;
            }
            for(auto &sub : subs) {
                auto sobj = obj->getSubObject(sub.c_str());
                if(sobj && objSet.insert(sobj).second)
                    ret.push_back(sobj);
            }
        }
    }
    return ret;
}

struct PixmapInfo {
    int count = 0;
    QByteArray tag;

    void generateIcon(QPixmap &px, const QPixmap &pxTag)
    {
        if (++this->count > 2)
            return;

        int tagWidth = pxTag.isNull() ? 0 : pxTag.width()/2;

        if (this->count == 1) {
            QPixmap pxOrig = px;
            px = QPixmap(64 + tagWidth, 64);
            px.fill(Qt::transparent);
            QPainter pt;
            pt.begin(&px);
            pt.setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);
            pt.setPen(Qt::NoPen);
            pt.setBrush(Qt::white);
            pt.drawRect(QRect(5, 5, 52, 52));
            pt.drawPixmap(7, 7, 48, 48, pxOrig, 0, 0, pxOrig.width(), pxOrig.height());
            pt.setPen(QPen(Qt::black, 2));
            pt.setBrush(QBrush());
            pt.drawRect(QRect(5, 5, 52, 52));
            if (!pxTag.isNull())
                pt.drawPixmap(64-tagWidth, 0, pxTag);
            pt.end();
            return;
        }

        QPixmap pxCopy = px;
        px = QPixmap(pxCopy.size());
        px.fill(Qt::transparent);
        QPainter pt;
        pt.begin(&px);
        pt.setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);
        pt.setPen(QPen(Qt::black, 2));
        pt.setBrush(Qt::white);
        pt.drawRect(QRect(1, 1, 53, 53));
        pt.drawPixmap(5, 5, pxCopy);
        pt.end();
    }
};

Gui::ViewProviderDocumentObject *ViewProviderSubShapeBinder::getLinkedViewProvider(
        std::string *subname, bool recursive) const
{
    (void)subname;
    auto self = const_cast<ViewProviderSubShapeBinder*>(this);
    if(!pcObject || !pcObject->getNameInDocument())
        return self;

    auto binder = Base::freecad_dynamic_cast<Part::SubShapeBinder>(pcObject);
    auto linked = binder ? binder->_getLinkedObject(recursive) : pcObject->getLinkedObject(recursive);
    if(!linked || linked == pcObject)
        return self;
    auto res = Base::freecad_dynamic_cast<Gui::ViewProviderDocumentObject>(
            Gui::Application::Instance->getViewProvider(linked));
    if(!res)
        res = self;
    return res;
}

void ViewProviderSubShapeBinder::getExtraIcons(
        std::vector<std::pair<QByteArray, QPixmap> > &icons) const
{
    generateIcons();
    for (auto &v : iconMap)
        icons.emplace_back(v.first, v.second.pixmap);
}

QString ViewProviderSubShapeBinder::getToolTip(const QByteArray &tag) const
{
    generateIcons();

    auto self = Base::freecad_dynamic_cast<Part::SubShapeBinder>(getObject());
    if (!self)
        return QString();

    std::ostringstream ss;
    auto doc = getObject()->getDocument()->getName();

    if (tag == Gui::treeMainIconTag()) {
        if (!self->Support.getValue()) {
            if (self->BindMode.getValue() == 2)
                return QObject::tr("Detached shape binder");
            return QString();
        }
        for(auto &link : self->Support.getSubListValues()) {
            auto obj = link.getValue();
            if(!obj || !obj->getNameInDocument())
                continue;
            const auto &subs = link.getSubValues();
            if(subs.size()) {
                for (auto &sub : subs)
                    ss << "\n" << App::SubObjectT(obj, sub.c_str()).getSubObjectFullName(doc);
            } else
                ss << "\n" << App::SubObjectT(obj, "").getObjectFullName(doc);
        }
    } else {
        auto it = iconMap.find(tag);
        if (it == iconMap.end())
            return inherited::getToolTip(tag);
        for (auto &objT : it->second.objs)
            ss << "\n" << objT.getSubObjectFullName(doc);
    }

    if (!ss.tellp())
        return QString();

    if (Gui::isTreeViewDragging()) {
        return QString::fromLatin1("%1\n%2\n%3").arg(
                QObject::tr("Drop to add more binding, or hold CTRL to clear before assign new bindings."),
                self->BindMode.getValue() == 1 ? 
                    QObject::tr("Frozen bound objects:") : QObject::tr("Current bound objects:"),
                QString::fromUtf8(ss.str().c_str()));
    }
    return QString::fromLatin1("%1 %2\n%3").arg(
            self->BindMode.getValue() == 1 ?
                QObject::tr("Frozen bound objects") : QObject::tr("Bound objects"),
            QObject::tr("(ALT + click this icon to select):"),
            QString::fromUtf8(ss.str().c_str()));
}

bool ViewProviderSubShapeBinder::iconMouseEvent(QMouseEvent *ev, const QByteArray &tag)
{
    auto self = dynamic_cast<Part::SubShapeBinder*>(getObject());
    if (!self)
        return false;
    if (ev->type() == QEvent::MouseButtonPress) {
        const std::vector<App::SubObjectT> *objs = nullptr;
        std::vector<App::SubObjectT> _objs;
        if (tag == Gui::treeMainIconTag()) {
            for(auto &link : self->Support.getSubListValues()) {
                auto obj = link.getValue();
                if(!obj || !obj->getNameInDocument())
                    continue;
                const auto &subs = link.getSubValues(false);
                if(subs.size()) {
                    for (auto &sub : subs)
                        _objs.emplace_back(obj, sub.c_str());
                } else
                    _objs.emplace_back(obj, "");
            }
            objs = &_objs;
        } else {
            auto it = iconMap.find(tag);
            if (it == iconMap.end())
                return inherited::iconMouseEvent(ev, tag);
            objs = &it->second.objs;
        }
        if (!objs || objs->empty())
            return false;

        bool singleSelect = !(ev->modifiers() & Qt::ControlModifier);
        if (singleSelect) {
            Gui::Selection().selStackPush();
            Gui::Selection().clearSelection();
        }
        for (auto &objT : *objs)
            Gui::Selection().addSelection(objT);
        if (singleSelect)
            Gui::Selection().selStackPush();
        return true;
    }

    return inherited::iconMouseEvent(ev, tag);
}

void ViewProviderSubShapeBinder::generateIcons() const
{
    if (iconMap.size())
        return;

    auto binder = Base::freecad_dynamic_cast<Part::SubShapeBinder>(getObject());
    if (!binder)
        return;

    std::unordered_map<qint64, PixmapInfo> cacheKeys;
    for(auto &l : binder->Support.getSubListValues()) {
        for (auto & sub : l.getSubValues(false)) {
            App::SubObjectT sobjT(l.getValue(), sub.c_str());
            auto sobj = sobjT.getSubObject();
            if (!sobj)
                continue;

            auto binder = Base::freecad_dynamic_cast<Part::SubShapeBinder>(sobj);
            Gui::ViewProvider *vp = nullptr;
            QPixmap px;
            if (binder) {
                // binder of binder, extract its first bound object's icon
                vp = Gui::Application::Instance->getViewProvider(
                        binder->_getLinkedObject(true));
            } else
                vp = Gui::Application::Instance->getViewProvider(sobj);

            if (vp) {
                px = vp->getIcon().pixmap(64, 64);
                unsigned long tagColor = 0;
                auto prop = Base::freecad_dynamic_cast<App::PropertyColor>(
                        vp->getPropertyByName("IconColor"));
                if (prop)
                    tagColor = prop->getValue().getPackedValue();

                QPixmap binderIcon;
                if (binder) {
                    auto binderVp = Base::freecad_dynamic_cast<ViewProviderSubShapeBinder>(
                            Gui::Application::Instance->getViewProvider(binder));
                    if (binderVp)
                        binderIcon = Gui::BitmapFactory().pixmap(binderVp->sPixmap);
                }

                auto & pxInfo = cacheKeys[(!binderIcon.isNull()?binderIcon.cacheKey():0)
                                          ^ px.cacheKey() ^ tagColor];
                QPixmap pxTag;
                auto it = iconMap.begin();
                if (pxInfo.count == 0) {
                    if (iconMap.size() >= 3)
                        break;
                    if (!binderIcon.isNull())
                        px = Gui::BitmapFactory().merge(px,
                               binderIcon.scaled(32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation),
                               Gui::BitmapFactoryInst::TopLeft);
                    if (tagColor) {
                        auto featVp = Base::freecad_dynamic_cast<PartGui::ViewProviderPart>(vp);
                        if (featVp)
                            pxTag = featVp->getTagIcon().scaled(40, 40,
                                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                    }
                    pxInfo.tag = QByteArray("Binder:") + QByteArray::number((int)iconMap.size());
                    it = iconMap.emplace(pxInfo.tag, IconInfo()).first;
                    it->second.pixmap = px;
                } else {
                    it = iconMap.find(pxInfo.tag);
                    assert(it != iconMap.end());
                }
                pxInfo.generateIcon(it->second.pixmap, pxTag);
                it->second.objs.push_back(std::move(sobjT));
            }
        }
        if (iconMap.size() >= 3)
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////

namespace Gui {
PROPERTY_SOURCE_TEMPLATE(PartGui::ViewProviderSubShapeBinderPython,
                         PartGui::ViewProviderSubShapeBinder)
template class PartGuiExport ViewProviderPythonFeatureT<ViewProviderSubShapeBinder>;
}
