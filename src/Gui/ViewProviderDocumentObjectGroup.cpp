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
# include <QMessageBox>
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

#else
    Q_UNUSED(prop);
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
    Gui::Command::doCommand(Gui::Command::Doc,"App.getDocument(\"%s\").getObject(\"%s\").removeObject("
            "App.getDocument(\"%s\").getObject(\"%s\"))",
            getObject()->getDocument()->getName(), getObject()->getNameInDocument(), 
            obj->getDocument()->getName(), obj->getNameInDocument() );
}

bool ViewProviderDocumentObjectGroup::canDropObjects() const
{
    return true;
}

void ViewProviderDocumentObjectGroup::dropObject(App::DocumentObject* obj)
{
    Gui::Command::doCommand(Gui::Command::Doc,"App.getDocument(\"%s\").getObject(\"%s\").addObject("
            "App.getDocument(\"%s\").getObject(\"%s\"))",
            getObject()->getDocument()->getName(), getObject()->getNameInDocument(), 
            obj->getDocument()->getName(), obj->getNameInDocument() );
}

std::vector<std::string> ViewProviderDocumentObjectGroup::getDisplayModes(void) const
{
    // empty
    return std::vector<std::string>();
}

bool ViewProviderDocumentObjectGroup::onDelete(const std::vector<std::string> &)
{
    App::DocumentObjectGroup *group = static_cast<App::DocumentObjectGroup *> (getObject());
    // If the group is nonempty ask the user if he wants to delete it's content
    if ( group->Group.getSize () ) {
        QMessageBox::StandardButton choice = 
            QMessageBox::question ( 0, QObject::tr ( "Delete group content?" ), 
                QObject::tr ( "The %1 is not empty, delete it's content as well?")
                    .arg ( QString::fromUtf8 ( group->Label.getValue () ) ), 
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );

        if ( choice == QMessageBox::Yes ) {
            Gui::Command::doCommand(Gui::Command::Doc,
                    "App.getDocument(\"%s\").getObject(\"%s\").removeObjectsFromDocument()"
                    ,getObject()->getDocument()->getName(), getObject()->getNameInDocument());
        }
    }
    return true;
}

bool ViewProviderDocumentObjectGroup::allowDrop(const std::vector<const App::DocumentObject*> &objList,
                                                Qt::KeyboardModifiers keys,
                                                Qt::MouseButtons mouseBts,
                                                const QPoint &pos)
{
    Q_UNUSED(keys);
    Q_UNUSED(mouseBts);
    Q_UNUSED(pos);

    for( std::vector<const App::DocumentObject*>::const_iterator it = objList.begin();it!=objList.end();++it)
        if ((*it)->getTypeId().isDerivedFrom(App::DocumentObjectGroup::getClassTypeId())) {
            if (static_cast<App::DocumentObjectGroup*>(getObject())->isChildOf(
                static_cast<const App::DocumentObjectGroup*>(*it))) {
                return false;
            }
        }

    return true;
}

void ViewProviderDocumentObjectGroup::drop(const std::vector<const App::DocumentObject*> &objList,
                                           Qt::KeyboardModifiers keys,
                                           Qt::MouseButtons mouseBts,
                                           const QPoint &pos)
{
    Q_UNUSED(keys);
    Q_UNUSED(mouseBts);
    Q_UNUSED(pos);

    // Open command
    App::DocumentObjectGroup* grp = static_cast<App::DocumentObjectGroup*>(getObject());
    App::Document* doc = grp->getDocument();
    Gui::Document* gui = Gui::Application::Instance->getDocument(doc);
    gui->openCommand("Move object");
    for( std::vector<const App::DocumentObject*>::const_iterator it = objList.begin();it!=objList.end();++it) {
        // get document object
        const App::DocumentObject* obj = *it;
        const App::DocumentObject* par = App::DocumentObjectGroup::getGroupOfObject(obj);
        if (par) {
            // allow an object to be in one group only
            QString cmd;
            cmd = QString::fromLatin1("App.getDocument(\"%1\").getObject(\"%2\").removeObject("
                              "App.getDocument(\"%1\").getObject(\"%3\"))")
                              .arg(QString::fromLatin1(doc->getName()))
                              .arg(QString::fromLatin1(par->getNameInDocument()))
                              .arg(QString::fromLatin1(obj->getNameInDocument()));
            Gui::Command::runCommand(Gui::Command::App, cmd.toUtf8());
        }

        // build Python command for execution
        QString cmd;
        cmd = QString::fromLatin1("App.getDocument(\"%1\").getObject(\"%2\").addObject("
                          "App.getDocument(\"%1\").getObject(\"%3\"))")
                          .arg(QString::fromLatin1(doc->getName()))
                          .arg(QString::fromLatin1(grp->getNameInDocument()))
                          .arg(QString::fromLatin1(obj->getNameInDocument()));

        Gui::Command::runCommand(Gui::Command::App, cmd.toUtf8());
    }
    gui->commitCommand();
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
