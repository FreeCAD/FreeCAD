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

#include <Gui/ActiveObjectList.h>

class QListWidget;
class QGridLayout;

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
    class PropertyLinkSub;
}

namespace Sketcher {
    class SketchObject;
}

namespace PartDesignGui {

/// Activate edit mode of the given object
bool setEdit(App::DocumentObject *obj, App::DocumentObject *container=0, const char *key=PDBODYKEY);

/// Call before editing object to setup visibilities
void beforeEdit(App::DocumentObject *editingObj);

/// Show the given object on top
void showObjectOnTop(const App::SubObjectT &objT);
void hideObjectOnTop(const App::SubObjectT &objT);
void highlightObjectOnTop(const App::SubObjectT &objT);
void selectObjectOnTop(const App::SubObjectT &objT, bool multiselect=false);
void unselectObjectOnTop(const App::SubObjectT &objT);
void toggleShowOnTop(Gui::ViewProviderDocumentObject *vp,
                     App::SubObjectT &last,
                     const char *prop,
                     bool init = false);
void toggleShowOnTop(Gui::ViewProviderDocumentObject *vp,
                     std::vector<App::SubObjectT> &last,
                     const char *prop,
                     bool init = false);

bool populateGeometryReferences(QListWidget *listWidget, App::PropertyLinkSub &prop, bool refresh);

/// Import an external feature into the body of the host feature using SubShapeBinder
App::SubObjectT importExternalObject(const App::SubObjectT &feature,
                                     bool report = true,
                                     bool wholeObject = true,
                                     bool noSubElement = false);
/// Import a feature with sub-element (Wire or Face) using SubShapeBinder
App::SubObjectT importExternalElement(App::SubObjectT feature, bool report=true);

/// Return active body or show a warning message
PartDesign::Body *getBody(bool messageIfNot, bool autoActivate=true, bool assertModern=true,
        App::DocumentObject **topParent=0, std::string *subname=0);

PartDesign::Body *getBody(App::SubObjectT &sobjT, 
        bool messageIfNot=true, bool autoActivate=true, bool assertModern=true);

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
                                  App::DocumentObject **topParent=0,
                                  std::string *subname=0);

/// Display error when there are existing Body objects, but none are active
void needActiveBodyError(void);

/// Create a Body object in doc, set it active, and return pointer to it
PartDesign::Body * makeBody(App::Document *doc);

/**
 * Finds a body for the given feature. And shows a message if not found
 * Also unlike Body::findBodyFor it checks if the active body has the feature first.
 */
PartDesign::Body *getBodyFor(const App::DocumentObject*, bool messageIfNot,
                             bool autoActivate=true, bool assertModern=true,
                             App::DocumentObject **topParent=0, std::string *subname=0);

App::Part        *getPartFor(const App::DocumentObject*, bool messageIfNot);
App::Part        *getActivePart(App::DocumentObject **topParent=0, std::string *subname=0);

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

PartDesign::Body *queryCommandOverride();

void initMonitor();

class MonitorProxy: public QObject
{
    Q_OBJECT

public:
    QGridLayout *addCheckBox(QWidget * parent, int index = 0);
    
protected Q_SLOTS:
    void onPreview(bool);
    void onPreviewTransparency(bool);
    void onShowOnTop(bool);
    void onEditTimer();
};

QGridLayout *addTaskCheckBox(QWidget * widget, int index = 0);

void fitViewWithDelay(int);

} /* PartDesignGui */

#endif /* end of include guard: UTILS_H_CS5LK2ZQ */
