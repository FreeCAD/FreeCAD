/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include "PreCompiled.h"

#include <Base/PyObjectBase.h>
#include <Base/Exception.h>
#include <Base/Console.h>

#include "UnitTestPy.h"
#include "UnitTestImp.h"


using namespace TestGui;


void UnitTestDialogPy::init_type()
{
    behaviors().name("TestGui.UnitTest");
    behaviors().doc("About TestGui.UnitTest");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();

    add_varargs_method("clearErrorList",&UnitTestDialogPy::clearErrorList,"clearErrorList");
    add_varargs_method("insertError",&UnitTestDialogPy::insertError,"insertError");
    add_varargs_method("setUnitTest",&UnitTestDialogPy::setUnitTest,"setUnitTest");
    add_varargs_method("getUnitTest",&UnitTestDialogPy::getUnitTest,"getUnitTest");
    add_varargs_method("setStatusText",&UnitTestDialogPy::setStatusText,"setStatusText");
    add_varargs_method("setProgressFraction",&UnitTestDialogPy::setProgressFrac,"setProgressFraction");
    add_varargs_method("errorDialog",&UnitTestDialogPy::errorDialog,"errorDialog");
    add_varargs_method("setRunCount",&UnitTestDialogPy::setRunCount,"setRunCount");
    add_varargs_method("setFailCount",&UnitTestDialogPy::setFailCount,"setFailCount");
    add_varargs_method("setErrorCount",&UnitTestDialogPy::setErrorCount,"setErrorCount");
    add_varargs_method("setRemainCount",&UnitTestDialogPy::setRemainCount,"setRemainCount");
    add_varargs_method("updateGUI",&UnitTestDialogPy::updateGUI,"updateGUI");
}

UnitTestDialogPy::UnitTestDialogPy()
{
}

UnitTestDialogPy::~UnitTestDialogPy()
{
}

Py::Object UnitTestDialogPy::repr()
{
    return Py::String("UnitTest");
}

Py::Object UnitTestDialogPy::getattr(const char * attr)
{
    return Py::PythonExtension<UnitTestDialogPy>::getattr(attr);
}

int UnitTestDialogPy::setattr(const char * attr, const Py::Object & value)
{
    return Py::PythonExtension<UnitTestDialogPy>::setattr(attr, value);
}

Py::Object UnitTestDialogPy::clearErrorList(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    UnitTestDialog::instance()->clearErrorList();
    return Py::None();
}

Py::Object UnitTestDialogPy::insertError(const Py::Tuple& args)
{
    char *failure=0;
    char *details=0;
    if (!PyArg_ParseTuple(args.ptr(), "ss", &failure,&details))
        throw Py::Exception();

    UnitTestDialog::instance()->insertError(QString::fromLatin1(failure),
                                            QString::fromLatin1(details));
    return Py::None();
}

Py::Object UnitTestDialogPy::setUnitTest(const Py::Tuple& args)
{
    char *pstr=0;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    UnitTestDialog::instance()->setUnitTest(QString::fromLatin1(pstr));
    return Py::None();
}

Py::Object UnitTestDialogPy::getUnitTest(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::String((const char*)UnitTestDialog::instance()->getUnitTest().toAscii());
}

Py::Object UnitTestDialogPy::setStatusText(const Py::Tuple& args)
{
    char *pstr=0;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr))
        throw Py::Exception();

    UnitTestDialog::instance()->setStatusText(QString::fromLatin1(pstr));
    return Py::None();
}

Py::Object UnitTestDialogPy::setProgressFrac(const Py::Tuple& args)
{
    float fraction;
    char* pColor=0;
    if (!PyArg_ParseTuple(args.ptr(), "f|s",&fraction, &pColor))
        throw Py::Exception();

    if (pColor)
        UnitTestDialog::instance()->setProgressFraction(fraction,QString::fromLatin1(pColor));
    else
      UnitTestDialog::instance()->setProgressFraction(fraction);
    return Py::None();
}

Py::Object UnitTestDialogPy::errorDialog(const Py::Tuple& args)
{
    char *title=0;
    char *message=0;
    if (!PyArg_ParseTuple(args.ptr(), "ss", &title, &message))
        throw Py::Exception();
    UnitTestDialog::instance()->showErrorDialog(title,message);
    return Py::None();
}

Py::Object UnitTestDialogPy::setRunCount(const Py::Tuple& args)
{
    int count;
    if (!PyArg_ParseTuple(args.ptr(), "i", &count))
        throw Py::Exception();
    UnitTestDialog::instance()->setRunCount(count);
    return Py::None();
}

Py::Object UnitTestDialogPy::setFailCount(const Py::Tuple& args)
{
    int count;
    if (!PyArg_ParseTuple(args.ptr(), "i", &count))
        throw Py::Exception();
    UnitTestDialog::instance()->setFailCount(count);
    return Py::None();
}

Py::Object UnitTestDialogPy::setErrorCount(const Py::Tuple& args)
{
    int count;
    if (!PyArg_ParseTuple(args.ptr(), "i", &count))
        throw Py::Exception();
    UnitTestDialog::instance()->setErrorCount(count);
    return Py::None();
}

Py::Object UnitTestDialogPy::setRemainCount(const Py::Tuple& args)
{
    int count;
    if (!PyArg_ParseTuple(args.ptr(), "i", &count))
        throw Py::Exception();
    UnitTestDialog::instance()->setRemainCount(count);
    return Py::None();
}

Py::Object UnitTestDialogPy::updateGUI(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    qApp->processEvents();
    return Py::None();
}

//--------------------------------------------------------------------------
// Type structure
//--------------------------------------------------------------------------

PyTypeObject TestGui::UnitTestPy::Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,						/*ob_size*/
	"TestGui.UnitTest",				/*tp_name*/
	sizeof(UnitTestPy),			/*tp_basicsize*/
	0,						/*tp_itemsize*/
	/* methods */
	PyDestructor,	  		/*tp_dealloc*/
	0,			 			/*tp_print*/
	__getattr, 				/*tp_getattr*/
	__setattr, 				/*tp_setattr*/
	0,						/*tp_compare*/
	__repr,					/*tp_repr*/
	0,						/*tp_as_number*/
	0,						/*tp_as_sequence*/
	0,						/*tp_as_mapping*/
	0,						/*tp_hash*/
	0,						/*tp_call */
  0,                                                /*tp_str  */
  0,                                                /*tp_getattro*/
  0,                                                /*tp_setattro*/
  /* --- Functions to access object as input/output buffer ---------*/
  0,                                                /* tp_as_buffer */
  /* --- Flags to define presence of optional/expanded features */
  Py_TPFLAGS_HAVE_CLASS,                            /*tp_flags */
  "About TestGui.UnitTest",                         /*tp_doc */
  0,                                                /*tp_traverse */
  0,                                                /*tp_clear */
  0,                                                /*tp_richcompare */
  0,                                                /*tp_weaklistoffset */
  0,                                                /*tp_iter */
  0,                                                /*tp_iternext */
  0,                                                /*tp_methods */
  0,                                                /*tp_members */
  0,                                                /*tp_getset */
  &Base::PyObjectBase::Type,                        /*tp_base */
  0,                                                /*tp_dict */
  0,                                                /*tp_descr_get */
  0,                                                /*tp_descr_set */
  0,                                                /*tp_dictoffset */
  0,                                                /*tp_init */
  0,                                                /*tp_alloc */
  UnitTestPy::PyMake,                               /*tp_new */
  0,                                                /*tp_free   Low-level free-memory routine */
  0,                                                /*tp_is_gc  For PyObject_IS_GC */
  0,                                                /*tp_bases */
  0,                                                /*tp_mro    method resolution order */
  0,                                                /*tp_cache */
  0,                                                /*tp_subclasses */
  0                                                 /*tp_weaklist */

};

//--------------------------------------------------------------------------
// Methods structure
//--------------------------------------------------------------------------
PyMethodDef TestGui::UnitTestPy::Methods[] = {
  PYMETHODEDEF(clearErrorList)
  PYMETHODEDEF(insertError)
  PYMETHODEDEF(setUnitTest)
  PYMETHODEDEF(getUnitTest)
  PYMETHODEDEF(setStatusText)
  PYMETHODEDEF(setProgressFraction)
  PYMETHODEDEF(errorDialog)
  PYMETHODEDEF(setRunCount)
  PYMETHODEDEF(setFailCount)
  PYMETHODEDEF(setErrorCount)
  PYMETHODEDEF(setRemainCount)
  PYMETHODEDEF(updateGUI)
  {NULL, NULL}		/* Sentinel */
};

//--------------------------------------------------------------------------
// Parents structure
//--------------------------------------------------------------------------
PyParentObject TestGui::UnitTestPy::Parents[] = {&PyObjectBase::Type, NULL};

//--------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------
TestGui::UnitTestPy::UnitTestPy(PyTypeObject *T)
: PyObjectBase(0, T)
{
}

PyObject *UnitTestPy::PyMake(PyTypeObject *ignored, PyObject *args, PyObject *kwds)	// Python wrapper
{
	return new UnitTestPy();
}

//--------------------------------------------------------------------------
// destructor
//--------------------------------------------------------------------------
UnitTestPy::~UnitTestPy()						// Everything handled in parent
{
}


//--------------------------------------------------------------------------
// UnitTestPy representation
//--------------------------------------------------------------------------
PyObject *UnitTestPy::_repr(void)
{
	return Py_BuildValue("s", "UnitTest");
}

//--------------------------------------------------------------------------
// UnitTestPy Attributes
//--------------------------------------------------------------------------
PyObject *UnitTestPy::_getattr(char *attr)				// __getattr__ function: note only need to handle new state
{ 
   _getattr_up(PyObjectBase);
} 

int UnitTestPy::_setattr(char *attr, PyObject *value) 	// __setattr__ function: note only need to handle new state
{ 
  return PyObjectBase::_setattr(attr, value); 						
} 


//--------------------------------------------------------------------------
// Python wrappers
//--------------------------------------------------------------------------

PYFUNCIMP_D(UnitTestPy,clearErrorList)
{
  PY_TRY {
    UnitTestDialog::instance()->clearErrorList();
    Py_Return;
  }PY_CATCH;
}

PYFUNCIMP_D(UnitTestPy,insertError)
{
  char *failure=0;
  char *details=0;
  if (!PyArg_ParseTuple(args, "ss", &failure,&details))     // convert args: Python->C 
    return NULL;                             // NULL triggers exception
  PY_TRY {
      UnitTestDialog::instance()->insertError(QString::fromLatin1(failure),
                                              QString::fromLatin1(details));
    Py_Return;
  }PY_CATCH;
}

PYFUNCIMP_D(UnitTestPy,setUnitTest)
{
  char *pstr=0;
  if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
    return NULL;                             // NULL triggers exception
  PY_TRY {
    UnitTestDialog::instance()->setUnitTest(QString::fromLatin1(pstr));
    Py_Return;
  }PY_CATCH;
}

PYFUNCIMP_D(UnitTestPy,getUnitTest)
{
  if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
    return NULL;                             // NULL triggers exception
  PY_TRY {
    return Py_BuildValue("s", (const char*)UnitTestDialog::instance()->getUnitTest().toAscii());
  }PY_CATCH;
}

PYFUNCIMP_D(UnitTestPy,setStatusText)
{
  char *pstr=0;
  if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
    return NULL;                             // NULL triggers exception
  PY_TRY {
    UnitTestDialog::instance()->setStatusText(QString::fromLatin1(pstr));
    Py_Return;
  }PY_CATCH;
}

PYFUNCIMP_D(UnitTestPy,setProgressFraction)
{
  float fraction;
  char* pColor=0;
  if (!PyArg_ParseTuple(args, "f|s",&fraction, &pColor))     // convert args: Python->C
    return NULL;                       // NULL triggers exception

  PY_TRY {
    if (pColor)
        UnitTestDialog::instance()->setProgressFraction(fraction,QString::fromLatin1(pColor));
    else
      UnitTestDialog::instance()->setProgressFraction(fraction);
    Py_Return;
  }PY_CATCH;
}

PYFUNCIMP_D(UnitTestPy,errorDialog)
{
  char *title=0;
  char *message=0;
  if (!PyArg_ParseTuple(args, "ss", &title, &message))     // convert args: Python->C 
    return NULL;                             // NULL triggers exception
  PY_TRY {
    UnitTestDialog::instance()->showErrorDialog(title,message);
    Py_Return;
  }PY_CATCH;
}

PYFUNCIMP_D(UnitTestPy,setRunCount)
{
  int count;
  if (!PyArg_ParseTuple(args, "i", &count))     // convert args: Python->C 
    return NULL;                             // NULL triggers exception
  PY_TRY {
    UnitTestDialog::instance()->setRunCount(count);
    Py_Return;
  }PY_CATCH;
}

PYFUNCIMP_D(UnitTestPy,setFailCount)
{
  int count;
  if (!PyArg_ParseTuple(args, "i", &count))     // convert args: Python->C 
    return NULL;                             // NULL triggers exception
  PY_TRY {
    UnitTestDialog::instance()->setFailCount(count);
    Py_Return;
  }PY_CATCH;
}

PYFUNCIMP_D(UnitTestPy,setErrorCount)
{
  int count;
  if (!PyArg_ParseTuple(args, "i", &count))     // convert args: Python->C 
    return NULL;                             // NULL triggers exception
  PY_TRY {
    UnitTestDialog::instance()->setErrorCount(count);
    Py_Return;
  }PY_CATCH;
}

PYFUNCIMP_D(UnitTestPy,setRemainCount)
{
  int count;
  if (!PyArg_ParseTuple(args, "i", &count))     // convert args: Python->C 
    return NULL;                             // NULL triggers exception
  PY_TRY {
    UnitTestDialog::instance()->setRemainCount(count);
    Py_Return;
  }PY_CATCH;
}

PYFUNCIMP_D(UnitTestPy,updateGUI)
{
  PY_TRY {
    qApp->processEvents();
	  Py_Return;
  }PY_CATCH;
}

