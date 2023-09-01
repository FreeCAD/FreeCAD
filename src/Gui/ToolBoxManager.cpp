/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QStyle>
# include <QToolBar>
# include <QToolButton>
#endif

#include "ToolBoxManager.h"
#include "Application.h"
#include "Command.h"
#include "ToolBarManager.h"
#include "ToolBox.h"


using namespace Gui;
using DockWnd::ToolBox;

ToolBoxManager* ToolBoxManager::_instance=nullptr;

ToolBoxManager* ToolBoxManager::getInstance()
{
    if ( !_instance )
        _instance = new ToolBoxManager;
    return _instance;
}

void ToolBoxManager::destruct()
{
    delete _instance;
    _instance = nullptr;
}

ToolBoxManager::ToolBoxManager() = default;

ToolBoxManager::~ToolBoxManager() = default;

void ToolBoxManager::setToolBox( DockWnd::ToolBox* tb )
{
    _toolBox = tb;
}

void ToolBoxManager::setup( ToolBarItem* toolBar ) const
{
    if ( !toolBar || !_toolBox )
        return; // empty tool bar

    int ct = _toolBox->count();
    for ( int i=0; i<ct; i++ )
    {
        // get always the first item widget
        QWidget* w = _toolBox->widget(0);
        _toolBox->removeItem(0);
        delete w;
    }

    CommandManager& mgr = Application::Instance->commandManager();
    QList<ToolBarItem*> items = toolBar->getItems();

    for ( auto item : items )
    {
        auto bar = new QToolBar();
        bar->setOrientation(Qt::Vertical);
        bar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        std::string toolbarName = item->command();
        bar->setObjectName(QString::fromLatin1(item->command().c_str()));
        bar->setWindowTitle(QObject::tr(toolbarName.c_str())); // i18n
        _toolBox->addItem( bar, bar->windowTitle() );

        QList<ToolBarItem*> subitems = item->getItems();
        for (auto subitem : subitems)
        {
            if ( subitem->command() == "Separator" ) {
                //bar->addSeparator();
            } else {
                mgr.addTo(subitem->command().c_str(), bar);
            }
        }

        // Now set the right size policy for each tool button
        QList<QToolButton*> tool = bar->findChildren<QToolButton*>();
        for (auto it : tool) {
            it->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            // When setting the horizontal size policy but no icon is set we use the following trick
            // to make the button text left aligned.
            QIcon icon = it->icon();
            if (icon.isNull())
            {
                // Create an icon filled with the button color
                int size = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
                QPixmap p(size, size);
                p.fill(Qt::transparent);
                it->setIcon(p);
            }
        }
    }
}

void ToolBoxManager::retranslate() const
{
    int ct = _toolBox->count();
    for (int i=0; i<ct; i++) {
        // get always the first item widget
        QWidget* w = _toolBox->widget(i);
        QByteArray toolbarName = w->objectName().toUtf8();
        w->setWindowTitle(QObject::tr(toolbarName.constData()));
        _toolBox->setItemText(i, w->windowTitle());
    }
}
