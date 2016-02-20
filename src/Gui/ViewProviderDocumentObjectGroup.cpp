/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QPixmap>
#endif

#include <App/DocumentObjectGroup.h>
#include <App/Document.h>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "ViewProviderDocumentObjectGroup.h"
#include "Application.h"
#include "Command.h"
#include "BitmapFactory.h"
#include "Document.h"
#include "Tree.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"


using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderDocumentObjectGroup, Gui::ViewProviderDocumentObject)


/**
 * Creates the view provider for an object group.
 */
ViewProviderDocumentObjectGroup::ViewProviderDocumentObjectGroup() : visible(false)
{
#if 0
    setDefaultMode(SO_SWITCH_ALL);
#endif
}

ViewProviderDocumentObjectGroup::~ViewProviderDocumentObjectGroup()
{
}

/**
 * Whenever a property of the group gets changed then the same property of all
 * associated view providers of the objects of the object group get changed as well.
 */
void ViewProviderDocumentObjectGroup::onChanged(const App::Property* prop)
{
    ViewProviderDocumentObject::onChanged(prop);
}

void ViewProviderDocumentObjectGroup::attach(App::DocumentObject *pcObj)
{
    ViewProviderDocumentObject::attach(pcObj);
}

void ViewProviderDocumentObjectGroup::updateData(const App::Property* prop)
{
#if 0
    if (prop->getTypeId() == App::PropertyLinkList::getClassTypeId()) {
        std::vector<App::DocumentObject*> obj =
            static_cast<const App::PropertyLinkList*>(prop)->getValues();
        Gui::Document* doc = Gui::Application::Instance->getDocument
            (&this->getObject()->getDocument());
        MDIView* mdi = doc->getActiveView();
        if (mdi && mdi->isDerivedFrom(View3DInventor::getClassTypeId())) {
            View3DInventorViewer* view = static_cast<View3DInventor*>(mdi)->getViewer();
            SoSeparator* scene_graph = static_cast<SoSeparator*>(view->getSceneGraph());
            std::vector<ViewProvider*> current_nodes;
            for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it)
                current_nodes.push_back(doc->getViewProvider(*it));
            std::sort(current_nodes.begin(), current_nodes.end());
            std::sort(this->nodes.begin(), this->nodes.end());
            // get the removed views
            std::vector<ViewProvider*> diff_1, diff_2;
            std::back_insert_iterator<std::vector<ViewProvider*> > biit(diff_2);
            std::set_difference(this->nodes.begin(), this->nodes.end(), 
                                current_nodes.begin(), current_nodes.end(), biit);
            diff_1 = diff_2;
            diff_2.clear();
            // get the added views
            std::set_difference(current_nodes.begin(), current_nodes.end(),
                                this->nodes.begin(), this->nodes.end(), biit);
            this->nodes = current_nodes;
            // move from root node to switch
            for (std::vector<ViewProvider*>::iterator it = diff_1.begin(); it != diff_1.end(); ++it) {
                view->addViewProviderToGroup(*it, scene_graph);
                view->removeViewProviderFromGroup(*it, this->pcModeSwitch);
            }
            // move from switch node to root node
            for (std::vector<ViewProvider*>::iterator it = diff_2.begin(); it != diff_2.end(); ++it) {
                view->addViewProviderToGroup(*it, this->pcModeSwitch);
                view->removeViewProviderFromGroup(*it, scene_graph);
            }
        }
    }
#endif
}

std::vector<App::DocumentObject*> ViewProviderDocumentObjectGroup::claimChildren(void)const
{
    return std::vector<App::DocumentObject*>(static_cast<App::DocumentObjectGroup*>(getObject())->Group.getValues());
}

bool ViewProviderDocumentObjectGroup::canDragObjects() const
{
    return true;
}

void ViewProviderDocumentObjectGroup::dragObject(App::DocumentObject* obj)
{
    static_cast<App::DocumentObjectGroup*>(getObject())->removeObject(obj);
}

bool ViewProviderDocumentObjectGroup::canDropObjects() const
{
    return true;
}

void ViewProviderDocumentObjectGroup::dropObject(App::DocumentObject* obj)
{
    static_cast<App::DocumentObjectGroup*>(getObject())->addObject(obj);
}

std::vector<std::string> ViewProviderDocumentObjectGroup::getDisplayModes(void) const
{
    // empty
    return std::vector<std::string>();
}

bool ViewProviderDocumentObjectGroup::onDelete(const std::vector<std::string> &)
{
    Gui::Command::doCommand(Gui::Command::Doc,"App.getDocument(\"%s\").getObject(\"%s\").removeObjectsFromDocument()"
                                     ,getObject()->getDocument()->getName(), getObject()->getNameInDocument());
    return true;
}


void ViewProviderDocumentObjectGroup::hide(void)
{
    // when reading the Visibility property from file then do not hide the
    // objects of this group because they have stored their visibility status, too
    if (!Visibility.testStatus(App::Property::User1) && this->visible) {
        App::DocumentObject * group = getObject();
        if (group && group->getTypeId().isDerivedFrom(App::DocumentObjectGroup::getClassTypeId())) {
            const std::vector<App::DocumentObject*> & links = static_cast<App::DocumentObjectGroup*>
                (group)->Group.getValues();
            Gui::Document* doc = Application::Instance->getDocument(group->getDocument());
            for (std::vector<App::DocumentObject*>::const_iterator it = links.begin(); it != links.end(); ++it) {
                ViewProvider* view = doc->getViewProvider(*it);
                if (view) view->hide();
            }
        }
    }

    ViewProviderDocumentObject::hide();
    this->visible = false;
}

void ViewProviderDocumentObjectGroup::show(void)
{
    // when reading the Visibility property from file then do not hide the
    // objects of this group because they have stored their visibility status, too
    if (!Visibility.testStatus(App::Property::User1) && !this->visible) {
        App::DocumentObject * group = getObject();
        if (group && group->getTypeId().isDerivedFrom(App::DocumentObjectGroup::getClassTypeId())) {
            const std::vector<App::DocumentObject*> & links = static_cast<App::DocumentObjectGroup*>
                (group)->Group.getValues();
            Gui::Document* doc = Application::Instance->getDocument(group->getDocument());
            for (std::vector<App::DocumentObject*>::const_iterator it = links.begin(); it != links.end(); ++it) {
                ViewProvider* view = doc->getViewProvider(*it);
                if (view) view->show();
            }
        }
    }

    ViewProviderDocumentObject::show();
    this->visible = true;
}

bool ViewProviderDocumentObjectGroup::isShow(void) const
{
    return Visibility.getValue();
}

void ViewProviderDocumentObjectGroup::Restore(Base::XMLReader &reader)
{
    Visibility.setStatus(App::Property::User1, true); // tmp. set
    ViewProviderDocumentObject::Restore(reader);
    Visibility.setStatus(App::Property::User1, false); // unset
}

/**
 * Extracts the associated view providers of the objects of the associated object group group. 
 */
void ViewProviderDocumentObjectGroup::getViewProviders(std::vector<ViewProviderDocumentObject*>& vp) const
{
    App::DocumentObject* doc = getObject();
    if (doc->getTypeId().isDerivedFrom(App::DocumentObjectGroup::getClassTypeId())) {
        Gui::Document* gd = Application::Instance->getDocument(doc->getDocument());
        App::DocumentObjectGroup* grp = (App::DocumentObjectGroup*)doc;
        std::vector<App::DocumentObject*> obj = grp->getObjects();
        for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it) {
            ViewProvider* v = gd->getViewProvider(*it);
            if (v && v->getTypeId().isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
                vp.push_back((ViewProviderDocumentObject*)v);
        }
    }
}

/**
 * Returns the pixmap for the list item.
 */
QIcon ViewProviderDocumentObjectGroup::getIcon() const
{
    QIcon groupIcon;
    groupIcon.addPixmap(QApplication::style()->standardPixmap(QStyle::SP_DirClosedIcon),
                        QIcon::Normal, QIcon::Off);
    groupIcon.addPixmap(QApplication::style()->standardPixmap(QStyle::SP_DirOpenIcon),
                        QIcon::Normal, QIcon::On);
    return groupIcon;
}


// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderDocumentObjectGroupPython, Gui::ViewProviderDocumentObjectGroup)
/// @endcond

// explicit template instantiation
template class GuiExport ViewProviderPythonFeatureT<ViewProviderDocumentObjectGroup>;
}
