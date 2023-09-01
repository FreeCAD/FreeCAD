/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# ifdef FC_OS_WIN32
# include <xercesc/sax/SAXParseException.hpp>
# endif
# include <list>
# include <sstream>
# include <string>
# include <utility>
#endif

#ifdef FC_OS_LINUX
# include <unistd.h>
#endif

#include "Parameter.h"
#include "Exception.h"
#include "Interpreter.h"


namespace Base {

class ParameterGrpObserver : public ParameterGrp::ObserverType
{
public:
    explicit ParameterGrpObserver(const Py::Object& obj)
    {
        inst = obj;
    }
    ParameterGrpObserver(const Py::Object& obj, const Py::Object &callable, ParameterGrp *target)
        : callable(callable), _target(target), inst(obj)
    {
    }
    ~ParameterGrpObserver() override
    {
        Base::PyGILStateLocker lock;
        inst = Py::None();
        callable = Py::None();
    }
    void OnChange(ParameterGrp::SubjectType &rCaller,ParameterGrp::MessageType Reason) override
    {
        Base::PyGILStateLocker lock;
        try {
            ParameterGrp& rGrp = static_cast<ParameterGrp&>(rCaller);
            ParameterGrp::handle hGrp(&rGrp);
            Py::Callable method(this->inst.getAttr(std::string("onChange")));
            Py::Tuple args(2);
            args.setItem(0, Py::asObject(GetPyObject(hGrp)));
            // A Reason of null indicates to clear the parameter group
            if (Reason && Reason[0] != '\0')
                args.setItem(1, Py::String(Reason));
            method.apply(args);
        }
        catch (Py::Exception&) {
            Base::PyException e; // extract the Python error text
            e.ReportException();
        }
    }
    bool isEqual(const Py::Object& obj) const
    {
        return this->inst.is(obj);
    }

public:
    Py::Object callable;
    boost::signals2::scoped_connection conn;
    ParameterGrp *_target = nullptr; // no reference counted, do not access

private:
    Py::Object inst;
};

using ParameterGrpObserverList = std::list<ParameterGrpObserver*>;

class ParameterGrpPy : public Py::PythonExtension<ParameterGrpPy>
{
public:
    static void init_type();    // announce properties and methods

    explicit ParameterGrpPy(const Base::Reference<ParameterGrp> &rcParamGrp);
    ~ParameterGrpPy() override;

    Py::Object repr() override;

    Py::Object getGroup(const Py::Tuple&);
    Py::Object getGroupName(const Py::Tuple&);
    Py::Object getGroups(const Py::Tuple&);
    Py::Object remGroup(const Py::Tuple&);
    Py::Object hasGroup(const Py::Tuple&);

    Py::Object getManager(const Py::Tuple&);
    Py::Object getParent(const Py::Tuple&);

    Py::Object isEmpty(const Py::Tuple&);
    Py::Object clear(const Py::Tuple&);

    Py::Object attach(const Py::Tuple&);
    Py::Object attachManager(const Py::Tuple& args);
    Py::Object detach(const Py::Tuple&);
    Py::Object notify(const Py::Tuple&);
    Py::Object notifyAll(const Py::Tuple&);

    Py::Object setBool(const Py::Tuple&);
    Py::Object getBool(const Py::Tuple&);
    Py::Object getBools(const Py::Tuple&);
    Py::Object remBool(const Py::Tuple&);

    Py::Object setInt(const Py::Tuple&);
    Py::Object getInt(const Py::Tuple&);
    Py::Object getInts(const Py::Tuple&);
    Py::Object remInt(const Py::Tuple&);

    Py::Object setUnsigned(const Py::Tuple&);
    Py::Object getUnsigned(const Py::Tuple&);
    Py::Object getUnsigneds(const Py::Tuple&);
    Py::Object remUnsigned(const Py::Tuple&);

    Py::Object setFloat(const Py::Tuple&);
    Py::Object getFloat(const Py::Tuple&);
    Py::Object getFloats(const Py::Tuple&);
    Py::Object remFloat(const Py::Tuple&);

    Py::Object setString(const Py::Tuple&);
    Py::Object getString(const Py::Tuple&);
    Py::Object getStrings(const Py::Tuple&);
    Py::Object remString(const Py::Tuple&);

    Py::Object importFrom(const Py::Tuple&);
    Py::Object insert(const Py::Tuple&);
    Py::Object exportTo(const Py::Tuple&);

    Py::Object getContents(const Py::Tuple&);

private:
    ParameterGrp::handle _cParamGrp;
    ParameterGrpObserverList _observers;
};

// ---------------------------------------------------------

void ParameterGrpPy::init_type()
{
    behaviors().name("ParameterGrp");
    behaviors().doc("Python interface class to set parameters");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();
    behaviors().readyType();

    add_varargs_method("GetGroup",&ParameterGrpPy::getGroup,"GetGroup(str)");
    add_varargs_method("GetGroupName",&ParameterGrpPy::getGroupName,"GetGroupName()");
    add_varargs_method("GetGroups",&ParameterGrpPy::getGroups,"GetGroups()");
    add_varargs_method("RemGroup",&ParameterGrpPy::remGroup,"RemGroup(str)");
    add_varargs_method("HasGroup",&ParameterGrpPy::hasGroup,"HasGroup(str)");

    add_varargs_method("Manager",&ParameterGrpPy::getManager,"Manager()");
    add_varargs_method("Parent",&ParameterGrpPy::getParent,"Parent()");

    add_varargs_method("IsEmpty",&ParameterGrpPy::isEmpty,"IsEmpty()");
    add_varargs_method("Clear",&ParameterGrpPy::clear,"Clear()");

    add_varargs_method("Attach",&ParameterGrpPy::attach,"Attach()");
    add_varargs_method("AttachManager",&ParameterGrpPy::attachManager,
        "AttachManager(observer) -- attach parameter manager for notification\n\n"
        "This method attaches a user defined observer to the manager (i.e. the root)\n"
        "of the current parameter group to receive notification of all its parameters\n"
        "and those from its sub-groups\n\n"
        "The method expects the observer to have a callable attribute as shown below\n"
        "       slotParamChanged(param, tp, name, value)\n"
        "where 'param' is the parameter group causing the change, 'tp' is the type of\n"
        "the parameter, 'name' is the name of the parameter, and 'value' is the current\n"
        "value.\n\n"
        "The possible value of type are, 'FCBool', 'FCInt', 'FCUint', 'FCFloat', 'FCText',\n"
        "and 'FCParamGroup'. The notification is triggered when value is changed, in which\n"
        "case 'value' contains the new value in text form, or, when the parameter is removed,\n"
        "in which case 'value' is empty.\n\n"
        "For 'FCParamGroup' type, the observer will be notified in the following events.\n"
        "* Group creation: both 'name' and 'value' contain the name of the new group\n"
        "* Group removal: both 'name' and 'value' are empty\n"
        "* Group rename: 'name' is the new name, and 'value' is the old name");
    add_varargs_method("Detach",&ParameterGrpPy::detach,"Detach()");
    add_varargs_method("Notify",&ParameterGrpPy::notify,"Notify()");
    add_varargs_method("NotifyAll",&ParameterGrpPy::notifyAll,"NotifyAll()");

    add_varargs_method("SetBool",&ParameterGrpPy::setBool,"SetBool()");
    add_varargs_method("GetBool",&ParameterGrpPy::getBool,"GetBool()");
    add_varargs_method("GetBools",&ParameterGrpPy::getBools,"GetBools()");
    add_varargs_method("RemBool",&ParameterGrpPy::remBool,"RemBool()");

    add_varargs_method("SetInt",&ParameterGrpPy::setInt,"SetInt()");
    add_varargs_method("GetInt",&ParameterGrpPy::getInt,"GetInt()");
    add_varargs_method("GetInts",&ParameterGrpPy::getInts,"GetInts()");
    add_varargs_method("RemInt",&ParameterGrpPy::remInt,"RemInt()");

    add_varargs_method("SetUnsigned",&ParameterGrpPy::setUnsigned,"SetUnsigned()");
    add_varargs_method("GetUnsigned",&ParameterGrpPy::getUnsigned,"GetUnsigned()");
    add_varargs_method("GetUnsigneds",&ParameterGrpPy::getUnsigneds,"GetUnsigneds()");
    add_varargs_method("RemUnsigned",&ParameterGrpPy::remUnsigned,"RemUnsigned()");

    add_varargs_method("SetFloat",&ParameterGrpPy::setFloat,"SetFloat()");
    add_varargs_method("GetFloat",&ParameterGrpPy::getFloat,"GetFloat()");
    add_varargs_method("GetFloats",&ParameterGrpPy::getFloats,"GetFloats()");
    add_varargs_method("RemFloat",&ParameterGrpPy::remFloat,"RemFloat()");

    add_varargs_method("SetString",&ParameterGrpPy::setString,"SetString()");
    add_varargs_method("GetString",&ParameterGrpPy::getString,"GetString()");
    add_varargs_method("GetStrings",&ParameterGrpPy::getStrings,"GetStrings()");
    add_varargs_method("RemString",&ParameterGrpPy::remString,"RemString()");

    add_varargs_method("Import",&ParameterGrpPy::importFrom,"Import()");
    add_varargs_method("Insert",&ParameterGrpPy::insert,"Insert()");
    add_varargs_method("Export",&ParameterGrpPy::exportTo,"Export()");

    add_varargs_method("GetContents",&ParameterGrpPy::getContents,"GetContents()");
}

ParameterGrpPy::ParameterGrpPy(const Base::Reference<ParameterGrp> &rcParamGrp)
  : _cParamGrp(rcParamGrp)
{
}

ParameterGrpPy::~ParameterGrpPy()
{
    for (ParameterGrpObserver* obs : _observers) {
        if (!obs->_target)
            _cParamGrp->Detach(obs);
        delete obs;
    }
}

Py::Object ParameterGrpPy::repr()
{
    std::stringstream s;
    s << "<ParameterGrp at " << this << ">";
    return Py::String(s.str());
}

Py::Object ParameterGrpPy::importFrom(const Py::Tuple& args)
{
    char *pstr = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->importFrom(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::insert(const Py::Tuple& args)
{
    char *pstr = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->insert(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::exportTo(const Py::Tuple& args)
{
    char *pstr = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->exportTo(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::getGroup(const Py::Tuple& args)
{
    char *pstr = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    try {
        // get the Handle of the wanted group
        Base::Reference<ParameterGrp> handle = _cParamGrp->GetGroup(pstr);
        if (handle.isValid()) {
            // create a python wrapper class
            ParameterGrpPy *pcParamGrp = new ParameterGrpPy(handle);
            // increment the ref count
            return Py::asObject(pcParamGrp);
        }
        else {
            throw Py::RuntimeError("GetGroup failed");
        }
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        throw Py::Exception();
    }
}

Py::Object ParameterGrpPy::getManager(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    // get the Handle of the wanted group
    Base::Reference<ParameterGrp> handle = _cParamGrp->Manager();
    if (handle.isValid()) {
        // create a python wrapper class
        ParameterGrpPy *pcParamGrp = new ParameterGrpPy(handle);
        // increment the ref count
        return Py::asObject(pcParamGrp);
    }

    return Py::None();
}

Py::Object ParameterGrpPy::getParent(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    // get the Handle of the wanted group
    Base::Reference<ParameterGrp> handle = _cParamGrp->Parent();
    if (handle.isValid()) {
        // create a python wrapper class
        ParameterGrpPy *pcParamGrp = new ParameterGrpPy(handle);
        // increment the ref count
        return Py::asObject(pcParamGrp);
    }

    return Py::None();
}

Py::Object ParameterGrpPy::getGroupName(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    // get the Handle of the wanted group
    std::string name = _cParamGrp->GetGroupName();
    return Py::String(name);
}

Py::Object ParameterGrpPy::getGroups(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    // get the Handle of the wanted group
    std::vector<Base::Reference<ParameterGrp> > handle = _cParamGrp->GetGroups();
    Py::List list;
    for (const auto& it : handle) {
        list.append(Py::String(it->GetGroupName()));
    }

    return list;
}

Py::Object ParameterGrpPy::setBool(const Py::Tuple& args)
{
    char *pstr = nullptr;
    int  Bool = 0;
    if (!PyArg_ParseTuple(args.ptr(), "si", &pstr,&Bool))
        throw Py::Exception();

    _cParamGrp->SetBool(pstr,Bool!=0);
    return Py::None();
}

Py::Object ParameterGrpPy::getBool(const Py::Tuple& args)
{
    char *pstr = nullptr;
    int  Bool=0;
    if (!PyArg_ParseTuple(args.ptr(), "s|i", &pstr,&Bool))
        throw Py::Exception();

    return Py::Boolean(_cParamGrp->GetBool(pstr,Bool!=0));
}

Py::Object ParameterGrpPy::getBools(const Py::Tuple& args)
{
    char *filter=nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "|s", &filter))
        throw Py::Exception();

    std::vector<std::pair<std::string,bool> > map = _cParamGrp->GetBoolMap(filter);
    Py::List list;
    for (const auto& it : map) {
        list.append(Py::String(it.first));
    }

    return list;
}

Py::Object ParameterGrpPy::setInt(const Py::Tuple& args)
{
    char *pstr = nullptr;
    int  Int = 0;
    if (!PyArg_ParseTuple(args.ptr(), "si", &pstr,&Int))
        throw Py::Exception();

    _cParamGrp->SetInt(pstr,Int);
    return Py::None();
}

Py::Object ParameterGrpPy::getInt(const Py::Tuple& args)
{
    char *pstr = nullptr;
    int  Int=0;
    if (!PyArg_ParseTuple(args.ptr(), "s|i", &pstr,&Int))
        throw Py::Exception();
    return Py::Long(_cParamGrp->GetInt(pstr,Int));
}

Py::Object ParameterGrpPy::getInts(const Py::Tuple& args)
{
    char *filter=nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "|s", &filter))
        throw Py::Exception();

    std::vector<std::pair<std::string,long> > map = _cParamGrp->GetIntMap(filter);
    Py::List list;
    for (const auto& it : map) {
        list.append(Py::String(it.first));
    }

    return list;
}

Py::Object ParameterGrpPy::setUnsigned(const Py::Tuple& args)
{
    char *pstr = nullptr;
    unsigned int  UInt = 0;
    if (!PyArg_ParseTuple(args.ptr(), "sI", &pstr,&UInt))
        throw Py::Exception();

    _cParamGrp->SetUnsigned(pstr,UInt);
    return Py::None();
}

Py::Object ParameterGrpPy::getUnsigned(const Py::Tuple& args)
{
    char *pstr = nullptr;
    unsigned int  UInt=0;
    if (!PyArg_ParseTuple(args.ptr(), "s|I", &pstr,&UInt))
        throw Py::Exception();
    return Py::Long(_cParamGrp->GetUnsigned(pstr,UInt));
}

Py::Object ParameterGrpPy::getUnsigneds(const Py::Tuple& args)
{
    char *filter=nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "|s", &filter))
        throw Py::Exception();

    std::vector<std::pair<std::string,unsigned long> > map = _cParamGrp->GetUnsignedMap(filter);
    Py::List list;
    for (const auto& it : map) {
        list.append(Py::String(it.first));
    }

    return list;
}

Py::Object ParameterGrpPy::setFloat(const Py::Tuple& args)
{
    char *pstr = nullptr;
    double  Float{};
    if (!PyArg_ParseTuple(args.ptr(), "sd", &pstr,&Float))
        throw Py::Exception();

    _cParamGrp->SetFloat(pstr,Float);
    return Py::None();
}

Py::Object ParameterGrpPy::getFloat(const Py::Tuple& args)
{
    char *pstr = nullptr;
    double  Float=0.0;
    if (!PyArg_ParseTuple(args.ptr(), "s|d", &pstr,&Float))
        throw Py::Exception();

    return Py::Float(_cParamGrp->GetFloat(pstr,Float));
}

Py::Object ParameterGrpPy::getFloats(const Py::Tuple& args)
{
    char *filter=nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "|s", &filter))
        throw Py::Exception();

    std::vector<std::pair<std::string,double> > map = _cParamGrp->GetFloatMap(filter);
    Py::List list;
    for (const auto& it : map) {
        list.append(Py::String(it.first));
    }

    return list;
}

Py::Object ParameterGrpPy::setString(const Py::Tuple& args)
{
    char *pstr = nullptr;
    char *  str = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "ss", &pstr,&str))
        throw Py::Exception();

    _cParamGrp->SetASCII(pstr,str);
    return Py::None();
}

Py::Object ParameterGrpPy::getString(const Py::Tuple& args)
{
    char *pstr = nullptr;
    char *  str="";
    if (!PyArg_ParseTuple(args.ptr(), "s|s", &pstr,&str))
        throw Py::Exception();

    return Py::String(_cParamGrp->GetASCII(pstr,str));
}

Py::Object ParameterGrpPy::getStrings(const Py::Tuple& args)
{
    char *filter=nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "|s", &filter))
        throw Py::Exception();

    std::vector<std::pair<std::string,std::string> > map = _cParamGrp->GetASCIIMap(filter);
    Py::List list;
    for (const auto& it : map) {
        list.append(Py::String(it.first));
    }

    return list;
}

Py::Object ParameterGrpPy::remInt(const Py::Tuple& args)
{
    char *pstr = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->RemoveInt(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::remUnsigned(const Py::Tuple& args)
{
    char *pstr = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->RemoveUnsigned(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::remBool(const Py::Tuple& args)
{
    char *pstr = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->RemoveBool(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::remGroup(const Py::Tuple& args)
{
    char *pstr = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->RemoveGrp(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::remFloat(const Py::Tuple& args)
{
    char *pstr = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->RemoveFloat(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::remString(const Py::Tuple& args)
{
    char *pstr = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->RemoveASCII(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::clear(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    _cParamGrp->Clear();
    return Py::None();
}

Py::Object ParameterGrpPy::isEmpty(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    return Py::Boolean(_cParamGrp->IsEmpty());
}

Py::Object ParameterGrpPy::hasGroup(const Py::Tuple& args)
{
    char *pstr = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    return Py::Boolean(_cParamGrp->HasGroup(pstr));
}

Py::Object ParameterGrpPy::attach(const Py::Tuple& args)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "O", &obj))
        throw Py::Exception();

    Py::Object o(obj);
    if (!o.hasAttr(std::string("onChange")))
        throw Py::TypeError("Object has no onChange attribute");

    for (ParameterGrpObserver* it : _observers) {
        if (it->isEqual(o)) {
            throw Py::RuntimeError("Object is already attached.");
        }
    }

    ParameterGrpObserver* obs = new ParameterGrpObserver(o);
    _cParamGrp->Attach(obs);
    _observers.push_back(obs);

    return Py::None();
}

Py::Object ParameterGrpPy::attachManager(const Py::Tuple& args)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "O", &obj))
        throw Py::Exception();

    if (!_cParamGrp->Manager())
        throw Py::RuntimeError("Parameter has no manager");

    Py::Object o(obj);
    if (!o.hasAttr(std::string("slotParamChanged")))
        throw Py::TypeError("Object has no slotParamChanged attribute");

    Py::Object attr(o.getAttr("slotParamChanged"));
    if (!attr.isCallable())
        throw Py::TypeError("Object has no slotParamChanged callable attribute");

    for (ParameterGrpObserver* it : _observers) {
        if (it->isEqual(o)) {
            throw Py::RuntimeError("Object is already attached.");
        }
    }

    ParameterGrpObserver* obs = new ParameterGrpObserver(o, attr, _cParamGrp);
    obs->conn = _cParamGrp->Manager()->signalParamChanged.connect(
        [obs](ParameterGrp *Param, ParameterGrp::ParamType Type, const char *Name, const char *Value) {
            if (!Param) return;
            for (auto p = Param; p; p = p->Parent()) {
                if (p == obs->_target) {
                    Base::PyGILStateLocker lock;
                    Py::TupleN args(
                        Py::asObject(new ParameterGrpPy(Param)),
                        Py::String(ParameterGrp::TypeName(Type)),
                        Py::String(Name ? Name : ""),
                        Py::String(Value ? Value : ""));
                    try {
                        Py::Callable(obs->callable).apply(args);
                    } catch (Py::Exception &) {
                        Base::PyException e;
                        e.ReportException();
                    }
                    break;
                }
            }
        });

    _observers.push_back(obs);
    return Py::None();
}

Py::Object ParameterGrpPy::detach(const Py::Tuple& args)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "O", &obj))
        throw Py::Exception();

    Py::Object o(obj);
    if (!o.hasAttr(std::string("onChange")))
        throw Py::TypeError("Object has no onChange attribute");

    for (ParameterGrpObserverList::iterator it = _observers.begin(); it != _observers.end(); ++it) {
        if ((*it)->isEqual(o)) {
            ParameterGrpObserver* obs = *it;
            _observers.erase(it);
            _cParamGrp->Detach(obs);
            delete obs;
            break;
        }
    }

    return Py::None();
}

Py::Object ParameterGrpPy::notify(const Py::Tuple& args)
{
    char *pstr = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->Notify(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::notifyAll(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    _cParamGrp->NotifyAll();
    return Py::None();
}

Py::Object ParameterGrpPy::getContents(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    if (_cParamGrp->IsEmpty())
        return Py::None();

    Py::List list;
    // filling up Text nodes
    std::vector<std::pair<std::string,std::string> > mcTextMap = _cParamGrp->GetASCIIMap();
    for (const auto & it : mcTextMap) {
        Py::Tuple t2(3);
        t2.setItem(0,Py::String("String"));
        t2.setItem(1,Py::String(it.first.c_str()));
        t2.setItem(2,Py::String(it.second.c_str()));
        list.append(t2);
    }

    // filling up Int nodes
    std::vector<std::pair<std::string,long> > mcIntMap = _cParamGrp->GetIntMap();
    for (const auto & it : mcIntMap) {
        Py::Tuple t3(3);
        t3.setItem(0,Py::String("Integer"));
        t3.setItem(1,Py::String(it.first.c_str()));
        t3.setItem(2,Py::Long(it.second));
        list.append(t3);
    }

    // filling up Float nodes
    std::vector<std::pair<std::string,double> > mcFloatMap = _cParamGrp->GetFloatMap();
    for (const auto & it : mcFloatMap) {
        Py::Tuple t4(3);
        t4.setItem(0,Py::String("Float"));
        t4.setItem(1,Py::String(it.first.c_str()));
        t4.setItem(2,Py::Float(it.second));
        list.append(t4);
    }

    // filling up bool nodes
    std::vector<std::pair<std::string,bool> > mcBoolMap = _cParamGrp->GetBoolMap();
    for (const auto & it : mcBoolMap) {
        Py::Tuple t5(3);
        t5.setItem(0,Py::String("Boolean"));
        t5.setItem(1,Py::String(it.first.c_str()));
        t5.setItem(2,Py::Boolean(it.second));
        list.append(t5);
    }

    // filling up UInt nodes
    std::vector<std::pair<std::string,unsigned long> > mcUIntMap = _cParamGrp->GetUnsignedMap();
    for (const auto & it : mcUIntMap) {
        Py::Tuple t6(3);
        t6.setItem(0,Py::String("Unsigned Long"));
        t6.setItem(1,Py::String(it.first.c_str()));
        t6.setItem(2,Py::Long(it.second));
        list.append(t6);
    }

    return list;
}

} // namespace Base

/** python wrapper function
*/
PyObject* GetPyObject(const Base::Reference<ParameterGrp> &hcParamGrp)
{
    static bool init = false;
    if (!init) {
        init = true;
        Base::ParameterGrpPy::init_type();
    }

    return new Base::ParameterGrpPy(hcParamGrp);
}
