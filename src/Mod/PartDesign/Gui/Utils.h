/***************************************************************************
 *  Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>     *
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

#ifndef UTILS_H_CS5LK2ZQ
#define UTILS_H_CS5LK2ZQ

/** \file Utils.h
 *  This fiel contains some utility function used over PartDesignGui module
 */
namespace PartDesign {
    class Body;
    class Feature;
}

namespace App {
    class DocumentObject;
    class Part;
}

namespace Sketcher {
    class SketchObject;
}

namespace PartDesignGui {

/// Return active body or show a warning message
PartDesign::Body *getBody(bool messageIfNot);
/**
 * Finds a body for the given feature. And shows a message if not found
 * Also unlike Body::findBodyFor it checks if the active body has the feature first.
 */
PartDesign::Body *getBodyFor(const App::DocumentObject*, bool messageIfNot);
App::Part        *getPartFor(const App::DocumentObject*, bool messageIfNot);


/** Setup a Part for PartDesign
 * This methode is use to populate a Part object with all
 * necesarry PartDesign and base objects to allow the use
 * in PartDesign. Its called from within PartDesign as well
 * as from other modules which wish to set up a Part for PartDesin
 * (e.g. Assembly):
 * TODO any reasons why this should return a value? (2015-08-08, Fat-Zer)
 */
PartDesign::Body *setUpPart(const App::Part *);

/// Fix sketch support after moving a free sketch into a body
void fixSketchSupport(Sketcher::SketchObject* sketch);

/**
 * Returns true if document has any non-PartDesign objects that links to the given object.
 * If respectGroups is true don't count links from App::GeoFeatureGroup-derived objects (default is false)
 */
bool isAnyNonPartDesignLinksTo ( PartDesign::Feature *feature, bool respectGroups=false );

/// Relink all nonPartDesign features to the body instead of the given partDesign Feature
void relinkToBody ( PartDesign::Feature *feature );

} /* PartDesignGui */

#endif /* end of include guard: UTILS_H_CS5LK2ZQ */
