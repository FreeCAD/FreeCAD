

#ifndef APP_PROPERTYBOOLLIST_H
#define APP_PROPERTYBOOLLIST_H


#include <boost/dynamic_bitset.hpp>

#include "Property.h"

namespace App {

/** Bool list properties
 *
 */
class AppExport PropertyBoolList : public PropertyListsT<bool,boost::dynamic_bitset<> >
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
    using inherited = PropertyListsT<bool, boost::dynamic_bitset<> >;

public:
    PropertyBoolList();
    ~PropertyBoolList() override;

    PyObject *getPyObject() override;
    void setPyObject(PyObject* py) override;

    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    Property *Copy() const override;
    void Paste(const Property &from) override;
    unsigned int getMemSize () const override;

protected:
    bool getPyValue(PyObject* py) const override;
};

}

#endif