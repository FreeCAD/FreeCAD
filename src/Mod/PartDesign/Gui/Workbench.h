/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PARTDESIGN_WORKBENCH_H
#define PARTDESIGN_WORKBENCH_H

#include <Gui/Workbench.h>
#include <Mod/PartDesign/PartDesignGlobal.h>

namespace Gui {

class MenuItem;
class Document;
class ViewProviderDocumentObject;

}

namespace PartDesignGui {

/**
 * @author Werner Mayer
 */
class PartDesignGuiExport Workbench : public Gui::StdWorkbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Workbench();
    ~Workbench() override;

      /** Run some actions when the workbench gets activated. */
    void activated() override;
    /** Run some actions when the workbench gets deactivated. */
    void deactivated() override;

    /// Add custom entries to the context menu
    void setupContextMenu(const char* recipient, Gui::MenuItem*) const override;

protected:
  Gui::MenuItem* setupMenuBar() const override;
  Gui::ToolBarItem* setupToolBars() const override;
  Gui::ToolBarItem* setupCommandBars() const override;

private:
   /// Refresh the Body's highlighting when a document becomes active
   void slotActiveDocument(const Gui::Document&);
   /// Refresh the highlighting. Migrate legacy documents on loading
   void slotFinishRestoreDocument(const App::Document&);
   /// Ensure that there are base planes and a body in a new document
   void slotNewDocument(const App::Document&);
   /// Update the ActivePartObject etc. when a document is closed
   void slotDeleteDocument(const App::Document&);
   // Add new objects to the body, if appropriate
   //void slotNewObject(const App::DocumentObject& obj);

   void _switchToDocument(const App::Document* doc);

private:
   boost::signals2::connection activeDoc;
   boost::signals2::connection createDoc;
   boost::signals2::connection finishDoc;
   boost::signals2::connection deleteDoc;
};

} // namespace PartDesignGui


#endif // PARTDESIGN_WORKBENCH_H
