/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel  (FreeCAD@juergen-riegel.net>       *
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


#ifndef GUI_SelectionFilter_H
#define GUI_SelectionFilter_H

#include <string>
#include <CXX/Extensions.hxx>
#include "Selection.h"

namespace App {
    class DocumentObject;
}

namespace Gui {
    struct Node_Block;
    

/** Selection filter definition 
 *  This class builds up a type/count tree out of a string
 *  to test very fast a selection or object/subelement type 
 *  against it. 
 *
 *  Example strings are:
 *  "SELECT Part::Feature SUBELEMENT Edge", 
 *  "SELECT Robot::RobotObject", 
 *  "SELECT Robot::RobotObject COUNT 1..5"
 */
class GuiExport SelectionFilter 
{

public:
    /** Constructs a SelectionFilter object. */
    SelectionFilter(const char* filter);
    SelectionFilter(const std::string& filter);
    virtual ~SelectionFilter();

    /// Set a new filter string 
    void setFilter(const char* filter);
    /** Test to current selection
     *  This method tests the current selection set
     *  against the filter and returns true if the 
     *  described object(s) are selected.
     */
    bool match(void);
    /** Test objects
     *  This method tests if a given object is described in the 
     *  filter. If SubName is not NULL the Subelement gets also
     *  tested.
     */
    bool test(App::DocumentObject*pObj, const char*sSubName);

    void addError(const char* e);
 
    friend class SelectionSingleton;

    std::vector<std::vector<SelectionObject> > Result;

    /// true if a valid filter is set
    bool isValid(void) const {return Ast ? true : false;}

protected:
    std::string Filter;
    std::string Errors;
    bool parse(void);

    Node_Block *Ast;

};

/** Filter object for the SelectionSengleton
 * This object is a link between the selection 
 * filter class and the selection singleton. Created with a 
 * filter string and registered in the selection it will only 
 * allow the descibed object types to be selected.
 * @see SelectionFilter
 * @see SelectionSingleton
 */
class GuiExport SelectionFilterGate: public SelectionGate
{
public:
    /// construct with the filter string
    SelectionFilterGate(const char* filter);
    SelectionFilterGate(SelectionFilter* filter);
    ~SelectionFilterGate();
    virtual bool allow(App::Document*,App::DocumentObject*, const char*);

protected:
    SelectionFilter *Filter;
};

/**
 * Python binding for SelectionFilter class.
 * @see SelectionFilter
 * @author Werner Mayer
 */
class SelectionFilterPy : public Py::PythonExtension<SelectionFilterPy> 
{
private:
    SelectionFilter filter;

public:
    static void init_type(void);    // announce properties and methods

    SelectionFilterPy(const std::string&);
    ~SelectionFilterPy();

    Py::Object repr();
    Py::Object match(const Py::Tuple&);
    Py::Object result(const Py::Tuple&);
    Py::Object test(const Py::Tuple&);

private:
    static PyObject *PyMake(struct _typeobject *, PyObject *, PyObject *);
};

// === Abstract syntax tree (AST) ===========================================

struct Node_Slice 
{
    Node_Slice(int min=1,int max=INT_MAX):Min(min),Max(max){}
    int Min,Max;

};


struct Node_Object 
{
    Node_Object(std::string *type,std::string *subname,Node_Slice* slc )
        :Slice(slc)
    {
        ObjectType = Base::Type::fromName(type->c_str());
        delete (type);
        if(subname){
            SubName = *subname;
            delete subname;
        }
    }
    ~Node_Object(){
        delete Slice;
    }
    Base::Type ObjectType;
    Node_Slice  *Slice;
    std::string SubName;
};

struct Node_Block 
{
    Node_Block(Node_Object* obj){Objects.push_back(obj);}
    std::vector< Node_Object *> Objects;
};


} // namespace Gui


#endif // GUI_SelectionFilter_H

