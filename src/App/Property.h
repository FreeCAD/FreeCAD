/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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


#ifndef APP_PROPERTY_H
#define APP_PROPERTY_H

// Std. configurations

#include <Base/Persistence.h>
#include <string>
#include <bitset>


namespace App
{

class PropertyContainer;

/** Base class of all properties
 * This is the father of all properties. Properties are objects which are used
 * in the document tree to parametrize e.g. features and their graphical output.
 * They are also used to gain access from the scripting facility.
 * /par
 * This abstract base class defines all methods shared by all
 * possible properties. It is also possible to define user properties
 * and use them in the framework...
 */
class AppExport Property : public Base::Persistence
{
    TYPESYSTEM_HEADER();

public:
    Property();
    virtual ~Property();

    /** This method is used to get the size of objects
     * It is not meant to have the exact size, it is more or less an estimation
     * which runs fast! Is it two bytes or a GB?
     * This method is defined in Base::Persistence
     * @see Base::Persistence
     */
    virtual unsigned int getMemSize (void) const {
        // you have to implement this method in all property classes!
        return sizeof(father) + sizeof(StatusBits);
    }

    /// get the name of this property in the belonging container
    const char* getName(void) const;

    /// Get the class name of the associated property editor item
    virtual const char* getEditorName(void) const { return ""; }

    /// Get the type of the property in the container
    short getType(void) const; 

    /// Get the group of this property
    const char* getGroup(void) const;

    /// Get the documentation of this property
    const char* getDocumentation(void) const;

    /// Is called by the framework to set the father (container)
    void setContainer(PropertyContainer *Father);

    /// Get a pointer to the PropertyContainer derived class the property belongs to
    PropertyContainer *getContainer(void) const {return father;}
    /// Set the property touched
    void touch();
    /// Test if this property is touched 
    bool isTouched(void) const {return StatusBits.test(0);}
    /// Reset this property touched 
    void purgeTouched(void){StatusBits.reset(0);}

    /// Returns a new copy of the property (mainly for Undo/Redo and transactions)
    virtual Property *Copy(void) const = 0;
    /// Paste the value from the property (mainly for Undo/Redo and transactions)
    virtual void Paste(const Property &from) = 0;
    /// Encodes an attribute upon saving.
    std::string encodeAttribute(const std::string&) const;


    friend class PropertyContainer;

    /** Status bits of the property
     * The first 8 bits are used for the base system the rest can be used in
     * descendent classes to to mark special stati on the objects.
     * The bits and their meaning are listed below:
     * 0 - object is marked as 'touched'
     * 1 - object is marked as 'immutable'
     * 2 - object is marked as 'read-ony' (for property editor)
     * 3 - object is marked as 'hidden' (for property editor)
     */
    std::bitset<32> StatusBits;


protected:
    /// Gets called by all setValue() methods after the value has changed
    void hasSetValue(void);
    /// Gets called by all setValue() methods before the value has changed
    void aboutToSetValue(void);

private:
    // forbidden
    Property(const Property&);
    Property& operator = (const Property&);

private:
    PropertyContainer *father;
};


/** Base class of all property lists.
 * The PropertyLists class is the base class for properties which can contain
 * multiple values, not only a single value. 
 * All property types which may contain more than one value inherits this class.
 */
class AppExport PropertyLists : public Property
{
    TYPESYSTEM_HEADER();

public:
    virtual void setSize(int newSize)=0;   
    virtual int getSize(void) const =0;   
};

} // namespace App

#endif // APP_PROPERTY_H
