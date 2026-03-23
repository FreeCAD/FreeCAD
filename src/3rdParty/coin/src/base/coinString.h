#ifndef COINSTRING_H
#define COINSTRING_H
#include <Inventor/SbTypeInfo.h>

namespace CoinInternal {
  template<typename Target>
    Target
    FromString(const SbString & input, SbBool * conversionOk = NULL ) {
    typename SbTypeInfo<Target>::SFieldType to;
    SbBool worked = to.set(input.getString());
    if (conversionOk)
      *conversionOk = worked;
    return to.getValue();
  }
  
  template<typename Source>
    SbString
    ToString(const Source & val) {
    SbString s;
    typename SbTypeInfo<Source>::SFieldType From;
    From=val;
    From.get(s);
    return s;
  }
}

template<bool condition, class Then, class Else>
  struct IF
  { 
    typedef Then RET;
  };
template<class Then, class Else>
  struct IF<false,Then,Else>
{ 
  typedef Else RET;
};

template <typename T>
struct Convert {
  static T fromString(const SbString & input, SbBool * conversionOk = NULL)
  {
    T to;
    SbBool worked = to.fromString(input);
    if (conversionOk)
      *conversionOk = worked;
    return to;
  }
  static SbString
  ToString(const T & val)
  {
    return val.toString();
  }
  
};

#include <Inventor/fields/SoSFDouble.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFShort.h>
#include <Inventor/fields/SoSFBool.h>

template <typename T>
struct ConvertPrimitive {
  static T fromString(const SbString & input, SbBool * conversionOk = NULL)
  {
    return CoinInternal::FromString<T>(input,conversionOk);
  }
  static SbString
  ToString(const T & val)
  {
    return CoinInternal::ToString<T>(val);
  }
};


template<typename Target>
Target
FromString(const SbString & input, SbBool * conversionOk = NULL)
{
  return IF<
    SbTypeInfo<Target>::isPrimitive,
    ConvertPrimitive<Target>,
    Convert<Target>
    >::RET::fromString(input, conversionOk);
}

template<typename Source>
SbString
ToString(const Source & val) {
  return IF<
    SbTypeInfo<Source>::isPrimitive,
    ConvertPrimitive<Source>,
    Convert<Source>
    >::RET::ToString(val);
}
#endif //COINSTRING_H
