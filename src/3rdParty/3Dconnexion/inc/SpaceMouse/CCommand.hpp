#ifndef CCommand_HPP_INCLUDED
#define CCommand_HPP_INCLUDED
// <copyright file="CCommand.hpp" company="3Dconnexion">
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
// $Id: CCommand.hpp 16056 2019-04-10 13:42:31Z mbonk $
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
/// The <see cref="CCommand"/> class implements the application command node.
/// </summary>
class CCommand : public CCommandTreeNode {
  typedef CCommandTreeNode base_type;

public:
  CCommand() {
  }

  explicit CCommand(std::string id, std::string name, std::string description)
      : base_type(std::move(id), std::move(name), std::move(description),
                  SiActionNodeType_t::SI_ACTION_NODE) {
  }
  explicit CCommand(std::string id, std::string name)
      : base_type(std::move(id), std::move(name), SiActionNodeType_t::SI_ACTION_NODE) {
  }
#if defined(_MSC_VER) && _MSC_VER < 1900
  CCommand(CCommand &&other) : base_type(std::forward<base_type>(other)) {
  }
  CCommand &operator=(CCommand &&other) {
    base_type::operator=(std::forward<base_type>(other));
    return *this;
  }
#else
  CCommand(CCommand &&) = default;
  CCommand &operator=(CCommand &&) = default;
#endif
};
} // namespace SpaceMouse
} // namespace TDx

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#endif // CCommand_HPP_INCLUDED
