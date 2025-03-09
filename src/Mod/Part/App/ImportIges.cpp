/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRep_Builder.hxx>
# include <IGESBasic_Group.hxx>
# include <IGESBasic_SingularSubfigure.hxx>
# include <IGESControl_Controller.hxx>
# include <IGESControl_Reader.hxx>
# include <IGESSolid_ManifoldSolid.hxx>
# include <Message_MsgFile.hxx>
# include <Standard_Version.hxx>
# include <TColStd_HSequenceOfTransient.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Shape.hxx>
# include <Transfer_TransientProcess.hxx>
# include <XSControl_TransferReader.hxx>
# include <XSControl_WorkSession.hxx>
#endif

#include <App/Document.h>
#include <Base/Sequencer.h>

#include "ImportIges.h"
#include "PartFeature.h"


using namespace Part;

int Part::ImportIgesParts(App::Document *pcDoc, const char* FileName)
{
    try {
        Base::FileInfo fi(FileName);
        // read iges file
        IGESControl_Controller::Init();

        // load data exchange message files
        Message_MsgFile::LoadFromEnv("CSF_XSMessage","IGES");

        // load shape healing message files
        Message_MsgFile::LoadFromEnv("CSF_SHMessageStd","SHAPEStd");

        IGESControl_Reader aReader;
        if (aReader.ReadFile((Standard_CString)FileName) != IFSelect_RetDone)
            throw Base::FileException("Error in reading IGES");

        // Ignore construction elements
        // http://www.opencascade.org/org/forum/thread_20603/?forum=3
        aReader.SetReadVisible(Standard_True);

        // check file conformity and output stats
        aReader.PrintCheckLoad(Standard_True,IFSelect_GeneralInfo);

        std::string aName = fi.fileNamePure();

        // make model
        aReader.ClearShapes();
        //Standard_Integer nbRootsForTransfer = aReader.NbRootsForTransfer();
        aReader.TransferRoots();

        // put all other free-flying shapes into a single compound
        Standard_Boolean emptyComp = Standard_True;
        BRep_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);

        Standard_Integer nbShapes = aReader.NbShapes();
        for (Standard_Integer i=1; i<=nbShapes; i++) {
            TopoDS_Shape aShape = aReader.Shape(i);
            if (!aShape.IsNull()) {
                if (aShape.ShapeType() == TopAbs_SOLID ||
                    aShape.ShapeType() == TopAbs_COMPOUND ||
                    aShape.ShapeType() == TopAbs_SHELL) {
                        auto* obj = pcDoc->addObject<Part::Feature>(aName.c_str());
                        obj->Shape.setValue(aShape);
                }
                else {
                    builder.Add(comp, aShape);
                    emptyComp = Standard_False;
                }
            }
        }
        if (!emptyComp) {
            std::string name = fi.fileNamePure();
            auto* pcFeature = pcDoc->addObject<Part::Feature>(name.c_str());
            pcFeature->Shape.setValue(comp);
        }
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }

    return 0;
}
