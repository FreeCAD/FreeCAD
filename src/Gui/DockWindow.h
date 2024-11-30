/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DOCKWINDOW_H
#define GUI_DOCKWINDOW_H

#include <Gui/View.h>
#include <QWidget>


namespace Gui {
class MDIView;
class Application;

/** Base class of all dockable windows belonging to a document
 *  there are two ways of belonging to a document. The
 *  first way is to a fixed one. The second way is to always
 *  belong to the active document, that means switching every time
 *  the active document is changing. It also means that the view
 *  belongs sometimes to no document at all!
 *  @see TreeView
 *  @see Gui::Document
 *  @see Application
 *  @author Jürgen Riegel
 */
class GuiExport DockWindow : public QWidget, public BaseView
{
  Q_OBJECT

public:
  /** View constructor
   * Attach the view to the given document. If the document is 0
   * the view will attach to the active document. Be aware there isn't
   * always an active document available!
   */
  explicit DockWindow ( Gui::Document* pcDocument=nullptr, QWidget *parent=nullptr );
  /** View destructor
   * Detach the view from the document, if attached.
   */
  ~DockWindow() override;

  /** @name methods to override
   */
  //@{
  /// get called when the document is updated
  void onUpdate() override{}
  /// returns the name of the view (important for messages)
  const char *getName() const override { return "DockWindow"; }
  /// Message handler
  bool onMsg(const char* ,const char** ) override{ return false; }
  /// Message handler test
  bool onHasMsg(const char*) const override { return false; }
  /// overwrite when checking on close state
  bool canClose() override{return true;}
  //@}

Q_SIGNALS:
  /// sends a message to the document
  void sendCloseView(Gui::MDIView* theView);
};

} // namespace Gui

#endif // GUI_DOCKWINDOW_H
