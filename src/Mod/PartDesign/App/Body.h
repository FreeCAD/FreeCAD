/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTDESIGN_Body_H
#define PARTDESIGN_Body_H

#include <App/PropertyStandard.h>
#include <Mod/Part/App/BodyBase.h>


namespace PartDesign
{

class Feature;

class Body : public Part::BodyBase
{
    PROPERTY_HEADER(PartDesign::Body);

public:

    /// True if this body feature is active or was active when the document was last closed
    App::PropertyBool IsActive;

    Body();

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;    
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderBody";
    }
    //@}

    /// Get the tip shape
    const Part::TopoShape getTipShape();

    /**
      * Return the solid feature before the given feature, or before the Tip feature
      * That is, sketches and datum features are skipped
      * If inclusive is true, start or the Tip is returned if it is a solid feature
      */
    App::DocumentObject *getPrevSolidFeature(App::DocumentObject *start = NULL, const bool inclusive = true);

    /**
      * Return the next solid feature after the given feature, or after the Tip feature
      * That is, sketches and datum features are skipped
      * If inclusive is true, start or the Tip is returned if it is a solid feature
      */
    App::DocumentObject *getNextSolidFeature(App::DocumentObject* start = NULL, const bool inclusive = true);

    // Return the shape of the feature preceding this feature
    //const Part::TopoShape getPreviousSolid(const PartDesign::Feature* f);

    /// Return true if the feature belongs to this body
    const bool hasFeature(const App::DocumentObject *f) const;

    /// Return true if the feature is located after the current Tip feature
    const bool isAfterTip(const App::DocumentObject *f);

    /// Add the feature into the body at the current insert point (Tip feature)
    void addFeature(App::DocumentObject* feature);

    /// Remove the feature from the body
    void removeFeature(App::DocumentObject* feature);


    /**
      * Return true if the given feature is a solid feature allowed in a Body. Currently this is only valid
      * for features derived from PartDesign::Feature
      * Return false if the given feature is a Sketch or a Part::Datum feature
      */
    static const bool isSolidFeature(const App::DocumentObject* f);

    /**
      * Return true if the given feature is allowed in a Body. Currently allowed are
      * all features derived from PartDesign::Feature and Part::Datum and sketches
      */
    static const bool isAllowed(const App::DocumentObject* f);

    /// Return the body which this feature belongs too, or NULL
    static Body* findBodyOf(const App::DocumentObject* f);

    PyObject *getPyObject(void);

private:
    App::DocumentObject* rememberTip;
};

} //namespace PartDesign


#endif // PART_Body_H
