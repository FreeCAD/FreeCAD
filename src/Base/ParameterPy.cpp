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
# include <assert.h>
# include <fcntl.h>
# include <sys/types.h>
# include <sys/stat.h>
# ifdef FC_OS_WIN32
# include <io.h>
# include <xercesc/sax/SAXParseException.hpp>
# endif
# include <stdio.h>
# include <sstream>
# include <list>
#endif


#include <fcntl.h>
#ifdef FC_OS_LINUX
# include <unistd.h>
#endif

#include "Parameter.h"
#include "Exception.h"
#include "Console.h"
#include "PyObjectBase.h"
#include "Interpreter.h"
#include <CXX/Extensions.hxx>


namespace Base {

class ParameterGrpObserver : public ParameterGrp::ObserverType
{
public:
    ParameterGrpObserver(const Py::Object& obj)
    {
        inst = obj;
    }
    virtual ~ParameterGrpObserver()
    {
        Base::PyGILStateLocker lock;
        inst = Py::None();
    }
    virtual void OnChange(ParameterGrp::SubjectType &rCaller,ParameterGrp::MessageType Reason)
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

private:
    Py::Object inst;
};

typedef std::list<ParameterGrpObserver*> ParameterGrpObserverList;

class ParameterGrpPy : public Py::PythonExtension<ParameterGrpPy>
{
public:
    static void init_type(void);    // announce properties and methods

    ParameterGrpPy(const Base::Reference<ParameterGrp> &rcParamGrp);
    ~ParameterGrpPy();

    Py::Object repr();

    Py::Object getGroup(const Py::Tuple&);
    Py::Object getGroups(const Py::Tuple&);
    Py::Object remGroup(const Py::Tuple&);
    Py::Object hasGroup(const Py::Tuple&);

    Py::Object isEmpty(const Py::Tuple&);
    Py::Object clear(const Py::Tuple&);

    Py::Object attach(const Py::Tuple&);
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
    add_varargs_method("GetGroups",&ParameterGrpPy::getGroups,"GetGroups()");
    add_varargs_method("RemGroup",&ParameterGrpPy::remGroup,"RemGroup(str)");
    add_varargs_method("HasGroup",&ParameterGrpPy::hasGroup,"HasGroup(str)");

    add_varargs_method("IsEmpty",&ParameterGrpPy::isEmpty,"IsEmpty()");
    add_varargs_method("Clear",&ParameterGrpPy::clear,"Clear()");

    add_varargs_method("Attach",&ParameterGrpPy::attach,"Attach()");
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
    for (ParameterGrpObserverList::iterator it = _observers.begin(); it != _observers.end(); ++it) {
        ParameterGrpObserver* obs = *it;
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
    char *pstr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->importFrom(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::insert(const Py::Tuple& args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->insert(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::exportTo(const Py::Tuple& args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->exportTo(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::getGroup(const Py::Tuple& args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

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

Py::Object ParameterGrpPy::getGroups(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    // get the Handle of the wanted group
    std::vector<Base::Reference<ParameterGrp> > handle = _cParamGrp->GetGroups();
    Py::List list;
    for (auto it : handle) {
        list.append(Py::String(it->GetGroupName()));
    }

    return list;
}

Py::Object ParameterGrpPy::setBool(const Py::Tuple& args)
{
    char *pstr;
    int  Bool;
    if (!PyArg_ParseTuple(args.ptr(), "si", &pstr,&Bool))
        throw Py::Exception();

    _cParamGrp->SetBool(pstr,Bool!=0);
    return Py::None();
}

Py::Object ParameterGrpPy::getBool(const Py::Tuple& args)
{
    char *pstr;
    int  Bool=0;
    if (!PyArg_ParseTuple(args.ptr(), "s|i", &pstr,&Bool))
        throw Py::Exception();

    return Py::Boolean(_cParamGrp->GetBool(pstr,Bool!=0));
}

Py::Object ParameterGrpPy::getBools(const Py::Tuple& args)
{
    char *filter=0;
    if (!PyArg_ParseTuple(args.ptr(), "|s", &filter))
        throw Py::Exception();

    std::vector<std::pair<std::string,bool> > map = _cParamGrp->GetBoolMap(filter);
    Py::List list;
    for (auto it : map) {
        list.append(Py::String(it.first));
    }

    return list;
}

Py::Object ParameterGrpPy::setInt(const Py::Tuple& args)
{
    char *pstr;
    int  Int;
    if (!PyArg_ParseTuple(args.ptr(), "si", &pstr,&Int))
        throw Py::Exception();

    _cParamGrp->SetInt(pstr,Int);
    return Py::None(); 
}

Py::Object ParameterGrpPy::getInt(const Py::Tuple& args)
{
    char *pstr;
    int  Int=0;
    if (!PyArg_ParseTuple(args.ptr(), "s|i", &pstr,&Int))
        throw Py::Exception();
#if PY_MAJOR_VERSION < 3
    return Py::Int(_cParamGrp->GetInt(pstr,Int));
#else
    return Py::Long(_cParamGrp->GetInt(pstr,Int));
#endif
}

Py::Object ParameterGrpPy::getInts(const Py::Tuple& args)
{
    char *filter=0;
    if (!PyArg_ParseTuple(args.ptr(), "|s", &filter))
        throw Py::Exception();

    std::vector<std::pair<std::string,long> > map = _cParamGrp->GetIntMap(filter);
    Py::List list;
    for (auto it : map) {
        list.append(Py::String(it.first));
    }

    return list;
}

Py::Object ParameterGrpPy::setUnsigned(const Py::Tuple& args)
{
    char *pstr;
    unsigned int  UInt;
    if (!PyArg_ParseTuple(args.ptr(), "sI", &pstr,&UInt))
        throw Py::Exception();

    _cParamGrp->SetUnsigned(pstr,UInt);
    return Py::None();
}

Py::Object ParameterGrpPy::getUnsigned(const Py::Tuple& args)
{
    char *pstr;
    unsigned int  UInt=0;
    if (!PyArg_ParseTuple(args.ptr(), "s|I", &pstr,&UInt))
        throw Py::Exception();
#if PY_MAJOR_VERSION < 3
    PyObject* val = Py_BuildValue("I",_cParamGrp->GetUnsigned(pstr,UInt));
    return Py::asObject(val);
#else
    return Py::Long(_cParamGrp->GetUnsigned(pstr,UInt));
#endif
}

Py::Object ParameterGrpPy::getUnsigneds(const Py::Tuple& args)
{
    char *filter=0;
    if (!PyArg_ParseTuple(args.ptr(), "|s", &filter))
        throw Py::Exception();

    std::vector<std::pair<std::string,unsigned long> > map = _cParamGrp->GetUnsignedMap(filter);
    Py::List list;
    for (auto it : map) {
        list.append(Py::String(it.first));
    }

    return list;
}

Py::Object ParameterGrpPy::setFloat(const Py::Tuple& args)
{
    char *pstr;
    double  Float;
    if (!PyArg_ParseTuple(args.ptr(), "sd", &pstr,&Float))
        throw Py::Exception();

    _cParamGrp->SetFloat(pstr,Float);
    return Py::None(); 
}

Py::Object ParameterGrpPy::getFloat(const Py::Tuple& args)
{
    char *pstr;
    double  Float=0.0;
    if (!PyArg_ParseTuple(args.ptr(), "s|d", &pstr,&Float))
        throw Py::Exception();

    return Py::Float(_cParamGrp->GetFloat(pstr,Float));
}

Py::Object ParameterGrpPy::getFloats(const Py::Tuple& args)
{
    char *filter=0;
    if (!PyArg_ParseTuple(args.ptr(), "|s", &filter))
        throw Py::Exception();

    std::vector<std::pair<std::string,double> > map = _cParamGrp->GetFloatMap(filter);
    Py::List list;
    for (auto it : map) {
        list.append(Py::String(it.first));
    }

    return list;
}

Py::Object ParameterGrpPy::setString(const Py::Tuple& args)
{
    char *pstr;
    char *  str;
    if (!PyArg_ParseTuple(args.ptr(), "ss", &pstr,&str))
        throw Py::Exception();

    _cParamGrp->SetASCII(pstr,str);
    return Py::None(); 
}

Py::Object ParameterGrpPy::getString(const Py::Tuple& args)
{
    char *pstr;
    char *  str="";
    if (!PyArg_ParseTuple(args.ptr(), "s|s", &pstr,&str))
        throw Py::Exception();

    return Py::String(_cParamGrp->GetASCII(pstr,str));
}

Py::Object ParameterGrpPy::getStrings(const Py::Tuple& args)
{
    char *filter=0;
    if (!PyArg_ParseTuple(args.ptr(), "|s", &filter))
        throw Py::Exception();

    std::vector<std::pair<std::string,std::string> > map = _cParamGrp->GetASCIIMap(filter);
    Py::List list;
    for (auto it : map) {
        list.append(Py::String(it.first));
    }

    return list;
}

Py::Object ParameterGrpPy::remInt(const Py::Tuple& args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->RemoveInt(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::remUnsigned(const Py::Tuple& args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->RemoveUnsigned(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::remBool(const Py::Tuple& args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->RemoveBool(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::remGroup(const Py::Tuple& args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->RemoveGrp(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::remFloat(const Py::Tuple& args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    _cParamGrp->RemoveFloat(pstr);
    return Py::None();
}

Py::Object ParameterGrpPy::remString(const Py::Tuple& args)
{
    char *pstr;
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
    char *pstr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    return Py::Boolean(_cParamGrp->HasGroup(pstr));
}

Py::Object ParameterGrpPy::attach(const Py::Tuple& args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args.ptr(), "O", &obj))
        throw Py::Exception();

    Py::Object o(obj);
    if (!o.hasAttr(std::string("onChange")))
        throw Py::TypeError("Object has no onChange attribute");

    for (ParameterGrpObserverList::iterator it = _observers.begin(); it != _observers.end(); ++it) {
        if ((*it)->isEqual(o)) {
            throw Py::RuntimeError("Object is already attached.");
        }
    }

    ParameterGrpObserver* obs = new ParameterGrpObserver(o);
    _cParamGrp->Attach(obs);
    _observers.push_back(obs);

    return Py::None();
}

Py::Object ParameterGrpPy::detach(const Py::Tuple& args)
{
    PyObject* obj;
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
    char *pstr;
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
    for (std::vector<std::pair<std::string,std::string> >::iterator It2=mcTextMap.begin();It2!=mcTextMap.end();++It2) {
        Py::Tuple t2(3);
        t2.setItem(0,Py::String("String"));
        t2.setItem(1,Py::String(It2->first.c_str()));
        t2.setItem(2,Py::String(It2->second.c_str()));
        list.append(t2);
    }

    // filling up Int nodes
    std::vector<std::pair<std::string,long> > mcIntMap = _cParamGrp->GetIntMap();
    for (std::vector<std::pair<std::string,long> >::iterator It3=mcIntMap.begin();It3!=mcIntMap.end();++It3) {
        Py::Tuple t3(3);
        t3.setItem(0,Py::String("Integer"));
        t3.setItem(1,Py::String(It3->first.c_str()));
#if PY_MAJOR_VERSION < 3
        t3.setItem(2,Py::Int(It3->second));
#else
        t3.setItem(2,Py::Long(It3->second));
#endif
        list.append(t3);
    }

    // filling up Float nodes
    std::vector<std::pair<std::string,double> > mcFloatMap = _cParamGrp->GetFloatMap();
    for (std::vector<std::pair<std::string,double> >::iterator It4=mcFloatMap.begin();It4!=mcFloatMap.end();++It4) {
        Py::Tuple t4(3);
        t4.setItem(0,Py::String("Float"));
        t4.setItem(1,Py::String(It4->first.c_str()));
        t4.setItem(2,Py::Float(It4->second));
        list.append(t4);
    }

    // filling up bool nodes
    std::vector<std::pair<std::string,bool> > mcBoolMap = _cParamGrp->GetBoolMap();
    for (std::vector<std::pair<std::string,bool> >::iterator It5=mcBoolMap.begin();It5!=mcBoolMap.end();++It5) {
        Py::Tuple t5(3);
        t5.setItem(0,Py::String("Boolean"));
        t5.setItem(1,Py::String(It5->first.c_str()));
        t5.setItem(2,Py::Boolean(It5->second));
        list.append(t5);
    }

    // filling up UInt nodes
    std::vector<std::pair<std::string,unsigned long> > mcUIntMap = _cParamGrp->GetUnsignedMap();
    for (std::vector<std::pair<std::string,unsigned long> >::iterator It6=mcUIntMap.begin();It6!=mcUIntMap.end();++It6) {
        Py::Tuple t6(3);
        t6.setItem(0,Py::String("Unsigned Long"));
        t6.setItem(1,Py::String(It6->first.c_str()));
#if PY_MAJOR_VERSION < 3
        t6.setItem(2,Py::asObject(Py_BuildValue("I",It6->second)));
#else
        t6.setItem(2,Py::Long(It6->second));
#endif
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
