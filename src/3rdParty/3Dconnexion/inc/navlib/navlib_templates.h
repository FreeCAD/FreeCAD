#ifndef NAVLIB_TEMPLATES_H_INCLUDED_
#define NAVLIB_TEMPLATES_H_INCLUDED_
// <copyright file="navlib_templates.h" company="3Dconnexion">
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
// $Id: navlib_templates.h 19940 2023-01-25 07:17:44Z mbonk $
//
// 06/17/20 MSB Changed condition evaluation away from boolean macros to avoid C4668
// 01/23/14 MSB Initial design
// </history>
// <description>
// *************************************************************************************************
// File Description
//
// This header file defines the templates used in the 3dconnexion interface.
//
// *************************************************************************************************
// </description>
#if (defined _MSC_VER && _MSC_VER < 1900)
#define CONSTEXPR const
#else
#define CONSTEXPR constexpr
#define has_constexpr
#endif

namespace navlib {
template <class T> struct property_type_v {
  static CONSTEXPR propertyType_t type = unknown_type;
#ifdef has_constexpr
  static constexpr char const *name = "unknown_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
template <class T> const char *property_type_v<T>::name = "unknown_type";
#endif

template <> struct property_type_v<void *> {
  static CONSTEXPR propertyType_t type = voidptr_type;
#ifdef has_constexpr
  static constexpr char const *name = "voidptr_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const char *property_type_v<void *>::name = "voidptr_type";
#endif

template <> struct property_type_v<bool_t> {
  static CONSTEXPR propertyType_t type = bool_type;
#ifdef has_constexpr
  static constexpr char const *name = "bool_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const char *property_type_v<bool_t>::name = "bool_type";
#endif

template <> struct property_type_v<long> {
  static CONSTEXPR propertyType_t type = long_type;
#ifdef has_constexpr
  static constexpr char const *name = "long_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const char *property_type_v<long>::name = "long_type";
#endif

template <> struct property_type_v<float> {
  static CONSTEXPR propertyType_t type = float_type;
#ifdef has_constexpr
  static constexpr char const *name = "float_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const char *property_type_v<float>::name = "float_type";
#endif

template <> struct property_type_v<double> {
  static CONSTEXPR propertyType_t type = double_type;
#ifdef has_constexpr
  static constexpr char const *name = "double_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const char *property_type_v<double>::name = "double_type";
#endif

template <> struct property_type_v<point_t> {
  static CONSTEXPR propertyType_t type = point_type;
#ifdef has_constexpr
  static constexpr char const *name = "point_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const char *property_type_v<point_t>::name = "point_type";
#endif

template <> struct property_type_v<vector_t> {
  static CONSTEXPR propertyType_t type = vector_type;
#ifdef has_constexpr
  static constexpr char const *name = "vector_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const char *property_type_v<vector_t>::name = "vector_type";
#endif

template <> struct property_type_v<plane_t> {
  static CONSTEXPR propertyType_t type = plane_type;
#ifdef has_constexpr
  static constexpr char const *name = "plane_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const char *property_type_v<plane_t>::name = "plane_type";
#endif

template <> struct property_type_v<box_t> {
  static CONSTEXPR propertyType_t type = box_type;
#ifdef has_constexpr
  static constexpr char const *name = "box_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const char *property_type_v<box_t>::name = "box_type";
#endif

template <> struct property_type_v<frustum_t> {
  static CONSTEXPR propertyType_t type = frustum_type;
#ifdef has_constexpr
  static constexpr char const *name = "frustum_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const char *property_type_v<frustum_t>::name = "frustum_type";
#endif

template <> struct property_type_v<matrix_t> {
  static CONSTEXPR propertyType_t type = matrix_type;
#ifdef has_constexpr
  static constexpr char const *name = "matrix_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const char *property_type_v<matrix_t>::name = "matrix_type";
#endif

template <> struct property_type_v<SiActionNodeEx_t *> {
  static CONSTEXPR propertyType_t type = actionnodeexptr_type;
#ifdef has_constexpr
  static constexpr char const *name = "actionnodeexptr_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const
    char *property_type_v<SiActionNodeEx_t *>::name = "actionnodeexptr_type";
#endif

template <> struct property_type_v<string_t> {
  static CONSTEXPR propertyType_t type = string_type;
#ifdef has_constexpr
  static constexpr char const *name = "string_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const char *property_type_v<string_t>::name = "string_type";
#endif

template <> struct property_type_v<cstr_t> {
  static CONSTEXPR propertyType_t type = cstr_type;
#ifdef has_constexpr
  static constexpr char const *name = "cstr_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const char *property_type_v<cstr_t>::name = "cstr_type";
#endif

template <> struct property_type_v<imagearray_t> {
  static CONSTEXPR propertyType_t type = imagearray_type;
#ifdef has_constexpr
  static constexpr char const *name = "imagearray_type";
#else
  static const char *name;
#endif
};
#ifndef has_constexpr
__declspec(selectany) const char *property_type_v<imagearray_t>::name = "imagearray_type";
#endif

template <class T> struct remove_cvref {
  typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
};

// navlib property type from variable type
template <class T> struct property_type : property_type_v<typename remove_cvref<T>::type> {
  typedef property_type_v<typename remove_cvref<T>::type> base_type;
  typedef propertyType_t value_type;
  CONSTEXPR value_type operator()() const {
    return base_type::value;
  }
#ifdef has_constexpr
  constexpr operator value_type() const {
    return base_type::value;
  }
  constexpr operator char *() const {
    return base_type::name;
  }
#else
  operator const value_type() const {
    return base_type::value;
  }
  operator const char *() const {
    return base_type::name;
  }
#endif
};

template <typename T, typename V, typename TargetType> struct value_member {}; // primary template

// specialization for void*
template <typename T, typename V> struct value_member<T, V, void *> {
  T operator()(V &v) {
    return v.p;
  }
};

// specialization for void**
template <typename T, typename V> struct value_member<T, V, void **> {
  T operator()(V &v) {
    return &v.p;
  }
};

// specialization for bool
template <typename T, typename V> struct value_member<T, V, bool> {
  T operator()(V &v) {
    return v.b != 0;
  }
};

// specialization for bool_t
template <typename T, typename V> struct value_member<T, V, bool_t> {
  T operator()(V &v) {
    return v.b;
  }
};

// specialization for bool_t*
template <typename T, typename V> struct value_member<T, V, bool_t *> {
  T operator()(V &v) {
    return &v.b;
  }
};

// specialization for int
template <typename T, typename V> struct value_member<T, V, int> {
  T operator()(V &v) {
    return v.l;
  }
};

// specialization for long
template <typename T, typename V> struct value_member<T, V, long> {
  T operator()(V &v) {
    return v.l;
  }
};

// specialization for long*
template <typename T, typename V> struct value_member<T, V, long *> {
  T operator()(V &v) {
    return &v.l;
  }
};

// specialization for float
template <typename T, typename V> struct value_member<T, V, float> {
  T operator()(V &v) {
    return v.f;
  }
};

// specialization for float*
template <typename T, typename V> struct value_member<T, V, float *> {
  T operator()(V &v) {
    return &v.f;
  }
};

// specialization for double
template <typename T, typename V> struct value_member<T, V, double> {
  T operator()(V &v) {
    return v.d;
  }
};

// specialization for double*
template <typename T, typename V> struct value_member<T, V, double *> {
  T operator()(V &v) {
    return &v.d;
  }
};

// specialization for point_t
template <typename T, typename V> struct value_member<T, V, point_t> {
  T operator()(V &v) {
    return v.point;
  }
};

// specialization for point_t*
template <typename T, typename V> struct value_member<T, V, point_t *> {
  T operator()(V &v) {
    return &v.point;
  }
};

// specialization for vector_t
template <typename T, typename V> struct value_member<T, V, vector_t> {
  T operator()(V &v) {
    return v.vector;
  }
};

// specialization for vector_t*
template <typename T, typename V> struct value_member<T, V, vector_t *> {
  T operator()(V &v) {
    return &v.vector;
  }
};

// specialization for plane_t
template <typename T, typename V> struct value_member<T, V, plane_t> {
  T operator()(V &v) {
    return v.plane;
  }
};

// specialization for plane_t*
template <typename T, typename V> struct value_member<T, V, plane_t *> {
  T operator()(V &v) {
    return &v.plane;
  }
};

// specialization for box_t
template <typename T, typename V> struct value_member<T, V, box_t> {
  T operator()(V &v) {
    return v.box;
  }
};

// specialization for box_t*
template <typename T, typename V> struct value_member<T, V, box_t *> {
  T operator()(V &v) {
    return &v.box;
  }
};

// specialization for frustum_t
template <typename T, typename V> struct value_member<T, V, frustum_t> {
  T operator()(V &v) {
    return v.frustum;
  }
};

// specialization for frustum_t*
template <typename T, typename V> struct value_member<T, V, frustum_t *> {
  T operator()(V &v) {
    return &v.frustum;
  }
};

// specialization for matrix_t
template <typename T, typename V> struct value_member<T, V, matrix_t> {
  T operator()(V &v) {
    return v.matrix;
  }
};

// specialization for matrix_t*
template <typename T, typename V> struct value_member<T, V, matrix_t *> {
  T operator()(V &v) {
    return &v.matrix;
  }
};

// specialization for SiActionNodeEx_t*
template <typename T, typename V> struct value_member<T, V, const SiActionNodeEx_t *> {
  T operator()(V &v) {
    return v.pnode;
  }
};

// specialization for SiActionNodeEx_t**
template <typename T, typename V> struct value_member<T, V, SiActionNodeEx_t **> {
  T operator()(V &v) {
    return &v.pnode;
  }
};

// specialization for string_t
template <typename T, typename V> struct value_member<T, V, string_t> {
  T operator()(V &v) {
    return v.string;
  }
};

// specialization for string_t*
template <typename T, typename V> struct value_member<T, V, string_t *> {
  T operator()(V &v) {
    return &v.string;
  }
};

// specialization for cstr_t
template <typename T, typename V> struct value_member<T, V, cstr_t> {
  T operator()(V &v) {
    return v.cstr_;
  }
};

// specialization for cstr_t*
template <typename T, typename V> struct value_member<T, V, cstr_t *> {
  T operator()(V &v) {
    return &v.cstr_;
  }
};

// specialization for imagearray_t
template <typename T, typename V> struct value_member<T, V, imagearray_t> {
  T operator()(V &v) {
    return v.imagearray;
  }
};

// specialization for imagearray_t*
template <typename T, typename V> struct value_member<T, V, imagearray_t *> {
  T operator()(V &v) {
    return &v.imagearray;
  }
};

template <typename T, class V> struct cast_value {
  T operator()(V &value) {
    switch (value.type) {
    case bool_type:
      return static_cast<T>(value.b);

    case long_type:
      return static_cast<T>(value.l);

    case float_type:
      return static_cast<T>(value.f);

    case double_type:
      return static_cast<T>(value.d);

    default:
      return static_cast<T>(0);
    }
  }
};

// Specialization for bool
template <class V> struct cast_value<bool, V> {
  bool operator()(V &value) {
    switch (value.type) {
    case bool_type:
      return value.b != 0;

    case long_type:
      return value.l != 0;

    case float_type:
      return value.f != 0.0f;

    case double_type:
      return value.d != 0.0;

    default:
      return false;
    }
  }
};
} // namespace navlib
#endif      /* NAVLIB_TEMPLATES_H_INCLUDED_ */
