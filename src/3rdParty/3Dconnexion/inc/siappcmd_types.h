#ifndef siappcmd_types_H_INCLUDED_
#define siappcmd_types_H_INCLUDED_
// <copyright file="siappcmd_types.h" company="3Dconnexion">
// -------------------------------------------------------------------------------------------------
// This file is part of the FreeCAD CAx development system.
//
// Copyright (c) 2014-2023 3Dconnexion.
//
// This source code is released under the GNU Library General Public License, (see "LICENSE").
// -------------------------------------------------------------------------------------------------
// </copyright>
// <description>
// *************************************************************************************************
// File Description
//
// This header file describes the variable types used in the 3dconnexion interface that allows a
// user to assign an arbitrary action to a 3dconnexion device button.
//
// Data structures are described in detail below.
//
// *************************************************************************************************
// </description>
#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && (_MSC_VER<1600)
  typedef unsigned __int32 uint32_t;
  typedef unsigned __int8  uint8_t;
#if _WIN64
  typedef unsigned __int64    uintptr_t;
#else
  typedef unsigned int uintptr_t;
#endif
#else
#include <stdint.h>
#include <wchar.h>
#endif

  typedef enum siActionNodeType_e
  {
    SI_ACTIONSET_NODE = 0
    , SI_CATEGORY_NODE
    , SI_ACTION_NODE
  } SiActionNodeType_t;

  /*------------------------------------+---------------------------------------

      SiActionNodeEx_t

      The application passes a pointer to a structure of type SiActionNodeEx_t to
      the function SiAppCmdWriteActionSet

      A set of actions is composed of a linked list of SiActionNodeEx_t structures.
      Sibling nodes are linked by the next field of the structure and child nodes
      by the children field. The root node of the tree represents the name of the
      action set while the leaf nodes of the tree represent the actions that can be
      assigned to buttons and invoked by the user. The intermediate nodes represent
      categories and sub-categories for the actions. An example of this would be the
      menu item structure in a menu bar. The menus in the menu bar would be
      represented by the SiActionNodeEx_t structures with type SI_CATEGORY_NODE pointed
      to by each successively linked next field and the first menu item of each menu
      represented by the structure pointed to by their child fields (the rest of the
      menu items in each menu would again be linked by the next fields).

      size
          The size field must always be the byte size of siActionNodeEx_s

      type
          The type field specifies one of the following values.
              SI_ACTIONSET_NODE
              SI_CATEGORY_NODE
              SI_ACTION_NODE

          The root node (and only the root node) of the tree always has type
          SI_ACTIONSET_NODE. Only the leaf nodes of the tree have type SI_ACTION_NODE.
          All intermediate nodes have type SI_CATEGORY_NODE.

      id
          The id field specifies a UTF8 string identifier for the action set,
          category, or action represented by the node. The field is always non-NULL.
          This string needs to remain constant across application sessions and more
          or less constant across application releases. The id is used by the
          application to identify an action.

      label
          The label field specifies a UTF8 localized/internationalized name
          for the action set, category, or action represented by the node. The label
          field can be NULL for the root and intermediate category nodes that are not
          explicitly presented to users. All leaf (action) and intermediate nodes
          containing leaf nodes have non-NULL labels. If the application only has a
          single action tree set, then the label of the root (context) node can also
          be NULL.

      description
          The description field specifies a UTF8 localized/internationalized tooltip
          for the action set, category, or action represented by the node. The description
          field can be NULL for the root and intermediate category nodes that are not
          explicitly presented to users. Leaf (action) nodes should have non-NULL descriptions.

  --------------------------------------+-------------------------------------*/
  typedef struct siActionNodeEx_s
  {
    uint32_t                    size;
    SiActionNodeType_t          type;
    struct siActionNodeEx_s     *next;
    struct siActionNodeEx_s     *children;
    const char                  *id;
    const char                  *label;
    const char                  *description;
  } SiActionNodeEx_t;

  /*------------------------------------+---------------------------------------

        SiImage_t

        The application passes a pointer to an array of type SiImage_t to
        the function SiAppCmdWriteActionImages

        size
            The size field specifies the size of the SiImage_t type in bytes.

        id
            The id field specifies a UTF8 string identifier for the image. The field
            is always non-NULL. This string needs to remain constant across application
            sessions and more or less constant across application releases.
            The id is used by the application to identify the image. To associate an
            image with a command the id needs to be identical to the value of the
            SiActionNodeEx_t::id of the action.

        siImageData_s::size
            The siImageData_s::size field specifies the size of the data pointed to
            by the siImageData_s::data field in bytes.

        siImageData_s::data
            The image field contains a pointer to the image. The image may be in coded
            in any recognizable format.

  --------------------------------------+-------------------------------------*/
  typedef enum eSiImageType {
    e_none = 0
    , e_image_file
    , e_resource_file
    , e_image
    , e_glyph
  } SiImageType_t;

  struct siResource_s {
    const char         *file_name;
    const char         *id;
    const char         *type;
    uint32_t            index;
  };

  struct siImageFile_s {
    const char         *file_name;
    uint32_t            index;
  };

  struct siImageData_s {
    const uint8_t      *data;
    uintptr_t           size;
    uint32_t            index;
  };

  struct siImageGlyph_s {
    const char*         font_family;
    const wchar_t*      glyphs;
    const uint32_t*     reserved1;
    uint32_t            reserved2;
  };

  typedef struct siImage_s
  {
    uint32_t                size;
    SiImageType_t           type;
    const char             *id;
    union {
      struct siResource_s   resource;
      struct siImageFile_s  file;
      struct siImageData_s  image;
      struct siImageGlyph_s glyph;
      };
  } SiImage_t;

#ifdef __cplusplus
} // extern "C"
#endif

#endif  /* siappcmd_types_H_INCLUDED_  */