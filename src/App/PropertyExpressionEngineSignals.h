

#ifndef APP_PROPERTYSIGNALS_H
#define APP_PROPERTYSIGNALS_H

#include <boost/signals2.hpp>

// TODO: use this struct for all propertysignals, instead of individual ones
namespace App {
struct PropertyExpressionEngine::Public {
    ///signal called when an expression was changed
    
    const boost::signals2::signal<void (const App::ObjectIdentifier &)>* expressionChanged;
};
}

#endif