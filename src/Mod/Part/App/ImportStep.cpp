/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2008     *
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
# include <TopTools_HSequenceOfShape.hxx>
# include <STEPControl_Writer.hxx>
# include <STEPControl_Reader.hxx>
# include <StepData_StepModel.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Shape.hxx>
# include <TopoDS_Shell.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Compound.hxx>
# include <TopExp_Explorer.hxx>
# include <sstream>
#endif

#include <Standard_Version.hxx>
#include <XSControl_WorkSession.hxx>
#include <XSControl_TransferReader.hxx>
#include <XSControl_WorkSession.hxx>
#include <XSControl_TransferReader.hxx>
#include <Transfer_TransientProcess.hxx>

#include <STEPConstruct_Styles.hxx>
#include <TColStd_HSequenceOfTransient.hxx>
#include <STEPConstruct.hxx>
#include <StepVisual_StyledItem.hxx>
#include <StepShape_ShapeRepresentation.hxx>
#include <StepVisual_PresentationStyleByContext.hxx>
#include <StepVisual_StyleContextSelect.hxx>
#include <StepVisual_PresentationStyleByContext.hxx>
#include <Interface_EntityIterator.hxx>
#include <StepRepr_RepresentedDefinition.hxx>
#include <StepShape_ShapeDefinitionRepresentation.hxx>
#include <StepRepr_CharacterizedDefinition.hxx>
#include <StepRepr_ProductDefinitionShape.hxx>
#include <StepRepr_AssemblyComponentUsage.hxx>
#include <StepRepr_AssemblyComponentUsage.hxx>
#include <StepRepr_SpecifiedHigherUsageOccurrence.hxx>
#include <Quantity_Color.hxx>
#include <TCollection_ExtendedString.hxx>
#include <StepBasic_Product.hxx>
#include <StepBasic_Product.hxx>
#include <StepBasic_ProductDefinition.hxx>
#include <StepBasic_ProductDefinition.hxx>
#include <StepBasic_ProductDefinitionFormation.hxx>

#include <Base/Console.h>
#include <Base/Sequencer.h>
#include <App/Application.h>
#include <App/Document.h>

#include "ImportStep.h"
#include "PartFeature.h"
#include "ProgressIndicator.h"
#include "encodeFilename.h"

using namespace Part;

namespace Part {
bool ReadColors (const Handle(XSControl_WorkSession) &WS, std::map<int, Quantity_Color>& hash_col);
bool ReadNames (const Handle(XSControl_WorkSession) &WS);
}

int Part::ImportStepParts(App::Document *pcDoc, const char* Name)
{
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

    Handle(Message_ProgressIndicator) pi = new ProgressIndicator(100);
    aReader.WS()->MapReader()->SetProgress(pi);
    pi->NewScope(100, "Reading STEP file...");
    pi->Show();

    // Root transfers
    Standard_Integer nbr = aReader.NbRootsForTransfer();
    //aReader.PrintCheckTransfer (failsonly, IFSelect_ItemsByEntity);
    for (Standard_Integer n = 1; n<= nbr; n++) {
        Base::Console().Log("STEP: Transferring Root %d\n",n);
        aReader.TransferRoot(n);
    }
    pi->EndScope();

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

#if OCC_VERSION_HEX < 0x070000
static void findStyledSR (const Handle(StepVisual_StyledItem) &style,
                          Handle(StepShape_ShapeRepresentation)& aSR)
{
    // search Shape Represenatation for component styled item
    for (Standard_Integer j=1; j <= style->NbStyles(); j++) {
        Handle(StepVisual_PresentationStyleByContext) PSA = 
            Handle(StepVisual_PresentationStyleByContext)::DownCast(style->StylesValue ( j ));
        if (PSA.IsNull())
            continue;
        StepVisual_StyleContextSelect aStyleCntxSlct = PSA->StyleContext();
        Handle(StepShape_ShapeRepresentation) aCurrentSR = 
            Handle(StepShape_ShapeRepresentation)::DownCast(aStyleCntxSlct.Representation());
        if (aCurrentSR.IsNull())
            continue;
        aSR = aCurrentSR;
            break;
    }
}
#endif

bool Part::ReadColors (const Handle(XSControl_WorkSession) &WS, std::map<int, Quantity_Color>& hash_col)
{
#if OCC_VERSION_HEX >= 0x070000
    (void)WS;
    (void)hash_col;
    return Standard_False;
#else
    STEPConstruct_Styles Styles (WS);
    if (!Styles.LoadStyles()) {
#ifdef FC_DEBUG
        std::cout << "Warning: no styles are found in the model" << std::endl;
#endif
        return Standard_False;
    }
    // searching for invisible items in the model
    Handle(TColStd_HSequenceOfTransient) aHSeqOfInvisStyle = new TColStd_HSequenceOfTransient;
    Styles.LoadInvisStyles( aHSeqOfInvisStyle );

    // parse and search for color attributes
    Standard_Integer nb = Styles.NbStyles();
    for (Standard_Integer i=1; i <= nb; i++) {
        Handle(StepVisual_StyledItem) style = Styles.Style (i);
        if (style.IsNull()) continue;

        Standard_Boolean IsVisible = Standard_True;
        // check the visibility of styled item.
        for (Standard_Integer si = 1; si <= aHSeqOfInvisStyle->Length(); si++) {
            if (style != aHSeqOfInvisStyle->Value(si))
                continue;
            // found that current style is invisible.
#ifdef FC_DEBUG
            std::cout << "Warning: item No " << i << "(" << style->Item()->DynamicType()->Name() << ") is invisible" << std::endl;
#endif
            IsVisible = Standard_False;
            break;
        }

        Handle(StepVisual_Colour) SurfCol, BoundCol, CurveCol;
        // check if it is component style
        Standard_Boolean IsComponent = Standard_False;
        if (!Styles.GetColors (style, SurfCol, BoundCol, CurveCol, IsComponent) && IsVisible)
            continue;

        // find shape
        TopoDS_Shape S = STEPConstruct::FindShape(Styles.TransientProcess(), style->Item());
        //TopAbs_ShapeEnum type = S.ShapeType();
        Standard_Boolean isSkipSHUOstyle = Standard_False;
        // take shape with real location.
        while (IsComponent) {
            // take SR of NAUO
            Handle(StepShape_ShapeRepresentation) aSR;
            findStyledSR(style, aSR);
            // search for SR along model
            if (aSR.IsNull())
                break;
//          Handle(Interface_InterfaceModel) Model = WS->Model();
            Handle(XSControl_TransferReader) TR = WS->TransferReader();
            Handle(Transfer_TransientProcess) TP = TR->TransientProcess();
            Interface_EntityIterator subs = WS->HGraph()->Graph().Sharings( aSR );
            Handle(StepShape_ShapeDefinitionRepresentation) aSDR;
            for (subs.Start(); subs.More(); subs.Next()) {
                aSDR = Handle(StepShape_ShapeDefinitionRepresentation)::DownCast(subs.Value());
                if (aSDR.IsNull())
                    continue;
                StepRepr_RepresentedDefinition aPDSselect = aSDR->Definition();
                Handle(StepRepr_ProductDefinitionShape) PDS =
                    Handle(StepRepr_ProductDefinitionShape)::DownCast(aPDSselect.PropertyDefinition());
                if (PDS.IsNull())
                    continue;
                StepRepr_CharacterizedDefinition aCharDef = PDS->Definition();
        
                Handle(StepRepr_AssemblyComponentUsage) ACU = 
                    Handle(StepRepr_AssemblyComponentUsage)::DownCast(aCharDef.ProductDefinitionRelationship());
                // PTV 10.02.2003 skip styled item that refer to SHUO
                if (ACU->IsKind(STANDARD_TYPE(StepRepr_SpecifiedHigherUsageOccurrence))) {
                    isSkipSHUOstyle = Standard_True;
                    break;
                }
                Handle(StepRepr_NextAssemblyUsageOccurrence) NAUO =
                    Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(ACU);
                if (NAUO.IsNull())
                    continue;
        
                TopoDS_Shape aSh;
                // PTV 10.02.2003 to find component of assembly CORRECTLY
                STEPConstruct_Tool Tool( WS );
//              Handle(Transfer_Binder) binder = TP->Find(NAUO);
//              if (binder.IsNull() || ! binder->HasResult())
//                  continue;
//              aSh = TransferBRep::ShapeResult ( TP, binder );
                if (!aSh.IsNull()) {
                    S = aSh;
                    break;
                }
            }
            break;
        }
        if (isSkipSHUOstyle)
            continue; // skip styled item which refer to SHUO
    
        if ( S.IsNull() ) {
#ifdef FC_DEBUG
            std::cout << "Warning: item No " << i << "(" << style->Item()->DynamicType()->Name() << ") is not mapped to shape" << std::endl;
#endif
            continue;
        }
    
        if (!SurfCol.IsNull()) {
            Quantity_Color col;
            Styles.DecodeColor (SurfCol, col);
            //Base::Console().Message("%d: (%.2f,%.2f,%.2f)\n",col.Name(),col.Red(),col.Green(),col.Blue());
            hash_col[S.HashCode(INT_MAX)] = col;
        }
        if (!BoundCol.IsNull()) {
            Quantity_Color col;
            Styles.DecodeColor (BoundCol, col);
            //Base::Console().Message("%d: (%.2f,%.2f,%.2f)\n",col.Name(),col.Red(),col.Green(),col.Blue());
            hash_col[S.HashCode(INT_MAX)] = col;
        }
        if (!CurveCol.IsNull()) {
            Quantity_Color col;
            Styles.DecodeColor (CurveCol, col);
            //Base::Console().Message("%d: (%.2f,%.2f,%.2f)\n",col.Name(),col.Red(),col.Green(),col.Blue());
            hash_col[S.HashCode(INT_MAX)] = col;
        }
        if (!IsVisible) {
            // sets the invisibility for shape.
        }
    }
  
    return Standard_True;
#endif
}

bool Part::ReadNames (const Handle(XSControl_WorkSession) &WS)
{
#if OCC_VERSION_HEX >= 0x070000
    (void)WS;
    return Standard_False;
#else
    // get starting data
    Handle(Interface_InterfaceModel) Model = WS->Model();
    Handle(XSControl_TransferReader) TR = WS->TransferReader();
    Handle(Transfer_TransientProcess) TP = TR->TransientProcess();

    STEPConstruct_Tool Tool ( WS );

    // iterate on model to find all SDRs and CDSRs
    Standard_Integer nb = Model->NbEntities();
    Handle(Standard_Type) tNAUO = STANDARD_TYPE(StepRepr_NextAssemblyUsageOccurrence);
    Handle(Standard_Type) tPD  = STANDARD_TYPE(StepBasic_ProductDefinition);
    Handle(TCollection_HAsciiString) name;
    for (Standard_Integer i = 1; i <= nb; i++) {
        Handle(Standard_Transient) enti = Model->Value(i);

        // get description of NAUO
        if (enti->DynamicType() == tNAUO) {
            Handle(StepRepr_NextAssemblyUsageOccurrence) NAUO = 
                Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(enti);
            if (NAUO.IsNull()) continue;
            Interface_EntityIterator subs = WS->Graph().Sharings(NAUO);
            for (subs.Start(); subs.More(); subs.Next()) {
                Handle(StepRepr_ProductDefinitionShape) PDS = 
                    Handle(StepRepr_ProductDefinitionShape)::DownCast(subs.Value());
                if (PDS.IsNull()) continue;
                Handle(StepBasic_ProductDefinitionRelationship) PDR = PDS->Definition().ProductDefinitionRelationship();
                if (PDR.IsNull()) continue;
                if (PDR->HasDescription() && 
                    PDR->Description()->Length() >0 ) name = PDR->Description();
                else if (PDR->Name()->Length() >0) name = PDR->Name();
                else name = PDR->Id();
            }

            // find proper label
            TCollection_ExtendedString str ( name->String() );
            Base::Console().Message("Name: %s\n",name->String().ToCString());
        }

        // for PD get name of associated product
        if (enti->DynamicType() == tPD) {
            Handle(StepBasic_ProductDefinition) PD = 
                Handle(StepBasic_ProductDefinition)::DownCast(enti);
            if (PD.IsNull()) continue;
            Handle(StepBasic_Product) Prod = PD->Formation()->OfProduct();
            if (Prod->Name()->UsefullLength()>0) name = Prod->Name();
            else name = Prod->Id();

            TCollection_ExtendedString str ( name->String() );
            Base::Console().Message("Name: %s\n",name->String().ToCString());
        }
        // set a name to the document
        //TCollection_ExtendedString str ( name->String() );
        //TDataStd_Name::Set ( L, str );
    }

    return Standard_True;
#endif
}
