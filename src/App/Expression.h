/***************************************************************************
 *   Copyright (c) Eivind Kvedalen (eivind@kvedalen.name) 2015             *
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

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <string>
#include <boost/pool/pool_alloc.hpp>
#include <boost/tuple/tuple.hpp>
#include <Base/Exception.h>
#include <Base/Unit.h>
#include <App/PropertyLinks.h>
#include <App/ObjectIdentifier.h>
#include <Base/BaseClass.h>
#include <Base/Quantity.h>
#include <set>
#include <list>
#include <App/Range.h>

namespace Base {
class XMLReader;
}

namespace App  {

class DocumentObject;
class Expression;
class Document;

typedef std::unique_ptr<Expression> ExpressionPtr;

typedef std::map<App::DocumentObject*, std::map<std::string, std::vector<ObjectIdentifier> > > ExpressionDeps;

class AppExport ExpressionVisitor {
public:
    virtual ~ExpressionVisitor() {}
    virtual void visit(Expression &e) = 0;
    virtual void setExpressionChanged() {}
    virtual int changed() const { return 0;}
    virtual void reset() {}
    virtual App::PropertyLinkBase* getPropertyLink() {return 0;}

protected:
    void getIdentifiers(Expression &e, std::set<App::ObjectIdentifier> &); 
    void getDeps(Expression &e, ExpressionDeps &); 
    void getDepObjects(Expression &e, std::set<App::DocumentObject*> &, std::vector<std::string> *); 
    bool adjustLinks(Expression &e, const std::set<App::DocumentObject*> &inList);
    bool renameDocument(Expression &e, const std::string &oldName, const std::string &newName);
    bool renameObjectIdentifier(Expression &e,const std::map<ObjectIdentifier,ObjectIdentifier> &paths,
            const ObjectIdentifier &path);
    bool updateElementReference(Expression &e, App::DocumentObject *feature,bool reverse);
    void importSubNames(Expression &e, const ObjectIdentifier::SubNameMap &subNameMap);
    void updateLabelReference(Expression &e, App::DocumentObject *obj, 
            const std::string &ref, const char *newLabel);
    void moveCells(Expression &e, const CellAddress &address, int rowCount, int colCount);
    void offsetCells(Expression &e, int rowOffset, int colOffset);
};

template<class P> class ExpressionModifier : public ExpressionVisitor {
public:
    ExpressionModifier(P & _prop)
        : prop(_prop),_changed(0) 
    { 
        propLink = dynamic_cast<App::PropertyLinkBase*>(&prop);
    }

    virtual ~ExpressionModifier() { }

    virtual void setExpressionChanged() override{
        ++_changed;
        if (!signaller)
            signaller = boost::shared_ptr<typename AtomicPropertyChangeInterface<P>::AtomicPropertyChange>(AtomicPropertyChangeInterface<P>::getAtomicPropertyChange(prop));
    }

    virtual int changed() const override { return _changed; }

    virtual void reset() override {_changed = 0;}

    virtual App::PropertyLinkBase* getPropertyLink() override {return propLink;}

protected:
    P & prop;
    App::PropertyLinkBase *propLink;
    boost::shared_ptr<typename AtomicPropertyChangeInterface<P>::AtomicPropertyChange> signaller;
    int _changed;
};

// The reason of not using boost::pool_allocator directly is because the one
// come from boost 1.55 lacks proper support of std::move(), which is required
// in order to support vector of unique_ptr. Specifically, it lacks a
// forwarding construct() which is provided by this class.
//
// WARNING!!! Another change here is to default not using mutex. Assuming FC is
// (mostly) single threaded
#define _ExpressionAllocDefine(_name,_parent) \
template<typename T,\
    typename UserAllocator = boost::default_user_allocator_new_delete,\
    typename Mutex = boost::details::pool::null_mutex,\
    unsigned NextSize = 32,\
    unsigned MaxSize = 8192 > \
class _name: \
    public _parent<T, UserAllocator, Mutex, NextSize, MaxSize>\
{\
    typedef _parent<T, UserAllocator, Mutex, NextSize, MaxSize> inherited;\
public:\
    template <typename U>\
    struct rebind { \
      typedef _name<U, UserAllocator, Mutex, NextSize, MaxSize> other;\
    };\
    _name():inherited() {}\
    template <typename U>\
    _name(const _name<U, UserAllocator, Mutex, NextSize, MaxSize> &other)\
        :inherited(other)\
    {}\
    template <typename U, typename... Args>\
    static void construct(U* ptr, Args&&... args)\
    { new (ptr) U(std::forward<Args>(args)...); }\
}

_ExpressionAllocDefine(_ExpressionAllocator,boost::pool_allocator);
#define ExpressionAllocator(_t) _ExpressionAllocator<_t>

/**
  * Base class for expressions.
  *
  */

class AppExport Expression : public Base::BaseClass {
    TYPESYSTEM_HEADER();

public:
    virtual ~Expression();

    Expression(const App::DocumentObject *_owner);

    Expression(const Expression&) = delete;
    void operator=(const Expression &)=delete;

    virtual bool isTouched() const { return false; }

    enum EvalOption {
        OptionCallFrame = 1,
        OptionPythonMode = 2,
    };
    ExpressionPtr eval(int options=0) const;

    App::any getValueAsAny(int options=0) const;

    bool isSame(const Expression &other) const;

    std::string toString(bool persistent=false, bool checkPriority=false, int indent=0) const;

    void toString(std::ostream &ss, bool persistent, bool checkPriority, int indent) const;

    struct StringMaker {
        const Expression &e;
        bool persistent;
        bool checkPriority;
        int indent;

        inline StringMaker(const Expression &e, bool persistent, bool checkPriority, int indent)
            :e(e),persistent(persistent),checkPriority(checkPriority),indent(indent)
        {}

        friend inline std::ostream &operator<<(std::ostream &ss, const StringMaker &maker) {
            maker.e.toString(ss,maker.persistent,maker.checkPriority,maker.indent);
            return ss;
        }
    };
    inline StringMaker toStr(bool persistent=false, bool checkPriority=false, int indent=0) const {
        return StringMaker(*this,persistent,checkPriority,indent);
    }

    ExpressionPtr copy() const;

    static ExpressionPtr parse(const App::DocumentObject * owner, 
            const char* buffer, size_t len=0, bool verbose=false, bool pythonMode=false);

    static ExpressionPtr parse(const App::DocumentObject * owner, 
            const std::string &buffer, bool verbose=false, bool pythonMode=false) 
    {
        return parse(owner,buffer.c_str(),buffer.size(),verbose,pythonMode);
    }

    static ExpressionPtr parseUnit(const App::DocumentObject * owner, const char* buffer, size_t len=0);

    static ExpressionPtr parseUnit(const App::DocumentObject * owner, const std::string &buffer) {
        return parseUnit(owner,buffer.c_str(),buffer.size());
    }

    virtual int priority() const;

    virtual int jump() const {return 0;}

    void getIdentifiers(std::set<App::ObjectIdentifier> &) const;
    std::set<App::ObjectIdentifier> getIdentifiers() const;

    void getDeps(ExpressionDeps &deps) const;
    ExpressionDeps getDeps() const;

    std::set<App::DocumentObject*> getDepObjects(std::vector<std::string> *labels=0) const;
    void getDepObjects(std::set<App::DocumentObject*> &, std::vector<std::string> *labels=0) const;

    ExpressionPtr importSubNames(const std::map<std::string,std::string> &nameMap) const;

    ExpressionPtr updateLabelReference(App::DocumentObject *obj, 
            const std::string &ref, const char *newLabel) const;

    bool adjustLinks(const std::set<App::DocumentObject*> &inList);

    virtual ExpressionPtr simplify() const { return std::move(copy()); }

    void visit(ExpressionVisitor & v);

    class Exception : public Base::Exception {
    public:
        Exception(const char *sMessage) : Base::Exception(sMessage) { }
    };

    App::DocumentObject *getOwner() const { return owner; }

    struct Component;
    typedef std::unique_ptr<Component> ComponentPtr;

    virtual void addComponent(ComponentPtr &&component);

    typedef std::vector<ComponentPtr, ExpressionAllocator(ComponentPtr) > ComponentList;
    typedef std::vector<std::string, ExpressionAllocator(std::string) > StringList;
    typedef std::vector<ExpressionPtr, ExpressionAllocator(ExpressionPtr) > ExpressionList;

    static ComponentPtr createComponent(const std::string &n);
    static ComponentPtr createComponent(ExpressionPtr &&e1, ExpressionPtr &&e2=ExpressionPtr(), 
            ExpressionPtr &&e3=ExpressionPtr(), bool isRange=false);

    virtual bool needLineEnd() const {return false;};

    bool hasComponent() const {return !components.empty();}

protected:
    virtual bool _isIndexable() const {return false;}
    virtual ExpressionPtr _copy() const = 0;
    virtual void _toString(std::ostream &ss, bool persistent, int indent) const = 0;
    virtual void _getDeps(ExpressionDeps &) const  {}
    virtual void _getDepObjects(std::set<App::DocumentObject*> &, std::vector<std::string> *) const  {}
    virtual void _getIdentifiers(std::set<App::ObjectIdentifier> &) const  {}
    virtual bool _adjustLinks(const std::set<App::DocumentObject*> &, ExpressionVisitor &) {return false;}
    virtual bool _updateElementReference(App::DocumentObject *,bool,ExpressionVisitor &) {return false;}
    virtual bool _renameDocument(const std::string &, const std::string &, ExpressionVisitor &) {return false;}
    virtual void _importSubNames(const ObjectIdentifier::SubNameMap &) {}
    virtual void _updateLabelReference(App::DocumentObject *, const std::string &, const char *) {}
    virtual bool _renameObjectIdentifier(const std::map<ObjectIdentifier,ObjectIdentifier> &, 
                                         const ObjectIdentifier &, ExpressionVisitor &) {return false;}
    virtual void _moveCells(const CellAddress &, int, int, ExpressionVisitor &) {}
    virtual void _offsetCells(int, int, ExpressionVisitor &) {}
    virtual App::any _getValueAsAny() const = 0;
    virtual ExpressionPtr _eval() const {return 0;}
    virtual void _visit(ExpressionVisitor &) {}

    void swapComponents(Expression &other) {components.swap(other.components);}

    friend ExpressionVisitor;

protected:
    App::DocumentObject * owner; /**< The document object used to access unqualified variables (i.e local scope) */

    ComponentList components;
};

#define EXPR_TYPESYSTEM_HEADER() \
    TYPESYSTEM_HEADER();\
public:\
    void operator delete(void *p)
        
/**
  * Part of an expressions that contains a unit.
  *
  */

class  AppExport UnitExpression : public Expression {
    EXPR_TYPESYSTEM_HEADER();
public:
    static ExpressionPtr create(const App::DocumentObject *owner, 
            const Base::Quantity &quantity, std::string &&unitStr);

    static ExpressionPtr create(const App::DocumentObject *owner, 
            const Base::Quantity &quantity, const char *unitStr);

    virtual ExpressionPtr simplify() const;

    void setUnit(const Base::Quantity &_quantity);

    double getValue() const { return quantity.getValue(); }

    const Base::Unit & getUnit() const { return quantity.getUnit(); }

    const Base::Quantity & getQuantity() const { return quantity; }

    const std::string getUnitString() const { return unitStr; }

    double getScaler() const { return quantity.getValue(); }

protected:
    UnitExpression(const App::DocumentObject *_owner):Expression(_owner) {}

    virtual void _toString(std::ostream &ss, bool persistent, int indent) const;
    virtual ExpressionPtr _copy() const;
    virtual ExpressionPtr _eval() const;
    virtual App::any _getValueAsAny() const;

protected:
    Base::Quantity quantity;
    std::string unitStr; /**< The unit string from the original parsed string */
};

/**
  * Class implementing a number with an optional unit
  */

class AppExport NumberExpression : public UnitExpression {
    EXPR_TYPESYSTEM_HEADER();
public:
    static ExpressionPtr create(const App::DocumentObject *_owner, const Base::Quantity & quantity);
    static ExpressionPtr create(const App::DocumentObject *_owner, double fvalue);

    void negate();

protected:
    NumberExpression(const App::DocumentObject *_owner):UnitExpression(_owner) {}

    virtual void _toString(std::ostream &ss, bool persistent, int indent) const;
    virtual ExpressionPtr _copy() const;
};

class AppExport ConstantExpression : public NumberExpression {
    EXPR_TYPESYSTEM_HEADER();
public:
    static ExpressionPtr create(const App::DocumentObject *owner, 
            std::string &&name, const Base::Quantity &_quantity);

    static ExpressionPtr create(const App::DocumentObject *owner, 
            const char *name, const Base::Quantity &_quantity);

    std::string getName() const { return name; }

    virtual ExpressionPtr simplify() const;

protected:
    ConstantExpression(const App::DocumentObject *_owner):NumberExpression(_owner){}

    virtual ExpressionPtr _eval() const;
    virtual App::any _getValueAsAny() const;
    virtual void _toString(std::ostream &ss, bool persistent, int indent) const;
    virtual ExpressionPtr _copy() const;

    std::string name; /**< Constant's name */
};

} // end of namespace App

#endif // EXPRESSION_H
