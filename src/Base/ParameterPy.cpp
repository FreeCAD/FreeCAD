/***************************************************************************
 *   (c) Jürgen Riegel (juergen.riegel@web.de) 2002                        *
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
 *   Juergen Riegel 2002                                                   *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
#   include <assert.h>
#   include <fcntl.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   ifdef FC_OS_WIN32
#   include <io.h>
#   include <xercesc/sax/SAXParseException.hpp>
#   endif
#   include <stdio.h>
#   include <sstream>
#endif


#include <fcntl.h>
#ifdef FC_OS_LINUX
# include <unistd.h>
#endif

#include "Parameter.h"
#include "Exception.h"
#include "Console.h"
#include "PyObjectBase.h"
#include <CXX/Extensions.hxx>


using namespace Base;

class ParameterGrpPy : public Py::PythonExtension<ParameterGrpPy>
{
public:
    static void init_type(void);    // announce properties and methods

    ParameterGrpPy(const Base::Reference<ParameterGrp> &rcParamGrp);
    ~ParameterGrpPy();

    Py::Object repr();

    Py::Object getGroup(const Py::Tuple&);
    Py::Object remGroup(const Py::Tuple&);
    Py::Object hasGroup(const Py::Tuple&);

    Py::Object isEmpty(const Py::Tuple&);
    Py::Object clear(const Py::Tuple&);

    Py::Object notify(const Py::Tuple&);
    Py::Object notifyAll(const Py::Tuple&);

    Py::Object setBool(const Py::Tuple&);
    Py::Object getBool(const Py::Tuple&);
    Py::Object remBool(const Py::Tuple&);

    Py::Object setInt(const Py::Tuple&);
    Py::Object getInt(const Py::Tuple&);
    Py::Object remInt(const Py::Tuple&);

    Py::Object setUnsigned(const Py::Tuple&);
    Py::Object getUnsigned(const Py::Tuple&);
    Py::Object remUnsigned(const Py::Tuple&);

    Py::Object setFloat(const Py::Tuple&);
    Py::Object getFloat(const Py::Tuple&);
    Py::Object remFloat(const Py::Tuple&);

    Py::Object setString(const Py::Tuple&);
    Py::Object getString(const Py::Tuple&);
    Py::Object remString(const Py::Tuple&);

    Py::Object importFrom(const Py::Tuple&);
    Py::Object insert(const Py::Tuple&);
    Py::Object exportTo(const Py::Tuple&);

private:
    Base::Reference<ParameterGrp> _cParamGrp;
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
    add_varargs_method("RemGroup",&ParameterGrpPy::remGroup,"RemGroup(str)");
    add_varargs_method("HasGroup",&ParameterGrpPy::hasGroup,"HasGroup(str)");

    add_varargs_method("IsEmpty",&ParameterGrpPy::isEmpty,"IsEmpty()");
    add_varargs_method("Clear",&ParameterGrpPy::clear,"Clear()");

    add_varargs_method("Notify",&ParameterGrpPy::notify,"Notify()");
    add_varargs_method("NotifyAll",&ParameterGrpPy::notifyAll,"NotifyAll()");

    add_varargs_method("SetBool",&ParameterGrpPy::setBool,"SetBool()");
    add_varargs_method("GetBool",&ParameterGrpPy::getBool,"GetBool()");
    add_varargs_method("RemBool",&ParameterGrpPy::remBool,"RemBool()");

    add_varargs_method("SetInt",&ParameterGrpPy::setInt,"SetInt()");
    add_varargs_method("GetInt",&ParameterGrpPy::getInt,"GetInt()");
    add_varargs_method("RemInt",&ParameterGrpPy::remInt,"RemInt()");

    add_varargs_method("SetUnsigned",&ParameterGrpPy::setUnsigned,"SetUnsigned()");
    add_varargs_method("GetUnsigned",&ParameterGrpPy::getUnsigned,"GetUnsigned()");
    add_varargs_method("RemUnsigned",&ParameterGrpPy::remUnsigned,"RemUnsigned()");

    add_varargs_method("SetFloat",&ParameterGrpPy::setFloat,"SetFloat()");
    add_varargs_method("GetFloat",&ParameterGrpPy::getFloat,"GetFloat()");
    add_varargs_method("RemFloat",&ParameterGrpPy::remFloat,"RemFloat()");

    add_varargs_method("SetString",&ParameterGrpPy::setString,"SetString()");
    add_varargs_method("GetString",&ParameterGrpPy::getString,"GetString()");
    add_varargs_method("RemString",&ParameterGrpPy::remString,"RemString()");

    add_varargs_method("Import",&ParameterGrpPy::importFrom,"Import()");
    add_varargs_method("Insert",&ParameterGrpPy::insert,"Insert()");
    add_varargs_method("Export",&ParameterGrpPy::exportTo,"Export()");
}

ParameterGrpPy::ParameterGrpPy(const Base::Reference<ParameterGrp> &rcParamGrp)
  : _cParamGrp(rcParamGrp)
{
}

ParameterGrpPy::~ParameterGrpPy()
{
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
        // crate a python wrapper class
        ParameterGrpPy *pcParamGrp = new ParameterGrpPy(handle);
        // increment the reff count
        return Py::asObject(pcParamGrp);
    }
    else {
        throw Py::RuntimeError("GetGroup failed");
    }
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

/** python wrapper function
*/
PyObject* GetPyObject(const Base::Reference<ParameterGrp> &hcParamGrp)
{
    static bool init = false;
    if (!init) {
        init = true;
        ParameterGrpPy::init_type();
    }

    return new ParameterGrpPy(hcParamGrp); 
}
