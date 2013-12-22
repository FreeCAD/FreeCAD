/*
    openDCM, dimensional constraint manager
    Copyright (C) 2012  Stefan Troeger <stefantroeger@gmx.net>

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

#ifndef GCM_DEFINES_3D_H
#define GCM_DEFINES_3D_H

namespace dcm {

enum SolverFailureHandling {
    IgnoreResults,
    ApplyResults
};

enum SubsystemSolveHandling {
    Automatic,
    Manual
};

//options
struct solverfailure {

    typedef SolverFailureHandling type;
    typedef setting_property kind;
    struct default_value {
        SolverFailureHandling operator()() {
            return IgnoreResults;
        };
    };
};

struct subsystemsolving {

    typedef SubsystemSolveHandling type;
    typedef setting_property kind;
    struct default_value {
        SubsystemSolveHandling operator()() {
            return Manual;
        };
    };
};

namespace details {

enum { cluster3D = 100};

struct m3d {}; 	//base of module3d::type to allow other modules check for it

}

}

#endif
