#ifndef CCommandSet_HPP_INCLUDED
#define CCommandSet_HPP_INCLUDED
// <copyright file="CCommandSet.hpp" company="3Dconnexion">
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
// $Id: CCommandSet.hpp 16056 2019-04-10 13:42:31Z mbonk $
//
// </history>
#include <SpaceMouse/CCommandTreeNode.hpp>

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif

namespace TDx {
namespace SpaceMouse {
/// <summary>
/// The helper class implements the <see cref="SiActionNodeType_t::SI_ACTIONSET_NODE"/> node type.
/// </summary>
class CCommandSet : public CCommandTreeNode {
  typedef CCommandTreeNode base_type;

public:
  CCommandSet() {
  }

  explicit CCommandSet(std::string id, std::string name)
      : base_type(std::move(id), std::move(name), SiActionNodeType_t::SI_ACTIONSET_NODE) {
  }
#if defined(_MSC_VER) && _MSC_VER < 1900
  CCommandSet(CCommandSet &&other) : base_type(std::forward<base_type>(other)) {
  }
  CCommandSet &operator=(CCommandSet &&other) {
    base_type::operator=(std::forward<base_type>(other));
    return *this;
  }
#else
  CCommandSet(CCommandSet &&) = default;
  CCommandSet &operator=(CCommandSet &&) = default;
#endif
};
} // namespace SpaceMouse
} // namespace TDx

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#endif // CCommandSet_HPP_INCLUDED
