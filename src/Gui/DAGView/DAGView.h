// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2015 Thomas Anderson <blobfish[at]gmx.com>              *
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

#pragma once

#include <memory>
#include <QGraphicsView>
#include <fastsignals/signal.h>

#include <Gui/DockWindow.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>

#include "DAGModel.h"


namespace Gui
{
namespace DAG
{
//! @brief view for DAG viewer
class View: public QGraphicsView, public SelectionObserver
{
    Q_OBJECT
public:
    explicit View(QWidget* parentIn = nullptr);
    ~View() override;

public Q_SLOTS:
    void awakeSlot();  //!< hooked up to event dispatcher for update when idle.

private:
    void onSelectionChanged(const SelectionChanges& msg) override;

    void slotActiveDocument(const Gui::Document& documentIn);
    void slotDeleteDocument(const Gui::Document& documentIn);

    using ModelMap = std::map<const Gui::Document*, std::shared_ptr<Model>>;
    ModelMap modelMap;
    fastsignals::scoped_connection conActive;
    fastsignals::scoped_connection conDelete;
};

//! @brief dock window for DAG viewer
class DockWindow: public Gui::DockWindow
{
    Q_OBJECT
public:
    explicit DockWindow(Gui::Document* gDocumentIn = nullptr, QWidget* parent = nullptr);
    ~DockWindow() override = default;

private:
    View* dagView;
};
}  // namespace DAG
}  // namespace Gui
