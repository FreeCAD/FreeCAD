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

#include <bitset>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <boost/any.hpp>
#include <FCConfig.h>

namespace Py {
class Object;
}
namespace App
{

using any = boost::any;

template<class T>
inline const T &any_cast(const boost::any &value) {
    return boost::any_cast<const T&>(value);
}

template<class T>
inline T &any_cast(boost::any &value) {
    return boost::any_cast<T&>(value);
}

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
        explicit DocumentMapper(const std::map<std::string,std::string> &);
        ~DocumentMapper();
    };

    class String {
        friend class ObjectIdentifier;

    public:

        String(const std::string &s = "",
               bool _isRealString = false,
               bool _forceIdentifier = false)
            : str(s),
            isString(_isRealString),
            forceIdentifier(_forceIdentifier)
        {}//explicit bombs

        explicit String(std::string &&s,
               bool _isRealString = false,
               bool _forceIdentifier = false)
            : str(std::move(s)),
            isString(_isRealString),
            forceIdentifier(_forceIdentifier)
        {}

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

        // Operators

        explicit operator std::string() const { return str; }

        explicit operator const char *() const { return str.c_str(); }

        bool operator==(const String & other) const { return str == other.str; }

        bool operator!=(const String & other) const { return str != other.str; }

        bool operator>=(const String & other) const { return str >= other.str; }

        bool operator<(const String & other) const { return str < other.str; }

        bool operator>(const String & other) const { return str > other.str; }

        void checkImport(const App::DocumentObject *owner,
                const App::DocumentObject *obj=nullptr, String *objName=nullptr);
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
        FC_DEFAULT_CTORS(Component) {
            name = std::move(other.name);
            type = other.type;
            begin = other.begin;
            end = other.end;
            step = other.step;
            return *this;
        }

        Component(const String &_name = String(), typeEnum _type=SIMPLE,
                int begin=INT_MAX, int end=INT_MAX, int step=1);//explicit bombs
        Component(String &&_name, typeEnum _type=SIMPLE,
                int begin=INT_MAX, int end=INT_MAX, int step=1);//explicit bombs

        static Component SimpleComponent(const char * _component);

        static Component SimpleComponent(const String & _component);
        static Component SimpleComponent(String &&_component);

        static Component ArrayComponent(int _index);

        static Component RangeComponent(int _begin, int _end = INT_MAX, int _step=1);

        static Component MapComponent(const String &_key);
        static Component MapComponent(String &&_key);

        // Type queries

        bool isSimple() const { return type == SIMPLE; }

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

    static Component SimpleComponent(const String & _component)
        {return Component::SimpleComponent(_component);}

    static Component SimpleComponent(String &&_component)
        {return Component::SimpleComponent(std::move(_component));}

   static Component SimpleComponent(const std::string _component)
        {return Component::SimpleComponent(_component.c_str());}

    static Component ArrayComponent(int _index)
        {return Component::ArrayComponent(_index); }

    static Component RangeComponent(int _begin, int _end = INT_MAX, int _step=1)
        {return Component::RangeComponent(_begin,_end,_step);}

    static Component MapComponent(const String &_key)
        {return Component::MapComponent(_key);}

    static Component MapComponent(String &&_key)
        {return Component::MapComponent(_key);}

    explicit ObjectIdentifier(const App::PropertyContainer * _owner = nullptr,
            const std::string & property = std::string(), int index=INT_MAX);

    ObjectIdentifier(const App::PropertyContainer * _owner, bool localProperty);

    ObjectIdentifier(const App::Property & prop, int index=INT_MAX);//explicit bombs

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

    virtual ~ObjectIdentifier() = default;

    App::DocumentObject *getOwner() const { return owner; }

    // Components
    void addComponent(const Component &c) {
        components.push_back(c);
        _cache.clear();
    }

    // Components
    void addComponent(Component &&c) {
        components.push_back(std::move(c));
        _cache.clear();
    }

    std::string getPropertyName() const;

    template<typename C>
    void addComponents(const C &cs) { components.insert(components.end(), cs.begin(), cs.end()); }

    const Component & getPropertyComponent(int i, int *idx=nullptr) const;

    void setComponent(int idx, Component &&comp);
    void setComponent(int idx, const Component &comp);

    std::vector<Component> getPropertyComponents() const;
    const std::vector<Component> &getComponents() const { return components; }

    std::string getSubPathStr(bool toPython=false) const;

    int numComponents() const;

    int numSubComponents() const;

    const std::string &toString() const;

    std::string toPersistentString() const;

    std::string toEscapedString() const;

    bool isTouched() const;

    App::Property *getProperty(int *ptype=nullptr) const;

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

    using SubNameMap = std::map<std::pair<App::DocumentObject*,std::string>,std::string>;
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
    using Dependencies = std::map<App::DocumentObject *, std::set<std::string> >;

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
    Dependencies getDep(bool needProps, std::vector<std::string> *labels=nullptr) const;

    /** Get dependencies of this object identifier
     *
     * @param deps: returns the dependencies.
     * @param needProps: whether need property dependencies.
     * @param labels: optional return of any label references.
     *
     * In case of multi-object references, like 'Part.Group[0].Width', if no
     * property dependency is required, then this function will only return the
     * first referred object dependency. Or else, all object and property
     * dependencies will be returned.
     */
    void getDep(Dependencies &deps, bool needProps, std::vector<std::string> *labels=nullptr) const;

    /// Returns all label references
    void getDepLabels(std::vector<std::string> &labels) const;

    App::Document *getDocument(String name = String(), bool *ambiguous=nullptr) const;

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

    App::any getValue(bool pathValue=false, bool *isPseudoProperty=nullptr) const;

    Py::Object getPyValue(bool pathValue=false, bool *isPseudoProperty=nullptr) const;

    // Setter: is const because it does not alter the object state,
    // but does have an aiding effect.

    void setValue(const App::any & value) const;

    // Static functions

    static ObjectIdentifier parse(const App::DocumentObject *docObj, const std::string & str);

    std::string resolveErrorString() const;

    bool adjustLinks(ExpressionVisitor &v, const std::set<App::DocumentObject *> &inList);

    bool updateElementReference(ExpressionVisitor &v, App::DocumentObject *feature=nullptr, bool reverse=false);

    void resolveAmbiguity();

    bool verify(const App::Property &prop, bool silent=false) const;

    std::size_t hash() const;

protected:

    struct ResolveResults {

        explicit ResolveResults(const ObjectIdentifier & oi);

        int propertyIndex{0};
        App::Document * resolvedDocument{nullptr};
        String resolvedDocumentName;
        App::DocumentObject * resolvedDocumentObject{nullptr};
        String resolvedDocumentObjectName;
        String subObjectName;
        App::DocumentObject * resolvedSubObject{nullptr};
        App::Property * resolvedProperty{nullptr};
        std::string propertyName;
        int propertyType{0};
        std::bitset<32> flags;

        std::string resolveErrorString() const;
        void getProperty(const ObjectIdentifier &oi);
    };

    friend struct ResolveResults;

    App::Property *resolveProperty(const App::DocumentObject *obj,
        const char *propertyName, App::DocumentObject *&sobj,int &ptype) const;

    void getSubPathStr(std::ostream &ss, const ResolveResults &result, bool toPython=false) const;

    Py::Object access(const ResolveResults &rs,
            Py::Object *value=nullptr, Dependencies *deps=nullptr) const;

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
    using argument_type = App::ObjectIdentifier;
    using result_type = std::size_t;
    inline result_type operator()(argument_type const& s) const {
        return s.hash();
    }
};
}

#endif
