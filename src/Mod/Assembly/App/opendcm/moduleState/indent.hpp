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

#ifndef DCM_INDENT_H
#define DCM_INDENT_H

#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/filtering_stream.hpp>

class indent_filter : public boost::iostreams::output_filter {
public:
    explicit indent_filter() : indent(0) {};

    template<typename Sink>
    bool put(Sink& dest, int c) {

        if(c == '#') {
            indent++;
            return true;
        } else if(c == '$') {
            indent--;
            return true;
        } else if(c == '\n') {
            bool ret = boost::iostreams::put(dest, c);
            for(int i=0; (i<indent) && ret; i++) {
                ret = boost::iostreams::put(dest, ' ');
                ret = boost::iostreams::put(dest, ' ');
            }

            return ret;
        };

        indent = (indent < 0) ? 0 : indent;

        return boost::iostreams::put(dest, c);
    }

    template<typename Source>
    void close(Source&) {
        indent = 0;
    }
private:
    int indent;
};

#endif //DCM_INDENT_H
