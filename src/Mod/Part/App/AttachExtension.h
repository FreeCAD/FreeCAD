/***************************************************************************
 *   Copyright (c) 2015 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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
  * AttachExtensionh, .cpp contain a extension class to derive other features from, to make
  * them attachable.
  */

#ifndef PARTATTACHABLEOBJECT_H
#define PARTATTACHABLEOBJECT_H

#include <App/DocumentObjectExtension.h>
#include <App/ExtensionPython.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <Base/Exception.h>
#include <Base/Placement.h>

#include "Attacher.h"
#include "PartFeature.h"


namespace Part
{

class PartExport AttachEngineException : public Base::Exception
{
public:
   /// Construction
   AttachEngineException();
   explicit AttachEngineException(const char * sMessage);
   explicit AttachEngineException(const std::string& sMessage);
   /// Destruction
   ~AttachEngineException() noexcept override = default;
};

/**
 * @brief The AttachableObject class is the thing to extend an object with
 * that should be attachable. It includes the required properties, and
 * shortcuts for accessing the attachment math class.
 */
class PartExport AttachExtension : public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Part::AttachableObject);
public:
    AttachExtension();
    ~AttachExtension() override;

    /**
     * @brief setAttacher sets the AttachEngine object. The class takes the
     * ownership of the pointer, it will be deleted when the class is
     * destroyed, or when a new attacher is set. The default attacher is AttachEngine3D.
     * @param attacher. AttachableObject takes ownership and will delete it eventually.
     */
    virtual void setAttacher(Attacher::AttachEngine* attacher);

    /**
     * @brief changeAttacherType
     * @param typeName is the typename of new attacher class. Must be derived
     * from Attacher::AttachEngine.
     * @return true if attacher was changed. false if attacher is already of the
     * type requested. Throws if invalid type is supplied.
     */
    bool changeAttacherType(const char* typeName);

    Attacher::AttachEngine &attacher() const {if(!_attacher) throw AttachEngineException("AttachableObject: no attacher is set."); return *_attacher;}


    App::PropertyString         AttacherType;
    App::PropertyLinkSubList    Support;
    App::PropertyEnumeration    MapMode; //see AttachEngine::eMapMode
    App::PropertyBool           MapReversed; //inverts Z and X internal axes
    App::PropertyPlacement      AttachmentOffset;

    /**
      * @brief MapPathParameter is a parameter value for mmNormalToPath (the
      * sketch will be mapped normal to a curve at point specified by parameter
      * (from 0.0 to 1.0, from start to end) )
      */
    App::PropertyFloat MapPathParameter;

    /** calculate and update the Placement property based on the Support, and
      * mode. Can throw FreeCAD and OCC exceptions. Returns true if attached,
      * false if not, throws if attachment failed.
      */
    virtual bool positionBySupport();

    /** Return whether this attacher is active
     */
    bool isAttacherActive() const;

    virtual bool isTouched_Mapping()
    {return true; /*support.isTouched isn't true when linked objects are changed... why?..*/}

    short int extensionMustExecute() override;
    App::DocumentObjectExecReturn *extensionExecute() override;
    PyObject* getExtensionPyObject() override;
    void onExtendedDocumentRestored() override;

protected:
    void extensionOnChanged(const App::Property* /*prop*/) override;
    virtual void extHandleChangedPropertyName(Base::XMLReader &reader, const char* TypeName, const char* PropName);

    App::PropertyPlacement& getPlacement() const;

public:
    void updateAttacherVals();

private:
    Attacher::AttachEngine* _attacher = nullptr;
    mutable int _active = -1;
};


using AttachExtensionPython = App::ExtensionPythonT<AttachExtension>;

} // namespace Part

#endif // PARTATTACHABLEOBJECT_H
