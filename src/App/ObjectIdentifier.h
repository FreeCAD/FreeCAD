/***************************************************************************
 *   Copyright (c) Eivind Kvedalen <eivind@kvedalen.name> 2015             *
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


#ifndef APP_PATH_H
#define APP_PATH_H

#include <vector>
#include <string>
#include <boost/any.hpp>

namespace App
{

class Property;
class Document;
class PropertyContainer;
class DocumentObject;

AppExport std::string quote(const std::string &input);

class AppExport ObjectIdentifier {

public:

    class String {

    public:

        // Constructor
        String(const std::string & s = "", bool _isRealString = false, bool _forceIdentifier = false) : str(s), isString(_isRealString), forceIdentifier(_forceIdentifier) { }

        // Accessors

        /** Returns the string */
        std::string getString() const { return str; }

        /** Return true is string need to be quoted */
        bool isRealString() const { return isString; }

        bool isForceIdentifier() const { return forceIdentifier; }

        /** Returns a possibly quoted string */
        std::string toString() const;

        // Operators

        operator std::string() const { return str; }

        operator const char *() const { return str.c_str(); }

        bool operator==(const String & other) const { return str == other.str; }

        bool operator!=(const String & other) const { return str != other.str; }

        bool operator>=(const String & other) const { return str >= other.str; }

        bool operator<(const String & other) const { return str < other.str; }

        bool operator>(const String & other) const { return str > other.str; }

    private:

        std::string str;
        bool isString;
        bool forceIdentifier;

    };

    /**
     * @brief A component is a part of a Path object, and is used to either
     * name a property or a field within a property. A component can be either
     * a single entry, and array, or a map to other sub-fields.
     */

    class AppExport Component {

    private:

        enum typeEnum {
            SIMPLE,
            MAP,
            ARRAY
        } ;

    public:

        // Constructors

        Component(const String &_component, typeEnum _type = SIMPLE, int _index = -1, String _key = String());

        static Component SimpleComponent(const char * _component);

        static Component SimpleComponent(const String & _component);

        static Component ArrayComponent(const String &_component, int _index);

        static Component MapComponent(const String &_component, const String &_key);

        // Type queries

        bool isSimple() const { return type == SIMPLE; }

        bool isMap() const { return type == MAP; }

        bool isArray() const { return type == ARRAY; }

        // Accessors

        std::string toString() const;

        std::string getName() const { return name.getString(); }

        std::size_t getIndex() const { return static_cast<std::size_t>(index); }

        String getKey() const { return key; }

        bool getKeyIsString() const { return keyIsString; }

        // Operators

        bool operator==(const Component & other) const;

    private:

        String name;
        typeEnum type;
        int index;
        String key;
        bool keyIsString;

        friend class ObjectIdentifier;

    };

    ObjectIdentifier(const App::PropertyContainer * _owner = 0, const std::string & property = std::string());

    ObjectIdentifier(const App::Property & prop);

    // Components
    void addComponent(const Component &c) { components.push_back(c); }

    template<typename C>
    void addComponents(const C &cs) { components.insert(components.end(), cs.begin(), cs.end()); }

    const std::string getPropertyName() const;

    const Component & getPropertyComponent(int i) const;

    std::string getSubPathStr() const;

    int numComponents() const;

    int numSubComponents() const;

    virtual std::string toString() const;

    std::string toEscapedString() const;

    App::Property *getProperty() const;

    App::ObjectIdentifier canonicalPath() const;

    // Document-centric functions

    void setDocumentName(const String & name, bool force = false);

    const String getDocumentName() const;

    void setDocumentObjectName(const String & name, bool force = false);

    const String getDocumentObjectName() const;

    bool validDocumentObjectRename(const std::string &oldName, const std::string &newName);

    bool renameDocumentObject(const std::string & oldName, const std::string & newName);

    bool validDocumentRename(const std::string &oldName, const std::string &newName);

    bool renameDocument(const std::string &oldName, const std::string &newName);

    App::Document *getDocument(String name = String()) const;

    App::DocumentObject *getDocumentObject() const;

    std::vector<std::string> getStringList() const;

    App::ObjectIdentifier relativeTo(const App::ObjectIdentifier & other) const;

    // Operators

    App::ObjectIdentifier & operator<<(const Component & value);

    bool operator==(const ObjectIdentifier & other) const;

    bool operator!=(const ObjectIdentifier & other) const;

    bool operator<(const ObjectIdentifier &other) const;

    // Getter

    boost::any getValue() const;

    // Setter; is const because it does not alter the object state,
    // but does have a aide effect.

    void setValue(const boost::any & value) const;

    // Static functions

    static ObjectIdentifier parse(const App::DocumentObject *docObj, const std::string & str);

    std::string resolveErrorString() const;

protected:

    struct ResolveResults {

        ResolveResults(const ObjectIdentifier & oi);

        int propertyIndex;
        App::Document * resolvedDocument;
        String resolvedDocumentName;
        App::DocumentObject * resolvedDocumentObject;
        String resolvedDocumentObjectName;
        App::Property * resolvedProperty;
        std::string propertyName;

        std::string resolveErrorString() const;
    };

    std::string getPythonAccessor() const;

    void resolve(ResolveResults & results) const;

    App::DocumentObject *getDocumentObject(const App::Document *doc, const String &name, bool &byIdentifier) const;

    const App::PropertyContainer * owner;
    String  documentName;
    bool documentNameSet;
    String  documentObjectName;
    bool documentObjectNameSet;
    std::vector<Component> components;

};

std::size_t hash_value(const App::ObjectIdentifier & path);

}

#endif
