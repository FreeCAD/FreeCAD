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

#ifndef DCM_MODULEPARSER_H
#define DCM_MODULEPARSER_H

#define DCM_USE_MODULESTATE

#ifdef _WIN32
	//warning about to long decoraded names, won't affect the code correctness
	#pragma warning( disable : 4503 )
#endif

//use phoenix v3 to allow normal boost phoenix use outside of spirit and not the spirit phoenix v2 version
#define BOOST_SPIRIT_USE_PHOENIX_V3

#include "moduleState/module.hpp"
#include "moduleState/traits.hpp"

#ifdef DCM_EXTERNAL_STATE

#define DCM_EXTERNAL_STATE_INCLUDE_001 <opendcm/moduleState/imp/edge_vertex_generator_imp.hpp>
#define DCM_EXTERNAL_STATE_001( System )\
    template struct dcm::details::edge_generator<System>; \
    template struct dcm::details::vertex_generator<System>; \
    
     
#define DCM_EXTERNAL_STATE_INCLUDE_002 <opendcm/moduleState/imp/object_generator_imp.hpp>
#define DCM_EXTERNAL_STATE_002( System )\
    template struct dcm::details::obj_gen<System>; \
     
#define DCM_EXTERNAL_STATE_INCLUDE_003 <opendcm/moduleState/imp/property_generator_imp.hpp>
#define DCM_EXTERNAL_STATE_003( System )\
    template struct dcm::details::vertex_prop_gen<System>; \
    template struct dcm::details::edge_prop_gen<System>; \
    template struct dcm::details::cluster_prop_gen<System>; \
    template struct dcm::details::system_prop_gen<System>; \
    template struct dcm::details::kernel_prop_gen<System>; 

#define DCM_EXTERNAL_STATE_INCLUDE_004 <opendcm/moduleState/imp/generator_imp.hpp>
#define DCM_EXTERNAL_STATE_004( System )\
    template struct dcm::generator<System>; \
    
#define DCM_EXTERNAL_STATE_INCLUDE_005 <opendcm/moduleState/imp/property_parser_imp.hpp>
#define DCM_EXTERNAL_STATE_005( System )\
    template struct dcm::details::vertex_prop_par<System>; \
    template struct dcm::details::edge_prop_par<System>; \
    template struct dcm::details::cluster_prop_par<System>; \
    template struct dcm::details::system_prop_par<System>; \
    template struct dcm::details::kernel_prop_par<System>; 
    
#define DCM_EXTERNAL_STATE_INCLUDE_006 <opendcm/moduleState/imp/object_parser_imp.hpp>
#define DCM_EXTERNAL_STATE_006( System )\
    template struct dcm::details::obj_par<System>; \
    
#define DCM_EXTERNAL_STATE_INCLUDE_007 <opendcm/moduleState/imp/edge_vertex_parser_imp.hpp>
#define DCM_EXTERNAL_STATE_007( System )\
    template struct dcm::details::edge_parser<System>; \
    template struct dcm::details::vertex_parser<System>; \
    
#define DCM_EXTERNAL_STATE_INCLUDE_008 <opendcm/moduleState/imp/parser_imp.hpp>
#define DCM_EXTERNAL_STATE_008( System )\
    template struct dcm::parser<System>; \
    
#define DCM_EXTERNAL_STATE_INCLUDE_009 <opendcm/moduleState/imp/module_imp.hpp>
#define DCM_EXTERNAL_STATE_009( System )\
    template struct dcm::ModuleState::type<System>; \

#endif //external

#endif //DCM_MODULEPARSER_H

