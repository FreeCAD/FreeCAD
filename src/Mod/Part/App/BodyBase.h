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
    PROPERTY_HEADER(PartDesign::BodyBase);

public:
    BodyBase();

    App::PropertyLinkList   Model;
    App::PropertyLink       Tip;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the view provider
    //const char* getViewProviderName(void) const {
    //    return "PartDesignGui::ViewProviderBodyBase";
    //}
    //@}

    // These methods are located here to avoid a dependency of ViewProviderSketchObject on PartDesign
    /// Remove the feature from the body
    virtual void removeFeature(App::DocumentObject* feature){}

    /// Return true if the feature belongs to this body
    const bool hasFeature(const App::DocumentObject *f) const;

    /**
      * Return the solid feature before the given feature, or before the Tip feature
      * That is, sketches and datum features are skipped
      * If inclusive is true, start or the Tip is returned if it is a solid feature
      */
    virtual App::DocumentObject *getPrevSolidFeature(App::DocumentObject *start = NULL, const bool inclusive = true)
        { return NULL; }

    /// Return true if the feature is located after the current Tip feature
    const bool isAfterTip(const App::DocumentObject *f) const;

    /// Return the body which this feature belongs too, or NULL
    static BodyBase* findBodyOf(const App::DocumentObject* f);

};

} //namespace Part


#endif // PART_BodyBase_H
