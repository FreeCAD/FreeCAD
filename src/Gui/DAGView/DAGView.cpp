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
#endif

#include <sstream>

#include <Base/Console.h>

#include <App/Document.h>
#include <Gui/Document.h>
#include <Gui/Application.h>

#include "DAGModel.h"
#include "DAGView.h"

using namespace Gui;
using namespace DAG;

DAG::DockWindow::DockWindow(Gui::Document* gDocumentIn, QWidget* parent): Gui::DockWindow(gDocumentIn, parent)
{
  dagView = new View(this);
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(dagView);
  this->setLayout(layout);
}

View::View(QWidget* parentIn): QGraphicsView(parentIn)
{
  this->setRenderHint(QPainter::Antialiasing, true);
  this->setRenderHint(QPainter::TextAntialiasing, true);
  Application::Instance->signalActiveDocument.connect(boost::bind(&View::slotActiveDocument, this, _1));
  Application::Instance->signalDeleteDocument.connect(boost::bind(&View::slotDeleteDocument, this, _1));
  
  //just update the dagview when the gui process is idle.
  connect(QAbstractEventDispatcher::instance(), SIGNAL(awake()), this, SLOT(awakeSlot()));
}

View::~View()
{
  Application::Instance->signalActiveDocument.disconnect(boost::bind(&View::slotActiveDocument, this, _1));
  Application::Instance->signalDeleteDocument.disconnect(boost::bind(&View::slotDeleteDocument, this, _1));
}

void View::slotActiveDocument(const Document &documentIn)
{
  ModelMap::const_iterator it = modelMap.find(&documentIn);
  if (it == modelMap.end())
  {
    ModelMap::value_type entry(std::make_pair(&documentIn, std::shared_ptr<Model>(new Model(this, documentIn))));
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
  ModelMap::iterator it = modelMap.find(&documentIn);
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
  //dispatch to appropriate document.
  ModelMap::iterator it;
  for (auto it = modelMap.begin(); it != modelMap.end(); ++it)
  {
    if (std::string(it->first->getDocument()->getName()) == std::string(msg.pDocName))
    {
      it->second->selectionChanged(msg);
      return;
    }
  }
  
  //FIXME: why am I getting a spontaneous event with an empty name?
  //also getting events after the document has been removed from modelMap.
  //just ignore for now.
//   std::ostringstream stream;
//   stream << std::endl << "couldn't find document of name: " << std::string(msg.pDocName) << std::endl << std::endl;
//   Base::Console().Warning(stream.str().c_str());
//   assert(0); //no document of name.
}



#include "moc_DAGView.cpp"
