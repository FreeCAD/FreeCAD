
#ifndef APP_DOCUMENTOBJECTSIGNALS_H
#define APP_DOCUMENTOBJECTSIGNALS_H

#include <boost/signals2.hpp>


namespace App {
struct DocumentObject::Public {
    /// signal before changing a property of this object
    boost::signals2::signal<void (const App::DocumentObject&, const App::Property&)> signalBeforeChange;
    /// signal on changed  property of this object
    boost::signals2::signal<void (const App::DocumentObject&, const App::Property&)> signalChanged;
    /// signal on changed property of this object before document scoped signalChangedObject
    boost::signals2::signal<void (const App::DocumentObject&, const App::Property&)> signalEarlyChanged;
};
}


#endif
