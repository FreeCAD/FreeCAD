/***************************************************************************
 *   Copyright (c) Victor Titov (DeepSOIC)                                 *
 *                                           (vv.titov@gmail.com) 2015     *
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
/**
  * AttachableObject.h, .cpp contain a class to derive other features from, to make
  * them attachable.
  */

#ifndef PARTATTACHABLEOBJECT_H
#define PARTATTACHABLEOBJECT_H

#include <App/PropertyStandard.h>
#include <App/PropertyLinks.h>
#include <App/GeoFeature.h>
#include <Base/Vector3D.h>
#include <Base/Placement.h>
#include <Base/Exception.h>

#include "PartFeature.h"
#include "Attacher.h"

#include <QString>

#include <gp_Vec.hxx>

using namespace Attacher;

namespace Part
{

/**
 * @brief The AttachableObject class is the thing to be inherited by an object
 * that should be attachable. It includes the required properties, and
 * shortcuts for accessing the attachment math class.
 *
 * Todos to make it work:
 * - call Attacher::execute() when executing derived object. Make sure to deal
 * with its return value, otherwise it will leak memory upon fails.
 */
class PartExport AttachableObject : public Part::Feature
{
    PROPERTY_HEADER(Part::AttachableObject);
public:
    AttachableObject();
    ~AttachableObject();

    /**
     * @brief setAttacher sets the AttachEngine object. The class takes the
     * ownership of the pointer, it will be deleted when the class is
     * destroyed, or when a new attacher is set. The default attacher is AttachEngine3D.
     * @param attacher. AttachableObject takes ownership and will delete it eventually.
     */
    virtual void setAttacher(AttachEngine* attacher);
    AttachEngine* attacher(void) const {return _attacher;}

    /// if the 2DObject lies on the Face of an other object this links to it
    App::PropertyLinkSubList    Support;
    App::PropertyEnumeration    MapMode; //see AttachEngine::eMapMode
    App::PropertyBool           MapReversed; //inverts Z and X internal axes
    App::PropertyPlacement      superPlacement;

    /**
      * @brief MapPathParameter is a parameter value for mmNormalToPath (the
      * sketch will be mapped normal to a curve at point specified by parameter
      * (from 0.0 to 1.0, from start to end) )
      */
    App::PropertyFloat MapPathParameter;

    /** calculate and update the Placement property based on the Support, and
      * mode. Can throw FreeCAD and OCC exceptions.
      */
    virtual void positionBySupport(void);

    virtual bool isTouched_Mapping()
    {return true; /*support.isTouched isn't true when linked objects are changed... why?..*/};

    App::DocumentObjectExecReturn *execute(void);
protected:
    virtual void onChanged(const App::Property* /*prop*/);

public:
    void updateAttacherVals();

private:
    AttachEngine* _attacher;
};


} // namespace Part

#endif // PARTATTACHABLEOBJECT_H
