#ifndef CActionNode_HPP_INCLUDED
#define CActionNode_HPP_INCLUDED
// <copyright file="CActionNode.hpp" company="3Dconnexion">
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
// $Id: CActionNode.hpp 16051 2019-04-09 11:29:53Z mbonk $
//
// </history>

// navlib
#include <navlib/navlib_defines.h>

// 3dxware
#include <siappcmd_types.h>

// stdlib
#include <memory>
#include <string>
#include <vector>

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

/// <summary>
/// The TDx namespace provides support for the types used in the 3DxWare interface.
/// </summary>
namespace TDx {
/// <summary>
/// Wrapper class for the <see cref="SiActionNodeEx_t"/> structure.
/// </summary>
class CActionNode : private SiActionNodeEx_t {
  typedef SiActionNodeEx_t base_type;

public:
  /// <summary>
  /// Initializes a new instance of the <see cref="CActionNode"/> class.
  /// </summary>
#if defined(_MSC_VER) && _MSC_VER < 1800
  CActionNode() : base_type() {
    base_type::size = sizeof(base_type);
  }
#else
  CActionNode()
      : base_type({sizeof(base_type), SI_ACTIONSET_NODE, nullptr, nullptr,
                  nullptr, nullptr, nullptr}) {
  }
#endif

  /// <summary>
  /// Constructor a <see cref="CActionNode"/> with a label.
  /// </summary>
  /// <param name="nodeId">The unique node identifier.</param>
  /// <param name="text">Text to display to the user in the user interface.</param>
  /// <param name="nodeType">The <see cref="SiActionNodeType_t"/> of the node.</param>
  explicit CActionNode(std::string nodeId, std::string text, SiActionNodeType_t nodeType)
#if defined(_MSC_VER) && _MSC_VER < 1800
      : base_type(), m_id(std::move(nodeId)), m_label(std::move(text)) {
    base_type::size = sizeof(base_type);
    base_type::type = nodeType;
#else
      : base_type({sizeof(base_type), nodeType, nullptr, nullptr, nullptr, nullptr, nullptr}),
        m_id(std::move(nodeId)), m_label(std::move(text)) {
#endif

    if (!m_id.empty()) {
      base_type::id = m_id.c_str();
    }

    if (!m_label.empty()) {
      base_type::label = m_label.c_str();
    }
  }

  /// <summary>
  /// Constructor a <see cref="CActionNode"/> with a label and a description for a tooltip.
  /// </summary>
  /// <param name="nodeId">The unique node identifier</param>
  /// <param name="text">Text to display to the user in the user interface.</param>
  /// <param name="tooltip">Text to display in a tooltip.</param>
  /// <param name="nodeType">The <see cref="SiActionNodeType_t"/> of the node.</param>
  explicit CActionNode(std::string nodeId, std::string text, std::string tooltip,
                       SiActionNodeType_t nodeType)
#if defined(_MSC_VER) && _MSC_VER < 1800
      : base_type(), m_id(std::move(nodeId)), m_label(std::move(text)),
        m_description(std::move(tooltip)) {
    base_type::size = sizeof(base_type);
    base_type::type = nodeType;
#else
      : base_type({sizeof(base_type), nodeType, nullptr, nullptr, nullptr, nullptr, nullptr}),
        m_id(std::move(nodeId)), m_label(std::move(text)),
        m_description(std::move(tooltip)) {
#endif

    if (!m_id.empty()) {
      base_type::id = m_id.c_str();
    }

    if (!m_label.empty()) {
      base_type::label = m_label.c_str();
    }

    if (!m_description.empty()) {
      base_type::description = m_description.c_str();
    }
  }

  virtual ~CActionNode() {
    Tidy();
  }

  /// <summary>
  /// Move constructor
  /// </summary>
  /// <param name="other">The <see cref="CActionNode"/> to use for construction.</param>
  CActionNode(CActionNode &&other) NOEXCEPT
      : base_type(other), m_id(std::move(other.m_id)), m_label(std::move(other.m_label)),
        m_description(std::move(other.m_description)) {
            base_type zero = {sizeof(base_type), SI_ACTIONSET_NODE, nullptr, nullptr, nullptr,
                              nullptr, nullptr};
    static_cast<base_type &>(other) = zero;

    base_type::id = !m_id.empty() ? m_id.c_str() : nullptr;
    base_type::label = !m_label.empty() ? m_label.c_str() : nullptr;
    base_type::description = !m_description.empty() ? m_description.c_str() : nullptr;
  }

  /// <summary>
  /// Move assignment
  /// </summary>
  /// <param name="other">The <see cref="CActionNode"/> to use for construction.</param>
  CActionNode &operator=(CActionNode &&other) NOEXCEPT {
    static_cast<base_type &>(*this) = static_cast<base_type>(other);
    m_id = std::move(other.m_id);
    m_label = std::move(other.m_label);
    m_description = std::move(other.m_description);

    base_type zero = {sizeof(base_type), SI_ACTIONSET_NODE, nullptr, nullptr,
                      nullptr, nullptr, nullptr};
    static_cast<base_type &>(other) = zero;

    base_type::id = !m_id.empty() ? m_id.c_str() : nullptr;
    base_type::label = !m_label.empty() ? m_label.c_str() : nullptr;
    base_type::description = !m_description.empty() ? m_description.c_str() : nullptr;

    return *this;
  }

#if !defined(_MSC_VER) || _MSC_VER > 1700
  CActionNode(const CActionNode &) = delete;
  CActionNode &operator=(const CActionNode &) = delete;
#else
private:
  CActionNode(const CActionNode &);
  CActionNode &operator=(const CActionNode &);
#endif

public:
  /// <summary>
  /// Set the <see cref="SiActionNodeEx_t"/> child node.
  /// </summary>
  /// <param name="child">The child node.</param>
  template <typename Ty_> void PutChild(Ty_ &&child) {
    if (base_type::children) {
      delete static_cast<CActionNode *>(base_type::children);
    }
    base_type::children = child.release();
  }

  /// <summary>
  /// Set the next <see cref="SiActionNodeEx_t"/> node.
  /// </summary>
  /// <param name="next_">The next node.</param>
  template <typename Ty_> void PutNext(Ty_ &&next_) {
    if (base_type::next) {
      delete static_cast<CActionNode *>(base_type::next);
    }
    base_type::next = next_.release();
  }

#if defined(_MSC_EXTENSIONS)
  /// <summary>
  /// The <see cref="SiActionNodeEx_t"/> properties.
  /// </summary>
  __declspec(property(get = GetId, put = PutId)) std::string Id;
  __declspec(property(get = GetDescription, put = PutDescription)) std::string Description;
  __declspec(property(get = GetLabel, put = PutLabel)) std::string Label;
  __declspec(property(get = GetType, put = PutType)) SiActionNodeType_t Type;
#endif

  void PutType(const SiActionNodeType_t value) {
    base_type::type = value;
  }

  void PutId(std::string value) {
    m_id = std::move(value);
    base_type::id = m_id.c_str();
  }

  void PutLabel(std::string value) {
    m_label = std::move(value);
    base_type::label = m_label.c_str();
  }

  void PutDescription(std::string value) {
    m_description = std::move(value);
    base_type::description = m_description.c_str();
  }

  CActionNode *DetachChild() {
    CActionNode *p = static_cast<CActionNode *>(base_type::children);
    base_type::children = nullptr;
    return p;
  }

  CActionNode *DetachNext() {
    CActionNode *p = static_cast<CActionNode *>(base_type::next);
    base_type::next = nullptr;
    return p;
  }

  CActionNode *GetChild() {
    return static_cast<CActionNode *>(base_type::children);
  }

  const CActionNode *GetChild() const {
    return static_cast<const CActionNode *>(base_type::children);
  }

  CActionNode *GetNext() {
    return static_cast<CActionNode *>(base_type::next);
  }

  const CActionNode *GetNext() const {
    return static_cast<const CActionNode *>(base_type::next);
  }

  std::string GetId() const {
    return m_id;
  }

  std::string GetLabel() const {
    return m_label;
  }

  std::string GetDescription() const {
    return m_description;
  }

  SiActionNodeType_t GetType() const {
    return base_type::type;
  }

  const SiActionNodeEx_t &GetSiActionNode() const {
    return *this;
  }

  /// <summary>
  /// Clears this and the linked nodes.
  /// </summary>
  void clear() {
    base_type::id = base_type::label = base_type::description = nullptr;
    m_id.clear();
    m_label.clear();
    m_description.clear();
    Tidy();
  }

  /// <summary>
  /// Returns true if the node is empty and has no linked nodes
  /// </summary>
  /// <returns></returns>
  bool empty() const {
    return m_id.empty() && base_type::next == nullptr && base_type::children == nullptr &&
           m_label.empty() && m_description.empty();
  }

private:
  void AssignBaseDataValues() {
    base_type::id = !m_id.empty() ? m_id.c_str() : nullptr;
    base_type::label = !m_label.empty() ? m_label.c_str() : nullptr;
    base_type::description = !m_description.empty() ? m_description.c_str() : nullptr;
  }

  void Tidy() {
    if (base_type::next == nullptr && base_type::children == nullptr) {
      return;
    }

    CActionNode *nextNode = static_cast<CActionNode *>(base_type::next);
    CActionNode *childrenNodes = static_cast<CActionNode *>(base_type::children);

    base_type::next = nullptr;
    base_type::children = nullptr;

    // Fix to avoid a stack overflow when destructing large lists
    // This traverses to the end of the list and deletes from there
    std::vector<CActionNode *> vnodes;

    if (nextNode) {
      vnodes.push_back(nextNode);
    }

    if (childrenNodes) {
      vnodes.push_back(childrenNodes);
    }

    size_t i;
    for (i = 0; i < vnodes.size(); ++i) {
      nextNode = static_cast<CActionNode *>(vnodes[i]->next);

      childrenNodes = static_cast<CActionNode *>(vnodes[i]->children);

      if (nextNode) {
        vnodes[i]->next = nullptr;
        vnodes.push_back(nextNode);
      }

      if (childrenNodes) {
        vnodes[i]->children = nullptr;
        vnodes.push_back(childrenNodes);
      }
    }

    std::vector<CActionNode *>::reverse_iterator riter;
    for (riter = vnodes.rbegin(); riter != vnodes.rend(); ++riter) {
      delete (*riter);
    }
  }

private:
  std::string m_id;
  std::string m_label;
  std::string m_description;
};
} // namespace TDx

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
#endif // CActionNode_HPP_INCLUDED
