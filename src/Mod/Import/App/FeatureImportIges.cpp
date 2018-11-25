/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
#ifndef _PreComp_
# include <fcntl.h>
# include <TopTools_HSequenceOfShape.hxx>
# include <IGESControl_Writer.hxx>
# include <IGESControl_Reader.hxx>
# include <TopoDS_Shape.hxx>
# include <TFunction_Logbook.hxx>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>
#include "FeatureImportIges.h"


using namespace Import;

void FeatureImportIges::InitLabel(const TDF_Label &rcLabel)
{
	addProperty("String","FileName");

}

/*
bool FeaturePartImportStep::MustExecute(void)
{
	Base::Console().Log("PartBoxFeature::MustExecute()\n");
	return false;
}
*/
Standard_Integer FeatureImportIges::Execute(void)
{
	Base::Console().Log("FeaturePartImportIges::Execute()\n");

/*  cout << GetFloatProperty("x") << endl;
  cout << GetFloatProperty("y") << endl;
  cout << GetFloatProperty("z") << endl;
  cout << GetFloatProperty("l") << endl;
  cout << GetFloatProperty("h") << endl;
  cout << GetFloatProperty("w") << endl;*/

  try{

    IGESControl_Reader aReader;
    TopoDS_Shape aShape;

    std::string FileName = getPropertyString("FileName");

    int i=_open(FileName.c_str(),O_RDONLY);
	  if( i != -1)
	  {
		  _close(i);
	  }else{
      Base::Console().Log("FeaturePartImportIges::Execute() not able to open %s!\n",FileName.c_str());
		  return 1;
	  }

    // just do show the wait cursor when the Gui is up
    Base::Sequencer().start("Load IGES", 1);
    Base::Sequencer().next();

    // read iges-file
    if (aReader.ReadFile((const Standard_CString)FileName.c_str()) != IFSelect_RetDone)
      throw Base::FileException("IGES read failed (load file)");
  
    // check iges-file (memory)
    //if (!aReader.Check(Standard_True))
    //  Base::Console().Warning( "IGES model contains errors! try loading anyway....\n" );
  
    // make brep
    aReader.TransferRoots();
    // one shape, who contain's all subshapes
    aShape = aReader.OneShape();

	  setShape(aShape);
    Base::Sequencer().stop();
  }
  catch(...){
    Base::Sequencer().halt();
    Base::Console().Error("FeaturePartImportIges::Execute() failed!");
    return 1;
  }

  return 0;
}

/*
void FeatureImportIges::Validate(void)
{
	Base::Console().Log("FeaturePartImportStep::Validate()\n");
 
  // We validate the object label ( Label() ), all the arguments and the results of the object:
  log.SetValid(Label(), Standard_True);


}
*/



