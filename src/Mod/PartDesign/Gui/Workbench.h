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

namespace Gui {

class MenuItem;
class Document;
class ViewProviderDocumentObject;

}

namespace PartDesign {
	class Body;
}

namespace App {
	class Part;
}

namespace PartDesignGui {

// pointer to the active assembly object
//extern PartDesign::Body                *ActivePartObject;
//extern Gui::Document                   *ActiveGuiDoc;
//extern App::Document                   *ActiveAppDoc;
//extern Gui::ViewProviderDocumentObject *ActiveVp;

/// Return active body or show a warning message
PartDesign::Body *getBody(bool messageIfNot);

/**
 * @author Werner Mayer
 */
class PartDesignGuiExport Workbench : public Gui::StdWorkbench
{
    TYPESYSTEM_HEADER();

public:
    Workbench();
    virtual ~Workbench();

      /** Run some actions when the workbench gets activated. */
    virtual void activated();
    /** Run some actions when the workbench gets deactivated. */
    virtual void deactivated();

    /// Add custom entries to the context menu
    virtual void setupContextMenu(const char* recipient, Gui::MenuItem*) const;

	/** Setup a Part for PartDesign
	 * This methode is use to populate a Part object with all 
	 * necesarry PartDesign and base objects to allow the use 
	 * in PartDesign. Its called from within PartDesign as well 
	 * as from other modules which wish to set up a Part for PartDesin
	 * (e.g. Assembly):
	 */
	 static PartDesign::Body *setUpPart(const App::Part *);


protected:
  Gui::MenuItem* setupMenuBar() const;
  Gui::ToolBarItem* setupToolBars() const;
  Gui::ToolBarItem* setupCommandBars() const;

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

   void _doMigration(const App::Document* doc);
   void _switchToDocument(const App::Document* doc);

};

} // namespace PartDesignGui


#endif // PARTDESIGN_WORKBENCH_H 
