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


#ifndef GUI_SOFCDB_H
#define GUI_SOFCDB_H

#include <string>
#include <Inventor/SbBasic.h>

class SoNode;
class SoGroup;
namespace Gui {
/**
 * The FreeCAD database class to initialize all our own Inventor nodes.
 * @author Werner Mayer
 */
class GuiExport SoFCDB
{
public:
    static SbBool isInitialized(void);
    static void init();
    static void finish();
    static SoNode* replaceSwitches(SoNode* node);
    /// helper to apply a SoWriteAction to a node and write it to a string
    static const std::string& writeNodesToString(SoNode * root);
    static bool writeToVRML(SoNode* node, const char* filename, bool binary);
    static void writeToVRML(SoNode* node, std::string& buffer);
    // Write to VRML or Inventor file
    static bool writeToFile(SoNode* node, const char* filename, bool binary);
    /*! container for app lifetime storage. See SoFCCSysDragger for details
     * on why this is needed.
     */ 
    static SoGroup* getStorage();
};

} // namespace Gui

#endif // GUI_SOFCDB_H
