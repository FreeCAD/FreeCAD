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
#endif


#include <fcntl.h>
#ifdef FC_OS_LINUX
# include <unistd.h>
#endif

#include "Parameter.h"
#include "Exception.h"
#include "Console.h"
#include "PyObjectBase.h"

using namespace Base;

//**************************************************************************
//**************************************************************************
// FCPyParametrGrp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



/** The ParameterGrp wrapper class
 */
class ParameterGrpPy :public Base::PyObjectBase
{
    /** always start with Py_Header */
    Py_Header;

protected:
    /// Destruction 
    ~ParameterGrpPy();

public:


    //---------------------------------------------------------------------
    // construction / destruction +++++++++++++++++++++++++++++++++++++++++	
    //---------------------------------------------------------------------

    /// Constructor 
    ParameterGrpPy(const Base::Reference<ParameterGrp> &rcParamGrp, PyTypeObject *T = &Type);
    /// for Construction in python 
    static PyObject *PyMake(PyObject *, PyObject *);

    //---------------------------------------------------------------------
    // python exports  ++++++++++++++++++++++++++++++++++++++++++++++++++++	
    //---------------------------------------------------------------------

    PyObject *_getattr(char *attr);				// __getattr__ function
    // getter setter
    int _setattr(char *attr, PyObject *value);	// __setattr__ function
    // methods
    PYFUNCDEF_D (ParameterGrpPy,PyGetGrp);
    PYFUNCDEF_D (ParameterGrpPy,PyRemGrp);
    PYFUNCDEF_D (ParameterGrpPy,PyHasGroup);
    PYFUNCDEF_D (ParameterGrpPy,PyIsEmpty);
    PYFUNCDEF_D (ParameterGrpPy,PyClear);
    PYFUNCDEF_D (ParameterGrpPy,PyNotify);
    PYFUNCDEF_D (ParameterGrpPy,PyNotifyAll);

    PYFUNCDEF_D (ParameterGrpPy,PySetBool);
    PYFUNCDEF_D (ParameterGrpPy,PyGetBool);
    PYFUNCDEF_D (ParameterGrpPy,PyRemBool);

    PYFUNCDEF_D (ParameterGrpPy,PySetInt);
    PYFUNCDEF_D (ParameterGrpPy,PyGetInt);
    PYFUNCDEF_D (ParameterGrpPy,PyRemInt);

    PYFUNCDEF_D (ParameterGrpPy,PySetUnsigned);
    PYFUNCDEF_D (ParameterGrpPy,PyGetUnsigned);
    PYFUNCDEF_D (ParameterGrpPy,PyRemUnsigned);

    PYFUNCDEF_D (ParameterGrpPy,PySetFloat);
    PYFUNCDEF_D (ParameterGrpPy,PyGetFloat);
    PYFUNCDEF_D (ParameterGrpPy,PyRemFloat);

    PYFUNCDEF_D (ParameterGrpPy,PySetString);
    PYFUNCDEF_D (ParameterGrpPy,PyGetString);
    PYFUNCDEF_D (ParameterGrpPy,PyRemString);

    PYFUNCDEF_D (ParameterGrpPy,importFrom);
    PYFUNCDEF_D (ParameterGrpPy,insert);
    PYFUNCDEF_D (ParameterGrpPy,exportTo);

protected:
    Base::Reference<ParameterGrp> _cParamGrp;
};


//--------------------------------------------------------------------------
// Type structure
//--------------------------------------------------------------------------

PyTypeObject ParameterGrpPy::Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                                                      /*ob_size*/
    "ParameterGrp",                                         /*tp_name*/
    sizeof(ParameterGrpPy),                                 /*tp_basicsize*/
    0,                                                      /*tp_itemsize*/
    /* methods */
    PyDestructor,                                           /*tp_dealloc*/
    0,                                                      /*tp_print*/
    __getattr,                                              /*tp_getattr*/
    __setattr,                                              /*tp_setattr*/
    0,                                                      /*tp_compare*/
    __repr,                                                 /*tp_repr*/
    0,                                                      /*tp_as_number*/
    0,                                                      /*tp_as_sequence*/
    0,                                                      /*tp_as_mapping*/
    0,                                                      /*tp_hash*/
    0,                                                      /*tp_call */
    0,                                                      /*tp_str  */
    0,                                                      /*tp_getattro*/
    0,                                                      /*tp_setattro*/
    /* --- Functions to access object as input/output buffer ---------*/
    0,                                                      /* tp_as_buffer */
    /* --- Flags to define presence of optional/expanded features */
    0,                                                      /*tp_flags */
    "Python interface class to set parameters",             /*tp_doc */
    0,                                                      /*tp_traverse */
    0,                                                      /*tp_clear */
    0,                                                      /*tp_richcompare */
    0,                                                      /*tp_weaklistoffset */
    0,                                                      /*tp_iter */
    0,                                                      /*tp_iternext */
    0,                                                      /*tp_methods */
    0,                                                      /*tp_members */
    0,                                                      /*tp_getset */
    0,                                                      /*tp_base */
    0,                                                      /*tp_dict */
    0,                                                      /*tp_descr_get */
    0,                                                      /*tp_descr_set */
    0,                                                      /*tp_dictoffset */
    0,                                                      /*tp_init */
    0,                                                      /*tp_alloc */
    0,                                                      /*tp_new */
    0,                                                      /*tp_free   Low-level free-memory routine */
    0,                                                      /*tp_is_gc  For PyObject_IS_GC */
    0,                                                      /*tp_bases */
    0,                                                      /*tp_mro    method resolution order */
    0,                                                      /*tp_cache */
    0,                                                      /*tp_subclasses */
    0,                                                      /*tp_weaklist */
    0                                                       /*tp_del */
};

//--------------------------------------------------------------------------
// Methods structure
//--------------------------------------------------------------------------
PyMethodDef ParameterGrpPy::Methods[] = {
    {"GetGroup",         (PyCFunction) sPyGetGrp,          Py_NEWARGS, 0},
    {"RemGroup",         (PyCFunction) sPyRemGrp,          Py_NEWARGS, 0},
    {"HasGroup",         (PyCFunction) sPyHasGroup,        Py_NEWARGS, 0},
    {"IsEmpty",          (PyCFunction) sPyIsEmpty,         Py_NEWARGS, 0},
    {"Clear",            (PyCFunction) sPyClear,           Py_NEWARGS, 0},
    {"Notify",           (PyCFunction) sPyNotify,          Py_NEWARGS, 0},
    {"NotifyAll",        (PyCFunction) sPyNotifyAll,       Py_NEWARGS, 0},

    {"SetBool",          (PyCFunction) sPySetBool,         Py_NEWARGS, 0},
    {"GetBool",          (PyCFunction) sPyGetBool,         Py_NEWARGS, 0},
    {"RemBool",          (PyCFunction) sPyRemBool,         Py_NEWARGS, 0},

    {"SetInt",           (PyCFunction) sPySetInt,          Py_NEWARGS, 0},
    {"GetInt",           (PyCFunction) sPyGetInt,          Py_NEWARGS, 0},
    {"RemInt",           (PyCFunction) sPyRemInt,          Py_NEWARGS, 0},

    {"SetUnsigned",      (PyCFunction) sPySetUnsigned,     Py_NEWARGS, 0},
    {"GetUnsigned",      (PyCFunction) sPyGetUnsigned,     Py_NEWARGS, 0},
    {"RemUnsigned",      (PyCFunction) sPyRemUnsigned,     Py_NEWARGS, 0},

    {"SetFloat",         (PyCFunction) sPySetFloat,        Py_NEWARGS, 0},
    {"GetFloat",         (PyCFunction) sPyGetFloat,        Py_NEWARGS, 0},
    {"RemFloat",         (PyCFunction) sPyRemFloat,        Py_NEWARGS, 0},

    {"SetString",        (PyCFunction) sPySetString,       Py_NEWARGS, 0},
    {"GetString",        (PyCFunction) sPyGetString,       Py_NEWARGS, 0},
    {"RemString",        (PyCFunction) sPyRemString,       Py_NEWARGS, 0},

    {"Import",           (PyCFunction) simportFrom,        Py_NEWARGS, 0},
    {"Insert",           (PyCFunction) sinsert,            Py_NEWARGS, 0},
    {"Export",           (PyCFunction) sexportTo,          Py_NEWARGS, 0},

    {NULL, NULL, 0, NULL}		/* Sentinel */
};

//--------------------------------------------------------------------------
// Parents structure
//--------------------------------------------------------------------------
PyParentObject ParameterGrpPy::Parents[] = {&PyObjectBase::Type,&ParameterGrpPy::Type, NULL};     

//--------------------------------------------------------------------------
// constructor
//--------------------------------------------------------------------------
ParameterGrpPy::ParameterGrpPy(const Base::Reference<ParameterGrp> &rcParamGrp, PyTypeObject *T ) 
 : PyObjectBase(0, T),_cParamGrp(rcParamGrp)
{
    //Console().Log("Create Param Group %p\n",this);
}

PyObject *ParameterGrpPy::PyMake(PyObject* /*ignored*/, PyObject* /*args*/)	// Python wrapper
{
    //return new ParameterGrpPy();			// Make new Python-able object
    return 0;
}

//--------------------------------------------------------------------------
//  FCPyParametrGrp destructor 
//--------------------------------------------------------------------------
ParameterGrpPy::~ParameterGrpPy()						// Everything handled in parent
{
    //Console().Log("Destroy ParameterGrp %p\n",this);
} 

//--------------------------------------------------------------------------
// FCPyParametrGrp Attributes
//--------------------------------------------------------------------------
PyObject *ParameterGrpPy::_getattr(char *attr)              // __getattr__ function: note only need to handle new state
{
    _getattr_up(PyObjectBase);                              // send to parent
} 

int ParameterGrpPy::_setattr(char *attr, PyObject *value)   // __setattr__ function: note only need to handle new state
{
    return PyObjectBase::_setattr(attr, value);	// send up to parent
} 


//--------------------------------------------------------------------------
// Python wrappers
//--------------------------------------------------------------------------

PyObject *ParameterGrpPy::importFrom(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->importFrom(pstr);
    }PY_CATCH;
    Py_Return;
} 

PyObject *ParameterGrpPy::insert(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->insert(pstr);
    }PY_CATCH;
    Py_Return;
} 

PyObject *ParameterGrpPy::exportTo(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->exportTo(pstr);
    }PY_CATCH;
    Py_Return;
} 

PyObject *ParameterGrpPy::PyGetGrp(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        // get the Handle of the wanted group
        Base::Reference<ParameterGrp> handle = _cParamGrp->GetGroup(pstr);
        if(handle.isValid()){
            // crate a python wrapper class
            ParameterGrpPy *pcParamGrp = new ParameterGrpPy(handle);
            // increment the reff count
            //pcParamGrp->_INCREF();
            return pcParamGrp;
        }else{
            PyErr_SetString(PyExc_IOError, "GetGroup failed");
            return 0L;
        }
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PySetBool(PyObject *args)
{
    char *pstr;
    int  Bool;
    if (!PyArg_ParseTuple(args, "si", &pstr,&Bool))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->SetBool(pstr,Bool!=0);
        Py_Return; 
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PyGetBool(PyObject *args)
{
    char *pstr;
    int  Bool=0;
    if (!PyArg_ParseTuple(args, "s|i", &pstr,&Bool))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        return Py_BuildValue("i",_cParamGrp->GetBool(pstr,Bool!=0));
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PySetInt(PyObject *args)
{
    char *pstr;
    long  Int;
    if (!PyArg_ParseTuple(args, "si", &pstr,&Int))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->SetInt(pstr,Int);
        Py_Return; 
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PyGetInt(PyObject *args)
{
    char *pstr;
    long  Int=0;
    if (!PyArg_ParseTuple(args, "s|i", &pstr,&Int))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        return Py_BuildValue("i",_cParamGrp->GetInt(pstr,Int));
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PySetUnsigned(PyObject *args)
{
    char *pstr;
    unsigned long  UInt;
    if (!PyArg_ParseTuple(args, "sI", &pstr,&UInt))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->SetUnsigned(pstr,UInt);
        Py_Return; 
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PyGetUnsigned(PyObject *args)
{
    char *pstr;
    unsigned long  UInt=0;
    if (!PyArg_ParseTuple(args, "s|I", &pstr,&UInt))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        return Py_BuildValue("I",_cParamGrp->GetUnsigned(pstr,UInt));
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PySetFloat(PyObject *args)
{
    char *pstr;
    double  Float;
    if (!PyArg_ParseTuple(args, "sd", &pstr,&Float))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->SetFloat(pstr,Float);
        Py_Return; 
    }PY_CATCH;
}

PyObject *ParameterGrpPy::PyGetFloat(PyObject *args)
{
    char *pstr;
    double  Float=0.0;
    if (!PyArg_ParseTuple(args, "s|d", &pstr,&Float))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        return Py_BuildValue("d",_cParamGrp->GetFloat(pstr,Float));
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PySetString(PyObject *args)
{
    char *pstr;
    char *  str;
    if (!PyArg_ParseTuple(args, "ss", &pstr,&str))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->SetASCII(pstr,str);
        Py_Return; 
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PyGetString(PyObject *args)
{
    char *pstr;
    char *  str="";
    if (!PyArg_ParseTuple(args, "s|s", &pstr,&str))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        return Py_BuildValue("s",_cParamGrp->GetASCII(pstr,str).c_str());
    }PY_CATCH;
} 

//----

PyObject *ParameterGrpPy::PyRemInt(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->RemoveInt(pstr);
        Py_Return; 
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PyRemUnsigned(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->RemoveUnsigned(pstr);
        Py_Return; 
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PyRemBool(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->RemoveBool(pstr);
        Py_Return; 
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PyRemGrp(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->RemoveGrp(pstr);
        Py_Return; 
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PyRemFloat(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->RemoveFloat(pstr);
        Py_Return; 
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PyRemString(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->RemoveASCII(pstr);
        Py_Return; 
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PyClear(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->Clear();
        Py_Return; 
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PyIsEmpty(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        return Py_BuildValue("i",_cParamGrp->IsEmpty());
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PyHasGroup(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        return Py_BuildValue("i",_cParamGrp->HasGroup(pstr));
    }PY_CATCH;
} 

PyObject *ParameterGrpPy::PyNotify(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    PY_TRY {
        _cParamGrp->Notify(pstr);
        Py_Return;
    }PY_CATCH;
} 
PyObject *ParameterGrpPy::PyNotifyAll(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    PY_TRY {
        _cParamGrp->NotifyAll();
        Py_Return;
    }PY_CATCH;
} 

/** python wrapper function
*/
PyObject* GetPyObject( const Base::Reference<ParameterGrp> &hcParamGrp)
{
    return new ParameterGrpPy(hcParamGrp); 
}
