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

#include <boost/signals.hpp>

namespace PartDesign
{

class Feature;

class PartDesignExport Body : public Part::BodyBase
{
    PROPERTY_HEADER(PartDesign::Body);

public:

    /// True if this body feature is active or was active when the document was last closed
    //App::PropertyBool IsActive;

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

    /// Return the previous feature
    App::DocumentObject* getPrevFeature(App::DocumentObject *start = NULL) const;

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

    /// Add the feature into the body at the current insert point (Tip feature)
    void addFeature(App::DocumentObject* feature);

    /**
     * Insert the feature into the body after the given feature.
     *
     * @param feature  The feature to insert into the body
     * @param target   The feature relative which one should be inserted the given.
     *                 If target is NULL than insert into the end if where is InsertBefore
     *                 and into the begin if where is InsertAfter.
     * @param after    if true insert the feature after the target. Default is false.
     *
     * @note the methode doesn't modifies the Tip unlike addFeature()
     */
    void insertFeature(App::DocumentObject* feature, App::DocumentObject* target, bool after=false);

    /// Remove the feature from the body
    void removeFeature(App::DocumentObject* feature);

    /// Checks if the given document object is a feaure of this body
    bool isFeature(App::DocumentObject* feature);

    /// Return true if the given feature is member of a MultiTransform feature
    static const bool isMemberOfMultiTransform(const App::DocumentObject* f);

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

    /// Return the bounding box of the Tip Shape, taking into account datum features
    Base::BoundBox3d getBoundBox();

    PyObject *getPyObject(void);

protected:
    virtual void onSettingDocument();
    
private:
    App::DocumentObject* rememberTip;
    boost::signals::scoped_connection connection;
    void onDelete(const App::DocumentObject& obj);
};

} //namespace PartDesign


#endif // PART_Body_H
