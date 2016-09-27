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

#ifndef GCM_LOGGING_H
#define GCM_LOGGING_H

#ifdef USE_LOGGING

#define BOOST_LOG_DYN_LINK

#include <boost/log/core.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/expressions/formatters.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/expressions.hpp>

#include <boost/shared_ptr.hpp>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

namespace dcm {

enum severity_level {

    iteration,
    solving,
    manipulation,
    information,
    error
};

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)

static int counter = 0;
typedef sinks::synchronous_sink< sinks::text_file_backend > sink_t;
typedef src::severity_logger< severity_level > dcm_logger;

inline boost::shared_ptr< sink_t > init_log() {

    //create the filename
    std::stringstream str;
    str<<"openDCM_"<<counter<<"_%N.log";
    counter++;

    boost::shared_ptr< logging::core > core = logging::core::get();

    boost::shared_ptr< sinks::text_file_backend > backend =
        boost::make_shared< sinks::text_file_backend >(
            keywords::file_name = str.str(),             //file name pattern
            keywords::rotation_size = 10 * 1024 * 1024 	//Rotation size is 10MB
        );

    // Wrap it into the frontend and register in the core.
    // The backend requires synchronization in the frontend.
    boost::shared_ptr< sink_t > sink(new sink_t(backend));
    
    sink->set_formatter(
        expr::stream <<"[" << expr::attr<std::string>("Tag") <<"] "
        << expr::if_(expr::has_attr<std::string>("ID")) [
            expr::stream << "["<< expr::attr< std::string >("ID")<<"] "]
        << expr::smessage
    );

    core->add_sink(sink);
    return sink;
};

inline void stop_log(boost::shared_ptr< sink_t >& sink) {

    boost::shared_ptr< logging::core > core = logging::core::get();

    // Remove the sink from the core, so that no records are passed to it
    core->remove_sink(sink);
    sink.reset();
};

}; //dcm

#endif //USE_LOGGING

#endif //GCM_LOGGING_H
