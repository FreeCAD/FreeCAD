

#ifndef APP_PROPERTYSIGNALS_H
#define APP_PROPERTYSIGNALS_H

#include <boost/signals2.hpp>

// TODO: use this struct for all propertysignals, instead of individual ones
namespace App {
struct Property::Public {
    boost::signals2::signal<void (const App::Property&)> signalChanged;
};
}

#endif