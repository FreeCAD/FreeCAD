/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

/*!
  \class SIM::Coin3D::Quarter::DragDropHandler DragDropHandler.h Quarter/devices/DragDropHandler.h

  \brief The DragDropHandler event filter provides drag and drop
  functionality to the QuarterWidget.
*/

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QStringList>
#include <QUrl>
#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoSeparator.h>

#include "QuarterWidget.h"
#include "eventhandlers/DragDropHandler.h"


namespace SIM { namespace Coin3D { namespace Quarter {

class DragDropHandlerP {
public:
  DragDropHandlerP(DragDropHandler * master) {
    this->master = master;
  }
  void dragEnterEvent(QDragEnterEvent * event);
  void dropEvent(QDropEvent * event);

  QStringList suffixes;
  DragDropHandler * master;
  QuarterWidget * quarterwidget;
};

}}} // namespace

#define PRIVATE(obj) obj->pimpl
#define PUBLIC(obj) obj->master

using namespace SIM::Coin3D::Quarter;

/*!
  Constructor

  \sa QObject::QObject(QObject *)
*/
DragDropHandler::DragDropHandler(QuarterWidget * parent)
  : QObject(parent)
{
  PRIVATE(this) = new DragDropHandlerP(this);
  PRIVATE(this)->quarterwidget = parent;
  assert(PRIVATE(this)->quarterwidget);
  PRIVATE(this)->suffixes << "iv" << "wrl";
}

DragDropHandler::~DragDropHandler()
{
  delete PRIVATE(this);
}

/*!
  Detects a QDragEnterEvent and if the event is the dropping of a
  valid Inventor or VRML it opens the file, reads in the scenegraph
  and calls setSceneGraph on the QuarterWidget
 */
bool
DragDropHandler::eventFilter(QObject *, QEvent * event)
{
  switch (event->type()) {
  case QEvent::DragEnter:
    PRIVATE(this)->dragEnterEvent(dynamic_cast<QDragEnterEvent *>(event));
    return true;
  case QEvent::Drop:
    PRIVATE(this)->dropEvent(dynamic_cast<QDropEvent *>(event));
    return true;
  default:
    return false;
  }
}

void
DragDropHandlerP::dragEnterEvent(QDragEnterEvent * event)
{
  const QMimeData * mimedata = event->mimeData();
  if (!mimedata->hasUrls() && !mimedata->hasText())
      return;

  if (mimedata->hasUrls()) {
    QFileInfo fileinfo(mimedata->urls().constFirst().path());
    QString suffix = fileinfo.suffix().toLower();
    if (!this->suffixes.contains(suffix)) {
        return;
    }
  }

  event->acceptProposedAction();
}

void
DragDropHandlerP::dropEvent(QDropEvent * event)
{
  const QMimeData * mimedata = event->mimeData();

  SoSeparator * root;
  SoInput in;
  QByteArray bytes;

  if (mimedata->hasUrls()) {
    QUrl url = mimedata->urls().constFirst();
    if (url.scheme().isEmpty() || url.scheme().toLower() == QString("file") ) {
      // attempt to open file
      if (!in.openFile(url.toLocalFile().toLatin1().constData()))
          return;
    }
  } else if (mimedata->hasText()) {
    /* FIXME 2007-11-09 preng: dropping text buffer does not work on Windows Vista. */
    bytes = mimedata->text().toUtf8();
    in.setBuffer((void *) bytes.constData(), bytes.size());
    if (!in.isValidBuffer())
        return;
  }

  // attempt to import it
  root = SoDB::readAll(&in);
  if (!root)
      return;

  // set new scenegraph
  this->quarterwidget->setSceneGraph(root);
  this->quarterwidget->viewport()->update();
}

#undef PRIVATE
#undef PUBLIC
