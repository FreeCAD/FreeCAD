#ifndef CCategory_HPP_INCLUDED
#define CCategory_HPP_INCLUDED
// <copyright file="CCategory.hpp" company="3Dconnexion">
// ------------------------------------------------------------------------------------------------
// This file is part of the FreeCAD CAx development system.
//
// Copyright (c) 2014-2023 3Dconnexion.
//
// This source code is released under the GNU Library General Public License, (see "LICENSE").
// ------------------------------------------------------------------------------------------------
// </copyright>
// <history>
// ************************************************************************************************
// File History
//
// $Id: CCategory.hpp 16056 2019-04-10 13:42:31Z mbonk $
//
// </history>
#include <SpaceMouse/CCommandTreeNode.hpp>

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif

namespace TDx {
/// <summary>
/// Contains types used for programming the SpaceMouse.
/// </summary>
namespace SpaceMouse {
/// <summary>
/// The helper class implements the <see cref="SiActionNodeType_t::SI_CATEGORY_NODE"/> node type.
/// </summary>
class CCategory : public CCommandTreeNode {
  typedef CCommandTreeNode base_type;

public:
  CCategory() {
  }

  explicit CCategory(std::string id, std::string name)
      : base_type(std::move(id), std::move(name), SiActionNodeType_t::SI_CATEGORY_NODE) {
  }

#if defined(_MSC_VER) && _MSC_VER < 1900
  CCategory(CCategory &&other) : base_type(std::forward<base_type>(other)) {
  }
  CCategory &operator=(CCategory &&other) {
    base_type::operator=(std::forward<base_type>(other));
    return *this;
  }
#else
  CCategory(CCategory &&) = default;
  CCategory &operator=(CCategory &&) = default;
#endif
};
} // namespace SpaceMouse
} // namespace TDx

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#endif // CCategory_HPP_INCLUDED
