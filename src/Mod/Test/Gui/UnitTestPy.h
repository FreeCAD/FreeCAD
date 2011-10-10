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


#ifndef TESTGUI_UNITTESTPY_H
#define TESTGUI_UNITTESTPY_H

#include <Base/PyObjectBase.h>
#include <CXX/Extensions.hxx>


namespace TestGui
{

class UnitTestDialog;
class UnitTestDialogPy : public Py::PythonExtension<UnitTestDialogPy>
{
public:
    static void init_type(void);    // announce properties and methods

    UnitTestDialogPy();
    ~UnitTestDialogPy();

    Py::Object repr();
    Py::Object getattr(const char *);
    int setattr(const char *, const Py::Object &);

    Py::Object clearErrorList   (const Py::Tuple&);
    Py::Object insertError      (const Py::Tuple&);
    Py::Object setUnitTest      (const Py::Tuple&);
    Py::Object getUnitTest      (const Py::Tuple&);
    Py::Object setStatusText    (const Py::Tuple&);
    Py::Object setProgressFrac  (const Py::Tuple&);
    Py::Object errorDialog      (const Py::Tuple&);
    Py::Object setRunCount      (const Py::Tuple&);
    Py::Object setFailCount     (const Py::Tuple&);
    Py::Object setErrorCount    (const Py::Tuple&);
    Py::Object setRemainCount   (const Py::Tuple&);
    Py::Object updateGUI        (const Py::Tuple&);

private:
    typedef PyObject* (*method_varargs_handler)(PyObject *_self, PyObject *_args);
    static method_varargs_handler pycxx_handler;
    static PyObject *method_varargs_ext_handler(PyObject *_self, PyObject *_args);
};

//===========================================================================
// UnitTestPy - Python wrapper
//===========================================================================

class UnitTestPy :public Base::PyObjectBase
{
	Py_Header;

protected:
	~UnitTestPy();

public:
  UnitTestPy(PyTypeObject *T = &Type);
	static PyObject *PyMake(PyTypeObject *, PyObject *, PyObject *);

	//---------------------------------------------------------------------
	// python exports goes here +++++++++++++++++++++++++++++++++++++++++++
	//---------------------------------------------------------------------

	virtual PyObject *_repr(void);  				// the representation
	PyObject *_getattr(char *attr);					// __getattr__ function
	int _setattr(char *attr, PyObject *value);		// __setattr__ function

  PYFUNCDEF_D(UnitTestPy,clearErrorList)
  PYFUNCDEF_D(UnitTestPy,insertError)
  PYFUNCDEF_D(UnitTestPy,setUnitTest)
  PYFUNCDEF_D(UnitTestPy,getUnitTest)
  PYFUNCDEF_D(UnitTestPy,setStatusText)
  PYFUNCDEF_D(UnitTestPy,setProgressFraction)
  PYFUNCDEF_D(UnitTestPy,errorDialog)
  PYFUNCDEF_D(UnitTestPy,setRunCount)
  PYFUNCDEF_D(UnitTestPy,setFailCount)
  PYFUNCDEF_D(UnitTestPy,setErrorCount)
  PYFUNCDEF_D(UnitTestPy,setRemainCount)
  PYFUNCDEF_D(UnitTestPy,updateGUI)
};

} //namespace TESTGUI_UNITTESTPY_H


#endif

