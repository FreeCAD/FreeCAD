// SPDX-License-Identifier: BSL-1.0
// Copyright (c) 2020 PTC Inc.
// Copyright (c) 2022 Andy Maloney <asmaloney@gmail.com>

// For M_PI. This needs to be first, otherwise we might already include math header without M_PI and
// we would get nothing because of the header guards.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c)
#define _USE_MATH_DEFINES
#include <cmath>

#include "E57SimpleData.h"

#include "Common.h"
#include "StringFunctions.h"

namespace e57
{
   /// Validates a Data3D and throws on error.
   void _validateData3D( const Data3D &inData3D )
   {
      if ( inData3D.pointFields.pointRangeNodeType == NumericalNodeType::Integer )
      {
         throw E57_EXCEPTION2( ErrorInvalidNodeType, "pointRangeNodeType cannot be Integer" );
      }

      if ( inData3D.pointFields.angleNodeType == NumericalNodeType::Integer )
      {
         throw E57_EXCEPTION2( ErrorInvalidNodeType, "angleNodeType cannot be Integer" );
      }
   }

   /// To avoid exposing M_PI, we define the constructor here.
   SphericalBounds::SphericalBounds()
   {
      rangeMinimum = 0.;
      rangeMaximum = DOUBLE_MAX;
      azimuthStart = -M_PI;
      azimuthEnd = M_PI;

      constexpr auto HALF_PI = M_PI / 2.0;

      elevationMinimum = -HALF_PI;
      elevationMaximum = HALF_PI;
   }

   template <typename COORDTYPE>
   Data3DPointsData_t<COORDTYPE>::Data3DPointsData_t( Data3D &data3D ) : _selfAllocated( true )
   {
      static_assert( std::is_floating_point<COORDTYPE>::value, "Floating point type required." );

      _validateData3D( data3D );

      constexpr bool cIsFloat = std::is_same<COORDTYPE, float>::value;

      // We need to adjust min/max for floats.
      if ( cIsFloat )
      {
         data3D.pointFields.pointRangeMinimum = FLOAT_MIN;
         data3D.pointFields.pointRangeMaximum = FLOAT_MAX;
         data3D.pointFields.angleMinimum = FLOAT_MIN;
         data3D.pointFields.angleMaximum = FLOAT_MAX;
         data3D.pointFields.timeMinimum = FLOAT_MIN;
         data3D.pointFields.timeMaximum = FLOAT_MAX;
      }

      // IF point range node type is not ScaledInteger
      // THEN make sure the proper floating point type is set
      if ( data3D.pointFields.pointRangeNodeType != NumericalNodeType::ScaledInteger )
      {
         data3D.pointFields.pointRangeNodeType =
            ( cIsFloat ? NumericalNodeType::Float : NumericalNodeType::Double );
      }

      // IF angle node type is not ScaledInteger
      // THEN make sure the proper floating point type is set
      if ( data3D.pointFields.angleNodeType != NumericalNodeType::ScaledInteger )
      {
         data3D.pointFields.angleNodeType =
            ( cIsFloat ? NumericalNodeType::Float : NumericalNodeType::Double );
      }

      const auto cPointCount = data3D.pointCount;

      if ( data3D.pointFields.cartesianXField )
      {
         cartesianX = new COORDTYPE[cPointCount];
      }

      if ( data3D.pointFields.cartesianYField )
      {
         cartesianY = new COORDTYPE[cPointCount];
      }

      if ( data3D.pointFields.cartesianZField )
      {
         cartesianZ = new COORDTYPE[cPointCount];
      }

      if ( data3D.pointFields.cartesianInvalidStateField )
      {
         cartesianInvalidState = new int8_t[cPointCount];
      }

      if ( data3D.pointFields.intensityField )
      {
         intensity = new double[cPointCount];
      }

      if ( data3D.pointFields.isIntensityInvalidField )
      {
         isIntensityInvalid = new int8_t[cPointCount];
      }

      if ( data3D.pointFields.colorRedField )
      {
         colorRed = new uint16_t[cPointCount];
      }

      if ( data3D.pointFields.colorGreenField )
      {
         colorGreen = new uint16_t[cPointCount];
      }

      if ( data3D.pointFields.colorBlueField )
      {
         colorBlue = new uint16_t[cPointCount];
      }

      if ( data3D.pointFields.isColorInvalidField )
      {
         isColorInvalid = new int8_t[cPointCount];
      }

      if ( data3D.pointFields.sphericalRangeField )
      {
         sphericalRange = new COORDTYPE[cPointCount];
      }

      if ( data3D.pointFields.sphericalAzimuthField )
      {
         sphericalAzimuth = new COORDTYPE[cPointCount];
      }

      if ( data3D.pointFields.sphericalElevationField )
      {
         sphericalElevation = new COORDTYPE[cPointCount];
      }

      if ( data3D.pointFields.sphericalInvalidStateField )
      {
         sphericalInvalidState = new int8_t[cPointCount];
      }

      if ( data3D.pointFields.rowIndexField )
      {
         rowIndex = new int32_t[cPointCount];
      }

      if ( data3D.pointFields.columnIndexField )
      {
         columnIndex = new int32_t[cPointCount];
      }

      if ( data3D.pointFields.returnIndexField )
      {
         returnIndex = new int8_t[cPointCount];
      }

      if ( data3D.pointFields.returnCountField )
      {
         returnCount = new int8_t[cPointCount];
      }

      if ( data3D.pointFields.timeStampField )
      {
         timeStamp = new double[cPointCount];
      }

      if ( data3D.pointFields.isTimeStampInvalidField )
      {
         isTimeStampInvalid = new int8_t[cPointCount];
      }

      if ( data3D.pointFields.normalXField )
      {
         normalX = new float[cPointCount];
      }

      if ( data3D.pointFields.normalYField )
      {
         normalY = new float[cPointCount];
      }

      if ( data3D.pointFields.normalZField )
      {
         normalZ = new float[cPointCount];
      }
   }

   template <typename COORDTYPE> Data3DPointsData_t<COORDTYPE>::~Data3DPointsData_t()
   {
      static_assert( std::is_floating_point<COORDTYPE>::value, "Floating point type required." );

      if ( !_selfAllocated )
      {
         return;
      }

      delete[] cartesianX;
      delete[] cartesianY;
      delete[] cartesianZ;
      delete[] cartesianInvalidState;

      delete[] intensity;
      delete[] isIntensityInvalid;

      delete[] colorRed;
      delete[] colorGreen;
      delete[] colorBlue;
      delete[] isColorInvalid;

      delete[] sphericalRange;
      delete[] sphericalAzimuth;
      delete[] sphericalElevation;
      delete[] sphericalInvalidState;

      delete[] rowIndex;
      delete[] columnIndex;

      delete[] returnIndex;
      delete[] returnCount;

      delete[] timeStamp;
      delete[] isTimeStampInvalid;

      delete[] normalX;
      delete[] normalY;
      delete[] normalZ;

      // Set them all to nullptr.
      *this = Data3DPointsData_t<COORDTYPE>();
   }

#if defined( _MSC_VER )
   template struct E57_DLL Data3DPointsData_t<float>;
   template struct E57_DLL Data3DPointsData_t<double>;
#else
   template struct Data3DPointsData_t<float>;
   template struct Data3DPointsData_t<double>;
#endif
} // end namespace e57
