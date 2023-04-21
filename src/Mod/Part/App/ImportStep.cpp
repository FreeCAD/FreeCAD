/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <sstream>
# include <Quantity_Color.hxx>
# include <BRep_Builder.hxx>
# include <STEPControl_Reader.hxx>
# include <StepData_StepModel.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Shape.hxx>
# include <TopoDS_Shell.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Compound.hxx>
# include <TopExp_Explorer.hxx>
# include <Standard_Version.hxx>
# include <Transfer_TransientProcess.hxx>
# include <XSControl_TransferReader.hxx>
# include <XSControl_WorkSession.hxx>
#endif

#include <StepElement_AnalysisItemWithinRepresentation.hxx>
#include <StepVisual_AnnotationCurveOccurrence.hxx>

#include <App/Document.h>
#include <Base/Console.h>

#include "ImportStep.h"
#include "encodeFilename.h"
#include "PartFeature.h"
#include "ProgressIndicator.h"


namespace Part {
bool ReadColors (const Handle(XSControl_WorkSession) &WS, std::map<int, Quantity_Color>& hash_col);
bool ReadNames (const Handle(XSControl_WorkSession) &WS);
}

int Part::ImportStepParts(App::Document *pcDoc, const char* Name)
{
    // Use this to force to link against TKSTEPBase, TKSTEPAttr and TKStep209
    // in order to make RUNPATH working on Linux
    StepElement_AnalysisItemWithinRepresentation stepElement;
    StepVisual_AnnotationCurveOccurrence stepVis;

    STEPControl_Reader aReader;
    TopoDS_Shape aShape;
    Base::FileInfo fi(Name);

    if (!fi.exists()) {
        std::stringstream str;
        str << "File '" << Name << "' does not exist!";
        throw Base::FileException(str.str().c_str());
    }
    std::string encodednamestr = encodeFilename(std::string(Name));
    const char * encodedname = encodednamestr.c_str();

    if (aReader.ReadFile((Standard_CString)encodedname) !=
            IFSelect_RetDone) {
        throw Base::FileException("Cannot open STEP file");
    }

#if OCC_VERSION_HEX < 0x070500
    Handle(Message_ProgressIndicator) pi = new ProgressIndicator(100);
    aReader.WS()->MapReader()->SetProgress(pi);
    pi->NewScope(100, "Reading STEP file...");
    pi->Show();
#endif

    // Root transfers
    Standard_Integer nbr = aReader.NbRootsForTransfer();
    //aReader.PrintCheckTransfer (failsonly, IFSelect_ItemsByEntity);
    for (Standard_Integer n = 1; n<= nbr; n++) {
        Base::Console().Log("STEP: Transferring Root %d\n",n);
        aReader.TransferRoot(n);
    }
#if OCC_VERSION_HEX < 0x070500
    pi->EndScope();
#endif

    // Collecting resulting entities
    Standard_Integer nbs = aReader.NbShapes();
    if (nbs == 0) {
        throw Base::FileException("No shapes found in file ");
    }
    else {
        //Handle(StepData_StepModel) Model = aReader.StepModel();
        //Handle(XSControl_WorkSession) ws = aReader.WS();
        //Handle(XSControl_TransferReader) tr = ws->TransferReader();

        std::map<int, Quantity_Color> hash_col;
        //ReadColors(aReader.WS(), hash_col);
        //ReadNames(aReader.WS());

        for (Standard_Integer i=1; i<=nbs; i++) {
            Base::Console().Log("STEP:   Transferring Shape %d\n",i);
            aShape = aReader.Shape(i);

            // load each solid as an own object
            TopExp_Explorer ex;
            for (ex.Init(aShape, TopAbs_SOLID); ex.More(); ex.Next())
            {
                // get the shape
                const TopoDS_Solid& aSolid = TopoDS::Solid(ex.Current());

                std::string name = fi.fileNamePure();
                //Handle(Standard_Transient) ent = tr->EntityFromShapeResult(aSolid, 3);
                //if (!ent.IsNull()) {
                //    name += ws->Model()->StringLabel(ent)->ToCString();
                //}

                Part::Feature *pcFeature;
                pcFeature = static_cast<Part::Feature*>(pcDoc->addObject("Part::Feature", name.c_str()));
                pcFeature->Shape.setValue(aSolid);

                // This is a trick to access the GUI via Python and set the color property
                // of the associated view provider. If no GUI is up an exception is thrown
                // and cleared immediately
                std::map<int, Quantity_Color>::iterator it = hash_col.find(aSolid.HashCode(INT_MAX));
                if (it != hash_col.end()) {
                    try {
                        Py::Object obj(pcFeature->getPyObject(), true);
                        Py::Object vp(obj.getAttr("ViewObject"));
                        Py::Tuple col(3);
                        col.setItem(0, Py::Float(it->second.Red()));
                        col.setItem(1, Py::Float(it->second.Green()));
                        col.setItem(2, Py::Float(it->second.Blue()));
                        vp.setAttr("ShapeColor", col);
                        //Base::Console().Message("Set color to shape\n");
                    }
                    catch (Py::Exception& e) {
                        e.clear();
                    }
                }
            }
            // load all non-solids now
            for (ex.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); ex.More(); ex.Next())
            {
                // get the shape
                const TopoDS_Shell& aShell = TopoDS::Shell(ex.Current());

                std::string name = fi.fileNamePure();
                //Handle(Standard_Transient) ent = tr->EntityFromShapeResult(aShell, 3);
                //if (!ent.IsNull()) {
                //    name += ws->Model()->StringLabel(ent)->ToCString();
                //}

                Part::Feature *pcFeature = static_cast<Part::Feature*>(pcDoc->addObject("Part::Feature", name.c_str()));
                pcFeature->Shape.setValue(aShell);
            }

            // put all other free-flying shapes into a single compound
            Standard_Boolean emptyComp = Standard_True;
            BRep_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);

            for (ex.Init(aShape, TopAbs_FACE, TopAbs_SHELL); ex.More(); ex.Next()) {
                if (!ex.Current().IsNull()) {
                    builder.Add(comp, ex.Current());
                    emptyComp = Standard_False;
                }
            }
            for (ex.Init(aShape, TopAbs_WIRE, TopAbs_FACE); ex.More(); ex.Next()) {
                if (!ex.Current().IsNull()) {
                    builder.Add(comp, ex.Current());
                    emptyComp = Standard_False;
                }
            }
            for (ex.Init(aShape, TopAbs_EDGE, TopAbs_WIRE); ex.More(); ex.Next()) {
                if (!ex.Current().IsNull()) {
                    builder.Add(comp, ex.Current());
                    emptyComp = Standard_False;
                }
            }
            for (ex.Init(aShape, TopAbs_VERTEX, TopAbs_EDGE); ex.More(); ex.Next()) {
                if (!ex.Current().IsNull()) {
                    builder.Add(comp, ex.Current());
                    emptyComp = Standard_False;
                }
            }

            if (!emptyComp) {
                std::string name = fi.fileNamePure();
                Part::Feature *pcFeature = static_cast<Part::Feature*>(pcDoc->addObject
                    ("Part::Feature", name.c_str()));
                pcFeature->Shape.setValue(comp);
            }
        }
    }

    return 0;
}


bool Part::ReadColors (const Handle(XSControl_WorkSession) &WS, std::map<int, Quantity_Color>& hash_col)
{
    (void)WS;
    (void)hash_col;
    return Standard_False;
}

bool Part::ReadNames (const Handle(XSControl_WorkSession) &WS)
{
    (void)WS;
    return Standard_False;
}
