#ifndef NAVLIB_TYPES_H_INCLUDED_
#define NAVLIB_TYPES_H_INCLUDED_
// <copyright file="navlib_types.h" company="3Dconnexion">
// -------------------------------------------------------------------------------------------------
// This file is part of the FreeCAD CAx development system.
//
// Copyright (c) 2014-2023 3Dconnexion.
//
// This source code is released under the GNU Library General Public License, (see "LICENSE").
// -------------------------------------------------------------------------------------------------
// </copyright>
// <history>
// *************************************************************************************************
// File History
//
// $Id: navlib_types.h 20296 2023-07-12 06:07:32Z mbonk $
//
// 01/23/14 MSB Initial design
// </history>
// <description>
// *************************************************************************************************
// File Description
//
// This header file describes the variable types used in the 3dconnexion navlib interface.
//
// *************************************************************************************************
// </description>
#include <navlib/navlib_defines.h>

#if (defined(_MSC_VER) && _MSC_VER < 1600)
typedef __int8 int8_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#if _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int size_t;
#endif
#else
#include <stdint.h>
#endif

// 3dxware
#include <siappcmd_types.h>

// stdlib
#include <string.h>

#if __cplusplus
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#if defined(_MSC_VER) && (_MSC_VER <= 1800)
#include <type_traits>
#endif
/// <summary>
/// Contains the navigation library API types and functions
/// </summary>
NAVLIB_BEGIN_

/// <summary>
/// Defines a type of object to be thrown as exception. It reports errors that result from attempts
/// to convert a value to an incompatible type.
/// </summary>
class conversion_error : public std::logic_error {
  typedef std::logic_error Mybase;

public:

  explicit conversion_error(const std::string &message)
      : Mybase(message.c_str()) { // construct from message string
  }

  explicit conversion_error(const char *message)
      : Mybase(message) { // construct from message string
  }
};
NAVLIB_END_
#endif //__cplusplus

NAVLIB_BEGIN_
/// <summary>
/// Describes the type of a property as well as the type of the value passed in value_t
/// </summary>
/// <remarks>
/// Generally no type conversion is performed. So that if a property is defined as being of
/// float_type then the navlib will expect the value_t structure to contain a value with
/// type=float_type.
/// </remarks>
typedef enum propertyTypes {
  /// <summary>
  /// <see cref="auto"/>.
  /// </summary>
  auto_type = -2,
  /// <summary>
  /// The type is unknown.
  /// </summary>
  unknown_type = -1,
  /// <summary>
  /// <see cref="void"/>*.
  /// </summary>
  voidptr_type = 0,
  /// <summary>
  /// <see cref="bool_t"/>.
  /// </summary>
  bool_type,
  /// <summary>
  /// <see cref="long"/>.
  /// </summary>
  long_type,
  /// <summary>
  /// <see cref="float"/>.
  /// </summary>
  float_type,
  /// <summary>
  /// <see cref="double"/>.
  /// </summary>
  double_type,
  /// <summary>
  /// <see cref="point_t"/>.
  /// </summary>
  point_type,
  /// <summary>
  /// <see cref="vector_t"/>.
  /// </summary>
  vector_type,
  /// <summary>
  /// <see cref="matrix_t"/>.
  /// </summary>
  matrix_type,
  /// <summary>
  /// <see cref="string_t"/>.
  /// </summary>
  string_type,
  /// <summary>
  /// const <see cref="SiActionNodeEx_t"/>*.
  /// </summary>
  actionnodeexptr_type,
  /// <summary>
  /// <see cref="plane_t"/>.
  /// </summary>
  plane_type,
  /// <summary>
  /// <see cref="box_t"/>.
  /// </summary>
  box_type,
  /// <summary>
  /// <see cref="frustum_t"/>.
  /// </summary>
  frustum_type,
  /// <summary>
  /// <see cref="cstr_t"/>.
  /// </summary>
  cstr_type,
  /// <summary>
  /// <see cref="imagearray_t"/>.
  /// </summary>
  imagearray_type
} propertyType_t;

/// <summary>
/// Type used to identify which property is being addressed.
/// </summary>
typedef const char *property_t;

/// <summary>
/// Describes the available property access
/// </summary>
typedef enum propertyAccess {
  /// <summary>
  /// Property cannot be accessed.
  /// </summary>
  eno_access = 0,
  /// <summary>
  /// Property can be changed.
  /// </summary>
  ewrite_access,
  /// <summary>
  /// Property is read only.
  /// </summary>
  eread_access,
  /// <summary>
  /// Property can be read and written to.
  /// </summary>
  eread_write_access
} propertyAccess_t;

/// <summary>
/// Describes a property's type and required access.
/// </summary>
/// <remarks>
/// This defines the value of <see cref="value_t.type"/> used in <see cref="value_t"/> to pass the
/// property's value between the client and the navlib.
/// </remarks>
typedef struct {
  /// <summary>
  /// The name of the property.
  /// </summary>
  property_t name;
  /// <summary>
  /// The type of the value stored by the property.
  /// </summary>
  propertyType_t type;
  /// <summary>
  /// The access the client interface exposes to the navlib server
  /// </summary>
  propertyAccess_t client;
} propertyDescription_t;

/// <summary>
/// Type used to store bool values in <see cref="value_t>"/>
/// </summary>
typedef uint32_t bool_t;

/// <summary>
/// Type used for the client defined parameter used in <see cref="fnGetProperty_t"/> and <see
/// cref="fnSetProperty_t"/>.
/// </summary>
typedef uint64_t param_t;

#if defined(NAVLIB_USE_NON_STANDARD_STRUCT_EXTENSION)
#if defined(_MSC_VER)
#pragma message("warning: The non-standard anonymous structure definitions are deprecated and will be removed in the next release.")
#else
#warning The non-standard anonymous structure definitions are deprecated and will be removed in the next release.
#endif

/// <summary>
/// Represents an x-, y-, and z-coordinate location in 3-D space.
/// </summary>
typedef struct point {
  union {
    struct {
      double x, y, z;
    };
    double coordinates[3];
  };

#if __cplusplus
private:
  template <class T> static auto &get_value(T &t, size_t i) {
    switch (i) {
    case 0:
      return t.x;
    case 1:
      return t.y;
    case 2:
      return t.z;
    default:
      throw std::out_of_range("index i");
    }
  }

public:
  double operator[](size_t i) const {
    return get_value(*this, i);
  }

  double &operator[](size_t i) {
    return get_value(*this, i);
  }

  typedef const double (&const_array)[3];
  operator const_array() const {
    return coordinates;
  }
#endif
} point_t;

/// <summary>
/// Represents a displacement in 3-D space.
/// </summary>
typedef struct vector {
  union {
    struct {
      double x, y, z;
    };
    double components[3];
  };

#if __cplusplus
private:
  template <class T> static auto &get_value(T &t, size_t i) {
    switch (i) {
    case 0:
      return t.x;
    case 1:
      return t.y;
    case 2:
      return t.z;
    default:
      throw std::out_of_range("index i");
    }
  }

public:
  double operator[](size_t i) const {
    return get_value(*this, i);
  }

  double &operator[](size_t i) {
    return get_value(*this, i);
  }

  typedef const double (&const_array)[3];
  operator const_array() const {
    return components;
  }
#endif
} vector_t;

/// <summary>
/// Represents a plane in 3-D space.
/// </summary>
/// <remarks>The plane is defined as a unit normal <see cref="vector_t"/> to the plane and the
/// distance of the plane to the origin along the normal. A <see cref="point_t"/> p on the plane
/// satisfies the vector equation: <para>n.(p - point_t(0,0,0)) + d = 0;</para></remarks>
typedef struct plane {
  union {
    struct {
      double x, y, z, d;
    };
    struct {
      vector_t n;
      double d_;
    };
    double equation[4];
  };

#if __cplusplus
private:
  template <class T> static auto &get_value(T &t, size_t i) {
    switch (i) {
    case 0:
      return t.x;
    case 1:
      return t.y;
    case 2:
      return t.z;
    case 3:
      return t.d;
    default:
      throw std::out_of_range("index i");
    }
  }

public:
  double operator[](size_t i) const {
    return get_value(*this, i);
  }

  double &operator[](size_t i) {
    return get_value(*this, i);
  }

  typedef const double (&const_array)[4];
  operator const_array() const {
    return equation;
  }
#endif
} plane_t;

/// <summary>
/// Represents a 3D-rectangle volume.
/// </summary>
/// <remarks>
/// The volume is described by two diagonally opposing <see cref="point_t"/> locations.
/// <see cref="box_t.min"/> contains the coordinates with the smallest values.
/// </remarks>
typedef struct box {
  union {
    struct {
      double min_x, min_y, min_z, max_x, max_y, max_z;
    };
    struct {
      point_t min, max;
    };
    double b[6];
  };

#if __cplusplus
  /// <summary>
  /// checks whether the box is empty.
  /// </summary>
  /// <returns></returns>
  bool empty() const {
    return (max.x < min.x || max.y < min.y || max.z < min.z);
  }

  typedef const double (&const_array)[6];
  operator const_array() const {
    return b;
  }
#endif
} box_t;

/// <summary>
/// Represents a frustum.
/// </summary>
typedef struct frustum {
  union {
    struct {
      double left, right, bottom, top, nearVal, farVal;
    };
    double parameters[6];
  };
#if __cplusplus
  typedef const double (&const_array)[6];
  operator const_array() const {
    return parameters;
  }
#endif
} frustum_t;

/// <summary>
/// Represents a 4 x 4 matrix used for transformations in 3-D space.
/// </summary>
typedef struct matrix {
  union {
    struct {
      double m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33;
    };
    double m[16];
    double m4x4[4][4];
  };
#if __cplusplus
private:
  template <class T> static auto &get_value(T &t, size_t i) {
    switch (i) {
    case 0:
      return t.m00;
    case 1:
      return t.m01;
    case 2:
      return t.m02;
    case 3:
      return t.m03;
    case 4:
      return t.m10;
    case 5:
      return t.m11;
    case 6:
      return t.m12;
    case 7:
      return t.m13;
    case 8:
      return t.m20;
    case 9:
      return t.m21;
    case 10:
      return t.m22;
    case 11:
      return t.m23;
    case 12:
      return t.m30;
    case 13:
      return t.m31;
    case 14:
      return t.m32;
    case 15:
      return t.m33;
    default:
      throw std::out_of_range("index i");
    }
  }

  template <class T> static auto &get_value(T &&t, size_t row, size_t column) {
    return get_value(std::forward<T&&>(t), row * 4 + column);
  }

  public:
  double* begin() {
      return &m00;
  }

  double* end() {
    return &m[16];
  }

  double operator[](size_t i) const {
    return get_value(*this, i);
  }

  double &operator[](size_t i) {
    return get_value(*this, i);
  }

  double operator()(size_t row, size_t column) const {
    return get_value(*this, row, column);
    // return get_value(*this, row * 4 + column);
  }

  double &operator()(size_t row, size_t column) {
    return get_value(*this, row, column);
    // return get_value(*this, row * 4 + column);
  }

  typedef const double (&const_array)[16];
  operator const_array() const {
    return m;
  }
#endif
} matrix_t;
#else
/// <summary>
/// Represents an x-, y-, and z-coordinate location in 3-D space.
/// </summary>
typedef struct point {
  double x, y, z;

#if __cplusplus
private:
#if !defined(_MSC_VER) || (_MSC_VER > 1800)
  template <class T> static auto &get_value(T &t, size_t i) {
    switch (i) {
    case 0:
      return t.x;
    case 1:
      return t.y;
    case 2:
      return t.z;
    default:
      throw std::out_of_range("index i");
    }
  }
#else
  template <class T> static double get_value(const T &t, size_t i) {
    return get_value(const_cast<T &>(t), i);
  }

  template <class T> static double &get_value(T &t, size_t i) {
    switch (i) {
    case 0:
      return t.x;
    case 1:
      return t.y;
    case 2:
      return t.z;
    default:
      throw std::out_of_range("index i");
    }
  }
#endif

public:
  /// <summary>
  /// Allows <see cref="point"/> members to be accessed using array notation.
  /// </summary>
  /// <param name="i">Index of the member.</param>
  /// <returns>The value of the member.</returns>
  /// <remarks>The index of x=0, y=1, z=2.</remarks>
  double operator[](size_t i) const {
    return get_value(*this, i);
  }

  /// <summary>
  /// Allows <see cref="point"/> members to be accessed using array notation.
  /// </summary>
  /// <param name="i">Index of the member.</param>
  /// <returns>A reference to the<see cref="point"/> member.</returns>
  /// <remarks>The index of x=0, y=1, z=2.</remarks>
  double &operator[](size_t i) {
    return get_value(*this, i);
  }
#endif
} point_t;

/// <summary>
/// Represents a displacement in 3-D space.
/// </summary>
typedef struct vector {
  double x, y, z;

#if __cplusplus
private:
#if !defined(_MSC_VER) || (_MSC_VER > 1800)
  template <class T> static auto &get_value(T &t, size_t i) {
    switch (i) {
    case 0:
      return t.x;
    case 1:
      return t.y;
    case 2:
      return t.z;
    default:
      throw std::out_of_range("index i");
    }
  }
#else
  template <class T> static double get_value(const T &t, size_t i) {
    return get_value(const_cast<T &>(t), i);
  }

  template <class T> static double &get_value(T &t, size_t i){
    switch (i) {
    case 0:
      return t.x;
    case 1:
      return t.y;
    case 2:
      return t.z;
    default:
      throw std::out_of_range("index i");
    }
  }
#endif

public:
  /// <summary>
  /// Allows <see cref="vector"/> members to be accessed using array notation.
  /// </summary>
  /// <param name="i">Index of the member.</param>
  /// <returns>The value of the member.</returns>
  /// <remarks>The index of x=0, y=1, z=2.</remarks>
  double operator[](size_t i) const {
    return get_value(*this, i);
  }

  /// <summary>
  /// Allows <see cref="vector"/> members to be accessed using array notation.
  /// </summary>
  /// <param name="i">Index of the member.</param>
  /// <returns>A reference to the <see cref="vector"/> member.</returns>
  /// <remarks>The index of x=0, y=1, z=2.</remarks>
  double &operator[](size_t i) {
    return get_value(*this, i);
  }
#endif
} vector_t;

/// <summary>
/// Represents a plane in 3-D space.
/// </summary>
/// <remarks>The plane is defined as a unit normal <see cref="vector_t"/> to the plane and the
/// distance of the plane to the origin along the normal. A <see cref="point_t"/> p on the plane
/// satisfies the vector equation: <para>n.(p - point_t(0,0,0)) + d = 0;</para></remarks>
typedef struct plane {
  vector_t n;
  double d;
} plane_t;

/// <summary>
/// Represents a 3D-rectangle volume.
/// </summary>
/// <remarks>
/// The volume is described by two diagonally opposing <see cref="point_t"/> locations.
/// <see cref="box_t.min"/> contains the coordinates with the smallest values.
/// </remarks>
typedef struct box {
  point_t min, max;
#if __cplusplus
  /// <summary>
  /// checks whether the box is empty.
  /// </summary>
  /// <returns></returns>
  bool empty() const {
    return (max.x < min.x || max.y < min.y || max.z < min.z);
  }
#endif
} box_t;

/// <summary>
/// Represents a frustum.
/// </summary>
typedef struct frustum {
  double left, right, bottom, top, nearVal, farVal;
} frustum_t;

/// <summary>
/// Represents a 4 x 4 matrix used for transformations in 3-D space.
/// </summary>
typedef struct matrix {
  double m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33;

#if __cplusplus
private:
#if !defined(_MSC_VER) || (_MSC_VER > 1800)
  template <class T> static auto &get_value(T &t, size_t i) {
    switch (i) {
    case 0:
      return t.m00;
    case 1:
      return t.m01;
    case 2:
      return t.m02;
    case 3:
      return t.m03;
    case 4:
      return t.m10;
    case 5:
      return t.m11;
    case 6:
      return t.m12;
    case 7:
      return t.m13;
    case 8:
      return t.m20;
    case 9:
      return t.m21;
    case 10:
      return t.m22;
    case 11:
      return t.m23;
    case 12:
      return t.m30;
    case 13:
      return t.m31;
    case 14:
      return t.m32;
    case 15:
      return t.m33;
    default:
      throw std::out_of_range("index i");
    }
  }
#else
  template <class T> static double get_value(const T &t, size_t i) {
    return get_value(const_cast<T &>(t), i);
  }

  template <class T> static double &get_value(T &t, size_t i) {
    switch (i) {
    case 0:
      return t.m00;
    case 1:
      return t.m01;
    case 2:
      return t.m02;
    case 3:
      return t.m03;
    case 4:
      return t.m10;
    case 5:
      return t.m11;
    case 6:
      return t.m12;
    case 7:
      return t.m13;
    case 8:
      return t.m20;
    case 9:
      return t.m21;
    case 10:
      return t.m22;
    case 11:
      return t.m23;
    case 12:
      return t.m30;
    case 13:
      return t.m31;
    case 14:
      return t.m32;
    case 15:
      return t.m33;
    default:
      throw std::out_of_range("index i");
    }
  }
#endif

public:
  /// <summary>
  /// Allows <see cref="matrix"/> members to be accessed using array notation.
  /// </summary>
  /// <param name="i">Index of the member.</param>
  /// <returns>The value of the <see cref="matrix"/> member.</returns>
  /// <remarks>The index of an <see cref="matrix"/>.mxy is calculated from 4*x+y.</remarks>
  double operator[](size_t i) const {
    return get_value(*this, i);
  }

  /// <summary>
  /// Allows <see cref="matrix"/> members to be accessed using array notation.
  /// </summary>
  /// <param name="i">Index of the member.</param>
  /// <returns>A reference to the <see cref="matrix"/> member.</returns>
  /// <remarks>The index of an <see cref="matrix"/>.mxy is calculated from 4*x+y.</remarks>
  double &operator[](size_t i) {
    return get_value(*this, i);
  }

  /// <summary>
  /// Allows <see cref="matrix"/> member access using matrix notation.
  /// </summary>
  /// <param name="row">The matrix row index</param>
  /// <param name="column">The matrix column index</param>
  /// <returns>The value of the <see cref="matrix"/> member.</returns>
  double operator()(size_t row, size_t column) const {
    return (*this)[row * 4 + column];
  }

  /// <summary>
  /// Allows <see cref="matrix"/> member access using matrix notation.
  /// </summary>
  /// <param name="row">The matrix row index</param>
  /// <param name="column">The matrix column index</param>
  /// <returns>A reference to the <see cref="matrix"/> member.</returns>
  double &operator()(size_t row, size_t column) {
    return (*this)[row * 4 + column];
  }

  /// <summary>
  /// Random access iterator for the matrix.
  /// </summary>
  struct iterator : public std::allocator_traits<std::allocator<double>> {
    using base_type = std::allocator_traits<std::allocator<double>>;
    using reference = base_type::value_type&;
    using difference_type = base_type::difference_type;
    using iterator_category = std::random_access_iterator_tag;

    explicit iterator(struct matrix &m, size_type index = 0) : m_container(m), m_index(index){
    }

    iterator(const iterator &iter) : m_container(iter.m_container), m_index(iter.m_index) {
    }

    ~iterator() {
    }

    iterator &operator=(const iterator &other) {
      m_container = other.m_container;
      m_index = other.m_index;
      return *this;
    }

    bool operator==(const iterator &other) const {
      return std::addressof(m_container) == std::addressof(other.m_container) &&
             m_index == other.m_index;
    }

    bool operator!=(const iterator &other) const {
      return std::addressof(m_container) != std::addressof(other.m_container) ||
             m_index != other.m_index;
    }

    iterator &operator+(difference_type offset) {
      m_index += offset / sizeof(double);
      return *this;
    }

    iterator &operator++() {
      ++m_index;
      return *this;
    }

    reference operator*() NOEXCEPT {
      return m_container[m_index];
    }

    pointer operator->() NOEXCEPT {
      return &m_container[m_index];
    }

  protected:
    struct matrix &m_container;
    size_type m_index;
  };

  /// <summary>
  /// Returns an <see cref="iterator"/> to the beginning.
  /// </summary>
  /// <returns>An <see cref="iterator"/> to the first element of the matrix.</returns>
  iterator begin() {
    return iterator(*this, 0);
  }

  /// <summary>
  /// Returns an <see cref="iterator"/> to the end.
  /// </summary>
  /// <returns>An <see cref="iterator"/> to the element following the last element of the
  /// matrix.</returns>
  iterator end() {
    return iterator(*this, 16);
  }
#endif
} matrix_t;
#endif

/// <summary>
/// Represents a writable string.
/// </summary>
typedef struct {
  char *p;
  size_t length;
} string_t;

/// <summary>
/// Represents a read only string.
/// </summary>
typedef struct {
  const char *p;
  size_t length;
} cstr_t;

/// <summary>
/// Represents an array of <see cref="SiImage_t"/> pointers.
/// </summary>
typedef struct {
  const SiImage_t *p;
  size_t count;
} imagearray_t;
NAVLIB_END_
#if __cplusplus
#include <navlib/navlib_templates.h>
#endif
NAVLIB_BEGIN_

/// <summary>
/// Variant type used to transfer property values in the interface.
/// </summary>
/// <remarks><see cref="value_t"/> can hold one of the types defined in
/// <see cref="propertyType_t"/>.</remarks>
typedef struct value {
  /// <summary>
  /// The <see cref="propertyType_t"/> of the contained value.
  /// </summary>
  propertyType_t type;
  /// <summary>
  /// The contained data value.
  /// </summary>
  union {
    void *p;
    bool_t b;
    long l;
    float f;
    double d;
    point_t point;
    vector_t vector;
    plane_t plane;
    box_t box;
    frustum_t frustum;
    matrix_t matrix;
    const SiActionNodeEx_t *pnode;
    string_t string;
    cstr_t cstr_;
    imagearray_t imagearray;
  };

#if __cplusplus
  /// <summary>
  /// Instantiates an <see cref="auto_type"/> <see cref="value"/>.
  /// </summary>
  /// <remarks>
  /// <see cref="auto_type"/> values can be assigned to but not read. The data assignment also
  /// assigns the type.
  /// </remarks>
  value() : type(auto_type) {
  }
  /// <summary>
  /// Instantiates a <see cref="voidptr_type"/> <see cref="value"/> containing a
  /// <see cref="void"/>*.
  /// </summary>
  /// <param name="aVoidPointer">The <see cref="void"/>* to assign to the <see cref="value"/>
  /// struct.</param>
  value(void *aVoidPointer) : type(voidptr_type), p(aVoidPointer) {
  }
  /// <summary>
  /// Instantiates a <see cref="bool_type"/> <see cref="value"/> containing a <see cref="bool_t"/>.
  /// </summary>
  /// <param name="aBool">The <see cref="bool"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(bool aBool) : type(bool_type), b(static_cast<bool_t>(aBool)) {
  }
  /// <summary>
  /// Instantiates a <see cref="bool_type"/> <see cref="value"/> containing a <see cref="bool_t"/>.
  /// </summary>
  /// <param name="aBool">The <see cref="bool_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(bool_t aBool) : type(bool_type), b(aBool) {
  }
  /// <summary>
  /// Instantiates a <see cref="long_type"/> <see cref="value"/> containing a <see cref="long"/>.
  /// </summary>
  /// <param name="aLong">The <see cref="long"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(long aLong) : type(long_type), l(aLong) {
  }
  /// <summary>
  /// Instantiates a <see cref="float_type"/> <see cref="value"/> containing a <see cref="float"/>.
  /// </summary>
  /// <param name="aFloat">The <see cref="float"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(float aFloat) : type(float_type), f(aFloat) {
  }
  /// <summary>
  /// Instantiates a <see cref="double_type"/> <see cref="value"/> containing a
  /// <see cref="double"/>.
  /// </summary>
  /// <param name="aDouble">The <see cref="double"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(double aDouble) : type(double_type), d(aDouble) {
  }
  /// <summary>
  /// Instantiates a <see cref="point_type"/> <see cref="value"/> containing a
  /// <see cref="point_t"/>.
  /// </summary>
  /// <param name="aPoint">The <see cref="point_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(const point_t &aPoint) : type(point_type), point(aPoint) {
  }
  /// <summary>
  /// Instantiates a <see cref="vector_type"/> <see cref="value"/> containing a
  /// <see cref="vector_t"/>.
  /// </summary>
  /// <param name="aVector">The <see cref="vector_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(const vector_t &aVector) : type(vector_type), vector(aVector) {
  }
  /// <summary>
  /// Instantiates a <see cref="plane_type"/> <see cref="value"/> containing a
  /// <see cref="plane_t"/>.
  /// </summary>
  /// <param name="aPlane">The <see cref="plane_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(const plane_t &aPlane) : type(plane_type), plane(aPlane) {
  }
  /// <summary>
  /// Instantiates a <see cref="box_type"/> <see cref="value"/> containing a <see cref="box_t"/>.
  /// </summary>
  /// <param name="aBox">The <see cref="box_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(const box_t &aBox) : type(box_type), box(aBox) {
  }
  /// <summary>
  /// Instantiates a <see cref="frustum_type"/> <see cref="value"/> containing a
  /// <see cref="frustum_t"/>.
  /// </summary>
  /// <param name="aFrustum">The <see cref="frustum_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(const frustum_t &aFrustum) : type(frustum_type), frustum(aFrustum) {
  }
  /// <summary>
  /// Instantiates a <see cref="matrix_type"/> <see cref="value"/> containing a
  /// <see cref="matrix_t"/>.
  /// </summary>
  /// <param name="aMatrix">The <see cref="matrix_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(const matrix_t &aMatrix) : type(matrix_type), matrix(aMatrix) {
  }
  /// <summary>
  /// Instantiates a <see cref="actionnodeexptr_type"/> <see cref="value"/> containing a
  /// <see cref="SiActionNodeEx_t"/>*.
  /// </summary>
  /// <param name="aNode">The <see cref="SiActionNodeEx_t"/>* to assign to the <see cref="value"/>
  /// struct.</param>
  value(const SiActionNodeEx_t *aNode) : type(actionnodeexptr_type), pnode(aNode) {
  }
  /// <summary>
  /// Instantiates a <see cref="string_type"/> <see cref="value"/> containing a
  /// <see cref="string_t"/>.
  /// </summary>
  /// <param name="aString">The <see cref="string_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(string_t &aString) : type(string_type), string(aString) {
  }
  /// <summary>
  /// Instantiates a <see cref="cstr_type"/> <see cref="value"/> containing a <see cref="cstr_t"/>.
  /// </summary>
  /// <param name="cstr">The <see cref="cstr_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(cstr_t &cstr) : type(cstr_type), cstr_(cstr) {
  }
  /// <summary>
  /// Instantiates a <see cref="string_type"/> <see cref="value"/> containing a
  /// <see cref="string_t"/>.
  /// </summary>
  /// <param name="aString">The <see cref="char"/>* to assign to the <see cref="value"/>
  /// struct.</param>
  /// <param name="length">The length of the string.</param>
  value(char *aString, size_t length) : type(string_type) {
    string.length = length;
    string.p = aString;
  }
  /// <summary>
  /// Instantiates a <see cref="cstr_type"/> <see cref="value"/> containing a <see cref="cstr_t"/>.
  /// </summary>
  /// <param name="aString">The null terminated <see cref="char"/>* to assign to the <see cref="value"/>
  /// struct.</param>
  value(const char *aString) : type(cstr_type) {
    cstr_.p = aString;
    cstr_.length = aString != nullptr ? strlen(aString) + 1 : 0;
  }
  /// <summary>
  /// Instantiates a <see cref="cstr_type"/> <see cref="value"/> containing a <see cref="cstr_t"/>.
  /// </summary>
  /// <param name="aString">The <see cref="std::string"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(const std::string &aString) : type(cstr_type) {
    cstr_.p = aString.c_str();
    cstr_.length = aString.length() + 1;
  }
  /// <summary>
  /// Instantiates an <see cref="imagearray_type"/> <see cref="value"/> containing a
  /// <see cref="imagearray_t"/>.
  /// </summary>
  /// <param name="images">The <see cref="imagearray_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  value(const imagearray_t &images) : type(imagearray_type), imagearray(images) {
  }
  /// <summary>
  /// Instantiates an <see cref="imagearray_type"/> <see cref="value"/> containing a
  /// <see cref="imagearray_t"/>.
  /// </summary>
  /// <param name="images">The <see cref="SiImage_t"/>* to assign to the <see cref="value"/>
  /// struct.</param>
  /// <param name="count">The image count.</param>
  value(const SiImage_t *images, size_t count) : type(imagearray_type) {
    imagearray.count = count;
    imagearray.p = images;
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="bool"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator bool() const {
    return cast_value<bool, const value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="bool"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator bool() {
    return cast_value<bool, value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="bool_t"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator bool_t() const {
    return cast_value<bool_t, const value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="bool_t"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator bool_t() {
    return cast_value<bool_t, value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>An <see cref="int"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator int() const {
    return cast_value<int, const value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>An <see cref="int"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator int() {
    return cast_value<int, value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="long"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator long() const {
    return cast_value<long, const value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="long"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator long() {
    return cast_value<long, value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="float"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator float() const {
    return cast_value<float, const value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="float"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator float() {
    return cast_value<float, value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="double"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator double() const {
    return cast_value<double, const value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="double"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator double() {
    return cast_value<double, value>()(*this);
  }

  /// <summary>
  /// Template to enable implicit T& conversion.
  /// </summary>
  template <typename T> operator T &() {
    return conversion_operator<T &>();
  }

  /// <summary>
  /// Template to enable implicit T const & conversion.
  /// </summary>
  template <typename T> operator T const &() const {
    return cast_value<T &, const value>()(*this);
  }

  /// <summary>
  /// Template function to convert a value with type checking.
  /// </summary>
  /// <returns>The inner member variable of the union corresponding to the <see cref="property_t"/>
  /// type.</returns>
  /// <exception cref="conversion_error">Invalid conversion attempted.</exception>
  template <typename T> T conversion_operator() {
    if (std::is_reference<T>::value) {
      if (type == auto_type) {
        type = property_type<T>::type;
      }
    }

    if (type != property_type<T>::type) {
      if ((type == float_type && property_type<T>::type == double_type) ||
          (type == double_type && property_type<T>::type == float_type)) {
        type = property_type<T>::type;
      } else {
        throw_conversion_error(property_type<T>::name);
      }
    }

    return value_member<T, value, typename remove_cvref<T>::type>()(*this);
  }

  /// <summary>
  /// Template function to convert a value with type checking.
  /// </summary>
  /// <returns>The inner member variable of the union corresponding to the <see cref="property_t"/>
  /// type.</returns>
  /// <exception cref="conversion_error">Invalid conversion attempted.</exception>
  template <typename T> T conversion_operator() const {
    if (type != property_type<T>::type) {
      throw_conversion_error(property_type<T>::name);
    }

    return value_member<T, const value, typename remove_cvref<T>::type>()(*this);
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="void"/>*& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator void *&() {
    return conversion_operator<void *&>();
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="point_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator point_t &() {
    return conversion_operator<point_t &>();
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="point_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const point_t &() const {
    return conversion_operator<const point_t &>();
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="vector_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator vector_t &() {
    return conversion_operator<vector_t &>();
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="vector_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const vector_t &() const {
    return conversion_operator<const vector_t &>();
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="plane_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator plane_t &() {
    return conversion_operator<plane_t &>();
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="plane_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const plane_t &() const {
    return conversion_operator<const plane_t &>();
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="box_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator box_t &() {
    return conversion_operator<box_t &>();
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="box_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const box_t &() const {
    return conversion_operator<const box_t &>();
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="frustum_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator frustum_t &() {
    return conversion_operator<frustum_t &>();
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="frustum_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const frustum_t &() const {
    return conversion_operator<const frustum_t &>();
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="matrix_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator matrix_t &() {
    return conversion_operator<matrix_t &>();
  }

  /// <summary>
  /// Conversion.
  /// </summary>
  /// <returns>A <see cref="matrix_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const matrix_t &() const {
    return conversion_operator<const matrix_t &>();
  }

  typedef const SiActionNodeEx_t *cpSiActionNodeEx_t;

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="cpSiActionNodeEx_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator cpSiActionNodeEx_t &() {
    return conversion_operator<cpSiActionNodeEx_t &>();
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="imagearray_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator imagearray_t &() {
    return conversion_operator<imagearray_t &>();
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="imagearray_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const imagearray_t &() const {
    return conversion_operator<const imagearray_t &>();
  }

  /// <summary>
  /// Throws a <see cref="conversion_error"/> exception.
  /// </summary>
  /// <param name="target_type">The attempted <see cref="property_t"/> type conversion.</param>
  /// <exception cref="conversion_error">Invalid conversion attempted.</exception>
  void throw_conversion_error(const std::string &target_type) const {
    std::ostringstream stream;
    stream << "Invalid conversion from value type " << type << " to " << target_type.c_str();
    throw NAVLIB_ conversion_error(stream.str().c_str());
  }

  //////////////////////////////////////////////////////////////////
  // Strings - treat string_t and cstr_t the same to avoid confusion

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="char"/> const* representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const char *() const {
    if (type != cstr_type && type != string_type) {
      throw_conversion_error("string_type");
    }
    return type == string_type ? string.p : cstr_.p;
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="string_t"/> const* representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const string_t *() const {
    if (type != cstr_type && type != string_type) {
      throw_conversion_error("cstr_type");
    }
    return &string;
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="string_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const string_t &() const {
    if (type != cstr_type && type != string_type) {
      throw_conversion_error("cstr_type");
    }
    return string;
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="string_t"/>& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator string_t &() {
    if (type == auto_type) {
      type = string_type;
    }
    if (type != cstr_type && type != string_type) {
      throw_conversion_error("string_type");
    }
    return string;
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="cstr_t"/> const* representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const cstr_t *() const {
    if (type != cstr_type && type != string_type) {
      throw_conversion_error("cstr_type");
    }
    return &cstr_;
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="cstr_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const cstr_t &() const {
    if (type != cstr_type && type != string_type) {
      throw_conversion_error("cstr_type");
    }
    return cstr_;
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="cstr_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator cstr_t &() {
    if (type == auto_type) {
      type = cstr_type;
    }
    if (type != cstr_type && type != string_type) {
      throw_conversion_error("cstr_type");
    }
    return cstr_;
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="SiActionNodeEx_t"/> const* representation of the internal
  /// value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is
  /// not possible.</exception>
  operator const SiActionNodeEx_t *() const {
    if (type != actionnodeexptr_type) {
      throw_conversion_error("actionnodeexptr_type");
    }
    return pnode;
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="SiActionNodeEx_t"/> const& representation of the internal
  /// value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is
  /// not possible.</exception>
  operator const SiActionNodeEx_t &() const {
    if (type != actionnodeexptr_type) {
      throw_conversion_error("actionnodeexptr_type");
    }
    return *pnode;
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="imagearray_t"/> const* representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const imagearray_t *() const {
    if (type != imagearray_type) {
      throw_conversion_error("imagearray_type");
    }
    return &imagearray;
  }

  // Assignment operators

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="aVoidPointer">The <see cref="void"/>* to assign to the <see cref="value"/> struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="void"/>* to the internal value and sets
  /// <see cref="value.type"/> to <see cref="voidptr_type"/>.</remarks>
  struct value &operator=(void *aVoidPointer) {
    p = aVoidPointer;
    type = voidptr_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="aBool">The <see cref="bool"/> to assign to the <see cref="value"/> struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="bool"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="bool_type"/>.</remarks>
  struct value &operator=(bool aBool) {
    b = static_cast<bool_t>(aBool);
    type = bool_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="aLong">The <see cref="long"/> to assign to the <see cref="value"/> struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="long"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="long_type"/>.</remarks>
  struct value &operator=(long aLong) {
    l = aLong;
    type = long_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="aFloat">The <see cref="float"/> to assign to the <see cref="value"/> struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="float"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="float_type"/>.</remarks>
  struct value &operator=(float aFloat) {
    f = aFloat;
    type = float_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="aDouble">The <see cref="double"/> to assign to the <see cref="value"/> struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="double"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="double_type"/>.</remarks>
  struct value &operator=(double aDouble) {
    d = aDouble;
    type = double_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="aPoint">The <see cref="point_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="point_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="point_type"/>.
  /// </remarks>
  struct value &operator=(const point_t &aPoint) {
    point = aPoint;
    type = point_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="aPlane">The <see cref="plane_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="plane_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="plane_type"/>.
  /// </remarks>
  struct value &operator=(const plane_t &aPlane) {
    plane = aPlane;
    type = plane_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="aBox">The <see cref="box_t"/> to assign to the <see cref="value"/> struct.
  /// </param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="box_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="box_type"/>.</remarks>
  struct value &operator=(const box_t &aBox) {
    box = aBox;
    type = box_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="aFrustum">The <see cref="frustum_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="frustum_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="frustum_type"/>.</remarks>
  struct value &operator=(const frustum_t &aFrustum) {
    frustum = aFrustum;
    type = frustum_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="aMatrix">The <see cref="matrix_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="matrix_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="matrix_type"/>.</remarks>
  struct value &operator=(const matrix_t &aMatrix) {
    matrix = aMatrix;
    type = matrix_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="aNode">The <see cref="SiActionNodeEx_t"/> const* to assign to the
  /// <see cref="value"/> struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="SiActionNodeEx_t"/>* to the internal value and sets
  /// <see cref="value.type"/> to <see cref="actionnodeexptr_type"/>.</remarks>
  struct value &operator=(const SiActionNodeEx_t *aNode) {
    pnode = aNode;
    type = actionnodeexptr_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="aString">The <see cref="string_t"/> const& to assign to the <see cref="value"/>
  /// struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="string_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="string_type"/>.</remarks>
  struct value &operator=(const string_t &aString) {
    string = aString;
    type = string_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="cstr">The <see cref="cstr_t"/> const& to assign to the <see cref="value"/>
  /// struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="cstr_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="cstr_type"/>.</remarks>
  struct value &operator=(const cstr_t &cstr) {
    cstr_ = cstr;
    type = cstr_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="images">The <see cref="imagearray_t"/> const& to assign to the
  /// <see cref="value"/> struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="imagearray_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="imagearray_type"/>.</remarks>
  struct value &operator=(const imagearray_t &images) {
    imagearray = images;
    type = imagearray_type;
    return *this;
  }
#endif // __cplusplus
} value_t;

/// <summary>
/// The prototype of the function the navlib invokes to retrieve the value of a property.
/// </summary>
/// <param name="param">Value passed in <see cref="accessor_t.param"/> in <see cref="NlCreate"/>.
/// </param>
/// <param name="name">Value passed in <see cref="accessor_t.name"/> in <see cref="NlCreate"/>.
/// </param>
/// <param name="value">Pointer to a <see cref="value_t"/> for the value of the property.</param>
/// <returns>A navlib result code. <seealso cref="make_result_code"/>.</returns>
/// <remarks>When the application services the function call it sets the current value and the type
/// of the property into the <see cref="value_t"/> variant.</remarks>
typedef long(__cdecl *fnGetProperty_t)(const param_t param, const property_t name, value_t *value);

/// <summary>
/// The prototype of the function the navlib invokes to set the value of a property.
/// </summary>
/// <param name="param">Value passed in <see cref="accessor_t.param"/> in <see cref="NlCreate"/>.
/// </param>
/// <param name="name">Value passed in <see cref="accessor_t.name"/> in <see cref="NlCreate"/>.
/// </param>
/// <param name="value">A <see cref="value_t"/> containing the value of the property.</param>
/// <returns>A navlib result code. <seealso cref="make_result_code"/>.</returns>
/// <remarks>When the application services the function call it sets its current value of the
/// property from the <see cref="value_t"/> variant.</remarks>
typedef long(__cdecl *fnSetProperty_t)(const param_t param, const property_t name,
                                       const value_t *value);

/// <summary>
/// Represents a client property accessor and mutator.
/// </summary>
/// <remarks>
/// <para>
/// The application passes an array of accessor_t structures in <see cref="NlCreate"/> to define the
/// interface that the navlib can use to query or apply changes to a client property. Depending on
/// the user/3d mouse input the navlib will query the application for the values of properties
/// needed to fulfill the requirements of the navigation model the user has invoked and when a new
/// camera position has been calculated set the corresponding properties.
/// </para>
/// <para>
/// To retrieve the value of a property the navlib will call the fnGet member of the property's
/// accessor_t, passing in param, the name of the property as well as a pointer to a value_t.
/// Similarly, to apply a change to a property the navlib will invoke the fnSet member of the
/// property's accessor_t, passing in the param, property name and a pointer to a value_t.
/// </para>
/// <para>
/// If either of fnSet or fnGet is null, then the property is respectively read- or write-only.
/// </para>
/// </remarks>
typedef struct tagAccessor {
  /// <summary>
  /// The name of the property
  /// </summary>
  property_t name;
  /// <summary>
  /// The function the navlib calls to retrieve the property's value from the client application.
  /// </summary>
  fnGetProperty_t fnGet;
  /// <summary>
  /// The function the navlib calls to set the property's value in the client application.
  /// </summary>
  fnSetProperty_t fnSet;
  /// <summary>
  /// The value to pass to the <see cref="fnGetProperty_t"/> and <see cref="fnSetProperty_t"/>
  /// functions as the first parameter.
  /// </summary>
  param_t param;
} accessor_t;

/// <summary>
/// Type used for navlib handles.
/// </summary>
typedef uint64_t nlHandle_t;

/// <summary>
/// Configuration options
/// </summary>
typedef enum nlOptions {
  /// <summary>
  /// No options.
  /// </summary>
  none = 0,
  /// <summary>
  /// Use non-queued message passing.
  /// </summary>
  nonqueued_messages = 1,
  /// <summary>
  /// Matrices are stored in row major order.
  /// </summary>
  /// <remarks>
  /// The default is column major order.
  /// </remarks>
  row_major_order = 2,
  /// <summary>
  /// disable any popup notifications.
  /// </summary>
  no_ui = 4,
} nlOptions_t;

/// <summary>
/// Structure for navlib initialization options passed in the <see cref="NlCreate"/>.
/// </summary>
typedef struct tagNlCreateOptions {
  /// <summary>
  /// Size of this structure.
  /// </summary>
  uint32_t size;
  /// <summary>
  /// true is to use multi-threading, false for single-threaded.
  /// </summary>
  /// <remarks>The default is false (single-threaded).</remarks>
#if __cplusplus
  bool bMultiThreaded;
#else
  int8_t bMultiThreaded;
#endif
  /// <inheritdoc/>
  nlOptions_t options;
} nlCreateOptions_t;

#ifndef TDX_ALWAYS_INLINE
#ifdef _MSC_VER
#define TDX_ALWAYS_INLINE __forceinline
#else
#define TDX_ALWAYS_INLINE __attribute__((always_inline)) inline
#endif
#endif
/// <summary>
/// Makes a result code in the navlib facility.
/// </summary>
/// <param name="x">The error code.</param>
/// <returns>A result code in the navlib facility.</returns>
TDX_ALWAYS_INLINE long make_result_code(unsigned long x) {
#if __cplusplus
  return static_cast<long>(x) <= 0
             ? static_cast<long>(x)
             : static_cast<long>(((x)&0x0000FFFF) | (FACILITY_NAVLIB << 16) | 0x80000000);
#else
  return (long)x <= 0 ? (long)x : (long)(((x)&0x0000FFFF) | (FACILITY_NAVLIB << 16) | 0x80000000);
#endif
}

#if __cplusplus
/// <summary>
/// Contains error codes used by the navlib.
/// </summary>
namespace navlib_errc {
#endif
/// <summary>
/// Error codes used in the navlib facility.
/// </summary>
enum navlib_errc_t {
  /// <summary>
  /// Error.
  /// </summary>
  error = EIO,
  /// <summary>
  /// Already connected.
  /// </summary>
  already_connected = EISCONN,
  /// <summary>
  /// The function is not supported.
  /// </summary>
  function_not_supported = ENOSYS,
  /// <summary>
  /// The argument is invlaid.
  /// </summary>
  invalid_argument = EINVAL,
  /// <summary>
  /// No data is available.
  /// </summary>
  no_data_available = ENODATA,
  /// <summary>
  /// Not enough memory.
  /// </summary>
  not_enough_memory = ENOMEM,
  /// <summary>
  /// The buffer is too small.
  /// </summary>
  insufficient_buffer = ENOBUFS,
  /// <summary>
  /// THe operation is invalid in the current context.
  /// </summary>
  invalid_operation = EPERM,
  /// <summary>
  /// Sorry, not allowed.
  /// </summary>
  permission_denied = EACCES,
  /// <summary>
  /// The property does not exists.
  /// </summary>
  property_not_found = 0x201,
  /// <summary>
  /// The function does not exist.
  /// </summary>
  invalid_function = 0x202,
};
#if __cplusplus
} // namespace navlib::navlib_errc::
#endif

NAVLIB_END_

#endif /* NAVLIB_TYPES_H_INCLUDED_ */
