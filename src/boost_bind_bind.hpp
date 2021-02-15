#ifndef FREECAD_BIND_BIND_HPP_WORKAROUND
#define FREECAD_BIND_BIND_HPP_WORKAROUND

// Workaround for boost older than 1.60
#ifndef BOOST_BIND_BIND_HPP_INCLUDED
#include <boost/bind/bind.hpp>
#endif

#include <boost/version.hpp>
#if (BOOST_VERSION < 106000)
namespace boost { namespace placeholders {
    using ::_1;
    using ::_2;
    using ::_3;
    using ::_4;
    using ::_5;
    using ::_6;
    using ::_7;
    using ::_8;
    using ::_9;
}};
#endif

#endif // #ifndef FREECAD_BIND_BIND_HPP_WORKAROUND
