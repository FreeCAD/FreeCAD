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

#ifndef DAGVIEW_H
#define DAGVIEW_H

#include <memory>

#include <QGraphicsView>

#include <Gui/DockWindow.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>

#include "DAGModel.h"

namespace Gui
{
  namespace DAG
  {
    //! @brief view for DAG viewer
    class View : public QGraphicsView, public SelectionObserver
    {
      Q_OBJECT
    public:
      View(QWidget *parentIn = 0);
      virtual ~View() override;
      
    public Q_SLOTS:
      void awakeSlot(); //!< hooked up to event dispatcher for update when idle.
      
    private:
      virtual void onSelectionChanged(const SelectionChanges& msg) override;
      
      void slotActiveDocument(const Gui::Document &documentIn);
      void slotDeleteDocument(const Gui::Document &documentIn);
      
      typedef std::map<const Gui::Document*, std::shared_ptr<Model> > ModelMap;
      ModelMap modelMap;
    };
    
    //! @brief dock window for DAG viewer
    class DockWindow : public Gui::DockWindow
    {
        Q_OBJECT
    public:
        DockWindow(Gui::Document* gDocumentIn = 0, QWidget *parent = 0);
        ~DockWindow(){};

    private:
        View *dagView;
    };
  }
}

#endif // DAGVIEW_H
