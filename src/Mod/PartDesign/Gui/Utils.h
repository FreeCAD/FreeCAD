/***************************************************************************
 *   Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

/** \file PartDesign/Gui/Utils.h
 *  This file contains some utility function used over PartDesignGui module
 */
namespace PartDesign {
    class Body;
    class Feature;
}

namespace App {
    class Document;
    class DocumentObject;
    class Part;
}

namespace Sketcher {
    class SketchObject;
}

namespace PartDesignGui {

/// Activate edit mode of the given object
bool setEdit(App::DocumentObject *obj, PartDesign::Body *body = nullptr);

/// Return active body or show a warning message
PartDesign::Body *getBody(bool messageIfNot, bool autoActivate=true, bool assertModern=true,
        App::DocumentObject **topParent=nullptr, std::string *subname=nullptr);

/// Display a dialog to select or create a Body object when none is active
PartDesign::Body * needActiveBodyMessage(App::Document *doc,
                                         const QString& infoText=QString());

/**
 * Set given body active, and return pointer to it.
 * \param body the pointer to the body
 * \param doc the pointer to the document in question
 * \param topParent and
 * \param subname to be passed under certain circumstances
 *        (currently only subshapebinder)
 */
PartDesign::Body * makeBodyActive(App::DocumentObject *body, App::Document *doc,
                                  App::DocumentObject **topParent=nullptr,
                                  std::string *subname=nullptr);

/// Display error when there are existing Body objects, but none are active
void needActiveBodyError();

/// Create a Body object in doc, set it active, and return pointer to it
PartDesign::Body * makeBody(App::Document *doc);

/**
 * Finds a body for the given feature. And shows a message if not found
 * Also unlike Body::findBodyFor it checks if the active body has the feature first.
 */
PartDesign::Body *getBodyFor(const App::DocumentObject*, bool messageIfNot,
                             bool autoActivate=true, bool assertModern=true,
                             App::DocumentObject **topParent=nullptr, std::string *subname=nullptr);

App::Part        *getPartFor(const App::DocumentObject*, bool messageIfNot);
App::Part        *getActivePart();

/// Fix sketch support after moving a free sketch into a body
void fixSketchSupport(Sketcher::SketchObject* sketch);

/**
 * Returns true if document has any non-PartDesign objects that links to the given object.
 * If respectGroups is true don't count links from App::GeoFeatureGroup-derived objects (default is false)
 */
bool isAnyNonPartDesignLinksTo ( PartDesign::Feature *feature, bool respectGroups=false );

/// Relink all nonPartDesign features to the body instead of the given partDesign Feature
void relinkToBody ( PartDesign::Feature *feature );

/// Check if feature is dependent on anything except movable sketches and datums
bool isFeatureMovable(App::DocumentObject* feature);
/// Collect dependencies of the features during the move. Dependencies should only be dependent on origin
std::vector<App::DocumentObject*> collectMovableDependencies(std::vector<App::DocumentObject*>& features);
/// Relink sketches and datums to target body's origin
void relinkToOrigin(App::DocumentObject* feature, PartDesign::Body* body);

} /* PartDesignGui */

#endif /* end of include guard: UTILS_H_CS5LK2ZQ */
