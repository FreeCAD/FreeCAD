/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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
#if 0
#   include <boost/any.hpp>
#else
#   include "stx/any.hpp"
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

// Unfortunately VS2013 does not support default move constructor, so we have
// to implement them manually
#define FC_DEFAULT_CTORS(_t) \
    _t(const _t &) = default;\
    _t &operator=(const _t &) = default;\
    _t(_t &&other) { *this = std::move(other); }\
    _t &operator=(_t &&other)


class AppExport ObjectIdentifier {

public:

    class AppExport DocumentMapper {
    public:
        DocumentMapper(const std::map<std::string,std::string> &);
        ~DocumentMapper();
    };

    class String {
        friend class ObjectIdentifier;

    public:

        // Constructor
        String(const std::string & s = "", bool _isRealString = false, bool _forceIdentifier = false)
            : str(s), isString(_isRealString), forceIdentifier(_forceIdentifier) 
        { }

        String(std::string &&s, bool _isRealString = false, bool _forceIdentifier = false)
            : str(std::move(s)), isString(_isRealString), forceIdentifier(_forceIdentifier) 
        { }

        FC_DEFAULT_CTORS(String) {
            str = std::move(other.str);
            isString = other.isString;
            forceIdentifier = other.forceIdentifier;
            return *this;
        }

        // Accessors

        /** Returns the string */
        const std::string &getString() const { return str; }

        /** Return true is string need to be quoted */
        bool isRealString() const { return isString; }

        bool isForceIdentifier() const { return forceIdentifier; }

        /** Returns a possibly quoted string */
        std::string toString(bool toPython=false) const;

        void toString(std::ostream &os, bool toPython=false) const;

        friend inline std::ostream &operator<<(std::ostream &os, const String &s) {
            s.toString(os);
            return os;
        }

        // Operators

        operator const std::string&() const { return str; }

        operator const char *() const { return str.c_str(); }

        bool operator==(const String & other) const { return str == other.str; }

        bool operator!=(const String & other) const { return str != other.str; }

        bool operator>=(const String & other) const { return str >= other.str; }

        bool operator<(const String & other) const { return str < other.str; }

        bool operator>(const String & other) const { return str > other.str; }

        void checkImport(const App::DocumentObject *owner, 
                const App::DocumentObject *obj=0, String *objName=0);
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
            LABEL,
        } ;

    public:

        // Constructors
        FC_DEFAULT_CTORS(Component) {
            name = std::move(other.name);
            type = other.type;
            begin = other.begin;
            end = other.end;
            step = other.step;
            return *this;
        }

        Component(const String &_name = String(), typeEnum _type=SIMPLE, 
                int begin=INT_MAX, int end=INT_MAX, int step=1);
        Component(String &&_name, typeEnum _type=SIMPLE, 
                int begin=INT_MAX, int end=INT_MAX, int step=1);

        static Component SimpleComponent(const char * _component);
        static Component SimpleComponent(const String &_component); 
        static Component SimpleComponent(String &&_component);

        static Component LabelComponent(const char * _component);
        static Component LabelComponent(const std::string & _component);
        static Component LabelComponent(std::string &&_component);

        static Component ArrayComponent(int _index);

        static Component RangeComponent(int _begin, int _end = INT_MAX, int _step=1);

        static Component MapComponent(const String &_key);
        static Component MapComponent(String &&_key);

        // Type queries

        bool isSimple() const { return type == SIMPLE; }

        bool isLabel() const { return type == LABEL; }

        bool isMap() const { return type == MAP; }

        bool isArray() const { return type == ARRAY; }

        bool isRange() const { return type == RANGE; }

        // Accessors

        void toString(std::ostream &ss, bool toPython=false) const;

        const std::string &getName() const { return name.getString(); }

        int getIndex() const {return begin;}
        size_t getIndex(size_t count) const;

        int getBegin() const { return begin; }
        int getEnd() const { return end; }
        int getStep() const { return step; }

        // Operators

        bool operator==(const Component & other) const;
        bool operator<(const Component & other) const;

        Py::Object get(const Py::Object &pyobj) const;
        void set(Py::Object &pyobj, const Py::Object &value) const;
        void del(Py::Object &pyobj) const;

    private:

        String name;
        typeEnum type;
        int begin;
        int end;
        int step;
        friend class ObjectIdentifier;

    };

    static Component SimpleComponent(const char * _component) 
        {return Component::SimpleComponent(_component);}

    static Component SimpleComponent(const String &_component) 
        {return Component::SimpleComponent(_component);}

    static Component SimpleComponent(String &&_component) 
        {return Component::SimpleComponent(std::move(_component));}

    static Component LabelComponent(std::string &&_component) 
        {return Component::LabelComponent(std::move(_component));}

    static Component LabelComponent(const std::string &_component) 
        {return Component::LabelComponent(_component);}

    static Component ArrayComponent(int _index) 
        {return Component::ArrayComponent(_index); }

    static Component RangeComponent(int _begin, int _end = INT_MAX, int _step=1)
        {return Component::RangeComponent(_begin,_end,_step);}

    static Component MapComponent(const String &_key)
        {return Component::MapComponent(_key);}

    static Component MapComponent(String &&_key)
        {return Component::MapComponent(_key);}

    ObjectIdentifier(const App::PropertyContainer * _owner = 0, 
            const std::string & property = std::string(), int index=INT_MAX);

    ObjectIdentifier(const App::PropertyContainer * _owner, const char *property, int index=INT_MAX);

    ObjectIdentifier(const App::PropertyContainer * _owner, bool localProperty);

    ObjectIdentifier(const App::Property & prop, int index=INT_MAX);

    FC_DEFAULT_CTORS(ObjectIdentifier) {
        owner = other.owner;
        documentName = std::move(other.documentName);
        documentObjectName = std::move(other.documentObjectName);
        subObjectName = std::move(other.subObjectName);
        shadowSub = std::move(other.shadowSub);
        components = std::move(other.components);
        documentNameSet = other.documentNameSet;
        documentObjectNameSet = other.documentObjectNameSet;
        localProperty = other.localProperty;
        _cache = std::move(other._cache);
        _hash = other._hash;
        return *this;
    }

    virtual ~ObjectIdentifier() {}

    App::DocumentObject *getOwner() const { return owner; }

    // Components
    void addComponent(const Component &c);

    // Components
    void addComponent(Component &&c);

    // Pop components from back
    void popComponents(int count=1);

    const std::string & getPropertyName() const;

    static const std::vector<std::pair<const char *, App::Property*> > &getPseudoProperties();
    static bool isPseudoProperty(const App::Property *prop);

    const Component & getPropertyComponent(int i, int *idx=0) const;

    void setComponent(int idx, Component &&comp);
    void setComponent(int idx, const Component &comp);

    std::vector<Component> getPropertyComponents(int i=0) const;
    const std::vector<Component> &getComponents() const { return components; }

    std::string getSubPathStr(bool toPython=false, bool prefix=true) const;

    int numComponents() const;

    int numSubComponents() const;

    const std::string &toString() const;

    std::string toPersistentString() const;

    std::string toEscapedString() const;

    bool isTouched() const;

    App::Property *getProperty(int *ptype=0) const;

    App::ObjectIdentifier canonicalPath() const;

    // Document-centric functions

    void setDocumentName(String &&name, bool force = false);

    String getDocumentName() const;

    void setDocumentObjectName(String &&name, bool force = false, 
            String &&subname = String(), bool checkImport=false);

    void setDocumentObjectName(const App::DocumentObject *obj, bool force = false, 
            String &&subname = String(), bool checkImport=false);

    bool hasDocumentObjectName(bool forced=false) const;

    bool isLocalProperty() const { return localProperty; }

    String getDocumentObjectName() const;

    const std::string &getSubObjectName(bool newStyle) const;
    const std::string &getSubObjectName() const;

    typedef std::map<std::pair<App::DocumentObject*,std::string>,std::string> SubNameMap;
    void importSubNames(const SubNameMap &subNameMap);

    bool updateLabelReference(App::DocumentObject *, const std::string &, const char *);

    bool relabeledDocument(ExpressionVisitor &v, const std::string &oldLabel, const std::string &newLabel);

    /** Type for storing dependency of an ObjectIdentifier
     *
     * The dependency is a map from document object to a set of property names.
     * An object identifier may references multiple objects using syntax like
     * 'Part.Group[0].Width'. 
     *
     * Also, we use set of string instead of set of Property pointer, because
     * the property may not exist at the time this ObjectIdentifier is
     * constructed.
     */
    typedef std::map<App::DocumentObject *, std::set<std::string> > Dependencies;

    /** Get dependencies of this object identifier
     *
     * @param needProps: whether need property dependencies.
     * @param labels: optional return of any label references.
     *
     * In case of multi-object references, like 'Part.Group[0].Width', if no
     * property dependency is required, then this function will only return the
     * first referred object dependency. Or else, all object and property
     * dependencies will be returned.
     */
    Dependencies getDep(bool needProps, std::vector<std::string> *labels=0) const;

    /** Get dependencies of this object identifier
     *
     * @param deps: returns the depdenencies.
     * @param needProps: whether need property dependencies.
     * @param labels: optional return of any label references.
     *
     * In case of multi-object references, like 'Part.Group[0].Width', if no
     * property dependency is required, then this function will only return the
     * first referred object dependency. Or else, all object and property
     * dependencies will be returned.
     */
    void getDep(Dependencies &deps, bool needProps, std::vector<std::string> *labels=0) const;
    
    /// Returns all label references
    void getDepLabels(std::vector<std::string> &labels) const;

    App::Document *getDocument(String name = String(), bool *ambiguous=0) const;

    App::DocumentObject *getDocumentObject() const;
    
    std::vector<std::string> getStringList() const;

    App::ObjectIdentifier relativeTo(const App::ObjectIdentifier & other) const;

    bool replaceObject(ObjectIdentifier &res, const App::DocumentObject *parent,
            App::DocumentObject *oldObj, App::DocumentObject *newObj) const;

    // Operators

    App::ObjectIdentifier & operator<<(const Component & value);
    App::ObjectIdentifier & operator<<(Component &&value);

    bool operator==(const ObjectIdentifier & other) const;

    bool operator!=(const ObjectIdentifier & other) const;

    bool operator<(const ObjectIdentifier &other) const;

    // Getter

    App::any getValue(bool pathValue=false, bool *isPseudoProperty=0) const;

    Py::Object getPyValue(bool pathValue=false, bool *isPseudoProperty=0, bool *isReadOnly=0) const;

    // Setter: is const because it does not alter the object state,
    // but does have an aiding effect.

    void setValue(const App::any & value) const;

    void setPyValue(Py::Object value) const;

    // Static functions

    static ObjectIdentifier parse(const App::DocumentObject *docObj, const std::string & str);

    std::string resolveErrorString() const;

    bool adjustLinks(ExpressionVisitor &v, const std::set<App::DocumentObject *> &inList);

    bool updateElementReference(ExpressionVisitor &v, App::DocumentObject *feature=0, bool reverse=false);

    void resolveAmbiguity();

    bool verify(const App::Property &prop, bool silent=false) const;

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

    friend struct ResolveResults;

    App::Property *resolveProperty(const App::DocumentObject *obj, 
        String &subname, int &propertyIndex, App::DocumentObject *&sobj,int &ptype) const;

    void getSubPathStr(std::ostream &ss, const ResolveResults &result, bool toPython=false, bool prefix=true) const;

    Py::Object access(const ResolveResults &rs,
            Py::Object *value=0, Dependencies *deps=0) const;

    void resolve(ResolveResults & results) const;
    void resolveAmbiguity(ResolveResults &results);

    static App::DocumentObject *getDocumentObject(
            const App::Document *doc, const String &name, std::bitset<32> &flags);

    void getDepLabels(const ResolveResults &result, std::vector<std::string> &labels) const;

    App::DocumentObject * owner;
    String  documentName;
    String  documentObjectName;
    String  subObjectName;
    std::pair<std::string,std::string> shadowSub;
    std::vector<Component> components;
    bool documentNameSet;
    bool documentObjectNameSet;
    bool localProperty;

private:
    std::string _cache; // Cached string represstation of this identifier
    std::size_t _hash; // Cached hash of this string
};

inline std::size_t hash_value(const App::ObjectIdentifier & path) {
    return path.hash();
}

/** Helper function to convert Python object to/from App::any
*
* WARNING! Must hold Python global interpreter lock before calling these
* functions
*/
//@{
App::any AppExport pyObjectToAny(Py::Object pyobj, bool check=true);
Py::Object AppExport pyObjectFromAny(const App::any &value);
//@}
}

namespace std {

template<> 
struct hash<App::ObjectIdentifier> {
    typedef App::ObjectIdentifier argument_type;
    typedef std::size_t result_type;
    inline result_type operator()(argument_type const& s) const {
        return s.hash();
    }
};
}

#endif
