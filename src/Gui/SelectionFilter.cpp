/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel (FreeCAD@juergen-riegel.net)        *
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
#ifdef __GNUC__
# include <unistd.h>
#endif

#include <sstream>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <App/DocumentObject.h>
#include <Base/Interpreter.h>
#include <CXX/Objects.hxx>

#include "Selection.h"
#include "SelectionFilter.h"
//#include "SelectionFilterPy.h"
#include "Application.h"

using namespace Gui;

// suppress annoying warnings from generated source files
#ifdef _MSC_VER
# pragma warning(disable : 4003)
# pragma warning(disable : 4018)
# pragma warning(disable : 4065)
# pragma warning(disable : 4335) // disable MAC file format warning on VC
#endif



SelectionFilterGate::SelectionFilterGate(const char* filter)
{
    Filter = new SelectionFilter(filter);
}

SelectionFilterGate::SelectionFilterGate(SelectionFilter* filter)
{
    Filter = filter;
}

SelectionFilterGate::~SelectionFilterGate()
{
    delete Filter;
}

bool SelectionFilterGate::allow(App::Document*pDoc,App::DocumentObject*pObj, const char*sSubName)
{
    return Filter->test(pObj,sSubName);
}

// ----------------------------------------------------------------------------

SelectionGatePython::SelectionGatePython(const Py::Object& obj)
  : gate(obj)
{
}

SelectionGatePython::~SelectionGatePython()
{
}

bool SelectionGatePython::allow(App::Document* doc, App::DocumentObject* obj, const char* sub)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->gate.hasAttr(std::string("allow"))) {
            Py::Callable method(this->gate.getAttr(std::string("allow")));
            Py::Object pyDoc = Py::asObject(doc->getPyObject());
            Py::Object pyObj = Py::asObject(obj->getPyObject());
            Py::String pySub;
            if (sub)
                pySub = std::string(sub);
            Py::Tuple args(3);
            args.setItem(0, pyDoc);
            args.setItem(1, pyObj);
            args.setItem(2, pySub);
            Py::Boolean ok(method.apply(args));
            return (bool)ok;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return true;
}

// ----------------------------------------------------------------------------

SelectionFilterGatePython::SelectionFilterGatePython(SelectionFilterPy* obj) : filter(obj)
{
    Base::PyGILStateLocker lock;
    Py_INCREF(filter);
}

SelectionFilterGatePython::~SelectionFilterGatePython()
{
    Base::PyGILStateLocker lock;
    Py_DECREF(filter);
}

bool SelectionFilterGatePython::allow(App::Document*, App::DocumentObject* obj, const char* sub)
{
    return filter->filter.test(obj, sub);
}

// ----------------------------------------------------------------------------

SelectionFilter::SelectionFilter(const char* filter)
  : Ast(0)
{
    setFilter(filter);
}

SelectionFilter::SelectionFilter(const std::string& filter)
  : Ast(0)
{
    setFilter(filter.c_str());
}

void SelectionFilter::setFilter(const char* filter)
{
    if (!filter || filter[0] == 0) {
        delete Ast;
        Ast = 0;
    }
    else {
        Filter = filter;
        if (!parse())
            throw Base::Exception(Errors.c_str());
    }
}

SelectionFilter::~SelectionFilter()
{
}

bool SelectionFilter::match(void)
{
    if (!Ast)
        return false;
    Result.clear();

    for (std::vector< Node_Object *>::iterator it= Ast->Objects.begin();it!=Ast->Objects.end();++it) {
        int min;
        int max;

        if ((*it)->Slice) {
            min          = (*it)->Slice->Min;
            max          = (*it)->Slice->Max;
        }
        else {
            min          = 1;
            max          = 1;
        }

        std::vector<Gui::SelectionObject> temp = Gui::Selection().getSelectionEx(0,(*it)->ObjectType);

        // test if subnames present
        if ((*it)->SubName.empty()) {
            // if no subnames the count of the object get tested
            if ((int)temp.size()<min || (int)temp.size()>max)
                return false;
        }
        else {
            // if subnames present count all subs over the selected object of type
            int subCount=0;
            for (std::vector<Gui::SelectionObject>::const_iterator it2=temp.begin();it2!=temp.end();++it2) {
                const std::vector<std::string>& subNames = it2->getSubNames();
                if (subNames.empty())
                    return false;
                for (std::vector<std::string>::const_iterator it3=subNames.begin();it3!=subNames.end();++it3) {
                    if (it3->find((*it)->SubName) != 0)
                        return false;
                }
                subCount += subNames.size();
            }
            if (subCount<min || subCount>max)
                return false;
        }
        Result.push_back(temp);
    }
    return true;
}

bool SelectionFilter::test(App::DocumentObject*pObj, const char*sSubName)
{
    if (!Ast)
        return false;

    for (std::vector< Node_Object *>::iterator it= Ast->Objects.begin();it!=Ast->Objects.end();++it) {
        if (pObj->getTypeId().isDerivedFrom((*it)->ObjectType)) {
            if (!sSubName)
                return true;
            if ((*it)->SubName.empty())
                return true;
            if (std::string(sSubName).find((*it)->SubName) == 0)
                return true;
        }
    }
    return false;
}

void SelectionFilter::addError(const char* e)
{
    Errors+=e;
    Errors += '\n';
}

// ----------------------------------------------------------------------------

void SelectionFilterPy::init_type()
{
    behaviors().name("SelectionFilter");
    behaviors().doc("Filter for certain selection");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();
    behaviors().set_tp_new(PyMake);
    add_varargs_method("match",&SelectionFilterPy::match,"match()");
    add_varargs_method("result",&SelectionFilterPy::result,"result()");
    add_varargs_method("test",&SelectionFilterPy::test,"test()");
    add_varargs_method("setFilter",&SelectionFilterPy::setFilter,"setFilter()");
}

PyObject *SelectionFilterPy::PyMake(struct _typeobject *, PyObject *args, PyObject *)
{
    char* str;
    if (!PyArg_ParseTuple(args, "s",&str))
        return 0;
    return new SelectionFilterPy(str);
}

SelectionFilterPy::SelectionFilterPy(const std::string& s)
  : filter(s)
{
}

SelectionFilterPy::~SelectionFilterPy()
{
}

Py::Object SelectionFilterPy::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "SelectionFilter";
    return Py::String(s_out.str());
}

Py::Object SelectionFilterPy::match(const Py::Tuple& args)
{
    return Py::Boolean(filter.match());
}

Py::Object SelectionFilterPy::test(const Py::Tuple& args)
{
    PyObject * pcObj;
    char* text=0;
    if (!PyArg_ParseTuple(args.ptr(), "O!|s",&(App::DocumentObjectPy::Type),&pcObj,&text))
        throw Py::Exception();

    App::DocumentObjectPy* docObj = static_cast<App::DocumentObjectPy*>(pcObj);

    return Py::Boolean(filter.test(docObj->getDocumentObjectPtr(),text));
}

Py::Object SelectionFilterPy::result(const Py::Tuple&)
{
    Py::List list;
    std::vector<std::vector<SelectionObject> >::iterator it;
    for (it = filter.Result.begin(); it != filter.Result.end(); ++it) {
        std::vector<SelectionObject>::iterator jt;
        Py::Tuple tuple(it->size());
        int index=0;
        for (jt = it->begin(); jt != it->end(); ++jt) {
            tuple[index++] = Py::asObject(jt->getPyObject());
        }
        list.append(tuple);
    }
    return list;
}

Py::Object SelectionFilterPy::setFilter(const Py::Tuple& args)
{
    char* text=0;
    if (!PyArg_ParseTuple(args.ptr(), "s",&text))
        throw Py::Exception();
    filter.setFilter(text);
    return Py::None();
}

// === Parser & Scanner stuff ===============================================

// include the Scanner and the Parser for the filter language

SelectionFilter* ActFilter=0;
Node_Block *TopBlock=0;

// error func
void yyerror(char *errorinfo)
	{  ActFilter->addError(errorinfo);  }


// for VC9 (isatty and fileno not supported anymore)
#ifdef _MSC_VER
int isatty (int i) {return _isatty(i);}
int fileno(FILE *stream) {return _fileno(stream);}
#endif

namespace SelectionParser {

// show the parser the lexer method
#define yylex SelectionFilterlex
int SelectionFilterlex(void);

// Parser, defined in SelectionFilter.y
#include "SelectionFilter.tab.c"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Scanner, defined in SelectionFilter.l
#include "lex.SelectionFilter.c"
#endif // DOXYGEN_SHOULD_SKIP_THIS
}

bool SelectionFilter::parse(void)
{
    Errors = "";
    SelectionParser::YY_BUFFER_STATE my_string_buffer = SelectionParser::SelectionFilter_scan_string (Filter.c_str());
    // be aware that this parser is not reentrant! Dont use with Threats!!!
    assert(!ActFilter);
    ActFilter = this;
    /*int my_parse_result =*/ SelectionParser::yyparse();
    ActFilter = 0;
    Ast = TopBlock;
    TopBlock = 0;
    SelectionParser::SelectionFilter_delete_buffer (my_string_buffer);

    if(Errors == "")
        return true;
    else{
        return false;
        delete Ast;
    }
}
