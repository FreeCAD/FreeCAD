/******************************************************************************
 *   Copyright (c)2012 Konstantinos Poulios <logari81@gmail.com>              *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#ifndef GUI_ReferenceSelection_H
#define GUI_ReferenceSelection_H

#include <Gui/SelectionFilter.h>

namespace PartDesignGui {

class ReferenceSelection : public Gui::SelectionFilterGate
{
    // TODO Replace this set of bools with bitwice enum (2015-09-04, Fat-Zer)
    const App::DocumentObject* support;
    // If set to true, allow picking edges or planes or both
    bool edge, plane;
    // If set to true, allow only linear edges and planar faces
    bool planar;
    // If set to true, allow picking datum points
    bool point;
    // If set to true, allow picking objects from another body in the same part
    bool allowOtherBody;
    // Allow whole object selection
    bool whole;

public:
    ReferenceSelection(const App::DocumentObject* support_,
                       const bool edge_, const bool plane_, const bool planar_, 
                       const bool point_ = false, bool whole_ = false)
        : Gui::SelectionFilterGate((Gui::SelectionFilter*)0),
          support(support_), edge(edge_), plane(plane_), 
          planar(planar_), point(point_), allowOtherBody(true), whole(whole_)
    {
    }
    /**
      * Allow the user to pick only edges or faces (or both) from the defined support
      * Optionally restrict the selection to planar edges/faces
      */
    bool allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName);
};

class NoDependentsSelection : public Gui::SelectionFilterGate
{
    const App::DocumentObject* support;

public:
    NoDependentsSelection(const App::DocumentObject* support_)
        : Gui::SelectionFilterGate((Gui::SelectionFilter*)0), support(support_)
    {
    }
    /**
    * Allow the user to pick only objects which are not in objs getDependencyList
    */
    bool allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName) override;
};

class CombineSelectionFilterGates: public Gui::SelectionFilterGate
{
    std::unique_ptr<Gui::SelectionFilterGate> filter1;
    std::unique_ptr<Gui::SelectionFilterGate> filter2;

public:
    CombineSelectionFilterGates(std::unique_ptr<Gui::SelectionFilterGate> &filter1_, std::unique_ptr<Gui::SelectionFilterGate> &filter2_)
        : Gui::SelectionFilterGate((Gui::SelectionFilter*)0), filter1(std::move(filter1_)), filter2(std::move(filter2_))
    {
    }
    bool allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName) override;
};
// Convenience methods
/// Extract reference from Selection
void getReferencedSelection(const App::DocumentObject* thisObj, const Gui::SelectionChanges& msg,
                            App::DocumentObject*& selObj, std::vector<std::string>& selSub);
/// Return reference as string for UI elements (format <obj>:<subelement>
QString getRefStr(const App::DocumentObject* obj, const std::vector<std::string>& sub);
/// Return reference as string for python in the format (<obj> ["sub1", "sub2", ...])
std::string buildLinkSubPythonStr(const App::DocumentObject* obj, const std::vector<std::string>& subs);
/// Return reference as string for python in the format (<obj> ["sub"?])
std::string buildLinkSingleSubPythonStr(const App::DocumentObject* obj, const std::vector<std::string>& subs);
/// Return reference as string for python in the format [obj1, obj2, ...,]
std::string buildLinkListPythonStr(const std::vector<App::DocumentObject*> & objs);
/// Returns sub reference list as a python string in the format [(obj1,"sub1"),(obj2,"sub2"),...]
std::string buildLinkSubListPythonStr(const std::vector<App::DocumentObject*> & objs,
        const std::vector<std::string>& subs);
} //namespace PartDesignGui

#endif // GUI_ReferenceSelection_H
