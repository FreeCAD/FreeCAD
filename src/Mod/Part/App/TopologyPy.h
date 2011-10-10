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


#ifndef __TOPOLOGY_H__
#define __TOPOLOGY_H__
#if 0
// Std. configurations

#include <Base/PyObjectBase.h>


#include <TopoDS_Shape.hxx>

namespace Part
{

/** The TopoDSShape wrapper class
 *  This class wrapps the functionality of the Topology package. It wrapps not
 *  strictly after the OCC rules. It includes also a lot algorithems from oter
 *  packages like BRepTools and BRepBuilder. Also iterators and so on.
 */
class AppPartExport TopoShapePyOld :public Base::PyObjectBase
{
	/** always start with Py_Header */
	Py_Header;

protected:
	/// Destruction 
	~TopoShapePyOld();

public:

	/// Constructer 
  TopoShapePyOld(PyTypeObject *T = &Type);
  TopoShapePyOld(const TopoDS_Shape &cShape, PyTypeObject *T = &TopoShapePyOld::Type);
	/// for Construction in python 
  static PyObject *PyMake(PyTypeObject*, PyObject*, PyObject*);
  static int PyInit(PyObject*, PyObject*, PyObject*);


	//---------------------------------------------------------------------
	// exported functions goes here +++++++++++++++++++++++++++++++++++++++
	//---------------------------------------------------------------------

	/// Gets the OCC Label
	TopoDS_Shape getTopoShape(void){return _cTopoShape;}

	//---------------------------------------------------------------------
	// python exports goes here +++++++++++++++++++++++++++++++++++++++++++	
	//---------------------------------------------------------------------

	virtual PyObject *_repr(void);  				// the representation
	PyObject *_getattr(char *attr);				// __getattr__ function
	// getter setter
	int _setattr(char *attr, PyObject *value);	// __setattr__ function

	// methods
	PYFUNCDEF_D (TopoShapePyOld,hasChild);
	PYFUNCDEF_D (TopoShapePyOld,isNull);
	PYFUNCDEF_D (TopoShapePyOld,isValid);
	PYFUNCDEF_D (TopoShapePyOld,analyze);
	PYFUNCDEF_D (TopoShapePyOld,importIGES);
	PYFUNCDEF_D (TopoShapePyOld,exportIGES);
	PYFUNCDEF_D (TopoShapePyOld,importSTEP);
	PYFUNCDEF_D (TopoShapePyOld,exportSTEP);
	PYFUNCDEF_D (TopoShapePyOld,importBREP);
	PYFUNCDEF_D (TopoShapePyOld,exportBREP);
	PYFUNCDEF_D (TopoShapePyOld,exportSTL);


  TopoDS_Shape &getShape(void){return _cTopoShape;}


protected:
	/// The OCC Label
	TopoDS_Shape _cTopoShape;

};


} //namespace App
#endif
#endif // __TOPOLOGY_H__
