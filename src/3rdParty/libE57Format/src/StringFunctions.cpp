#include "StringFunctions.h"

#include <cassert>
#include <locale>

namespace e57
{
   template <class FTYPE> std::string floatingPointToStr( FTYPE value, int precision )
   {
      static_assert( std::is_floating_point<FTYPE>::value, "Floating point type required." );

      std::stringstream ss;
      ss.imbue( std::locale::classic() );

      ss << std::scientific << std::setprecision( precision ) << value;

      // Try to remove trailing zeroes and decimal point
      // e.g. 1.23456000000000000e+005  ==> 1.23456e+005
      // e.g. 2.00000000000000000e+005  ==> 2e+005

      std::string s = ss.str();

      // Split into mantissa and exponent
      // e.g. 1.23456000000000000e+005  ==> "1.23456000000000000" + "e+005"
      auto index = s.find_last_of( 'e' );
      assert( index != std::string::npos ); // should not be possible

      std::string mantissa = s.substr( 0, index );
      const std::string exponent = s.substr( index );

      // Double check that we understand the formatting
      if ( exponent[0] == 'e' )
      {
         // Trim trailing zeros from mantissa
         while ( mantissa.back() == '0' )
         {
            mantissa.pop_back();
         }

         // Trim trailing decimal point if possible
         if ( mantissa.back() == '.' )
         {
            mantissa.pop_back();
         }

         // Reassemble whole floating point number
         // Check if can drop exponent.
         if ( ( exponent == "e+00" ) || ( exponent == "e+000" ) )
         {
            s = mantissa;
         }
         else
         {
            s = mantissa + exponent;
         }
      }

      return s;
   }

   template std::string floatingPointToStr<float>( float value, int precision );
   template std::string floatingPointToStr<double>( double value, int precision );

   double strToDouble( const std::string &inStr )
   {
      std::istringstream iss{ inStr };
      iss.imbue( std::locale::classic() );
      double res = 0.;
      iss >> res;
      return res;
   }

}
