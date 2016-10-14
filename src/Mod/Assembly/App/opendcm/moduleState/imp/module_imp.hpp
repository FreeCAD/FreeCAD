/*
    openDCM, dimensional constraint manager
    Copyright (C) 2013  Stefan Troeger <stefantroeger@gmx.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef DCM_MODULE_STATE_IMP_H
#define DCM_MODULE_STATE_IMP_H

#include <iosfwd>

#include "../module.hpp"
#include "../indent.hpp"
#include "../generator.hpp"
#include "../parser.hpp"
#include "../defines.hpp"

namespace qi = boost::spirit::qi;

namespace dcm {

template<typename Sys>
void ModuleState::type<Sys>::inheriter::saveState(std::ostream& stream) {

    boost::iostreams::filtering_ostream indent_stream;
    indent_stream.push(indent_filter());
    indent_stream.push(stream);

    std::ostream_iterator<char> out(indent_stream);
    generator<Sys> gen;

    karma::generate(out, gen, *m_this);
};

template<typename Sys>
void ModuleState::type<Sys>::inheriter::loadState(std::istream& stream) {

    //disable skipping of whitespace
    stream.unsetf(std::ios::skipws);

    // wrap istream into iterator
    boost::spirit::istream_iterator begin(stream);
    boost::spirit::istream_iterator end;

    // use iterator to parse file data
    parser<Sys> par;
    m_this->clear();
    qi::phrase_parse(begin, end, par, qi::space, *m_this);
};

}

#endif //DCM_MODULE_STATE_H




