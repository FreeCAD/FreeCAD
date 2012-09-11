/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#ifndef PARTDESIGN_FeatureTransformed_H
#define PARTDESIGN_FeatureTransformed_H

#include <gp_Trsf.hxx>

#include <App/PropertyStandard.h>
#include "Feature.h"


namespace PartDesign
{

/**
 * Abstract superclass of all features that are created by transformation of another feature
 * Transformations are translation, rotation and mirroring
 */
class PartDesignExport Transformed : public PartDesign::Feature
{
    PROPERTY_HEADER(PartDesign::Transformed);

public:
    Transformed();

    /** The shapes to be transformed
        if Originals is empty the instance is just a container for storing transformation data */
    App::PropertyLinkList Originals;

    /// Return first original, which serves as "Support" until Body feature becomes functional
    App::DocumentObject* getOriginalObject() const;

    /// Get the list of transformations describing the members of the pattern
    // Note: Only the Scaled feature requires the originals
    virtual const std::list<gp_Trsf> getTransformations(const std::vector<App::DocumentObject*> originals) {
        return std::list<gp_Trsf>(); // Default method
    }

   /** @name methods override feature */
    //@{
    /** Recalculate the feature
      * Gets the transformations from the virtual getTransformations() method of the sub class
      * and applies them to every member of Originals. The total number of copies including
      * the untransformed Originals will be sizeof(Originals) times sizeof(getTransformations())
      * If Originals is empty, execute() returns immediately without doing anything as
      * the actual processing will happen in the MultiTransform feature
      */
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    //@}

    void positionBySupport(void);

protected:
    void buildTransformHistory(BRepBuilderAPI_MakeShape& mkFuse,
                               const TopoDS_Shape& newShape,
                               const TopoDS_Shape& oldShape,
                               const int index);
};

} //namespace PartDesign


#endif // PARTDESIGN_FeatureTransformed_H
