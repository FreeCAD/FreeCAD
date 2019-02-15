/****************************************************************************
 *   Copyright (c) 2017 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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
# include <sstream>
# include <QMessageBox>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include "Command.h"
#include "Action.h"
#include "Application.h"
#include "MainWindow.h"
#include "Tree.h"
#include "Document.h"
#include "Selection.h"
#include "ViewProviderDocumentObject.h"

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>

FC_LOG_LEVEL_INIT("CommandLink",true,true);

using namespace Gui;

static void setLinkLabel(App::DocumentObject *obj, const char *doc, const char *name) {
    const char *label = obj->Label.getValue();
    Command::doCommand(Command::Doc,"App.getDocument('%s').getObject('%s').Label='%s'",doc,name,label);
}

////////////////////////////////////////////////////////////////////////////////////////////

class StdCmdLinkMakeGroup : public Gui::Command
{
public:
    StdCmdLinkMakeGroup();
    const char* className() const
    { return "StdCmdLinkMakeGroup"; }

protected:
    virtual void activated(int iMsg);
    virtual bool isActive(void);
    virtual Action * createAction(void);
    virtual void languageChange();
};

StdCmdLinkMakeGroup::StdCmdLinkMakeGroup()
  : Command("Std_LinkMakeGroup")
{
    sGroup        = QT_TR_NOOP("Link");
    sMenuText     = QT_TR_NOOP("Make link group");
    sToolTipText  = QT_TR_NOOP("Create a group of links");
    sWhatsThis    = "Std_LinkMakeGroup";
    sStatusTip    = sToolTipText;
    eType         = AlterDoc;
    sPixmap       = "LinkGroup";
}

bool StdCmdLinkMakeGroup::isActive() {
    return !!App::GetApplication().getActiveDocument();
}

Action * StdCmdLinkMakeGroup::createAction(void)
{
    ActionGroup* pcAction = new ActionGroup(this, getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    // add the action items
    pcAction->addAction(QObject::tr("Plain group"));
    pcAction->addAction(QObject::tr("Group with links"));
    pcAction->addAction(QObject::tr("Group with transform links"));
    return pcAction;
}

void StdCmdLinkMakeGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    ActionGroup* pcAction = qobject_cast<ActionGroup*>(_pcAction);
    QList<QAction*> acts = pcAction->actions();
    acts[0]->setText(QObject::tr("Plain group"));
    acts[1]->setText(QObject::tr("Group with links"));
    acts[2]->setText(QObject::tr("Group with transform links"));
}


void StdCmdLinkMakeGroup::activated(int option) {

    std::vector<App::DocumentObject*> objs;
    std::set<App::DocumentObject*> objset;

    auto doc = App::GetApplication().getActiveDocument();
    if(!doc) {
        FC_ERR("no active document");
        return;
    }

    for(auto &sel : Selection().getCompleteSelection()) {
        if(sel.pObject && sel.pObject->getNameInDocument() &&
           objset.insert(sel.pObject).second)
            objs.push_back(sel.pObject);
    }

    App::GetApplication().setActiveTransaction("Make link group");
    try {
        std::string groupName = doc->getUniqueObjectName("LinkGroup");
        Command::doCommand(Command::Doc,
            "App.getDocument('%s').addObject('App::LinkGroup','%s')",doc->getName(),groupName.c_str());
        if(objs.size()) {
            Command::doCommand(Command::Doc,"__objs__ = []");
            for(auto obj : objs) {
                std::string name;
                if(option!=0 || doc!=obj->getDocument()) {
                    name = doc->getUniqueObjectName("Link");
                    Command::doCommand(Command::Doc,
                        "App.getDocument('%s').addObject('App::Link','%s').setLink("
                            "App.getDocument('%s').getObject('%s'))",
                        doc->getName(),name.c_str(),obj->getDocument()->getName(),obj->getNameInDocument());
                    setLinkLabel(obj,doc->getName(),name.c_str());
                    if(option==2)
                        Command::doCommand(Command::Doc,
                            "App.getDocument('%s').getObject('%s').LinkTransform = True",
                            doc->getName(),name.c_str());
                    else if(obj->getPropertyByName("Placement"))
                        Command::doCommand(Command::Doc,
                            "App.getDocument('%s').getObject('%s').Placement = "
                                "App.getDocument('%s').getObject('%s').Placement",
                            doc->getName(),name.c_str(),obj->getDocument()->getName(),obj->getNameInDocument());
                }else
                    name = obj->getNameInDocument();
                Command::doCommand(Command::Doc,"__objs__.append(App.getDocument('%s').getObject('%s'))",
                        doc->getName(),name.c_str());
                Command::doCommand(Command::Doc,
                        "App.getDocument('%s').getObject('%s').ViewObject.Visibility=False",
                        doc->getName(),name.c_str());
            }
            Command::doCommand(Command::Doc,"App.getDocument('%s').getObject('%s').setLink(__objs__)",
                    doc->getName(),groupName.c_str());
            Command::doCommand(Command::Doc,"del __objs__");
        }
        if(option!=0) {
            Command::doCommand(Command::Doc,
                    "App.getDocument('%s').getObject('%s').LinkMode = 'Auto Delete'",
                    doc->getName(),groupName.c_str());
        }
        App::GetApplication().closeActiveTransaction();
    } catch (const Base::Exception& e) {
        QMessageBox::critical(getMainWindow(), QObject::tr("Create link group failed"),
            QString::fromLatin1(e.what()));
        App::GetApplication().closeActiveTransaction(true);
        e.ReportException();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////

DEF_STD_CMD(StdCmdLinkMake)

StdCmdLinkMake::StdCmdLinkMake()
  : Command("Std_LinkMake")
{
    sGroup        = QT_TR_NOOP("Link");
    sMenuText     = QT_TR_NOOP("Make link");
    sToolTipText  = QT_TR_NOOP("Create a link to the selected object(s)");
    sWhatsThis    = "Std_LinkMake";
    sStatusTip    = sToolTipText;
    eType         = AlterDoc;
    sPixmap       = "Link";
}

void StdCmdLinkMake::activated(int) {
    auto doc = App::GetApplication().getActiveDocument();
    if(!doc) {
        FC_ERR("no active document");
        return;
    }

    std::set<App::DocumentObject*> objs;
    for(auto &sel : Selection().getCompleteSelection()) {
        if(sel.pObject && sel.pObject->getNameInDocument())
           objs.insert(sel.pObject);
    }

    App::GetApplication().setActiveTransaction("Make link");
    try {
        if(objs.empty()) {
            std::string name = doc->getUniqueObjectName("Link");
            Command::doCommand(Command::Doc, "App.getDocument('%s').addObject('App::Link','%s')",
                doc->getName(),name.c_str());
        }else{
            for(auto obj : objs) {
                std::string name = doc->getUniqueObjectName("Link");
                Command::doCommand(Command::Doc,
                    "App.getDocument('%s').addObject('App::Link','%s').setLink(App.getDocument('%s').%s)",
                    doc->getName(),name.c_str(),obj->getDocument()->getName(),obj->getNameInDocument());
                setLinkLabel(obj,doc->getName(),name.c_str());
            }
        }
        App::GetApplication().closeActiveTransaction();
    } catch (const Base::Exception& e) {
        QMessageBox::critical(getMainWindow(), QObject::tr("Create link failed"),
            QString::fromLatin1(e.what()));
        App::GetApplication().closeActiveTransaction(true);
        e.ReportException();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////

DEF_STD_CMD_A(StdCmdLinkMakeRelative)

StdCmdLinkMakeRelative::StdCmdLinkMakeRelative()
  : Command("Std_LinkMakeRelative")
{
    sGroup        = QT_TR_NOOP("Link");
    sMenuText     = QT_TR_NOOP("Make relative link");
    sToolTipText  = QT_TR_NOOP("Create a relative link of two selected objects");
    sWhatsThis    = "Std_LinkMakeRelative";
    sStatusTip    = sToolTipText;
    eType         = AlterDoc;
    sPixmap       = "LinkSub";
}

static App::DocumentObject *resolveLinkRelative(std::string *subname=0) {
    const auto &sels = Selection().getCompleteSelection(false);
    if(sels.size()==1 && sels[0].SubName && sels[0].SubName[0]) {
        auto sobj = sels[0].pObject->getSubObject(sels[0].SubName);
        if(sobj && sobj!=sels[0].pObject) {
            if(subname)
                *subname = sels[0].SubName;
            return sels[0].pObject;
        }
    }
    if(sels.size()!=2 || 
       !sels[0].pObject || !sels[0].pObject->getNameInDocument() ||
       !sels[1].pObject || !sels[1].pObject->getNameInDocument())
        return 0;
    auto len1 = strlen(sels[0].SubName);
    auto len2 = strlen(sels[1].SubName);
    if(len1>len2) {
        if(strncmp(sels[0].SubName,sels[1].SubName,len2)==0) {
            if(subname)
                *subname = sels[0].SubName+len2;
            return sels[1].pObject;
        }
    }else if(len1<len2) {
        if(strncmp(sels[1].SubName,sels[0].SubName,len1)==0) {
            if(subname)
                *subname = sels[1].SubName+len1;
            return sels[0].pObject;
        }
    }
    return 0;
}

bool StdCmdLinkMakeRelative::isActive() {
    return resolveLinkRelative()!=0;
}

void StdCmdLinkMakeRelative::activated(int) {
    auto doc = App::GetApplication().getActiveDocument();
    if(!doc) {
        FC_ERR("no active document");
        return;
    }
    std::string subname;
    auto owner = resolveLinkRelative(&subname);
    if(!owner) {
        FC_ERR("invalid selection");
        return;
    }
    auto obj = owner->getSubObject(subname.c_str());
    if(!obj) {
        FC_ERR("invalid sub-object " << owner->getFullName() << '.' << subname);
        return;
    }

    std::string name = doc->getUniqueObjectName("Link");
    App::GetApplication().setActiveTransaction("Make link sub");
    try {
        Command::doCommand(Command::Doc, 
            "App.getDocument('%s').addObject('App::Link','%s').setLink(App.getDocument('%s').%s,'%s')", 
            doc->getName(),name.c_str(),
            owner->getDocument()->getName(),owner->getNameInDocument(), subname.c_str());
        auto link = owner->getDocument()->getObject(name.c_str());
        FCMD_OBJ_CMD(link,"LinkTransform = True");
        setLinkLabel(obj,doc->getName(),name.c_str());
        App::GetApplication().closeActiveTransaction();
    } catch (const Base::Exception& e) {
        QMessageBox::critical(getMainWindow(), QObject::tr("Create link sub failed"),
            QString::fromLatin1(e.what()));
        App::GetApplication().closeActiveTransaction(true);
        e.ReportException();
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////////////

static void linkConvert(bool unlink) {
    // We are trying to replace an object with a link (App::Link), or replace a
    // link back to its linked object (i.e. unlink). This is a very complex
    // operation. It works by reassign the link property of the parent of the
    // selected object(s) to a newly created link to the original object.
    // Everything should remain the same. To simplify the operation a bit, we
    // restrict ourself to replace selected objects of the active document only.
    //
    // But still, the complication arises because there may have relative links
    // that are affected by this operation. Currently, the only supported
    // relative link are any objects containing a PropertyXLink with a non-empty
    // 'Subname'. The ProeprtyXLink is pointed to the parent object, while the
    // subname references some child object inside. The subname may contain
    // multiple levels of indirection. So, if the subname refers to an object
    // being replace through the exact same parent object, the relative link
    // will be broken. We shall take care of this situation, and make the
    // appropriate adjustment.

    App::Document *doc = App::GetApplication().getActiveDocument();
    if(!doc) return;

    // first, generate partial (link name not known yet) command for
    // objects that are replacable. The commands are keyed by a
    // pair(parent, object)
    std::map<std::pair<App::DocumentObject*,App::DocumentObject*>, std::vector<std::string> > replaceCmds;
    for(auto sel : TreeWidget::getSelection(doc)) {
        auto obj = sel.second->getObject();
        auto parent = sel.first;
        if(!parent) {
            FC_WARN("skip '" << obj->getFullName() << "' with no parent");
            continue;
        }
        auto parentObj = parent->getObject();
        if(unlink) {
            auto linked = obj->getLinkedObject(false);
            if(!linked || linked == obj) {
                FC_WARN("skip non link");
                continue;
            }
        }
        if(parentObj->getDocument()!=obj->getDocument()) {
            FC_WARN("cannot convert link for external object '" << obj->getFullName() << "'");
            continue;
        }

        std::map<std::string, App::Property*> props;
        parentObj->getPropertyMap(props);

        std::vector<std::string> cmds;
        for(auto &v : props) {
            auto &propName = v.first;
            auto prop = v.second;
            auto linkProp = dynamic_cast<App::PropertyLink*>(prop);
            if(linkProp) {
                if(prop->testStatus(App::Property::Immutable) || parentObj->isReadOnly(prop))
                    continue;
                if(linkProp->getValue()==obj) {
                    std::ostringstream str;
                    str << Gui::Command::getObjectCmd(parentObj) << '.'
                        << propName << '=' << "App.ActiveDocument.getObject('%s')";
                    cmds.push_back(str.str());
                }
                continue;
            }
            auto linksProp = dynamic_cast<App::PropertyLinkList*>(prop);
            if(linksProp) {
                if(prop->testStatus(App::Property::Immutable) || parentObj->isReadOnly(prop))
                    continue;
                int i;
                if(linksProp->find(obj->getNameInDocument(),&i)) {
                    std::ostringstream str;
                    str << Gui::Command::getObjectCmd(parentObj) <<'.'
                        << propName << "={"<<i<<":App.ActiveDocument.getObject('%s')}";
                    cmds.push_back(str.str());
                }
                continue;
            }
        }
        if(cmds.size()) {
            int vis = parentObj->isElementVisible(obj->getNameInDocument());
            if(vis>=0) {
                if(vis == 0)
                    cmds.push_back(Gui::Command::getObjectCmd(parentObj)+".setElementVisible('%s',False)");
                cmds.push_back("App.ActiveDocument.getObject('%s').Visibility = False");
            }
            replaceCmds[std::make_pair(parentObj,obj)].swap(cmds);
        }else 
            FC_WARN("skip '" << obj->getFullName() << "': no link property found");
    }

    // Collect all realtive links in all documents, so that we can make
    // corrections for those that are affected by this operation.
    std::map<App::Document*, std::vector<std::pair<std::string,App::PropertyXLink*> > > links;
    for(auto pDoc : App::GetApplication().getDocuments()) {
        for(auto pObj : pDoc->getObjects()) {
            if(!pObj->getNameInDocument())
                continue;
            std::map<std::string, App::Property*> props;
            pObj->getPropertyMap(props);
            for(auto &v : props) {
                auto &propName = v.first;
                auto prop = v.second;
                auto link = dynamic_cast<App::PropertyXLink*>(prop);
                if(!link || link->testStatus(App::Property::Immutable) || pObj->isReadOnly(link))
                    continue;
                auto linked = link->getValue();
                if(!linked || !linked->getNameInDocument())
                    continue;
                auto sub = link->getSubName();
                if(!sub || !sub[0])
                    continue;
                links[pDoc].push_back(std::make_pair(propName,link));
            }
        }
    }

    std::map<App::DocumentObject*,std::string> replacedObjs;
    std::vector<std::string> changeCmds;

    // now, do actual operation
    const char *transactionName = unlink?"Unlink":"Replace with link";
    App::GetApplication().setActiveTransaction(transactionName);
    try {
        for(auto &v : replaceCmds) {
            auto parent = v.first.first;
            auto obj =  v.first.second;
            auto &rcmds = v.second;
            auto it  = replacedObjs.find(obj);
            std::string name;
            // create the link if not done yet
            if(it!=replacedObjs.end())
                name = it->second;
            else if(unlink) {
                auto linked = obj->getLinkedObject(false);
                if(!linked || linked == obj || !linked->getNameInDocument())
                    continue;
                name = linked->getNameInDocument();
                it = replacedObjs.insert(std::make_pair(obj,name)).first;
            }else{
                name = doc->getUniqueObjectName("Link");
                Command::doCommand(Command::Doc,
                    "App.ActiveDocument.addObject('App::Link','%s')."
                    "setLink(App.ActiveDocument.getObject('%s'))",
                    name.c_str(),obj->getNameInDocument());
                setLinkLabel(obj,doc->getName(),name.c_str());
                if(obj->getPropertyByName("Placement"))
                    Command::doCommand(Command::Doc,
                        "App.ActiveDocument.getObject('%s').Placement = "
                        "App.ActiveDocument.getObject('%s').Placement",
                        name.c_str(),obj->getNameInDocument());
                else
                    Command::doCommand(Command::Doc,
                        "App.ActiveDocument.getObject('%s').LinkTransform = True", name.c_str());
                it = replacedObjs.insert(std::make_pair(obj,name)).first;
            }

            // do the replacement operation
            for(auto &cmd : rcmds)
                Command::doCommand(Command::Doc,cmd.c_str(),name.c_str());

            // generate command for relative link correction

            std::string objName(obj->getNameInDocument());
            objName += '.';
            std::string subName(parent->getNameInDocument());
            subName += '.';
            auto offset = subName.size();
            subName += objName;
            for(auto &v : links) {
                auto pDoc = v.first;
                for(auto &vv : v.second) {
                    auto &propName = vv.first;
                    auto link = vv.second;
                    auto pObj = static_cast<App::DocumentObject*>(link->getContainer());
                    auto linked = link->getValue();
                    if(!linked || !linked->getNameInDocument())
                        continue;
                    auto sub = link->getSubName();
                    if(linked == parent) {
                        if(boost::algorithm::starts_with(sub,objName.c_str())) {
                            std::ostringstream str;
                            str << "App.getDocument('"<<pDoc->getName()<<"').getObject('"
                                << pObj->getNameInDocument() << "')." << propName
                                << "=(App.ActiveDocument.getObject('"
                                << parent->getNameInDocument() << "'),'" 
                                << name << '.' << (sub+objName.size()) << "')";
                            changeCmds.push_back(str.str());
                        }
                        continue;
                    }
                    auto pos = strstr(sub,subName.c_str());
                    if(!pos) continue;
                    if(pos!=sub && pos[-1]!='.') {
                        FC_LOG("subname mismatch " << linked->getFullName() << '.' << sub);
                        continue;
                    }
                    std::string tmpSub(sub,pos-sub+offset);
                    auto subObj = linked->getSubObject(tmpSub.c_str());
                    if(subObj != parent) {
                        FC_LOG("sub object mismatch " << linked->getFullName() << '.' << sub);
                        continue;
                    }
                    std::ostringstream str;
                    str << "App.getDocument('" << pDoc->getName() << "').getObject('"
                        << pObj->getNameInDocument() << "')." << propName
                        << "=(App.ActiveDocument.getObject('"<< linked->getNameInDocument() 
                        << "'),'" << tmpSub << '.' << name << '.' << (pos+offset) << "')";
                    changeCmds.push_back(str.str());
                }
            }
        }

        // run the command for realtive link correction
        for(auto &cmd : changeCmds)
            Command::runCommand(Gui::Command::Doc, cmd.c_str());

        App::GetApplication().closeActiveTransaction();

    } catch (const Base::Exception& e) {
        auto title = unlink?QObject::tr("Unlink failed"):QObject::tr("Replace link failed");
        QMessageBox::critical(getMainWindow(), title, QString::fromLatin1(e.what()));
        App::GetApplication().closeActiveTransaction(true);
        e.ReportException();
        return;
    }
}

static bool linkConvertible(bool unlink) {
    App::Document *doc = App::GetApplication().getActiveDocument();
    if(!doc) return false;

    int count = 0;
    for(auto sel : TreeWidget::getSelection(doc)) {
        auto parent = sel.first;
        if(!parent) return false;
        App::DocumentObject *parentObj = parent->getObject();
        auto obj = sel.second->getObject();
        if(obj->getDocument()!=parentObj->getDocument()) 
            return false;
        if(unlink) {
            auto linked = obj->getLinkedObject(false);
            if(!linked || linked == obj)
                return false;
        }
        std::map<std::string, App::Property*> props;
        parentObj->getPropertyMap(props);
        bool found = false;
        for(auto &v : props) {
            auto linkProp = dynamic_cast<App::PropertyLink*>(v.second);
            if(linkProp) {
                if(v.second->testStatus(App::Property::Immutable) || parentObj->isReadOnly(v.second))
                    continue;
                if(linkProp->getValue()==obj) {
                    found = true;
                    break;
                }
                continue;
            }
            auto linksProp = dynamic_cast<App::PropertyLinkList*>(v.second);
            if(linksProp) {
                if(v.second->testStatus(App::Property::Immutable) || parentObj->isReadOnly(v.second))
                    continue;
                if(linksProp->find(obj->getNameInDocument())) {
                    found = true;
                    break;
                }
                continue;
            }
        }
        if(!found) return false;
        ++count;
    }
    return count!=0;
}

////////////////////////////////////////////////////////////////////////////////////////////

DEF_STD_CMD_A(StdCmdLinkReplace)

StdCmdLinkReplace::StdCmdLinkReplace()
  : Command("Std_LinkReplace")
{
    sGroup        = QT_TR_NOOP("Link");
    sMenuText     = QT_TR_NOOP("Replace with link");
    sToolTipText  = QT_TR_NOOP("Replace the selected object(s) with link");
    sWhatsThis    = "Std_LinkReplace";
    sStatusTip    = sToolTipText;
    eType         = AlterDoc;
    sPixmap       = "LinkReplace";
}

bool StdCmdLinkReplace::isActive() {
    return linkConvertible(false);
}

void StdCmdLinkReplace::activated(int) {
    linkConvert(false);
}

////////////////////////////////////////////////////////////////////////////////////////////

DEF_STD_CMD_A(StdCmdLinkUnlink)

StdCmdLinkUnlink::StdCmdLinkUnlink()
  : Command("Std_LinkUnlink")
{
    sGroup        = QT_TR_NOOP("Link");
    sMenuText     = QT_TR_NOOP("Unlink");
    sToolTipText  = QT_TR_NOOP("Strip on level of link");
    sWhatsThis    = "Std_LinkUnlink";
    sStatusTip    = sToolTipText;
    eType         = AlterDoc;
    sPixmap       = "Unlink";
}

bool StdCmdLinkUnlink::isActive() {
    return linkConvertible(true);
}

void StdCmdLinkUnlink::activated(int) {
    linkConvert(true);
}

////////////////////////////////////////////////////////////////////////////////////////////

DEF_STD_CMD_A(StdCmdLinkImport)

StdCmdLinkImport::StdCmdLinkImport()
  : Command("Std_LinkImport")
{
    sGroup        = QT_TR_NOOP("Link");
    sMenuText     = QT_TR_NOOP("Import links");
    sToolTipText  = QT_TR_NOOP("Import selected external link(s)");
    sWhatsThis    = "Std_LinkImport";
    sStatusTip    = sToolTipText;
    eType         = AlterDoc;
    sPixmap       = "LinkImport";
}

static std::map<App::Document*, std::vector<App::DocumentObject*> > getLinkImportSelections() 
{
    std::map<App::Document*, std::vector<App::DocumentObject*> > objMap;
    for(auto &sel : Selection().getCompleteSelection(false)) {
        auto obj = sel.pObject->resolve(sel.SubName);
        if(!obj || !obj->getNameInDocument())
            continue;
        for(auto o : obj->getOutList()) {
            if(o && o->getNameInDocument() && o->getDocument()!=obj->getDocument()) {
                objMap[obj->getDocument()].push_back(obj);
                break;
            }
        }
    }
    return objMap;
}

bool StdCmdLinkImport::isActive() {
    return !getLinkImportSelections().empty();
}

void StdCmdLinkImport::activated(int) {
    App::GetApplication().setActiveTransaction("Import links");
    try {
        for(auto &v : getLinkImportSelections()) {
            auto doc = v.first;
            // TODO: Is it possible to do this using interpreter?
            for(auto obj : doc->importLinks(v.second))
                obj->Visibility.setValue(false);
        }
        App::GetApplication().closeActiveTransaction();
    }catch (const Base::Exception& e) {
        QMessageBox::critical(getMainWindow(), QObject::tr("Failed to import links"),
            QString::fromLatin1(e.what()));
        App::GetApplication().closeActiveTransaction(true);
        e.ReportException();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////

DEF_STD_CMD_A(StdCmdLinkImportAll)

StdCmdLinkImportAll::StdCmdLinkImportAll()
  : Command("Std_LinkImportAll")
{
    sGroup        = QT_TR_NOOP("Link");
    sMenuText     = QT_TR_NOOP("Import all links");
    sToolTipText  = QT_TR_NOOP("Import all links of the active document");
    sWhatsThis    = "Std_LinkImportAll";
    sStatusTip    = sToolTipText;
    eType         = AlterDoc;
    sPixmap       = "LinkImportAll";
}

bool StdCmdLinkImportAll::isActive() {
    auto doc = App::GetApplication().getActiveDocument();
    return doc && App::PropertyXLink::hasXLink(doc);
}

void StdCmdLinkImportAll::activated(int) {
    App::GetApplication().setActiveTransaction("Import all links");
    try {
        std::ostringstream str;
        str << "for _o in App.ActiveDocument.importLinks():" << std::endl
                << "  _o.ViewObject.Visibility=False" << std::endl
            << "_o = None";
        Command::runCommand(Command::Doc,str.str().c_str());
        App::GetApplication().closeActiveTransaction();
    } catch (const Base::Exception& e) {
        QMessageBox::critical(getMainWindow(), QObject::tr("Failed to import all links"),
            QString::fromLatin1(e.what()));
        App::GetApplication().closeActiveTransaction(true);
        e.ReportException();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////

DEF_STD_CMD_A(StdCmdLinkSelectLinked)

StdCmdLinkSelectLinked::StdCmdLinkSelectLinked()
  : Command("Std_LinkSelectLinked")
{
    sGroup        = QT_TR_NOOP("Link");
    sMenuText     = QT_TR_NOOP("Select linked object");
    sToolTipText  = QT_TR_NOOP("Select the linked object");
    sWhatsThis    = "Std_LinkSelectLinked";
    sStatusTip    = sToolTipText;
    eType         = AlterSelection;
    sPixmap       = "LinkSelect";
    sAccel        = "S, G";
}

static App::DocumentObject *getSelectedLink(bool finalLink, std::string *subname=0) {
    const auto &sels = Selection().getSelection("*",0,true);
    if(sels.empty())
        return 0;
    auto sobj = sels[0].pObject->getSubObject(sels[0].SubName);
    if(!sobj)
        return 0;
    auto linked = sobj->getLinkedObject(false);
    if(!linked)
        return 0;
    if(linked==sobj) {
        auto ext = sobj->getExtensionByType<App::LinkBaseExtension>(true);
        if(ext) {
            linked = ext->getTrueLinkedObject(false);
            if(linked && linked!=sobj) {
                if(finalLink) {
                    auto linkFinal = ext->getTrueLinkedObject(true);
                    if(linkFinal!=linked)
                        return linkFinal;
                    return 0;
                }
                return linked;
            }
        }
        if(sobj->getDocument()!=sels[0].pObject->getDocument()) {
            if(finalLink)
                return sobj;
            for(const char *dot=strchr(sels[0].SubName,'.');dot;dot=strchr(dot+1,'.')) {
                std::string sub(sels[0].SubName,dot+1-sels[0].SubName);
                auto obj = sels[0].pObject->getSubObject(sub.c_str());
                if(!obj)
                    return 0;
                obj = obj->getLinkedObject(true);
                if(obj->getDocument()!=sels[0].pObject->getDocument()) {
                    if(subname) {
                        *subname = std::string(dot+1);
                        return obj;
                    }
                }
            }
            return sobj;
        }
        return 0;
    }

    if(finalLink) {
        auto linkedFinal = sobj->getLinkedObject(true);
        if(linkedFinal == linked)
            return 0;
        return linkedFinal;
    }
    return linked;
}

bool StdCmdLinkSelectLinked::isActive() {
    return getSelectedLink(false)!=0;
}

void StdCmdLinkSelectLinked::activated(int)
{
    std::string subname;
    auto linked = getSelectedLink(false,&subname);
    if(!linked){
        FC_WARN("invalid selection");
        return;
    }
    Selection().selStackPush();
    Selection().clearCompleteSelection();
    if(subname.size()) {
        Selection().addSelection(linked->getDocument()->getName(),linked->getNameInDocument(),subname.c_str());
        auto doc = Application::Instance->getDocument(linked->getDocument());
        if(doc) {
            auto vp = dynamic_cast<ViewProviderDocumentObject*>(Application::Instance->getViewProvider(linked));
            doc->setActiveView(vp);
        }
    } else {
        for(auto tree : getMainWindow()->findChildren<TreeWidget*>())
            tree->selectLinkedObject(linked);
    }
    Selection().selStackPush();
}

////////////////////////////////////////////////////////////////////////////////////////////

DEF_STD_CMD_A(StdCmdLinkSelectLinkedFinal)

StdCmdLinkSelectLinkedFinal::StdCmdLinkSelectLinkedFinal()
  : Command("Std_LinkSelectLinkedFinal")
{
    sGroup        = QT_TR_NOOP("Link");
    sMenuText     = QT_TR_NOOP("Select the deepest linked object");
    sToolTipText  = QT_TR_NOOP("Select the deepest linked object");
    sWhatsThis    = "Std_LinkSelectLinkedFinal";
    sStatusTip    = sToolTipText;
    eType         = AlterSelection;
    sPixmap       = "LinkSelectFinal";
    sAccel        = "S, D";
}

bool StdCmdLinkSelectLinkedFinal::isActive() {
    return getSelectedLink(true)!=0;
}

void StdCmdLinkSelectLinkedFinal::activated(int) {
    auto linked = getSelectedLink(true);
    if(!linked){
        FC_WARN("invalid selection");
        return;
    }
    Selection().selStackPush();
    Selection().clearCompleteSelection();
    for(auto tree : getMainWindow()->findChildren<TreeWidget*>())
        tree->selectLinkedObject(linked);
    Selection().selStackPush();
}

////////////////////////////////////////////////////////////////////////////////////////////

DEF_STD_CMD_A(StdCmdLinkSelectAllLinks)

StdCmdLinkSelectAllLinks::StdCmdLinkSelectAllLinks()
  : Command("Std_LinkSelectAllLinks")
{
    sGroup        = QT_TR_NOOP("Link");
    sMenuText     = QT_TR_NOOP("Select all links");
    sToolTipText  = QT_TR_NOOP("Select all links to the current selected object");
    sWhatsThis    = "Std_LinkSelectAllLinks";
    sStatusTip    = sToolTipText;
    eType         = AlterSelection;
    sPixmap       = "LinkSelectAll";
}

bool StdCmdLinkSelectAllLinks::isActive() {
    const auto &sels = Selection().getSelection("*",true,true);
    if(sels.empty())
        return false;
    return App::GetApplication().hasLinksTo(sels[0].pObject);
}

void StdCmdLinkSelectAllLinks::activated(int)
{
    auto sels = Selection().getSelection("*",true,true);
    if(sels.empty())
        return;
    Selection().selStackPush();
    Selection().clearCompleteSelection();
    for(auto tree : getMainWindow()->findChildren<TreeWidget*>())
        tree->selectAllLinks(sels[0].pObject);
    Selection().selStackPush();
}

//===========================================================================
// Instantiation
//===========================================================================


namespace Gui {

void CreateLinkCommands(void)
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();
    rcCmdMgr.addCommand(new StdCmdLinkSelectLinked());
    rcCmdMgr.addCommand(new StdCmdLinkSelectLinkedFinal());
    rcCmdMgr.addCommand(new StdCmdLinkSelectAllLinks());
    rcCmdMgr.addCommand(new StdCmdLinkMake());
    rcCmdMgr.addCommand(new StdCmdLinkMakeRelative());
    rcCmdMgr.addCommand(new StdCmdLinkMakeGroup());
    rcCmdMgr.addCommand(new StdCmdLinkReplace());
    rcCmdMgr.addCommand(new StdCmdLinkUnlink());
    rcCmdMgr.addCommand(new StdCmdLinkImport());
    rcCmdMgr.addCommand(new StdCmdLinkImportAll());
}

} // namespace Gui

