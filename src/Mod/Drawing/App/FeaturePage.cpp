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
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

#include <boost/regex.hpp>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>

#include "FeatureClip.h"
#include "FeaturePage.h"
#include "FeatureView.h"


using namespace Drawing;
using namespace std;

//===========================================================================
// FeaturePage
//===========================================================================

PROPERTY_SOURCE(Drawing::FeaturePage, App::DocumentObjectGroup)

FeaturePage::FeaturePage(void)
    : numChildren(0)
{
    static const char* group = "Drawing view";

    ADD_PROPERTY_TYPE(PageResult,
                      (nullptr),
                      group,
                      App::Prop_Output,
                      "Resulting SVG document of that page");
    ADD_PROPERTY_TYPE(Template, (""), group, App::Prop_None, "Template for the page");
    ADD_PROPERTY_TYPE(EditableTexts,
                      (""),
                      group,
                      App::Prop_None,
                      "Substitution values for the editable strings in the template");
}

FeaturePage::~FeaturePage()
{}

void FeaturePage::onBeforeChange(const App::Property* prop)
{
    if (prop == &Group) {
        numChildren = Group.getSize();
    }

    App::DocumentObjectGroup::onBeforeChange(prop);
}

/// get called by the container when a Property was changed
void FeaturePage::onChanged(const App::Property* prop)
{
    if (prop == &PageResult) {
        if (this->isRestoring()) {
            // When loading a document the included file
            // doesn't need to exist at this point.
            Base::FileInfo fi(PageResult.getValue());
            if (!fi.exists()) {
                return;
            }
        }
    }
    else if (prop == &EditableTexts) {
        if (!this->isRestoring()) {
            this->execute();
            return;
        }
    }
    else if (prop == &Template) {
        if (!this->isRestoring()) {
            EditableTexts.setValues(getEditableTextsFromTemplate());
        }
    }
    else if (prop == &Group) {
        if (Group.getSize() != numChildren) {
            numChildren = Group.getSize();
            touch();
        }
    }

    App::DocumentObjectGroup::onChanged(prop);
}

void FeaturePage::onDocumentRestored()
{
    // Needs to be tmp. set because otherwise the custom text gets overridden (#0002064)
    this->StatusBits.set(App::Restore);  // the 'Restore' flag

    Base::FileInfo templateInfo(Template.getValue());
    if (!templateInfo.exists()) {
        Base::FileInfo fi(Template.getValue());
        if (fi.fileName().empty()) {
            fi.setFile(PageResult.getValue());
        }
        std::string path =
            App::Application::getResourceDir() + "Mod/Drawing/Templates/" + fi.fileName();
        // try to find the template in user dir/Templates first
        Base::FileInfo tempfi(App::Application::getUserAppDataDir() + "Templates/" + fi.fileName());
        if (tempfi.exists()) {
            path = tempfi.filePath();
        }
        Template.setValue(path);
    }

    this->StatusBits.reset(App::Restore);  // the 'Restore' flag
}

App::DocumentObjectExecReturn* FeaturePage::execute(void)
{
    std::string temp = Template.getValue();
    if (temp.empty()) {
        return App::DocumentObject::StdReturn;
    }

    Base::FileInfo fi(temp);
    if (!fi.isReadable()) {
        // if there is a old absolute template file set use a redirect
        fi.setFile(App::Application::getResourceDir() + "Mod/Drawing/Templates/" + fi.fileName());
        // try the redirect
        if (!fi.isReadable()) {
            Base::Console().Log("FeaturePage::execute() not able to open %s!\n",
                                Template.getValue());
            std::string error = std::string("Cannot open file ") + Template.getValue();
            return new App::DocumentObjectExecReturn(error);
        }
    }

    if (std::string(PageResult.getValue()).empty()) {
        PageResult.setValue(fi.filePath().c_str());
    }

    // open Template file
    string line;
    ifstream file(fi.filePath().c_str());

    // make a temp file for FileIncluded Property
    string tempName = PageResult.getExchangeTempFile();
    ostringstream ofile;
    string tempendl = "--endOfLine--";

    while (getline(file, line)) {
        // check if the marker in the template is found
        if (line.find("<!-- DrawingContent -->") == string::npos) {
            // if not -  write through
            ofile << line << tempendl;
        }
        else {
            // get through the children and collect all the views
            const std::vector<App::DocumentObject*>& Grp = Group.getValues();
            for (std::vector<App::DocumentObject*>::const_iterator It = Grp.begin();
                 It != Grp.end();
                 ++It) {
                if ((*It)->getTypeId().isDerivedFrom(Drawing::FeatureView::getClassTypeId())) {
                    Drawing::FeatureView* View = static_cast<Drawing::FeatureView*>(*It);
                    if (View->Visible.getValue()) {
                        ofile << View->ViewResult.getValue();
                        ofile << tempendl << tempendl << tempendl;
                    }
                }
                else if ((*It)->getTypeId().isDerivedFrom(Drawing::FeatureClip::getClassTypeId())) {
                    Drawing::FeatureClip* Clip = static_cast<Drawing::FeatureClip*>(*It);
                    if (Clip->Visible.getValue()) {
                        ofile << Clip->ViewResult.getValue();
                        ofile << tempendl << tempendl << tempendl;
                    }
                }
                else if ((*It)->getTypeId().isDerivedFrom(
                             App::DocumentObjectGroup::getClassTypeId())) {
                    // getting children inside subgroups too
                    App::DocumentObjectGroup* SubGroup =
                        static_cast<App::DocumentObjectGroup*>(*It);
                    const std::vector<App::DocumentObject*>& SubGrp = SubGroup->Group.getValues();
                    for (std::vector<App::DocumentObject*>::const_iterator Grit = SubGrp.begin();
                         Grit != SubGrp.end();
                         ++Grit) {
                        if ((*Grit)->getTypeId().isDerivedFrom(
                                Drawing::FeatureView::getClassTypeId())) {
                            Drawing::FeatureView* SView = static_cast<Drawing::FeatureView*>(*Grit);
                            if (SView->Visible.getValue()) {
                                ofile << SView->ViewResult.getValue();
                                ofile << tempendl << tempendl << tempendl;
                            }
                        }
                    }
                }
            }
        }
    }

    file.close();

    // checking for freecad editable texts
    string outfragment(ofile.str());
    const std::vector<std::string>& editText = EditableTexts.getValues();
    if (!editText.empty()) {
        boost::regex e1("<text.*?freecad:editable=\"(.*?)\".*?<tspan.*?>(.*?)</tspan>");
        string::const_iterator begin, end;
        begin = outfragment.begin();
        end = outfragment.end();
        boost::match_results<std::string::const_iterator> what;
        std::size_t count = 0;
        std::string newfragment;
        newfragment.reserve(outfragment.size());

        while (boost::regex_search(begin, end, what, e1)) {
            if (count < editText.size()) {
                // change values of editable texts
                boost::regex e2("(<text.*?freecad:editable=\"" + what[1].str()
                                + "\".*?<tspan.*?)>(.*?)(</tspan>)");
                std::back_insert_iterator<std::string> out(newfragment);
                boost::regex_replace(out,
                                     begin,
                                     what[0].second,
                                     e2,
                                     "$1>" + editText[count] + "$3");
            }
            count++;
            begin = what[0].second;
        }

        // now copy the rest
        newfragment.insert(newfragment.end(), begin, end);
        outfragment = newfragment;
    }

    // restoring linebreaks and saving the file
    boost::regex e3("--endOfLine--");
    string fmt = "\\n";
    outfragment = boost::regex_replace(outfragment, e3, fmt);
    ofstream outfinal(tempName.c_str());
    outfinal << outfragment;
    outfinal.close();

    PageResult.setValue(tempName.c_str());

    return App::DocumentObject::StdReturn;
}

std::vector<std::string> FeaturePage::getEditableTextsFromTemplate(void) const
{
    // getting editable texts from "freecad:editable" attributes in SVG template

    std::vector<string> eds;

    std::string temp = Template.getValue();
    if (!temp.empty()) {
        Base::FileInfo tfi(temp);
        if (!tfi.isReadable()) {
            // if there is a old absolute template file set use a redirect
            tfi.setFile(App::Application::getResourceDir() + "Mod/Drawing/Templates/"
                        + tfi.fileName());
            // try the redirect
            if (!tfi.isReadable()) {
                return eds;
            }
        }
        string tline, tfrag;
        ifstream tfile(tfi.filePath().c_str());
        while (getline(tfile, tline)) {
            tfrag += tline;
            tfrag += "--endOfLine--";
        }
        tfile.close();
        boost::regex e("<text.*?freecad:editable=\"(.*?)\".*?<tspan.*?>(.*?)</tspan>");
        string::const_iterator tbegin, tend;
        tbegin = tfrag.begin();
        tend = tfrag.end();
        boost::match_results<std::string::const_iterator> twhat;
        while (boost::regex_search(tbegin, tend, twhat, e)) {
            eds.push_back(twhat[2]);
            tbegin = twhat[0].second;
        }
    }
    return eds;
}
