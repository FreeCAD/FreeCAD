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

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <deque>
#include <set>
#include <string>

#include <App/PropertyLinks.h>
#include <App/ObjectIdentifier.h>
#include <App/Range.h>
#include <Base/Exception.h>
#include <Base/BaseClass.h>


namespace Base
{
class Quantity;
}

namespace App  {

class DocumentObject;
class Expression;
class Document;

using ExpressionPtr = std::unique_ptr<Expression>;

AppExport bool isAnyEqual(const App::any &v1, const App::any &v2);
AppExport Base::Quantity anyToQuantity(const App::any &value, const char *errmsg = nullptr);

// Map of depending objects to a map of depending property name to the full referencing object identifier
using ExpressionDeps = std::map<App::DocumentObject*, std::map<std::string, std::vector<ObjectIdentifier> > >;

class AppExport ExpressionVisitor {
public:
    virtual ~ExpressionVisitor() = default;
    virtual void visit(Expression &e) = 0;
    virtual void aboutToChange() {}
    virtual int changed() const { return 0;}
    virtual void reset() {}
    virtual App::PropertyLinkBase* getPropertyLink() {return nullptr;}

protected:
    void getIdentifiers(Expression &e, std::map<App::ObjectIdentifier, bool> &);
    bool adjustLinks(Expression &e, const std::set<App::DocumentObject*> &inList);
    bool relabeledDocument(Expression &e, const std::string &oldName, const std::string &newName);
    bool renameObjectIdentifier(Expression &e,
            const std::map<ObjectIdentifier,ObjectIdentifier> &, const ObjectIdentifier &);
    void collectReplacement(Expression &e, std::map<ObjectIdentifier,ObjectIdentifier> &,
            const App::DocumentObject *parent, App::DocumentObject *oldObj, App::DocumentObject *newObj) const;
    bool updateElementReference(Expression &e, App::DocumentObject *feature,bool reverse);
    void importSubNames(Expression &e, const ObjectIdentifier::SubNameMap &subNameMap);
    void updateLabelReference(Expression &e, App::DocumentObject *obj,
            const std::string &ref, const char *newLabel);
    void moveCells(Expression &e, const CellAddress &address, int rowCount, int colCount);
    void offsetCells(Expression &e, int rowOffset, int colOffset);
};

template<class P> class ExpressionModifier : public ExpressionVisitor {
public:
    explicit ExpressionModifier(P & _prop)
        : prop(_prop)
        , propLink(Base::freecad_dynamic_cast<App::PropertyLinkBase>(&prop))
        , signaller(_prop,false)
    {}

    ~ExpressionModifier() override = default;

    void aboutToChange() override{
        ++_changed;
        signaller.aboutToChange();
    }

    int changed() const override { return _changed; }

    void reset() override {_changed = 0;}

    App::PropertyLinkBase* getPropertyLink() override {return propLink;}

protected:
    P & prop;
    App::PropertyLinkBase *propLink;
    typename AtomicPropertyChangeInterface<P>::AtomicPropertyChange signaller;
    int _changed{0};
};

/**
  * Base class for expressions.
  *
  */

class AppExport Expression : public Base::BaseClass {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:

    explicit Expression(const App::DocumentObject * _owner);

    ~Expression() override;

    virtual bool isTouched() const { return false; }

    Expression * eval() const;

    std::string toString(bool persistent=false, bool checkPriority=false, int indent=0) const;
    void toString(std::ostream &os, bool persistent=false, bool checkPriority=false, int indent=0) const;

    static Expression * parse(const App::DocumentObject * owner, const std::string& buffer);

    Expression * copy() const;

    virtual int priority() const;

    void getIdentifiers(std::map<App::ObjectIdentifier,bool> &) const;
    std::map<App::ObjectIdentifier,bool> getIdentifiers() const;

    enum DepOption {
        DepNormal,
        DepHidden,
        DepAll,
    };
    void getDeps(ExpressionDeps &deps, int option=DepNormal) const;
    ExpressionDeps getDeps(int option=DepNormal) const;

    std::map<App::DocumentObject*,bool> getDepObjects(std::vector<std::string> *labels=nullptr) const;
    void getDepObjects(std::map<App::DocumentObject*,bool> &, std::vector<std::string> *labels=nullptr) const;

    ExpressionPtr importSubNames(const std::map<std::string,std::string> &nameMap) const;

    ExpressionPtr updateLabelReference(App::DocumentObject *obj,
            const std::string &ref, const char *newLabel) const;

    ExpressionPtr replaceObject(const App::DocumentObject *parent,
            App::DocumentObject *oldObj, App::DocumentObject *newObj) const;

    bool adjustLinks(const std::set<App::DocumentObject*> &inList);

    virtual Expression * simplify() const = 0;

    void visit(ExpressionVisitor & v);

    class Exception : public Base::Exception {
    public:
        explicit Exception(const char *sMessage) : Base::Exception(sMessage) { }
    };

    App::DocumentObject *  getOwner() const { return owner; }
    struct Component;

    virtual void addComponent(Component* component);

    using ComponentList = std::vector<Component*>;

    static Component *createComponent(const std::string &n);
    static Component *createComponent(Expression *e1, Expression *e2=nullptr,
            Expression *e3=nullptr, bool isRange=false);

    bool hasComponent() const {return !components.empty();}

    boost::any getValueAsAny() const;

    Py::Object getPyValue() const;

    bool isSame(const Expression &other, bool checkComment=true) const;

    friend class ExpressionVisitor;

protected:
    virtual bool _isIndexable() const {return false;}
    virtual Expression *_copy() const = 0;
    virtual void _toString(std::ostream &ss, bool persistent, int indent=0) const = 0;
    virtual void _getIdentifiers(std::map<App::ObjectIdentifier,bool> &) const  {}
    virtual bool _adjustLinks(const std::set<App::DocumentObject*> &, ExpressionVisitor &) {return false;}
    virtual bool _updateElementReference(App::DocumentObject *,bool,ExpressionVisitor &) {return false;}
    virtual bool _relabeledDocument(const std::string &, const std::string &, ExpressionVisitor &) {return false;}
    virtual void _importSubNames(const ObjectIdentifier::SubNameMap &) {}
    virtual void _updateLabelReference(App::DocumentObject *, const std::string &, const char *) {}
    virtual bool _renameObjectIdentifier(const std::map<ObjectIdentifier,ObjectIdentifier> &,
                                         const ObjectIdentifier &, ExpressionVisitor &) {return false;}
    virtual void _collectReplacement(std::map<ObjectIdentifier,ObjectIdentifier> &,
        const App::DocumentObject *parent, App::DocumentObject *oldObj, App::DocumentObject *newObj) const
    {
        (void)parent;
        (void)oldObj;
        (void)newObj;
    }
    virtual void _moveCells(const CellAddress &, int, int, ExpressionVisitor &) {}
    virtual void _offsetCells(int, int, ExpressionVisitor &) {}
    virtual Py::Object _getPyValue() const = 0;
    virtual void _visit(ExpressionVisitor &) {}

protected:
    App::DocumentObject * owner; /**< The document object used to access unqualified variables (i.e local scope) */

    ComponentList components;

public:
    std::string comment;
};

}

#endif // EXPRESSION_H
