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
# include <BRep_Builder.hxx>
# include <Interface_Static.hxx>
# include <Quantity_Color.hxx>
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

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>

#include "ImportStep.h"
#include "encodeFilename.h"
#include "PartFeature.h"
#include "ProgressIndicator.h"


using namespace Part;


void ImportExportSettings::initialize()
{
    // set the user-defined settings
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part");
    initGeneral(hGrp);
    initSTEP(hGrp);
    initIGES(hGrp);
}

void ImportExportSettings::initGeneral(Base::Reference<ParameterGrp> hGrp)
{
    // General
    Base::Reference<ParameterGrp> hGenGrp = hGrp->GetGroup("General");
    // http://www.opencascade.org/org/forum/thread_20801/
    // read.surfacecurve.mode:
    // A preference for the computation of curves in an entity which has both 2D and 3D representation.
    // Each TopoDS_Edge in TopoDS_Face must have a 3D and 2D curve that references the surface.
    // If both 2D and 3D representation of the entity are present, the computation of these curves depends on
    // the following values of parameter:
    // 0: "Default" - no preference, both curves are taken
    // 3: "3DUse_Preferred" - 3D curves are used to rebuild 2D ones
    // Additional modes for IGES
    //  2: "2DUse_Preferred" - the 2D is used to rebuild the 3D in case of their inconsistency
    // -2: "2DUse_Forced" - the 2D is always used to rebuild the 3D (even if 2D is present in the file)
    // -3: "3DUse_Forced" - the 3D is always used to rebuild the 2D (even if 2D is present in the file)
    int readsurfacecurve = hGenGrp->GetInt("ReadSurfaceCurveMode", 0);
    Interface_Static::SetIVal("read.surfacecurve.mode", readsurfacecurve);

    // write.surfacecurve.mode (STEP-only):
    // This parameter indicates whether parametric curves (curves in parametric space of surface) should be
    // written into the STEP file. This parameter can be set to Off in order to minimize the size of the resulting
    // STEP file.
    // Off (0) : writes STEP files without pcurves. This mode decreases the size of the resulting file.
    // On (1) : (default) writes pcurves to STEP file
    int writesurfacecurve = hGenGrp->GetInt("WriteSurfaceCurveMode", 0);
    Interface_Static::SetIVal("write.surfacecurve.mode", writesurfacecurve);
}

void ImportExportSettings::initIGES(Base::Reference<ParameterGrp> hGrp)
{
    //IGES handling
    Base::Reference<ParameterGrp> hIgesGrp = hGrp->GetGroup("IGES");
    int value = Interface_Static::IVal("write.iges.brep.mode");
    bool brep = hIgesGrp->GetBool("BrepMode", value > 0);
    Interface_Static::SetIVal("write.iges.brep.mode",brep ? 1 : 0);
    Interface_Static::SetCVal("write.iges.header.company", hIgesGrp->GetASCII("Company").c_str());
    Interface_Static::SetCVal("write.iges.header.author", hIgesGrp->GetASCII("Author").c_str());
    Interface_Static::SetCVal("write.iges.header.product", hIgesGrp->GetASCII("Product",
       Interface_Static::CVal("write.iges.header.product")).c_str());

    int unitIges = hIgesGrp->GetInt("Unit", 0);
    switch (unitIges) {
        case 1:
            Interface_Static::SetCVal("write.iges.unit","M");
            break;
        case 2:
            Interface_Static::SetCVal("write.iges.unit","INCH");
            break;
        default:
            Interface_Static::SetCVal("write.iges.unit","MM");
            break;
    }
}

void ImportExportSettings::initSTEP(Base::Reference<ParameterGrp> hGrp)
{
    //STEP handling
    Base::Reference<ParameterGrp> hStepGrp = hGrp->GetGroup("STEP");
    int unitStep = hStepGrp->GetInt("Unit", 0);
    switch (unitStep) {
        case 1:
            Interface_Static::SetCVal("write.step.unit","M");
            break;
        case 2:
            Interface_Static::SetCVal("write.step.unit","INCH");
            break;
        default:
            Interface_Static::SetCVal("write.step.unit","MM");
            break;
    }

    std::string ap = hStepGrp->GetASCII("Scheme", Interface_Static::CVal("write.step.schema"));
    Interface_Static::SetCVal("write.step.schema", ap.c_str());
    Interface_Static::SetCVal("write.step.product.name", hStepGrp->GetASCII("Product",
       Interface_Static::CVal("write.step.product.name")).c_str());
}

ImportExportSettings::ImportExportSettings()
{
    pGroup = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Import");
}

void ImportExportSettings::setReadShapeCompoundMode(bool on)
{
    auto grp = pGroup->GetGroup("hSTEP");
    grp->SetBool("ReadShapeCompoundMode", on);
}

bool ImportExportSettings::getReadShapeCompoundMode() const
{
    auto grp = pGroup->GetGroup("hSTEP");
    return grp->GetBool("ReadShapeCompoundMode", true);
}

void ImportExportSettings::setExportHiddenObject(bool on)
{
    pGroup->SetBool("ExportHiddenObject", on);
}

bool ImportExportSettings::getExportHiddenObject() const
{
    return pGroup->GetBool("ExportHiddenObject", true);
}

void ImportExportSettings::setImportHiddenObject(bool on)
{
    pGroup->SetBool("ImportHiddenObject", on);
}

bool ImportExportSettings::getImportHiddenObject() const
{
    return pGroup->GetBool("ImportHiddenObject", false);
}

void ImportExportSettings::setExportLegacy(bool on)
{
    pGroup->SetBool("ExportLegacy", on);
}

bool ImportExportSettings::getExportLegacy() const
{
    return pGroup->GetBool("ExportLegacy", false);
}

void ImportExportSettings::setExportKeepPlacement(bool on)
{
    pGroup->SetBool("ExportKeepPlacement", on);
}

bool ImportExportSettings::getExportKeepPlacement() const
{
    return pGroup->GetBool("ExportKeepPlacement", false);
}

void ImportExportSettings::setUseLinkGroup(bool on)
{
    pGroup->SetBool("UseLinkGroup", on);
}

bool ImportExportSettings::getUseLinkGroup() const
{
    return pGroup->GetBool("UseLinkGroup", false);
}

void ImportExportSettings::setUseBaseName(bool on)
{
    pGroup->SetBool("UseBaseName", on);
}

bool ImportExportSettings::getUseBaseName() const
{
    return pGroup->GetBool("UseBaseName", false);
}

void ImportExportSettings::setReduceObjects(bool on)
{
    pGroup->SetBool("ReduceObjects", on);
}

bool ImportExportSettings::getReduceObjects() const
{
    return pGroup->GetBool("ReduceObjects", false);
}

void ImportExportSettings::setExpandCompound(bool on)
{
    pGroup->SetBool("ExpandCompound", on);
}

bool ImportExportSettings::getExpandCompound() const
{
    return pGroup->GetBool("ExpandCompound", false);
}

void ImportExportSettings::setShowProgress(bool on)
{
    pGroup->SetBool("ShowProgress", on);
}

bool ImportExportSettings::getShowProgress() const
{
    return pGroup->GetBool("ShowProgress", false);
}

void ImportExportSettings::setImportMode(ImportExportSettings::ImportMode mode)
{
    pGroup->SetInt("ImportMode", static_cast<long>(mode));
}

ImportExportSettings::ImportMode ImportExportSettings::getImportMode() const
{
    return static_cast<ImportExportSettings::ImportMode>(pGroup->GetInt("ImportMode", 0));
}


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
