/***************************************************************************
 *   Copyright (c) 2013 Juergen Riegel                                     *
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
#endif

#include "Quantity.h"
#include "Exception.h"

using namespace Base;

Quantity::Quantity()
{
    this->_Value = 0.0;
}

Quantity::Quantity(const Quantity& that)
{
    *this = that ;
}

Quantity::Quantity(double Value, const Unit& unit)
{
    this->_Unit = unit;
    this->_Value = Value;
}


bool Quantity::operator ==(const Quantity& that) const
{
    return (this->_Value == that._Value) && (this->_Unit == that._Unit) ;
}


Quantity Quantity::operator *(const Quantity &p) const
{
    return Quantity(this->_Value * p._Value,this->_Unit * p._Unit);
}
Quantity Quantity::operator /(const Quantity &p) const
{
    return Quantity(this->_Value / p._Value,this->_Unit / p._Unit);
}

Quantity Quantity::pow(const Quantity &p) const
{
    if(!p._Unit.isEmpty()) 
        throw Base::Exception("Quantity::pow(): exponent must not have a unit");
    return Quantity(
        std::pow(this->_Value, p._Value),
        this->_Unit.pow((short)p._Value)
        );
}


Quantity Quantity::operator +(const Quantity &p) const
{
    if(this->_Unit != p._Unit) 
        throw Base::Exception("Quantity::operator +(): Unit missmatch in plus operation");
    return Quantity(this->_Value + p._Value,this->_Unit);
}
Quantity Quantity::operator -(const Quantity &p) const
{
    if(this->_Unit != p._Unit) 
        throw Base::Exception("Quantity::operator +(): Unit missmatch in plus operation");
    return Quantity(this->_Value - p._Value,this->_Unit);
}

Quantity Quantity::operator -(void) const
{
    return Quantity(-(this->_Value),this->_Unit);
}

Quantity& Quantity::operator = (const Quantity &New)
{
    this->_Value = New._Value;
    this->_Unit = New._Unit;
    return *this;
}

// === Parser & Scanner stuff ===============================================

// include the Scanner and the Parser for the Quantitys

Quantity ScanResult;

#ifndef  DOUBLE_MAX
# define DOUBLE_MAX 1.7976931348623157E+308    /* max decimal value of a "double"*/
#endif
#ifndef  DOUBLE_MIN
# define DOUBLE_MIN 2.2250738585072014E-308    /* min decimal value of a "double"*/
#endif


// error func
void Quantity_yyerror(char *errorinfo)
{  
    throw Base::Exception(errorinfo);  
}


// for VC9 (isatty and fileno not supported anymore)
//#ifdef _MSC_VER
//int isatty (int i) {return _isatty(i);}
//int fileno(FILE *stream) {return _fileno(stream);}
//#endif

namespace QuantityParser {

// show the parser the lexer method
#define yylex QuantityLexer
int QuantityLexer(void);

// Parser, defined in QuantityParser.y
#include "QuantityParser.c"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Scanner, defined in QuantityParser.l
#include "QuantityLexer.c"
#endif // DOXYGEN_SHOULD_SKIP_THIS
}

Quantity Quantity::parse(const char* buffer)
{
    // parse from buffer
    QuantityParser::YY_BUFFER_STATE my_string_buffer = QuantityParser::yy_scan_string (buffer);
    // set the global return variables
    ScanResult = Quantity(DOUBLE_MIN);
    // run the parser
    QuantityParser::yyparse ();
    // free the scan buffer
    QuantityParser::yy_delete_buffer (my_string_buffer);

    if (ScanResult == Quantity(DOUBLE_MIN))
        throw Base::Exception("Unknown error in Quantity expression");
    return ScanResult;
}
