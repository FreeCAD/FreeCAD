/***************************************************************************
 *   Copyright (c) WandererFan - 2016    (wandererfan@gmail.com)           *
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
# include <sstream>
#endif

#include <iomanip>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>

#include "DrawViewDraft.h"

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewDraft
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewDraft, TechDraw::DrawViewSymbol)


DrawViewDraft::DrawViewDraft(void)
{
    static const char *group = "Draft view";

    ADD_PROPERTY_TYPE(Source ,(0),group,App::Prop_None,"Draft object for this view");
    Source.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(LineWidth,(0.35),group,App::Prop_None,"Line width of this view");
    ADD_PROPERTY_TYPE(FontSize,(12.0),group,App::Prop_None,"Text size for this view");
    ADD_PROPERTY_TYPE(Direction ,(0,0,1.0),group,App::Prop_None,"Projection direction. The direction you are looking from.");
    ADD_PROPERTY_TYPE(Color,(0.0f,0.0f,0.0f),group,App::Prop_None,"The default color of text and lines");
    ADD_PROPERTY_TYPE(LineStyle,("Solid") ,group,App::Prop_None,"A line style to use for this view. Can be Solid, Dashed, Dashdot, Dot or a SVG pattern like 0.20,0.20");
    ADD_PROPERTY_TYPE(LineSpacing,(1.0f),group,App::Prop_None,"The spacing between lines to use for multiline texts");
    ScaleType.setValue("Custom");
}

DrawViewDraft::~DrawViewDraft()
{
}

short DrawViewDraft::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result = Source.isTouched() ||
                    LineWidth.isTouched() ||
                    FontSize.isTouched() ||
                    Direction.isTouched() ||
                    Color.isTouched() ||
                    LineStyle.isTouched() ||
                    LineSpacing.isTouched();
    }
    if ((bool) result) {
        return result;
    }
    return DrawViewSymbol::mustExecute();
}



App::DocumentObjectExecReturn *DrawViewDraft::execute(void)
{
//    Base::Console().Message("DVDr::execute() \n");
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    App::DocumentObject* sourceObj = Source.getValue();
    if (sourceObj) {
        std::string svgFrag;
        std::string svgHead = getSVGHead();
        std::string svgTail = getSVGTail();
        std::string FeatName = getNameInDocument();
        std::string SourceName = sourceObj->getNameInDocument();
        // Draft.getSVG(obj,scale=1,linewidth=0.35,fontsize=12,fillstyle="shape color",direction=None,linestyle=None,color=None,linespacing=None,techdraw=False)

        std::stringstream paramStr;
        App::Color col = Color.getValue();
        paramStr << ",scale=" << getScale() 
                 << ",linewidth=" << LineWidth.getValue() 
                 << ",fontsize=" << FontSize.getValue()
                 // TODO treat fillstyle here
                 << ",direction=FreeCAD.Vector(" << Direction.getValue().x << "," << Direction.getValue().y << "," << Direction.getValue().z << ")"
                 << ",linestyle=\"" << LineStyle.getValue() << "\""
                 << ",color=\"" << col.asCSSString() << "\""
                 << ",linespacing=" << LineSpacing.getValue()
                 // We must set techdraw to "true" becausea couple of things behave differently than in Drawing
                 << ",techdraw=True";

// this is ok for a starting point, but should eventually make dedicated Draft functions that build the svg for all the special cases
// (Arch section, etc)
// like Draft.makeDrawingView, but we don't need to create the actual document objects in Draft, just the svg.
        Base::Interpreter().runString("import Draft");
        Base::Interpreter().runStringArg("svgBody = Draft.getSVG(App.activeDocument().%s %s)",
                                         SourceName.c_str(),paramStr.str().c_str());
//        Base::Interpreter().runString("print svgBody");
        Base::Interpreter().runStringArg("App.activeDocument().%s.Symbol = '%s' + svgBody + '%s'",
                                          FeatName.c_str(),svgHead.c_str(),svgTail.c_str());
        }
//    requestPaint();
    return DrawView::execute();
}

std::string DrawViewDraft::getSVGHead(void)
{
    std::string head = std::string("<svg\\n") +
                       std::string("	xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"\\n") +
                       std::string("	xmlns:freecad=\"http://www.freecadweb.org/wiki/index.php?title=Svg_Namespace\">\\n");
    return head;
}

std::string DrawViewDraft::getSVGTail(void)
{
    std::string tail = "\\n</svg>";
    return tail;
}

//DVD is still compatible with old Source PropertyLink so doesn't need DV::Restore logic
void DrawViewDraft::Restore(Base::XMLReader &reader)
{
// this is temporary code for backwards compat (within v0.17).  can probably be deleted once there are no development
// fcstd files with old property types in use. 
    reader.readElement("Properties");
    int Cnt = reader.getAttributeAsInteger("Count");

    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Property");
        const char* PropName = reader.getAttribute("name");
        const char* TypeName = reader.getAttribute("type");
        App::Property* schemaProp = getPropertyByName(PropName);
        try {
            if(schemaProp){
                if (strcmp(schemaProp->getTypeId().getName(), TypeName) == 0){        //if the property type in obj == type in schema
                    schemaProp->Restore(reader);                                      //nothing special to do
                } else if (strcmp(PropName, "Source") == 0) {
                    App::PropertyLinkGlobal glink;
                    App::PropertyLink link;
                    if (strcmp(glink.getTypeId().getName(),TypeName) == 0) {            //property in file is plg
                        glink.setContainer(this);
                        glink.Restore(reader);
                        if (glink.getValue() != nullptr) {
                            static_cast<App::PropertyLink*>(schemaProp)->setScope(App::LinkScope::Global);
                            static_cast<App::PropertyLink*>(schemaProp)->setValue(glink.getValue());
                        }
                    } else if (strcmp(link.getTypeId().getName(),TypeName) == 0) {            //property in file is pl
                        link.setContainer(this);
                        link.Restore(reader);
                        if (link.getValue() != nullptr) {
                            static_cast<App::PropertyLink*>(schemaProp)->setScope(App::LinkScope::Global);
                            static_cast<App::PropertyLink*>(schemaProp)->setValue(link.getValue());
                        }
                    
                    } else {
                        // has Source prop isn't PropertyLink or PropertyLinkGlobal! 
                        Base::Console().Log("DrawViewDraft::Restore - old Document Source is weird: %s\n", TypeName);
                        // no idea
                    }
                } else {
                    Base::Console().Log("DrawViewDraft::Restore - old Document has unknown Property\n");
                }
            }
        }
        catch (const Base::XMLParseException&) {
            throw; // re-throw
        }
        catch (const Base::Exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const std::exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const char* e) {
            Base::Console().Error("%s\n", e);
        }
#ifndef FC_DEBUG
        catch (...) {
            Base::Console().Error("PropertyContainer::Restore: Unknown C++ exception thrown\n");
        }
#endif

        reader.readEndElement("Property");
    }
    reader.readEndElement("Properties");

}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewDraftPython, TechDraw::DrawViewDraft)
template<> const char* TechDraw::DrawViewDraftPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderDraft";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewDraft>;
}
