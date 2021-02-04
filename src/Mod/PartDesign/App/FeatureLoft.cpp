/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <TopoDS_Solid.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepOffsetAPI_ThruSections.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <TopoDS.hxx>
# include <Precision.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Console.h>
#include <Base/Reader.h>
#include <App/Document.h>
#include <Mod/Part/App/FaceMakerCheese.h>

//#include "Body.h"
#include "FeatureLoft.h"


using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::Loft, PartDesign::ProfileBased)

Loft::Loft()
{
    ADD_PROPERTY_TYPE(Sections,(0),"Loft",App::Prop_None,"List of sections");
    Sections.setSize(0);
    ADD_PROPERTY_TYPE(Ruled,(false),"Loft",App::Prop_None,"Create ruled surface");
    ADD_PROPERTY_TYPE(Closed,(false),"Loft",App::Prop_None,"Close Last to First Profile");
    ADD_PROPERTY_TYPE(WireOrders,(""),"Loft",App::Prop_None,"Order of wires per profile/section");
}

void Loft::onChanged(const App::Property* prop)
{
    if (prop == &WireOrders) {
        wireOrders = getWireOrders();
    }

    ProfileBased::onChanged(prop);
}

short Loft::mustExecute() const
{
    if (Sections.isTouched())
        return 1;
    if (Ruled.isTouched())
        return 1;
    if (Closed.isTouched())
        return 1;
    if (WireOrders.isTouched())
        return 1;

    return ProfileBased::mustExecute();
}

/** Gets the wire orders from the property Wire Orders
 *  and converts to vector of int vectors to be held
 *  in the wireOrders variable.  If an empty list is
 *  returned Wire Orders property gets set to defaults
 *  elsewhere in the code
 */
const std::vector<std::vector<int> > Loft::getWireOrders()
{
    std::vector<std::vector<int>> orders; //wireOrders is set to this on return
    const std::vector<std::string> lines = WireOrders.getValue();
    if (!lines.empty()){
        if (lines[0].empty()){
            return orders;
        }
    }
    bool bSuccess = true;
    for (std::string line : lines){
        std::vector<std::string> vals_as_strings;
        //strip the profile/section name from the left of the line first
        int colon_idx = line.rfind(":");
        line = line.substr(colon_idx+1, std::string::npos);
        std::stringstream s_stream(line);
        while(s_stream.good()){
            std::string str_val;
            std::getline(s_stream, str_val, ';');
            vals_as_strings.push_back(str_val);
        }
        std::vector<int> line_vals;
        for (std::string val : vals_as_strings ){
            try{
                line_vals.push_back(std::stoi(val));
            } catch (const std::invalid_argument&){
                Base::Console().Warning("Loft: Invalid argument: %s\n", val.c_str());
                Base::Console().Warning("Ignoring Wire Orders\n");
                bSuccess = false;
                break;
            } catch (const std::out_of_range&){ //number is too big to fit into an int
                Base::Console().Warning("Loft: Out of range error: %s\n", val.c_str());
                Base::Console().Warning("Ignoring Wire Orders\n");
                bSuccess = false;
                break;
            }
        }
        orders.push_back(line_vals);

        if (!bSuccess){
            orders.clear();
        } else {
            int size = orders.front().size();
            for (std::vector<int> v : orders){
                if (size != (int) v.size()){
                    Base::Console().Warning("Loft: Error parsing Wire Orders (size mismatch).\n");
                    Base::Console().Warning("Ignoring wire orders\n");
                    orders.clear();
                    break;
                }
            }
        }
    }
    return orders;
}

/** Sets Wire Orders property from vector of vector of ints */
void Loft::setWireOrders(const std::vector<std::vector<int>> wireorders)
{
    std::vector<std::string> orders;
    std::vector<std::string> labels;
    labels.push_back(Profile.getValue()->Label.getValue());
    for (auto section : Sections.getValues()){
        labels.push_back(section->Label.getValue());
    }
    int ii=0;
    for (std::vector<int> vec : wireorders){
        std::ostringstream s_stream;
        if ((int)labels.size() > ii){
            s_stream << labels[ii++] << ": ";
            for (int jj : vec){
                s_stream << jj << ";";
            }
            std::string str = s_stream.str();
            str.pop_back(); //remove trailing ;
            orders.push_back(str);
        }
    }
    WireOrders.setValues(orders);
    WireOrders.touch();
}


/** Sets Wire Orders property to reasonable defaults */
void Loft::setDefaultWireOrders(const std::vector<std::vector<TopoDS_Wire>> wiresections)
{
    std::vector<std::string> orders;
    int wireCount = wiresections.size();
    int sectionCount = wiresections.front().size();
    std::vector<std::string> labels;
    labels.push_back(Profile.getValue()->Label.getValue());
    for (auto section : Sections.getValues()){
        labels.push_back(section->Label.getValue());
    }
    for (int ii = 0; ii < sectionCount; ii++){
        std::ostringstream s_stream;
        if ((int)labels.size() > ii)
            s_stream << labels[ii] << ": ";
        for (int jj = 0; jj< wireCount - 1; jj++){
            s_stream << jj << ";";
        }
        s_stream << wireCount - 1;
        orders.push_back(s_stream.str());
    }
    WireOrders.setValues(orders);
}

const std::vector<std::vector<int>> Loft::getDefaultWireOrders(const std::vector<std::vector<TopoDS_Wire>> wiresections)
{
    std::vector<std::vector<int>> orders;
    int wireCount = wiresections.size();
    int sectionCount = wiresections.front().size();
    for (int ii = 0; ii < sectionCount; ii++){
        std::vector<int> wirePositions;
        for (int jj = 0; jj< wireCount; jj++){
            wirePositions.push_back(jj);
        }
        orders.push_back(wirePositions);
    }
    return orders;
}

App::DocumentObjectExecReturn *Loft::execute(void)
{

    std::vector<TopoDS_Wire> wires;
    try {
        wires = getProfileWires();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    TopoDS_Shape sketchshape = getVerifiedFace();
    if (sketchshape.IsNull())
        return new App::DocumentObjectExecReturn("Loft: Creating a face from sketch failed");

    // if the Base property has a valid shape, fuse the pipe into it
    TopoDS_Shape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
        base = TopoDS_Shape();
    }

    try {
        //setup the location
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        if(!base.IsNull())
            base.Move(invObjLoc);

        //build up multisections
        auto multisections = Sections.getValues();
        if(multisections.empty())
            return new App::DocumentObjectExecReturn("Loft: At least one section is needed");

        std::vector<std::vector<TopoDS_Wire>> wiresections;
        for(TopoDS_Wire& wire : wires)
            wiresections.emplace_back(1, wire);

        for(App::DocumentObject* obj : multisections) {
            if(!obj->isDerivedFrom(Part::Feature::getClassTypeId()))
                return  new App::DocumentObjectExecReturn("Loft: All sections need to be part features");

            TopExp_Explorer ex;
            size_t i=0;
            for (ex.Init(static_cast<Part::Feature*>(obj)->Shape.getValue(), TopAbs_WIRE); ex.More(); ex.Next(), ++i) {
                if(i>=wiresections.size())
                    return new App::DocumentObjectExecReturn("Loft: Sections need to have the same amount of inner wires as the base section");
                wiresections[i].push_back(TopoDS::Wire(ex.Current()));
            }
            if(i<wiresections.size())
                    return new App::DocumentObjectExecReturn("Loft: Sections need to have the same amount of inner wires as the base section");

        }

        /** begin wire orders code
         *  in here we modify wiresections if necessary
        */

        if (wireOrders.empty()){ //initialize Wire Orders property and skip wire orders code
            setDefaultWireOrders(wiresections);
        } else {
            bool bIsValid = true;
            std::vector<std::vector<int>> defaults = getDefaultWireOrders(wiresections);
            if (wireOrders.size() != defaults.size()){
                bIsValid = false;
            } else {
                for (int ii = 0; ii < (int) wireOrders.size(); ii++){
                    if (wireOrders[ii].size() != defaults[ii].size()){
                        bIsValid = false;
                        break;
                    }
                }
                if (wireOrders == defaults){
                    bIsValid = false;
                }
            }
            if (bIsValid){ //use them

                /** Remap wiresections based on the Wire Orders
                 *
                 *  wiresections are arranged differently from wireOrders
                 *  With wireOrders each element is a vector representing the wires in each sketch
                 *  So, if you have 3 sketches with 2 wires each, say a circle inside a square
                 *  wireOrders by default would be:
                 *  (0,1)  3 vectors with 2 elements each
                 *  (0,1)
                 *  (0,1)
                 *  But wiresections would be:
                 *  (0, 1, 2) 2 vectors of 3 elements each
                 *  (0, 1, 2)
                 *  This is the reason for ii and jj being swapped below
                 */
                bool bSuccess = true;
                std::vector<std::vector<TopoDS_Wire>> tempwiresections = wiresections;
                for (int ii = 0; ii < (int) wireOrders.size(); ii++){
                    for (int jj = 0; jj < (int) wireOrders[ii].size(); jj++){
                        int mapping = wireOrders[ii][jj];
                        if (mapping >= 0 && mapping < (int) wiresections.size()){
                            tempwiresections[jj][ii] = wiresections[mapping][ii];
                        } else {
                            bSuccess = false;
                            Base::Console().Warning("Loft: Invalid Wire Order (index out of bounds): %d\n",mapping);
                            Base::Console().Warning("Ignoring Wire Orders\n");
                            break;
                        }
                    }
                    if (!bSuccess)
                        break;
                }
                if (bSuccess){
                    wiresections = tempwiresections;
                }
            } else {
                /** This code is reached when the user has changed either the number of sketches
                 *  or the number of wires per sketch so we must rebuild to defaults.
                 *  If so give the user a reference to what the old values were, but
                 *  avoid the message if user was only using defaults
                 */
                bool bIsDefault = true;
                for (std::vector<int> order : wireOrders){
                    int previous = -1;
                    for (int curval : order){
                        if (curval < previous){
                            bIsDefault = false;
                            break;
                        } else {
                            previous = curval;
                        }
                    }
                    if (!bIsDefault){
                        break;
                    }
                }
                if (!bIsDefault){
                    std::ostringstream msg;
                    msg << "Loft: Due to changes in geometry it is necessary to rebuild Wire Orders to defaults.\n";
                    msg << "Previous Wire Orders values for your reference:\n";
                    std::vector<std::string> old_orders = WireOrders.getValues();
                    for (std::string oo : old_orders){
                        msg << oo << std::endl;
                    }
                    Base::Console().Warning(msg.str().c_str());
                }
                setDefaultWireOrders(wiresections);
            }
        }
        /** end wire orders code*/

        //build all shells
        std::vector<TopoDS_Shape> shells;
        for(std::vector<TopoDS_Wire>& wires : wiresections) {

            BRepOffsetAPI_ThruSections mkTS(false, Ruled.getValue(), Precision::Confusion());

            for(TopoDS_Wire& wire : wires)   {
                 wire.Move(invObjLoc);
                 mkTS.AddWire(wire);
            }

            mkTS.Build();
            if (!mkTS.IsDone())
                return new App::DocumentObjectExecReturn("Loft could not be built");

            //build the shell use simulate to get the top and bottom wires in an easy way
            shells.push_back(mkTS.Shape());
        }

        //build the top and bottom face, sew the shell and build the final solid
        TopoDS_Shape front = getVerifiedFace();
        front.Move(invObjLoc);
        std::vector<TopoDS_Wire> backwires;
        for(std::vector<TopoDS_Wire>& wires : wiresections)
            backwires.push_back(wires.back());

        TopoDS_Shape back = Part::FaceMakerCheese::makeFace(backwires);

        BRepBuilderAPI_Sewing sewer;
        sewer.SetTolerance(Precision::Confusion());
        sewer.Add(front);
        sewer.Add(back);
        for(TopoDS_Shape& s : shells)
            sewer.Add(s);

        sewer.Perform();

        //build the solid
        BRepBuilderAPI_MakeSolid mkSolid;
        mkSolid.Add(TopoDS::Shell(sewer.SewedShape()));
        if(!mkSolid.IsDone())
            return new App::DocumentObjectExecReturn("Loft: Result is not a solid");

        TopoDS_Shape result = mkSolid.Shape();
        BRepClass3d_SolidClassifier SC(result);
        SC.PerformInfinitePoint(Precision::Confusion());
        if ( SC.State() == TopAbs_IN) {
            result.Reverse();
        }

        AddSubShape.setValue(result);

        if(base.IsNull()) {
            Shape.setValue(getSolid(result));
            return App::DocumentObject::StdReturn;
        }

        if(getAddSubType() == FeatureAddSub::Additive) {

            BRepAlgoAPI_Fuse mkFuse(base, result);
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn("Loft: Adding the loft failed");
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkFuse.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn("Loft: Resulting shape is not a solid");
            int solidCount = countSolids(boolOp);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn("Loft: Result has multiple solids. This is not supported at this time.");
            }

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }
        else if(getAddSubType() == FeatureAddSub::Subtractive) {

            BRepAlgoAPI_Cut mkCut(base, result);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn("Loft: Subtracting the loft failed");
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkCut.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn("Loft: Resulting shape is not a solid");
            int solidCount = countSolids(boolOp);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn("Loft: Result has multiple solids. This is not supported at this time.");
            }

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("Loft: A fatal error occurred when making the loft");
    }
}


PROPERTY_SOURCE(PartDesign::AdditiveLoft, PartDesign::Loft)
AdditiveLoft::AdditiveLoft() {
    addSubType = Additive;
}

PROPERTY_SOURCE(PartDesign::SubtractiveLoft, PartDesign::Loft)
SubtractiveLoft::SubtractiveLoft() {
    addSubType = Subtractive;
}
