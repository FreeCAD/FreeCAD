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

#include <climits>
#include <memory>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <bitset>
#ifndef BOOST_105400
#include <boost/any.hpp>
#else
#include <boost_any_1_55.hpp>
#endif
#include <CXX/Objects.hxx>

namespace Base {
class PythonVariables;
}

namespace App
{

class Property;
class Document;
class PropertyContainer;
class DocumentObject;
class ExpressionVisitor;

AppExport std::string quote(const std::string &input, bool toPython=false);
AppExport std::string unquote(const std::string &input, bool isRaw=false);

class AppExport ObjectIdentifier {

public:

    class String {
        friend class ObjectIdentifier;

    public:

        // Constructor
        String(const std::string & s = "", bool _isRealString = false, bool _forceIdentifier = false) : str(s), isString(_isRealString), forceIdentifier(_forceIdentifier) { }

        // Accessors

        /** Returns the string */
        const std::string &getString() const { return str; }

        /** Return true is string need to be quoted */
        bool isRealString() const { return isString; }

        bool isForceIdentifier() const { return forceIdentifier; }

        /** Returns a possibly quoted string */
        std::string toString(bool toPython=false) const;

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
            ARRAY,
            RANGE,
        } ;

    public:

        // Constructors

        Component(const String &_name = String(), typeEnum _type=SIMPLE, 
                int begin=INT_MAX, int end=INT_MAX, int step=1);

        // Type queries

        bool isSimple() const { return type == SIMPLE; }

        bool isMap() const { return type == MAP; }

        bool isArray() const { return type == ARRAY; }

        bool isRange() const { return type == RANGE; }

        // Accessors

        void toString(std::ostringstream &ss, bool toPython=false) const;

        const std::string &getName() const { return name.getString(); }

        int getIndex() const {return begin;}
        size_t getIndex(size_t count) const;

        int getBegin() const { return begin; }
        int getEnd() const { return end; }

        // Operators

        bool operator==(const Component & other) const;
        bool operator<(const Component & other) const;

    private:

        String name;
        typeEnum type;
        int begin;
        int end;
        int step;
        friend class ObjectIdentifier;

    };

    static Component SimpleComponent(const char * _component);

    static Component SimpleComponent(const String & _component);

    static Component ArrayComponent(int _index);

    static Component RangeComponent(int _begin, int _end = INT_MAX, int _step=1);

    static Component MapComponent(const String &_key);


    ObjectIdentifier(const App::PropertyContainer * _owner = 0, 
            const std::string & property = std::string(), int index=INT_MAX);

    ObjectIdentifier(const App::Property & prop, int index=INT_MAX);

    virtual ~ObjectIdentifier() {}

    // Components
    void addComponent(const Component &c) { 
        components.push_back(c);
        _cache.clear();
    }

    template<typename C>
    void addComponents(const C &cs) { 
        components.insert(components.end(), cs.begin(), cs.end());
        _cache.clear();
    }

    std::string getPropertyName() const;

    const Component & getPropertyComponent(int i) const;

    std::vector<Component> getComponents(bool propertyOnly=true) const;

    std::string getSubPathStr(bool toPython=false) const;

    int numComponents() const;

    int numSubComponents() const;

    const std::string &toString() const;

    std::string toPersistentString() const;

    std::string toEscapedString() const;

    bool isTouched() const;

    App::Property *getProperty(int *ptype=0) const;

    App::ObjectIdentifier canonicalPath() const;

    // Document-centric functions

    void setDocumentName(const String & name, bool force = false);

    String getDocumentName() const;

    void setDocumentObjectName(const String & name, bool force = false, 
            const String &subname = String());

    void setDocumentObjectName(const App::DocumentObject *obj, bool force = false, 
            const String &subname = String());

    bool hasDocumentObjectName(bool forced=false) const;

    String getDocumentObjectName() const;

    const std::string &getSubObjectName(bool newStyle=false) const;

    void importSubNames(const std::map<std::string,std::string> &subNameMap);

    bool updateLabelReference(App::DocumentObject *, const std::string &, const char *);

    bool renameDocument(const std::string &oldName, const std::string &newName, ExpressionVisitor *v);

    std::pair<App::DocumentObject*,std::string> getDep(std::vector<std::string> *labels=0) const;

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

    boost::any getValue(bool pathValue=false) const;

    // Setter; is const because it does not alter the object state,
    // but does have a aide effect.

    void setValue(const boost::any & value) const;

    // Static functions

    static ObjectIdentifier parse(const App::DocumentObject *docObj, const std::string & str);

    std::string resolveErrorString() const;

    bool adjustLinks(const std::set<App::DocumentObject *> &inList, ExpressionVisitor *v);

    bool updateElementReference(App::DocumentObject *feature=0, bool reverse=false, ExpressionVisitor *v=0);

    void resolveAmbiguity();

    std::size_t hash() const;

protected:

    struct ResolveResults {

        ResolveResults(const ObjectIdentifier & oi);

        int propertyIndex;
        App::Document * resolvedDocument;
        String resolvedDocumentName;
        App::DocumentObject * resolvedDocumentObject;
        String resolvedDocumentObjectName;
        String subObjectName;
        App::DocumentObject * resolvedSubObject;
        App::Property * resolvedProperty;
        std::string propertyName;
        int propertyType;
        std::bitset<32> flags;

        std::string resolveErrorString() const;
        void getProperty(const ObjectIdentifier &oi);
    };

    friend class ResolveResults;

    App::Property *resolveProperty(const App::DocumentObject *obj, 
        const char *propertyName, App::DocumentObject *&sobj,int &ptype) const;

    void getSubPathStr(std::ostringstream &ss, const ResolveResults &result, bool toPython=false) const;

    std::string getPythonAccessor(const ResolveResults &rs, Base::PythonVariables &vars) const;

    void resolve(ResolveResults & results) const;
    void resolveAmbiguity(ResolveResults &results);

    App::DocumentObject *getDocumentObject(const App::Document *doc, const String &name, std::bitset<32> &flags) const;

    App::DocumentObject * owner;
    String  documentName;
    String  documentObjectName;
    String  subObjectName;
    std::pair<std::string,std::string> shadowSub;
    std::vector<Component> components;
    bool documentNameSet;
    bool documentObjectNameSet;

private:
    mutable std::string _cache; // Cached string represstation of this identifier
    mutable std::size_t _hash; // Cached hash of this string
};

std::size_t AppExport hash_value(const App::ObjectIdentifier & path);

/** Helper function to convert Python object to/from boost::any
*
* WARNING! Must hold Python global interpreter lock before calling these
* functions
*/
//@{
boost::any AppExport pyObjectToAny(Py::Object pyobj, bool check=true);
Py::Object AppExport pyObjectFromAny(const boost::any &value);
//@}

}

#endif
