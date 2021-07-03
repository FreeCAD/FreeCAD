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
#if 0
#ifndef _PreComp_
# include <assert.h>
# include <sstream>
# include <BRepTools.hxx>
# include <BRep_Builder.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <BRepCheck_Result.hxx>
# include <BRepCheck_ListIteratorOfListOfStatus.hxx>
# include <IGESControl_Controller.hxx>
# include <IGESControl_Writer.hxx>
# include <IGESControl_Reader.hxx>
# include <STEPControl_Writer.hxx>
# include <STEPControl_Reader.hxx>
# include <StlAPI_Writer.hxx>
# include <Interface_Static.hxx>
# include <TopoDS_Iterator.hxx>
#endif

#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Console.h>
#include <Base/Sequencer.h>
#include "TopologyPy.h"

# include <BRepOffsetAPI_MakeOffsetShape.hxx>
# include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Section.hxx>
# include <GeomAPI_IntSS.hxx>

using Base::Console;

using namespace Part;


//===========================================================================
// TopoShapePyOld - Warpper for the TopoDS classes
//===========================================================================

//--------------------------------------------------------------------------
// Type structure
//--------------------------------------------------------------------------

PyTypeObject TopoShapePyOld::Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,						/*ob_size*/
	"Part.Shape",				/*tp_name*/
	sizeof(TopoShapePyOld),	/*tp_basicsize*/
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
  "About shape",                               /*tp_doc */
  0,                                                /*tp_traverse */
  0,                                                /*tp_clear */
  0,                                                /*tp_richcompare */
  0,                                                /*tp_weaklistoffset */
  0,                                                /*tp_iter */
  0,                                                /*tp_iternext */
  TopoShapePyOld::Methods,                             /*tp_methods */
  0,                                                /*tp_members */
  0,                                                /*tp_getset */
  &Base::PyObjectBase::Type,                        /*tp_base */
  0,                                                /*tp_dict */
  0,                                                /*tp_descr_get */
  0,                                                /*tp_descr_set */
  0,                                                /*tp_dictoffset */
  PyInit,                                           /*tp_init */
  0,                                                /*tp_alloc */
  PyMake,                                           /*tp_new */
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
PyMethodDef TopoShapePyOld::Methods[] = {
  {"hasChild",         (PyCFunction) shasChild,         Py_NEWARGS},
  {"isNull",           (PyCFunction) sisNull,           Py_NEWARGS},
  {"isValid",          (PyCFunction) sisValid,          Py_NEWARGS},
  {"analyze",          (PyCFunction) sanalyze,          Py_NEWARGS},
  {"importIGES",       (PyCFunction) simportIGES,       Py_NEWARGS},
  {"exportIGES",       (PyCFunction) sexportIGES,       Py_NEWARGS},
  {"importSTEP",       (PyCFunction) simportSTEP,       Py_NEWARGS},
  {"exportSTEP",       (PyCFunction) sexportSTEP,       Py_NEWARGS},
  {"importBREP",       (PyCFunction) simportBREP,       Py_NEWARGS},
  {"exportBREP",       (PyCFunction) sexportBREP,       Py_NEWARGS},
  {"exportSTL",        (PyCFunction) sexportSTL,        Py_NEWARGS},

  {NULL, NULL}		/* Sentinel */
};

PyObject *TopoShapePyOld::PyMake(PyTypeObject  *ignored, PyObject *args, PyObject *kwds)  // Python wrapper
{
  return new TopoShapePyOld();
}

int TopoShapePyOld::PyInit(PyObject* self, PyObject* args, PyObject*)
{
  PyObject *pcObj=0;
  if (!PyArg_ParseTuple(args, "|O!", &(TopoShapePyOld::Type), &pcObj))     // convert args: Python->C 
    return -1;                             // NULL triggers exception 

  if ( pcObj )
  {
    TopoShapePyOld* pcShape = (TopoShapePyOld*)pcObj;
    ((TopoShapePyOld*)self)->_cTopoShape = pcShape->_cTopoShape;
  }

  return 0;
}

//--------------------------------------------------------------------------
// Parents structure
//--------------------------------------------------------------------------
PyParentObject TopoShapePyOld::Parents[] = {&Base::PyObjectBase::Type,&TopoShapePyOld::Type, NULL};     

//--------------------------------------------------------------------------
// constructor
//--------------------------------------------------------------------------
TopoShapePyOld::TopoShapePyOld(PyTypeObject *T)
: Base::PyObjectBase(0,T)
{
  Base::Console().Log("Create TopoShape: %p \n",this);
}

TopoShapePyOld::TopoShapePyOld(const TopoDS_Shape &cShape, PyTypeObject *T) 
 : PyObjectBase( 0,T), _cTopoShape(cShape)
{
	Console().Log("Create TopoShape %p\n",this);
}

//--------------------------------------------------------------------------
//  TopoShapePyOld destructor 
//--------------------------------------------------------------------------
TopoShapePyOld::~TopoShapePyOld()						// Everything handled in parent
{
	Console().Log("Destroy TopoShape %p\n",this);
} 

//--------------------------------------------------------------------------
// TopoShapePyOld Attributes
//--------------------------------------------------------------------------
PyObject *TopoShapePyOld::_getattr(const char *attr)				// __getattr__ function: note only need to handle new state
{ 
	try{
		// Access the number of attributes at this label
		if (Base::streq(attr, "AttributeCount"))
		{
			return Py_BuildValue("i", 1); 
		}else
			_getattr_up(PyObjectBase); 						// send to parent
	}catch(...){
		Console().Log("Exception in TopoShapePyOld::_getattr()\n");
		return 0;
	}
} 

int TopoShapePyOld::_setattr(const char *attr, PyObject *value) 	// __setattr__ function: note only need to handle new state
{ 
	if (Base::streq(attr, "Real"))						// settable new state
		; 
	else  
		return PyObjectBase::_setattr(attr, value);	// send up to parent
	return 0;
} 

//--------------------------------------------------------------------------
// PartFeaturePy representation
//--------------------------------------------------------------------------
PyObject *TopoShapePyOld::_repr(void)
{
  std::stringstream a;
  a << "TopoShape: [ ";
  a << "]" << std::endl;
	return Py_BuildValue("s", a.str().c_str());
}


//--------------------------------------------------------------------------
// Python wrappers
//--------------------------------------------------------------------------

PyObject *TopoShapePyOld::hasChild(PyObject *args)
{ 
  if (!PyArg_ParseTuple(args, "" ))   
    return NULL;                      
	Py_Return; 
}

PyObject *TopoShapePyOld::isNull(PyObject *args)
{ 
  if (!PyArg_ParseTuple(args, "" ))   
    return NULL;

  return Py_BuildValue("O", (_cTopoShape.IsNull() ? Py_True : Py_False));
}

PyObject *TopoShapePyOld::isValid(PyObject *args)
{ 
  if (!PyArg_ParseTuple(args, "" ))   
    return NULL;
  if ( !_cTopoShape.IsNull() )
  {
    BRepCheck_Analyzer aChecker(_cTopoShape);
    return Py_BuildValue("O", (aChecker.IsValid() ? Py_True : Py_False));
  }

  return Py_BuildValue("O", Py_False); 
}

PyObject *TopoShapePyOld::analyze(PyObject *args)
{ 
  if (!PyArg_ParseTuple(args, "" ))   
    return NULL;
  if ( !_cTopoShape.IsNull() )
  {
    BRepCheck_Analyzer aChecker(_cTopoShape);
    if (!aChecker.IsValid())
    {
      TopoDS_Iterator it(_cTopoShape);
      for (;it.More(); it.Next())
      {
        if (!aChecker.IsValid(it.Value()))
        {
          const Handle(BRepCheck_Result)& result = aChecker.Result(it.Value());
          const BRepCheck_ListOfStatus& status = result->StatusOnShape(it.Value());

          BRepCheck_ListIteratorOfListOfStatus it(status);
          while ( it.More() )
          {
            BRepCheck_Status& val = it.Value();
            switch (val)
            {
            case BRepCheck_NoError:
              PyErr_SetString(PyExc_StandardError, "No error");
              break;
            case BRepCheck_InvalidPointOnCurve:
              PyErr_SetString(PyExc_StandardError, "Invalid point on curve");
              break;
            case BRepCheck_InvalidPointOnCurveOnSurface:
              PyErr_SetString(PyExc_StandardError, "Invalid point on curve on surface");
              break;
            case BRepCheck_InvalidPointOnSurface:
              PyErr_SetString(PyExc_StandardError, "Invalid point on surface");
              break;
            case BRepCheck_No3DCurve:
              PyErr_SetString(PyExc_StandardError, "No 3D curve");
              break;
            case BRepCheck_Multiple3DCurve:
              PyErr_SetString(PyExc_StandardError, "Multiple 3D curve");
              break;
            case BRepCheck_Invalid3DCurve:
              PyErr_SetString(PyExc_StandardError, "Invalid 3D curve");
              break;
            case BRepCheck_NoCurveOnSurface:
              PyErr_SetString(PyExc_StandardError, "No curve on surface");
              break;
            case BRepCheck_InvalidCurveOnSurface:
              PyErr_SetString(PyExc_StandardError, "Invalid curve on surface");
              break;
            case BRepCheck_InvalidCurveOnClosedSurface:
              PyErr_SetString(PyExc_StandardError, "Invalid curve on closed surface");
              break;
            case BRepCheck_InvalidSameRangeFlag:
              PyErr_SetString(PyExc_StandardError, "Invalid same-range flag");
              break;
            case BRepCheck_InvalidSameParameterFlag:
              PyErr_SetString(PyExc_StandardError, "Invalid same-parameter flag");
              break;
            case BRepCheck_InvalidDegeneratedFlag:
              PyErr_SetString(PyExc_StandardError, "Invalid degenerated flag");
              break;
            case BRepCheck_FreeEdge:
              PyErr_SetString(PyExc_StandardError, "Free edge");
              break;
            case BRepCheck_InvalidMultiConnexity:
              PyErr_SetString(PyExc_StandardError, "Invalid multi-connexity");
              break;
            case BRepCheck_InvalidRange:
              PyErr_SetString(PyExc_StandardError, "Invalid range");
              break;
            case BRepCheck_EmptyWire:
              PyErr_SetString(PyExc_StandardError, "Empty wire");
              break;
            case BRepCheck_RedundantEdge:
              PyErr_SetString(PyExc_StandardError, "Redundant edge");
              break;
            case BRepCheck_SelfIntersectingWire:
              PyErr_SetString(PyExc_StandardError, "Self-intersecting wire");
              break;
            case BRepCheck_NoSurface:
              PyErr_SetString(PyExc_StandardError, "No surface");
              break;
            case BRepCheck_InvalidWire:
              PyErr_SetString(PyExc_StandardError, "Invalid wires");
              break;
            case BRepCheck_RedundantWire:
              PyErr_SetString(PyExc_StandardError, "Redundant wires");
              break;
            case BRepCheck_IntersectingWires:
              PyErr_SetString(PyExc_StandardError, "Intersecting wires");
              break;
            case BRepCheck_InvalidImbricationOfWires:
              PyErr_SetString(PyExc_StandardError, "Invalid imbrication of wires");
              break;
            case BRepCheck_EmptyShell:
              PyErr_SetString(PyExc_StandardError, "Empty shell");
              break;
            case BRepCheck_RedundantFace:
              PyErr_SetString(PyExc_StandardError, "Redundant face");
              break;
            case BRepCheck_UnorientableShape:
              PyErr_SetString(PyExc_StandardError, "Unorientable shape");
              break;
            case BRepCheck_NotClosed:
              PyErr_SetString(PyExc_StandardError, "Not closed");
              break;
            case BRepCheck_NotConnected:
              PyErr_SetString(PyExc_StandardError, "Not connected");
              break;
            case BRepCheck_SubshapeNotInShape:
              PyErr_SetString(PyExc_StandardError, "Sub-shape not in shape");
              break;
            case BRepCheck_BadOrientation:
              PyErr_SetString(PyExc_StandardError, "Bad orientation");
              break;
            case BRepCheck_BadOrientationOfSubshape:
              PyErr_SetString(PyExc_StandardError, "Bad orientation of sub-shape");
              break;
            case BRepCheck_InvalidToleranceValue:
              PyErr_SetString(PyExc_StandardError, "Invalid tolerance value");
              break;
            case BRepCheck_CheckFail:
              PyErr_SetString(PyExc_StandardError, "Check failed");
              break;
            }

            if (PyErr_Occurred())
              PyErr_Print();
            it.Next();
          }
        }
      }
    }
  }

  Py_Return; 
}

#if 0 // need a define for version of OCC
//#include <Message_ProgressIndicator.hxx>
//#include <Message_ProgressScale.hxx>
#include <MoniTool_ProgressIndicator.hxx>
#include <MoniTool_ProgressScale.hxx>
#include <Transfer_FinderProcess.hxx>
#include <TCollection_HAsciiString.hxx>
#include <Transfer_TransientProcess.hxx>
#include <IGESData_IGESEntity.hxx>
#include <XSControl_TransferReader.hxx>
#include <XSControl_WorkSession.hxx>
#include <Interface_InterfaceModel.hxx>

class ProgressIndicator : public MoniTool_ProgressIndicator
//class ProgressIndicator : public Message_ProgressIndicator
{
public:
  ProgressIndicator() {}
  virtual ~ProgressIndicator() 
  {
    Base::Sequencer().stop();
  }
  
  virtual  void Reset()
  {
  }
  virtual  Standard_Boolean UserBreak()
  {
    return Base::Sequencer().wasCanceled();
  }
  virtual  Standard_Boolean Show(const Standard_Boolean force = Standard_True)
  {
    Standard_Real min, max, step; Standard_Boolean isInf;
    GetScale(min, max, step, isInf);
    Standard_Real val = GetValue();
    Standard_Integer scopes = GetNbScopes();

    const MoniTool_ProgressScale& scale = GetScope(scopes);
    Handle(TCollection_HAsciiString) name = scale.GetName();

    if ( val < 2.0 )
      Base::Sequencer().start(name->ToCString(), (unsigned long)max);
    else
      Base::Sequencer().next();

    return false;
  }
};
#endif

PyObject *TopoShapePyOld::importIGES(PyObject *args)
{
  char* filename;
  if (!PyArg_ParseTuple(args, "s", &filename ))   
    return NULL;

  PY_TRY {
 
    // checking for the file
    Base::FileInfo File(filename);
    if(!File.isReadable()) {
      PyErr_SetString(PartExceptionOCCError,"File to read does not exist or is not readable");
      return NULL;
    }

    // read iges file
    IGESControl_Reader aReader;

    if (aReader.ReadFile((const Standard_CString)filename) != IFSelect_RetDone) {
      PyErr_SetString(PartExceptionOCCError,"Reading IGES failed");
      return NULL;
    }

#if 0
    // get all root shapes 
    Handle(TColStd_HSequenceOfTransient) aList=aReader.GiveList("xst-transferrable-roots");
    for (Standard_Integer j=1; j<=aList->Length(); j++) {
      Handle(IGESData_IGESEntity) igesEntity=Handle(IGESData_IGESEntity)::DownCast(aList->Value(j));
      // get names 
      Handle(TCollection_HAsciiString) name = igesEntity->NameValue();
      if ( !name.IsNull() ) {
        const char* cname = name->ToCString();
      }
      if (igesEntity->HasShortLabel()) {
        name = igesEntity->ShortLabel();
        if ( !name.IsNull() ) {
          const char* cname = name->ToCString();
        }
      }
      const char* type = igesEntity->DynamicType()->Name();
    }
 
    //Standard_Integer val = Interface_Static::IVal("read.iges.bspline.continuity");
    //Interface_Static::SetIVal("read.iges.bspline.continuity", 2);
    //Standard_Integer ic = Interface_Static::IVal("read.precision.mode");
    //Standard_Real rp = Interface_Static::RVal("read.precision.val");
    
    //Handle(TColStd_HSequenceOfTransient) aList; 
    //aList = aReader.GiveList ("iges-type(114)");
    //int ct = aList->Length();
    //Reader.TransferList(aList);
    //ct = aReader.NbShapes();
#endif

    // one shape that contains all subshapes
    aReader.TransferRoots();
    //ct = aReader.NbShapes();
    
    _cTopoShape = aReader.OneShape();
  } PY_CATCH;

  Py_Return; 
}

PyObject *TopoShapePyOld::exportIGES(PyObject *args)
{
  char* filename;
  if (!PyArg_ParseTuple(args, "s", &filename ))   
    return NULL;

  PY_TRY {

#if 0
    // An OCC example
    IGESControl_Controller::Init();
	  IGESControl_Writer writer( Interface_Static::CVal( "XSTEP.iges.unit" ),
                               Interface_Static::IVal( "XSTEP.iges.writebrep.mode" ) );
		writer.AddShape ( _cTopoShape );
	  writer.ComputeModel();
	  writer.Write( (const Standard_CString)filename );
#endif

    // write iges file
    IGESControl_Controller::Init();
    IGESControl_Writer aWriter;

#if 0
    Handle(Transfer_FinderProcess) proc = aWriter.TransferProcess();
    Handle(MoniTool_ProgressIndicator) prog = new ProgressIndicator();
    proc->SetProgress(prog);

    Standard_CString byvalue = Interface_Static::CVal("write.iges.header.author");
    Interface_Static::SetCVal ("write.iges.header.author", "FreeCAD");
    //Interface_Static::SetCVal ("write.iges.header.company", "FreeCAD");
#endif

    aWriter.AddShape(_cTopoShape);
#if 0
    aWriter.ComputeModel();
#endif

    if (aWriter.Write((const Standard_CString)filename) != IFSelect_RetDone) {
      PyErr_SetString(PartExceptionOCCError,"Writing IGES failed");
      return NULL;
    }

  } PY_CATCH;

  Py_Return; 
}

PyObject *TopoShapePyOld::importSTEP(PyObject *args)
{
  char* filename;
  if (!PyArg_ParseTuple(args, "s", &filename ))   
    return NULL;

  PY_TRY {
    // checking for the file
    Base::FileInfo File(filename);
    if(!File.isReadable()) {
      PyErr_SetString(PartExceptionOCCError,"File to read does not exist or is not readable");
      return NULL;
    }

    // read step file
    STEPControl_Reader aReader;
    if (aReader.ReadFile((const Standard_CString)filename) != IFSelect_RetDone) {
      PyErr_SetString(PartExceptionOCCError,"Reading STEP failed");
      return NULL;
    }

#if 0 // Some interesting stuff
    Handle(TColStd_HSequenceOfTransient) aList=aReader.GiveList("xst-transferrable-roots");
    for (Standard_Integer j=1; j<=aList->Length(); j++) {
      Handle(IGESData_IGESEntity) igesEntity=Handle(IGESData_IGESEntity)::DownCast(aList->Value(j));
      // get names 
      Handle(TCollection_HAsciiString) name = igesEntity->NameValue();
      if ( !name.IsNull() ) {
        const char* cname = name->ToCString();
      }
      if (igesEntity->HasShortLabel()) {
        name = igesEntity->ShortLabel();
        if ( !name.IsNull() ) {
          const char* cname = name->ToCString();
        }
      }
      const char* type = igesEntity->DynamicType()->Name();
    }
#endif

    aReader.TransferRoots();
    _cTopoShape = aReader.OneShape();

#if 0 // Some interesting stuff
	  Handle(XSControl_WorkSession) ws = aReader.WS();
	  //SetModel( reader.StepModel() );
	  Handle(XSControl_TransferReader) tr = ws->TransferReader();
	  Handle(Standard_Transient) ent = tr->EntityFromShapeResult(_cTopoShape, 3);
	  if ( ! ent.IsNull() ) {
		  //printf( "Name of STEP-Model: %s\n", ws->Model()->StringLabel(ent)->String() );
	  }
	  TopTools_IndexedMapOfShape smap;
	  TopExp::MapShapes( _cTopoShape, smap);
	  for ( Standard_Integer k = 1; k <= smap.Extent(); k++ ) {
		  const TopoDS_Shape& tsh = smap(k);
		  Handle(Standard_Transient) ent = tr->EntityFromShapeResult(tsh, 3);
		  if ( ! ent.IsNull() ) {
			  //printf( "Part %s ", ws->Model()->StringLabel(ent)->String() );
			  //printf( "is a %s\n", ws->Model()->TypeName(ent) );
        //MoniTool_DataMapOfShapeTransient map;
			  //map.Bind(tsh, ws->Model()->StringLabel(ent)->ShallowCopy() );
		  }
	  }
#endif

  } PY_CATCH;

  Py_Return; 
}

PyObject *TopoShapePyOld::exportSTEP(PyObject *args)
{
  char* filename;
  if (!PyArg_ParseTuple(args, "s", &filename))
    return NULL;

  PY_TRY {

    // write step file
    STEPControl_Writer aWriter;

    //FIXME: Does not work this way!!!
    if (aWriter.Transfer(_cTopoShape, STEPControl_AsIs)) {
      PyErr_SetString(PartExceptionOCCError,"Transferring STEP failed");
      return NULL;
    }

    if (aWriter.Write((const Standard_CString)filename) != IFSelect_RetDone) {
      PyErr_SetString(PartExceptionOCCError,"Writing STEP failed");
      return NULL;
    }
        
  } PY_CATCH;

  Py_Return; 
}

PyObject *TopoShapePyOld::importBREP(PyObject *args)
{
  char* filename;
  if (!PyArg_ParseTuple(args, "s", &filename ))   
    return NULL;

  PY_TRY {
    // checking for the file
    Base::FileInfo File(filename);
    if(!File.isReadable()) {
      PyErr_SetString(PartExceptionOCCError,"File to read does not exist or is not readable");
      return NULL;
    }
    
    // read brep file
    BRep_Builder aBuilder;
    if (!BRepTools::Read(_cTopoShape,(const Standard_CString)filename,aBuilder)) {
      PyErr_SetString(PartExceptionOCCError,"Reading BREP failed");
      return NULL;
    }
  } PY_CATCH;

  Py_Return; 
}

PyObject *TopoShapePyOld::exportBREP(PyObject *args)
{
  char* filename;
  if (!PyArg_ParseTuple(args, "s", &filename ))
    return NULL;

  PY_TRY {
    // read brep file
    if (!BRepTools::Write(_cTopoShape,(const Standard_CString)filename)) {
      PyErr_SetString(PartExceptionOCCError,"Writing BREP failed");
      return NULL;
    }
  } PY_CATCH;

  Py_Return; 
}

PyObject *TopoShapePyOld::exportSTL(PyObject *args)
{
  char* filename;
  if (!PyArg_ParseTuple(args, "s", &filename ))
    return NULL;

  PY_TRY {
    // write STL file
  	StlAPI_Writer writer;
	  writer.Write( _cTopoShape, (const Standard_CString)filename );
  } PY_CATCH;

  Py_Return; 
}

#endif
