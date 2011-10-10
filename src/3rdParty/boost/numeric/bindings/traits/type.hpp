#ifndef boost_numeric_bindings_type_hpp
#define boost_numeric_bindings_type_hpp

// This header provides typedefs to float complex and double complex.
// This makes it possible to redefine the complex class being used.

#include <complex>

namespace boost { namespace numeric { namespace bindings { namespace traits {

  /* The types for single and double precision complex numbers.
   * You can use your own types if you define
   * BOOST_NUMERIC_BINDINGS_USE_CUSTOM_COMPLEX_TYPE.
   * Note that these types must have the same memory layout as the
   * corresponding FORTRAN types.
   * For that reason you can even use a different type in each translation
   * unit and the resulting binary will still work!
   */
#ifndef BOOST_NUMERIC_BINDINGS_USE_CUSTOM_COMPLEX_TYPE
  typedef std::complex< float >  complex_f ;
  typedef std::complex< double > complex_d ; 
#endif

  template <typename T> 
  T real (std::complex<T> const& c) { return std::real (c); }
  template <typename T> 
  T imag (std::complex<T> const& c) { return std::imag (c); }

}}}}

#endif // boost_numeric_bindings_type_hpp
