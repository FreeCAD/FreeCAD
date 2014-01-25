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

#ifndef DCM_MODULE_STATE_H
#define DCM_MODULE_STATE_H

#include <iosfwd>
#include "defines.hpp"

namespace dcm {

struct ModuleState {

    template<typename Sys>
    struct type {

        typedef Unspecified_Identifier Identifier;

        struct inheriter {

            inheriter()  {
                m_this = (Sys*) this;
            };

            Sys* m_this;

            void saveState(std::ostream& stream);
            void loadState(std::istream& stream);

            void system_sub(boost::shared_ptr<Sys> subsys) {};
        };


        //add only a property to the cluster as we need it to store the clusers global vertex
        typedef mpl::vector1<details::cluster_vertex_prop>  properties;
        typedef mpl::vector0<>  objects;
        typedef mpl::vector0<>  geometries;
        typedef mpl::map0<> signals;

        //nothing to do on startup and copy
        static void system_init(Sys& sys) {};
        static void system_copy(const Sys& from, Sys& into) {};
    };
};

}

#ifndef DCM_EXTERNAL_STATE
#include "imp/module_imp.hpp"
#endif

#endif //DCM_MODULE_STATE_H




