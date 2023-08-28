/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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

#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <App/DocumentObject.h>
#include <Base/Interpreter.h>

#include "Selection.h"
#include "SelectionFilter.h"
#include "SelectionFilterPy.h"
#include "SelectionObject.h"


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

SelectionFilterGate::SelectionFilterGate()
{
    Filter = nullptr;
}

SelectionFilterGate::~SelectionFilterGate()
{
    delete Filter;
}

bool SelectionFilterGate::allow(App::Document* /*pDoc*/, App::DocumentObject*pObj, const char*sSubName)
{
    return Filter->test(pObj,sSubName);
}

// ----------------------------------------------------------------------------

SelectionGatePython::SelectionGatePython(const Py::Object& obj)
  : gate(obj)
{
}

SelectionGatePython::~SelectionGatePython() = default;

bool SelectionGatePython::allow(App::Document* doc, App::DocumentObject* obj, const char* sub)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->gate.hasAttr(std::string("allow"))) {
            Py::Callable method(this->gate.getAttr(std::string("allow")));
            Py::Object pyDoc = Py::asObject(doc->getPyObject());
            Py::Object pyObj = Py::asObject(obj->getPyObject());
            Py::Object pySub = Py::None();
            if (sub)
                pySub = Py::String(sub);
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
  : Ast(nullptr)
{
    setFilter(filter);
}

SelectionFilter::SelectionFilter(const std::string& filter)
  : Ast(nullptr)
{
    setFilter(filter.c_str());
}

void SelectionFilter::setFilter(const char* filter)
{
    if (!filter || filter[0] == 0) {
        Ast.reset();
        Filter.clear();
    }
    else {
        Filter = filter;
        if (!parse())
            throw Base::ParserError(Errors.c_str());
    }
}

SelectionFilter::~SelectionFilter() = default;

bool SelectionFilter::match()
{
    if (!Ast)
        return false;
    Result.clear();

    for (const auto& it : Ast->Objects) {
        std::size_t min = 1;
        std::size_t max = 1;

        if (it->Slice) {
            min = it->Slice->Min;
            max = it->Slice->Max;
        }

        std::vector<Gui::SelectionObject> temp = Gui::Selection().getSelectionEx(nullptr, it->ObjectType);

        // test if subnames present
        if (it->SubName.empty()) {
            // if no subnames the count of the object get tested
            if (temp.size() < min || temp.size() > max)
                return false;
        }
        else {
            // if subnames present count all subs over the selected object of type
            std::size_t subCount = 0;
            for (const auto & it2 : temp) {
                const std::vector<std::string>& subNames = it2.getSubNames();
                if (subNames.empty())
                    return false;
                for (const auto & subName : subNames) {
                    if (subName.find(it->SubName) != 0)
                        return false;
                }
                subCount += subNames.size();
            }

            if (subCount < min || subCount > max)
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

    for (const auto& it : Ast->Objects) {
        if (pObj->getTypeId().isDerivedFrom(it->ObjectType)) {
            if (!sSubName)
                return true;
            if (it->SubName.empty())
                return true;
            if (std::string(sSubName).find(it->SubName) == 0)
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

// === Parser & Scanner stuff ===============================================

// include the Scanner and the Parser for the filter language

SelectionFilter* ActFilter=nullptr;
Node_Block *TopBlock=nullptr;

// error func
void yyerror(char *errorinfo)
    {  ActFilter->addError(errorinfo);  }


// for VC9 (isatty and fileno not supported anymore)
#ifdef _MSC_VER
int isatty (int i) {return _isatty(i);}
int fileno(FILE *stream) {return _fileno(stream);}
#endif

namespace SelectionParser {

/*!
 * \brief The StringFactory class
 * Helper class to record the created strings used by the parser.
 */
class StringFactory {
    std::list<std::unique_ptr<std::string>> data;
    std::size_t max_elements = 20;
public:
    static StringFactory* instance() {
        static auto inst = new StringFactory();
        return inst;
    }
    std::string* make(const std::string& str) {
        data.push_back(std::make_unique<std::string>(str));
        return data.back().get();
    }
    static std::string* New(const std::string& str) {
        return StringFactory::instance()->make(str);
    }
    void clear() {
        if (data.size() > max_elements)
            data.clear();
    }
};

// show the parser the lexer method
#define yylex SelectionFilterlex
int SelectionFilterlex();

// Parser, defined in SelectionFilter.y
#include "SelectionFilter.tab.c"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Scanner, defined in SelectionFilter.l
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wsign-compare"
# pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#elif defined (__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#include "lex.SelectionFilter.c"
#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined (__GNUC__)
# pragma GCC diagnostic pop
#endif
#endif // DOXYGEN_SHOULD_SKIP_THIS
}

bool SelectionFilter::parse()
{
    Errors = "";
    SelectionParser::YY_BUFFER_STATE my_string_buffer = SelectionParser::SelectionFilter_scan_string (Filter.c_str());
    // be aware that this parser is not reentrant! Don't use with Threats!!!
    assert(!ActFilter);
    ActFilter = this;
    /*int my_parse_result =*/ SelectionParser::yyparse();
    ActFilter = nullptr;
    Ast.reset(TopBlock);
    TopBlock = nullptr;
    SelectionParser::SelectionFilter_delete_buffer (my_string_buffer);
    SelectionParser::StringFactory::instance()->clear();

    if (Errors.empty()) {
        return true;
    }
    else {
        return false;
    }
}
