/***************************************************************************
 *   Copyright (c) 2015 Ian Rees <ian.rees@gmail.com>                      *
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


#ifndef BASE_ENUMERATION_H
#define BASE_ENUMERATION_H

#include <memory>
#include <string>
#include <vector>
#include <FCGlobal.h>

namespace App
{
    /// A bidirectional string-integer mapping
    /*!
     * This is mainly intended for two purposes: working around the difficulty
     * in C++ of sharing enumerations between different source files,
     * namespaces, etc. and as the data type stored by App::PropertyEnumeration
     *
     * Internally, Enumeration maintains
     *  -# Either a const pointer to an array of C-style strings, or a vector
     *  of C++ std::strings
     *  -# An integer index into that array/vector representing the string
     *  representing the instance's value.
     *
     * If built with FC_DEBUG defined, some boundaries of passed in pointers
     * will be checked.  Otherwise, the caller has the responsibility of
     * checking the limits of given indices.
     *
     * \todo Implement lazy copy
     */
    class AppExport Enumeration
    {
    public:
        class Object {
        public:
            virtual ~Object() = default;
            virtual const char* data() const = 0;
            virtual bool isEqual(const char*) const = 0;
            virtual bool isCustom() const = 0;
        };

    public:
        /// Constructs an empty Enumeration object
        Enumeration();

        /// Standard copy constructor
        Enumeration(const Enumeration& other);

        /// Constructs an Enumeration with a single element
        explicit Enumeration(const char *valStr);

        /// Constructs an Enumeration using val within list
        Enumeration(const char **list, const char *valStr);

        /// Standard destructor
        ~Enumeration();

        /** Sets the enumeration string list
         * The list is a NULL terminated array of pointers to const
         * char* strings.
         * \code
         * const char enums[] = {"Black","White","Other",NULL}
         * \endcode
         *
         * If Enumeration was already valid, will attempt to preserve
         * the string-representation value of the Enumeration
         *
         * Enumeration does not take ownership of the passed object
         */
        void setEnums(const char **plEnums);

        /// Set all enum values as vector of strings
        /*!
         * This method causes the Enumeration to dynamically allocate
         * it's own array of C Strings, which will be deleted by the
         * destructor or subsequent calls to setEnums().  So, it is
         * important to make sure the Enumeration stays in scope as
         * long as values returned by getCStr are in use.
         *
         * If Enumeration was already valid, will attempt to preserve
         * the string-representation value of the Enumeration
         */
        void setEnums(const std::vector<std::string> &values);

        /// Set the enum using a C string
        void setValue(const char *value);

        /// Overload of setValue(const char *value)
        void setValue(const std::string &value) {setValue(value.c_str());}

        /// Set the enum using a long
        /*!
         * if checkRange is set to true, throws Base::ValueError when
         * values are set out of range
         *
         * Checks for boundaries via assert()
         */
        void setValue(long value, bool checkRange = false);

        /// Checks if the property is set to a certain string value
        bool isValue(const char *value) const;

        /// Checks if a string is included in the enumeration
        bool contains(const char *value) const;

        /// Return the value as C string
        /*!
         * Returns NULL if the enumeration is invalid.
         */
        const char * getCStr() const;

        /// Return value as integer
        /*!
         * Returns -1 if the Enumeration isn't valid
         */
        int getInt() const;

        /// get all possible enum values as vector of strings
        std::vector<std::string> getEnumVector() const;

        /// returns true if the enum list is non-empty, false otherwise
        bool hasEnums() const;

        /// Returns true if the instance is in a usable state
        bool isValid() const;

        /// Returns the highest usable integer value for this enum
        /*!
         * Returns -1 if the enumeration is not valid according to isValid()
         */
        int maxValue() const;

        /// Returns true if any of the items is a user-defined string
        bool isCustom() const;

        /// Assignment operator
        Enumeration & operator=(const Enumeration &other);

        /// true iff our string representation matches other's
        /*!
         * Returns false if either Enumeration is not valid.
         */
        bool operator==(const Enumeration &other) const;

        /// true iff our string representation matches other
        /*!
         * Returns false if Enumeration is not valid.
         */
        bool operator==(const char *other) const;
    protected:

        /// Number of items
        int countItems() const;

    private:
        /// Handle to C Strings of possible enumeration values
        using ObjectPtr = std::shared_ptr<Object>;
        std::vector<ObjectPtr> enumArray;

        /// Integer value of the enumeration
        /*!
         * This serves as an index into enumArray to get the string
         * representation.
         */
        int _index;

        friend class PropertyEnumeration;
    };  // class Enumeration
}   // namespace App

#endif // #ifndef BASE_ENUMERATION_H
