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
#ifndef BOOST_105400
#include <boost/any.hpp>
#else
#include <boost_any_1_55.hpp>
#endif
#include <string>
#include <bitset>


namespace App
{

class PropertyContainer;
class ObjectIdentifier;

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
    enum Status
    {
        Touched = 0, // touched property
        Immutable = 1, // can't modify property
        ReadOnly = 2, // for property editor
        Hidden = 3, // for property editor
        Single = 4, // for save/load of floating point numbers
        Ordered = 5, // for PropertyLists whether the order of the elements is relevant for the container using it
        User1 = 28, // user-defined status
        User2 = 29, // user-defined status
        User3 = 30, // user-defined status
        User4 = 31  // user-defined status
    };

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

    /// Set value of property
    virtual void setPathValue(const App::ObjectIdentifier & path, const boost::any & value);

    /// Get value of property
    virtual const boost::any getPathValue(const App::ObjectIdentifier & path) const;

    /// Convert p to a canonical representation of it
    virtual const App::ObjectIdentifier canonicalPath(const App::ObjectIdentifier & p) const;

    /// Get valid paths for this property; used by auto completer
    virtual void getPaths(std::vector<App::ObjectIdentifier> & paths) const;

    /** Property status handling
     */
    //@{
    /// Set the property touched
    void touch();
    /// Test if this property is touched
    inline bool isTouched(void) const {
        return StatusBits.test(Touched);
    }
    /// Reset this property touched
    inline void purgeTouched(void) {
        StatusBits.reset(Touched);
    }
    /// return the status bits
    inline unsigned long getStatus() const {
        return StatusBits.to_ulong();
    }
    inline bool testStatus(Status pos) const {
        return StatusBits.test(static_cast<size_t>(pos));
    }
    inline void setStatus(Status pos, bool on) {
        StatusBits.set(static_cast<size_t>(pos), on);
    }
    ///Sets property editable/grayed out in property editor
    void setReadOnly(bool readOnly);
    inline bool isReadOnly() const {
        return testStatus(App::Property::ReadOnly);
    }
    /// Sets precision of properties using floating point
    /// numbers to single, the default is double.
    void setSinglePrecision(bool single) {
        setStatus(App::Property::Single, single);
    }
    /// Gets precision of properties using floating point numbers
    inline bool isSinglePrecision() const {
        return testStatus(App::Property::Single);
    }
    //@}

    /// Returns a new copy of the property (mainly for Undo/Redo and transactions)
    virtual Property *Copy(void) const = 0;
    /// Paste the value from the property (mainly for Undo/Redo and transactions)
    virtual void Paste(const Property &from) = 0;


    friend class PropertyContainer;

protected:
    /** Status bits of the property
     * The first 8 bits are used for the base system the rest can be used in
     * descendent classes to mark special statuses on the objects.
     * The bits and their meaning are listed below:
     * 0 - object is marked as 'touched'
     * 1 - object is marked as 'immutable'
     * 2 - object is marked as 'read-only' (for property editor)
     * 3 - object is marked as 'hidden' (for property editor)
     */
    std::bitset<32> StatusBits;

protected:
    /// Gets called by all setValue() methods after the value has changed
    void hasSetValue(void);
    /// Gets called by all setValue() methods before the value has changed
    void aboutToSetValue(void);

    /// Verify a path for the current property
    virtual void verifyPath(const App::ObjectIdentifier & p) const;

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
    PropertyLists() {};
    
    virtual void setSize(int newSize)=0;
    virtual int getSize(void) const =0;
    
    // if the order of the elements in the list relevant?
    // if yes, certain operations, like restoring must make sure that the
    // order is kept despite errors.
    inline void setOrderRelevant(bool on) { this->setStatus(Status::Ordered,on); };
    inline bool isOrderRelevant() const { return this->testStatus(Status::Ordered);}
};

/** A template class that is used to inhibit multiple nested calls to aboutToSetValue/hasSetValue for properties.
 *
 * A template class that is used to inhibit multiple nested calls to
 * aboutToSetValue/hasSetValue for properties, and only invoke it the first and
 * last time it is needed. This is useful in cases where you want to change multiple
 * values in a property "atomically", using possibly multiple primitive functions
 * that normally would trigger aboutToSetValue/hasSetValue calls on their own.
 *
 * To use, inherit privately from the AtomicPropertyChangeInterface class, using
 * your class name as the template argument. In all cases where you normally would
 * call aboutToSetValue/hasSetValue before and after a change, create an
 * AtomicPropertyChange object before you do the change. Depending on a counter
 * in the main property, the constructor might invoke aboutToSetValue. When the
 * AtomicPropertyChange object is destructed, it might call hasSetValue if it is
 * found necessary to do (i.e last item on the AtomicPropertyChange stack).
 * This makes it easy to match the calls, and it is also exception safe in the
 * sense that the destructors are guaranteed to be called during unwinding and
 * exception handling, making the calls to boutToSetValue and hasSetValue balanced.
 *
 */

template<class P> class AtomicPropertyChangeInterface {
protected:
    AtomicPropertyChangeInterface() : signalCounter(0) { }

public:
    class AtomicPropertyChange {
    public:
        AtomicPropertyChange(P & prop) : mProp(prop) {
            // Signal counter == 0? Then we need to invoke the aboutToSetValue in the property.
            if (mProp.signalCounter == 0)
                mProp.aboutToSetValue();

            mProp.signalCounter++;
        }

        ~AtomicPropertyChange() {
            mProp.signalCounter--;

            // Signal counter == 0? Then we need to invoke the hasSetValue in the property.
            if (mProp.signalCounter == 0)
                   mProp.hasSetValue();
        }

    private:
        P & mProp; /**< Referenced to property we work on */
    };

    static AtomicPropertyChange * getAtomicPropertyChange(P & prop) { return new AtomicPropertyChange(prop); }

private:

    int signalCounter; /**< Counter for invoking transaction start/stop */
};

} // namespace App

#endif // APP_PROPERTY_H
