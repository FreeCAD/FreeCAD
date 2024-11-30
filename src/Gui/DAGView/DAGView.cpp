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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QAbstractEventDispatcher>
#include <QVBoxLayout>
#include <memory>
#endif

#include <Gui/Application.h>
#include <Gui/Document.h>

#include "DAGView.h"
#include "DAGModel.h"


using namespace Gui;
using namespace DAG;
namespace sp = std::placeholders;

DAG::DockWindow::DockWindow(Gui::Document* gDocumentIn, QWidget* parent): Gui::DockWindow(gDocumentIn, parent)
{
  dagView = new View(this);
  auto layout = new QVBoxLayout();
  layout->addWidget(dagView);
  this->setLayout(layout);
}

View::View(QWidget* parentIn): QGraphicsView(parentIn)
{
  this->setRenderHint(QPainter::Antialiasing, true);
  this->setRenderHint(QPainter::TextAntialiasing, true);
  //NOLINTBEGIN
  conActive = Application::Instance->signalActiveDocument.connect(std::bind(&View::slotActiveDocument, this, sp::_1));
  conDelete = Application::Instance->signalDeleteDocument.connect(std::bind(&View::slotDeleteDocument, this, sp::_1));
  //NOLINTEND

  //just update the dagview when the gui process is idle.
  connect(QAbstractEventDispatcher::instance(), &QAbstractEventDispatcher::awake,
          this, &View::awakeSlot);
}

View::~View() = default;

void View::slotActiveDocument(const Document &documentIn)
{
  if (Gui::Selection().hasSelection())
      return;
  ModelMap::const_iterator it = modelMap.find(&documentIn);
  if (it == modelMap.end())
  {
    ModelMap::value_type entry(std::make_pair(&documentIn, std::make_shared<Model>(this, documentIn)));
    modelMap.insert(entry);
    this->setScene(entry.second.get());
  }
  else
  {
    this->setScene(it->second.get());
  }
}

void View::slotDeleteDocument(const Document &documentIn)
{
  ModelMap::const_iterator it = modelMap.find(&documentIn);
  if (it != modelMap.end())
    modelMap.erase(it);
}

void View::awakeSlot()
{
  Model *model = dynamic_cast<Model *>(this->scene());
  if (model)
    model->awake();
}

void View::onSelectionChanged(const SelectionChanges& msg)
{
  switch(msg.Type) {
  case SelectionChanges::AddSelection:
  case SelectionChanges::RmvSelection:
  case SelectionChanges::SetSelection:
    if (!msg.pDocName || !msg.pDocName[0])
      return;
    break;
  case SelectionChanges::ClrSelection:
    if (!msg.pDocName || !msg.pDocName[0]) {
      for (auto &v : modelMap) {
        v.second->selectionChanged(msg);
      }
      return;
    }
    break;
  default:
    return;
  }
  auto doc = Gui::Application::Instance->getDocument(msg.pDocName);
  if (!doc)
      return;
  auto &model = modelMap[doc];
  if(!model)
    model = std::make_shared<Model>(this, *doc);
  this->setScene(model.get());
  model->selectionChanged(msg);
}



#include "moc_DAGView.cpp"
