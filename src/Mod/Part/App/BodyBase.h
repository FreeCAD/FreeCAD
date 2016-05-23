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


#ifndef PART_BodyBase_H
#define PART_BodyBase_H

#include <App/PropertyStandard.h>
#include <Mod/Part/App/PartFeature.h>

namespace App {
    class Origin;
}

namespace Part
{
/** Base class of all body objects in FreeCAD
 * A body is used, e.g. in PartDesign, to agregate
 * some modeling features to one shape. As long as not
 * in edit or active on a workbench, the body shows only the
 * resulting shape to the outside (Tip link).
 */
class PartExport BodyBase : public Part::Feature
{
    PROPERTY_HEADER(Part::BodyBase);

public:
    BodyBase();

    /// The list of features
    App::PropertyLinkList   Model;

    /**
     * The final feature of the body it is associated with.
     * Note: tip may either point to the BaseFeature or to some feature inside the Model list.
     *       in case it points to the model the PartDesign::Body guaranties that it is a solid.
     */
    App::PropertyLink       Tip;

    /// Origin linked to the property, please use getOrigin () to access it
    App::PropertyLink Origin;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    virtual App::DocumentObjectExecReturn* execute(void);
    virtual short mustExecute() const;

    /// returns the type name of the view provider
    virtual const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderBodyBase";
    }

    //@}

    /// Returns all Model objects (in PartDesign, this gets prepanded by BaseFeature)
    virtual std::vector<App::DocumentObject *> getFullModel () const {
        return this->Model.getValues();
    }

    // These methods are located here to avoid a dependency of ViewProviderSketchObject on PartDesign
    virtual void addFeature(App::DocumentObject* feature);

    /// Remove the feature from the body
    virtual void removeFeature(App::DocumentObject* feature);

    /// Delets all the objects linked to the model.
    virtual void removeModelFromDocument();

    /// Return true if the feature belongs to this body or either the body is based on the feature
    bool hasFeature(const App::DocumentObject *f) const;

    /**
     * Return the body which this feature belongs too, or NULL.
     * Note: Normally each PartDesign feature belongs to a single body,
     *       But if a body is based on the feature it also will be return...
     *       But there are could be more features based on the same body.
     * TODO introduce a findBodiesOf() if needed (2015-08-04, Fat-Zer)
     */
    static BodyBase* findBodyOf(const App::DocumentObject* f);

    /// Returns the origin link or throws an exception
    App::Origin* getOrigin () const;

    /// Creates the corresponding Origin object
    virtual void setupObject ();
    /// Removes all planes and axis if they are still linked to the document
    virtual void unsetupObject ();

    PyObject* getPyObject(void);

protected:

};

} //namespace Part


#endif // PART_BodyBase_H
