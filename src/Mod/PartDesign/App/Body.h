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

#include <boost/signals2.hpp>

namespace App {
    class Origin;
}

namespace PartDesign
{

class Feature;

class PartDesignExport Body : public Part::BodyBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Body);

public:

    /// True if this body feature is active or was active when the document was last closed
    //App::PropertyBool IsActive;

    App::PropertyBool SingleSolid;

    Body();

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void) override;
    short mustExecute() const override;

    /// returns the type name of the view provider
    const char* getViewProviderName(void) const override {
        return "PartDesignGui::ViewProviderBody";
    }
    //@}

    /// Return the previous feature
    PartDesign::Feature* getPrevFeature(App::DocumentObject *start = NULL) const;

    /**
     * Add the feature into the body at the current insert point.
     * The insertion poin is the before next solid after the Tip feature
     */
    virtual std::vector<App::DocumentObject*> addObject(App::DocumentObject*) override;
    virtual std::vector< DocumentObject* > addObjects(std::vector< DocumentObject* > obj) override;

    /**
     * Insert the feature into the body after the given feature.
     *
     * @param feature  The feature to insert into the body
     * @param target   The feature relative which one should be inserted the given.
     *                 If target is NULL than insert into the end if where is InsertBefore
     *                 and into the begin if where is InsertAfter.
     * @param after    if true insert the feature after the target. Default is false.
     *
     * @note the method doesn't modify the Tip unlike addObject()
     */
    void insertObject(App::DocumentObject* feature, App::DocumentObject* target, bool after=false);

    void setBaseProperty(App::DocumentObject* feature);

    /// Remove the feature from the body
    virtual std::vector<DocumentObject*> removeObject(DocumentObject* obj) override;

    /**
     * Checks if the given document object lays after the current insert point
     * (place before next solid after the Tip)
     */
    bool isAfterInsertPoint(App::DocumentObject* feature);

    /// Return true if the given feature is member of a MultiTransform feature
    static bool isMemberOfMultiTransform(const App::DocumentObject* f);

    /**
      * Return true if the given feature is a solid feature allowed in a Body. Currently this is only valid
      * for features derived from PartDesign::Feature
      * Return false if the given feature is a Sketch or a Part::Datum feature
      */
    static bool isSolidFeature(const App::DocumentObject* f);

    /**
      * Return true if the given feature is allowed in a Body. Currently allowed are
      * all features derived from PartDesign::Feature and Part::Datum and sketches
      */
    static bool isAllowed(const App::DocumentObject* f);
    virtual bool allowObject(DocumentObject* f) override {return isAllowed(f);}

    /**
     * Return the body which this feature belongs too, or NULL
     * The only difference to BodyBase::findBodyOf() is that this one casts value to Body*
     */
    static Body *findBodyOf(const App::DocumentObject* feature);

    PyObject *getPyObject(void) override;

    virtual std::vector<std::string> getSubObjects(int reason=0) const override;
    virtual App::DocumentObject *getSubObject(const char *subname, 
        PyObject **pyObj, Base::Matrix4D *pmat, bool transform, int depth) const override;

    void setShowTip(bool enable) {
        showTip = enable;
    }

protected:
    virtual void onSettingDocument() override;

    /// Adjusts the first solid's feature's base on BaseFeature getting set
    virtual void onChanged (const App::Property* prop) override;

    /**
      * Return the solid feature before the given feature, or before the Tip feature
      * That is, sketches and datum features are skipped
      */
    App::DocumentObject *getPrevSolidFeature(App::DocumentObject *start = NULL);

    /**
      * Return the next solid feature after the given feature, or after the Tip feature
      * That is, sketches and datum features are skipped
      */
    App::DocumentObject *getNextSolidFeature(App::DocumentObject* start = NULL);

    /// Creates the corresponding Origin object
    virtual void setupObject () override;
    /// Removes all planes and axis if they are still linked to the document
    virtual void unsetupObject () override;

    virtual void onDocumentRestored() override;

private:
    boost::signals2::scoped_connection connection;
    bool showTip = false;
};

} //namespace PartDesign


#endif // PART_Body_H
